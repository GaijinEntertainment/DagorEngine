#include "daScript/misc/platform.h"

#include "dasStdDlgTray.h"

#include <windows.h>
#include <shellapi.h>
#include <windowsx.h>

namespace das {

    static const UINT TRAY_CALLBACK_MESSAGE = WM_APP + 1;
    static const UINT TRAY_ICON_ID = 1;
    static const wchar_t * TRAY_WINDOW_CLASS = L"DasStdDlgTrayWindow";

    static wstring utf8ToWide(const char * text) {
        if ( !text || !*text ) return L"";
        int count = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
        if ( count <= 0 ) return L"";
        wstring result(size_t(count), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, text, -1, &result[0], count);
        result.resize(size_t(count - 1));
        return result;
    }

    static void copyTip(wchar_t * dst, size_t capacity, const char * text) {
        wstring wide = utf8ToWide(text);
        size_t n = wide.size() < capacity - 1 ? wide.size() : capacity - 1;
        memcpy(dst, wide.c_str(), n * sizeof(wchar_t));
        dst[n] = 0;
    }

    class WinTray final : public TrayBackend {
    public:
        HWND hwnd = nullptr;
        HICON icon = nullptr;
        HMENU menu = nullptr;
        UINT taskbarCreated = 0;
        string tooltip;
        vector<TrayMenuEntry> entries;
        vector<TrayEvent> events;
        bool iconAdded = false;
        bool callbacksV4 = false;
        bool menuOpen = false;

        virtual ~WinTray() override {
            if ( iconAdded ) {
                NOTIFYICONDATAW nid = iconData();
                Shell_NotifyIconW(NIM_DELETE, &nid);
            }
            if ( menu ) DestroyMenu(menu);
            if ( icon ) DestroyIcon(icon);
            if ( hwnd ) {
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
                HWND dying = hwnd;
                hwnd = nullptr;
                DestroyWindow(dying);
            }
            UnregisterClassW(TRAY_WINDOW_CLASS, GetModuleHandleW(nullptr));
        }

        NOTIFYICONDATAW iconData() {
            NOTIFYICONDATAW nid = {};
            nid.cbSize = sizeof(nid);
            nid.hWnd = hwnd;
            nid.uID = TRAY_ICON_ID;
            return nid;
        }

        virtual bool create(const char * tip) override {
            tooltip = tip;
            WNDCLASSW wc = {};
            wc.lpfnWndProc = &WinTray::windowProc;
            wc.hInstance = GetModuleHandleW(nullptr);
            wc.lpszClassName = TRAY_WINDOW_CLASS;
            RegisterClassW(&wc);
            hwnd = CreateWindowExW(0, TRAY_WINDOW_CLASS, L"", WS_OVERLAPPED, 0, 0, 0, 0, nullptr, nullptr, wc.hInstance, this);
            if ( !hwnd ) return false;
            taskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
            ChangeWindowMessageFilterEx(hwnd, taskbarCreated, MSGFLT_ALLOW, nullptr);
            menu = CreatePopupMenu();
            return addIcon();
        }

        bool addIcon() {
            NOTIFYICONDATAW nid = iconData();
            nid.uFlags = NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP | (icon ? NIF_ICON : 0);
            nid.uCallbackMessage = TRAY_CALLBACK_MESSAGE;
            nid.hIcon = icon;
            copyTip(nid.szTip, sizeof(nid.szTip) / sizeof(nid.szTip[0]), tooltip.c_str());
            if ( !Shell_NotifyIconW(NIM_ADD, &nid) ) return false;
            nid.uVersion = NOTIFYICON_VERSION_4;
            callbacksV4 = Shell_NotifyIconW(NIM_SETVERSION, &nid) != FALSE;
            iconAdded = true;
            return true;
        }

        void modifyIcon(UINT flags) {
            if ( !iconAdded ) return;
            NOTIFYICONDATAW nid = iconData();
            nid.uFlags = flags | NIF_TIP | NIF_SHOWTIP;
            nid.hIcon = icon;
            copyTip(nid.szTip, sizeof(nid.szTip) / sizeof(nid.szTip[0]), tooltip.c_str());
            Shell_NotifyIconW(NIM_MODIFY, &nid);
        }

        virtual void setIcon(const uint8_t * rgba8, int32_t width, int32_t height) override {
            BITMAPV5HEADER bi = {};
            bi.bV5Size = sizeof(bi);
            bi.bV5Width = width;
            bi.bV5Height = -height;
            bi.bV5Planes = 1;
            bi.bV5BitCount = 32;
            bi.bV5Compression = BI_BITFIELDS;
            bi.bV5RedMask = 0x00FF0000;
            bi.bV5GreenMask = 0x0000FF00;
            bi.bV5BlueMask = 0x000000FF;
            bi.bV5AlphaMask = 0xFF000000;
            void * bits = nullptr;
            HDC dc = GetDC(nullptr);
            HBITMAP color = CreateDIBSection(dc, (BITMAPINFO *) &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
            ReleaseDC(nullptr, dc);
            if ( !color || !bits ) return;
            uint8_t * dst = (uint8_t *) bits;
            size_t count = size_t(width) * size_t(height);
            for ( size_t i = 0; i != count; ++i ) {
                dst[i * 4 + 0] = rgba8[i * 4 + 2];
                dst[i * 4 + 1] = rgba8[i * 4 + 1];
                dst[i * 4 + 2] = rgba8[i * 4 + 0];
                dst[i * 4 + 3] = rgba8[i * 4 + 3];
            }
            vector<uint8_t> maskBits(size_t(((width + 15) / 16) * 2) * size_t(height), 0);
            HBITMAP mask = CreateBitmap(width, height, 1, 1, maskBits.data());
            ICONINFO ii = {};
            ii.fIcon = TRUE;
            ii.hbmMask = mask;
            ii.hbmColor = color;
            HICON created = CreateIconIndirect(&ii);
            DeleteObject(mask);
            DeleteObject(color);
            if ( !created ) return;
            HICON previous = icon;
            icon = created;
            modifyIcon(NIF_ICON);
            if ( previous ) DestroyIcon(previous);
        }

        virtual void setTooltip(const char * text) override {
            tooltip = text;
            modifyIcon(NIF_TIP | NIF_SHOWTIP);
        }

        virtual void setMenu(const vector<TrayMenuEntry> & newEntries) override {
            entries = newEntries;
            if ( menu ) DestroyMenu(menu);
            menu = CreatePopupMenu();
            for ( const auto & e : entries ) {
                if ( e.separator ) {
                    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                    continue;
                }
                UINT flags = MF_STRING | (e.enabled ? MF_ENABLED : MF_GRAYED) | (e.checked ? MF_CHECKED : MF_UNCHECKED);
                AppendMenuW(menu, flags, UINT_PTR(e.id), utf8ToWide(e.label.c_str()).c_str());
            }
        }

        virtual void poll(vector<TrayEvent> & out) override {
            MSG msg;
            while ( PeekMessageW(&msg, hwnd, 0, 0, PM_REMOVE) ) {
                DispatchMessageW(&msg);
            }
            out.swap(events);
            events.clear();
        }

        virtual bool notify(const char * title, const char * body) override {
            if ( !iconAdded ) return false;
            NOTIFYICONDATAW nid = iconData();
            nid.uFlags = NIF_INFO | (icon ? NIF_ICON : 0);
            nid.hIcon = icon;
            nid.dwInfoFlags = icon ? (NIIF_USER | NIIF_LARGE_ICON) : NIIF_INFO;
            nid.hBalloonIcon = icon;
            copyTip(nid.szInfoTitle, sizeof(nid.szInfoTitle) / sizeof(nid.szInfoTitle[0]), title);
            copyTip(nid.szInfo, sizeof(nid.szInfo) / sizeof(nid.szInfo[0]), body);
            return Shell_NotifyIconW(NIM_MODIFY, &nid) != FALSE;
        }

        void showMenu(int32_t x, int32_t y) {
            if ( !menu || GetMenuItemCount(menu) == 0 || menuOpen ) return;
            menuOpen = true;
            SetForegroundWindow(hwnd);
            UINT picked = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY, x, y, 0, hwnd, nullptr);
            PostMessageW(hwnd, WM_NULL, 0, 0);
            menuOpen = false;
            if ( picked != 0 ) events.push_back(TrayEvent{TrayEventKind::menu, int32_t(picked), 0, 0});
        }

        //! Without the version-4 handshake the shell packs the legacy layout: wParam is the icon id
        //! and lParam the mouse message, with no coordinates.
        UINT legacyEvent(LPARAM lParam, int32_t & x, int32_t & y) {
            POINT cursor = {};
            GetCursorPos(&cursor);
            x = cursor.x;
            y = cursor.y;
            switch ( UINT(lParam) ) {
                case WM_LBUTTONUP: return NIN_SELECT;
                case WM_RBUTTONUP: return WM_CONTEXTMENU;
                case WM_LBUTTONDBLCLK: return WM_LBUTTONDBLCLK;
                case NIN_BALLOONUSERCLICK: return NIN_BALLOONUSERCLICK;
                default: return 0;
            }
        }

        LRESULT onMessage(UINT message, WPARAM wParam, LPARAM lParam) {
            if ( message == TRAY_CALLBACK_MESSAGE ) {
                int32_t x = 0;
                int32_t y = 0;
                UINT event = 0;
                if ( callbacksV4 ) {
                    x = GET_X_LPARAM(wParam);
                    y = GET_Y_LPARAM(wParam);
                    event = LOWORD(lParam);
                } else {
                    event = legacyEvent(lParam, x, y);
                }
                switch ( event ) {
                    case NIN_SELECT:
                    case NIN_KEYSELECT:
                        events.push_back(TrayEvent{TrayEventKind::click, 0, x, y});
                        break;
                    case WM_LBUTTONDBLCLK:
                        events.push_back(TrayEvent{TrayEventKind::double_click, 0, x, y});
                        break;
                    case WM_CONTEXTMENU:
                        events.push_back(TrayEvent{TrayEventKind::right_click, 0, x, y});
                        showMenu(x, y);
                        break;
                    case NIN_BALLOONUSERCLICK:
                        events.push_back(TrayEvent{TrayEventKind::notification, 0, x, y});
                        break;
                    default:
                        break;
                }
                return 0;
            }
            if ( taskbarCreated && message == taskbarCreated ) {
                iconAdded = false;
                addIcon();
                return 0;
            }
            return DefWindowProcW(hwnd, message, wParam, lParam);
        }

        static LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
            if ( message == WM_NCCREATE ) {
                auto cs = (CREATESTRUCTW *) lParam;
                SetWindowLongPtrW(window, GWLP_USERDATA, (LONG_PTR) cs->lpCreateParams);
            }
            auto tray = (WinTray *) GetWindowLongPtrW(window, GWLP_USERDATA);
            if ( !tray || !tray->hwnd ) return DefWindowProcW(window, message, wParam, lParam);
            return tray->onMessage(message, wParam, lParam);
        }
    };

    bool TrayPlatformAvailable() {
        HWINSTA station = GetProcessWindowStation();
        USEROBJECTFLAGS flags = {};
        DWORD needed = 0;
        if ( !station || !GetUserObjectInformationW(station, UOI_FLAGS, &flags, sizeof(flags), &needed) ) return false;
        if ( (flags.dwFlags & WSF_VISIBLE) == 0 ) return false;
        return FindWindowW(L"Shell_TrayWnd", nullptr) != nullptr;
    }

    TrayBackend * TrayPlatformCreate() {
        if ( !TrayPlatformAvailable() ) return nullptr;
        return new WinTray();
    }
}
