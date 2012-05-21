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

#include <libgupnp-av/gupnp-av.h>

#include "korva-upnp-constants-private.h"
#include "korva-upnp-host-data.h"

G_DEFINE_TYPE (KorvaUPnPHostData, korva_upnp_host_data, G_TYPE_OBJECT)

struct _KorvaUPnPHostDataPrivate {
    GFile      *file;
    GHashTable *meta_data;
    char       *protocol_info;
    GList      *peers;
    uint        timeout_id;
};

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

static void
korva_upnp_host_data_class_init (KorvaUPnPHostDataClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (KorvaUPnPHostDataPrivate));

    object_class->constructed = korva_upnp_host_data_constructed;
    object_class->set_property = korva_upnp_host_data_set_property;
    object_class->dispose = korva_upnp_host_data_dispose;
    object_class->finalize = korva_upnp_host_data_finalize;

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

    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                              KORVA_TYPE_UPNP_HOST_DATA,
                                              KorvaUPnPHostDataPrivate);
}

static void
korva_upnp_host_data_finalize (GObject *object)
{
    KorvaUPnPHostData *self = KORVA_UPNP_HOST_DATA (object);

    if (self->priv->peers != NULL) {
        g_list_free_full (self->priv->peers, g_free);
    }

    if (self->priv->protocol_info != NULL) {
        g_free (self->priv->protocol_info);
        self->priv->protocol_info = NULL;
    }
    
    G_OBJECT_CLASS (korva_upnp_host_data_parent_class)->finalize (object);
}

static void
korva_upnp_host_data_dispose (GObject *object)
{
    KorvaUPnPHostData *self = KORVA_UPNP_HOST_DATA (object);

    korva_upnp_host_data_cancel_timeout (self);

    g_clear_object (&(self->priv->file));

    if (self->priv->meta_data != NULL) {
        g_hash_table_unref (self->priv->meta_data);
        self->priv->meta_data = NULL;
    }
    
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
KorvaUPnPHostData *
korva_upnp_host_data_new (GFile *file, GHashTable *meta_data, const char *address)
{
    return g_object_new (KORVA_TYPE_UPNP_HOST_DATA,
                         "file", file,
                         "meta-data", meta_data,
                         "address", address,
                         NULL);
}

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

char *
korva_upnp_host_data_get_id (KorvaUPnPHostData *self)
{
    char *hash, *uri;

    uri = g_file_get_uri (self->priv->file);
    hash = g_compute_checksum_for_string (G_CHECKSUM_MD5, uri, -1);
    g_free (uri);

    return hash;
}

char *
korva_upnp_host_data_get_uri (KorvaUPnPHostData *self, const char *iface, guint port)
{
    char *hash, *result;

    hash = korva_upnp_host_data_get_id (self);

    result = g_strdup_printf ("http://%s:%u/item/%s",
                              iface,
                              port,
                              hash);

    g_free (hash);

    return result;
}

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

gboolean
korva_upnp_host_data_valid_for_peer (KorvaUPnPHostData *self, const char *peer)
{
    GList *it;

    it = g_list_find_custom (self->priv->peers, peer, (GCompareFunc) g_strcmp0);

    return it != NULL;
}

void
korva_upnp_host_data_start_timeout (KorvaUPnPHostData *self)
{
    korva_upnp_host_data_cancel_timeout (self);

    self->priv->timeout_id = g_timeout_add_seconds (KORVA_UPNP_FILE_SERVER_DEFAULT_TIMEOUT,
                                                    korva_upnp_host_data_on_timeout,
                                                    self);
}

void
korva_upnp_host_data_cancel_timeout (KorvaUPnPHostData *self)
{
    if (self->priv->timeout_id != 0) {
        g_source_remove (self->priv->timeout_id);
        self->priv->timeout_id = 0;
    }
}

/* korva_upnp_host_data_get_file:
 *
 * Get a #GFile that points to the file that is represented by this host data.
 *
 * Returns: (transfer full): A #GFile. Use g_object_unref() on the file after
 * done with it.
 */
GFile *
korva_upnp_host_data_get_file (KorvaUPnPHostData *self)
{
    return g_object_ref (self->priv->file);
}

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

GVariant *
korva_upnp_host_data_lookup_meta_data (KorvaUPnPHostData *self, const char *key)
{
    return g_hash_table_lookup (self->priv->meta_data, key);
}

GHashTable *
korva_upnp_host_data_get_meta_data (KorvaUPnPHostData *self)
{
    return self->priv->meta_data;
}

const char *
korva_upnp_host_data_get_content_type (KorvaUPnPHostData *self)
{
    GVariant *value;

    value = g_hash_table_lookup (self->priv->meta_data, "ContentType");

    return g_variant_get_string (value, NULL);
}

gboolean
korva_upnp_host_data_has_peers (KorvaUPnPHostData *self)
{
    return self->priv->peers != NULL;
}