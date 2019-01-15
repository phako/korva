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

#define G_LOG_DOMAIN "Korva-UPnP-Device"

#include <libsoup/soup.h>
#include <libgupnp-av/gupnp-av.h>

#include <korva-error.h>
#include <korva-icon-cache.h>

#include "korva-upnp-device.h"
#include "korva-upnp-file-server.h"

#define AV_TRANSPORT "urn:schemas-upnp-org:service:AVTransport"
#define CONNECTION_MANAGER "urn:schemas-upnp-org:service:ConnectionManager"

#define AV_STATE_STOPPED "STOPPED"
#define AV_STATE_PAUSED_PLAYBACK "PAUSED_PLAYBACK"
#define AV_STATE_NO_MEDIA_PRESENT "NO_MEDIA_PRESENT"
#define AV_STATE_PLAYING "PLAYING"
#define AV_STATE_UNKNOWN "UNKNOWN"

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
korva_upnp_device_introspection_finish (KorvaUPnPDevice *self,
                                        GError          *error);

static void
korva_upnp_device_on_last_change (GUPnPServiceProxy *proxy,
                                  const char        *variable,
                                  GValue            *value,
                                  gpointer           user_data);

#define korva_upnp_device_introspection_success(self) \
    korva_upnp_device_introspection_finish ((self), NULL)

static void
korva_upnp_device_on_get_protocol_info (GUPnPServiceProxy       *proxy,
                                        GUPnPServiceProxyAction *action,
                                        gpointer                 user_data);

static void
korva_upnp_device_get_icon (KorvaUPnPDevice *self);

static void
korva_upnp_device_on_icon_ready (SoupMessage *message, gpointer user_data);

static void
korva_upnp_device_on_set_av_transport_uri (GUPnPServiceProxy       *proxy,
                                           GUPnPServiceProxyAction *action,
                                           gpointer                 user_data);
static void
korva_upnp_device_on_play (GUPnPServiceProxy       *proxy,
                           GUPnPServiceProxyAction *action,
                           gpointer                 user_data);
static void
korva_upnp_device_on_stop (GUPnPServiceProxy       *proxy,
                           GUPnPServiceProxyAction *action,
                           gpointer                 user_data);

static void
korva_upnp_device_update_ip_address (KorvaUPnPDevice *self);

static void
korva_upnp_device_on_get_transport_info (GUPnPServiceProxy       *proxy,
                                         GUPnPServiceProxyAction *action,
                                         gpointer                 user_data);

static void
korva_upnp_device_drop_current_file (KorvaUPnPDevice *self);

/* GAsyncInitable */
static void
korva_upnp_device_init_async (GAsyncInitable     *initable,
                              int                 io_priority,
                              GCancellable       *cancellable,
                              GAsyncReadyCallback callback,
                              gpointer            user_data);

static gboolean
korva_upnp_device_init_finish (GAsyncInitable *initable,
                               GAsyncResult   *res,
                               GError        **error);

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

static void
korva_upnp_device_push_async (KorvaDevice        *self,
                              GVariant           *source,
                              GCancellable       *cancellable,
                              GAsyncReadyCallback callback,
                              gpointer            user_data);

static char *
korva_upnp_device_push_finish (KorvaDevice  *self,
                               GAsyncResult *result,
                               GError      **error);

static void
korva_upnp_device_unshare_async (KorvaDevice        *self,
                                 const char         *tag,
                                 GCancellable       *cancellable,
                                 GAsyncReadyCallback callback,
                                 gpointer            user_data);

static gboolean
korva_upnp_device_unshare_finish (KorvaDevice  *self,
                                  GAsyncResult *result,
                                  GError      **error);

struct _KorvaUPnPDevicePrivate {
    union {
        GUPnPDeviceProxy *proxy;
        GUPnPDeviceInfo  *info;
    };

    char                      *udn;
    char                      *friendly_name;
    char                      *icon_uri;
    KorvaDeviceType            device_type;
    GTask                     *result;
    GHashTable                *services;
    GUPnPServiceIntrospection *introspection;
    char                      *protocol_info;
    SoupSession               *session;
    GList                     *other_proxies;
    GUPnPLastChangeParser     *last_change_parser;
    char                      *state;
    char                      *ip_address;
    char                      *current_tag;
    char                      *current_uri;
    GFile                     *current_file;
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
    self->priv->session = soup_session_new ();
    self->priv->last_change_parser = gupnp_last_change_parser_new ();
    self->priv->state = g_strdup (AV_STATE_UNKNOWN);
}

static void proxy_list_free (GList *proxies)
{
    g_list_free_full (proxies, g_object_unref);
}

static void
korva_upnp_device_dispose (GObject *obj)
{
    KorvaUPnPDevice *self = KORVA_UPNP_DEVICE (obj);

    g_clear_object (&self->priv->proxy);
    g_clear_pointer (&self->priv->services, g_hash_table_destroy);
    g_clear_object (&self->priv->session);
    g_clear_pointer (&self->priv->other_proxies, proxy_list_free);
    g_clear_object (&self->priv->last_change_parser);
    g_clear_object (&self->priv->last_change_parser);

    G_OBJECT_CLASS (korva_upnp_device_parent_class)->dispose (obj);
}

static void
korva_upnp_device_finalize (GObject *obj)
{
    KorvaUPnPDevice *self = KORVA_UPNP_DEVICE (obj);

    g_clear_pointer (&self->priv->protocol_info, g_free);
    g_clear_pointer (&self->priv->ip_address, g_free);
    g_clear_pointer (&self->priv->current_tag, g_free);
    g_clear_pointer (&self->priv->current_uri, g_free);

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

    g_object_class_install_property (object_class,
                                     PROP_PROXY,
                                     g_param_spec_object ("proxy",
                                                          "proxy",
                                                          "proxy",
                                                          GUPNP_TYPE_DEVICE_PROXY,
                                                          G_PARAM_CONSTRUCT_ONLY |
                                                          G_PARAM_WRITABLE |
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
    iface->push_async = korva_upnp_device_push_async;
    iface->push_finish = korva_upnp_device_push_finish;
    iface->unshare_async = korva_upnp_device_unshare_async;
    iface->unshare_finish = korva_upnp_device_unshare_finish;
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
    KorvaUPnPDevice *self = KORVA_UPNP_DEVICE (device);

    return self->priv->icon_uri;
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
                           g_variant_new_string (self->priv->icon_uri ? self->priv->icon_uri : ""));
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
korva_upnp_device_init_async (GAsyncInitable     *initable,
                              int                 io_priority,
                              GCancellable       *cancellable,
                              GAsyncReadyCallback callback,
                              gpointer            user_data)
{
    GTask *result;
    KorvaUPnPDevice *self;
    const char *device_type;

    self = KORVA_UPNP_DEVICE (initable);

    self->priv->friendly_name = gupnp_device_info_get_friendly_name (self->priv->info);
    self->priv->udn = g_strdup (gupnp_device_info_get_udn (self->priv->info));
    self->priv->icon_uri = korva_icon_cache_lookup (self->priv->udn);

    device_type = gupnp_device_info_get_device_type (self->priv->info);
    result = g_task_new (self, cancellable, callback, user_data);
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
        korva_upnp_device_introspection_finish (self, error);

        return;
    }
}

static gboolean
korva_upnp_device_init_finish (GAsyncInitable *initable,
                               GAsyncResult   *res,
                               GError        **error)
{
    g_return_val_if_fail (g_task_is_valid (res, initable), FALSE);

    return g_task_propagate_boolean (G_TASK (res), error);
}

static void
korva_upnp_device_introspect_renderer (KorvaUPnPDevice *self)
{
    GUPnPDeviceInfo *info;
    GUPnPServiceInfo *service;
    GUPnPServiceProxy *proxy;

    info = GUPNP_DEVICE_INFO (self->priv->proxy);
    g_debug ("Starting introspection of rendering device %s (%s)",
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

        korva_upnp_device_introspection_finish (self, error);

        return;
    }

    g_hash_table_insert (self->priv->services,
                         (char *) AV_TRANSPORT,
                         GUPNP_SERVICE_PROXY (service));
    gupnp_service_proxy_add_notify (GUPNP_SERVICE_PROXY (service),
                                    "LastChange", G_TYPE_STRING,
                                    korva_upnp_device_on_last_change,
                                    self);
    gupnp_service_proxy_set_subscribed (GUPNP_SERVICE_PROXY (service), TRUE);


    service = gupnp_device_info_get_service (info, CONNECTION_MANAGER);
    if (service == NULL) {
        GError *error;

        error = g_error_new (KORVA_UPNP_DEVICE_ERROR,
                             MISSING_SERVICE,
                             "Device %s is missing the 'ConnectionManager' service",
                             self->priv->udn);

        korva_upnp_device_introspection_finish (self, error);

        return;
    }

    /*
     * This is usually an IP address; If not, the comparison later should work
     * nevertheless.
     */
    korva_upnp_device_update_ip_address (self);

    g_hash_table_insert (self->priv->services,
                         (char *) CONNECTION_MANAGER,
                         GUPNP_SERVICE_PROXY (service));

    proxy = g_hash_table_lookup (self->priv->services, AV_TRANSPORT);
    gupnp_service_proxy_begin_action (proxy,
                                      "GetTransportInfo",
                                      korva_upnp_device_on_get_transport_info,
                                      self,
                                      "InstanceID", G_TYPE_UINT, 0,
                                      NULL);
}

static void
korva_upnp_device_on_get_transport_info (GUPnPServiceProxy       *proxy,
                                         GUPnPServiceProxyAction *action,
                                         gpointer                 user_data)
{
    GUPnPServiceProxy *cm_proxy;
    GError *error = NULL;
    KorvaUPnPDevice *self = KORVA_UPNP_DEVICE (user_data);

    gupnp_service_proxy_end_action (proxy,
                                    action,
                                    &error,
                                    "CurrentTransportState", G_TYPE_STRING, &(self->priv->state),
                                    NULL);
    if (error != NULL) {
        GError *inner_error;

        inner_error = g_error_new (KORVA_UPNP_DEVICE_ERROR,
                                   MISSING_SERVICE,
                                   "Call to 'GetTransportInfo' on device %s failed: %s",
                                   self->priv->udn,
                                   error->message);
        g_error_free (error);

        korva_upnp_device_introspection_finish (self, inner_error);

        return;
    }

    g_debug ("Device %s has state %s", self->priv->udn, self->priv->state);

    cm_proxy = g_hash_table_lookup (self->priv->services, CONNECTION_MANAGER);
    gupnp_service_proxy_begin_action (cm_proxy,
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
        korva_upnp_device_introspection_finish (self, error);
    } else {
        korva_upnp_device_get_icon (self);
    }
}

static void
korva_upnp_device_introspection_finish (KorvaUPnPDevice *self,
                                        GError          *error)
{
    if (error != NULL) {
        g_task_return_error (self->priv->result, error);
    } else {
        g_task_return_boolean (self->priv->result, TRUE);
    }
    g_object_unref (self->priv->result);
}

static void
korva_upnp_device_get_icon (KorvaUPnPDevice *self)
{
    char *uri;

    /* icon was already in cache */
    if (self->priv->icon_uri != NULL) {
        korva_upnp_device_introspection_success (self);

        return;
    }

    /* First try with PNG (for transparency) */
    uri = gupnp_device_info_get_icon_url (self->priv->info,
                                          "image/png",
                                          -1,
                                          64,
                                          64,
                                          TRUE,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL);

    if (uri == NULL) {
        uri = gupnp_device_info_get_icon_url (self->priv->info,
                                              "image/jpeg",
                                              -1,
                                              64,
                                              64,
                                              TRUE,
                                              NULL,
                                              NULL,
                                              NULL,
                                              NULL);
    }

    if (uri == NULL) {
        self->priv->icon_uri = korva_icon_cache_get_default (self->priv->device_type);
        korva_upnp_device_introspection_success (self);
    } else {
        SoupMessage *message;

        message = soup_message_new (SOUP_METHOD_GET, uri);
        g_signal_connect (G_OBJECT (message),
                          "finished",
                          G_CALLBACK (korva_upnp_device_on_icon_ready),
                          self);
        soup_session_queue_message (self->priv->session,
                                    message,
                                    NULL,
                                    NULL);
    }
}

static void
korva_upnp_device_on_icon_ready (SoupMessage *message, gpointer user_data)
{
    KorvaUPnPDevice *self = KORVA_UPNP_DEVICE (user_data);
    int status_code = 0;
    char *reason = NULL;
    GError *error = NULL;

    g_object_get (message,
                  SOUP_MESSAGE_STATUS_CODE, &status_code,
                  SOUP_MESSAGE_REASON_PHRASE, &reason,
                  NULL);

    if (status_code == SOUP_STATUS_FOUND || status_code == SOUP_STATUS_OK) {
        char *path;
        GFile *file;

        path = korva_icon_cache_create_path (self->priv->udn);
        file = g_file_new_for_path (path);
        g_free (path);

        g_file_replace_contents (file,
                                 message->response_body->data,
                                 message->response_body->length,
                                 NULL,
                                 FALSE,
                                 G_FILE_CREATE_NONE,
                                 NULL,
                                 NULL,
                                 &error);
        if (error == NULL) {
            self->priv->icon_uri = g_file_get_uri (file);
        } else {
            g_debug ("Could not write icon %s: %s", path, error->message);
            g_error_free (error);
        }

        g_object_unref (file);
    } else {
        g_debug ("Failed to download icon: %s", reason);
    }

    if (self->priv->icon_uri == NULL) {
        self->priv->icon_uri = korva_icon_cache_get_default (self->priv->device_type);
    }

    korva_upnp_device_introspection_finish (self, NULL);
}

static void
korva_upnp_device_on_last_change (GUPnPServiceProxy *proxy,
                                  const char        *variable,
                                  GValue            *value,
                                  gpointer           user_data)
{
    KorvaUPnPDevice *self = KORVA_UPNP_DEVICE (user_data);
    char *status = NULL;
    char *uri = NULL;
    gboolean result = FALSE;
    GError *error = NULL;

    result = gupnp_last_change_parser_parse_last_change (self->priv->last_change_parser,
                                                         0,
                                                         g_value_get_string (value),
                                                         &error,
                                                         "TransportState", G_TYPE_STRING, &status,
                                                         "AVTransportURI", G_TYPE_STRING, &uri,
                                                         NULL);

    if (!result) {
        g_warning ("Failed to parse LastState: %s", error->message);
        g_error_free (error);

        return;
    }

    if (status != NULL) {
        if (self->priv->state != NULL) {
            if (g_ascii_strcasecmp (self->priv->state, status) == 0) {
                g_free (status);

                return;
            }
            g_free (self->priv->state);
        }
        self->priv->state = status;
        g_debug ("Device %s has new state '%s'", self->priv->udn, self->priv->state);
    }

    if (uri != NULL &&
        self->priv->current_uri != NULL &&
        g_strcmp0 (uri, self->priv->current_uri) != 0) {

        g_debug ("Device has been modified externally.");
        korva_upnp_device_drop_current_file (self);
    }
}


void
korva_upnp_device_add_proxy (KorvaUPnPDevice *self, GUPnPDeviceProxy *proxy)
{
    self->priv->other_proxies = g_list_prepend (self->priv->other_proxies,
                                                g_object_ref (proxy));
}

gboolean
korva_upnp_device_remove_proxy (KorvaUPnPDevice *self, GUPnPDeviceProxy *proxy)
{
    GList *it, *keys = NULL;
    KorvaUPnPFileServer *server;

    it = g_list_find (self->priv->other_proxies, proxy);
    if (it != NULL) {
        self->priv->other_proxies = g_list_remove_link (self->priv->other_proxies,
                                                        it);
        g_object_unref (G_OBJECT (it->data));

        return FALSE;
    }

    if (self->priv->proxy != proxy) {
        g_warning ("Trying to remove unassociated proxy from device");

        return FALSE;
    }

    server = korva_upnp_file_server_get_default ();
    korva_upnp_file_server_unhost_by_peer (server, self->priv->ip_address);
    g_object_unref (server);

    /* That's the only proxy associated with this.
     * We can destroy the device. */
    if (self->priv->other_proxies == NULL) {
        return TRUE;
    }

    /* Just use the first other proxy to communicate with the device */
    g_object_unref (self->priv->proxy);
    self->priv->proxy = GUPNP_DEVICE_PROXY (self->priv->other_proxies->data);
    self->priv->other_proxies = g_list_remove_link (self->priv->other_proxies,
                                                    self->priv->other_proxies);

    korva_upnp_device_update_ip_address (self);

    /* and update the service proxies */
    it = keys = g_hash_table_get_keys (self->priv->services);
    while (it != NULL) {
        GUPnPServiceInfo *info;

        info = gupnp_device_info_get_service (self->priv->info,
                                              (const char *) it->data);
        g_hash_table_replace (self->priv->services,
                              it->data,
                              GUPNP_SERVICE_PROXY (info));

        it = it->next;
    }

    g_list_free (keys);

    return FALSE;
}

typedef struct {
    GTask           *result;
    KorvaUPnPDevice *device;
    GHashTable      *params;
    char            *uri;
    char            *meta_data;
    gboolean         unshare;
    GFile           *file;
    gboolean         transport_locked;
} HostPathData;

static void
host_path_data_free (HostPathData *data)
{
    g_free (data->uri);
    g_free (data->meta_data);
    g_clear_object (&data->file);

    g_free (data);
}

static void
korva_upnp_device_on_play (GUPnPServiceProxy       *proxy,
                           GUPnPServiceProxyAction *action,
                           gpointer                 user_data)
{
    GError *error = NULL;
    HostPathData *data = (HostPathData *) user_data;
    GTask *result = data->result;

    gupnp_service_proxy_end_action (proxy, action, &error, NULL);
    if (error != NULL) {
        KorvaUPnPFileServer *server;

        server = korva_upnp_file_server_get_default ();
        korva_upnp_file_server_unhost_file_for_peer (server,
                                                     data->file,
                                                     data->device->priv->ip_address);
        g_object_unref (server);

        g_task_return_error (result, error);
    } else {
        char *data = g_strdup (g_task_get_task_data (result));
        g_task_return_pointer (result, data, g_free);
    }

    data->device->priv->current_uri = g_strdup (data->uri);
    data->device->priv->current_file = data->file;
    data->file = NULL;

    host_path_data_free (data);
    g_object_unref (result);
}

static void
korva_upnp_device_on_stop (GUPnPServiceProxy       *proxy,
                           GUPnPServiceProxyAction *action,
                           gpointer                 user_data)
{
    GError *error = NULL;
    HostPathData *data = (HostPathData *) user_data;
    GTask *result = data->result;

    gupnp_service_proxy_end_action (proxy, action, &error, NULL);
    if (error != NULL) {
        KorvaUPnPFileServer *server;

        g_task_return_error (result, error);

        server = korva_upnp_file_server_get_default ();
        korva_upnp_file_server_unhost_file_for_peer (server,
                                                     data->file,
                                                     data->device->priv->ip_address);
        g_object_unref (server);
    } else {
        gupnp_service_proxy_begin_action (proxy,
                                          "SetAVTransportURI",
                                          korva_upnp_device_on_set_av_transport_uri,
                                          user_data,
                                          "InstanceID", G_TYPE_STRING, "0",
                                          "CurrentURI", G_TYPE_STRING, data->uri,
                                          "CurrentURIMetaData", G_TYPE_STRING, data->meta_data,
                                          NULL);

        return;
    }

    host_path_data_free (data);
    g_object_unref (result);
}

static void
korva_upnp_device_on_set_av_transport_uri (GUPnPServiceProxy       *proxy,
                                           GUPnPServiceProxyAction *action,
                                           gpointer                 user_data)
{
    GError *error = NULL;
    HostPathData *data = (HostPathData *) user_data;
    KorvaUPnPDevice *self = data->device;
    GTask *result = data->result;

    gupnp_service_proxy_end_action (proxy, action, &error, NULL);
    if (error != NULL) {
        /* Transport locked and we didn't come from a transport_locked state already */
        if (error->code == 705 && !data->transport_locked) {
            data->transport_locked = TRUE;
            g_debug ("Transport was locked, trying to stop the device");
            gupnp_service_proxy_begin_action (proxy,
                                              "Stop",
                                              korva_upnp_device_on_stop,
                                              user_data,
                                              "InstanceID", G_TYPE_STRING, "0",
                                              NULL);
            g_error_free (error);

            return;
        }

        g_task_return_error (result, error);

        if (!data->unshare) {
            KorvaUPnPFileServer *server;

            server = korva_upnp_file_server_get_default ();
            korva_upnp_file_server_unhost_file_for_peer (server,
                                                         data->file,
                                                         data->device->priv->ip_address);
            g_object_unref (server);
        }

        goto out;
    }

    /* This was called from Unshare. We're done now */
    if (data->unshare) {
        korva_upnp_device_drop_current_file (data->device);
        g_task_return_pointer (result, NULL, NULL);

        goto out;
    }

    if (g_ascii_strcasecmp (self->priv->state, AV_STATE_STOPPED) == 0 ||
        g_ascii_strcasecmp (self->priv->state, AV_STATE_NO_MEDIA_PRESENT) == 0 ||
        g_ascii_strcasecmp (self->priv->state, AV_STATE_PAUSED_PLAYBACK) == 0) {
        gupnp_service_proxy_begin_action (proxy,
                                          "Play",
                                          korva_upnp_device_on_play,
                                          user_data,
                                          "InstanceID", G_TYPE_STRING, "0",
                                          "Speed", G_TYPE_FLOAT, 1.0f,
                                          NULL);

        return;
    }

    data->device->priv->current_uri = g_strdup (data->uri);
    data->device->priv->current_file = data->file;
    data->file = NULL;

out:
    host_path_data_free (data);
    g_object_unref (result);
}

static void
korva_upnp_device_drop_current_file (KorvaUPnPDevice *self)
{
    KorvaUPnPFileServer *server;

    server = korva_upnp_file_server_get_default ();
    if (self->priv->current_tag != NULL) {
        korva_upnp_file_server_unhost_file_for_peer (server,
                                                     self->priv->current_file,
                                                     self->priv->ip_address);

        g_clear_pointer (&self->priv->current_tag, g_free);
        g_clear_pointer (&self->priv->current_uri, g_free);
        g_clear_pointer (&self->priv->current_file, g_free);
    }

    g_object_unref (server);
}

static void
korva_upnp_device_on_host_file_async (GObject      *source,
                                      GAsyncResult *res,
                                      gpointer      user_data)
{
    GError *error = NULL;
    GHashTable *params;
    HostPathData *data = (HostPathData *) user_data;
    GUPnPServiceProxy *proxy;
    GUPnPDIDLLiteWriter *writer;
    GUPnPDIDLLiteObject *object;
    GUPnPDIDLLiteResource *resource, *compat_resource;
    GUPnPProtocolInfo *protocol_info;
    const char *title, *content_type, *dlna_profile = NULL;
    GVariant *value;
    guint64 size;

    data->uri = korva_upnp_file_server_host_file_finish (KORVA_UPNP_FILE_SERVER (source),
                                                         res,
                                                         &params,
                                                         &error);
    if (params == NULL) {
        g_task_return_error (data->result, error);

        goto out;
    }

    value = g_hash_table_lookup (params, "Title");
    title = g_variant_get_string (value, NULL);
    value = g_hash_table_lookup (params, "ContentType");
    content_type = g_variant_get_string (value, NULL);
    value = g_hash_table_lookup (params, "Size");
    size = g_variant_get_uint64 (value);
    value = g_hash_table_lookup (params, "DLNAProfile");
    if (value != NULL) {
        dlna_profile = g_variant_get_string (value, NULL);
    }

    writer = gupnp_didl_lite_writer_new (NULL);
    object = GUPNP_DIDL_LITE_OBJECT (gupnp_didl_lite_writer_add_item (writer));
    gupnp_didl_lite_object_set_title (object, title);
    gupnp_didl_lite_object_set_id (object, "1");
    gupnp_didl_lite_object_set_parent_id (object, "-1");
    gupnp_didl_lite_object_set_restricted (object, TRUE);
    resource = gupnp_didl_lite_object_add_resource (object);
    gupnp_didl_lite_resource_set_uri (resource, data->uri);
    gupnp_didl_lite_resource_set_size64 (resource, size);
    protocol_info = gupnp_protocol_info_new_from_string ("http-get:*:*:DLNA.ORG_CI=0;DLNA.ORG_OP=01", NULL);
    gupnp_protocol_info_set_mime_type (protocol_info, content_type);
    if (dlna_profile != NULL) {
        gupnp_protocol_info_set_dlna_profile (protocol_info, dlna_profile);
    }
    gupnp_didl_lite_resource_set_protocol_info (resource, protocol_info);

    compat_resource = gupnp_didl_lite_object_get_compat_resource (object,
                                                                  data->device->priv->protocol_info,
                                                                  FALSE);
    data->meta_data = gupnp_didl_lite_writer_get_string (writer);
    g_object_unref (object);
    g_object_unref (writer);

    if (compat_resource == NULL) {
        g_set_error_literal (&error,
                             KORVA_CONTROLLER1_ERROR,
                             KORVA_CONTROLLER1_ERROR_NOT_COMPATIBLE,
                             "The file is not compatible with the selected renderer");

        g_task_return_error (data->result, error);
        korva_upnp_file_server_unhost_file_for_peer (KORVA_UPNP_FILE_SERVER (source),
                                                     data->file,
                                                     data->device->priv->ip_address);

        goto out;
    }
    g_object_unref (compat_resource);

    proxy = g_hash_table_lookup (data->device->priv->services, AV_TRANSPORT);
    gupnp_service_proxy_begin_action (proxy,
                                      "SetAVTransportURI",
                                      korva_upnp_device_on_set_av_transport_uri,
                                      user_data,
                                      "InstanceID", G_TYPE_STRING, "0",
                                      "CurrentURI", G_TYPE_STRING, data->uri,
                                      "CurrentURIMetaData", G_TYPE_STRING, data->meta_data,
                                      NULL);

    return;
out:
    g_object_unref (data->result);
    host_path_data_free (data);
}

static void
korva_upnp_device_update_ip_address (KorvaUPnPDevice *self)
{
    SoupURI *location;

    g_free (self->priv->ip_address);

    location = soup_uri_new (gupnp_device_info_get_location (self->priv->info));
    self->priv->ip_address = g_strdup (soup_uri_get_host (location));
    soup_uri_free (location);
}

static void
korva_upnp_device_push_async (KorvaDevice        *device,
                              GVariant           *source,
                              GCancellable       *cancellable,
                              GAsyncReadyCallback callback,
                              gpointer            user_data)
{
    KorvaUPnPDevice *self;
    GTask *result;
    GVariantIter *iter;
    gchar *key;
    GVariant *value;
    GHashTable *params;
    GError *error = NULL;
    KorvaUPnPFileServer *server;
    GFile *file = NULL;
    GUPnPContext *context;
    GVariant *uri;
    HostPathData *host_path_data;
    const char *iface;
    char *raw_tag;

    self = KORVA_UPNP_DEVICE (device);
    result = g_task_new (device, cancellable, callback, user_data);

    params = g_hash_table_new_full (g_str_hash,
                                    g_str_equal,
                                    g_free,
                                    (GDestroyNotify) g_variant_unref);

    iter = g_variant_iter_new (source);
    while (g_variant_iter_next (iter, "{sv}", &key, &value)) {
        g_hash_table_insert (params, key, value);
    }

    uri = g_hash_table_lookup (params, "URI");
    if (uri == NULL) {

        error = g_error_new (KORVA_CONTROLLER1_ERROR,
                             KORVA_CONTROLLER1_ERROR_INVALID_ARGS,
                             "'Push' to device %s is missing mandatory URI key",
                             korva_device_get_uid (device));

        g_task_return_error (result, error);
        g_object_unref (result);

        goto out;
    }

    context = gupnp_device_info_get_context (self->priv->info);
    iface = gssdp_client_get_host_ip (GSSDP_CLIENT (context));
    server = korva_upnp_file_server_get_default ();
    file = g_file_new_for_uri (g_variant_get_string (uri, NULL));
    raw_tag = g_strconcat (g_variant_get_string (uri, NULL), "\t", self->priv->udn, NULL);

    host_path_data = g_new0 (HostPathData, 1);
    host_path_data->result = result;
    host_path_data->device = self;
    host_path_data->params = params;
    host_path_data->file = g_object_ref (file);

    g_task_set_task_data (result,
                          g_compute_checksum_for_string (G_CHECKSUM_MD5, raw_tag, -1),
                          g_free);
    g_free (raw_tag);

    korva_upnp_device_drop_current_file (self);
    korva_upnp_file_server_host_file_async (server,
                                            file,
                                            params,
                                            iface,
                                            self->priv->ip_address,
                                            cancellable,
                                            korva_upnp_device_on_host_file_async,
                                            host_path_data);
    g_object_unref (server);

    return;
out:
    g_hash_table_destroy (params);
    g_clear_object (&file);
}

static char *
korva_upnp_device_push_finish (KorvaDevice  *device,
                               GAsyncResult *res,
                               GError      **error)
{
    KorvaUPnPDevice *self;
    char *tag;

    g_return_val_if_fail (g_task_is_valid (res, device), NULL);

    tag = g_task_propagate_pointer (G_TASK (res), error);
    if (tag != NULL) {
        self = KORVA_UPNP_DEVICE (device);
        self->priv->current_tag = g_strdup (tag);

        return tag;
    }

    return NULL;
}

static void
korva_upnp_device_unshare_async (KorvaDevice        *device,
                                 const char         *tag,
                                 GCancellable       *cancellable,
                                 GAsyncReadyCallback callback,
                                 gpointer            user_data)
{
    GTask *result;
    HostPathData *data;
    KorvaUPnPDevice *self = KORVA_UPNP_DEVICE (device);
    GUPnPServiceProxy *proxy;

    result = g_task_new (device, cancellable, callback, user_data);

    if (g_strcmp0 (self->priv->current_tag, tag) != 0) {
        g_task_return_new_error (result,
                                 KORVA_CONTROLLER1_ERROR,
                                 KORVA_CONTROLLER1_ERROR_NO_SUCH_TRANSFER,
                                 "Sharing operation '%s' is not valid",
                                 tag);
        g_object_unref (result);

        return;
    }

    data = g_new0 (HostPathData, 1);
    data->result = result;
    data->uri = g_strdup ("");
    data->meta_data = g_strdup ("");
    data->device = self;
    data->unshare = TRUE;
    data->file = g_object_ref (self->priv->current_file);

    proxy = g_hash_table_lookup (data->device->priv->services, AV_TRANSPORT);

    gupnp_service_proxy_begin_action (proxy,
                                      "Stop",
                                      korva_upnp_device_on_stop,
                                      data,
                                      "InstanceID", G_TYPE_STRING, "0",
                                      NULL);
}

static gboolean
korva_upnp_device_unshare_finish (KorvaDevice  *device,
                                  GAsyncResult *res,
                                  GError      **error)
{
    g_return_val_if_fail (g_task_is_valid (res, device), FALSE);

    return g_task_propagate_boolean (G_TASK (res), error);
}
