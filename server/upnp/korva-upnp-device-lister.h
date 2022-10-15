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
#include "korva-device-lister.h"
#include <glib-object.h>

#define KORVA_TYPE_UPNP_DEVICE_LISTER (korva_upnp_device_lister_get_type ())

G_DECLARE_FINAL_TYPE (KorvaUPnPDeviceLister, korva_upnp_device_lister, KORVA, UPNP_DEVICE_LISTER, GObject)

KorvaDeviceLister *
korva_upnp_device_lister_new (void);
