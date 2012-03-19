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

#ifndef __KORVA_SERVER_H__
#define __KORVA_SERVER_H__

#include <glib-object.h>

G_BEGIN_DECLS

GType korva_server_get_type (void);

#define KORVA_TYPE_SERVER \
    (korva_server_get_type ())
#define KORVA_SERVER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), KORVA_TYPE_SERVER, KorvaServer))
#define KORVA_IS_SERVER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), KORVA_TYPE_SERVER))
#define KORVA_SERVER_CLASS(obj) \
    (G_TYPE_CHECK_CLASS_CAST ((obj), KORVA_TYPE_SERVER, KorvaServerClass))
#define KORVA_IS_SERVER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), KORVA_TYPE_SERVER))
#define KORVA_SERVER_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), KORVA_TYPE_SERVER, KorvaServerClass))

typedef struct _KorvaServerPrivate KorvaServerPrivate;
typedef struct _KorvaServer KorvaServer;
typedef struct _KorvaServerClass KorvaServerClass;

struct _KorvaServer {
    GObject parent;

    KorvaServerPrivate *priv;
};

struct _KorvaServerClass {
    GObjectClass parent_class;
};

KorvaServer *
korva_server_new (void);

void korva_server_run (KorvaServer *self);

G_END_DECLS

#endif /* __KORVA_SERVER_H__ */
