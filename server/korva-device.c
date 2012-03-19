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

#include "korva-device.h"

G_DEFINE_INTERFACE (KorvaDevice, korva_device, G_TYPE_OBJECT)

static void
korva_device_default_init (KorvaDeviceInterface *g_iface)
{
}
const char *
korva_device_get_uid (KorvaDevice *self)
{
    return KORVA_DEVICE_GET_INTERFACE (self)->get_uid (self);
}

const char *
korva_device_get_display_name (KorvaDevice *self)
{
    return KORVA_DEVICE_GET_INTERFACE (self)->get_display_name (self);
}

const char *
korva_device_get_icon_uri (KorvaDevice *self)
{
    return KORVA_DEVICE_GET_INTERFACE (self)->get_icon_uri (self);
}

KorvaDeviceProtocol
korva_device_get_protocol (KorvaDevice *self)
{
    return KORVA_DEVICE_GET_INTERFACE (self)->get_protocol (self);
}

KorvaDeviceType
korva_device_get_device_type (KorvaDevice *self)
{
    return KORVA_DEVICE_GET_INTERFACE (self)->get_device_type (self);
}

GVariant *
korva_device_serialize (KorvaDevice *self)
{
    return KORVA_DEVICE_GET_INTERFACE (self)->serialize (self);
}
