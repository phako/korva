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

#define G_LOG_DOMAIN "Korva-Server"

#include <glib.h>

#ifdef G_OS_UNIX
#   include <glib-unix.h>
#endif

#include "korva-error.h"
#include "korva-server.h"
#include "korva-device-lister.h"
#include "korva-dbus-interface.h"

#include "upnp/korva-upnp-device-lister.h"

#define DEFAULT_TIMEOUT 600

struct _KorvaBackend {
    KorvaDeviceLister *lister;
};
typedef struct _KorvaBackend KorvaBackend;

static void
korva_backend_free (KorvaBackend *backend)
{
    g_object_unref (backend->lister);
    g_free (backend);
}

struct _KorvaServerPrivate {
    GMainLoop        *loop;
    KorvaController1 *dbus_controller;
    GList            *backends;
    guint             bus_id;
    GHashTable       *tags;
    guint             timeout_id;
};

G_DEFINE_TYPE_WITH_PRIVATE (KorvaServer, korva_server, G_TYPE_OBJECT);

/* forward declarations */

/* Source functions */
static gboolean
korva_server_on_timeout (gpointer user_data);

static void
korva_server_reset_timeout (KorvaServer *self);

/* g_bus_own_name */
static void
korva_server_on_bus_aquired  (GDBusConnection *connection,
                              const gchar     *name,
                              gpointer         user_data);
static void
korva_server_on_name_aquired (GDBusConnection *connection,
                              const gchar     *name,
                              gpointer         user_data);
static void
korva_server_on_name_lost    (GDBusConnection *connection,
                              const gchar     *name,
                              gpointer         user_data);

/* KorvaController1 signal handlers */
static gboolean
korva_server_on_handle_get_devices (KorvaController1      *iface,
                                    GDBusMethodInvocation *invocation,
                                    gpointer               user_data);

static gboolean
korva_server_on_handle_get_device_info (KorvaController1      *iface,
                                        GDBusMethodInvocation *invocation,
                                        const gchar           *uid,
                                        gpointer               user_data);

static gboolean
korva_server_on_handle_push (KorvaController1      *iface,
                             GDBusMethodInvocation *invocation,
                             GVariant              *source,
                             const char            *uid,
                             gpointer               user_data);

static gboolean
korva_server_on_handle_unshare (KorvaController1      *iface,
                                GDBusMethodInvocation *invocation,
                                const char            *tag,
                                gpointer               user_data);
/* Backend signal handlers */
static void
korva_server_on_device_available (KorvaDeviceLister *source,
                                  KorvaDevice       *device,
                                  gpointer           user_data);

static void
korva_server_on_device_unavailable (KorvaDeviceLister *source,
                                    const char        *uid,
                                    gpointer           user_data);

static gboolean
korva_server_signal_handler (gpointer user_data)
{
    KorvaServer *self = KORVA_SERVER (user_data);

    g_main_loop_quit (self->priv->loop);

    return FALSE;
}

static void
korva_server_init (KorvaServer *self)
{
    self->priv = korva_server_get_instance_private (self);
    self->priv->loop = g_main_loop_new (NULL, FALSE);

    self->priv->bus_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                                         "org.jensge.Korva",
                                         G_BUS_NAME_OWNER_FLAGS_NONE,
                                         korva_server_on_bus_aquired,
                                         korva_server_on_name_aquired,
                                         korva_server_on_name_lost,
                                         g_object_ref (self),
                                         g_object_unref);

    self->priv->tags = g_hash_table_new_full (g_str_hash,
                                              (GEqualFunc) g_str_equal,
                                              g_free,
                                              g_free);

#ifdef G_OS_UNIX
    g_unix_signal_add (SIGINT, korva_server_signal_handler, self);
#endif

    korva_server_reset_timeout (self);
}

static void
backend_list_free (GList *backend)
{
    g_list_free_full (backend, (GDestroyNotify) korva_backend_free);
}

static void
korva_server_dispose (GObject *object)
{
    KorvaServer *self = KORVA_SERVER (object);

    g_clear_pointer (&self->priv->backends, backend_list_free);
    g_clear_object (&self->priv->dbus_controller);

    G_OBJECT_CLASS (korva_server_parent_class)->dispose (object);
}

static void
korva_server_finalize (GObject *object)
{
    KorvaServer *self = KORVA_SERVER (object);

    g_clear_pointer (&self->priv->tags, g_hash_table_destroy);

    G_OBJECT_CLASS (korva_server_parent_class)->finalize (object);
}

static void
korva_server_class_init (KorvaServerClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = korva_server_dispose;
    object_class->finalize = korva_server_finalize;
}

KorvaServer *
korva_server_new (void)
{
    KorvaServer *self;

    self = g_object_new (KORVA_TYPE_SERVER, NULL);

    return self;
}

void
korva_server_run (KorvaServer *self)
{
    g_return_if_fail (self != NULL);
    g_return_if_fail (KORVA_IS_SERVER (self));

    g_main_loop_run (self->priv->loop);
    g_bus_unown_name (self->priv->bus_id);
}


/* DBus callbacks */
static void
korva_server_on_bus_aquired  (GDBusConnection *connection,
                              const gchar     *name,
                              gpointer         user_data)
{
    KorvaBackend *backend;
    GError *error = NULL;
    KorvaController1 *controller;

    KorvaServer *self = KORVA_SERVER (user_data);

    controller = korva_controller1_skeleton_new ();
    self->priv->dbus_controller = controller;

    g_signal_connect (G_OBJECT (controller),
                      "handle-get-devices",
                      G_CALLBACK (korva_server_on_handle_get_devices),
                      user_data);
    g_signal_connect (G_OBJECT (controller),
                      "handle-get-device-info",
                      G_CALLBACK (korva_server_on_handle_get_device_info),
                      user_data);
    g_signal_connect (G_OBJECT (controller),
                      "handle-push",
                      G_CALLBACK (korva_server_on_handle_push),
                      user_data);

    g_signal_connect (G_OBJECT (controller),
                      "handle-unshare",
                      G_CALLBACK (korva_server_on_handle_unshare),
                      user_data);

    g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (controller),
                                      connection,
                                      "/org/jensge/Korva",
                                      &error);

    if (error != NULL) {
        g_warning ("Failed to export D-Bus interface: %s",
                   error->message);

        g_error_free (error);

        g_main_loop_quit (self->priv->loop);

        return;
    }

    backend = g_new0 (KorvaBackend, 1);
    backend->lister = korva_upnp_device_lister_new ();
    self->priv->backends = g_list_append (self->priv->backends, backend);
    g_signal_connect (backend->lister,
                      "device-available",
                      G_CALLBACK (korva_server_on_device_available),
                      self);

    g_signal_connect (backend->lister,
                      "device-unavailable",
                      G_CALLBACK (korva_server_on_device_unavailable),
                      self);
}

static void
korva_server_on_name_aquired (GDBusConnection *connection,
                              const gchar     *name,
                              gpointer         user_data)
{
}

static void
korva_server_on_name_lost    (GDBusConnection *connection,
                              const gchar     *name,
                              gpointer         user_data)
{
}


static void
serialize_device_list (gpointer data, gpointer user_data)
{
    GVariantBuilder *builder = (GVariantBuilder *) user_data;
    KorvaDevice *device = (KorvaDevice *) data;

    g_variant_builder_add_value (builder,
                                 korva_device_serialize (device));
}

static void
serialize_device_list_backend (KorvaBackend    *backend,
                               GVariantBuilder *builder)
{
    GList *devices;

    devices = korva_device_lister_get_devices (backend->lister);
    if (devices != NULL) {
        g_list_foreach (devices, serialize_device_list, builder);
    }
}

/* Controller1 interface callbacks */
static gboolean
korva_server_on_handle_get_devices (KorvaController1      *iface,
                                    GDBusMethodInvocation *invocation,
                                    gpointer               user_data)
{
    KorvaServer *self = KORVA_SERVER (user_data);
    GVariantBuilder *builder;
    GVariant *result;
    GList *it;
    gboolean devices = FALSE;

    korva_server_reset_timeout (self);

    builder = g_variant_builder_new (G_VARIANT_TYPE_ARRAY);
    it = self->priv->backends;
    while (it) {
        KorvaBackend *backend = (KorvaBackend *) it->data;
        if (korva_device_lister_get_device_count (backend->lister) > 0) {
            devices = TRUE;
            serialize_device_list_backend (backend, builder);
        }
        it = it->next;
    }

    /* Add empty hash if no devices found */
    if (!devices) {
        g_variant_builder_add_value (builder, g_variant_new ("a{sv}", NULL));
    }

    result = g_variant_builder_end (builder);

    korva_controller1_complete_get_devices (iface, invocation, result);

    return TRUE;
}

typedef struct {
    const char  *uid;
    KorvaDevice *device;
} FindDeviceData;

static void
find_device_func (gpointer data, gpointer user_data)
{
    FindDeviceData *device_data = (FindDeviceData *) user_data;
    KorvaBackend *backend = (KorvaBackend *) data;

    if (device_data->device != NULL) {
        return;
    }

    device_data->device = korva_device_lister_get_device_info (backend->lister,
                                                               device_data->uid);
}

static KorvaDevice *
korva_server_get_device (KorvaServer *self, const char *uid)
{
    FindDeviceData data;

    data.uid = uid;
    data.device = NULL;

    g_list_foreach (self->priv->backends, find_device_func, &data);

    return data.device;
}

static gboolean
korva_server_on_handle_get_device_info (KorvaController1      *iface,
                                        GDBusMethodInvocation *invocation,
                                        const char            *uid,
                                        gpointer               user_data)
{
    KorvaServer *self = KORVA_SERVER (user_data);
    KorvaDevice *device;

    korva_server_reset_timeout (self);

    device = korva_server_get_device (self, uid);

    if (device != NULL) {
        GVariant *result;

        result = korva_device_serialize (device);
        korva_controller1_complete_get_device_info (iface, invocation, result);
    } else {
        g_dbus_method_invocation_return_error (invocation,
                                               KORVA_CONTROLLER1_ERROR,
                                               KORVA_CONTROLLER1_ERROR_NO_SUCH_DEVICE,
                                               "Device '%s' does not exist",
                                               uid);
    }

    return TRUE;
}

typedef struct {
    KorvaServer           *self;
    GDBusMethodInvocation *invocation;
} PushAsyncData;

static void
korva_server_on_push_async_ready (GObject      *obj,
                                  GAsyncResult *res,
                                  gpointer      user_data)
{
    KorvaDevice *device = KORVA_DEVICE (obj);
    PushAsyncData *data = (PushAsyncData *) user_data;
    GError *error = NULL;
    char *tag;

    tag = korva_device_push_finish (device, res, &error);
    if (tag == NULL) {
        g_dbus_method_invocation_return_gerror (data->invocation,
                                                error);

        goto out;
    }

    g_hash_table_insert (data->self->priv->tags,
                         g_strdup (tag),
                         g_strdup (korva_device_get_uid (device)));

    korva_controller1_complete_push (data->self->priv->dbus_controller, data->invocation, tag);
    g_free (tag);

out:
    g_free (data);
}

static gboolean
korva_server_on_handle_push (KorvaController1      *iface,
                             GDBusMethodInvocation *invocation,
                             GVariant              *source,
                             const char            *uid,
                             gpointer               user_data)
{
    KorvaServer *self = KORVA_SERVER (user_data);
    KorvaDevice *device;
    PushAsyncData *data;

    korva_server_reset_timeout (self);

    device = korva_server_get_device (self, uid);

    if (!g_variant_is_of_type (source, G_VARIANT_TYPE_VARDICT)) {
        g_dbus_method_invocation_return_error (invocation,
                                               KORVA_CONTROLLER1_ERROR,
                                               KORVA_CONTROLLER1_ERROR_INVALID_ARGS,
                                               "'source' parameter needs to be 'a{sv}'");

        return TRUE;
    }

    if (device == NULL) {
        g_dbus_method_invocation_return_error (invocation,
                                               KORVA_CONTROLLER1_ERROR,
                                               KORVA_CONTROLLER1_ERROR_NO_SUCH_DEVICE,
                                               "Device '%s' does not exist",
                                               uid);

        return TRUE;
    }

    data = g_new0 (PushAsyncData, 1);
    data->self = self;
    data->invocation = invocation;

    korva_device_push_async (device, source, NULL, korva_server_on_push_async_ready, data);

    return TRUE;
}

static void
korva_server_on_unshare_async_ready (GObject      *obj,
                                     GAsyncResult *res,
                                     gpointer      user_data)
{
    KorvaDevice *device = KORVA_DEVICE (obj);
    PushAsyncData *data = (PushAsyncData *) user_data;
    GError *error = NULL;

    if (!korva_device_unshare_finish (device, res, &error)) {
        g_dbus_method_invocation_return_gerror (data->invocation,
                                                error);

        goto out;
    }

    korva_controller1_complete_unshare (data->self->priv->dbus_controller, data->invocation);

out:
    g_free (data);
}

static gboolean
korva_server_on_handle_unshare (KorvaController1      *iface,
                                GDBusMethodInvocation *invocation,
                                const char            *tag,
                                gpointer               user_data)
{
    KorvaServer *self = KORVA_SERVER (user_data);
    KorvaDevice *device;
    PushAsyncData *data;
    const char *uid;

    korva_server_reset_timeout (self);

    uid = g_hash_table_lookup (self->priv->tags, tag);
    if (uid == NULL) {
        g_dbus_method_invocation_return_error (invocation,
                                               KORVA_CONTROLLER1_ERROR,
                                               KORVA_CONTROLLER1_ERROR_NO_SUCH_TRANSFER,
                                               "Push operation '%s' does not exist",
                                               tag);

        return TRUE;
    }

    device = korva_server_get_device (self, uid);
    g_hash_table_remove (self->priv->tags, tag);

    if (device == NULL) {
        g_dbus_method_invocation_return_error (invocation,
                                               KORVA_CONTROLLER1_ERROR,
                                               KORVA_CONTROLLER1_ERROR_NO_SUCH_DEVICE,
                                               "Device '%s' does not exist",
                                               uid);

        return TRUE;
    }

    data = g_new0 (PushAsyncData, 1);
    data->self = self;
    data->invocation = invocation;

    korva_device_unshare_async (device, tag, NULL, korva_server_on_unshare_async_ready, data);

    return TRUE;
}


static void
korva_server_on_device_available (KorvaDeviceLister *source,
                                  KorvaDevice       *device,
                                  gpointer           user_data)
{
    KorvaServer *self = KORVA_SERVER (user_data);

    g_signal_emit_by_name (self->priv->dbus_controller,
                           "device-available",
                           korva_device_serialize (device));
}

static void
korva_server_on_device_unavailable (KorvaDeviceLister *source,
                                    const char        *uid,
                                    gpointer           user_data)
{
    KorvaServer *self = KORVA_SERVER (user_data);

    g_signal_emit_by_name (self->priv->dbus_controller,
                           "device-unavailable",
                           uid);
}

static void
idle_collector (gpointer data, gpointer user_data)
{
    KorvaBackend *backend = (KorvaBackend *) data;
    gboolean *is_idle = (gboolean *) user_data;

    /* already found busy back-end */
    if (!(*is_idle)) {
        return;
    }

    if (!korva_device_lister_idle (backend->lister)) {
        g_debug ("Backend claims it's not idle");
        *is_idle = FALSE;
    }
}

static void
korva_server_reset_timeout (KorvaServer *self)
{
    if (self->priv->timeout_id != 0) {
        g_source_remove (self->priv->timeout_id);
    }

    g_debug ("Setting timeout to %d seconds", DEFAULT_TIMEOUT);
    self->priv->timeout_id = g_timeout_add_seconds (DEFAULT_TIMEOUT,
                                                    korva_server_on_timeout,
                                                    self);
}

static gboolean
korva_server_on_timeout (gpointer user_data)
{
    KorvaServer *self = KORVA_SERVER (user_data);
    gboolean is_idle = TRUE;

    g_list_foreach (self->priv->backends, idle_collector, &is_idle);

    if (is_idle) {
        g_main_loop_quit (self->priv->loop);
    }

    /* If we were not idle, just restart the timeout */
    return !is_idle;
}
