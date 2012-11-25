/* tui.h generated by valac 0.14.2, the Vala compiler, do not modify */


#ifndef __TUI_H__
#define __TUI_H__

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>

G_BEGIN_DECLS


#define TYPE_TRANSFER_UI (transfer_ui_get_type ())
#define TRANSFER_UI(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_TRANSFER_UI, TransferUi))
#define IS_TRANSFER_UI(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_TRANSFER_UI))
#define TRANSFER_UI_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), TYPE_TRANSFER_UI, TransferUiIface))

typedef struct _TransferUi TransferUi;
typedef struct _TransferUiIface TransferUiIface;

#define TYPE_TRANSFER_UI_PROXY (transfer_ui_proxy_get_type ())

struct _TransferUiIface {
	GTypeInterface parent_iface;
	gchar* (*register_transient_transfer) (TransferUi* self, const gchar* name, gint type, GError** error);
	void (*started) (TransferUi* self, const gchar* id, gdouble progress, GError** error);
	void (*set_values) (TransferUi* self, const gchar* id, GHashTable* key_values, GError** error);
	void (*done) (TransferUi* self, const gchar* id, GError** error);
	void (*cancelled) (TransferUi* self, const gchar* id, GError** error);
};


GType transfer_ui_proxy_get_type (void) G_GNUC_CONST;
guint transfer_ui_register_object (void* object, GDBusConnection* connection, const gchar* path, GError** error);
GType transfer_ui_get_type (void) G_GNUC_CONST;
gchar* transfer_ui_register_transient_transfer (TransferUi* self, const gchar* name, gint type, GError** error);
void transfer_ui_started (TransferUi* self, const gchar* id, gdouble progress, GError** error);
void transfer_ui_set_values (TransferUi* self, const gchar* id, GHashTable* key_values, GError** error);
void transfer_ui_done (TransferUi* self, const gchar* id, GError** error);
void transfer_ui_cancelled (TransferUi* self, const gchar* id, GError** error);
TransferUi* transfer_ui_create (void);
#define TRANSFER_UI_TRANSFER_TYPE_UPLOAD 0


G_END_DECLS

#endif
