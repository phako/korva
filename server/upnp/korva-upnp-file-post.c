/*
    This file is part of Korva.

    Copyright (C) 2012 Jens Georg.
    Author: Jens Georg <mail@jensge.org>

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

#define G_LOG_DOMAIN "Korva-UPnP-File-Post"

#include <libsoup/soup.h>

#include "korva-upnp-file-post.h"

struct _KorvaUPnPFilePostPrivate {
    char *uri;
    char *content_type;
    GFile *file;
    GInputStream *stream;
    SoupSession *session;
    GCancellable *cancellable;
    SoupMessage *message;
    GTask *result;
    gint64 bytes_left;
};

G_DEFINE_TYPE_WITH_PRIVATE (KorvaUPnPFilePost, korva_upnp_file_post, G_TYPE_OBJECT)

enum _PostProps {
    PROP_0,
    PROP_SESSION,
    PROP_URI,
    PROP_FILE,
    PROP_SIZE,
    PROP_CONTENT_TYPE
};

typedef enum _PostProps PostProps;

static void
post_dispose (GObject *object)
{
    KorvaUPnPFilePost *self = KORVA_UPNP_FILE_POST (object);

    g_debug ("======> dispose");

    if (self->priv->message != NULL) {
        if (self->priv->session != NULL) {
            soup_session_cancel_message (self->priv->session,
                                         self->priv->message,
                                         SOUP_STATUS_CANCELLED);
        }
        g_object_unref (self->priv->message);
        self->priv->message = NULL;
    }

    if (self->priv->session != NULL) {
        g_object_unref (self->priv->session);
        self->priv->session = NULL;
    }

    if (self->priv->file != NULL) {
        g_object_unref (self->priv->file);
        self->priv->file = NULL;
    }

    if (self->priv->stream != NULL) {
        g_object_unref (self->priv->stream);
        self->priv->stream = NULL;
    }

    G_OBJECT_CLASS (korva_upnp_file_post_parent_class)->dispose (object);
}

static void
post_finalize (GObject *object)
{
    KorvaUPnPFilePost *self = KORVA_UPNP_FILE_POST (object);

    g_free (self->priv->file);
    self->priv->file = NULL;

    g_free (self->priv->content_type);
    self->priv->content_type = NULL;

    G_OBJECT_CLASS (korva_upnp_file_post_parent_class)->finalize (object);
}

static void
post_set_property (GObject      *object,
                   guint         property_id,
                   const GValue *value,
                   GParamSpec   *pspec)
{
    KorvaUPnPFilePost *self = KORVA_UPNP_FILE_POST (object);

    switch (property_id) {
        case PROP_SESSION:
            self->priv->session = g_value_dup_object (value);
            break;
        case PROP_URI:
            self->priv->uri = g_value_dup_string (value);
            break;
        case PROP_FILE:
            self->priv->file = g_value_dup_object (value);
            break;
        case PROP_SIZE:
            self->priv->bytes_left = g_value_get_int64 (value);
            break;
        case PROP_CONTENT_TYPE:
            self->priv->content_type = g_value_dup_string (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
post_constructed (GObject *object)
{
    KorvaUPnPFilePost *self = KORVA_UPNP_FILE_POST (object);

    self->priv->stream = G_INPUT_STREAM (g_file_read (self->priv->file,
                                                      self->priv->cancellable,
                                                      NULL));
}

static void
korva_upnp_file_post_init (KorvaUPnPFilePost *self)
{
    self->priv = korva_upnp_file_post_get_instance_private (self);
}

static void
korva_upnp_file_post_class_init (KorvaUPnPFilePostClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = post_dispose;
    object_class->finalize = post_finalize;
    object_class->set_property = post_set_property;
    object_class->constructed = post_constructed;

    g_object_class_install_property (object_class,
                                     PROP_SESSION,
                                     g_param_spec_object ("session",
                                                          "session",
                                                          "session",
                                                          SOUP_TYPE_SESSION,
                                                          G_PARAM_CONSTRUCT_ONLY |
                                                          G_PARAM_WRITABLE |
                                                          G_PARAM_STATIC_STRINGS));
    g_object_class_install_property (object_class,
                                     PROP_FILE,
                                     g_param_spec_object ("file",
                                                          "file",
                                                          "file",
                                                          G_TYPE_FILE,
                                                          G_PARAM_CONSTRUCT_ONLY |
                                                          G_PARAM_WRITABLE |
                                                          G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (object_class,
                                     PROP_URI,
                                     g_param_spec_string ("uri",
                                                          "uri",
                                                          "uri",
                                                          NULL,
                                                          G_PARAM_CONSTRUCT_ONLY |
                                                          G_PARAM_WRITABLE |
                                                          G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (object_class,
                                     PROP_SIZE,
                                     g_param_spec_int64 ("size",
                                                         "size",
                                                         "size",
                                                         0,
                                                         G_MAXINT64,
                                                         0,
                                                         G_PARAM_CONSTRUCT_ONLY |
                                                         G_PARAM_WRITABLE |
                                                         G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (object_class,
                                     PROP_CONTENT_TYPE,
                                     g_param_spec_string ("content-type",
                                                          "content-type",
                                                          "content-type",
                                                          NULL,
                                                          G_PARAM_CONSTRUCT_ONLY |
                                                          G_PARAM_WRITABLE |
                                                          G_PARAM_STATIC_STRINGS));
}

KorvaUPnPFilePost *
korva_upnp_file_post_new (const char *uri,
                          GFile *file,
                          gint64 size,
                          const char *content_type,
                          SoupSession *session)
{
    g_return_val_if_fail (uri != NULL, NULL);
    g_return_val_if_fail (G_IS_OBJECT (file), NULL);

    return g_object_new (KORVA_TYPE_UPNP_FILE_POST,
                         "uri", uri,
                         "file", file,
                         "session", session,
                         "size", size,
                         "content-type", content_type,
                         NULL);
}

static void
post_next_chunk (SoupMessage *message, KorvaUPnPFilePost *self)
{
    gint64 chunk_size;
    gchar *chunk;
    gsize bytes_read;
    GError *error = NULL;

    if (self->priv->bytes_left <= 0) {
        return;
    }

    chunk_size = MIN (G_MAXUINT16, self->priv->bytes_left);

    chunk = g_malloc0 (chunk_size);
    if (!g_input_stream_read_all (self->priv->stream,
                                  chunk,
                                  chunk_size,
                                  &bytes_read,
                                  self->priv->cancellable,
                                  &error))
    {
        soup_session_cancel_message (self->priv->session, message,
                                     SOUP_STATUS_CANCELLED);

        g_task_return_error (self->priv->result, error);
        g_object_unref (self->priv->result);

        self->priv->result = NULL;

        return;
    }

    soup_message_body_append (message->request_body, SOUP_MEMORY_TAKE,
                              chunk, bytes_read);
    self->priv->bytes_left -= bytes_read;
}

static void
post_on_informational (SoupMessage *message, KorvaUPnPFilePost *self)
{
    if (message->status_code == SOUP_STATUS_CONTINUE) {
        return;
    }

    soup_session_cancel_message (self->priv->session,
                                 message,
                                 SOUP_STATUS_CANCELLED);
    g_task_return_new_error (self->priv->result,
                             G_IO_ERROR,
                             G_IO_ERROR_CANCELLED,
                             "%s",
                             "Server does not want the file");
    g_clear_object (&self->priv->result);
}

static void
post_on_finished (SoupMessage *message, KorvaUPnPFilePost *self)
{
    g_debug("==> FInished!");
    if (self->priv->result != NULL) {
        g_task_return_boolean (self->priv->result, SOUP_STATUS_IS_SUCCESSFUL (message->status_code));
        g_clear_object (&self->priv->result);
    }

    self->priv->message = NULL;
}

void
korva_upnp_file_post_run_async (KorvaUPnPFilePost   *self,
                                GCancellable        *cancellable,
                                GAsyncReadyCallback  callback,
                                gpointer             user_data)
{
    SoupMessage *message;

    g_return_if_fail (KORVA_IS_UPNP_FILE_POST (self));

    self->priv->cancellable = cancellable;
    //TODO: Connect to cancellable
    self->priv->message = message = soup_message_new ("POST", self->priv->uri);

    soup_message_headers_set_content_length (message->request_headers,
                                             self->priv->bytes_left);
    soup_message_headers_set_content_type (message->request_headers,
                                           self->priv->content_type,
                                           NULL);
    soup_message_headers_set_expectations (message->request_headers,
                                           SOUP_EXPECTATION_CONTINUE);
    soup_message_body_set_accumulate (message->request_body, FALSE);
    soup_message_headers_append (message->request_headers,
                                 "Connection",
                                 "close");

    g_signal_connect (G_OBJECT (message),
                      "wrote-headers",
                      G_CALLBACK (post_next_chunk),
                      self);

    g_signal_connect (G_OBJECT (message),
                      "wrote-chunk",
                      G_CALLBACK (post_next_chunk),
                      self);
    g_signal_connect (G_OBJECT (message),
                      "got-informational",
                      G_CALLBACK (post_on_informational),
                      self);
    g_signal_connect (G_OBJECT (message),
                      "finished",
                      G_CALLBACK (post_on_finished),
                      self);

    self->priv->result = g_task_new (self, cancellable, callback, user_data);
    soup_session_queue_message (self->priv->session, message, NULL, self);
}

gboolean
korva_upnp_file_post_finish    (KorvaUPnPFilePost  *post,
                                GAsyncResult       *res,
                                GError            **error)
{
    g_return_val_if_fail (g_task_is_valid (res, post), FALSE);

    return g_task_propagate_boolean (G_TASK (res), error);
}
