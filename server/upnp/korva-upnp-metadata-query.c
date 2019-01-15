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

#include <korva-error.h>

#include "korva-upnp-metadata-query.h"

enum {
    PROP_0,
    PROP_FILE,
    PROP_PARAMS
};

struct _KorvaUPnPMetadataQueryPrivate {
    GFile        *file;
    GTask        *result;
    GHashTable   *params;
};

G_DEFINE_TYPE (KorvaUPnPMetadataQuery, korva_upnp_metadata_query, G_TYPE_OBJECT);

/* Forward declarations */
static void
korva_upnp_metadata_query_on_file_query_info_async (GObject      *source,
                                                    GAsyncResult *res,
                                                    gpointer      user_data);

static void
korva_upnp_metadata_query_init (KorvaUPnPMetadataQuery *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                              KORVA_TYPE_UPNP_METADATA_QUERY,
                                              KorvaUPnPMetadataQueryPrivate);
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

    switch (prop_id) {
        case PROP_FILE:
            self->priv->file = g_value_dup_object (value);
            break;
        case PROP_PARAMS:
            self->priv->params = g_value_dup_boxed (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
korva_upnp_metadata_query_class_init (KorvaUPnPMetadataQueryClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (KorvaUPnPMetadataQueryPrivate));

    object_class->finalize = korva_upnp_metadata_query_finalize;
    object_class->set_property = korva_upnp_metadata_query_set_property;

    g_object_class_install_property (object_class,
                                     PROP_FILE,
                                     g_param_spec_object ("file",
                                                          "file",
                                                          "file",
                                                          G_TYPE_FILE,
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
                                                         G_PARAM_WRITABLE |
                                                         G_PARAM_CONSTRUCT |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));
}


KorvaUPnPMetadataQuery *
korva_upnp_metadata_query_new (GFile *file, GHashTable *params)
{
    return KORVA_UPNP_METADATA_QUERY (g_object_new (KORVA_TYPE_UPNP_METADATA_QUERY,
                                                    "file", file,
                                                    "params", params,
                                                    NULL));
}

void
korva_upnp_metadata_query_run_async (KorvaUPnPMetadataQuery *self, GAsyncReadyCallback callback, GCancellable *cancellable, gpointer user_data)
{
    self->priv->result = g_task_new (self, cancellable, callback, user_data);

    g_file_query_info_async (self->priv->file,
                             G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ","
                             G_FILE_ATTRIBUTE_STANDARD_SIZE ","
                             G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME ","
                             G_FILE_ATTRIBUTE_ACCESS_CAN_READ,
                             G_FILE_QUERY_INFO_NONE,
                             G_PRIORITY_DEFAULT_IDLE,
                             g_task_get_cancellable (self->priv->result),
                             korva_upnp_metadata_query_on_file_query_info_async,
                             self);
}

gboolean
korva_upnp_metadata_query_run_finish (KorvaUPnPMetadataQuery *self, GAsyncResult *res, GError **error)
{
    if (!g_task_is_valid (G_TASK (res), G_OBJECT (self))) {
        return FALSE;
    }

    return g_task_propagate_boolean (G_TASK (res), error);
}

static void
korva_upnp_metadata_query_on_file_query_info_async (GObject      *source,
                                                    GAsyncResult *res,
                                                    gpointer      user_data)
{
    KorvaUPnPMetadataQuery *self = KORVA_UPNP_METADATA_QUERY (user_data);
    GError *error = NULL;
    GVariant *value;
    GFileInfo *info;
    gboolean can_read;
    goffset size;

    info = g_file_query_info_finish (self->priv->file, res, &error);
    if (info == NULL) {
        g_task_return_error (self->priv->result, error);

        goto out;
    }

    can_read = g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ);
    if (!can_read) {
        g_task_return_new_error (self->priv->result,
                                 KORVA_CONTROLLER1_ERROR,
                                 KORVA_CONTROLLER1_ERROR_NOT_ACCESSIBLE,
                                 "%s",
                                 "Can not read file");

        goto out;
    }

    value = g_hash_table_lookup (self->priv->params, "Size");

    size = g_file_info_get_size (info);
    g_hash_table_replace (self->priv->params,
                          g_strdup ("Size"),
                          g_variant_new_uint64 (size));

    value = g_hash_table_lookup (self->priv->params, "ContentType");
    if (value == NULL) {
        const char *content_type = g_file_info_get_content_type (info);
        g_hash_table_insert (self->priv->params,
                             g_strdup ("ContentType"),
                             g_variant_new_string (content_type));
    }

    value = g_hash_table_lookup (self->priv->params, "Title");
    if (value == NULL) {
        g_hash_table_insert (self->priv->params,
                             g_strdup ("Title"),
                             g_variant_new_string (g_file_info_get_display_name (info)));
    }

    g_task_return_boolean (self->priv->result, TRUE);
out:
    g_clear_object (&info);

    g_object_unref (G_OBJECT (self->priv->result));
}
