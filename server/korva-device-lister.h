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

#include <glib.h>
#include <glib-object.h>

#include "korva-device.h"

G_BEGIN_DECLS

#define KORVA_TYPE_DEVICE_LISTER        (korva_device_lister_get_type ())
G_DECLARE_INTERFACE(KorvaDeviceLister, korva_device_lister, KORVA, DEVICE_LISTER, GObject)

/**
 * KorvaDeviceLister:
 *
 * An interface to be implmented by classes that take care of device
 * discovery.
 */
struct _KorvaDeviceListerInterface {
    GTypeInterface parent;

    GList        * (*get_devices)(KorvaDeviceLister * self);
    KorvaDevice  * (*get_device_info)(KorvaDeviceLister * self, const char *uid);
    gint           (*get_device_count)(KorvaDeviceLister *self);
    gboolean       (*idle)(KorvaDeviceLister *self);

    /* signals */
    void           (*device_available)(KorvaDevice *device);
    void           (*device_unavailable)(const char *uid);
};


GList *
korva_device_lister_get_devices (KorvaDeviceLister *self);

KorvaDevice *
korva_device_lister_get_device_info (KorvaDeviceLister *self,
                                     const char        *uid);
gint
korva_device_lister_get_device_count (KorvaDeviceLister *self);

gboolean
korva_device_lister_idle (KorvaDeviceLister *self);

G_END_DECLS
