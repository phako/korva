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

#ifndef __KORVA_ICON_CACHE_H__
#define __KORVA_ICON_CACHE_H__

#include <glib.h>

#include "korva-device.h"

G_BEGIN_DECLS

void
korva_icon_cache_init ();

char *
korva_icon_cache_lookup (const char *uid);

char *
korva_icon_cache_create_path (const char *uid);

char *
korva_icon_cache_get_default (KorvaDeviceType type);

G_END_DECLS

#endif /* __KORVA_ICON_CACHE_H__ */