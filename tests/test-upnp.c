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

#include <string.h>

#include <glib/gstdio.h>

#include <libsoup/soup.h>

#include <korva-error.h>
#include <korva-device.h>
#include <korva-icon-cache.h>

#include "korva-upnp-device.h"
#include "korva-upnp-file-server.h"
#include "korva-upnp-constants-private.h"

#include "mock-dmr/mock-dmr.h"

static gboolean
quit_main_loop_source_func (gpointer user_data)
{
    g_main_loop_quit ((GMainLoop *) user_data);

    return FALSE;
}

static void
test_upnp_fileserver_single_instance (void)
{
    KorvaUPnPFileServer *server, *server2;

    server = korva_upnp_file_server_get_default ();
    server2 = korva_upnp_file_server_get_default ();
    g_assert (server == server2);

    g_object_unref (server);
    g_object_unref (server2);
}

typedef struct {
    GMainLoop           *loop;
    char                *result_uri;
    char                *in_uri;
    KorvaUPnPFileServer *server;
    GFile               *in_file;
    GHashTable          *in_params;
    GHashTable          *result_params;
    GError              *result_error;
} HostFileTestData;

typedef struct {
    GError *error;
    GMainLoop *loop;
    GBytes *data;
} WaitForMessageData;

#define WAIT_FOR_MESSAGE_DATA_INIT(loop) { NULL, (loop), NULL };

static void
wait_for_message_data_reset (WaitForMessageData *data)
{
    g_clear_pointer (&data->data, g_bytes_unref);
    g_clear_pointer (&data->error, g_error_free);
}

G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(WaitForMessageData, wait_for_message_data_reset)

static void
on_message_ready (GObject *source, GAsyncResult *res, gpointer user_data)
{
    WaitForMessageData *data = user_data;

    data->data = soup_session_send_and_read_finish (SOUP_SESSION (source), res, &data->error);
    g_main_loop_quit (data->loop);
}

static void
schedule_request_and_wait (SoupSession *session, SoupMessage *message, WaitForMessageData *data)
{
    soup_session_send_and_read_async (session, message, G_PRIORITY_DEFAULT, NULL, on_message_ready, data);
    g_main_loop_run (data->loop);
}

static void
test_host_file_setup (HostFileTestData *data, gconstpointer user_data)
{
    memset (data, 0, sizeof (HostFileTestData));

    data->in_file = g_file_new_for_commandline_arg (TEST_DATA_DIR "/test-upnp-image.jpg");
    data->in_uri = g_file_get_uri (data->in_file);

    data->in_params = g_hash_table_new_full (g_str_hash,
                                             (GEqualFunc) g_str_equal,
                                             g_free,
                                             (GDestroyNotify) g_variant_unref);

    g_hash_table_insert (data->in_params, g_strdup ("URI"), g_variant_new_string (data->in_uri));

    data->server = korva_upnp_file_server_get_default ();
    g_assert (data->server != NULL);
    g_assert (korva_upnp_file_server_idle (data->server));

    data->loop = g_main_loop_new (NULL, FALSE);
}

static void
test_host_file_teardown (HostFileTestData *data, gconstpointer user_data)
{
    g_free (data->in_uri);
    g_free (data->result_uri);
    g_object_unref (data->server);
    g_main_loop_unref (data->loop);
}

static void
test_upnp_fileserver_host_file_on_host_file (GObject      *source,
                                             GAsyncResult *res,
                                             gpointer      user_data)
{
    HostFileTestData *data = (HostFileTestData *) user_data;
    KorvaUPnPFileServer *server = KORVA_UPNP_FILE_SERVER (source);

    data->result_uri = korva_upnp_file_server_host_file_finish (server,
                                                                res,
                                                                &(data->result_params),
                                                                &(data->result_error));

    g_main_loop_quit (data->loop);
}

static void
test_upnp_fileserver_host_file (HostFileTestData *data, gconstpointer user_data)
{
    g_autoptr (SoupSession) session = NULL;
    g_autoptr (SoupMessage) message = NULL;
    GHashTable *params;
    GVariant *value;

    params = data->in_params;

    korva_upnp_file_server_host_file_async (data->server,
                                            data->in_file,
                                            data->in_params,
                                            "127.0.0.1",
                                            "127.0.0.1",
                                            NULL,
                                            test_upnp_fileserver_host_file_on_host_file,
                                            data);

    g_main_loop_run (data->loop);

    g_assert (data->result_uri != NULL);
    g_assert (data->result_error == NULL);
    g_assert (data->result_params == params);

    /* Check that the server has set the size and the content-type */
    value = g_hash_table_lookup (params, "Size");
    g_assert (value != NULL);

    value = g_hash_table_lookup (params, "ContentType");
    g_assert (value != NULL);

    g_auto (WaitForMessageData) wfm = WAIT_FOR_MESSAGE_DATA_INIT (data->loop);

    /* Test that the file is reachable under the uri */
    session = soup_session_new ();
    message = soup_message_new (SOUP_METHOD_HEAD, data->result_uri);
    schedule_request_and_wait (session, message, &wfm);

    g_assert_no_error (wfm.error);
    g_assert_cmpuint (soup_message_get_status(message), ==, SOUP_STATUS_OK);

    /* Share it again for different peer and verify we get the same data */
    korva_upnp_file_server_host_file_async (data->server,
                                            data->in_file,
                                            data->in_params,
                                            "127.0.0.1",
                                            "192.168.4.5",
                                            NULL,
                                            test_upnp_fileserver_host_file_on_host_file,
                                            data);

    g_main_loop_run (data->loop);

    g_assert (data->result_uri != NULL);
    g_assert (data->result_error == NULL);
    g_assert (data->result_params == params);

    /* Check that removing one peer doesn't remove the whole file */
    korva_upnp_file_server_unhost_file_for_peer (data->server, data->in_file, "192.168.4.5");
    g_assert (!korva_upnp_file_server_idle (data->server));

    korva_upnp_file_server_unhost_file_for_peer (data->server, data->in_file, "127.0.0.1");
    g_assert (korva_upnp_file_server_idle (data->server));
}

static void
test_upnp_fileserver_host_file_dont_override_data (HostFileTestData *data, gconstpointer user_data)
{
    GHashTable *params;
    GVariant *value;

    params = data->in_params;
    g_hash_table_replace (params, g_strdup ("Size"), g_variant_new_uint64 (123456));
    g_hash_table_replace (params, g_strdup ("ContentType"), g_variant_new_string ("x-custom/content"));
    g_hash_table_replace (params, g_strdup ("Title"), g_variant_new_string ("TestTitle01"));

    korva_upnp_file_server_host_file_async (data->server,
                                            data->in_file,
                                            data->in_params,
                                            "127.0.0.1",
                                            "127.0.0.1",
                                            NULL,
                                            test_upnp_fileserver_host_file_on_host_file,
                                            data);

    g_main_loop_run (data->loop);

    g_assert (data->result_uri != NULL);
    g_assert (data->result_error == NULL);
    g_assert (data->result_params == params);

    /* Check that the server has changed the size but left content-type alone
     * */
    value = g_hash_table_lookup (params, "Size");
    g_assert (value != NULL);
    g_assert_cmpint (g_variant_get_uint64 (value), !=, 123456);

    value = g_hash_table_lookup (params, "ContentType");
    g_assert (value != NULL);
    g_assert_cmpstr (g_variant_get_string (value, NULL), ==, "x-custom/content");

    value = g_hash_table_lookup (params, "Title");
    g_assert (value != NULL);
    g_assert_cmpstr (g_variant_get_string (value, NULL), ==, "TestTitle01");
}


static void
test_upnp_fileserver_host_file_error (HostFileTestData *data, gconstpointer user_data)
{
    g_autofree char *uri = NULL;
    g_autoptr (GFile) file = NULL;

    file = g_file_new_for_path ("/this/file/does/not/exist");
    uri = g_file_get_uri (file);
    g_hash_table_replace (data->in_params, g_strdup ("URI"), g_variant_new_string (uri));

    korva_upnp_file_server_host_file_async (data->server,
                                            file,
                                            data->in_params,
                                            "127.0.0.1",
                                            "127.0.0.1",
                                            NULL,
                                            test_upnp_fileserver_host_file_on_host_file,
                                            data);

    g_main_loop_run (data->loop);

    g_assert (data->result_uri == NULL);
    g_assert (data->result_error != NULL);
    g_assert_cmpint (data->result_error->code, ==, KORVA_CONTROLLER1_ERROR_FILE_NOT_FOUND);
    g_assert (data->result_params == NULL);
}

static void
test_upnp_file_server_host_file_no_access (HostFileTestData *data, gconstpointer user_data)
{
    int fd;
    g_autofree char *template = NULL;
    g_autofree char *uri = NULL;
    g_autoptr (GFile) file = NULL;

    template = g_build_filename (g_get_tmp_dir (), "korva_test_upnp_XXXXXX", NULL);

    fd = g_mkstemp_full (template, 0, 0000);
    g_assert (fd > 0);

    file = g_file_new_for_path (template);
    uri = g_file_get_uri (file);
    g_hash_table_replace (data->in_params, g_strdup ("URI"), g_variant_new_string (uri));

    korva_upnp_file_server_host_file_async (data->server,
                                            file,
                                            data->in_params,
                                            "127.0.0.1",
                                            "127.0.0.1",
                                            NULL,
                                            test_upnp_fileserver_host_file_on_host_file,
                                            data);

    g_main_loop_run (data->loop);

    g_assert (data->result_uri == NULL);
    g_assert (data->result_error != NULL);
    g_assert_cmpint (data->result_error->code, ==, KORVA_CONTROLLER1_ERROR_NOT_ACCESSIBLE);
    g_assert (data->result_params == NULL);

    close (fd);
    g_remove (template);
}

static void
test_upnp_fileserver_host_file_timeout (HostFileTestData *data, gconstpointer user_data)
{
    if (!g_test_slow ()) {
        return;
    }

    korva_upnp_file_server_host_file_async (data->server,
                                            data->in_file,
                                            data->in_params,
                                            "127.0.0.1",
                                            "127.0.0.1",
                                            NULL,
                                            test_upnp_fileserver_host_file_on_host_file,
                                            data);

    g_main_loop_run (data->loop);

    g_assert (data->result_uri != NULL);
    g_assert (data->result_error == NULL);

    g_timeout_add_seconds (KORVA_UPNP_FILE_SERVER_DEFAULT_TIMEOUT + 10,
                           quit_main_loop_source_func,
                           data->loop);
    g_main_loop_run (data->loop);

    g_assert (korva_upnp_file_server_idle (data->server));
}

static gpointer
blocking_request (gpointer user_data)
{
    HostFileTestData *data = user_data;
    g_autoptr (GMainContext) ctx = g_main_context_new ();
    g_main_context_push_thread_default (ctx);
    g_autoptr (GError) error = NULL;

    g_autoptr (SoupSession) session = soup_session_new ();
    g_autoptr (SoupMessage) message = soup_message_new (SOUP_METHOD_GET, data->result_uri);
    g_autoptr (GMainLoop) loop = g_main_loop_new (ctx, FALSE);

    GInputStream *is = soup_session_send (session, message, NULL, &error);
    g_debug ("Send has finished, sleeping...");
    usleep ((KORVA_UPNP_FILE_SERVER_DEFAULT_TIMEOUT + 20) * G_USEC_PER_SEC);
    g_debug ("Sleeping done, finishing thread");
    g_assert_no_error (error);
    g_input_stream_close (is, NULL, NULL);

    g_main_context_pop_thread_default (ctx);

    return NULL;
}

static void
test_upnp_fileserver_host_file_timeout2 (HostFileTestData *data, gconstpointer user_data)
{
    g_autoptr (SoupSession) session = NULL;
    g_autoptr (SoupMessage) message = NULL;

    if (!g_test_slow ()) {
        return;
    }

    g_autoptr (GThread) thread = g_thread_new ("blocking request thread", blocking_request, data);

    g_timeout_add_seconds (KORVA_UPNP_FILE_SERVER_DEFAULT_TIMEOUT + 10,
                           quit_main_loop_source_func,
                           data->loop);
    g_main_loop_run (data->loop);

    g_assert (!korva_upnp_file_server_idle (data->server));

    g_thread_join (thread);
}

static void
test_upnp_fileserver_http_server_setup (HostFileTestData *data, gconstpointer user_data)
{
    test_host_file_setup (data, user_data);
    korva_upnp_file_server_host_file_async (data->server,
                                            data->in_file,
                                            data->in_params,
                                            "127.0.0.1",
                                            "127.0.0.1",
                                            NULL,
                                            test_upnp_fileserver_host_file_on_host_file,
                                            data);

    g_main_loop_run (data->loop);

    g_assert (data->result_uri != NULL);
    g_assert (data->result_error == NULL);
}

static void
test_upnp_fileserver_http_server (HostFileTestData *data, gconstpointer user_data)
{
    g_autoptr (SoupSession) session = NULL;
    g_autoptr (SoupMessage) message = NULL;
    g_autofree char *base_uri = NULL;
    g_autofree char *uri = NULL;
    char *needle = NULL;

    const char *empty_md5 = "d41d8cd98f00b204e9800998ecf8427e";

    base_uri = g_strdup (data->result_uri);
    needle = g_strrstr_len (base_uri, -1, "/") + 1;
    *needle = '\0';

    session = soup_session_new ();

    /* Check invalid HTTP method */
    message = soup_message_new (SOUP_METHOD_DELETE, data->result_uri);
    g_auto (WaitForMessageData) wfm = WAIT_FOR_MESSAGE_DATA_INIT (data->loop);
    schedule_request_and_wait (session, message, &wfm);

    g_assert_no_error (wfm.error);
    g_assert_cmpint (soup_message_get_status(message), ==, SOUP_STATUS_METHOD_NOT_ALLOWED);
    g_object_unref (message);

    /* Check URI format with invalid item format */
    uri = g_strconcat (base_uri, "should_not_work", NULL);
    message = soup_message_new (SOUP_METHOD_HEAD, uri);
    wait_for_message_data_reset (&wfm);
    schedule_request_and_wait (session, message, &wfm);

    g_assert_no_error (wfm.error);
    g_assert_cmpint (soup_message_get_status(message), ==, SOUP_STATUS_NOT_FOUND);
    g_free (uri);
    g_object_unref (message);

    /* Check URI format with invalid MD5 */
    uri = g_strconcat (base_uri, empty_md5, NULL);
    message = soup_message_new (SOUP_METHOD_HEAD, uri);
    wait_for_message_data_reset (&wfm);
    schedule_request_and_wait (session, message, &wfm);

    g_assert_no_error (wfm.error);
    g_assert_cmpint (soup_message_get_status(message), ==, SOUP_STATUS_NOT_FOUND);
    g_free (uri);
    g_object_unref (message);

    /* Check that we get a 404 when trying to access it with different IP */
    uri = g_strdup (data->result_uri);
    korva_upnp_file_server_host_file_async (data->server,
                                            data->in_file,
                                            data->in_params,
                                            "127.0.0.1",
                                            "192.168.4.5",
                                            NULL,
                                            test_upnp_fileserver_host_file_on_host_file,
                                            data);
    g_main_loop_run (data->loop);
    korva_upnp_file_server_unhost_file_for_peer (data->server, data->in_file, "127.0.0.1");
    message = soup_message_new (SOUP_METHOD_HEAD, uri);
    wait_for_message_data_reset (&wfm);
    schedule_request_and_wait (session, message, &wfm);

    g_assert_no_error (wfm.error);
    g_assert_cmpint (soup_message_get_status(message), ==, SOUP_STATUS_NOT_FOUND);
}

static void
test_upnp_fileserver_http_server_ranges (HostFileTestData *data, gconstpointer user_data)
{
    g_autoptr (SoupSession) session = NULL;
    SoupMessage *message = NULL;
    goffset size;
    GVariant *value;
    goffset start = 0, end = 0, total_length = 0, content_length;
    g_autoptr (GMappedFile) file = NULL;

    file = g_mapped_file_new (TEST_DATA_DIR "/test-upnp-image.jpg", FALSE, NULL);
    g_assert (file != NULL);

    value = g_hash_table_lookup (data->in_params, "Size");
    g_assert (value != NULL);
    size = g_variant_get_uint64 (value);
    g_assert (size > 2048);

    session = soup_session_new ();
    g_auto (WaitForMessageData) wfm = WAIT_FOR_MESSAGE_DATA_INIT (data->loop);

    /* Check proper range request */
    message = soup_message_new (SOUP_METHOD_GET, data->result_uri);
    SoupMessageHeaders *request_headers = soup_message_get_request_headers (message);
    SoupMessageHeaders *response_headers = soup_message_get_response_headers (message);

    soup_message_headers_set_range (request_headers, 0, 2047);
    schedule_request_and_wait (session, message, &wfm);
    g_assert_cmpint (soup_message_get_status(message), ==, SOUP_STATUS_PARTIAL_CONTENT);

    g_assert_no_error (wfm.error);
    g_assert (soup_message_headers_get_content_range (response_headers, &start, &end, &total_length));
    content_length = soup_message_headers_get_content_length (response_headers);
    g_assert_cmpint (start, ==, 0);
    g_assert_cmpint (end, ==, 2047);
    g_assert_cmpint (total_length, ==, size);
    g_assert_cmpint (content_length, ==, 2048);
    g_assert (memcmp (g_bytes_get_data(wfm.data, NULL), g_mapped_file_get_contents (file) + start, content_length) == 0);
    g_object_unref (message);
    wait_for_message_data_reset (&wfm);

    /* Request first byte */
    message = soup_message_new (SOUP_METHOD_GET, data->result_uri);
    request_headers = soup_message_get_request_headers (message);
    response_headers = soup_message_get_response_headers (message);
    soup_message_headers_set_range (request_headers, 0, 0);
    schedule_request_and_wait (session, message, &wfm);
    g_assert_no_error (wfm.error);
    g_assert_cmpint (soup_message_get_status(message), ==, SOUP_STATUS_PARTIAL_CONTENT);

    g_assert (soup_message_headers_get_content_range (response_headers, &start, &end, &total_length));
    content_length = soup_message_headers_get_content_length (response_headers);
    g_assert_cmpint (start, ==, 0);
    g_assert_cmpint (end, ==, 0);
    g_assert_cmpint (total_length, ==, size);
    g_assert_cmpint (content_length, ==, 1);
    g_assert (memcmp (g_bytes_get_data(wfm.data, NULL), g_mapped_file_get_contents (file) + start, content_length) == 0);
    g_object_unref (message);
    wait_for_message_data_reset (&wfm);

    /* Request last 100 bytes */
    message = g_object_ref (soup_message_new (SOUP_METHOD_GET, data->result_uri));
    request_headers = soup_message_get_request_headers (message);
    response_headers = soup_message_get_response_headers (message);
    soup_message_headers_set_range (request_headers, -100, -1);
    schedule_request_and_wait (session, message, &wfm);
    g_assert_no_error (wfm.error);
    g_assert_cmpint (soup_message_get_status(message), ==, SOUP_STATUS_PARTIAL_CONTENT);

    g_assert (soup_message_headers_get_content_range (response_headers, &start, &end, &total_length));
    content_length = soup_message_headers_get_content_length (response_headers);
    g_assert_cmpint (start, ==, size - 100);
    g_assert_cmpint (end, ==, size - 1);
    g_assert_cmpint (total_length, ==, size);
    g_assert_cmpint (content_length, ==, 100);
    g_assert (memcmp (g_bytes_get_data (wfm.data, NULL), g_mapped_file_get_contents (file) + start, content_length) == 0);
    g_object_unref (message);
    wait_for_message_data_reset (&wfm);

    /* Request last 100 bytes by using negative offset */
    message = g_object_ref (soup_message_new (SOUP_METHOD_GET, data->result_uri));
    request_headers = soup_message_get_request_headers (message);
    response_headers = soup_message_get_response_headers (message);
    soup_message_headers_set_range (request_headers, size - 100, -1);
    schedule_request_and_wait (session, message, &wfm);
    g_assert_no_error (wfm.error);
    g_assert_cmpint (soup_message_get_status(message), ==, SOUP_STATUS_PARTIAL_CONTENT);

    g_assert (soup_message_headers_get_content_range (response_headers, &start, &end, &total_length));
    content_length = soup_message_headers_get_content_length (response_headers);
    g_assert_cmpint (start, ==, size - 100);
    g_assert_cmpint (end, ==, size - 1);
    g_assert_cmpint (total_length, ==, size);
    g_assert_cmpint (content_length, ==, 100);
    g_assert (memcmp (g_bytes_get_data (wfm.data, NULL), g_mapped_file_get_contents (file) + start, content_length) == 0);
    g_object_unref (message);
    wait_for_message_data_reset (&wfm);

    /* Check range request beyond file end */
    message = g_object_ref (soup_message_new (SOUP_METHOD_HEAD, data->result_uri));
    request_headers = soup_message_get_request_headers (message);
    soup_message_headers_set_range (request_headers, 0, size + 1);
    schedule_request_and_wait (session, message, &wfm);
    g_assert_no_error (wfm.error);
    g_assert_cmpint (soup_message_get_status(message), ==, SOUP_STATUS_REQUESTED_RANGE_NOT_SATISFIABLE);
    g_object_unref (message);
}

static void
test_upnp_fileserver_http_server_content_features (HostFileTestData *data, gconstpointer user_data)
{
    g_autoptr (SoupSession) session = NULL;
    SoupMessage *message;
    g_auto (WaitForMessageData) wfm = WAIT_FOR_MESSAGE_DATA_INIT (data->loop);

    session = soup_session_new ();
    message = soup_message_new (SOUP_METHOD_HEAD, data->result_uri);
    SoupMessageHeaders *request_headers = soup_message_get_request_headers (message);
    SoupMessageHeaders *response_headers = soup_message_get_response_headers (message);
    soup_message_headers_append (request_headers, "getContentFeatures.dlna.org", "1");
    schedule_request_and_wait (session, message, &wfm);
    g_assert_no_error (wfm.error);
    g_assert_cmpint (soup_message_get_status (message), ==, SOUP_STATUS_OK);
    g_assert_cmpstr (soup_message_headers_get_one (response_headers, "contentFeatures.dlna.org"), ==, "*");
    g_object_unref (message);
    wait_for_message_data_reset (&wfm);

    g_hash_table_insert (data->in_params, g_strdup ("DLNAProfile"), g_variant_new_string ("JPEG_SM"));

    message = soup_message_new (SOUP_METHOD_HEAD, data->result_uri);
    request_headers = soup_message_get_request_headers (message);
    response_headers = soup_message_get_response_headers (message);
    soup_message_headers_append (request_headers, "getContentFeatures.dlna.org", "1");
    schedule_request_and_wait (session, message, &wfm);

    g_assert_no_error (wfm.error);
    g_assert_cmpint (soup_message_get_status(message), ==, SOUP_STATUS_OK);
    g_assert_cmpstr (soup_message_headers_get_one (response_headers, "contentFeatures.dlna.org"), ==,
                     "http-get:*:image/jpeg:DLNA.ORG_PN=JPEG_SM;DLNA.ORG_OP=01");
    g_object_unref (message);
}

typedef struct {
    GMainLoop         *loop;
    MockDMR           *dmr;
    GUPnPDeviceProxy  *proxy;
    KorvaUPnPDevice   *device;
    GUPnPControlPoint *cp;
    gboolean           init_result;
    GError            *init_error;
    GError            *result_error;
    char              *result_tag;
} UPnPDeviceData;

static void
device_setup_on_device_init (GObject      *object,
                             GAsyncResult *result,
                             gpointer      user_data)
{
    UPnPDeviceData *data = (UPnPDeviceData *) user_data;

    data->init_result = g_async_initable_init_finish (G_ASYNC_INITABLE (object),
                                                      result,
                                                      &(data->init_error));
    g_main_loop_quit (data->loop);
}

static void
device_setup_on_proxy_available (GUPnPControlPoint *cp,
                                 GUPnPDeviceProxy  *proxy,
                                 gpointer           user_data)
{
    UPnPDeviceData *data = (UPnPDeviceData *) user_data;

    data->proxy = g_object_ref (proxy);
    data->device = g_object_new (KORVA_TYPE_UPNP_DEVICE,
                                 "proxy", data->proxy,
                                 NULL);
    g_async_initable_init_async (G_ASYNC_INITABLE (data->device),
                                 G_PRIORITY_DEFAULT,
                                 NULL,
                                 device_setup_on_device_init,
                                 data);
}

static gboolean
test_device_fatal_handler (const gchar   *log_domain,
                           GLogLevelFlags log_level,
                           const gchar   *message,
                           gpointer       user_data)
{
    if (log_domain == NULL && ((log_level & G_LOG_LEVEL_WARNING) == G_LOG_LEVEL_WARNING)) {
        return FALSE;
    }

    return TRUE;
}

static void
test_upnp_device_setup (UPnPDeviceData *data, gconstpointer user_data)
{
    GUPnPContext *context;
    int dmr_fault = GPOINTER_TO_INT (user_data);

    memset (data, 0, sizeof (UPnPDeviceData));

    data->loop = g_main_loop_new (NULL, FALSE);
    data->dmr = mock_dmr_new (dmr_fault);
    if (dmr_fault == MOCK_DMR_FAULT_PROTOCOL_INFO_CALL_INVALID ||
        dmr_fault == MOCK_DMR_FAULT_PROTOCOL_INFO_CALL_ERROR) {
        mock_dmr_set_protocol_info (data->dmr, NULL, NULL);
        g_test_log_set_fatal_handler (test_device_fatal_handler, NULL);
    } else {
        mock_dmr_set_protocol_info (data->dmr, NULL, "*:*:*:*,http-get:*:image/jpeg:*");
    }
    gupnp_root_device_set_available(GUPNP_ROOT_DEVICE (data->dmr), TRUE);
    context = gupnp_device_info_get_context (GUPNP_DEVICE_INFO (data->dmr));
    data->cp = gupnp_control_point_new (context, MOCK_DMR_UDN);
    gssdp_resource_browser_set_active (GSSDP_RESOURCE_BROWSER (data->cp), TRUE);
    g_signal_connect (data->cp, "device-proxy-available", G_CALLBACK (device_setup_on_proxy_available), data);

    g_main_loop_run (data->loop);
}

static void
test_upnp_device_teardown (UPnPDeviceData *data, gconstpointer user_data)
{
    g_clear_object (&(data->device));
    g_clear_object (&(data->cp));
    g_clear_object (&(data->dmr));

    g_main_loop_unref (data->loop);
}

#define KEY_UID (1 << 0)
#define KEY_DISPLAY_NAME (1 << 1)
#define KEY_ICON_URI (1 << 2)
#define KEY_PROTOCOL (1 << 3)
#define KEY_TYPE (1 << 4)
#define KEY_ALL (KEY_DISPLAY_NAME | KEY_ICON_URI | KEY_PROTOCOL | KEY_TYPE | KEY_UID)

static void
test_upnp_device (UPnPDeviceData *data, gconstpointer user_data)
{
    GVariant *info, *value;
    GVariantIter iter;
    guint found_keys = 0;
    char *key;
    int fault = GPOINTER_TO_INT (user_data);

    if (fault == MOCK_DMR_FAULT_NO_AV_TRANSPORT) {
        g_main_loop_run (data->loop);
    }

    if (fault == MOCK_DMR_FAULT_PROTOCOL_INFO_CALL_INVALID || fault == MOCK_DMR_FAULT_NO_AV_TRANSPORT ||
        fault == MOCK_DMR_FAULT_NO_CONNECTION_MANAGER || fault == MOCK_DMR_FAULT_PROTOCOL_INFO_CALL_ERROR) {
        g_assert (!data->init_result);
        g_assert (data->init_error != NULL);
        g_assert (data->init_error->domain == KORVA_UPNP_DEVICE_ERROR);
        g_assert_cmpint (data->init_error->code, ==, MISSING_SERVICE);

        return;
    } else if (fault == MOCK_DMR_FAULT_GET_TRANSPORT_INFO_FAIL) {
        g_assert (!data->init_result);
        g_assert (data->init_error != NULL);
        g_assert (data->init_error->domain == KORVA_UPNP_DEVICE_ERROR);
        g_assert_cmpint (data->init_error->code, ==, MISSING_SERVICE);

        return;
    }

    g_assert (data->init_result);
    g_assert (data->init_error == NULL);

    g_assert_cmpint (korva_device_get_protocol (KORVA_DEVICE (data->device)), ==, DEVICE_PROTOCOL_UPNP);

    info = korva_device_serialize (KORVA_DEVICE (data->device));

    g_assert_cmpstr ((const char *) g_variant_get_type (info), ==, (const char *) G_VARIANT_TYPE_VARDICT);
    g_variant_iter_init (&iter, info);

    /* Check the mandatory keys */
    while (g_variant_iter_next (&iter, "{sv}", &key, &value)) {
        if (g_strcmp0 (key, "UID") == 0) {
            found_keys |= KEY_UID;
            g_assert_cmpstr (g_variant_get_string (value, NULL), ==, MOCK_DMR_UDN);
            g_assert_cmpstr (g_variant_get_string (value, NULL), ==, korva_device_get_uid (KORVA_DEVICE (data->device)));
        } else if (g_strcmp0 (key, "DisplayName") == 0) {
            found_keys |= KEY_DISPLAY_NAME;
            g_assert_cmpstr (g_variant_get_string (value, NULL), ==, gupnp_device_info_get_friendly_name (GUPNP_DEVICE_INFO (data->dmr)));
            g_assert_cmpstr (g_variant_get_string (value, NULL), ==, korva_device_get_display_name (KORVA_DEVICE (data->device)));
        } else if (g_strcmp0 (key, "IconURI") == 0) {
            found_keys |= KEY_ICON_URI;
            g_assert (g_variant_get_string (value, NULL));
            g_assert_cmpstr (g_variant_get_string (value, NULL), !=, "");
            g_assert_cmpstr (g_variant_get_string (value, NULL), ==, korva_device_get_icon_uri (KORVA_DEVICE (data->device)));
        } else if (g_strcmp0 (key, "Protocol") == 0) {
            found_keys |= KEY_PROTOCOL;
            g_assert_cmpstr (g_variant_get_string (value, NULL), ==, "UPnP");
        } else if (g_strcmp0 (key, "Type") == 0) {
            found_keys |= KEY_TYPE;
            g_assert_cmpint (g_variant_get_uint32 (value), ==, DEVICE_TYPE_PLAYER);
            g_assert_cmpint (g_variant_get_uint32 (value), ==, korva_device_get_device_type (KORVA_DEVICE (data->device)));
        }
    }

    /* check that we get the default icon as the DMR doesn't have one */
    g_assert_cmpstr (korva_device_get_icon_uri (KORVA_DEVICE (data->device)), ==,
                     korva_icon_cache_get_default (DEVICE_TYPE_PLAYER));

    g_assert_cmpint (found_keys & KEY_ALL, ==, KEY_ALL);
}

static void
test_upnp_device_multiple_proxies (UPnPDeviceData *data, gconstpointer user_data)
{
    g_assert (data->proxy != NULL);
    korva_upnp_device_add_proxy (data->device, data->proxy);
    g_assert (!korva_upnp_device_remove_proxy (data->device, data->proxy));
    g_assert (korva_upnp_device_remove_proxy (data->device, data->proxy));
}

static void
on_test_upnp_device_share_push_async (GObject      *source,
                                      GAsyncResult *res,
                                      gpointer      user_data)
{
    UPnPDeviceData *data = (UPnPDeviceData *) user_data;

    data->result_tag = korva_device_push_finish (KORVA_DEVICE (source),
                                                 res,
                                                 &(data->result_error));
    g_main_loop_quit (data->loop);
}

static void
on_test_upnp_device_share_unshare_async (GObject      *source,
                                         GAsyncResult *res,
                                         gpointer      user_data)
{
    UPnPDeviceData *data = (UPnPDeviceData *) user_data;

    korva_device_unshare_finish (KORVA_DEVICE (source),
                                 res,
                                 &(data->result_error));

    g_main_loop_quit (data->loop);
}

static void
test_upnp_device_share (UPnPDeviceData *data, gconstpointer user_data)
{
    GVariantBuilder *source;
    GFile *file;
    char *uri;
    KorvaUPnPFileServer *server;

    /* create server instance here to make sure the device uses the same server */
    server = korva_upnp_file_server_get_default ();
    g_assert (korva_upnp_file_server_idle (server));

    file = g_file_new_for_commandline_arg (TEST_DATA_DIR "/test-upnp-image.jpg");
    uri = g_file_get_uri (file);

    source = g_variant_builder_new (G_VARIANT_TYPE_VARDICT);
    g_variant_builder_add (source, "{sv}", "URI", g_variant_new_string (uri));
    korva_device_push_async (KORVA_DEVICE (data->device),
                             g_variant_builder_end (source),
                             NULL,
                             on_test_upnp_device_share_push_async,
                             data);

    g_main_loop_run (data->loop);

    g_assert (data->result_error == NULL);
    g_assert (data->result_tag != NULL);
    g_assert (!korva_upnp_file_server_idle (server));

    korva_device_unshare_async (KORVA_DEVICE (data->device),
                                data->result_tag,
                                NULL,
                                on_test_upnp_device_share_unshare_async,
                                data);

    g_main_loop_run (data->loop);

    g_assert (data->result_error == NULL);
    g_assert (korva_upnp_file_server_idle (server));

    korva_device_unshare_async (KORVA_DEVICE (data->device),
                                "ThisIsAnInvalidTag",
                                NULL,
                                on_test_upnp_device_share_unshare_async,
                                data);

    g_main_loop_run (data->loop);

    g_assert (data->result_error != NULL);
    g_assert_cmpint (data->result_error->domain, ==, KORVA_CONTROLLER1_ERROR);
    g_assert_cmpint (data->result_error->code, ==, KORVA_CONTROLLER1_ERROR_NO_SUCH_TRANSFER);

    g_object_unref (server);
    g_free (uri);
}

static void
test_upnp_device_share_transport_locked (UPnPDeviceData *data, gconstpointer user_data)
{
    GVariantBuilder *source;
    GFile *file;
    char *uri;
    KorvaUPnPFileServer *server;

    /* create server instance here to make sure the device uses the same server */
    server = korva_upnp_file_server_get_default ();
    g_assert (korva_upnp_file_server_idle (server));

    file = g_file_new_for_commandline_arg (TEST_DATA_DIR "/test-upnp-image.jpg");
    uri = g_file_get_uri (file);

    source = g_variant_builder_new (G_VARIANT_TYPE_VARDICT);
    g_variant_builder_add (source, "{sv}", "URI", g_variant_new_string (uri));
    korva_device_push_async (KORVA_DEVICE (data->device),
                             g_variant_builder_end (source),
                             NULL,
                             on_test_upnp_device_share_push_async,
                             data);

    g_main_loop_run (data->loop);

    g_assert (data->result_error == NULL);
    g_assert (data->result_tag != NULL);
    g_assert (!korva_upnp_file_server_idle (server));

    korva_device_unshare_async (KORVA_DEVICE (data->device),
                                data->result_tag,
                                NULL,
                                on_test_upnp_device_share_unshare_async,
                                data);

    g_main_loop_run (data->loop);

    g_assert (data->result_error == NULL);
    g_assert (korva_upnp_file_server_idle (server));

    /* Check that we don't get stuck in an endless loop when the transport
     * doesn't get unlocked
     */
    mock_dmr_set_fault (data->dmr, MOCK_DMR_FAULT_TRANSPORT_LOCKED);

    source = g_variant_builder_new (G_VARIANT_TYPE_VARDICT);
    g_variant_builder_add (source, "{sv}", "URI", g_variant_new_string (uri));
    korva_device_push_async (KORVA_DEVICE (data->device),
                             g_variant_builder_end (source),
                             NULL,
                             on_test_upnp_device_share_push_async,
                             data);

    g_main_loop_run (data->loop);

    g_assert (data->result_error != NULL);
    g_print ("%s\n", data->result_error->message);
    g_assert (data->result_tag == NULL);
    g_object_unref (server);
}

static void
test_upnp_device_share_ops_fail (UPnPDeviceData *data, gconstpointer user_data)
{
    GVariantBuilder *source;
    GFile *file;
    char *uri;
    KorvaUPnPFileServer *server;
    int fault = GPOINTER_TO_INT (user_data);

    /* create server instance here to make sure the device uses the same server */
    server = korva_upnp_file_server_get_default ();
    g_assert (korva_upnp_file_server_idle (server));

    file = g_file_new_for_commandline_arg (TEST_DATA_DIR "/test-upnp-image.jpg");
    uri = g_file_get_uri (file);

    source = g_variant_builder_new (G_VARIANT_TYPE_VARDICT);
    g_variant_builder_add (source, "{sv}", "URI", g_variant_new_string (uri));
    korva_device_push_async (KORVA_DEVICE (data->device),
                             g_variant_builder_end (source),
                             NULL,
                             on_test_upnp_device_share_push_async,
                             data);

    g_main_loop_run (data->loop);

    if (fault == MOCK_DMR_FAULT_PLAY_FAIL) {
        g_assert (korva_upnp_file_server_idle (server));
        g_assert (data->result_error != NULL);
        g_assert (data->result_tag == NULL);
    } else {
        g_assert (!korva_upnp_file_server_idle (server));
        g_assert (data->result_error == NULL);
        g_assert (data->result_tag != NULL);
    }

    if (fault == MOCK_DMR_FAULT_STOP_FAIL) {
        korva_device_unshare_async (KORVA_DEVICE (data->device),
                                    data->result_tag,
                                    NULL,
                                    on_test_upnp_device_share_unshare_async,
                                    data);

        g_main_loop_run (data->loop);

        g_assert (korva_upnp_file_server_idle (server));
        g_assert (data->result_error != NULL);
    }

    g_object_unref (server);
}

static void
test_upnp_device_share_not_compatible (UPnPDeviceData *data, gconstpointer user_data)
{
    GVariantBuilder *source;
    GFile *file;
    char *uri;
    KorvaUPnPFileServer *server;

    /* create server instance here to make sure the device uses the same server */
    mock_dmr_set_protocol_info (data->dmr, "", "");
    server = korva_upnp_file_server_get_default ();
    g_assert (korva_upnp_file_server_idle (server));

    file = g_file_new_for_commandline_arg (TEST_DATA_DIR "/test-upnp-image.jpg");
    uri = g_file_get_uri (file);

    source = g_variant_builder_new (G_VARIANT_TYPE_VARDICT);
    g_variant_builder_add (source, "{sv}", "URI", g_variant_new_string (uri));
    korva_device_push_async (KORVA_DEVICE (data->device),
                             g_variant_builder_end (source),
                             NULL,
                             on_test_upnp_device_share_push_async,
                             data);

    g_main_loop_run (data->loop);

    g_assert (data->result_error != NULL);
    g_assert (data->result_tag == NULL);
    g_assert_cmpint (data->result_error->domain, ==, KORVA_CONTROLLER1_ERROR);
    g_assert_cmpint (data->result_error->code, ==, KORVA_CONTROLLER1_ERROR_NOT_COMPATIBLE);
    g_assert (korva_upnp_file_server_idle (server));

    g_object_unref (server);
}

int main (int argc, char *argv[])
{
    korva_icon_cache_init ();
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/korva/server/upnp/fileserver/single-instance",
                     test_upnp_fileserver_single_instance);

    g_test_add ("/korva/server/upnp/fileserver/host-file",
                HostFileTestData,
                NULL,
                test_host_file_setup,
                test_upnp_fileserver_host_file,
                test_host_file_teardown);

    g_test_add ("/korva/server/upnp/fileserver/host-file/dont-override-data",
                HostFileTestData,
                NULL,
                test_host_file_setup,
                test_upnp_fileserver_host_file_dont_override_data,
                test_host_file_teardown);

    g_test_add ("/korva/server/upnp/fileserver/host-file/timeout",
                HostFileTestData,
                NULL,
                test_host_file_setup,
                test_upnp_fileserver_host_file_timeout,
                test_host_file_teardown);

    g_test_add ("/korva/server/upnp/fileserver/host-file/timeout2",
                HostFileTestData,
                NULL,
                test_upnp_fileserver_http_server_setup,
                test_upnp_fileserver_host_file_timeout2,
                test_host_file_teardown);

    g_test_add ("/korva/server/upnp/fileserver/host-file/non-existing",
                HostFileTestData,
                NULL,
                test_host_file_setup,
                test_upnp_fileserver_host_file_error,
                test_host_file_teardown);

    g_test_add ("/korva/server/upnp/fileserver/host-file/not-accessible",
                HostFileTestData,
                NULL,
                test_host_file_setup,
                test_upnp_file_server_host_file_no_access,
                test_host_file_teardown);

    g_test_add ("/korva/server/upnp/fileserver/http-server",
                HostFileTestData,
                NULL,
                test_upnp_fileserver_http_server_setup,
                test_upnp_fileserver_http_server,
                test_host_file_teardown);

    g_test_add ("/korva/server/upnp/fileserver/http-server/ranges",
                HostFileTestData,
                NULL,
                test_upnp_fileserver_http_server_setup,
                test_upnp_fileserver_http_server_ranges,
                test_host_file_teardown);

    g_test_add ("/korva/server/upnp/fileserver/http-server/content-features",
                HostFileTestData,
                NULL,
                test_upnp_fileserver_http_server_setup,
                test_upnp_fileserver_http_server_content_features,
                test_host_file_teardown);

    g_test_add ("/korva/server/upnp/device",
                UPnPDeviceData,
                NULL,
                test_upnp_device_setup,
                test_upnp_device,
                test_upnp_device_teardown);

    g_test_add ("/korva/server/upnp/device/protocol_info_error",
                UPnPDeviceData,
                GINT_TO_POINTER (MOCK_DMR_FAULT_PROTOCOL_INFO_CALL_ERROR),
                test_upnp_device_setup,
                test_upnp_device,
                test_upnp_device_teardown);

    g_test_add ("/korva/server/upnp/device/protocol_info_invalid",
                UPnPDeviceData,
                GINT_TO_POINTER (MOCK_DMR_FAULT_PROTOCOL_INFO_CALL_INVALID),
                test_upnp_device_setup,
                test_upnp_device,
                test_upnp_device_teardown);

    g_test_add ("/korva/server/upnp/device/transport-info-fail",
                UPnPDeviceData,
                GINT_TO_POINTER (MOCK_DMR_FAULT_GET_TRANSPORT_INFO_FAIL),
                test_upnp_device_setup,
                test_upnp_device,
                test_upnp_device_teardown);

    g_test_add ("/korva/server/upnp/device/multiple-proxies",
                UPnPDeviceData,
                NULL,
                test_upnp_device_setup,
                test_upnp_device_multiple_proxies,
                test_upnp_device_teardown);

/*    g_test_add ("/korva/server/upnp/device/no-avtransport",
                UPnPDeviceData,
                GINT_TO_POINTER (MOCK_DMR_FAULT_NO_AV_TRANSPORT),
                test_upnp_device_setup,
                test_upnp_device,
                test_upnp_device_teardown);

    g_test_add ("/korva/server/upnp/device/no-connectionmanager",
                UPnPDeviceData,
                GINT_TO_POINTER (MOCK_DMR_FAULT_NO_CONNECTION_MANAGER),
                test_upnp_device_setup,
                test_upnp_device,
                test_upnp_device_teardown);
 */

    g_test_add ("/korva/server/upnp/device/share",
                UPnPDeviceData,
                NULL,
                test_upnp_device_setup,
                test_upnp_device_share,
                test_upnp_device_teardown);

    g_test_add ("/korva/server/upnp/device/share/transport-locked",
                UPnPDeviceData,
                GINT_TO_POINTER (MOCK_DMR_FAULT_TRANSPORT_LOCKED_ONCE),
                test_upnp_device_setup,
                test_upnp_device_share_transport_locked,
                test_upnp_device_teardown);

    g_test_add ("/korva/server/upnp/device/share/play-fail",
                UPnPDeviceData,
                GINT_TO_POINTER (MOCK_DMR_FAULT_PLAY_FAIL),
                test_upnp_device_setup,
                test_upnp_device_share_ops_fail,
                test_upnp_device_teardown);

    g_test_add ("/korva/server/upnp/device/share/stop-fail",
                UPnPDeviceData,
                GINT_TO_POINTER (MOCK_DMR_FAULT_STOP_FAIL),
                test_upnp_device_setup,
                test_upnp_device_share_ops_fail,
                test_upnp_device_teardown);

    g_test_add ("/korva/server/upnp/device/share/not-compatible",
                UPnPDeviceData,
                GINT_TO_POINTER (MOCK_DMR_FAULT_EMPTY_PROTOCOL_INFO),
                test_upnp_device_setup,
                test_upnp_device_share_not_compatible,
                test_upnp_device_teardown);

    g_test_run ();

    return 0;
}
