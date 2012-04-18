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

#ifndef _KORVA_UPN_PFILE_SERVER_H_
#define _KORVA_UPN_PFILE_SERVER_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define KORVA_TYPE_UPNP_FILE_SERVER             (korva_upnp_file_server_get_type ())
#define KORVA_UPNP_FILE_SERVER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), KORVA_TYPE_UPNP_FILE_SERVER, KorvaUPnPFileServer))
#define KORVA_UPNP_FILE_SERVER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), KORVA_TYPE_UPNP_FILE_SERVER, KorvaUPnPFileServerClass))
#define KORVA_IS_UPNP_FILE_SERVER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), KORVA_TYPE_UPNP_FILE_SERVER))
#define KORVA_IS_UPNP_FILE_SERVER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), KORVA_TYPE_UPNP_FILE_SERVER))
#define KORVA_UPNP_FILE_SERVER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), KORVA_TYPE_UPNP_FILE_SERVER, KorvaUPnPFileServerClass))

typedef struct _KorvaUPnPFileServerClass KorvaUPnPFileServerClass;
typedef struct _KorvaUPnPFileServer KorvaUPnPFileServer;
typedef struct _KorvaUPnPFileServerPrivate KorvaUPnPFileServerPrivate;

struct _KorvaUPnPFileServerClass
{
    GObjectClass parent_class;
};

struct _KorvaUPnPFileServer
{
    GObject parent_instance;

    KorvaUPnPFileServerPrivate *priv;
};

GType
korva_upnp_file_server_get_type (void) G_GNUC_CONST;

KorvaUPnPFileServer *
korva_upnp_file_server_get_default (void);

void
korva_upnp_file_server_host_file_async (KorvaUPnPFileServer *self,
                                        GFile *file,
                                        GHashTable *params,
                                        const char *iface,
                                        const char *peer,
                                        GAsyncReadyCallback callback,
                                        gpointer user_data);

char *
korva_upnp_file_server_host_file_finish (KorvaUPnPFileServer  *self,
                                         GAsyncResult         *result,
                                         GHashTable          **params,
                                         GError              **error);

gboolean
korva_upnp_file_server_idle (KorvaUPnPFileServer *self);

void
korva_upnp_file_server_unhost_file_by_peer (KorvaUPnPFileServer *self,
                                            const char *peer);

G_END_DECLS

#endif /* _KORVA_UPN_PFILE_SERVER_H_ */
