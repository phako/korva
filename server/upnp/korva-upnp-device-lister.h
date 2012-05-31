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

#ifndef __KORVA_UPNP_DEVICE_LISTER_H__
#define __KORVA_UPNP_DEVICE_LISTER_H__

#include <glib-object.h>
#include "korva-device-lister.h"

GType korva_upnp_device_lister_get_type (void);

#define KORVA_TYPE_UPNP_DEVICE_LISTER \
    (korva_upnp_device_lister_get_type ())
#define KORVA_UPNP_DEVICE_LISTER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                 KORVA_TYPE_UPNP_DEVICE_LISTER, \
                                 KorvaUPnPDeviceLister))
#define KORVA_IS_UPNP_DEVICE_LISTER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                 KORVA_TYPE_UPNP_DEVICE_LISTER))
#define KORVA_UPNP_DEVICE_LISTER_CLASS(obj) \
    (G_TYPE_CHECK_CLASS_CAST ((obj), \
                              KORVA_TYPE_UPNP_DEVICE_LISTER, KorvaUPnPDeviceListerClass))
#define KORVA_IS_UPNP_DEVICE_LISTER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), KORVA_TYPE_UPNP_DEVICE_LISTER))
#define KORVA_UPNP_DEVICE_LISTER_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                                KORVA_TYPE_UPNP_DEVICE_LISTER, \
                                KorvaUPnPDeviceListerClass))

typedef struct _KorvaUPnPDeviceListerPrivate KorvaUPnPDeviceListerPrivate;
typedef struct _KorvaUPnPDeviceLister KorvaUPnPDeviceLister;
typedef struct _KorvaUPnPDeviceListerClass KorvaUPnPDeviceListerClass;

struct _KorvaUPnPDeviceLister {
    GObject                       parent;

    KorvaUPnPDeviceListerPrivate *priv;
};

struct _KorvaUPnPDeviceListerClass {
    GObjectClass parent_class;
};

KorvaDeviceLister *
korva_upnp_device_lister_new (void);
#endif /* __KORVA_UPNP_DEVICE_LISTER_H__ */
