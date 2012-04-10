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

#define G_LOG_DOMAIN "Korva-Device"

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

/**
 * korva_device_serialize:
 *
 * Serialize a device into a #G_VARIANT_TYPE_VARDICT #GVariant for D-Bus use.
 * @self: device to serialize
 * Returns: a newly allocated #GVariant containing the meta-data for this
 * device
 */
GVariant *
korva_device_serialize (KorvaDevice *self)
{
    return KORVA_DEVICE_GET_INTERFACE (self)->serialize (self);
}

/**
 * korva_device_push_async:
 *
 * Initiate media push to the device.
 * @self: device to push to
 * @source: an "a{sv}" variant, containing at least the mandatory key "URI"
 * @callback: #GAsyncReady call-back to call after the push operation succeeds
 * @user_data: user data
 */
void
korva_device_push_async (KorvaDevice         *self,
                         GVariant            *source,
                         GAsyncReadyCallback  callback,
                         gpointer             user_data)
{
    KORVA_DEVICE_GET_INTERFACE (self)->push_async (self,
                                                   source,
                                                   callback,
                                                   user_data);
}

/**
 * korva_device_push_finish:
 *
 * Finalize media push to the device.
 * To be called in the call-back passed to #korva_device_push_async.
 * @self: device pushed to
 * @result: the #GAsyncResult passed in the callback
 * @error: A location to store an error to or %NULL
 * @user_data: user_data
 * @returns: %TRUE on success.
 */
gboolean
korva_device_push_finish (KorvaDevice   *self,
                          GAsyncResult  *result,
                          GError       **error)
{
    return KORVA_DEVICE_GET_INTERFACE (self)->push_finish (self,
                                                           result,
                                                           error);
}

