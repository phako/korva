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

#define KORVA_TYPE_UPNP_FILE_SERVER             (korva_upnp_file_server_get_type ())
G_DECLARE_FINAL_TYPE (KorvaUPnPFileServer, korva_upnp_file_server, KORVA, UPNP_FILE_SERVER, GObject)


KorvaUPnPFileServer *
korva_upnp_file_server_get_default (void);

void
korva_upnp_file_server_host_file_async (KorvaUPnPFileServer *self,
                                        GFile               *file,
                                        GHashTable          *params,
                                        const char          *iface,
                                        const char          *peer,
                                        GCancellable        *cancellable,
                                        GAsyncReadyCallback  callback,
                                        gpointer             user_data);

char *
korva_upnp_file_server_host_file_finish (KorvaUPnPFileServer *self,
                                         GAsyncResult        *result,
                                         GHashTable         **params,
                                         GError             **error);

gboolean
korva_upnp_file_server_idle (KorvaUPnPFileServer *self);

void
korva_upnp_file_server_unhost_by_peer (KorvaUPnPFileServer *self,
                                       const char          *peer);

void
korva_upnp_file_server_unhost_file_for_peer (KorvaUPnPFileServer *self,
                                             GFile               *file,
                                             const char          *peer);
G_END_DECLS
