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

#ifndef _KORVA_UPNP_METADATA_QUERY_H_
#define _KORVA_UPNP_METADATA_QUERY_H_

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define KORVA_TYPE_UPNP_METADATA_QUERY             (korva_upnp_metadata_query_get_type ())
#define KORVA_UPNP_METADATA_QUERY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), KORVA_TYPE_UPNP_METADATA_QUERY, KorvaUPnPMetadataQuery))
#define KORVA_UPNP_METADATA_QUERY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), KORVA_TYPE_UPNP_METADATA_QUERY, KorvaUPnPMetadataQueryClass))
#define KORVA_IS_UPNP_METADATA_QUERY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), KORVA_TYPE_UPNP_METADATA_QUERY))
#define KORVA_IS_UPNP_METADATA_QUERY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), KORVA_TYPE_UPNP_METADATA_QUERY))
#define KORVA_UPNP_METADATA_QUERY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), KORVA_TYPE_UPNP_METADATA_QUERY, KorvaUPnPMetadataQueryClass))

typedef struct _KorvaUPnPMetadataQueryClass KorvaUPnPMetadataQueryClass;
typedef struct _KorvaUPnPMetadataQuery KorvaUPnPMetadataQuery;
typedef struct _KorvaUPnPMetadataQueryPrivate KorvaUPnPMetadataQueryPrivate;

struct _KorvaUPnPMetadataQueryClass {
    GObjectClass parent_class;
};

struct _KorvaUPnPMetadataQuery {
    GObject                        parent_instance;

    KorvaUPnPMetadataQueryPrivate *priv;
};

GType korva_upnp_metadata_query_get_type (void) G_GNUC_CONST;

KorvaUPnPMetadataQuery *
korva_upnp_metadata_query_new (GFile *file, GHashTable *params);

void
korva_upnp_metadata_query_run_async (KorvaUPnPMetadataQuery *query,
                                     GAsyncReadyCallback     callback,
                                     GCancellable           *cancellable,
                                     gpointer                user_data);

gboolean
korva_upnp_metadata_query_run_finish (KorvaUPnPMetadataQuery *query,
                                      GAsyncResult           *res,
                                      GError                **error);

G_END_DECLS

#endif /* _KORVA_UPNP_METADATA_QUERY_H_ */
