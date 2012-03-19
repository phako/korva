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

#include <glib.h>

#ifdef G_OS_UNIX
#   include <glib-unix.h>
#endif

#include "korva-server.h"
#include "korva-device-lister.h"
#include "korva-dbus-interface.h"

struct _KorvaBackend {
    KorvaDeviceLister *lister;
};
typedef struct _KorvaBackend KorvaBackend;

static void
korva_backend_free (KorvaBackend *backend) {
    g_object_unref (backend->lister);
    g_free (backend);
}

G_DEFINE_TYPE(KorvaServer, korva_server, G_TYPE_OBJECT);

/* forward declarations */

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
                             gpointer               user_data);

/* Backend signal handlers */
static void
korva_server_on_device_available (KorvaDeviceLister *source,
                                  KorvaDevice       *device,
                                  gpointer           user_data);

static void
korva_server_on_device_unavailable (KorvaDeviceLister *source,
                                    const char        *uid,
                                    gpointer user_data);

struct _KorvaServerPrivate {
    GMainLoop        *loop;
    KorvaController1 *dbus_controller;
    GList            *backends;
    guint             bus_id;
};

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
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                              KORVA_TYPE_SERVER,
                                              KorvaServerPrivate);
    self->priv->loop = g_main_loop_new (NULL, FALSE);

    self->priv->bus_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                                         "org.jensge.Korva",
                                         G_BUS_NAME_OWNER_FLAGS_NONE,
                                         korva_server_on_bus_aquired,
                                         korva_server_on_name_aquired,
                                         korva_server_on_name_lost,
                                         g_object_ref (self),
                                         g_object_unref);

#ifdef G_OS_UNIX
    g_unix_signal_add (SIGINT, korva_server_signal_handler, self);
#endif
}

static void
korva_server_dispose (GObject *object)
{
    KorvaServer *self = KORVA_SERVER (object);

    if (self->priv->backends != NULL) {
        g_list_foreach (self->priv->backends, (GFunc) korva_backend_free, NULL);
        self->priv->backends = NULL;
    }

    if (self->priv->dbus_controller != NULL) {
        g_object_unref (self->priv->dbus_controller);
        self->priv->dbus_controller = NULL;
    }
}

static void
korva_server_finalize (GObject *object)
{
}

static void
korva_server_class_init (KorvaServerClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = korva_server_dispose;
    object_class->finalize = korva_server_finalize;
    
    g_type_class_add_private (klass, sizeof (KorvaServerPrivate));
}

KorvaServer*
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
    
    g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON(controller),
                                      connection,
                                      "/org/jensge/Korva",
                                      &error);

    if (error != NULL) {
        g_warning ("Failed to export D-Bus interface: %s",
                   error->message);

        g_error_free (error);

        g_main_loop_quit (self->priv->loop);
    }
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
serialize_device_list_backend (KorvaBackend *backend,
                               GVariantBuilder *builder)
{
    GList *devices;

    devices = korva_device_lister_get_devices (backend->lister);
    if (devices != NULL)
        g_list_foreach (devices, serialize_device_list, builder);
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

    builder = g_variant_builder_new (G_VARIANT_TYPE_ARRAY);
    it = self->priv->backends;
    while (it) {
        KorvaBackend *backend = (KorvaBackend *)it->data;
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

static gboolean
korva_server_on_handle_get_device_info (KorvaController1      *iface,
                                        GDBusMethodInvocation *invocation,
                                        const char            *uid,
                                        gpointer               user_data)
{
    korva_controller1_complete_get_device_info (iface, invocation, NULL);

    return TRUE;
}

static gboolean
korva_server_on_handle_push (KorvaController1      *iface,
                             GDBusMethodInvocation *invocation,
                             gpointer               user_data)
{
    korva_controller1_complete_push (iface, invocation);

    return TRUE;
}

static void
korva_server_on_device_available (KorvaDeviceLister *source,
                                  KorvaDevice *device,
                                  gpointer user_data)
{
    KorvaServer *self = KORVA_SERVER (user_data);

    g_signal_emit_by_name (self->priv->dbus_controller,
                           "device-available",
                           korva_device_serialize (device));
}

static void
korva_server_on_device_unavailable (KorvaDeviceLister *source,
                                    const char *uid,
                                    gpointer user_data)
{
    KorvaServer *self = KORVA_SERVER (user_data);

    g_signal_emit_by_name (self->priv->dbus_controller,
                           "device-unavailable",
                           uid);
}
