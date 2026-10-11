#include "daScript/misc/platform.h"

#include "dasStdDlgTray.h"

#include <dbus/dbus.h>
#include <dlfcn.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>

namespace das {

#define DAS_DBUS_SYMBOLS(X) \
    X(dbus_error_init) \
    X(dbus_error_free) \
    X(dbus_bus_get_private) \
    X(dbus_bus_request_name) \
    X(dbus_bus_add_match) \
    X(dbus_connection_set_exit_on_disconnect) \
    X(dbus_connection_add_filter) \
    X(dbus_connection_remove_filter) \
    X(dbus_connection_register_object_path) \
    X(dbus_connection_unregister_object_path) \
    X(dbus_connection_read_write_dispatch) \
    X(dbus_connection_dispatch) \
    X(dbus_connection_send) \
    X(dbus_connection_send_with_reply_and_block) \
    X(dbus_connection_flush) \
    X(dbus_connection_close) \
    X(dbus_connection_unref) \
    X(dbus_message_new_method_call) \
    X(dbus_message_new_method_return) \
    X(dbus_message_new_error) \
    X(dbus_message_new_signal) \
    X(dbus_message_unref) \
    X(dbus_message_set_no_reply) \
    X(dbus_message_is_method_call) \
    X(dbus_message_is_signal) \
    X(dbus_message_get_member) \
    X(dbus_message_get_interface) \
    X(dbus_message_get_type) \
    X(dbus_message_get_sender) \
    X(dbus_message_iter_init) \
    X(dbus_message_iter_get_arg_type) \
    X(dbus_message_iter_get_basic) \
    X(dbus_message_iter_next) \
    X(dbus_message_iter_recurse) \
    X(dbus_message_iter_init_append) \
    X(dbus_message_iter_append_basic) \
    X(dbus_message_iter_append_fixed_array) \
    X(dbus_message_iter_open_container) \
    X(dbus_message_iter_close_container)

    struct DBusApi {
#define DAS_DBUS_DECLARE(name) decltype(&::name) name = nullptr;
        DAS_DBUS_SYMBOLS(DAS_DBUS_DECLARE)
#undef DAS_DBUS_DECLARE
        void * lib = nullptr;
        bool load() {
            if ( lib ) return true;
            lib = dlopen("libdbus-1.so.3", RTLD_NOW | RTLD_LOCAL);
            if ( !lib ) return false;
#define DAS_DBUS_RESOLVE(sym) \
            sym = (decltype(&::sym)) dlsym(lib, #sym); \
            if ( !sym ) { dlclose(lib); *this = DBusApi(); return false; }
            DAS_DBUS_SYMBOLS(DAS_DBUS_RESOLVE)
#undef DAS_DBUS_RESOLVE
            return true;
        }
    };

    static DBusApi & dbusApi() {
        static DBusApi api;
        return api;
    }

    static const char * SNI_WATCHER = "org.kde.StatusNotifierWatcher";
    static const char * SNI_WATCHER_PATH = "/StatusNotifierWatcher";
    static const char * SNI_IFACE = "org.kde.StatusNotifierItem";
    static const char * SNI_PATH = "/StatusNotifierItem";
    static const char * MENU_IFACE = "com.canonical.dbusmenu";
    static const char * MENU_PATH = "/MenuBar";
    static const char * PROPS_IFACE = "org.freedesktop.DBus.Properties";
    static const char * INTROSPECT_IFACE = "org.freedesktop.DBus.Introspectable";
    static const char * NOTIFY_NAME = "org.freedesktop.Notifications";
    static const char * NOTIFY_PATH = "/org/freedesktop/Notifications";

    static const char * SNI_INTROSPECTION =
        "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" "
        "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
        "<node>\n"
        " <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
        "  <method name=\"Introspect\"><arg name=\"data\" type=\"s\" direction=\"out\"/></method>\n"
        " </interface>\n"
        " <interface name=\"org.freedesktop.DBus.Properties\">\n"
        "  <method name=\"Get\"><arg type=\"s\" direction=\"in\"/><arg type=\"s\" direction=\"in\"/><arg type=\"v\" direction=\"out\"/></method>\n"
        "  <method name=\"GetAll\"><arg type=\"s\" direction=\"in\"/><arg type=\"a{sv}\" direction=\"out\"/></method>\n"
        " </interface>\n"
        " <interface name=\"org.kde.StatusNotifierItem\">\n"
        "  <property name=\"Category\" type=\"s\" access=\"read\"/>\n"
        "  <property name=\"Id\" type=\"s\" access=\"read\"/>\n"
        "  <property name=\"Title\" type=\"s\" access=\"read\"/>\n"
        "  <property name=\"Status\" type=\"s\" access=\"read\"/>\n"
        "  <property name=\"WindowId\" type=\"i\" access=\"read\"/>\n"
        "  <property name=\"IconName\" type=\"s\" access=\"read\"/>\n"
        "  <property name=\"IconPixmap\" type=\"a(iiay)\" access=\"read\"/>\n"
        "  <property name=\"OverlayIconName\" type=\"s\" access=\"read\"/>\n"
        "  <property name=\"OverlayIconPixmap\" type=\"a(iiay)\" access=\"read\"/>\n"
        "  <property name=\"AttentionIconName\" type=\"s\" access=\"read\"/>\n"
        "  <property name=\"AttentionIconPixmap\" type=\"a(iiay)\" access=\"read\"/>\n"
        "  <property name=\"AttentionMovieName\" type=\"s\" access=\"read\"/>\n"
        "  <property name=\"ToolTip\" type=\"(sa(iiay)ss)\" access=\"read\"/>\n"
        "  <property name=\"ItemIsMenu\" type=\"b\" access=\"read\"/>\n"
        "  <property name=\"Menu\" type=\"o\" access=\"read\"/>\n"
        "  <property name=\"IconThemePath\" type=\"s\" access=\"read\"/>\n"
        "  <method name=\"Activate\"><arg name=\"x\" type=\"i\" direction=\"in\"/><arg name=\"y\" type=\"i\" direction=\"in\"/></method>\n"
        "  <method name=\"SecondaryActivate\"><arg name=\"x\" type=\"i\" direction=\"in\"/><arg name=\"y\" type=\"i\" direction=\"in\"/></method>\n"
        "  <method name=\"ContextMenu\"><arg name=\"x\" type=\"i\" direction=\"in\"/><arg name=\"y\" type=\"i\" direction=\"in\"/></method>\n"
        "  <method name=\"Scroll\"><arg name=\"delta\" type=\"i\" direction=\"in\"/><arg name=\"orientation\" type=\"s\" direction=\"in\"/></method>\n"
        "  <signal name=\"NewTitle\"/>\n"
        "  <signal name=\"NewIcon\"/>\n"
        "  <signal name=\"NewToolTip\"/>\n"
        "  <signal name=\"NewStatus\"><arg name=\"status\" type=\"s\"/></signal>\n"
        " </interface>\n"
        "</node>\n";

    static const char * MENU_INTROSPECTION =
        "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" "
        "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
        "<node>\n"
        " <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
        "  <method name=\"Introspect\"><arg name=\"data\" type=\"s\" direction=\"out\"/></method>\n"
        " </interface>\n"
        " <interface name=\"org.freedesktop.DBus.Properties\">\n"
        "  <method name=\"Get\"><arg type=\"s\" direction=\"in\"/><arg type=\"s\" direction=\"in\"/><arg type=\"v\" direction=\"out\"/></method>\n"
        "  <method name=\"GetAll\"><arg type=\"s\" direction=\"in\"/><arg type=\"a{sv}\" direction=\"out\"/></method>\n"
        " </interface>\n"
        " <interface name=\"com.canonical.dbusmenu\">\n"
        "  <property name=\"Version\" type=\"u\" access=\"read\"/>\n"
        "  <property name=\"TextDirection\" type=\"s\" access=\"read\"/>\n"
        "  <property name=\"Status\" type=\"s\" access=\"read\"/>\n"
        "  <property name=\"IconThemePath\" type=\"as\" access=\"read\"/>\n"
        "  <method name=\"GetLayout\">\n"
        "   <arg name=\"parentId\" type=\"i\" direction=\"in\"/><arg name=\"recursionDepth\" type=\"i\" direction=\"in\"/><arg name=\"propertyNames\" type=\"as\" direction=\"in\"/>\n"
        "   <arg name=\"revision\" type=\"u\" direction=\"out\"/><arg name=\"layout\" type=\"(ia{sv}av)\" direction=\"out\"/>\n"
        "  </method>\n"
        "  <method name=\"GetGroupProperties\">\n"
        "   <arg name=\"ids\" type=\"ai\" direction=\"in\"/><arg name=\"propertyNames\" type=\"as\" direction=\"in\"/>\n"
        "   <arg name=\"properties\" type=\"a(ia{sv})\" direction=\"out\"/>\n"
        "  </method>\n"
        "  <method name=\"GetProperty\"><arg name=\"id\" type=\"i\" direction=\"in\"/><arg name=\"name\" type=\"s\" direction=\"in\"/><arg name=\"value\" type=\"v\" direction=\"out\"/></method>\n"
        "  <method name=\"Event\"><arg name=\"id\" type=\"i\" direction=\"in\"/><arg name=\"eventId\" type=\"s\" direction=\"in\"/><arg name=\"data\" type=\"v\" direction=\"in\"/><arg name=\"timestamp\" type=\"u\" direction=\"in\"/></method>\n"
        "  <method name=\"EventGroup\"><arg name=\"events\" type=\"a(isvu)\" direction=\"in\"/><arg name=\"idErrors\" type=\"ai\" direction=\"out\"/></method>\n"
        "  <method name=\"AboutToShow\"><arg name=\"id\" type=\"i\" direction=\"in\"/><arg name=\"needUpdate\" type=\"b\" direction=\"out\"/></method>\n"
        "  <method name=\"AboutToShowGroup\"><arg name=\"ids\" type=\"ai\" direction=\"in\"/><arg name=\"updatesNeeded\" type=\"ai\" direction=\"out\"/><arg name=\"idErrors\" type=\"ai\" direction=\"out\"/></method>\n"
        "  <signal name=\"ItemsPropertiesUpdated\"><arg type=\"a(ia{sv})\"/><arg type=\"a(ias)\"/></signal>\n"
        "  <signal name=\"LayoutUpdated\"><arg name=\"revision\" type=\"u\"/><arg name=\"parent\" type=\"i\"/></signal>\n"
        " </interface>\n"
        "</node>\n";

    static string executableBaseName() {
        char buf[4096];
        ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if ( n <= 0 ) return "daslang";
        buf[n] = 0;
        const char * slash = strrchr(buf, '/');
        return slash ? slash + 1 : buf;
    }

    struct MessageWriter {
        DBusApi & api;
        DBusMessageIter * iter;
        MessageWriter(DBusApi & a, DBusMessageIter * it) : api(a), iter(it) {}
        void string_(const char * value) {
            api.dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &value);
        }
        void objectPath(const char * value) {
            api.dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &value);
        }
        void int32(int32_t value) {
            dbus_int32_t v = value;
            api.dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &v);
        }
        void uint32(uint32_t value) {
            dbus_uint32_t v = value;
            api.dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT32, &v);
        }
        void boolean(bool value) {
            dbus_bool_t v = value ? TRUE : FALSE;
            api.dbus_message_iter_append_basic(iter, DBUS_TYPE_BOOLEAN, &v);
        }
        void bytes(const uint8_t * data, int32_t count) {
            DBusMessageIter sub;
            api.dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, "y", &sub);
            const uint8_t * ptr = data;
            api.dbus_message_iter_append_fixed_array(&sub, DBUS_TYPE_BYTE, &ptr, count);
            api.dbus_message_iter_close_container(iter, &sub);
        }
        template <typename Fn>
        void container(int type, const char * signature, Fn && body) {
            DBusMessageIter sub;
            api.dbus_message_iter_open_container(iter, type, signature, &sub);
            MessageWriter inner(api, &sub);
            body(inner);
            api.dbus_message_iter_close_container(iter, &sub);
        }
        template <typename Fn>
        void variant(const char * signature, Fn && body) {
            container(DBUS_TYPE_VARIANT, signature, body);
        }
        template <typename Fn>
        void dictEntry(const char * key, const char * signature, Fn && body) {
            container(DBUS_TYPE_DICT_ENTRY, nullptr, [&](MessageWriter & w) {
                w.string_(key);
                w.variant(signature, body);
            });
        }
    };

    class LinuxTray;

    struct PropertyWriter {
        const char * name;
        void (*write)(LinuxTray &, MessageWriter &);
    };

    class LinuxTray final : public TrayBackend {
    public:
        DBusApi & api;
        DBusConnection * conn = nullptr;
        bool connected = false;
        string busName;
        string appId;
        string tooltip;
        int32_t iconWidth = 0;
        int32_t iconHeight = 0;
        vector<uint8_t> iconArgb;
        vector<uint8_t> iconRgba;
        vector<TrayMenuEntry> menu;
        uint32_t menuRevision = 1;
        vector<TrayEvent> events;
        bool sniRegistered = false;
        bool menuRegistered = false;
        bool filterAdded = false;

        LinuxTray() : api(dbusApi()) {}

        virtual ~LinuxTray() override {
            if ( !conn ) return;
            if ( filterAdded ) api.dbus_connection_remove_filter(conn, &LinuxTray::onBusSignal, this);
            if ( sniRegistered ) api.dbus_connection_unregister_object_path(conn, SNI_PATH);
            if ( menuRegistered ) api.dbus_connection_unregister_object_path(conn, MENU_PATH);
            api.dbus_connection_flush(conn);
            api.dbus_connection_close(conn);
            api.dbus_connection_unref(conn);
        }

        virtual bool create(const char * tip) override {
            tooltip = tip;
            appId = executableBaseName();
            DBusError err;
            api.dbus_error_init(&err);
            conn = api.dbus_bus_get_private(DBUS_BUS_SESSION, &err);
            if ( !conn ) {
                api.dbus_error_free(&err);
                return false;
            }
            connected = true;
            api.dbus_connection_set_exit_on_disconnect(conn, FALSE);
            static int32_t instanceCounter = 0;
            ++instanceCounter;
            char nameBuf[128];
            snprintf(nameBuf, sizeof(nameBuf), "org.kde.StatusNotifierItem-%d-%d", int(getpid()), int(instanceCounter));
            busName = nameBuf;
            int rn = api.dbus_bus_request_name(conn, busName.c_str(), DBUS_NAME_FLAG_DO_NOT_QUEUE, &err);
            if ( rn != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER ) {
                api.dbus_error_free(&err);
                return false;
            }
            DBusObjectPathVTable sniVTable = {};
            sniVTable.message_function = &LinuxTray::onSniMessage;
            sniRegistered = api.dbus_connection_register_object_path(conn, SNI_PATH, &sniVTable, this);
            DBusObjectPathVTable menuVTable = {};
            menuVTable.message_function = &LinuxTray::onMenuMessage;
            menuRegistered = api.dbus_connection_register_object_path(conn, MENU_PATH, &menuVTable, this);
            if ( !sniRegistered || !menuRegistered ) return false;
            filterAdded = api.dbus_connection_add_filter(conn, &LinuxTray::onBusSignal, this, nullptr);
            string rule = string("type='signal',sender='org.freedesktop.DBus',interface='org.freedesktop.DBus',member='NameOwnerChanged',arg0='") + SNI_WATCHER + "'";
            api.dbus_bus_add_match(conn, rule.c_str(), nullptr);
            registerWithWatcher();
            api.dbus_connection_flush(conn);
            return true;
        }

        //! A no-reply call: with no watcher on the bus the daemon answers an error nobody waits for,
        //! and the NameOwnerChanged filter repeats the call when a watcher appears.
        void registerWithWatcher() {
            DBusMessage * msg = api.dbus_message_new_method_call(SNI_WATCHER, SNI_WATCHER_PATH, SNI_WATCHER, "RegisterStatusNotifierItem");
            if ( !msg ) return;
            DBusMessageIter iter;
            api.dbus_message_iter_init_append(msg, &iter);
            MessageWriter(api, &iter).string_(busName.c_str());
            api.dbus_message_set_no_reply(msg, TRUE);
            api.dbus_connection_send(conn, msg, nullptr);
            api.dbus_message_unref(msg);
        }

        void emitSignal(const char * path, const char * iface, const char * name, uint32_t revision = 0, bool withLayoutArgs = false) {
            if ( !connected ) return;
            DBusMessage * sig = api.dbus_message_new_signal(path, iface, name);
            if ( !sig ) return;
            if ( withLayoutArgs ) {
                DBusMessageIter iter;
                api.dbus_message_iter_init_append(sig, &iter);
                MessageWriter w(api, &iter);
                w.uint32(revision);
                w.int32(0);
            }
            api.dbus_connection_send(conn, sig, nullptr);
            api.dbus_message_unref(sig);
            api.dbus_connection_flush(conn);
        }

        virtual void setIcon(const uint8_t * rgba8, int32_t width, int32_t height) override {
            iconWidth = width;
            iconHeight = height;
            size_t count = size_t(width) * size_t(height);
            iconRgba.assign(rgba8, rgba8 + count * 4);
            iconArgb.resize(count * 4);
            for ( size_t i = 0; i != count; ++i ) {
                iconArgb[i * 4 + 0] = rgba8[i * 4 + 3];
                iconArgb[i * 4 + 1] = rgba8[i * 4 + 0];
                iconArgb[i * 4 + 2] = rgba8[i * 4 + 1];
                iconArgb[i * 4 + 3] = rgba8[i * 4 + 2];
            }
            emitSignal(SNI_PATH, SNI_IFACE, "NewIcon");
        }

        virtual void setTooltip(const char * text) override {
            tooltip = text;
            emitSignal(SNI_PATH, SNI_IFACE, "NewToolTip");
            emitSignal(SNI_PATH, SNI_IFACE, "NewTitle");
        }

        virtual void setMenu(const vector<TrayMenuEntry> & entries) override {
            menu = entries;
            menuRevision++;
            emitSignal(MENU_PATH, MENU_IFACE, "LayoutUpdated", menuRevision, true);
        }

        virtual void poll(vector<TrayEvent> & out) override {
            if ( connected ) {
                if ( !api.dbus_connection_read_write_dispatch(conn, 0) ) connected = false;
                while ( api.dbus_connection_dispatch(conn) == DBUS_DISPATCH_DATA_REMAINS ) {}
            }
            out.swap(events);
            events.clear();
        }

        virtual bool notify(const char * title, const char * body) override {
            if ( !connected ) return false;
            DBusMessage * msg = api.dbus_message_new_method_call(NOTIFY_NAME, NOTIFY_PATH, NOTIFY_NAME, "Notify");
            if ( !msg ) return false;
            DBusMessageIter iter;
            api.dbus_message_iter_init_append(msg, &iter);
            MessageWriter w(api, &iter);
            w.string_(appId.c_str());
            w.uint32(0);
            w.string_("");
            w.string_(title);
            w.string_(body);
            w.container(DBUS_TYPE_ARRAY, "s", [](MessageWriter &) {});
            w.container(DBUS_TYPE_ARRAY, "{sv}", [&](MessageWriter & hints) {
                if ( iconWidth > 0 ) {
                    hints.dictEntry("image-data", "(iiibiiay)", [&](MessageWriter & v) {
                        v.container(DBUS_TYPE_STRUCT, nullptr, [&](MessageWriter & s) {
                            s.int32(iconWidth);
                            s.int32(iconHeight);
                            s.int32(iconWidth * 4);
                            s.boolean(true);
                            s.int32(8);
                            s.int32(4);
                            s.bytes(iconRgba.data(), int32_t(iconRgba.size()));
                        });
                    });
                }
            });
            w.int32(-1);
            DBusError err;
            api.dbus_error_init(&err);
            DBusMessage * reply = api.dbus_connection_send_with_reply_and_block(conn, msg, 2000, &err);
            api.dbus_message_unref(msg);
            api.dbus_error_free(&err);
            if ( !reply ) return false;
            api.dbus_message_unref(reply);
            return true;
        }

        void pushEvent(TrayEventKind kind, int32_t id, int32_t x, int32_t y) {
            events.push_back(TrayEvent{kind, id, x, y});
        }

        bool readInt32(DBusMessageIter & iter, int32_t & value) {
            if ( api.dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_INT32 ) return false;
            dbus_int32_t v = 0;
            api.dbus_message_iter_get_basic(&iter, &v);
            value = v;
            return true;
        }

        bool readString(DBusMessageIter & iter, const char * & value) {
            if ( api.dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING ) return false;
            api.dbus_message_iter_get_basic(&iter, &value);
            return true;
        }

        bool readInt32Pair(DBusMessage * msg, int32_t & a, int32_t & b) {
            DBusMessageIter iter;
            int32_t va = 0, vb = 0;
            if ( !api.dbus_message_iter_init(msg, &iter) || !readInt32(iter, va) ) return false;
            if ( !api.dbus_message_iter_next(&iter) || !readInt32(iter, vb) ) return false;
            a = va;
            b = vb;
            return true;
        }

        bool readStringPair(DBusMessage * msg, const char * & a, const char * & b) {
            DBusMessageIter iter;
            if ( !api.dbus_message_iter_init(msg, &iter) || !readString(iter, a) ) return false;
            if ( !api.dbus_message_iter_next(&iter) || !readString(iter, b) ) return false;
            return true;
        }

        void reply(DBusMessage * msg, DBusMessage * response) {
            if ( !response ) return;
            api.dbus_connection_send(conn, response, nullptr);
            api.dbus_message_unref(response);
            api.dbus_connection_flush(conn);
        }

        void replyError(DBusMessage * msg, const char * name, const char * text) {
            reply(msg, api.dbus_message_new_error(msg, name, text));
        }

        void replyEmpty(DBusMessage * msg) {
            reply(msg, api.dbus_message_new_method_return(msg));
        }

        template <typename Fn>
        void replyWith(DBusMessage * msg, Fn && body) {
            DBusMessage * response = api.dbus_message_new_method_return(msg);
            if ( !response ) return;
            DBusMessageIter iter;
            api.dbus_message_iter_init_append(response, &iter);
            MessageWriter w(api, &iter);
            body(w);
            reply(msg, response);
        }

        void writePixmaps(MessageWriter & w) {
            w.container(DBUS_TYPE_ARRAY, "(iiay)", [&](MessageWriter & arr) {
                if ( iconWidth <= 0 ) return;
                arr.container(DBUS_TYPE_STRUCT, nullptr, [&](MessageWriter & s) {
                    s.int32(iconWidth);
                    s.int32(iconHeight);
                    s.bytes(iconArgb.data(), int32_t(iconArgb.size()));
                });
            });
        }

        static void writeEmptyPixmaps(MessageWriter & w) {
            w.container(DBUS_TYPE_ARRAY, "(iiay)", [](MessageWriter &) {});
        }

        void writeToolTip(MessageWriter & w) {
            w.container(DBUS_TYPE_STRUCT, nullptr, [&](MessageWriter & s) {
                s.string_("");
                writeEmptyPixmaps(s);
                s.string_(tooltip.c_str());
                s.string_("");
            });
        }

        static const PropertyWriter * sniProperties(size_t & count) {
            static const PropertyWriter props[] = {
                {"Category", [](LinuxTray &, MessageWriter & w) { w.variant("s", [](MessageWriter & v) { v.string_("ApplicationStatus"); }); }},
                {"Id", [](LinuxTray & t, MessageWriter & w) { w.variant("s", [&](MessageWriter & v) { v.string_(t.appId.c_str()); }); }},
                {"Title", [](LinuxTray & t, MessageWriter & w) { w.variant("s", [&](MessageWriter & v) { v.string_(t.tooltip.c_str()); }); }},
                {"Status", [](LinuxTray &, MessageWriter & w) { w.variant("s", [](MessageWriter & v) { v.string_("Active"); }); }},
                {"WindowId", [](LinuxTray &, MessageWriter & w) { w.variant("i", [](MessageWriter & v) { v.int32(0); }); }},
                {"IconName", [](LinuxTray &, MessageWriter & w) { w.variant("s", [](MessageWriter & v) { v.string_(""); }); }},
                {"IconPixmap", [](LinuxTray & t, MessageWriter & w) { w.variant("a(iiay)", [&](MessageWriter & v) { t.writePixmaps(v); }); }},
                {"OverlayIconName", [](LinuxTray &, MessageWriter & w) { w.variant("s", [](MessageWriter & v) { v.string_(""); }); }},
                {"OverlayIconPixmap", [](LinuxTray &, MessageWriter & w) { w.variant("a(iiay)", [](MessageWriter & v) { writeEmptyPixmaps(v); }); }},
                {"AttentionIconName", [](LinuxTray &, MessageWriter & w) { w.variant("s", [](MessageWriter & v) { v.string_(""); }); }},
                {"AttentionIconPixmap", [](LinuxTray &, MessageWriter & w) { w.variant("a(iiay)", [](MessageWriter & v) { writeEmptyPixmaps(v); }); }},
                {"AttentionMovieName", [](LinuxTray &, MessageWriter & w) { w.variant("s", [](MessageWriter & v) { v.string_(""); }); }},
                {"ToolTip", [](LinuxTray & t, MessageWriter & w) { w.variant("(sa(iiay)ss)", [&](MessageWriter & v) { t.writeToolTip(v); }); }},
                {"ItemIsMenu", [](LinuxTray &, MessageWriter & w) { w.variant("b", [](MessageWriter & v) { v.boolean(false); }); }},
                {"Menu", [](LinuxTray &, MessageWriter & w) { w.variant("o", [](MessageWriter & v) { v.objectPath(MENU_PATH); }); }},
                {"IconThemePath", [](LinuxTray &, MessageWriter & w) { w.variant("s", [](MessageWriter & v) { v.string_(""); }); }},
            };
            count = sizeof(props) / sizeof(props[0]);
            return props;
        }

        static const PropertyWriter * menuProperties(size_t & count) {
            static const PropertyWriter props[] = {
                {"Version", [](LinuxTray &, MessageWriter & w) { w.variant("u", [](MessageWriter & v) { v.uint32(3); }); }},
                {"TextDirection", [](LinuxTray &, MessageWriter & w) { w.variant("s", [](MessageWriter & v) { v.string_("ltr"); }); }},
                {"Status", [](LinuxTray &, MessageWriter & w) { w.variant("s", [](MessageWriter & v) { v.string_("normal"); }); }},
                {"IconThemePath", [](LinuxTray &, MessageWriter & w) { w.variant("as", [](MessageWriter & v) { v.container(DBUS_TYPE_ARRAY, "s", [](MessageWriter &) {}); }); }},
            };
            count = sizeof(props) / sizeof(props[0]);
            return props;
        }

        DBusHandlerResult handleProperties(DBusMessage * msg, const char * expectedIface, const PropertyWriter * props, size_t count) {
            if ( api.dbus_message_is_method_call(msg, PROPS_IFACE, "Get") ) {
                const char * iface = nullptr; const char * prop = nullptr;
                if ( !readStringPair(msg, iface, prop) ) {
                    replyError(msg, DBUS_ERROR_INVALID_ARGS, "Get expects (ss)");
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
                if ( iface && *iface && strcmp(iface, expectedIface) != 0 ) {
                    replyError(msg, DBUS_ERROR_UNKNOWN_INTERFACE, iface);
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
                for ( size_t i = 0; i != count; ++i ) {
                    if ( strcmp(props[i].name, prop) != 0 ) continue;
                    replyWith(msg, [&](MessageWriter & w) { props[i].write(*this, w); });
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
                replyError(msg, DBUS_ERROR_UNKNOWN_PROPERTY, prop);
                return DBUS_HANDLER_RESULT_HANDLED;
            }
            if ( api.dbus_message_is_method_call(msg, PROPS_IFACE, "GetAll") ) {
                DBusMessageIter iter;
                const char * iface = nullptr;
                bool matches = true;
                if ( api.dbus_message_iter_init(msg, &iter) && readString(iter, iface) && iface && *iface ) {
                    matches = strcmp(iface, expectedIface) == 0;
                }
                replyWith(msg, [&](MessageWriter & w) {
                    w.container(DBUS_TYPE_ARRAY, "{sv}", [&](MessageWriter & dict) {
                        if ( !matches ) return;
                        for ( size_t i = 0; i != count; ++i ) {
                            dict.container(DBUS_TYPE_DICT_ENTRY, nullptr, [&](MessageWriter & entry) {
                                entry.string_(props[i].name);
                                props[i].write(*this, entry);
                            });
                        }
                    });
                });
                return DBUS_HANDLER_RESULT_HANDLED;
            }
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }

        DBusHandlerResult sniMessage(DBusMessage * msg) {
            if ( api.dbus_message_get_type(msg) != DBUS_MESSAGE_TYPE_METHOD_CALL ) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
            if ( api.dbus_message_is_method_call(msg, INTROSPECT_IFACE, "Introspect") ) {
                replyWith(msg, [](MessageWriter & w) { w.string_(SNI_INTROSPECTION); });
                return DBUS_HANDLER_RESULT_HANDLED;
            }
            size_t count = 0;
            const PropertyWriter * props = sniProperties(count);
            auto handled = handleProperties(msg, SNI_IFACE, props, count);
            if ( handled == DBUS_HANDLER_RESULT_HANDLED ) return handled;
            const char * member = api.dbus_message_get_member(msg);
            const char * iface = api.dbus_message_get_interface(msg);
            if ( !member || (iface && strcmp(iface, SNI_IFACE) != 0) ) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
            int32_t x = 0, y = 0;
            if ( strcmp(member, "Activate") == 0 ) {
                readInt32Pair(msg, x, y);
                pushEvent(TrayEventKind::click, 0, x, y);
            } else if ( strcmp(member, "ContextMenu") == 0 ) {
                readInt32Pair(msg, x, y);
                pushEvent(TrayEventKind::right_click, 0, x, y);
            } else if ( strcmp(member, "SecondaryActivate") != 0 && strcmp(member, "Scroll") != 0 ) {
                return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
            }
            replyEmpty(msg);
            return DBUS_HANDLER_RESULT_HANDLED;
        }

        //! dbusmenu ids: 0 is the root, an entry carries its own positive id, a separator carries
        //! minus its position minus one - so a click on a layout the host fetched before a rebuild
        //! still names the entry it showed, or nothing.
        int32_t layoutIdAt(size_t index) const {
            const TrayMenuEntry & e = menu[index];
            return e.separator ? -int32_t(index + 1) : e.id;
        }

        const TrayMenuEntry * entryByLayoutId(int32_t id) const {
            if ( id == 0 ) return nullptr;
            for ( size_t i = 0; i != menu.size(); ++i ) {
                if ( layoutIdAt(i) == id ) return &menu[i];
            }
            return nullptr;
        }

        bool validLayoutId(int32_t id) const {
            return id == 0 || entryByLayoutId(id) != nullptr;
        }

        void dispatchMenuEvent(DBusMessageIter & fields) {
            int32_t id = 0;
            const char * eventId = nullptr;
            if ( !readInt32(fields, id) ) return;
            if ( !api.dbus_message_iter_next(&fields) || !readString(fields, eventId) ) return;
            if ( !eventId || strcmp(eventId, "clicked") != 0 ) return;
            const TrayMenuEntry * e = entryByLayoutId(id);
            if ( e && !e->separator && e->enabled ) pushEvent(TrayEventKind::menu, e->id, 0, 0);
        }

        static void writeMenuItemProperties(MessageWriter & w, const TrayMenuEntry & e) {
            w.container(DBUS_TYPE_ARRAY, "{sv}", [&](MessageWriter & dict) {
                if ( e.separator ) {
                    dict.dictEntry("type", "s", [](MessageWriter & v) { v.string_("separator"); });
                    return;
                }
                dict.dictEntry("label", "s", [&](MessageWriter & v) { v.string_(e.label.c_str()); });
                dict.dictEntry("enabled", "b", [&](MessageWriter & v) { v.boolean(e.enabled); });
                if ( e.checked ) {
                    dict.dictEntry("toggle-type", "s", [](MessageWriter & v) { v.string_("checkmark"); });
                    dict.dictEntry("toggle-state", "i", [](MessageWriter & v) { v.int32(1); });
                }
            });
        }

        static void writeRootProperties(MessageWriter & w) {
            w.container(DBUS_TYPE_ARRAY, "{sv}", [](MessageWriter & dict) {
                dict.dictEntry("children-display", "s", [](MessageWriter & v) { v.string_("submenu"); });
            });
        }

        void writeNodeProperties(MessageWriter & w, int32_t layoutId) const {
            if ( layoutId == 0 ) writeRootProperties(w);
            else writeMenuItemProperties(w, *entryByLayoutId(layoutId));
        }

        void writeLayoutNode(MessageWriter & w, int32_t layoutId, bool withChildren) {
            w.container(DBUS_TYPE_STRUCT, nullptr, [&](MessageWriter & s) {
                s.int32(layoutId);
                writeNodeProperties(s, layoutId);
                s.container(DBUS_TYPE_ARRAY, "v", [&](MessageWriter & children) {
                    if ( layoutId != 0 || !withChildren ) return;
                    for ( size_t i = 0; i != menu.size(); ++i ) {
                        children.variant("(ia{sv}av)", [&](MessageWriter & v) {
                            writeLayoutNode(v, layoutIdAt(i), false);
                        });
                    }
                });
            });
        }

        DBusHandlerResult menuMessage(DBusMessage * msg) {
            if ( api.dbus_message_get_type(msg) != DBUS_MESSAGE_TYPE_METHOD_CALL ) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
            if ( api.dbus_message_is_method_call(msg, INTROSPECT_IFACE, "Introspect") ) {
                replyWith(msg, [](MessageWriter & w) { w.string_(MENU_INTROSPECTION); });
                return DBUS_HANDLER_RESULT_HANDLED;
            }
            size_t count = 0;
            const PropertyWriter * props = menuProperties(count);
            auto handled = handleProperties(msg, MENU_IFACE, props, count);
            if ( handled == DBUS_HANDLER_RESULT_HANDLED ) return handled;
            if ( api.dbus_message_is_method_call(msg, MENU_IFACE, "GetLayout") ) {
                DBusMessageIter iter;
                int32_t parentId = 0;
                int32_t depth = -1;
                if ( api.dbus_message_iter_init(msg, &iter) && readInt32(iter, parentId) ) {
                    if ( api.dbus_message_iter_next(&iter) ) readInt32(iter, depth);
                }
                if ( !validLayoutId(parentId) ) {
                    replyError(msg, DBUS_ERROR_INVALID_ARGS, "GetLayout: unknown parent id");
                    return DBUS_HANDLER_RESULT_HANDLED;
                }
                replyWith(msg, [&](MessageWriter & w) {
                    w.uint32(menuRevision);
                    writeLayoutNode(w, parentId, depth != 0);
                });
                return DBUS_HANDLER_RESULT_HANDLED;
            }
            if ( api.dbus_message_is_method_call(msg, MENU_IFACE, "GetGroupProperties") ) {
                vector<int32_t> ids;
                DBusMessageIter iter;
                if ( api.dbus_message_iter_init(msg, &iter) && api.dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY ) {
                    DBusMessageIter sub;
                    api.dbus_message_iter_recurse(&iter, &sub);
                    int32_t id = 0;
                    while ( readInt32(sub, id) ) {
                        ids.push_back(id);
                        api.dbus_message_iter_next(&sub);
                    }
                }
                if ( ids.empty() ) {
                    ids.push_back(0);
                    for ( size_t i = 0; i != menu.size(); ++i ) ids.push_back(layoutIdAt(i));
                }
                replyWith(msg, [&](MessageWriter & w) {
                    w.container(DBUS_TYPE_ARRAY, "(ia{sv})", [&](MessageWriter & arr) {
                        for ( int32_t id : ids ) {
                            if ( !validLayoutId(id) ) continue;
                            arr.container(DBUS_TYPE_STRUCT, nullptr, [&](MessageWriter & s) {
                                s.int32(id);
                                writeNodeProperties(s, id);
                            });
                        }
                    });
                });
                return DBUS_HANDLER_RESULT_HANDLED;
            }
            if ( api.dbus_message_is_method_call(msg, MENU_IFACE, "GetProperty") ) {
                replyError(msg, DBUS_ERROR_UNKNOWN_PROPERTY, "GetProperty: use GetGroupProperties");
                return DBUS_HANDLER_RESULT_HANDLED;
            }
            if ( api.dbus_message_is_method_call(msg, MENU_IFACE, "Event") ) {
                DBusMessageIter iter;
                if ( api.dbus_message_iter_init(msg, &iter) ) dispatchMenuEvent(iter);
                replyEmpty(msg);
                return DBUS_HANDLER_RESULT_HANDLED;
            }
            if ( api.dbus_message_is_method_call(msg, MENU_IFACE, "EventGroup") ) {
                DBusMessageIter iter;
                if ( api.dbus_message_iter_init(msg, &iter) && api.dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY ) {
                    DBusMessageIter events;
                    api.dbus_message_iter_recurse(&iter, &events);
                    while ( api.dbus_message_iter_get_arg_type(&events) == DBUS_TYPE_STRUCT ) {
                        DBusMessageIter fields;
                        api.dbus_message_iter_recurse(&events, &fields);
                        dispatchMenuEvent(fields);
                        api.dbus_message_iter_next(&events);
                    }
                }
                replyWith(msg, [](MessageWriter & w) { w.container(DBUS_TYPE_ARRAY, "i", [](MessageWriter &) {}); });
                return DBUS_HANDLER_RESULT_HANDLED;
            }
            if ( api.dbus_message_is_method_call(msg, MENU_IFACE, "AboutToShow") ) {
                replyWith(msg, [](MessageWriter & w) { w.boolean(false); });
                return DBUS_HANDLER_RESULT_HANDLED;
            }
            if ( api.dbus_message_is_method_call(msg, MENU_IFACE, "AboutToShowGroup") ) {
                replyWith(msg, [](MessageWriter & w) {
                    w.container(DBUS_TYPE_ARRAY, "i", [](MessageWriter &) {});
                    w.container(DBUS_TYPE_ARRAY, "i", [](MessageWriter &) {});
                });
                return DBUS_HANDLER_RESULT_HANDLED;
            }
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }

        static DBusHandlerResult onSniMessage(DBusConnection *, DBusMessage * msg, void * self) {
            return ((LinuxTray *) self)->sniMessage(msg);
        }

        static DBusHandlerResult onMenuMessage(DBusConnection *, DBusMessage * msg, void * self) {
            return ((LinuxTray *) self)->menuMessage(msg);
        }

        static DBusHandlerResult onBusSignal(DBusConnection *, DBusMessage * msg, void * self) {
            LinuxTray * tray = (LinuxTray *) self;
            DBusApi & api = tray->api;
            if ( !api.dbus_message_is_signal(msg, "org.freedesktop.DBus", "NameOwnerChanged") ) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
            const char * sender = api.dbus_message_get_sender(msg);
            if ( !sender || strcmp(sender, "org.freedesktop.DBus") != 0 ) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
            DBusMessageIter iter;
            const char * name = nullptr; const char * oldOwner = nullptr; const char * newOwner = nullptr;
            if ( !api.dbus_message_iter_init(msg, &iter) || !tray->readString(iter, name) ) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
            if ( !api.dbus_message_iter_next(&iter) || !tray->readString(iter, oldOwner) ) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
            if ( !api.dbus_message_iter_next(&iter) || !tray->readString(iter, newOwner) ) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
            if ( strcmp(name, SNI_WATCHER) == 0 && newOwner && *newOwner ) tray->registerWithWatcher();
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }
    };

    //! The session bus address is the only discovery path on purpose: libdbus's autolaunch would
    //! spawn a bus (and an X connection) on a headless box that has no tray to offer.
    bool TrayPlatformAvailable() {
        const char * address = getenv("DBUS_SESSION_BUS_ADDRESS");
        if ( !address || !*address ) return false;
        return dbusApi().load();
    }

    TrayBackend * TrayPlatformCreate() {
        if ( !TrayPlatformAvailable() ) return nullptr;
        return new LinuxTray();
    }
}
