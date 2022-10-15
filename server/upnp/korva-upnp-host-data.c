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

#include <string.h>

#include <libgupnp-av/gupnp-av.h>

#include "korva-upnp-constants-private.h"
#include "korva-upnp-host-data.h"

struct _KorvaUPnPHostDataPrivate {
    GFile      *file;
    GHashTable *meta_data;
    char       *protocol_info;
    GList      *peers;
    uint        timeout_id;
    char       *extension;
    uint        request_count;
};
typedef struct _KorvaUPnPHostDataPrivate KorvaUPnPHostDataPrivate;

/**
 * KorvaUPnPHostData:
 *
 * Object used by #KorvaUPnPFileServer to describe a file that is currently shared.
 * The object keeps track of the peers the contained file is shared to and will
 * signalize via the ::timeout signal that the file has not been accessed or shared
 * to any device in #KORVA_UPNP_FILE_SERVER_DEFAULT_TIMEOUT seconds
 */
struct _KorvaUPnPHostData {
    GObject                   parent;

    KorvaUPnPHostDataPrivate *priv;
};

G_DEFINE_TYPE_WITH_PRIVATE (KorvaUPnPHostData, korva_upnp_host_data, G_TYPE_OBJECT)

enum KorvaUPnPHostDataProperties {
    PROP_0,
    PROP_FILE,
    PROP_META_DATA,
    PROP_ADDRESS
};

enum KorvaUPnPHostDataSignals {
    SIGNAL_TIMEOUT,
    SIGNAL_COUNT
};

/* Forward declarations */
/* GObject VFuncs */
static void
korva_upnp_host_data_finalize (GObject *object);

static void
korva_upnp_host_data_dispose (GObject *object);

static void
korva_upnp_host_data_constructed (GObject *object);

static void
korva_upnp_host_data_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec);

/* KorvaUPnPHostData private functions */
static gboolean
korva_upnp_host_data_on_timeout (gpointer user_data);

static const char *
korva_upnp_host_data_get_extension (KorvaUPnPHostData *self);

static void
korva_upnp_host_data_class_init (KorvaUPnPHostDataClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->constructed = korva_upnp_host_data_constructed;
    object_class->set_property = korva_upnp_host_data_set_property;
    object_class->dispose = korva_upnp_host_data_dispose;
    object_class->finalize = korva_upnp_host_data_finalize;

    /**
     * KorvaUPnPHostData:file:
     *
     * A #GFile pointing to the on-disk file this #KorvaUPnPHostData wraps
     */
    g_object_class_install_property (object_class,
                                     PROP_FILE,
                                     g_param_spec_object ("file",
                                                          "file",
                                                          "file",
                                                          G_TYPE_FILE,
                                                          G_PARAM_WRITABLE |
                                                          G_PARAM_CONSTRUCT_ONLY |
                                                          G_PARAM_STATIC_BLURB |
                                                          G_PARAM_STATIC_NAME |
                                                          G_PARAM_STATIC_NICK));

    /**
     * KorvaUPnPHostData:meta-data: (element-type utf8, Variant):
     *
     * A #GHashTable with string keys and #GVariant values holding various meta-data
     * information about the file such as
     * - URI
     * - Size
     * - Title
     * - ContentType
     * - ...
     */
    g_object_class_install_property (object_class,
                                     PROP_META_DATA,
                                     g_param_spec_boxed ("meta-data",
                                                         "meta-data",
                                                         "meta-data",
                                                         G_TYPE_HASH_TABLE,
                                                         G_PARAM_WRITABLE |
                                                         G_PARAM_CONSTRUCT_ONLY |
                                                         G_PARAM_STATIC_BLURB |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK));

    /**
     * KorvaUPnPHostData:address:
     *
     * An IP address in string representation that denotes the peer the file was
     * shared first to. It's added to the list of peers in the private structure.
     */
    g_object_class_install_property (object_class,
                                     PROP_ADDRESS,
                                     g_param_spec_string ("address",
                                                          "address",
                                                          "address",
                                                          NULL,
                                                          G_PARAM_WRITABLE |
                                                          G_PARAM_CONSTRUCT_ONLY |
                                                          G_PARAM_STATIC_BLURB |
                                                          G_PARAM_STATIC_NAME |
                                                          G_PARAM_STATIC_NICK));

    /**
     * KorvaUPnPHostData::timeout:
     *
     * Triggered if the file was not accessed by any peer during the last
     * #KORVA_UPNP_FILE_SERVER_DEFAULT_TIMEOUT seconds.
     */
    g_signal_new ("timeout",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0,
                  NULL);
}

static void
korva_upnp_host_data_init (KorvaUPnPHostData *self)
{

    self->priv = korva_upnp_host_data_get_instance_private (self);
}

static void
peer_list_free (GList *list)
{
    g_list_free_full (list, g_free);
}

static void
korva_upnp_host_data_finalize (GObject *object)
{
    KorvaUPnPHostData *self = KORVA_UPNP_HOST_DATA (object);

    g_clear_pointer (&self->priv->peers, peer_list_free);
    g_clear_pointer (&self->priv->protocol_info, g_free);
    g_clear_pointer (&self->priv->extension, g_free);

    G_OBJECT_CLASS (korva_upnp_host_data_parent_class)->finalize (object);
}

static void
korva_upnp_host_data_dispose (GObject *object)
{
    KorvaUPnPHostData *self = KORVA_UPNP_HOST_DATA (object);

    korva_upnp_host_data_cancel_timeout (self);

    g_clear_object (&(self->priv->file));
    g_clear_pointer (&self->priv->meta_data, g_hash_table_unref);

    G_OBJECT_CLASS (korva_upnp_host_data_parent_class)->dispose (object);
}

static void
korva_upnp_host_data_constructed (GObject *object)
{
    KorvaUPnPHostData *self = KORVA_UPNP_HOST_DATA (object);

    korva_upnp_host_data_start_timeout (self);
}

static void
korva_upnp_host_data_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
    KorvaUPnPHostData *self = KORVA_UPNP_HOST_DATA (object);

    switch (property_id) {
        case PROP_FILE:
            self->priv->file = g_value_dup_object (value);
            break;
        case PROP_META_DATA:
            self->priv->meta_data = g_value_dup_boxed (value);
            break;
        case PROP_ADDRESS:
            self->priv->peers = g_list_prepend (self->priv->peers, g_value_dup_string (value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

/**
 * korva_upnp_host_data_on_timeout:
 *
 * File has not been shared or accessed by a known peer for #KORVA_UPNP_FILE_SERVER_DEFAULT_TIMEOUT seconds.
 * Emit the ::timeout signal so the #KorvaUPnPFileServer can remove the file from
 * its HTTP server.
 */
static gboolean
korva_upnp_host_data_on_timeout (gpointer user_data)
{
    char *uri;

    KorvaUPnPHostData *self = KORVA_UPNP_HOST_DATA (user_data);

    self->priv->timeout_id = 0;
    uri = g_file_get_uri (self->priv->file);
    g_debug ("File '%s' was not accessed for %d seconds; removing.",
             uri,
             KORVA_UPNP_FILE_SERVER_DEFAULT_TIMEOUT);
    g_free (uri);

    g_signal_emit_by_name (self, "timeout", NULL);

    return FALSE;
}

/* KorvaUPnPHostData public function implementation */

/**
 * korva_upnp_host_data_new:
 *
 * Create a new instance of #KorvaUPnPHostData.
 *
 * @file: A #GFile this #KorvaUPnPHostData represents.
 * @meta_data: (element-type utf-8, Variant): A #GHashTable containing meta-data
     information about @file.
 * @address: An IP address for the remote device the @file will be shared to initially.
 *
 * Returns: A new instance of #KorvaUPnPHostData.
 */
KorvaUPnPHostData *
korva_upnp_host_data_new (GFile *file, GHashTable *meta_data, const char *address)
{
    return g_object_new (KORVA_TYPE_UPNP_HOST_DATA,
                         "file", file,
                         "meta-data", meta_data,
                         "address", address,
                         NULL);
}

/**
 * korva_upnp_host_data_add_peer:
 *
 * Add a remote device to the list of peers that are allowed to access the :file
 * in this #KorvaUPnPHostData.
 *
 * @self: An instance of #KorvaUPnPHostData
 * @peer: An IP address of a remote device which shall gain access to the :file
 */
void
korva_upnp_host_data_add_peer (KorvaUPnPHostData *self, const char *peer)
{
    GList *it;

    it = g_list_find_custom (self->priv->peers, peer, (GCompareFunc) g_strcmp0);
    if (it != NULL) {
        return;
    }

    self->priv->peers = g_list_prepend (self->priv->peers, g_strdup (peer));
}

/**
 * korva_upnp_host_data_remove_peer:
 *
 * Revoke access for a remote device to the :file.
 *
 * @self: An instance of #KorvaUPnPHostData
 * @peer: IP address of the remote device which shall not have access to the
 * device anymore.
 */
void
korva_upnp_host_data_remove_peer (KorvaUPnPHostData *self, const char *peer)
{
    GList *it;

    it = g_list_find_custom (self->priv->peers, peer, (GCompareFunc) g_strcmp0);
    if (it != NULL) {
        g_free (it->data);

        self->priv->peers = g_list_remove_link (self->priv->peers, it);
    }
}

/**
 * korva_upnp_host_data_get_id:
 *
 * Get a uniquish identifier for this #KorvaUPnPHostData.
 * It is used to generate the URLs served by #KorvaUPnPFileServer.
 *
 * @self: An instance of #KorvaUPnPHostData
 *
 * Returns: (transfer full): A new string containing the id. Free after use.
 */
char *
korva_upnp_host_data_get_id (KorvaUPnPHostData *self)
{
    char *hash, *uri;

    uri = g_file_get_uri (self->priv->file);
    hash = g_compute_checksum_for_string (G_CHECKSUM_MD5, uri, -1);
    g_free (uri);

    return hash;
}

/**
 * korva_upnp_host_data_get_uri:
 *
 * Create a HTTP URL for this :file.
 *
 * @self: An instance of #KorvaUPnPHostData
 * @iface: IP address of the network interface the HTTP server is running on.
 * @port: TCP port the HTTP server is listening on.
 *
 * Returns: (transfer full): A new string containing the uri. Free after use.
 */
char *
korva_upnp_host_data_get_uri (KorvaUPnPHostData *self, const char *iface, guint port)
{
    char *hash, *result;
    const char *ext;

    hash = korva_upnp_host_data_get_id (self);

    ext = korva_upnp_host_data_get_extension (self);

    result = g_strdup_printf ("http://%s:%u/item/%s.%s",
                              iface,
                              port,
                              hash,
                              ext);

    g_free (hash);

    return result;
}

/**
 * korva_upnp_host_data_get_protocol_info:
 *
 * Get the UPnP-AV/DLNA protocol info string for the :file. The transport is
 * always "http-get". The ci-param is "0" (original source) and op-param is "01"
 * (byte seek only). If :meta-data contains a DLNA profile it will be added as
 * well as the the file's content type.
 *
 * The function lazy-creates the string when used for the first time.
 *
 * @self: An instance of #KorvaUPnPHostData
 *
 * Returns: (transfer none): A protocol info string.
 */
const char *
korva_upnp_host_data_get_protocol_info (KorvaUPnPHostData *self)
{
    if (self->priv->protocol_info == NULL) {
        GVariant *value;
        const char *dlna_profile;
        GUPnPProtocolInfo *info;

        info = gupnp_protocol_info_new_from_string ("http-get:*:*:DLNA.ORG_CI=0;DLNA.ORG_OP=01", NULL);
        gupnp_protocol_info_set_mime_type (info,
                                           korva_upnp_host_data_get_content_type (self));

        value = g_hash_table_lookup (self->priv->meta_data, "DLNAProfile");
        if (value != NULL) {
            dlna_profile = g_variant_get_string (value, NULL);
            gupnp_protocol_info_set_dlna_profile (info, dlna_profile);
        }

        self->priv->protocol_info = gupnp_protocol_info_to_string (info);
        g_object_unref (info);
    }

    return self->priv->protocol_info;
}

/**
 * korva_upnp_host_data_valid_for_peer:
 *
 * Check if a peer is supposed to be able to access the file.
 *
 * @self: An instance of #KorvaUPnPHostData
 *
 * Returns: %TRUE, if @peer is allowed, %FALSE otherwise.
 */
gboolean
korva_upnp_host_data_valid_for_peer (KorvaUPnPHostData *self, const char *peer)
{
    GList *it;

    it = g_list_find_custom (self->priv->peers, peer, (GCompareFunc) g_strcmp0);

    return it != NULL;
}

/**
 * korva_upnp_host_data_start_timeout:
 *
 * (Re-)start the idle timeout for this file.
 *
 * @self: An instance of #KorvaUPnPHostData
 */
void
korva_upnp_host_data_start_timeout (KorvaUPnPHostData *self)
{
    korva_upnp_host_data_cancel_timeout (self);

    self->priv->timeout_id = g_timeout_add_seconds (KORVA_UPNP_FILE_SERVER_DEFAULT_TIMEOUT,
                                                    korva_upnp_host_data_on_timeout,
                                                    self);
}

/**
 * korva_upnp_host_data_cancel_timeout:
 *
 * (Re-)start the idle timeout for this file.
 *
 * @self: An instance of #KorvaUPnPHostData
 */
void
korva_upnp_host_data_cancel_timeout (KorvaUPnPHostData *self)
{
    if (self->priv->timeout_id != 0) {
        g_source_remove (self->priv->timeout_id);
        self->priv->timeout_id = 0;
    }
}

/**
 * korva_upnp_host_data_get_file:
 *
 * Get a #GFile that points to the file that is represented by this host data.
 *
 * @self: An instance of #KorvaUPnPHostData
 *
 * Returns: (transfer full): A #GFile. Use g_object_unref() on the file after
 * done with it.
 */
GFile *
korva_upnp_host_data_get_file (KorvaUPnPHostData *self)
{
    return g_object_ref (self->priv->file);
}

/**
 * korva_upnp_host_data_get_size:
 *
 * Get the size of the file.
 *
 * @self: An instance of #KorvaUPnPHostData
 *
 * Returns: The size in bytes.
 */
goffset
korva_upnp_host_data_get_size (KorvaUPnPHostData *self)
{
    GVariant *value;

    value = g_hash_table_lookup (self->priv->meta_data, "Size");
    if (value == NULL) {
        return 0;
    }

    return g_variant_get_uint64 (value);
}

/**
 * korva_upnp_host_data_lookup_meta_data:
 *
 * Convenience wrapper for korva_upnp_host_data_get_meta_data() and
 *   g_hash_table_look_up(). Look up a meta-data key in :meta-data.
 *
 * @self: An instance of #KorvaUPnPHostData
 *
 * Returns: (transfer none): A #GVariant with the meta-data or %NULL if the key
 *   does not exist.
 */
GVariant *
korva_upnp_host_data_lookup_meta_data (KorvaUPnPHostData *self, const char *key)
{
    return g_hash_table_lookup (self->priv->meta_data, key);
}

/**
 * korva_upnp_host_data_get_meta_data:
 *
 * @self: An instance of #KorvaUPnPHostData
 *
 * Returns: (transfer none) (element-type utf8, Variant): A #GHashTable
 *   containing meta information about the file.
 */
GHashTable *
korva_upnp_host_data_get_meta_data (KorvaUPnPHostData *self)
{
    return self->priv->meta_data;
}

/**
 * korva_upnp_host_data_get_content_type:
 *
 * @self:  An instance of #KorvaUPnPHostData
 *
 * Returns: (transfer none): The content-type of the file.
 */
const char *
korva_upnp_host_data_get_content_type (KorvaUPnPHostData *self)
{
    GVariant *value;

    value = g_hash_table_lookup (self->priv->meta_data, "ContentType");

    return g_variant_get_string (value, NULL);
}

/**
 * korva_upnp_host_data_has_peers:
 *
 * Check if the file is shared to any remote device.
 *
 * @self:  An instance of #KorvaUPnPHostData
 * Returns: %TRUE, if still shared, %FALSE otherwise.
 */
gboolean
korva_upnp_host_data_has_peers (KorvaUPnPHostData *self)
{
    return self->priv->peers != NULL;
}

void
korva_upnp_host_data_add_request (KorvaUPnPHostData *self)
{
    self->priv->request_count++;
}

void
korva_upnp_host_data_remove_request (KorvaUPnPHostData *self)
{
    self->priv->request_count--;
}

gboolean
korva_upnp_host_data_has_requests (KorvaUPnPHostData *self)
{
    return self->priv->request_count != 0;
}

/**
 * korva_upnp_host_data_get_extension:
 *
 * Get the file extension for the current file.
 *
 * @self:  An instance of #KorvaUPnPHostData
 * Returns: (transfer none): The extension associated with the file.
 */
static const char *
korva_upnp_host_data_get_extension (KorvaUPnPHostData *self)
{
    GVariant *value;
    const char *dlna_profile, *content_type, *ext;
    char *path = NULL;

    if (self->priv->extension != NULL) {
        goto out;
    }

    path = g_file_get_path (self->priv->file);
    ext = g_strrstr (path, ".");
    if (!(ext == NULL || *ext == '\0' || strlen (++ext) > 4)) {
        self->priv->extension = g_strdup (ext);

        goto out;
    }

    value = g_hash_table_lookup (self->priv->meta_data, "DLNAProfile");
    if (value != NULL) {
        dlna_profile = g_variant_get_string (value, NULL);
        /* Images */
        if (g_strstr_len (dlna_profile, -1, "JPEG") != NULL) {
            self->priv->extension = g_strdup ("jpg");
        } else if (g_strstr_len (dlna_profile, -1, "PNG") != NULL) {
            self->priv->extension = g_strdup ("png");
        } else if (g_strstr_len (dlna_profile, -1, "GIF") != NULL) {
            self->priv->extension = g_strdup ("gif");
        } else if (g_strstr_len (dlna_profile, -1, "_MP4_") != NULL) {
            self->priv->extension = g_strdup ("mp4");
        } else if (g_strstr_len (dlna_profile, -1, "MP3") != NULL) {
            self->priv->extension = g_strdup ("mp3");
        } else if (g_strstr_len (dlna_profile, -1, "_ISO") != NULL) {
            self->priv->extension = g_strdup ("m4a");
        } else if (g_strstr_len (dlna_profile, -1, "ADTS") != NULL) {
            self->priv->extension = g_strdup ("adts");
        } else if (g_strstr_len (dlna_profile, -1, "WMV") != NULL) {
            self->priv->extension = g_strdup ("wmv");
        } else if (g_strstr_len (dlna_profile, -1, "WMA") != NULL) {
            self->priv->extension = g_strdup ("wma");
        } else if (g_strstr_len (dlna_profile, -1, "_TS_") != NULL ||
                   g_strstr_len (dlna_profile, -1, "_PS_") != NULL) {
            self->priv->extension = g_strdup ("mpg");
        } else if (g_strstr_len (dlna_profile, -1, "MKV") != NULL) {
            self->priv->extension = g_strdup ("mkv");
        }

        if (self->priv->extension != NULL) {
            goto out;
        }
    }

    value = g_hash_table_lookup (self->priv->meta_data, "ContentType");
    if (value != NULL) {
        content_type = g_variant_get_string (value, NULL);
        if (g_strcmp0 (content_type, "application/ogg") == 0 ||
            g_strstr_len (content_type, -1, "+ogg") != NULL) {
            self->priv->extension = g_strdup ("ogg");
        } else if (g_strcmp0 (content_type, "audio/ogg") == 0) {
            self->priv->extension = g_strdup ("oga");
        } else if (g_strcmp0 (content_type, "video/ogg") == 0) {
            self->priv->extension = g_strdup ("ogv");
        } else if (g_strcmp0 (content_type, "audio/x-matroska") == 0) {
            self->priv->extension = g_strdup ("mka");
        } else if (g_strcmp0 (content_type, "video/x-matroska") == 0) {
            self->priv->extension = g_strdup ("mkv");
        } else if (g_strcmp0 (content_type, "video/webm") == 0) {
            self->priv->extension = g_strdup ("webm");
        } else if (g_strcmp0 (content_type, "image/jpeg") == 0) {
            self->priv->extension = g_strdup ("jpg");
        } else if (g_strcmp0 (content_type, "image/png") == 0) {
            self->priv->extension = g_strdup ("png");
        } else if (g_strcmp0 (content_type, "image/gif") == 0) {
            self->priv->extension = g_strdup ("gif");
        } else if (g_strcmp0 (content_type, "audio/mpeg") == 0) {
            /* Could be anything, but let's use MP3 */
            self->priv->extension = g_strdup ("mp3");
        } else if (g_strrstr (content_type, "/mp4") != NULL) {
            self->priv->extension = g_strdup ("mp4");
        }

        if (self->priv->extension != NULL) {
            goto out;
        }
    }
    self->priv->extension = g_strdup ("dat");

out:
    if (path != NULL) {
        g_free (path);
    }

    return self->priv->extension;
}
