/*
    This file is part of Korva.

    Copyright (C) 2012 Jens Georg.
    Author: Jens Georg <mail@jensge.org>

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

#ifndef _KORVA_UPNP_FILE_POST_H_
#define _KORVA_UPNP_FILE_POST_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define KORVA_TYPE_UPNP_FILE_POST \
    (korva_upnp_file_post_get_type ())
#define KORVA_UPNP_FILE_POST(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), KORVA_TYPE_UPNP_FILE_POST, KorvaUPnPFilePost))
#define KORVA_IS_UPNP_FILE_POST(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), KORVA_TYPE_UPNP_FILE_POST))
#define KORVA_UPNP_FILE_POST_CLASS(obj) \
    (G_TYPE_CHECK_CLASS_CAST ((obj), KORVA_TYPE_UPNP_FILE_POST, KorvaUPnPFilePostClass))
#define KORVA_IS_UPNP_FILE_POST_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), KORVA_TYPE_UPNP_FILE_POST))
#define KORVA_UPNP_FILE_POST_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), KORVA_TYPE_UPNP_FILE_POST, KorvaUPnPFilePostClass))

typedef struct _KorvaUPnPFilePostPrivate KorvaUPnPFilePostPrivate;
typedef struct _KorvaUPnPFilePostClass KorvaUPnPFilePostClass;
typedef struct _KorvaUPnPFilePost KorvaUPnPFilePost;

GType korva_upnp_file_post_get_type (void) G_GNUC_CONST;

struct _KorvaUPnPFilePostClass {
    GObjectClass parent_class;
};

struct _KorvaUPnPFilePost {
    GObject parent;
    KorvaUPnPFilePostPrivate *priv;
};

KorvaUPnPFilePost *
korva_upnp_file_post_new (const char *uri,
                          GFile *file,
                          gint64 size,
                          const char *content_type,
                          SoupSession *session);

void
korva_upnp_file_post_run_async (KorvaUPnPFilePost   *post,
                                GCancellable        *cancellable,
                                GAsyncReadyCallback  callback,
                                gpointer             user_data);

gboolean
korva_upnp_file_post_finish    (KorvaUPnPFilePost  *post,
                                GAsyncResult       *result,
                                GError            **error);

G_END_DECLS

#endif /* _KORVA_UPNP_FILE_POST_H_ */
