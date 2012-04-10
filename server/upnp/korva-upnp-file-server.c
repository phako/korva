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
    goffset     size;
    GFileInfo  *info;
    const char *content_type;
} HostData;

static HostData *
host_data_new (GHashTable *meta_data, GFileInfo *info, const char *address)
{
    HostData *self;
    GVariant *value;
    
    self = g_slice_new0 (HostData);
    self->meta_data = meta_data;
    self->ifaces = g_list_prepend (self->ifaces, g_strdup (address));
    self->info = g_object_ref (info);
    value = g_hash_table_lookup (meta_data, "Size");
    if (value != NULL) {
        self->size = (goffset) g_variant_get_uint64 (value);
    } else {
        self->size = g_file_info_get_size (info);
        g_hash_table_insert (meta_data, g_strdup ("Size"), g_variant_new_uint64 (self->size));
    }

    value = g_hash_table_lookup (meta_data, "ContentType");
    if (value != NULL) {
        self->content_type = g_variant_get_string (value, NULL);
    } else {
        self->content_type = g_file_info_get_content_type (info);
        g_hash_table_insert (meta_data,
                             g_strdup ("ContentType"),
                             g_variant_new_string (self->content_type));
    }

    value = g_hash_table_lookup (meta_data, "Title");
    if (value == NULL) {
        g_hash_table_insert (meta_data,
                             g_strdup ("Title"),
                             g_variant_new_string (g_file_info_get_display_name (info)));
    }

    return self;
}

static void
host_data_add_interface (HostData *self, const char *iface)
{
    self->ifaces = g_list_prepend (self->ifaces, g_strdup (iface));
}

static char *
host_data_get_id (HostData *self)
{
    char *hash, *uri;

    uri = g_file_get_uri (self->file);
    hash = g_compute_checksum_for_string (G_CHECKSUM_MD5, uri, -1);
    g_free (uri);

    return hash;
}

static char *
host_data_get_uri (HostData *self, const char *iface, guint port)
{
    char *hash, *result;

    hash = host_data_get_id (self);

    result = g_strdup_printf ("http://%s:%u/item/%s",
                              iface,
                              port,
                              hash);

    g_free (hash);

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
    g_object_unref (self->info);

    g_slice_free (HostData, self);
}

struct _KorvaUPnPFileServerPrivate {
    SoupServer *http_server;
    GHashTable *host_data;
    GHashTable *id_map;
    guint       port;
    GRegex     *path_regex;
};

typedef struct _ServeData {
    SoupServer *server;
    GMappedFile *file;
    goffset start;
    goffset end;
} ServeData;

static void
korva_upnp_file_server_on_wrote_chunk (SoupMessage *msg,
                                       gpointer     user_data)
{
    ServeData *data = (ServeData *) user_data;
    SoupBuffer *buffer;
    int chunk_size;
    char *file_buffer;


    soup_server_pause_message (data->server, msg);

    chunk_size = MIN (data->end - data->start + 1, 65536);

    if (chunk_size <= 0) {
        soup_message_body_complete (msg->response_body);
        soup_server_unpause_message (data->server, msg);

        return;
    }

    file_buffer = g_mapped_file_get_contents (data->file) + data->start;

    buffer = soup_buffer_new_with_owner (file_buffer,
                                         chunk_size,
                                         data->file,
                                         NULL);
    data->start += chunk_size;
    soup_message_body_append_buffer (msg->response_body, buffer);

    soup_server_unpause_message (data->server, msg);
}

static void
korva_upnp_file_server_on_finished (SoupMessage *msg,
                                    gpointer     user_data)
{
    ServeData *data = (ServeData *) user_data;

    g_debug ("Handled request for '%s'",
             soup_uri_to_string (soup_message_get_uri (msg), FALSE));

    g_mapped_file_unref (data->file);
    g_slice_free (ServeData, data);
}

static void print_header (const char *name, const char *value, gpointer user_data)
{
    g_debug ("    %s: %s", name, value);
}

static void
korva_upnp_file_server_handle_request (SoupServer *server,
                                       SoupMessage *msg,
                                       const char *path,
                                       GHashTable *query,
                                       SoupClientContext *client,
                                       gpointer user_data)
{
    KorvaUPnPFileServer *self = KORVA_UPNP_FILE_SERVER (user_data);
    GMatchInfo *info;
    char *id, *file_path;
    GFile *file;
    HostData *data;
    ServeData *serve_data;
    SoupRange *ranges = NULL;
    int length;
    GError *error = NULL;

    if (msg->method != SOUP_METHOD_HEAD &&
        msg->method != SOUP_METHOD_GET) {
        soup_message_set_status (msg, SOUP_STATUS_METHOD_NOT_ALLOWED);
    }

    g_debug ("Got %s request for uri: %s", msg->method, path);
    soup_message_headers_foreach (msg->request_headers, print_header, NULL);

    soup_server_pause_message (server, msg);
    soup_message_set_status (msg, SOUP_STATUS_OK);

    if (!g_regex_match (self->priv->path_regex,
                        path,
                        0,
                        &info)) {
        soup_message_set_status (msg, SOUP_STATUS_NOT_FOUND);
        g_match_info_free (info);

        goto out;
    }

    id = g_match_info_fetch (info, 1);
    g_match_info_free (info);

    file = g_hash_table_lookup (self->priv->id_map, id);
    g_free (id);

    if (file == NULL) {
        soup_message_set_status (msg, SOUP_STATUS_NOT_FOUND);

        goto out;
    }

    data = g_hash_table_lookup (self->priv->host_data, file);
    if (data == NULL) {
        soup_message_set_status (msg, SOUP_STATUS_NOT_FOUND);

        goto out;
    }

    /* TODO: Check IP of requesting client with list of IPs attached to host
     * data
     */
    serve_data = g_slice_new0 (ServeData);
    if (soup_message_headers_get_ranges (msg->request_headers, data->size, &ranges, &length)) {
        goffset start, end;
        start = ranges[0].start;
        end = ranges[0].end;

        if (start > data->size || start > end || end > data->size) {
            soup_message_set_status (msg, SOUP_STATUS_REQUESTED_RANGE_NOT_SATISFIABLE);

            goto out;
        } else {
            soup_message_set_status (msg, SOUP_STATUS_PARTIAL_CONTENT);
        }
        serve_data->start = start;
        serve_data->end = end;
    } else {
        serve_data->start = 0;
        serve_data->end = data->size - 1;
        soup_message_set_status (msg, SOUP_STATUS_OK);
    }

    soup_message_headers_set_content_length (msg->response_headers,
                                             serve_data->end - serve_data->start + 1);
    soup_message_headers_set_content_type (msg->response_headers,
                                           data->content_type,
                                           NULL);

    if (g_ascii_strcasecmp (msg->method, "HEAD") == 0) {
        g_debug ("Handled HEAD request of %s: %d", path, msg->status_code);
        soup_message_headers_append (msg->response_headers, "Connection", "close");
        soup_message_headers_foreach (msg->response_headers, print_header, NULL);
        g_slice_free (ServeData, serve_data);

        goto out;
    }

    soup_message_headers_set_encoding (msg->response_headers, SOUP_ENCODING_CONTENT_LENGTH);
    soup_message_body_set_accumulate (msg->response_body, FALSE);

    file_path = g_file_get_path (file);
    serve_data->file = g_mapped_file_new (file_path, FALSE, &error);
    serve_data->server = server;
    g_free (file_path);
    if (error != NULL) {
        g_warning ("Failed to MMAP file %s: %s",
                   path,
                   error->message);

        g_error_free (error);
        g_slice_free (ServeData, serve_data);

        soup_message_set_status (msg, SOUP_STATUS_INTERNAL_SERVER_ERROR);

        goto out;
    }

    g_signal_connect (msg,
                      "wrote-chunk",
                      G_CALLBACK (korva_upnp_file_server_on_wrote_chunk),
                      serve_data);
    g_signal_connect (msg,
                      "wrote-headers",
                      G_CALLBACK (korva_upnp_file_server_on_wrote_chunk),
                      serve_data);
    g_signal_connect (msg,
                      "finished",
                      G_CALLBACK (korva_upnp_file_server_on_finished),
                      serve_data);

out:
    if (ranges != NULL) {
        soup_message_headers_free_ranges (msg->request_headers, ranges);
    }

    soup_server_unpause_message (server, msg);
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
    self->priv->id_map = g_hash_table_new_full (g_str_hash,
                                                (GEqualFunc) g_str_equal,
                                                g_free,
                                                g_object_unref);
    self->priv->path_regex = g_regex_new ("^/item/([0-9a-zA-Z]{32})$",
                                          G_REGEX_OPTIMIZE,
                                          G_REGEX_MATCH_NEWLINE_ANY,
                                          NULL);
}

static void
korva_upnp_file_server_dispose (GObject *object)
{
    KorvaUPnPFileServer *self = KORVA_UPNP_FILE_SERVER (object);

    if (self->priv->http_server != NULL) {
        g_object_unref (self->priv->http_server);
        self->priv->http_server = NULL;
    }

    G_OBJECT_CLASS (korva_upnp_file_server_parent_class)->dispose (object);
}

static void
korva_upnp_file_server_finalize (GObject *object)
{
    KorvaUPnPFileServer *self = KORVA_UPNP_FILE_SERVER (object);

    /* TODO: Add deinitalization code here */
    if (self->priv->host_data != NULL) {
        g_hash_table_destroy (self->priv->host_data);
        self->priv->host_data = NULL;
    }

    if (self->priv->id_map != NULL) {
        g_hash_table_destroy (self->priv->id_map);
        self->priv->id_map = NULL;
    }

    if (self->priv->path_regex != NULL) {
        g_regex_unref (self->priv->path_regex);
        self->priv->path_regex = NULL;
    }

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
    object_class->dispose = korva_upnp_file_server_dispose;

    g_type_class_add_private (klass, sizeof (KorvaUPnPFileServerPrivate));
}

KorvaUPnPFileServer*
korva_upnp_file_server_get_default (void)
{
    return g_object_new (KORVA_TYPE_UPNP_FILE_SERVER, NULL);
}

typedef struct {
    GHashTable *params;
    char *uri;
} HostFileResult;

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
    guint port;
    GFileInfo *info;
    GError *error;
    HostFileResult *result_data;

    result = g_simple_async_result_new (G_OBJECT (self),
                                        callback,
                                        user_data,
                                        (gpointer) korva_upnp_file_server_host_file_async);

    info = g_file_query_info (file,
                              G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE","
                              G_FILE_ATTRIBUTE_STANDARD_SIZE","
                              G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
                              0,
                              NULL,
                              &error);
    if (info == NULL) {
        char *tmp;
        GError *inner_error;

        inner_error = g_error_new (KORVA_CONTROLLER1_ERROR,
                                   KORVA_CONTROLLER1_ERROR_FILE_NOT_FOUND,
                                   "Error accessing '%s': %s",
                                   (tmp = g_file_get_uri (file)),
                                    error->message);
        g_free (tmp);
        g_error_free (error);
        g_simple_async_result_take_error (result, inner_error);
        g_hash_table_destroy (params);

        goto out;
    }

    data = g_hash_table_lookup (self->priv->host_data, file);
    if (data == NULL) {
        data = host_data_new (params, info, iface);
        data->file = g_object_ref (file);

        g_hash_table_insert (self->priv->host_data, file, data);
        g_hash_table_insert (self->priv->id_map,
                             host_data_get_id (data),
                             g_object_ref (file));
    } else {
        host_data_add_interface (data, iface);
    }

    g_object_unref (info);

    port = soup_server_get_port (self->priv->http_server);

    result_data = g_new0 (HostFileResult, 1);
    result_data->params = data->meta_data;
    result_data->uri = host_data_get_uri (data, iface, port);

    g_simple_async_result_set_op_res_gpointer (result, result_data, g_free);

out:
    g_simple_async_result_complete_in_idle (result);
    g_object_unref (result);
}

char *
korva_upnp_file_server_host_file_finish (KorvaUPnPFileServer  *self,
                                         GAsyncResult         *res,
                                         GHashTable          **params,
                                         GError              **error)
{
    GSimpleAsyncResult *result;
    HostFileResult *result_data;

    if (!g_simple_async_result_is_valid (res,
                                         G_OBJECT (self),
                                         korva_upnp_file_server_host_file_async)) {
        return NULL;
    }

    result = (GSimpleAsyncResult *) res;
    if (g_simple_async_result_propagate_error (result, error)) {
        return NULL;
    }

    result_data = (HostFileResult *) g_simple_async_result_get_op_res_gpointer (result);
    *params = result_data->params;

    return result_data->uri;
}
