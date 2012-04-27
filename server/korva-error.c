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

#include <glib-object.h>
#include <gio/gio.h>

#include "korva-error.h"

static const GDBusErrorEntry korva_controller1_error_entries[] =
{
    { KORVA_CONTROLLER1_ERROR_FILE_NOT_FOUND,   "org.jensge.Korva.Error.FileNotFound" },
    { KORVA_CONTROLLER1_ERROR_NOT_COMPATIBLE,   "org.jensge.Korva.Error.NotCompatible" },
    { KORVA_CONTROLLER1_ERROR_NO_SUCH_DEVICE,   "org.jensge.Korva.Error.NoSuchDevice" },
    { KORVA_CONTROLLER1_ERROR_TIMEOUT,          "org.jensge.Korva.Error.Timeout" },
    { KORVA_CONTROLLER1_ERROR_INVALID_ARGS,     "org.jensge.Korva.Error.InvalidArgs" },
    { KORVA_CONTROLLER1_ERROR_NO_SUCH_TRANSFER, "org.jensge.Korva.Error.NoSuchTransfer" },
    { KORVA_CONTROLLER1_ERROR_NOT_ACCESSIBLE,   "org.jensge.Korva.Error.NotAccessible" }
};

GQuark
korva_controller1_error_quark ()
{
    static volatile gsize error_quark = 0;

    g_dbus_error_register_error_domain ("korva-controller1-error-quark",
                                        &error_quark,
                                        korva_controller1_error_entries,
                                        G_N_ELEMENTS (korva_controller1_error_entries));

    return error_quark;
}