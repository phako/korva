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

#include <gio/gio.h>
#include <glib-object.h>

#define KORVA_TYPE_DEVICE (korva_device_get_type ())
G_DECLARE_INTERFACE (KorvaDevice, korva_device, KORVA, DEVICE, GObject)

enum _KorvaDeviceProtocol
{
    DEVICE_PROTOCOL_UPNP
};
typedef enum _KorvaDeviceProtocol KorvaDeviceProtocol;

enum _KorvaDeviceType
{
    DEVICE_TYPE_SERVER,
    DEVICE_TYPE_PLAYER
};
typedef enum _KorvaDeviceType KorvaDeviceType;

struct _KorvaDeviceInterface {
    GTypeInterface parent;

    const char *(*get_uid) (KorvaDevice *self);
    const char *(*get_display_name) (KorvaDevice *self);
    const char *(*get_icon_uri) (KorvaDevice *self);
    KorvaDeviceProtocol (*get_protocol) (KorvaDevice *self);
    KorvaDeviceType (*get_device_type) (KorvaDevice *self);

    GVariant *(*serialize) (KorvaDevice *self);
    void (*push_async) (KorvaDevice *self,
                        GVariant *source,
                        GCancellable *cancellable,
                        GAsyncReadyCallback callback,
                        gpointer user_data);
    char *(*push_finish) (KorvaDevice *self, GAsyncResult *result, GError **error);
    void (*unshare_async) (KorvaDevice *self,
                           const char *tag,
                           GCancellable *cancellable,
                           GAsyncReadyCallback callback,
                           gpointer user_data);
    gboolean (*unshare_finish) (KorvaDevice *self, GAsyncResult *result, GError **error);
};

const char *
korva_device_get_uid (KorvaDevice *self);

const char *
korva_device_get_display_name (KorvaDevice *self);

const char *
korva_device_get_icon_uri (KorvaDevice *self);

KorvaDeviceProtocol
korva_device_get_protocol (KorvaDevice *self);

KorvaDeviceType
korva_device_get_device_type (KorvaDevice *self);

GVariant *
korva_device_serialize (KorvaDevice *self);

void
korva_device_push_async (KorvaDevice *self,
                         GVariant *source,
                         GCancellable *cancellable,
                         GAsyncReadyCallback callback,
                         gpointer user_data);

char *
korva_device_push_finish (KorvaDevice *self, GAsyncResult *result, GError **error);

void
korva_device_unshare_async (KorvaDevice *self,
                            const char *tag,
                            GCancellable *cancellable,
                            GAsyncReadyCallback callback,
                            gpointer user_data);

gboolean
korva_device_unshare_finish (KorvaDevice *self, GAsyncResult *result, GError **error);

G_END_DECLS
