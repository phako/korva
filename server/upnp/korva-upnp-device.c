/*
    This file is part of Korva.

    Copyright (C) 2012 Openismus GmbH.
    Author: Jens Georg <jensg@openismus.com>

    Korva is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Korva is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Korva.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "korva-upnp-device.h"

#define AV_TRANSPORT "urn:schemas-upnp-org:service:AVTransport"
#define CONNECTION_MANAGER "urn:schemas-upnp-org:service:ConnectionManager"

static void
korva_upnp_device_async_initable_init (GAsyncInitableIface *iface);

static void
korva_upnp_device_korva_device_init (KorvaDeviceInterface *iface);

static GRegex *media_server_regex;
static GRegex *media_renderer_regex;

GQuark
korva_upnp_device_error_quark ()
{
    return g_quark_from_static_string ("korva-upnp-device-error");
}

G_DEFINE_TYPE_WITH_CODE (KorvaUPnPDevice,
                         korva_upnp_device,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_INITABLE,
                                                korva_upnp_device_async_initable_init)
                         G_IMPLEMENT_INTERFACE (KORVA_TYPE_DEVICE,
                                                korva_upnp_device_korva_device_init))

/* forward declarations */

/* KorvaUPnPDevice */
static void
korva_upnp_device_introspect_renderer (KorvaUPnPDevice *self);

static void
korva_upnp_device_on_get_protocol_info (GUPnPServiceProxy       *proxy,
                                        GUPnPServiceProxyAction *action,
                                        gpointer                 user_data);

/* GAsyncInitable */
static void
korva_upnp_device_init_async (GAsyncInitable      *initable,
                              int                  io_priority,
                              GCancellable        *cancellable,
                              GAsyncReadyCallback  callback,
                              gpointer             user_data);

static gboolean
korva_upnp_device_init_finish (GAsyncInitable      *initable,
                               GAsyncResult        *res,
                               GError             **error);

/* KorvaDevice functions */
static const char *
korva_upnp_device_get_uid (KorvaDevice *device);

static const char *
korva_upnp_device_get_display_name (KorvaDevice *device);

static const char *
korva_upnp_device_get_icon_uri (KorvaDevice *device);

static KorvaDeviceProtocol
korva_upnp_device_get_protocol (KorvaDevice *device);

static KorvaDeviceType
korva_upnp_device_get_device_type (KorvaDevice *device);


static GVariant *
korva_upnp_device_serialize (KorvaDevice *device);

struct _KorvaUPnPDevicePrivate {
    GUPnPDeviceProxy          *proxy;
    char                      *udn;
    char                      *friendly_name;
    KorvaDeviceType            device_type;
    GSimpleAsyncResult        *result;
    GHashTable                *services;
    GUPnPServiceIntrospection *introspection;
    char                      *protocol_info;
};

enum Properties {
    PROP_0,
    PROP_PROXY
};

static void
korva_upnp_device_init (KorvaUPnPDevice *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                              KORVA_TYPE_UPNP_DEVICE,
                                              KorvaUPnPDevicePrivate);
    self->priv->services = g_hash_table_new_full (g_str_hash,
                                                  g_str_equal,
                                                  NULL,
                                                  g_object_unref);
}

static void
korva_upnp_device_dispose (GObject *obj)
{
    KorvaUPnPDevice *self = KORVA_UPNP_DEVICE (obj);

    if (self->priv->proxy != NULL) {
        g_object_unref (self->priv->proxy);
        self->priv->proxy = NULL;
    }

    if (self->priv->services != NULL) {
        g_hash_table_destroy (self->priv->services);
        self->priv->services = NULL;
    }

    G_OBJECT_CLASS (korva_upnp_device_parent_class)->dispose (obj);
}

static void
korva_upnp_device_finalize (GObject *obj)
{
    KorvaUPnPDevice *self = KORVA_UPNP_DEVICE (obj);

    if (self->priv->protocol_info != NULL) {
        g_free (self->priv->protocol_info);
        self->priv->protocol_info = NULL;
    }

    G_OBJECT_CLASS (korva_upnp_device_parent_class)->finalize (obj);
}

static void
korva_upnp_device_set_property (GObject      *obj,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
    KorvaUPnPDevice *self = KORVA_UPNP_DEVICE (obj);

    switch (property_id) {
        case PROP_PROXY:
            self->priv->proxy = g_value_get_object (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
    }
}

static void
korva_upnp_device_get_property (GObject      *obj,
                                guint         property_id,
                                GValue *value,
                                GParamSpec   *pspec)
{
    KorvaUPnPDevice *self = KORVA_UPNP_DEVICE (obj);

    switch (property_id) {
        case PROP_PROXY:
            g_value_set_object (value, self->priv->proxy);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
    }
}

static void
korva_upnp_device_class_init (KorvaUPnPDeviceClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    media_server_regex = g_regex_new ("MediaServer:\\d+$",
                                      G_REGEX_OPTIMIZE,
                                      0,
                                      NULL);
    media_renderer_regex = g_regex_new ("MediaRenderer:\\d+$",
                                        G_REGEX_OPTIMIZE,
                                        0,
                                        NULL);

    g_type_class_add_private (klass, sizeof (KorvaUPnPDevicePrivate));
    
    object_class->dispose = korva_upnp_device_dispose;
    object_class->finalize = korva_upnp_device_finalize;
    object_class->set_property = korva_upnp_device_set_property;
    object_class->get_property = korva_upnp_device_get_property;

    g_object_class_install_property (object_class,
                                     PROP_PROXY,
                                     g_param_spec_object ("proxy",
                                                          "proxy",
                                                          "proxy",
                                                          GUPNP_TYPE_DEVICE_PROXY,
                                                          G_PARAM_CONSTRUCT_ONLY |
                                                          G_PARAM_READWRITE | 
                                                          G_PARAM_STATIC_BLURB |
                                                          G_PARAM_STATIC_NAME |
                                                          G_PARAM_STATIC_NICK));
}

/* KorvaDevice functions implementation */

static void
korva_upnp_device_korva_device_init (KorvaDeviceInterface *iface)
{
    iface->get_uid = korva_upnp_device_get_uid;
    iface->get_display_name = korva_upnp_device_get_display_name;
    iface->get_icon_uri = korva_upnp_device_get_icon_uri;
    iface->get_protocol = korva_upnp_device_get_protocol;
    iface->get_device_type = korva_upnp_device_get_device_type;
    iface->serialize = korva_upnp_device_serialize;
}

static const char *
korva_upnp_device_get_uid (KorvaDevice *device)
{
    KorvaUPnPDevice *self = KORVA_UPNP_DEVICE (device);

    return self->priv->udn;
}

static const char *
korva_upnp_device_get_display_name (KorvaDevice *device)
{
    KorvaUPnPDevice *self = KORVA_UPNP_DEVICE (device);

    return self->priv->friendly_name;
}

static const char *
korva_upnp_device_get_icon_uri (KorvaDevice *device)
{
    return NULL;
}

static KorvaDeviceProtocol
korva_upnp_device_get_protocol (KorvaDevice *device)
{
    return DEVICE_PROTOCOL_UPNP;
}

static KorvaDeviceType
korva_upnp_device_get_device_type (KorvaDevice *device)
{
    KorvaUPnPDevice *self = KORVA_UPNP_DEVICE (device);

    return self->priv->device_type;
}

static GVariant *
korva_upnp_device_serialize (KorvaDevice *device)
{
    KorvaUPnPDevice *self = KORVA_UPNP_DEVICE (device);
    GVariantBuilder *builder;

    builder = g_variant_builder_new (G_VARIANT_TYPE_ARRAY);
    g_variant_builder_add (builder,
                           "{sv}",
                           "UID",
                           g_variant_new_string (self->priv->udn),
                           NULL);
    g_variant_builder_add (builder,
                           "{sv}",
                           "DisplayName",
                           g_variant_new_string (self->priv->friendly_name));
    g_variant_builder_add (builder,
                           "{sv}",
                           "IconURI",
                           g_variant_new_string (""));
    g_variant_builder_add (builder,
                           "{sv}",
                           "Protocol",
                           g_variant_new_string ("UPnP"));
    g_variant_builder_add (builder,
                           "{sv}",
                           "Type",
                           g_variant_new_uint32 ((int) self->priv->device_type));
    return g_variant_builder_end (builder);
}

/* GASyncableInit functions */
static void
korva_upnp_device_async_initable_init (GAsyncInitableIface *iface)
{
    iface->init_async = korva_upnp_device_init_async;
    iface->init_finish = korva_upnp_device_init_finish;
}

static void
korva_upnp_device_init_async (GAsyncInitable      *initable,
                              int                  io_priority,
                              GCancellable        *cancellable,
                              GAsyncReadyCallback  callback,
                              gpointer             user_data)
{
    GSimpleAsyncResult *result;
    KorvaUPnPDevice *self;
    GUPnPDeviceInfo *info;
    const char *device_type;

    self = KORVA_UPNP_DEVICE (initable);
    info = GUPNP_DEVICE_INFO (self->priv->proxy);

    self->priv->friendly_name = gupnp_device_info_get_friendly_name (info);
    self->priv->udn = g_strdup (gupnp_device_info_get_udn (info));
    device_type = gupnp_device_info_get_device_type (info);
    result = g_simple_async_result_new (G_OBJECT (self),
                                        callback,
                                        user_data,
                                        korva_upnp_device_init_async);
    g_simple_async_result_set_op_res_gboolean (result, FALSE);
    self->priv->result = result;

    if (g_regex_match (media_server_regex, device_type, 0, NULL)) {
        self->priv->device_type = DEVICE_TYPE_SERVER;
    } else if (g_regex_match (media_renderer_regex, device_type, 0, NULL)) {
        self->priv->device_type = DEVICE_TYPE_PLAYER;
        korva_upnp_device_introspect_renderer (self);
    } else {
        GError *error;

        error = g_error_new (KORVA_UPNP_DEVICE_ERROR,
                             INVALID_DEVICE_TYPE,
                             "Device %s is not the device we are looking for",
                             self->priv->udn);

        g_simple_async_result_set_from_error (result, error);
        g_simple_async_result_complete_in_idle (result);
        g_object_unref (result);
                                             
        return;
    }
}

static gboolean
korva_upnp_device_init_finish (GAsyncInitable      *initable,
                               GAsyncResult        *res,
                               GError             **error)
{
    GSimpleAsyncResult *result;
    gboolean success = TRUE;

    if (!g_simple_async_result_is_valid (res,
                                         G_OBJECT (initable),
                                         korva_upnp_device_init_async)) {
        return FALSE;
    }

    result = (GSimpleAsyncResult *) res;
    if (g_simple_async_result_propagate_error (result, error)) {
        success = FALSE;
    }

    return success;
}

static void
korva_upnp_device_introspect_renderer (KorvaUPnPDevice *self)
{
    GUPnPDeviceInfo *info;
    GUPnPServiceInfo *service;

    info = GUPNP_DEVICE_INFO (self->priv->proxy);
    g_debug ("Starting introspection of device %s (%s)",
             self->priv->friendly_name,
             self->priv->udn);

    /* check if we have all the necessary proxys */
    service = gupnp_device_info_get_service (info, AV_TRANSPORT);
    if (service == NULL) {
        GError *error;

        error = g_error_new (KORVA_UPNP_DEVICE_ERROR,
                             MISSING_SERVICE,
                             "Device %s is missing the 'AVTransport' service",
                             self->priv->udn);

                             
        g_simple_async_result_take_error (self->priv->result, error);
        g_simple_async_result_complete_in_idle (self->priv->result);
        g_object_unref (self->priv->result);

        return;
    }
    g_hash_table_insert (self->priv->services,
                         (char *)AV_TRANSPORT,
                         GUPNP_SERVICE_PROXY (service));

    service = gupnp_device_info_get_service (info, CONNECTION_MANAGER);
    if (service == NULL) {
        GError *error;

        error = g_error_new (KORVA_UPNP_DEVICE_ERROR,
                             MISSING_SERVICE,
                             "Device %s is missing the 'ConnectionManager' service",
                             self->priv->udn);
                             
        g_simple_async_result_take_error (self->priv->result, error);
        g_simple_async_result_complete_in_idle (self->priv->result);
        g_object_unref (self->priv->result);

        return;
    }
    g_hash_table_insert (self->priv->services,
                         (char *)CONNECTION_MANAGER,
                         GUPNP_SERVICE_PROXY (service));

    gupnp_service_proxy_begin_action (GUPNP_SERVICE_PROXY (service),
                                      "GetProtocolInfo",
                                      korva_upnp_device_on_get_protocol_info,
                                      self,
                                      NULL);
}

static void
korva_upnp_device_on_get_protocol_info (GUPnPServiceProxy       *proxy,
                                        GUPnPServiceProxyAction *action,
                                        gpointer                 user_data)
{
    KorvaUPnPDevice *self = KORVA_UPNP_DEVICE (user_data);
    GError *error = NULL;
    const char *variable;

    variable = self->priv->device_type == DEVICE_TYPE_PLAYER ? "Sink" : "Source";

    gupnp_service_proxy_end_action (proxy,
                                    action,
                                    &error,
                                    variable,
                                    G_TYPE_STRING,
                                    &self->priv->protocol_info,
                                    NULL);

    if (self->priv->protocol_info == NULL && error == NULL) {
        error = g_error_new (KORVA_UPNP_DEVICE_ERROR,
                             MISSING_SERVICE,
                             "Device %s did not properly reply to GetProtocolInfo call",
                             self->priv->udn);
    }
    
    if (error != NULL) {
        g_simple_async_result_take_error (self->priv->result, error);
    } else {
        g_simple_async_result_set_op_res_gboolean (self->priv->result,
                                                   TRUE);
    }

    g_simple_async_result_complete_in_idle (self->priv->result);
    g_object_unref (self->priv->result);
}
