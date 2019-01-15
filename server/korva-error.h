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
#ifndef __KORVA_ERROR_H__
#define __KORVA_ERROR_H__

#include <glib.h>

G_BEGIN_DECLS

#define KORVA_CONTROLLER1_ERROR korva_controller1_error_quark ()

GQuark korva_controller1_error_quark ();

enum _KorvaController1Error {
    KORVA_CONTROLLER1_ERROR_NO_SUCH_DEVICE,
    KORVA_CONTROLLER1_ERROR_TIMEOUT,
    KORVA_CONTROLLER1_ERROR_FILE_NOT_FOUND,
    KORVA_CONTROLLER1_ERROR_NOT_COMPATIBLE,
    KORVA_CONTROLLER1_ERROR_INVALID_ARGS,
    KORVA_CONTROLLER1_ERROR_NO_SUCH_TRANSFER,
    KORVA_CONTROLLER1_ERROR_NOT_ACCESSIBLE,
    KORVA_CONTROLLER1_ERROR_NO_SERVER,
};

G_END_DECLS

#endif /* __KORVA_ERROR_H__ */
