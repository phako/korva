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

#ifndef __KORVA_UPNP_DEVICE_H__
#define __KORVA_UPNP_DEVICE_H__

#include <glib-object.h>

#include <libgupnp/gupnp.h>

#include "korva-device.h"

#define KORVA_TYPE_UPNP_DEVICE \
    (korva_upnp_device_get_type ())
#define KORVA_UPNP_DEVICE(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), KORVA_TYPE_UPNP_DEVICE, KorvaUPnPDevice))
#define KORVA_IS_UPNP_DEVICE(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), KORVA_TYPE_UPNP_DEVICE))
#define KORVA_UPNP_DEVICE_CLASS(obj) \
    (G_TYPE_CHECK_CLASS_CAST ((obj), KORVA_TYPE_UPNP_DEVICE, KorvaUPnPDeviceClass))
#define KORVA_IS_UPNP_DEVICE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), KORVA_TYPE_UPNP_DEVICE))
#define KORVA_UPNP_DEVICE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), KORVA_TYPE_UPNP_DEVICE, KorvaUPnPDeviceClass))


typedef struct _KorvaUPnPDevicePrivate KorvaUPnPDevicePrivate;
typedef struct _KorvaUPnPDeviceClass KorvaUPnPDeviceClass;
typedef struct _KorvaUPnPDevice KorvaUPnPDevice;

GType korva_upnp_device_get_type (void);

GQuark korva_upnp_device_error_quark ();
#define KORVA_UPNP_DEVICE_ERROR korva_upnp_device_error_quark ()
enum _KorvaUPnPDeviceError {
    INVALID_DEVICE_TYPE,
    MISSING_SERVICE,
    TIMEOUT
};

struct _KorvaUPnPDeviceClass {
    GObjectClass parent_class;
};

struct _KorvaUPnPDevice {
    GObject parent;

    KorvaUPnPDevicePrivate *priv;
};

void
korva_upnp_device_add_proxy (KorvaUPnPDevice *self, GUPnPDeviceProxy *proxy);

gboolean
korva_upnp_device_remove_proxy (KorvaUPnPDevice *self, GUPnPDeviceProxy *proxy);

void
korva_upnp_device_push_async (KorvaDevice         *self,
                              GVariant            *source,
                              GAsyncReadyCallback  callback,
                              gpointer             user_data);

gboolean
korva_upnp_device_push_finish (KorvaDevice   *self,
                               GAsyncResult  *result,
                               GError       **error);
G_END_DECLS
#endif /* __KORVA_UPNP_DEVICE_H__ */
