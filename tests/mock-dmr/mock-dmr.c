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

#include <libxml/xpath.h>

#include "mock-dmr.h"


struct _MockDMRPrivate {
    GUPnPServiceInfo *connection_manager;
	GUPnPServiceInfo *av_transport;
	char *source_protocol_info;
	char *sink_protocol_info;
	MockDMRFault fault;
	gboolean unlocked;
    char *state;
};

G_DEFINE_TYPE(MockDMR, mock_dmr, GUPNP_TYPE_ROOT_DEVICE)

enum MockDMRProperties {
	PROP_0,
	PROP_FAULT,
    PROP_STATE
};

static void
mock_dmr_constructed (GObject *object);

static void
mock_dmr_finalize (GObject *object);

static void
mock_dmr_set_property (GObject      *obj,
                       guint         property_id,
                       const GValue *value,
                       GParamSpec   *pspec);

static void
mock_dmr_get_property (GObject      *obj,
                       guint         property_id,
                       GValue *value,
                       GParamSpec   *pspec);

static void
mock_dmr_class_init (MockDMRClass *klass)
{
    GObjectClass *object_class;
	
    object_class = G_OBJECT_CLASS (klass);
    object_class->constructed = mock_dmr_constructed;
	object_class->finalize = mock_dmr_finalize;
	object_class->get_property = mock_dmr_get_property;
	object_class->set_property = mock_dmr_set_property;

    g_object_class_install_property (object_class,
                                     PROP_FAULT,
                                     g_param_spec_int ("fault",
                                                       "fault",
                                                       "fault",
                                                       MOCK_DMR_FAULT_NONE,
                                                       MOCK_DMR_FAULT_COUNT - 1,
                                                       MOCK_DMR_FAULT_NONE,
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_STATIC_BLURB |
                                                       G_PARAM_STATIC_NAME |
                                                       G_PARAM_STATIC_NICK));

    g_object_class_install_property (object_class,
                                     PROP_STATE,
                                     g_param_spec_string ("state",
                                                          "state",
                                                          "state",
                                                          "UNKNOWN",
                                                          G_PARAM_CONSTRUCT_ONLY |
                                                          G_PARAM_READWRITE |
                                                          G_PARAM_STATIC_BLURB |
                                                          G_PARAM_STATIC_NAME |
                                                          G_PARAM_STATIC_NICK));
	
    g_type_class_add_private (klass, sizeof (MockDMRPrivate));
}

static void
mock_dmr_init (MockDMR *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                              TYPE_MOCK_DMR,
                                              MockDMRPrivate);
}

static void
mock_dmr_set_property (GObject      *obj,
                       guint         property_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
    MockDMR *self = MOCK_DMR (obj);

    switch (property_id) {
        case PROP_FAULT:
            self->priv->fault = g_value_get_int (value);
			break;
        case PROP_STATE:
            if (self->priv->state != NULL) {
                g_free (self->priv->state);
            }
            self->priv->state = g_value_dup_string (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
    }
}

static void
mock_dmr_get_property (GObject      *obj,
                       guint         property_id,
                       GValue *value,
                       GParamSpec   *pspec)
{
    MockDMR *self = MOCK_DMR (obj);

    switch (property_id) {
        case PROP_FAULT:
            g_value_set_int (value, self->priv->fault);
            break;
        case PROP_STATE:
            g_value_set_string (value, self->priv->state);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
    }
}


static void
on_get_protocol_info (GUPnPService *service,
                      GUPnPServiceAction *action,
                      gpointer user_data)
{
    MockDMR *self = MOCK_DMR (user_data);

	if (self->priv->fault == MOCK_DMR_FAULT_PROTOCOL_INFO_CALL_ERROR) {
		gupnp_service_action_return_error (action, 501, "Deliberately fail");

		return;
	} else if (self->priv->fault == MOCK_DMR_FAULT_EMPTY_PROTOCOL_INFO) {
		gupnp_service_action_set (action, "Source", G_TYPE_STRING, "", NULL);
		gupnp_service_action_set (action, "Sink", G_TYPE_STRING, "", NULL);
        gupnp_service_action_return (action);

        return;
    }

	if (self->priv->source_protocol_info != NULL) { 
		gupnp_service_action_set (action, "Source",
    		                  	  G_TYPE_STRING, self->priv->source_protocol_info,
        		                  NULL);
	}

	if (self->priv->sink_protocol_info != NULL &&
	    self->priv->fault != MOCK_DMR_FAULT_PROTOCOL_INFO_CALL_INVALID) {
		gupnp_service_action_set (action, "Sink",
    		                      G_TYPE_STRING, self->priv->sink_protocol_info,
		                          NULL);
	}
    gupnp_service_action_return (action);
}

static void
on_play (GUPnPService *service,
         GUPnPServiceAction *action,
         MockDMR *self)
{
    if (self->priv->fault == MOCK_DMR_FAULT_PLAY_FAIL) {
        gupnp_service_action_return_error (action, 721, "Deliberately fail");
    } else {
	    gupnp_service_action_return (action);
    }
}

static void
on_stop (GUPnPService *service,
         GUPnPServiceAction *action,
         MockDMR *self)
{
    if (self->priv->fault == MOCK_DMR_FAULT_STOP_FAIL) {
        gupnp_service_action_return_error (action, 701, "Deliberately fail");
    } else {
	    gupnp_service_action_return (action);
    }
}

static void
on_pause (GUPnPService *service,
          GUPnPServiceAction *action,
          gpointer user_data)
{
	gupnp_service_action_return (action);
}

static void
on_set_av_transport_uri (GUPnPService *service,
                         GUPnPServiceAction *action,
                         gpointer user_data)
{
    MockDMR *self = MOCK_DMR (user_data);
	GValue instance_id, uri, meta_data, last_change;
	GString *last_change_str;

	if (self->priv->fault == MOCK_DMR_FAULT_TRANSPORT_LOCKED ||
	    (self->priv->fault == MOCK_DMR_FAULT_TRANSPORT_LOCKED_ONCE &&
	     self->priv->unlocked)) {
		self->priv->unlocked = TRUE;
		gupnp_service_action_return_error (action, 705, "Transport locked!");

		return;
	}
	
	memset (&instance_id, 0, sizeof (GValue));
	memset (&uri, 0, sizeof (GValue));
	memset (&meta_data, 0, sizeof (GValue));
	memset (&last_change, 0, sizeof (GValue));
	
	g_value_init (&instance_id, G_TYPE_INT);
	g_value_init (&uri, G_TYPE_STRING);
	g_value_init (&meta_data, G_TYPE_STRING);

	gupnp_service_action_get_value (action, "InstanceID", &instance_id);
	gupnp_service_action_get_value (action, "CurrentURI", &uri);
	gupnp_service_action_get_value (action, "CurrentURIMetaData", &meta_data);

	g_assert (g_value_get_string (&uri) != NULL);
	g_assert (g_value_get_string (&meta_data) != NULL);
	g_assert_cmpint (g_value_get_int (&instance_id), ==, 0);

	last_change_str = g_string_new ("<Event xmlns=\"urn:schemas-upnp-org:metadata-1-0/AVT/\">");
	g_string_append (last_change_str, "<InstanceID val=\"0\">");
	g_string_append_printf (last_change_str, "<CurrentURI val=\"%s\" />", g_value_get_string (&uri));
	g_string_append (last_change_str, "</InstanceID></Event>");

	g_value_init (&last_change, G_TYPE_STRING);
	g_value_take_string (&last_change, last_change_str->str);

	gupnp_service_notify_value (GUPNP_SERVICE (self->priv->av_transport), "LastChange", &last_change);
	g_string_free (last_change_str, FALSE);
	
	gupnp_service_action_return (action);
}

static void
on_query_last_change (GUPnPService *service,
                      char         *variable,
                      GValue       *value,
                      MockDMR      *self)
{
    GString *last_change_str;

	last_change_str = g_string_new ("<Event xmlns=\"urn:schemas-upnp-org:metadata-1-0/AVT/\">");
	g_string_append (last_change_str, "<InstanceID val=\"0\">");
	g_string_append_printf (last_change_str, "<TransportState val=\"%s\" />", self->priv->state);
	g_string_append (last_change_str, "</InstanceID></Event>");

	g_value_init (value, G_TYPE_STRING);
	g_value_take_string (value, last_change_str->str);
}
                      

static void
mock_dmr_constructed (GObject *object)
{
    GUPnPContext *context;
    MockDMR *self = MOCK_DMR (object);

    context = gupnp_device_info_get_context (GUPNP_DEVICE_INFO (self));

    gupnp_context_host_path (context,
                             "MediaRenderer2.xml",
                             TEST_DATA_DIR"/mock-dmr/MediaRenderer2.xml");
    gupnp_context_host_path (context, 
                             "AVTransport2.xml",
                             TEST_DATA_DIR"/mock-dmr/AVTransport2.xml");
    gupnp_context_host_path (context, 
                             "ConnectionManager.xml",
                             TEST_DATA_DIR"/mock-dmr/ConnectionManager.xml");
    gupnp_context_host_path (context,
                             "RenderingControl2.xml",
                             TEST_DATA_DIR"/mock-dmr/RenderingControl2.xml");
    gupnp_root_device_set_available (GUPNP_ROOT_DEVICE (self), TRUE);

	if (self->priv->fault != MOCK_DMR_FAULT_NO_CONNECTION_MANAGER) { 
		self->priv->connection_manager = gupnp_device_info_get_service (GUPNP_DEVICE_INFO (self),
    		                                                            "urn:schemas-upnp-org:service:ConnectionManager:2");

		g_signal_connect (self->priv->connection_manager,
		                  "action-invoked::GetProtocolInfo",
		                  G_CALLBACK (on_get_protocol_info),
		                  self);

	}

	if (self->priv->fault != MOCK_DMR_FAULT_NO_AV_TRANSPORT) {
		self->priv->av_transport = gupnp_device_info_get_service (GUPNP_DEVICE_INFO (self),
		                                                          "urn:schemas-upnp-org:service:AVTransport:2");

		g_signal_connect (self->priv->av_transport,
		                  "action-invoked::Play",
		                  G_CALLBACK (on_play),
		                  self);
		
		g_signal_connect (self->priv->av_transport,
		                  "action-invoked::Pause",
		                  G_CALLBACK (on_pause),
		                  self);

		g_signal_connect (self->priv->av_transport,
		                  "action-invoked::Stop",
		                  G_CALLBACK (on_stop),
		                  self);

		g_signal_connect (self->priv->av_transport,
		                  "action-invoked::SetAVTransportURI",
		                  G_CALLBACK (on_set_av_transport_uri),
		                  self);

        g_signal_connect (self->priv->av_transport,
                          "query-variable::LastChange",
                          G_CALLBACK (on_query_last_change),
                          self);
	}
}

static void
mock_dmr_finalize (GObject *object)
{
	GObjectClass *parent_class;
	MockDMR *self = MOCK_DMR (object);

	g_clear_object (&(self->priv->connection_manager));
	g_clear_object (&(self->priv->av_transport));
	
	mock_dmr_set_protocol_info (self, NULL, NULL);
	
	parent_class = G_OBJECT_CLASS (mock_dmr_parent_class);
	parent_class->finalize (object);
}

#define XPATH_TEMPLATE "//*[.='%s']"
#define CONNECTION_MANAGER "urn:schemas-upnp-org:service:ConnectionManager:2"
#define AV_TRANSPORT "urn:schemas-upnp-org:service:AVTransport:2"

MockDMR *
mock_dmr_new (MockDMRFault fault)
{
    GUPnPContext *context;
    GError *error = NULL;
	GUPnPXMLDoc *doc;
    const char *state = "STOPPED";

    context = gupnp_context_new (NULL, "lo", 0, &error);
    g_assert (error == NULL);
	
	doc = gupnp_xml_doc_new_from_path (TEST_DATA_DIR"/mock-dmr/MediaRenderer2.xml", &error);
	g_assert (error == NULL);

	if (fault == MOCK_DMR_FAULT_NO_AV_TRANSPORT ||
	    fault == MOCK_DMR_FAULT_NO_CONNECTION_MANAGER) {

		xmlXPathContextPtr ctx;
		xmlXPathObjectPtr xpo;
		char *path, out;
		int out_len;

		path = g_strdup_printf (XPATH_TEMPLATE,
		                        fault == MOCK_DMR_FAULT_NO_AV_TRANSPORT ? AV_TRANSPORT : CONNECTION_MANAGER); 
		ctx = xmlXPathNewContext (doc->doc);
		xpo = xmlXPathEvalExpression ((const xmlChar *) path, ctx);
		if (xpo != NULL &&
		    xpo->type == XPATH_NODESET &&
		    !xmlXPathNodeSetIsEmpty (xpo->nodesetval)) {
			xmlNodePtr node;
			
			node = xmlXPathNodeSetItem (xpo->nodesetval, 0);

			xmlNodeSetContent (node, (const xmlChar *) "ThisIsInvalid");
		}

		xmlDocDumpMemoryEnc (doc->doc, (xmlChar **) &out, &out_len, "UTF-8");
		/* TODO: Dump file and use that instead */
		g_free (path);
	}

    if (fault == MOCK_DMR_FAULT_PLAY_FAIL || fault == MOCK_DMR_FAULT_STOP_FAIL) {
        state = "STOPPED";
    }

    return g_object_new (TYPE_MOCK_DMR,
                         "resource-factory", gupnp_resource_factory_get_default (),
                         "context", context,
                         "description-dir", TEST_DATA_DIR"/mock-dmr",
                         "description-path", "MediaRenderer2.xml",
                         "fault", fault,
                         "state", state,
                         "description-doc", doc,
                         NULL);    
}

void
mock_dmr_set_protocol_info (MockDMR *self, const char *source, const char *sink)
{
	if (self->priv->sink_protocol_info != NULL) {
		g_free (self->priv->sink_protocol_info);
		self->priv->sink_protocol_info = NULL;
	}

	if (self->priv->source_protocol_info != NULL) {
		g_free (self->priv->source_protocol_info);
		self->priv->source_protocol_info = NULL;
	}

	if (source != NULL) {
		self->priv->source_protocol_info = g_strdup (source);
	}

	if (sink != NULL) {
		self->priv->sink_protocol_info = g_strdup (sink);
	}
}

void
mock_dmr_set_fault (MockDMR *self, MockDMRFault fault)
{
	self->priv->fault = fault;
	self->priv->unlocked = FALSE;
}