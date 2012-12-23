[DBus (name="com.meego.transferui")]
public interface TransferUi : Object {
    public static const int TRANSFER_TYPE_UPLOAD = 0;

    [DBus (name="registerTransientTransfer")]
    public abstract string register_transient_transfer (string name, int type) throws IOError, DBusError;
    [DBus (name="started")]
    public abstract void started (string id, double progress) throws IOError, DBusError;
    [DBus (name="setValues")]
    public abstract void set_values (string id, HashTable<string, Variant> key_values) throws IOError, DBusError;
    [DBus (name="done")]
    public abstract void done (string id) throws IOError, DBusError;
    [DBus (name="cancelled")]
    public abstract void cancelled (string id) throws IOError, DBusError;

    [DBus (name="cancel")]
    public signal void cancel (string id);

    public static TransferUi? create () {
        TransferUi ui;
        try {
            ui = Bus.get_proxy_sync (BusType.SESSION,
                                     "com.meego.transferui",
                                     "/com/meego/transferui");

            return ui;
        } catch (IOError e) {
            return null;
        }
    }
}
