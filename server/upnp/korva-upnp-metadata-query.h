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

#pragma once

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define KORVA_TYPE_UPNP_METADATA_QUERY             (korva_upnp_metadata_query_get_type ())
G_DECLARE_FINAL_TYPE(KorvaUPnPMetadataQuery, korva_upnp_metadata_query, KORVA, UPNP_METADATA_QUERY, GObject)

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
