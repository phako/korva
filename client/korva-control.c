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

#include <stdlib.h>
#include <string.h>

#include <gio/gio.h>

#include <korva-dbus-interface.h>

typedef enum {
    KORVA_CONTROL_MODE_NONE,
    KORVA_CONTROL_MODE_LIST,
    KORVA_CONTROL_MODE_PUSH,
    KORVA_CONTROL_MODE_UNSHARE
} KorvaControlMode;

static char *file = NULL;
static char *device = NULL;
static char *tag = NULL;
static KorvaControlMode mode = KORVA_CONTROL_MODE_NONE;

static gboolean
parse_mode (const char *option_name,
            const char *value,
            gpointer data,
            GError **error)
{
    if (mode != KORVA_CONTROL_MODE_NONE) {
        g_set_error_literal (error,
                             G_OPTION_ERROR,
                             G_OPTION_ERROR_FAILED,
                             "Mode already set");
        return FALSE;
    }

    if (g_ascii_strcasecmp (value, "push") == 0) {
        mode = KORVA_CONTROL_MODE_PUSH;
    } else  if (g_ascii_strcasecmp (value, "list") == 0) {
        mode = KORVA_CONTROL_MODE_LIST;
    } else  if (g_ascii_strcasecmp (value, "unshare") == 0) {
        mode = KORVA_CONTROL_MODE_UNSHARE;
    } else {
        g_set_error (error,
                     G_OPTION_ERROR,
                     G_OPTION_ERROR_FAILED,
                     "Unknown action '%s'",
                     value);

        return FALSE;
    }

    return TRUE;
}

static gboolean
set_list_mode (const char *option_name,
               const char *value,
               gpointer data,
               GError **error)
{
    if (mode != KORVA_CONTROL_MODE_NONE) {
        g_set_error_literal (error,
                             G_OPTION_ERROR,
                             G_OPTION_ERROR_FAILED,
                             "Mode already set");
        return FALSE;
    }

    mode = KORVA_CONTROL_MODE_LIST;

    return TRUE;
}

static GOptionEntry entries[] =
{
    { "list", 'l', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, set_list_mode, "Show available devices; short for --action=list", NULL },
    { "action", 'a', 0, G_OPTION_ARG_CALLBACK, parse_mode, "ACTION to perform (push, unshare, list)", "ACTION" },
    { "file", 'f', 0, G_OPTION_ARG_FILENAME, &file, "Path to a FILE", "FILE" },
    { "device", 'd', 0, G_OPTION_ARG_STRING, &device, "UID of a device", "UID" },
    { "tag", 't', 0, G_OPTION_ARG_STRING, &tag, "TAG of a previously done push operation", "TAG" },
    { NULL }
};

static void
korva_control_list_devices (KorvaController1 *proxy)
{
    GVariant *devices, *dict, *value;
    char *key;
    GVariantIter *outer, *inner;
    GError *error = NULL;

    korva_controller1_call_get_devices_sync (proxy,
                                             &devices,
                                             NULL,
                                             &error);
    if (error != NULL) {
        g_print ("Could not get device list: %s\n", error->message);
        g_error_free (error);

        return;
    }

    outer = g_variant_iter_new (devices);
    dict = g_variant_iter_next_value (outer);
    while (dict != NULL) {
        g_print ("Device:\n");
        inner = g_variant_iter_new (dict);
        while (g_variant_iter_next (inner, "{sv}", &key, &value)) {
            if (strcmp (key, "UID") == 0 ||
                strcmp (key, "DisplayName") == 0) {
                g_print ("    %s: %s\n", key, g_variant_get_string (value, NULL));
            }

            g_free (key);
            g_variant_unref (value);
        }
        g_variant_iter_free (inner);

        g_variant_unref (dict);
        dict = g_variant_iter_next_value (outer);
    }

    g_variant_iter_free (outer);
}

static void
korva_control_push (KorvaController1 *controller, const char *path, const char *uid)
{
    GFile *source;
    GVariantBuilder *builder;
    char *out_tag;
    GError *error = NULL;

    source = g_file_new_for_commandline_arg (path);
    builder = g_variant_builder_new (G_VARIANT_TYPE_ARRAY);
    g_variant_builder_add (builder, "{sv}", "URI", g_variant_new_string (g_file_get_uri (source)));

    korva_controller1_call_push_sync (controller,
                                      g_variant_builder_end (builder),
                                      device,
                                      &out_tag,
                                      NULL,
                                      &error);
    if (error != NULL) {
        g_print ("Failed to Push %s to %s: %s\n",
                 file,
                 device,
                 error->message);

        g_error_free (error);
    } else {
        g_print ("Pushed %s to %s. The ID is %s\n", file, device, out_tag);
    }
}

static void usage (GOptionContext *context)
{
    g_print ("%s", g_option_context_get_help (context, FALSE, NULL));

    exit (0);
}

int main(int argc, char *argv[])
{
    GOptionContext *context;
    GError *error = NULL;
    KorvaController1 *controller;

    g_type_init ();

    context = g_option_context_new ("- control a korva server");
    g_option_context_add_main_entries (context, entries, NULL);
    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        g_print ("Error parsing options: %s\n", error->message);

        exit (1);
    }

    controller = korva_controller1_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                           G_DBUS_PROXY_FLAGS_NONE,
                                                           "org.jensge.Korva",
                                                           "/org/jensge/Korva",
                                                           NULL,
                                                           &error);
    if (error != NULL) {
        g_print ("Could not get connection to D-Bus peer: %s\n", error->message);

        exit (1);
    }

    switch (mode) {
        case KORVA_CONTROL_MODE_LIST:
            korva_control_list_devices (controller);

            break;
        case KORVA_CONTROL_MODE_PUSH:
            if (file == NULL || device == NULL) {
                usage (context);
            }
            korva_control_push (controller, file, device);

            break;
        case KORVA_CONTROL_MODE_UNSHARE:
            if (tag == NULL) {
                usage (context);
            }
            korva_controller1_call_unshare_sync (controller, tag, NULL, &error);

            if (error != NULL) {
                g_print ("Failed to Unshare tag %s: %s\n",
                         tag,
                         error->message);

                g_error_free (error);
            }

            break;
        case KORVA_CONTROL_MODE_NONE:
            usage (context);

            break;
        default:
            g_assert_not_reached ();
            break;
    }

    g_object_unref (controller);

    return 0;
}
