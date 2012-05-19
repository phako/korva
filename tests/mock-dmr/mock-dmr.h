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

#ifndef __MOCK_DMR_H__
#define __MOCK_DMR_H__

#include <glib-object.h>

#include <libgupnp/gupnp.h>

G_BEGIN_DECLS

#define MOCK_DMR_UDN "uuid:0a91ecf9-edc0-4631-a954-2a4e94d1e495"

GType mock_dmr_get_type (void);

#define TYPE_MOCK_DMR (mock_dmr_get_type ())
#define MOCK_DMR(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_MOCK_DMR, MockDMR))
#define IS_MOCK_DMR(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_MOCK_DMR))
#define MOCK_DMR_CLASS(obj) \
    (G_TYPE_CHECK_CLASS_CAST ((obj), TYPE_MOCK_DMR, MockDMRClass))
#define IS_MOCK_DMR_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_MOCK_DMR))
#define MOCK_DMR_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_MOCK_DMR, MockDMRClass))

typedef struct _MockDMRPrivate MockDMRPrivate;
typedef struct _MockDMR MockDMR;
typedef struct _MockDMRClass MockDMRClass;

struct _MockDMR {
    GUPnPRootDevice parent;

    MockDMRPrivate *priv;
};

struct _MockDMRClass {
    GUPnPRootDeviceClass parent_class;
};

typedef enum {
	MOCK_DMR_FAULT_NONE = 0,
	MOCK_DMR_FAULT_PROTOCOL_INFO_CALL_ERROR,
	MOCK_DMR_FAULT_PROTOCOL_INFO_CALL_INVALID,
	MOCK_DMR_FAULT_NO_AV_TRANSPORT,
	MOCK_DMR_FAULT_NO_CONNECTION_MANAGER,
	MOCK_DMR_FAULT_TRANSPORT_LOCKED,
	MOCK_DMR_FAULT_TRANSPORT_LOCKED_ONCE,
    MOCK_DMR_FAULT_PLAY_FAIL,
    MOCK_DMR_FAULT_STOP_FAIL,
    MOCK_DMR_FAULT_EMPTY_PROTOCOL_INFO,
	MOCK_DMR_FAULT_COUNT
} MockDMRFault;

MockDMR *
mock_dmr_new (MockDMRFault fault);

void
mock_dmr_set_protocol_info (MockDMR *self, const char *source, const char *sink);

void
mock_dmr_set_fault (MockDMR *self, MockDMRFault fault);
G_END_DECLS

#endif /* __MOCK_DMR_H__ */
