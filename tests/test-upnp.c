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

#include "korva-upnp-file-server.h"
#include "korva-upnp-constants-private.h"

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
    GMainLoop *loop;
    char *result_uri;
    char *in_uri;
    KorvaUPnPFileServer *server;
    GFile *in_file;
    GHashTable *in_params;
    GHashTable *result_params;
    GError *result_error;
} HostFileTestData;

static void
test_host_file_setup (HostFileTestData *data, gconstpointer user_data)
{
    memset (data, 0, sizeof (HostFileTestData));

    data->in_file = g_file_new_for_commandline_arg (TEST_DATA_DIR"/test-upnp-image.jpg");
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
test_upnp_fileserver_host_file_on_host_file (GObject *source,
                                             GAsyncResult *res,
                                             gpointer user_data)
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
    SoupSession *session;
    SoupMessage *message;
    GHashTable *params;
    GVariant *value;

    params = data->in_params;

    korva_upnp_file_server_host_file_async (data->server,
                                            data->in_file,
                                            data->in_params,
                                            "127.0.0.1",
                                            "127.0.0.1",
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

    /* Test that the file is reachable under the uri */
    session = soup_session_async_new ();
    message = g_object_ref (soup_message_new (SOUP_METHOD_HEAD, data->result_uri));
    soup_session_queue_message (session, message, NULL, NULL);
    g_signal_connect_swapped (message, "finished", G_CALLBACK (g_main_loop_quit), data->loop);

    g_main_loop_run (data->loop);
    g_assert_cmpuint (message->status_code, ==, SOUP_STATUS_OK);
    g_object_unref (message);

    /* Share it again for different peer and verify we get the same data */
    korva_upnp_file_server_host_file_async (data->server,
                                            data->in_file,
                                            data->in_params,
                                            "127.0.0.1",
                                            "192.168.4.5",
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

    g_object_unref (session);
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
    char *uri;
    GFile *file;

    file = g_file_new_for_path ("/this/file/does/not/exist");
    uri = g_file_get_uri (file);
    g_hash_table_replace (data->in_params, g_strdup ("URI"), g_variant_new_string (uri));

    korva_upnp_file_server_host_file_async (data->server,
                                            file,
                                            data->in_params,
                                            "127.0.0.1",
                                            "127.0.0.1",
                                            test_upnp_fileserver_host_file_on_host_file,
                                            data);

    g_main_loop_run (data->loop);

    g_assert (data->result_uri == NULL);
    g_assert (data->result_error != NULL);
    g_assert_cmpint (data->result_error->code, ==, KORVA_CONTROLLER1_ERROR_FILE_NOT_FOUND);
    g_assert (data->result_params == NULL);    

    g_free (uri);
}

static void
test_upnp_file_server_host_file_no_access (HostFileTestData *data, gconstpointer user_data)
{
    int fd;
    char *template, *uri;
    GFile *file;

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
                                            test_upnp_fileserver_host_file_on_host_file,
                                            data);

    g_main_loop_run (data->loop);

    g_assert (data->result_uri == NULL);
    g_assert (data->result_error != NULL);
    g_assert_cmpint (data->result_error->code, ==, KORVA_CONTROLLER1_ERROR_NOT_ACCESSIBLE);
    g_assert (data->result_params == NULL);    

    close (fd);
    g_remove (template);
    
    g_free (uri);
    g_free (template);
}

static void
test_upnp_fileserver_host_file_timeout (HostFileTestData *data, gconstpointer user_data)
{
    if (!g_test_slow()) {
        return;
    }
    korva_upnp_file_server_host_file_async (data->server,
                                            data->in_file,
                                            data->in_params,
                                            "127.0.0.1",
                                            "127.0.0.1",
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

static void
test_upnp_fileserver_http_server_setup (HostFileTestData *data, gconstpointer user_data)
{
    test_host_file_setup (data, user_data);
    korva_upnp_file_server_host_file_async (data->server,
                                            data->in_file,
                                            data->in_params,
                                            "127.0.0.1",
                                            "127.0.0.1",
                                            test_upnp_fileserver_host_file_on_host_file,
                                            data);

    g_main_loop_run (data->loop);

    g_assert (data->result_uri != NULL);
    g_assert (data->result_error == NULL);
}

static void
test_upnp_fileserver_http_server (HostFileTestData *data, gconstpointer user_data)
{
    SoupSession *session;
    SoupMessage *message;
    char *base_uri, *uri, *needle;
    const char *empty_md5 = "d41d8cd98f00b204e9800998ecf8427e";

    base_uri = g_strdup (data->result_uri);
    needle = g_strrstr_len (base_uri, -1, "/") + 1;
    *needle = '\0';

    session = soup_session_async_new ();

    /* Check invalid HTTP method */
    message = soup_message_new (SOUP_METHOD_DELETE, data->result_uri);
    soup_session_queue_message (session, message, NULL, NULL);
    g_signal_connect_swapped (message, "finished", G_CALLBACK (g_main_loop_quit), data->loop);

    g_main_loop_run (data->loop);

    g_assert_cmpint (message->status_code, ==, SOUP_STATUS_METHOD_NOT_ALLOWED);

    /* Check URI format with invalid item format */
    uri = g_strconcat (base_uri, "should_not_work", NULL);
    message = soup_message_new (SOUP_METHOD_HEAD, uri);
    soup_session_queue_message (session, message, NULL, NULL);
    g_signal_connect_swapped (message, "finished", G_CALLBACK (g_main_loop_quit), data->loop);

    g_main_loop_run (data->loop);

    g_assert_cmpint (message->status_code, ==, SOUP_STATUS_NOT_FOUND);
    g_free (uri);

    /* Check URI format with invalid MD5 */
    uri = g_strconcat (base_uri, empty_md5, NULL);
    message = soup_message_new (SOUP_METHOD_HEAD, uri);
    soup_session_queue_message (session, message, NULL, NULL);
    g_signal_connect_swapped (message, "finished", G_CALLBACK (g_main_loop_quit), data->loop);

    g_main_loop_run (data->loop);

    g_assert_cmpint (message->status_code, ==, SOUP_STATUS_NOT_FOUND);
    g_free (uri);

    /* Check that we get a 404 when trying to access it with different IP */
    uri = g_strdup (data->result_uri);
    korva_upnp_file_server_host_file_async (data->server,
                                            data->in_file,
                                            data->in_params,
                                            "127.0.0.1",
                                            "192.168.4.5",
                                            test_upnp_fileserver_host_file_on_host_file,
                                            data);
    g_main_loop_run (data->loop);
    korva_upnp_file_server_unhost_file_for_peer (data->server, data->in_file, "127.0.0.1");
    message = soup_message_new (SOUP_METHOD_HEAD, uri);
    soup_session_queue_message (session, message, NULL, NULL);
    g_signal_connect_swapped (message, "finished", G_CALLBACK (g_main_loop_quit), data->loop);

    g_main_loop_run (data->loop);

    g_assert_cmpint (message->status_code, ==, SOUP_STATUS_NOT_FOUND);
    g_free (uri);

    g_object_unref (session);
    g_free (base_uri);
}

static void
schedule_request_and_wait (SoupSession *session, SoupMessage *message, HostFileTestData *data)
{
    soup_session_queue_message (session, message, NULL, NULL);
    g_signal_connect_swapped (message, "finished", G_CALLBACK (g_main_loop_quit), data->loop);
    g_main_loop_run (data->loop);
}

static void
test_upnp_fileserver_http_server_ranges (HostFileTestData *data, gconstpointer user_data)
{
    SoupSession *session;
    SoupMessage *message;
    goffset size;
    GVariant *value;
    goffset start = 0, end = 0, total_length = 0, content_length;
    GMappedFile *file;

    file = g_mapped_file_new (TEST_DATA_DIR "/test-upnp-image.jpg", FALSE, NULL);
    g_assert (file != NULL);

    value = g_hash_table_lookup (data->in_params, "Size");
    g_assert (value != NULL);
    size = g_variant_get_uint64 (value);
    g_assert (size > 2048);

    session = soup_session_async_new ();

    /* Check proper range request */
    message = g_object_ref (soup_message_new (SOUP_METHOD_GET, data->result_uri));
    soup_message_headers_set_range (message->request_headers, 0, 2047);
    schedule_request_and_wait (session, message, data);
    g_assert_cmpint (message->status_code, ==, SOUP_STATUS_PARTIAL_CONTENT);

    g_assert (soup_message_headers_get_content_range (message->response_headers, &start, &end, &total_length));
    content_length = soup_message_headers_get_content_length (message->response_headers);
    g_assert_cmpint (start, ==, 0);
    g_assert_cmpint (end, ==, 2047);
    g_assert_cmpint (total_length, ==, size);
    g_assert_cmpint (content_length, ==, 2048);
    g_assert (memcmp (message->response_body->data, g_mapped_file_get_contents (file) + start, content_length) == 0);
    g_object_unref (message);

    /* Request first byte */
    message = g_object_ref (soup_message_new (SOUP_METHOD_GET, data->result_uri));
    soup_message_headers_set_range (message->request_headers, 0, 0);
    schedule_request_and_wait (session, message, data);
    g_assert_cmpint (message->status_code, ==, SOUP_STATUS_PARTIAL_CONTENT);

    g_assert (soup_message_headers_get_content_range (message->response_headers, &start, &end, &total_length));
    content_length = soup_message_headers_get_content_length (message->response_headers);
    g_assert_cmpint (start, ==, 0);
    g_assert_cmpint (end, ==, 0);
    g_assert_cmpint (total_length, ==, size);
    g_assert_cmpint (content_length, ==, 1);
    g_assert (memcmp (message->response_body->data, g_mapped_file_get_contents (file) + start, content_length) == 0);
    g_object_unref (message);

    /* Request last 100 bytes */
    message = g_object_ref (soup_message_new (SOUP_METHOD_GET, data->result_uri));
    soup_message_headers_set_range (message->request_headers, -100, -1);
    schedule_request_and_wait (session, message, data);
    g_assert_cmpint (message->status_code, ==, SOUP_STATUS_PARTIAL_CONTENT);

    g_assert (soup_message_headers_get_content_range (message->response_headers, &start, &end, &total_length));
    content_length = soup_message_headers_get_content_length (message->response_headers);
    g_assert_cmpint (start, ==, size - 100);
    g_assert_cmpint (end, ==, size - 1);
    g_assert_cmpint (total_length, ==, size);
    g_assert_cmpint (content_length, ==, 100);
    g_assert (memcmp (message->response_body->data, g_mapped_file_get_contents (file) + start, content_length) == 0);
    g_object_unref (message);

    /* Request last 100 bytes by using negative offset */
    message = g_object_ref (soup_message_new (SOUP_METHOD_GET, data->result_uri));
    soup_message_headers_set_range (message->request_headers, size - 100, -1);
    schedule_request_and_wait (session, message, data);
    g_assert_cmpint (message->status_code, ==, SOUP_STATUS_PARTIAL_CONTENT);

    g_assert (soup_message_headers_get_content_range (message->response_headers, &start, &end, &total_length));
    content_length = soup_message_headers_get_content_length (message->response_headers);
    g_assert_cmpint (start, ==, size - 100);
    g_assert_cmpint (end, ==, size - 1);
    g_assert_cmpint (total_length, ==, size);
    g_assert_cmpint (content_length, ==, 100);
    g_assert (memcmp (message->response_body->data, g_mapped_file_get_contents (file) + start, content_length) == 0);
    g_object_unref (message);

    /* Check range request beyond file end */
    message = g_object_ref (soup_message_new (SOUP_METHOD_HEAD, data->result_uri));
    soup_message_headers_set_range (message->request_headers, 0, size + 1);
    schedule_request_and_wait (session, message, data);
    g_assert_cmpint (message->status_code, ==, SOUP_STATUS_REQUESTED_RANGE_NOT_SATISFIABLE);
    g_object_unref (message);

    /* Check range request with inverted parameters */
    message = g_object_ref (soup_message_new (SOUP_METHOD_HEAD, data->result_uri));
    soup_message_headers_set_range (message->request_headers, 10, 0);
    schedule_request_and_wait (session, message, data);
    g_assert_cmpint (message->status_code, ==, SOUP_STATUS_REQUESTED_RANGE_NOT_SATISFIABLE);
    g_object_unref (message);

    g_mapped_file_unref (file);
    g_object_unref (session);
}

static void
test_upnp_fileserver_http_server_content_features (HostFileTestData *data, gconstpointer user_data)
{
    SoupSession *session;
    SoupMessage *message;

    session = soup_session_async_new ();
    message = g_object_ref (soup_message_new (SOUP_METHOD_HEAD, data->result_uri));
    soup_message_headers_append (message->request_headers, "getContentFeatures.dlna.org", "1");
    schedule_request_and_wait (session, message, data);
    g_assert_cmpint (message->status_code, ==, SOUP_STATUS_OK);
    g_assert_cmpstr (soup_message_headers_get_one (message->response_headers, "contentFeatures.dlna.org"), ==, "*");
    g_object_unref (message);

    g_hash_table_insert (data->in_params, g_strdup ("DLNAProfile"), g_variant_new_string ("JPEG_SM"));

    message = g_object_ref (soup_message_new (SOUP_METHOD_HEAD, data->result_uri));
    soup_message_headers_append (message->request_headers, "getContentFeatures.dlna.org", "1");
    schedule_request_and_wait (session, message, data);
    g_assert_cmpint (message->status_code, ==, SOUP_STATUS_OK);
    g_assert_cmpstr (soup_message_headers_get_one (message->response_headers, "contentFeatures.dlna.org"), ==,
                     "http-get:*:image/jpeg:DLNA.ORG_PN=JPEG_SM;DLNA.ORG_OP=01");
    g_object_unref (message);
    g_object_unref (session);
}

int main(int argc, char *argv[])
{
    g_type_init ();
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

    g_test_run ();

    return 0;
}
