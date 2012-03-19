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

#ifndef __KORVA_DEVICE_H__
#define __KORVA_DEVICE_H__

#include <glib-object.h>
#define KORVA_TYPE_DEVICE        (korva_device_get_type ())
#define KORVA_DEVICE(obj)        (G_TYPE_CHECK_INSTANCE_CAST ((obj), KORVA_TYPE_DEVICE, KorvaDevice))
#define KORVA_IS_DEVICE(obj)     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), KORVA_TYPE_DEVICE))
#define KORVA_DEVICE_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), KORVA_TYPE_DEVICE, KorvaDeviceInterface))

typedef struct _KorvaDevice KorvaDevice;
typedef struct _KorvaDeviceInterface KorvaDeviceInterface;

GType korva_device_get_type (void);

enum _KorvaDeviceProtocol {
    DEVICE_PROTOCOL_UPNP
};
typedef enum _KorvaDeviceProtocol KorvaDeviceProtocol;

enum _KorvaDeviceType {
    DEVICE_TYPE_SERVER,
    DEVICE_TYPE_PLAYER
};
typedef enum _KorvaDeviceType KorvaDeviceType;

struct _KorvaDeviceInterface {
    GTypeInterface parent;

    const char          *(* get_uid) (KorvaDevice *self);
    const char          *(* get_display_name) (KorvaDevice *self);
    const char          *(* get_icon_uri) (KorvaDevice *self);
    KorvaDeviceProtocol  (* get_protocol) (KorvaDevice *self);
    KorvaDeviceType      (* get_device_type) (KorvaDevice *self);

    GVariant            *(* serialize)  (KorvaDevice *self);
};

const char *
korva_device_get_uid (KorvaDevice *self);
const char *
korva_device_get_display_name (KorvaDevice *self);

const char *
korva_device_get_icon_uri (KorvaDevice *self);

KorvaDeviceProtocol
korva_device_get_protocol (KorvaDevice *self);

KorvaDeviceType
korva_device_get_device_type (KorvaDevice *self);

GVariant *
korva_device_serialize (KorvaDevice *self);

G_END_DECLS
#endif /* __KORVA_DEVICE_H__ */
