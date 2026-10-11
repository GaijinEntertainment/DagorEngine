#include "daScript/misc/platform.h"

#include "dasStdDlgTray.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_interop.h"

namespace das {

    static TrayBackend * g_tray = nullptr;
    static string g_tooltip;
    static vector<uint8_t> g_iconRgba;
    static int32_t g_iconWidth = 0;
    static int32_t g_iconHeight = 0;
    static vector<TrayMenuEntry> g_pendingMenu;
    static vector<TrayMenuEntry> g_committedMenu;
    static bool g_menuCommitted = false;

    bool TrayAvailable() {
        return TrayPlatformAvailable();
    }

    bool TrayCreate(const char * tooltip) {
        g_tooltip = tooltip ? tooltip : "";
        if ( g_tray ) {
            g_tray->setTooltip(g_tooltip.c_str());
            return true;
        }
        TrayBackend * backend = TrayPlatformCreate();
        if ( !backend ) return false;
        if ( !backend->create(g_tooltip.c_str()) ) {
            delete backend;
            return false;
        }
        g_tray = backend;
        if ( g_iconWidth > 0 ) g_tray->setIcon(g_iconRgba.data(), g_iconWidth, g_iconHeight);
        if ( g_menuCommitted ) g_tray->setMenu(g_committedMenu);
        return true;
    }

    void TraySetIcon(const TArray<uint8_t> & rgba8, int32_t width, int32_t height, Context * context, LineInfoArg * at) {
        if ( width <= 0 || height <= 0 || width > TRAY_ICON_MAX_SIDE || height > TRAY_ICON_MAX_SIDE ) {
            context->throw_error_at(at, "tray_set_icon: icon size %dx%d (each side must be 1..%d)", width, height, TRAY_ICON_MAX_SIDE);
        }
        int64_t need = int64_t(width) * int64_t(height) * 4;
        if ( int64_t(rgba8.size) != need ) {
            context->throw_error_at(at, "tray_set_icon: %d bytes for a %dx%d RGBA8 icon (need %lld)",
                int(rgba8.size), width, height, (long long) need);
        }
        g_iconWidth = width;
        g_iconHeight = height;
        g_iconRgba.assign((const uint8_t *) rgba8.data, (const uint8_t *) rgba8.data + need);
        if ( g_tray ) g_tray->setIcon(g_iconRgba.data(), width, height);
    }

    void TraySetTooltip(const char * text) {
        g_tooltip = text ? text : "";
        if ( g_tray ) g_tray->setTooltip(g_tooltip.c_str());
    }

    void TrayMenuClear() {
        g_pendingMenu.clear();
    }

    void TrayMenuAdd(int32_t id, const char * label, bool enabled, bool checked, Context * context, LineInfoArg * at) {
        if ( id < 1 ) {
            context->throw_error_at(at, "tray_menu_add: menu entry id %d (ids are positive)", id);
        }
        for ( const auto & e : g_pendingMenu ) {
            if ( !e.separator && e.id == id ) {
                context->throw_error_at(at, "tray_menu_add: menu entry id %d is already in this menu", id);
            }
        }
        g_pendingMenu.push_back(TrayMenuEntry{id, label ? label : "", enabled, checked, false});
    }

    void TrayMenuAddSeparator() {
        g_pendingMenu.push_back(TrayMenuEntry{0, "", false, false, true});
    }

    void TrayMenuCommit() {
        g_committedMenu = g_pendingMenu;
        g_menuCommitted = true;
        if ( g_tray ) g_tray->setMenu(g_committedMenu);
    }

    void TrayPoll(const TBlock<void, const TrayEvent &> & blk, Context * context, LineInfoArg * at) {
        if ( !g_tray ) return;
        vector<TrayEvent> events;
        g_tray->poll(events);
        for ( auto & ev : events ) {
            das_invoke<void>::invoke<const TrayEvent &>(context, at, blk, ev);
        }
    }

    bool TrayNotify(const char * title, const char * body) {
        if ( !g_tray ) return false;
        return g_tray->notify(title ? title : "", body ? body : "");
    }

    void TrayDestroy() {
        delete g_tray;
        g_tray = nullptr;
        g_pendingMenu.clear();
        g_committedMenu.clear();
        g_menuCommitted = false;
        g_iconRgba.clear();
        g_iconWidth = 0;
        g_iconHeight = 0;
        g_tooltip.clear();
    }
}
