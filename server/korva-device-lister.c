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

#define G_LOG_DOMAIN "Korva-Device-Lister"

#include "korva-device-lister.h"

G_DEFINE_INTERFACE (KorvaDeviceLister, korva_device_lister, G_TYPE_OBJECT)

static void
korva_device_lister_default_init (KorvaDeviceListerInterface *g_iface)
{
    /**
     * KorvaDeviceLister::device-available
     * @device: The new device
     *
     * ::device-available is emitted when the lister found a new device on the
     * network
     */
    g_signal_new ("device-available",
                  G_TYPE_FROM_INTERFACE (g_iface),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (KorvaDeviceListerInterface, device_available),
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE,
                  1,
                  KORVA_TYPE_DEVICE);

    /**
     * KorvaDeviceLister::device-unavailable
     * @uid: Unique identifier of the device
     *
     * ::device-unavailable is emitted when a device has disappeared from the
     * network
     */
    g_signal_new ("device-unavailable",
                  G_TYPE_FROM_INTERFACE (g_iface),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (KorvaDeviceListerInterface, device_unavailable),
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_STRING);
}

/**
 * korva_device_lister_get_devices:
 *
 * Get a list of the devices currently known to the device lister.
 *
 * @self: a #KorvaDeviceLister
 *
 * Returns: (transfer container) (element-type KorvaDevice) (allow-none): A list of devices or %NULL if none is available
 */
GList *
korva_device_lister_get_devices (KorvaDeviceLister *self)
{
    return KORVA_DEVICE_LISTER_GET_INTERFACE (self)->get_devices (self);
}

/**
 * korva_device_lister_get_device_info:
 *
 * @self: a #KorvaDeviceLister
 * @uid: Identifier of the device to look-up
 * Returns: (transfer none) (allow-none): The device matching the uid or %NULL
 * if not found.
 */
KorvaDevice *
korva_device_lister_get_device_info (KorvaDeviceLister *self,
                                     const char        *uid)
{
    return KORVA_DEVICE_LISTER_GET_INTERFACE (self)->get_device_info (self, uid);
}

/**
 * korva_device_lister_get_device_count:
 *
 * Get the number of devices currently known to this lister.
 * @self: a #KorvaDeviceLister
 *
 * Returns: Number of devices available.
 */
gint
korva_device_lister_get_device_count (KorvaDeviceLister *self)
{
    return KORVA_DEVICE_LISTER_GET_INTERFACE (self)->get_device_count (self);
}

/**
 * korva_device_lister_idle:
 *
 * Get the idle status of the device lister. What "idle" means is up to the
 * device lister.
 * @self: a #KorvaDeviceLister
 *
 * Returns: %TRUE if idle, %FALSE otherwise.
 */
gboolean
korva_device_lister_idle (KorvaDeviceLister *self)
{
    return KORVA_DEVICE_LISTER_GET_INTERFACE (self)->idle (self);
}