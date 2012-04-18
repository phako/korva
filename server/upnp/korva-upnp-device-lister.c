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

#define G_LOG_DOMAIN "Korva-UPnP-Device-Lister"

#include <libgupnp/gupnp.h>

#include "korva-upnp-device-lister.h"
#include "korva-upnp-device.h"
#include "korva-upnp-file-server.h"

#include "korva-device-lister.h"

#define MEDIA_SERVER "urn:schemas-upnp-org:device:MediaServer:1"
#define MEDIA_RENDERER "urn:schemas-upnp-org:device:MediaRenderer:1"

struct _KorvaUPnPDeviceListerPrivate {
    GUPnPContextManager *context_manager;
    GHashTable          *devices;
    GHashTable          *pending_devices;
    KorvaUPnPFileServer *server;
};

static void
korva_upnp_device_lister_iface_init (gpointer, gpointer);

G_DEFINE_TYPE_EXTENDED (KorvaUPnPDeviceLister,
                        korva_upnp_device_lister,
                        G_TYPE_OBJECT,
                        0,
                        G_IMPLEMENT_INTERFACE (KORVA_TYPE_DEVICE_LISTER,
                                               korva_upnp_device_lister_iface_init))

/* Forward declarations */

/* KorvaDeviceLister interface */
static GList *
korva_upnp_device_lister_get_devices (KorvaDeviceLister *self);

static KorvaDevice *
korva_upnp_device_lister_get_device_info (KorvaDeviceLister *self, const char *uid);

static gint
korva_upnp_device_lister_get_device_count (KorvaDeviceLister *self);

static gboolean
korva_upnp_device_lister_idle (KorvaDeviceLister *lister);

/* ContextManager callbacks */
static void
korva_upnp_device_lister_on_context_available (GUPnPContextManager *cm,
                                               GUPnPContext        *context,
                                               gpointer             user_data);

/* ControlPoint callbacks */
static void
korva_upnp_device_lister_on_renderer_available (GUPnPControlPoint *cp,
                                                GUPnPDeviceProxy  *proxy,
                                                gpointer           user_data);

static void
korva_upnp_device_lister_on_renderer_unavailable (GUPnPControlPoint *cp,
                                                  GUPnPDeviceProxy  *proxy,
                                                  gpointer           user_data);

static void
korva_upnp_device_lister_iface_init (gpointer g_iface,
                                     gpointer iface_data)
{
    KorvaDeviceListerInterface *iface = (KorvaDeviceListerInterface*) g_iface;

    iface->get_devices = korva_upnp_device_lister_get_devices;
    iface->get_device_info = korva_upnp_device_lister_get_device_info;
    iface->get_device_count = korva_upnp_device_lister_get_device_count;
    iface->idle = korva_upnp_device_lister_idle;
}

static void
korva_upnp_device_lister_init (KorvaUPnPDeviceLister *self)
{
    GUPnPContextManager *cm;

    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                              KORVA_TYPE_UPNP_DEVICE_LISTER,
                                              KorvaUPnPDeviceListerPrivate);
    self->priv->devices = g_hash_table_new_full (g_str_hash,
                                                 g_str_equal,
                                                 g_free,
                                                 g_object_unref);

    self->priv->pending_devices = g_hash_table_new_full (g_str_hash,
                                                         g_str_equal,
                                                         g_free,
                                                         g_object_unref);
    self->priv->server = korva_upnp_file_server_get_default ();

    cm = gupnp_context_manager_create (0);
    self->priv->context_manager = cm;
    g_signal_connect (cm,
                      "context-available",
                      G_CALLBACK (korva_upnp_device_lister_on_context_available),
                      self);
}

static void
korva_upnp_device_lister_dispose (GObject *object)
{
    KorvaUPnPDeviceLister *self = KORVA_UPNP_DEVICE_LISTER (object);

    if (self->priv->context_manager != NULL) {
        g_object_unref (self->priv->context_manager);
        self->priv->context_manager = NULL;
    }

    if (self->priv->devices != NULL) {
        g_hash_table_destroy (self->priv->devices);
        self->priv->devices = NULL;
    }

    if (self->priv->pending_devices != NULL) {
        g_hash_table_destroy (self->priv->pending_devices);
        self->priv->pending_devices = NULL;
    }

    if (self->priv->server != NULL) {
        g_object_unref (self->priv->server);
        self->priv->server = NULL;
    }
}

static void
korva_upnp_device_lister_class_init (KorvaUPnPDeviceListerClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = korva_upnp_device_lister_dispose;

    g_type_class_add_private (klass, sizeof (KorvaUPnPDeviceListerPrivate));
}

KorvaDeviceLister *
korva_upnp_device_lister_new (void)
{
    return KORVA_DEVICE_LISTER (g_object_new (KORVA_TYPE_UPNP_DEVICE_LISTER,
                                              NULL));
}

/* KorvaDeviceLister interface implementation */
static GList *
korva_upnp_device_lister_get_devices (KorvaDeviceLister *lister)
{
    KorvaUPnPDeviceLister *self;

    g_return_val_if_fail (KORVA_IS_UPNP_DEVICE_LISTER (lister), NULL);
    self = KORVA_UPNP_DEVICE_LISTER (lister);

    return g_hash_table_get_values (self->priv->devices);
}

static KorvaDevice *
korva_upnp_device_lister_get_device_info (KorvaDeviceLister *lister,
                                          const char *uid)
{
    KorvaUPnPDeviceLister *self;

    g_return_val_if_fail (KORVA_IS_UPNP_DEVICE_LISTER (lister), 0);
    self = KORVA_UPNP_DEVICE_LISTER (lister);

    return g_hash_table_lookup (self->priv->devices, uid);
}

static gint
korva_upnp_device_lister_get_device_count (KorvaDeviceLister *lister)
{
    KorvaUPnPDeviceLister *self;

    g_return_val_if_fail (KORVA_IS_UPNP_DEVICE_LISTER (lister), 0);
    self = KORVA_UPNP_DEVICE_LISTER (lister);

    return g_hash_table_size (self->priv->devices);
}

static gboolean
korva_upnp_device_lister_idle (KorvaDeviceLister *lister)
{
    gboolean result;
    KorvaUPnPFileServer *server;

    server = korva_upnp_file_server_get_default ();
    result = korva_upnp_file_server_idle (server);
    g_object_unref (server);

    return result;
}

static void
korva_upnp_device_lister_on_context_available (GUPnPContextManager *cm,
                                               GUPnPContext        *context,
                                               gpointer             user_data)
{
    GUPnPControlPoint *cp;

    g_debug ("New network context available: %s",
             gssdp_client_get_host_ip (GSSDP_CLIENT (context)));

    cp = gupnp_control_point_new (context, MEDIA_RENDERER);
    gupnp_context_manager_manage_control_point (cm, cp);
    g_signal_connect (cp,
                      "device-proxy-available",
                      G_CALLBACK (korva_upnp_device_lister_on_renderer_available),
                      user_data);

    g_signal_connect (cp,
                      "device-proxy-unavailable",
                      G_CALLBACK (korva_upnp_device_lister_on_renderer_unavailable),
                      user_data);
    gssdp_resource_browser_set_active (GSSDP_RESOURCE_BROWSER (cp),
                                       TRUE);
    g_object_unref (cp);
}

static void
korva_upnp_device_lister_on_device_ready (GObject      *source,
                                          GAsyncResult *res,
                                          gpointer      user_data)
{
    GError *error = NULL;
    gboolean ok;
    KorvaUPnPDevice *device = KORVA_UPNP_DEVICE (source);
    KorvaUPnPDeviceLister *self = KORVA_UPNP_DEVICE_LISTER (user_data);

    g_object_ref (source);
    g_hash_table_remove (self->priv->pending_devices,
                         korva_device_get_uid (KORVA_DEVICE (device)));

    ok = g_async_initable_init_finish (G_ASYNC_INITABLE (source),
                                       res,
                                       &error);
    if (ok) {
        g_debug ("Device %s ready to use, adding",
                 korva_device_get_uid (KORVA_DEVICE (device)));
        g_hash_table_insert (self->priv->devices,
                             g_strdup (korva_device_get_uid (KORVA_DEVICE (device))),
                             device);
        g_signal_emit_by_name (self, "device-available", device);
    } else {
        g_warning ("Failed to add device: %s", error->message);
        g_error_free (error);
        g_object_unref (device);
    }
}

static void
korva_upnp_device_lister_on_renderer_available (GUPnPControlPoint *cp,
                                                GUPnPDeviceProxy  *proxy,
                                                gpointer           user_data)
{
    KorvaUPnPDeviceLister *self = KORVA_UPNP_DEVICE_LISTER (user_data);
    KorvaUPnPDevice *device;
    const char *uid;

    uid = gupnp_device_info_get_udn (GUPNP_DEVICE_INFO (proxy));

    g_debug ("A new device appeared: %s", uid);

    /* check if we already have such a device */
    device = g_hash_table_lookup (self->priv->devices, uid);
    if (device == NULL) {
        device = g_hash_table_lookup (self->priv->pending_devices, uid);
    }

    if (device == NULL) {
        device = g_object_new (KORVA_TYPE_UPNP_DEVICE,
                               "proxy", g_object_ref (proxy),
                               NULL);
        g_hash_table_insert (self->priv->pending_devices,
                             g_strdup (uid),
                             device);
        g_async_initable_init_async (G_ASYNC_INITABLE (device),
                                     G_PRIORITY_DEFAULT,
                                     NULL,
                                     korva_upnp_device_lister_on_device_ready,
                                     user_data);
    } else {
        korva_upnp_device_add_proxy (device, proxy);
        g_debug ("Device '%s' already known, ignoring…", uid);
    }
}

static void
korva_upnp_device_lister_on_renderer_unavailable (GUPnPControlPoint *cp,
                                                  GUPnPDeviceProxy  *proxy,
                                                  gpointer           user_data)
{
    KorvaUPnPDeviceLister *self = KORVA_UPNP_DEVICE_LISTER (user_data);
    const char *uid;
    KorvaDevice *device;

    uid = gupnp_device_info_get_udn (GUPNP_DEVICE_INFO (proxy));

    if (uid == NULL) {
        g_warning ("Device is invalid. NULL UDN");

        return;
    }

    g_debug ("Device %s disappeared", uid);

    device = g_hash_table_lookup (self->priv->devices, uid);

    if (device == NULL) {
        g_debug ("Unkown device %s. Ignoring.", uid);

        return;
    }

    /* only emit signal when we have no means to reach the device anymore */
    if (korva_upnp_device_remove_proxy (KORVA_UPNP_DEVICE (device), proxy)) {
        g_hash_table_remove (self->priv->devices, uid);
        g_signal_emit_by_name (self, "device-unavailable", uid);
    }
}
