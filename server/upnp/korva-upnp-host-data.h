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

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define KORVA_TYPE_UPNP_HOST_DATA \
    (korva_upnp_host_data_get_type ())

G_DECLARE_FINAL_TYPE (KorvaUPnPHostData, korva_upnp_host_data, KORVA, UPNP_HOST_DATA, GObject)

KorvaUPnPHostData *
korva_upnp_host_data_new (GFile *file, GHashTable *meta_data, const char *address);

void
korva_upnp_host_data_add_peer (KorvaUPnPHostData *self, const char *peer);

void
korva_upnp_host_data_remove_peer (KorvaUPnPHostData *self, const char *peer);

char *
korva_upnp_host_data_get_id (KorvaUPnPHostData *self);

char *
korva_upnp_host_data_get_uri (KorvaUPnPHostData *self, const char *iface, guint port);

const char *
korva_upnp_host_data_get_protocol_info (KorvaUPnPHostData *self);

gboolean
korva_upnp_host_data_valid_for_peer (KorvaUPnPHostData *self, const char *peer);

void
korva_upnp_host_data_start_timeout (KorvaUPnPHostData *self);

void
korva_upnp_host_data_cancel_timeout (KorvaUPnPHostData *self);

GFile *
korva_upnp_host_data_get_file (KorvaUPnPHostData *self);

goffset
korva_upnp_host_data_get_size (KorvaUPnPHostData *self);

GVariant *
korva_upnp_host_data_lookup_meta_data (KorvaUPnPHostData *self, const char *key);

GHashTable *
korva_upnp_host_data_get_meta_data (KorvaUPnPHostData *self);

const char *
korva_upnp_host_data_get_content_type (KorvaUPnPHostData *self);

gboolean
korva_upnp_host_data_has_peers (KorvaUPnPHostData *self);

void
korva_upnp_host_data_add_request (KorvaUPnPHostData *self);

void
korva_upnp_host_data_remove_request (KorvaUPnPHostData *self);

gboolean
korva_upnp_host_data_has_requests (KorvaUPnPHostData *self);

G_END_DECLS
