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

#define G_LOG_DOMAIN "Korva-UPnP-File-Server"

#include <stdlib.h>

#include <gio/gio.h>
#include <libsoup/soup.h>
#include <libgupnp-av/gupnp-av.h>

#include <korva-error.h>

#include "korva-upnp-file-server.h"
#include "korva-upnp-metadata-query.h"

#define DEFAULT_IDLE_TIMEOUT 300

G_DEFINE_TYPE (KorvaUPnPFileServer, korva_upnp_file_server, G_TYPE_OBJECT);

struct _KorvaUPnPFileServerPrivate {
    SoupServer *http_server;
    GHashTable *host_data;
    GHashTable *id_map;
    guint       port;
    GRegex     *path_regex;
};

typedef struct {
    GFile      *file;
    GHashTable *meta_data;
    GList      *peers;
    goffset     size;
    const char *content_type;
    char       *protocol_info;
    KorvaUPnPFileServer *self;
    uint timeout_id;

    /* Helper members for first host_async call */
    GSimpleAsyncResult *result;
    char *iface;
} HostData;

static char *
host_data_get_id (HostData *self);

static gboolean
host_data_on_timeout (gpointer user_data)
{
    HostData *self = (HostData *) user_data;
    char *id, *uri;
    GFile *file;

    file = g_object_ref (self->file);
    uri = g_file_get_uri (file);
    g_debug ("File '%s' was not accessed for %d seconds; removing.",
             uri,
             DEFAULT_IDLE_TIMEOUT);
    g_free (uri);

    id = host_data_get_id (self);

    g_hash_table_remove (self->self->priv->id_map, id);
    g_free (id);

    g_hash_table_remove (self->self->priv->host_data, file);
    g_object_unref (file);

    return FALSE;
}

static HostData *
host_data_new (GFile *file, GHashTable *meta_data, const char *address)
{
    HostData *self;

    self = g_slice_new0 (HostData);
    self->meta_data = meta_data;
    self->peers = g_list_prepend (self->peers, g_strdup (address));
    self->file = g_object_ref (file);
    self->timeout_id = g_timeout_add_seconds (DEFAULT_IDLE_TIMEOUT, host_data_on_timeout, self);

    return self;
}

static void
host_data_add_peer (HostData *self, const char *iface)
{
    GList *it;

    it = g_list_find_custom (self->peers, iface, (GCompareFunc) g_strcmp0);
    if (it != NULL) {
        return;
    }

    self->peers = g_list_prepend (self->peers, g_strdup (iface));
}

static void
host_data_remove_peer (HostData *self, const char *peer)
{
    GList *it;

    it = g_list_find_custom (self->peers, peer, (GCompareFunc) g_strcmp0);
    if (it != NULL) {
        g_free (it->data);

        self->peers = g_list_remove_link (self->peers, it);
    }
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

static const char *
host_data_get_protocol_info (HostData *self)
{
    if (self->protocol_info == NULL) {
        GVariant *value;
        const char *dlna_profile;
        GUPnPProtocolInfo *info;

        info = gupnp_protocol_info_new_from_string ("http-get:*:*:DLNA.ORG_CI=0;DLNA.ORG_OP=01", NULL);
        gupnp_protocol_info_set_mime_type (info, self->content_type);

        value = g_hash_table_lookup (self->meta_data, "DLNAProfile");
        if (value != NULL) {
            dlna_profile = g_variant_get_string (value, NULL);
            gupnp_protocol_info_set_dlna_profile (info, dlna_profile);
        }

        self->protocol_info = gupnp_protocol_info_to_string (info);
        g_object_unref (info);
    }

    return self->protocol_info;
}

static gboolean
host_data_valid_for_peer (HostData *self, const char *peer)
{
    GList *it;

    it = g_list_find_custom (self->peers, peer, (GCompareFunc) g_strcmp0);

    return it != NULL;
}

static void
host_data_free (HostData *self)
{
    if (self == NULL) {
        return;
    }

    g_hash_table_destroy (self->meta_data);
    g_object_unref (self->file);
    g_list_free_full (self->peers, g_free);

    if (self->protocol_info != NULL) {
        g_free (self->protocol_info);
    }

    if (self->timeout_id != 0) {
        g_source_remove (self->timeout_id);
    }

    g_slice_free (HostData, self);
}

typedef struct _ServeData {
    SoupServer *server;
    GMappedFile *file;
    goffset start;
    goffset end;
    HostData *host_data;
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

    data->host_data->timeout_id = g_timeout_add_seconds (DEFAULT_IDLE_TIMEOUT, host_data_on_timeout, data->host_data);

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
    const char *content_features;
    GError *error = NULL;

    if (msg->method != SOUP_METHOD_HEAD &&
        msg->method != SOUP_METHOD_GET) {
        soup_message_set_status (msg, SOUP_STATUS_METHOD_NOT_ALLOWED);

        return;
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

    if (!host_data_valid_for_peer (data, soup_client_context_get_host (client))) {
        soup_message_set_status (msg, SOUP_STATUS_NOT_FOUND);

        goto out;
    }

    serve_data = g_slice_new0 (ServeData);
    serve_data->host_data = data;
    if (soup_message_headers_get_ranges (msg->request_headers, data->size, &ranges, &length)) {
        goffset start, end;
        start = ranges[0].start;
        end = ranges[0].end;

        if (start > data->size || start > end || end > data->size) {
            soup_message_set_status (msg, SOUP_STATUS_REQUESTED_RANGE_NOT_SATISFIABLE);
            g_slice_free (ServeData, serve_data);

            goto out;
        } else {
            soup_message_set_status (msg, SOUP_STATUS_PARTIAL_CONTENT);
            soup_message_headers_set_content_range (msg->response_headers, start, end, data->size);
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

    content_features = soup_message_headers_get_one (msg->request_headers,
                                                      "getContentFeatures.dlna.org");
    if (content_features != NULL && atol (content_features) == 1) {
        GVariant *value;

        value = g_hash_table_lookup (data->meta_data, "DLNAProfile");
        if (value == NULL) {
            soup_message_headers_append (msg->response_headers,
                                         "contentFeatures.dlna.org", "*");
        } else {
            soup_message_headers_append (msg->response_headers,
                                         "contentFeatures.dlna.org",
                                         host_data_get_protocol_info (data));
        }
    }

    soup_message_headers_append (msg->response_headers, "Connection", "close");

    g_debug ("Response headers:");
    soup_message_headers_foreach (msg->response_headers, print_header, NULL);

    if (g_ascii_strcasecmp (msg->method, "HEAD") == 0) {
        g_debug ("Handled HEAD request of %s: %d", path, msg->status_code);
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

    /* Drop timeout until the message is done */
    g_source_remove (data->timeout_id);
    data->timeout_id = 0;

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

static void
korva_upnp_file_server_on_metadata_query_run_done (GObject *sender,
                                                   GAsyncResult *res,
                                                   gpointer      user_data)
{
    HostData *data = (HostData *) user_data;
    GSimpleAsyncResult *result = data->result;
    HostFileResult *result_data;
    guint port;
    GError *error = NULL;
    GVariant *value;

    if (!korva_upnp_metadata_query_run_finish (KORVA_UPNP_METADATA_QUERY (sender),
                                               res,
                                               &error)) {
        g_simple_async_result_take_error (data->result, error);
        host_data_free (data);

        goto out;
    }

    g_hash_table_insert (data->self->priv->host_data, data->file, data);
    g_hash_table_insert (data->self->priv->id_map,
                         host_data_get_id (data),
                         g_object_ref (data->file));

    value = g_hash_table_lookup (data->meta_data, "Size");
    data->size = g_variant_get_uint64 (value);

    value = g_hash_table_lookup (data->meta_data, "ContentType");
    data->content_type = g_variant_get_string (value, NULL);

    port = soup_server_get_port (data->self->priv->http_server);

    result_data = g_new0 (HostFileResult, 1);
    result_data->params = data->meta_data;
    result_data->uri = host_data_get_uri (data, data->iface, port);

    g_simple_async_result_set_op_res_gpointer (result, result_data, g_free);

    data->result = NULL;

out:
    g_free (data->iface);
    data->iface = NULL;

    g_simple_async_result_complete_in_idle (result);
    g_object_unref (result);
    g_object_unref (sender);
}

void
korva_upnp_file_server_host_file_async (KorvaUPnPFileServer *self,
                                        GFile *file,
                                        GHashTable *params,
                                        const char *iface,
                                        const char *peer,
                                        GAsyncReadyCallback callback,
                                        gpointer user_data)
{
    HostData *data;
    GSimpleAsyncResult *result;
    guint port;
    HostFileResult *result_data;
    KorvaUPnPMetadataQuery *query;

    result = g_simple_async_result_new (G_OBJECT (self),
                                        callback,
                                        user_data,
                                        (gpointer) korva_upnp_file_server_host_file_async);

    data = g_hash_table_lookup (self->priv->host_data, file);
    if (data == NULL) {
        data = host_data_new (file, params, peer);
        data->self = self;
        data->result = result;
        data->iface = g_strdup (iface);

        query = korva_upnp_metadata_query_new (file, params);
        korva_upnp_metadata_query_run_async (query,
                                             korva_upnp_file_server_on_metadata_query_run_done,
                                             NULL,
                                             data);

        return;
    }

    host_data_add_peer (data, peer);

    port = soup_server_get_port (self->priv->http_server);

    result_data = g_new0 (HostFileResult, 1);
    result_data->params = data->meta_data;
    result_data->uri = host_data_get_uri (data, iface, port);

    g_simple_async_result_set_op_res_gpointer (result, result_data, g_free);

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

    *params = NULL;

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

gboolean
korva_upnp_file_server_idle (KorvaUPnPFileServer *self)
{
    return g_hash_table_size (self->priv->host_data) == 0;
}

void
korva_upnp_file_server_unhost_file_by_peer (KorvaUPnPFileServer *self,
                                            const char *peer)
{
    GHashTableIter iter;
    HostData *value;
    char *key;

    g_hash_table_iter_init (&iter, self->priv->host_data);
    while (g_hash_table_iter_next (&iter, (gpointer) &key, (gpointer) &value)) {
        host_data_remove_peer (value, peer);
        if (value->peers == NULL) {
            char *id, *uri;

            id = host_data_get_id (value);
            g_hash_table_remove (self->priv->id_map, id);
            g_free (id);

            uri = g_file_get_uri (value->file);
            g_debug ("File '%s' no longer shared to any peer, removing…",
                     uri);
            g_free (uri);

            g_hash_table_iter_remove (&iter);
        }
    }
}
