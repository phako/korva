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

#define G_LOG_DOMAIN "Korva-Icon-Cache"

#include <glib.h>
#include <gio/gio.h>

#include "korva-icon-cache.h"

static char *cache_path;

void
korva_icon_cache_init (void)
{
    const char *cache_dir = g_get_user_cache_dir ();
    cache_path = g_build_filename (cache_dir, "korva", "icons", NULL);
    g_mkdir_with_parents (cache_path, 0700);

    g_debug ("Using %s as icon cache directory…", cache_path);
}

char *
korva_icon_cache_lookup (const char *uid)
{
    char *file_path;
    GFile *file;

    file_path = korva_icon_cache_create_path (uid);
    file = g_file_new_for_path (file_path);
    g_free (file_path);
    file_path = NULL;

    if (g_file_query_exists (file, NULL)) {
        file_path = g_file_get_uri (file);
    }

    g_object_unref (file);

    return file_path;
}

char *
korva_icon_cache_create_path (const char *uid)
{
    return g_build_filename (cache_path, uid, NULL);
}
