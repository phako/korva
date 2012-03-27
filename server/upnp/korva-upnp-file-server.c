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

#include <gio/gio.h>
#include <libsoup/soup.h>

#include <korva-error.h>

#include "korva-upnp-file-server.h"

G_DEFINE_TYPE (KorvaUPnPFileServer, korva_upnp_file_server, G_TYPE_OBJECT);

typedef struct {
    GFile      *file;
    GHashTable *meta_data;
    GList      *ifaces;
} HostData;

static HostData *
host_data_new (GHashTable *meta_data, const char *address)
{
    HostData *self;
    
    self = g_slice_new0 (HostData);
    self->meta_data = meta_data;
    self->ifaces = g_list_prepend (self->ifaces, g_strdup (address));

    return self;
}

static void
host_data_add_interface (HostData *self, const char *iface)
{
}

static char *
host_data_get_uri (HostData *self, const char *iface, guint port)
{
    char *hash, *result, *uri;

    uri = g_file_get_uri (self->file);
    hash = g_compute_checksum_for_string (G_CHECKSUM_MD5, uri, -1);

    result = g_strdup_printf ("http://%s:%u/item/%s",
                              iface,
                              port,
                              hash);

    g_free (hash);
    g_free (uri);

    return result;
}

static void
host_data_free (HostData *self)
{
    if (self == NULL) {
        return;
    }

    g_hash_table_destroy (self->meta_data);
    g_object_unref (self->file);
    g_list_free_full (self->ifaces, g_free);

    g_slice_free (HostData, self);
}

struct _KorvaUPnPFileServerPrivate {
    SoupServer *http_server;
    GHashTable *host_data;
    guint       port;
};

static void
korva_upnp_file_server_handle_request (SoupServer *server,
                                       SoupMessage *msg,
                                       const char *path,
                                       GHashTable *query,
                                       SoupClientContext *client,
                                       gpointer user_data)
{
    g_debug ("Got request for uri: %s", path);
    soup_message_set_status (msg, SOUP_STATUS_NOT_FOUND);
}

static void
korva_upnp_file_server_init (KorvaUPnPFileServer *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                              KORVA_TYPE_UPNP_FILE_SERVER,
                                              KorvaUPnPFileServerPrivate);
    self->priv->http_server = soup_server_new (NULL, NULL);
    soup_server_add_handler (self->priv->http_server,
                             "/item",
                             korva_upnp_file_server_handle_request,
                             self,
                             NULL);
    soup_server_run_async (self->priv->http_server);
    self->priv->host_data = g_hash_table_new_full (g_file_hash,
                                                   (GEqualFunc) g_file_equal,
                                                   g_object_unref,
                                                   (GDestroyNotify) host_data_free);
}

static void
korva_upnp_file_server_finalize (GObject *object)
{
    /* TODO: Add deinitalization code here */

    G_OBJECT_CLASS (korva_upnp_file_server_parent_class)->finalize (object);
}

static GObject*
korva_upnp_file_server_constructor (GType                  type,
                                    guint                  n_construct_params,
                                    GObjectConstructParam *construct_params)
{
    static GObject *instance;

    if (instance == NULL) {
        GObjectClass *parent_class;
        parent_class = G_OBJECT_CLASS (korva_upnp_file_server_parent_class); 
        instance = parent_class->constructor (type,
                                              n_construct_params,
                                              construct_params);
        g_object_add_weak_pointer (instance, (gpointer *) &instance);

        return instance;
    }

    return g_object_ref (instance);
}

static void
korva_upnp_file_server_class_init (KorvaUPnPFileServerClass *klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS (klass);

    object_class->constructor = korva_upnp_file_server_constructor;
    object_class->finalize = korva_upnp_file_server_finalize;

    g_type_class_add_private (klass, sizeof (KorvaUPnPFileServerPrivate));
}

KorvaUPnPFileServer*
korva_upnp_file_server_get_default (void)
{
    return g_object_new (KORVA_TYPE_UPNP_FILE_SERVER, NULL);
}

void
korva_upnp_file_server_host_file_async (KorvaUPnPFileServer *self,
                                        GFile *file,
                                        GHashTable *params,
                                        const char *iface,
                                        GAsyncReadyCallback callback,
                                        gpointer user_data)
{
    HostData *data;
    GSimpleAsyncResult *result;
    char *uri;
    guint port;

    result = g_simple_async_result_new (G_OBJECT (self),
                                        callback,
                                        user_data,
                                        (gpointer) korva_upnp_file_server_host_file_async);

    info = g_file_query_info (file,
                              G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                              0,
                              NULL,
                              &error);
    if (info == NULL) {
        char *tmp;
        GError *error;

        error = g_error_new (KORVA_CONTROLLER1_ERROR,
                             KORVA_CONTROLLER1_ERROR_FILE_NOT_FOUND,
                             "'%s' does not exist",
                             (tmp = g_file_get_uri (file)));
        g_free (tmp);
        g_simple_async_result_take_error (result, error);

        goto out;
    }

    data = g_hash_table_lookup (self->priv->host_data, file);
    if (data == NULL) {
        data = host_data_new (params, iface);
        data->file = g_object_ref (file);

        g_hash_table_insert (self->priv->host_data, file, data);
    } else {
        host_data_add_interface (data, iface);
    }

    port = soup_server_get_port (self->priv->http_server);
    uri = host_data_get_uri (data, iface, port);
    
    g_simple_async_result_set_op_res_gpointer (result, uri, g_free);
    
out:
    g_simple_async_result_complete_in_idle (result);
    g_object_unref (result);
}

char *
korva_upnp_file_server_host_file_finish (KorvaUPnPFileServer  *self,
                                         GAsyncResult         *res,
                                         GError              **error)
{
    GSimpleAsyncResult *result;

    if (!g_simple_async_result_is_valid (res,
                                         G_OBJECT (self),
                                         korva_upnp_file_server_host_file_async)) {
        return NULL;
    }

    result = (GSimpleAsyncResult *) res;
    if (g_simple_async_result_propagate_error (result, error)) {
        return NULL;
    }

    return g_simple_async_result_get_op_res_gpointer (result);
}
