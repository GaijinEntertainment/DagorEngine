// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_linuxGUI.h>
#include <startup/dag_globalSettings.h>
#include <debug/dag_logSys.h>
#include <stdlib.h>

#if USE_X11
#include "x11.h"
#else
struct X11
{};
#endif

#if USE_WAYLAND
#include "wayland.h"
#else
struct Wayland
{};
#endif

using namespace linux_GUI;

namespace
{
X11 x11;
Wayland wayland;
bool usingWayland;
bool inited = false;
}; // namespace

#if USE_WAYLAND && USE_X11
#define ROUTE(expr)      \
  G_ASSERT(inited);      \
  if (usingWayland)      \
  {                      \
    return wayland.expr; \
  }                      \
  else                   \
  {                      \
    return x11.expr;     \
  }
#elif USE_X11
#define ROUTE(expr) \
  G_ASSERT(inited); \
  return x11.expr;
#elif USE_WAYLAND
#define ROUTE(expr) \
  G_ASSERT(inited); \
  return wayland.expr;
#endif

namespace workcycle_internal
{
void idle_loop() { process_messages(); }
} // namespace workcycle_internal

namespace linux_GUI
{

void init_wayland()
{
#if USE_WAYLAND
  const char *xdg_dir = getenv("XDG_RUNTIME_DIR");
  if (xdg_dir)
  {
    debug("wayland: present because XDG_RUNTIME_DIR found (value: %s)", xdg_dir);
    bool useWayland = false;
#if USE_X11
    useWayland = ::dgs_get_settings()->getBlockByNameEx("linux")->getBool("wayland", false) || ::dgs_get_argv("wayland");
#else
    useWayland = true;
#endif
    if (useWayland)
      inited = wayland.init();
    else
      debug("wayland: use x11 due to configuration");
  }
#endif
}

void init_x11()
{
#if USE_X11
  usingWayland = false;
  inited = x11.init();
#endif
}

bool init()
{
  if (inited)
    return true;

  usingWayland = true;
  init_wayland();

  if (!inited)
    init_x11();

  return inited;
}

bool is_wayland() { return inited && usingWayland; }
bool is_x11() { return inited && !usingWayland; }

void shutdown()
{
  ROUTE(shutdown());
  inited = false;
}

bool change_gamma(float value) { ROUTE(changeGamma(value)); }
void get_display_size(int &width, int &height, bool for_primary_output) { ROUTE(getDisplaySize(width, height, for_primary_output)); }
void get_video_mode_list(Tab<String> &list) { ROUTE(getVideoModeList(list)); }
void *get_main_window_ptr_handle() { ROUTE(getMainWindowPtrHandle()); }
bool is_main_window(void *wnd) { ROUTE(isMainWindow(wnd)); }
void destroy_main_window() { ROUTE(destroyMainWindow()); }
bool init_window(const char *title, int winWidth, int winHeight) { ROUTE(initWindow(title, winWidth, winHeight)); }
void get_window_position(void *w, int &cx, int &cy) { ROUTE(getWindowPosition(w, cx, cy)); }
void set_title(const char *title, const char *tooltip) { ROUTE(setTitle(title, tooltip)); }
void set_title_utf8(const char *title, const char *tooltip) { ROUTE(setTitleUTF8(title, tooltip)); }
int get_screen_refresh_rate() { ROUTE(getScreenRefreshRate()); }
void set_fullscreen_mode(bool enable) { ROUTE(setFullscreenMode(enable)); }
void process_messages() { ROUTE(processMessages()); }
bool get_window_screen_rect(void *w, RECT *rect, RECT *rect_unclipped) { ROUTE(getWindowScreenRect(w, rect, rect_unclipped)); }
bool get_last_cursor_pos(int *cx, int *cy, void *w) { ROUTE(getLastCursorPos(cx, cy, w)); }
void set_cursor(void *w, const char *cursor_name) { ROUTE(setCursor(w, cursor_name)); }
void set_cursor_position(int cx, int cy, void *w) { ROUTE(setCursorPosition(cx, cy, w)); }
void get_absolute_cursor_position(int &cx, int &cy) { ROUTE(getAbsoluteCursorPosition(cx, cy)); }
bool get_cursor_delta(int &cx, int &cy, void *w) { ROUTE(getCursorDelta(cx, cy, w)); }
void clip_cursor() { ROUTE(clipCursor()); }
void unclip_cursor() { ROUTE(unclipCursor()); }
void hide_cursor(bool hide) { ROUTE(hideCursor(hide)); }
void *get_native_display() { ROUTE(getNativeDisplay()); }
void *get_native_window(void *w) { ROUTE(getNativeWindow(w)); };
bool get_clipboard_utf8_text(char *dest, int buf_size) { ROUTE(getClipboardUTF8Text(dest, buf_size)); }
bool set_clipboard_utf8_text(const char *text) { ROUTE(setClipboardUTF8Text(text)); }

#undef ROUTE

} // namespace linux_GUI
