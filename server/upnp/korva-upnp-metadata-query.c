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

#include <tracker-sparql.h>

#include "korva-upnp-metadata-query.h"

#define ITEM_QUERY \
        "SELECT " \
        "    ?r " \
        "    rdf:type(?r) " \
        "    %s " \
        "    nmm:dlnaProfile(?r) " \
        "    nfo:fileSize(?r) " \
        "    nie:title(?r) " \
        "    nfo:width(?r) " \
        "    nfo:height(?r) " \
        "{ ?r nie:url '%s' } "

#define PR1_2_CONTENT_TYPE_QUERY "tracker:coalesce(nmm:dlnaMime(?r), nie:mimeType(?r))"
#define PR1_0_CONTENT_TYPE_QUERY "nie:mimeType(?r)"

enum
{
    PROP_0,
    PROP_FILE,
    PROP_PARAMS,
    PROP_VERSION
};

struct _KorvaUPnPMetadataQueryPrivate {
    GFile              *file;
    GSimpleAsyncResult *result;
    GCancellable       *cancellable;
    GHashTable          *params;
    TrackerSparqlCursor *cursor;
    TrackerSparqlConnection *connection;
    int                      version;
};

G_DEFINE_TYPE (KorvaUPnPMetadataQuery, korva_upnp_metadata_query, G_TYPE_OBJECT);

/* Forward declarations */
static void
korva_upnp_metadata_query_on_sparql_connection_get (GObject      *source,
                                                    GAsyncResult *res,
                                                    gpointer      user_data);

static void
korva_upnp_metadata_query_on_query_async (GObject *source,
                                           GAsyncResult *res,
                                           gpointer user_data);

static void
korva_upnp_metadata_query_on_cursor_next (GObject *source,
                                          GAsyncResult *res,
                                          gpointer user_data);

static void
korva_upnp_metadata_query_init (KorvaUPnPMetadataQuery *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                              KORVA_TYPE_UPNP_METADATA_QUERY,
                                              KorvaUPnPMetadataQueryPrivate);
}

static void
korva_upnp_metadata_query_dispose (GObject *object)
{
    KorvaUPnPMetadataQuery *self = KORVA_UPNP_METADATA_QUERY (object);

    if (self->priv->cursor != NULL) {
        g_object_unref (self->priv->cursor);
        self->priv->cursor = NULL;
    }

    if (self->priv->connection != NULL) {
       g_object_unref (self->priv->connection);
       self->priv->connection = NULL;
    }

    G_OBJECT_CLASS (korva_upnp_metadata_query_parent_class)->dispose (object);
}

static void
korva_upnp_metadata_query_finalize (GObject *object)
{
    KorvaUPnPMetadataQuery *self = KORVA_UPNP_METADATA_QUERY (object);

    if (self->priv->file != NULL) {
        g_object_unref (self->priv->file);
        self->priv->file = NULL;
    }

    if (self->priv->params != NULL) {
        g_hash_table_unref (self->priv->params);
        self->priv->params = NULL;
    }

    G_OBJECT_CLASS (korva_upnp_metadata_query_parent_class)->finalize (object);
}

static void
korva_upnp_metadata_query_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    KorvaUPnPMetadataQuery *self = KORVA_UPNP_METADATA_QUERY (object);

    g_return_if_fail (KORVA_IS_UPNP_METADATA_QUERY (object));

    switch (prop_id)
    {
    case PROP_FILE:
        self->priv->file = g_value_dup_object (value);
        break;
    case PROP_PARAMS:
        self->priv->params = g_value_dup_boxed (value);
        break;
    case PROP_VERSION:
        self->priv->version = g_value_get_int (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
korva_upnp_metadata_query_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    g_return_if_fail (KORVA_IS_UPNP_METADATA_QUERY (object));
    KorvaUPnPMetadataQuery *self = KORVA_UPNP_METADATA_QUERY (object);

    switch (prop_id)
    {
    case PROP_FILE:
        g_value_set_object (value, self->priv->file);
        break;
    case PROP_PARAMS:
        g_value_set_boxed (value, self->priv->params);
        break;
    case PROP_VERSION:
        g_value_set_int (value, self->priv->version);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
korva_upnp_metadata_query_class_init (KorvaUPnPMetadataQueryClass *klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (KorvaUPnPMetadataQueryPrivate));

    object_class->finalize = korva_upnp_metadata_query_finalize;
    object_class->dispose = korva_upnp_metadata_query_dispose;
    object_class->set_property = korva_upnp_metadata_query_set_property;
    object_class->get_property = korva_upnp_metadata_query_get_property;

    g_object_class_install_property (object_class,
                                     PROP_FILE,
                                     g_param_spec_object ("file",
                                                          "file",
                                                          "file",
                                                          G_TYPE_FILE,
                                                          G_PARAM_READABLE |
                                                          G_PARAM_WRITABLE |
                                                          G_PARAM_CONSTRUCT_ONLY |
                                                          G_PARAM_STATIC_NAME |
                                                          G_PARAM_STATIC_NICK |
                                                          G_PARAM_STATIC_BLURB));

    g_object_class_install_property (object_class,
                                     PROP_PARAMS,
                                     g_param_spec_boxed ("params",
                                                          "params",
                                                          "params",
                                                          G_TYPE_HASH_TABLE,
                                                          G_PARAM_READABLE |
                                                          G_PARAM_WRITABLE |
                                                          G_PARAM_CONSTRUCT |
                                                          G_PARAM_STATIC_NAME |
                                                          G_PARAM_STATIC_NICK |
                                                          G_PARAM_STATIC_BLURB));

    g_object_class_install_property (object_class,
                                     PROP_VERSION,
                                     g_param_spec_int ("version",
                                                       "version",
                                                       "version",
                                                       0,
                                                       20,
                                                       10,
                                                       G_PARAM_READABLE |
                                                       G_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT |
                                                       G_PARAM_STATIC_NAME |
                                                       G_PARAM_STATIC_NICK |
                                                       G_PARAM_STATIC_BLURB));
}


KorvaUPnPMetadataQuery*
korva_upnp_metadata_query_new (GFile *file, GHashTable *params, int version)
{
    return KORVA_UPNP_METADATA_QUERY (g_object_new (KORVA_TYPE_UPNP_METADATA_QUERY,
                                                    "file", file,
                                                    "params", params,
                                                    "version", version,
                                                    NULL));
}

void
korva_upnp_metadata_query_run_async (KorvaUPnPMetadataQuery *self, GAsyncReadyCallback callback, GCancellable *cancellable, gpointer user_data)
{
    self->priv->result = g_simple_async_result_new (G_OBJECT (self),
                                                    callback,
                                                    user_data,
                                                    (gpointer) korva_upnp_metadata_query_run_async);
    self->priv->cancellable = cancellable;

    tracker_sparql_connection_get_async (cancellable,
                                         korva_upnp_metadata_query_on_sparql_connection_get,
                                         self);
}

gboolean
korva_upnp_metadata_query_run_finish (KorvaUPnPMetadataQuery *self, GAsyncResult *res, GError **error)
{
    GSimpleAsyncResult *result;

    if (!g_simple_async_result_is_valid (res,
                                         G_OBJECT (self),
                                         korva_upnp_metadata_query_run_async)) {
        return FALSE;
    }

    result = (GSimpleAsyncResult *) res;
    if (g_simple_async_result_propagate_error (result, error)) {
        return FALSE;
    }

    return TRUE;
}

static void
korva_upnp_metadata_query_on_sparql_connection_get (GObject      *source,
                                                    GAsyncResult *res,
                                                    gpointer      user_data)
{
    KorvaUPnPMetadataQuery *self = KORVA_UPNP_METADATA_QUERY (user_data);
    GError                 *error = NULL;
    char *query, *uri;

    self->priv->connection = tracker_sparql_connection_get_finish (res, &error);
    if (self->priv->connection == NULL) {
        g_simple_async_result_take_error (self->priv->result, error);
        g_simple_async_result_complete_in_idle (self->priv->result);
        g_object_unref (self->priv->result);

        return;
    }

    uri = g_file_get_uri (self->priv->file);
    query = g_strdup_printf (ITEM_QUERY,
                             self->priv->version >= 12 ? PR1_2_CONTENT_TYPE_QUERY : PR1_0_CONTENT_TYPE_QUERY,
                             uri);
    g_free (uri);
    tracker_sparql_connection_query_async (self->priv->connection,
                                           query,
                                           self->priv->cancellable,
                                           korva_upnp_metadata_query_on_query_async,
                                           self);
    g_free (query);
}

static void
korva_upnp_metadata_query_on_query_async (GObject *source,
                                          GAsyncResult *res,
                                          gpointer user_data)
{
    KorvaUPnPMetadataQuery *self = KORVA_UPNP_METADATA_QUERY (user_data);
    GError                 *error = NULL;
    TrackerSparqlCursor *cursor;

    cursor = tracker_sparql_connection_query_finish (self->priv->connection,
                                                     res,
                                                     &error);

    if (cursor == NULL) {
        g_simple_async_result_take_error (self->priv->result, error);
        g_simple_async_result_complete_in_idle (self->priv->result);
        g_object_unref (self->priv->result);

        return;
    }

    self->priv->cursor = cursor;

    tracker_sparql_cursor_next_async (cursor,
                                      self->priv->cancellable,
                                      korva_upnp_metadata_query_on_cursor_next,
                                      self);
}

static void
korva_upnp_metadata_query_on_cursor_next (GObject *source,
                                          GAsyncResult *res,
                                          gpointer user_data)
{
    KorvaUPnPMetadataQuery *self = KORVA_UPNP_METADATA_QUERY (user_data);
    GError                 *error = NULL;
    gboolean next;
    const char *upnp_class, *value, *content_type, *dlna_profile;
    gint64 size;

    next = tracker_sparql_cursor_next_finish (self->priv->cursor,
                                              res,
                                              &error);
    if (error != NULL) {
        g_simple_async_result_take_error (self->priv->result, error);

        goto out;
    }

    if (!next) {
        goto out;
    }

    value = tracker_sparql_cursor_get_string (self->priv->cursor, 0, NULL);
    g_hash_table_insert (self->priv->params,
                         g_strdup ("Tracker:Id"),
                         g_variant_new_string (value));

    value = tracker_sparql_cursor_get_string (self->priv->cursor, 1, NULL);
    if (g_strstr_len (value, -1, "nmm#Video") != NULL) {
        upnp_class = "object.item.videoItem";
    } else if (g_strstr_len (value, -1, "nmm#MusicPiece") != NULL) {
        upnp_class = "object.item.audioItem.musicTrack";
    } else if (g_strstr_len (value, -1, "nmm#Photo") != NULL) {
        upnp_class = "object.item.imageItem.photo";
    }

    g_hash_table_insert (self->priv->params,
                         g_strdup ("UPnPClass"),
                         g_variant_new_string (upnp_class));

    content_type = tracker_sparql_cursor_get_string (self->priv->cursor, 2, NULL);
    g_hash_table_insert (self->priv->params,
                         g_strdup ("ContentType"),
                         g_variant_new_string (content_type));

    if (tracker_sparql_cursor_is_bound (self->priv->cursor, 3)) {
        value = tracker_sparql_cursor_get_string (self->priv->cursor, 3, NULL);
        g_hash_table_insert (self->priv->params,
                             g_strdup ("DLNAProfile"),
                             g_variant_new_string (value));
    } else {
        /* Simple DLNA profile guessing */
        if (g_strcmp0 (upnp_class, "object.item.videoItem") == 0) {
            char *uri;

            uri = g_file_get_uri (self->priv->file);
            if (g_strstr_len (uri, -1, "/DCIM/") != NULL &&
                g_strcmp0 (content_type, "video/mp4") == 0) {
                dlna_profile = "MPEG4_P2_MP4_SP_L6_AAC";
            }
            g_free (uri);
        } else if (g_strcmp0 (upnp_class, "object.item.imageItem.photo") == 0) {
            if (g_strcmp0 (content_type, "image/png") == 0) {
                dlna_profile = "PNG_LRG";
            } else if (g_strcmp0 (content_type, "image/jpeg") == 0) {
                gint64 width;
                gint64 height;

                width = tracker_sparql_cursor_get_integer (self->priv->cursor,
                                                           6);
                height = tracker_sparql_cursor_get_integer (self->priv->cursor,
                                                            7);
                if (width <= 640 && height <= 480) {
                    dlna_profile = "JPEG_SM";
                } else if (width <= 1024 && height <= 768) {
                    dlna_profile = "JPEG_MED";
                } else if (width <= 4096 && height <= 4096) {
                    dlna_profile = "JPEG_LRG";
                }
            }
        } else if (g_strcmp0 (upnp_class, "object.item.audioItem.musicTrack") == 0) {
            if (g_strcmp0 (content_type, "audio/mpeg") == 0) {
                /* Yeah... Best guess. Could really be anything */
                dlna_profile = "MP3";
            }
        }

        g_debug ("Guessed DLNA profile: %s", dlna_profile);

        g_hash_table_insert (self->priv->params,
                             g_strdup ("DLNAProfile"),
                             g_variant_new_string (dlna_profile));
    }

    size = tracker_sparql_cursor_get_integer (self->priv->cursor, 4);
    g_hash_table_insert (self->priv->params,
                         g_strdup ("Size"),
                         g_variant_new_uint64 (size));

    value = tracker_sparql_cursor_get_string (self->priv->cursor, 5, NULL);
    g_hash_table_insert (self->priv->params,
                         g_strdup ("Title"),
                         g_variant_new_string (value));

out:
    g_simple_async_result_complete_in_idle (self->priv->result);
    g_object_unref (self->priv->result);
}
