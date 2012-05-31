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

#define G_LOG_DOMAIN "Korva-Server"

#include "korva-server.h"
#include "korva-icon-cache.h"

int main (int argc, char *argv[])
{
    g_debug ("Starting korva...");
    g_type_init ();
    korva_icon_cache_init ();

    KorvaServer *server = korva_server_new ();

    korva_server_run (server);

    g_object_unref (server);

    return 0;
}
