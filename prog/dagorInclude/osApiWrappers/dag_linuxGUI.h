//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#ifndef _TARGET_PC_LINUX
#error using linux specific implementation with wrong platform
#endif

#include <generic/dag_tab.h>
#include <math/integer/dag_IPoint2.h>
#include <util/dag_string.h>

#include <supp/dag_define_KRNLIMP.h>

#include <EASTL/optional.h>

namespace linux_GUI
{

struct RECT
{
  int left, top, right, bottom;
};

struct WindowCreationOptions
{
  eastl::optional<IPoint2> position;
  bool maximized = true; // Only when windowMode is WindowMode::WINDOWED_RESIZABLE.
};

// Init linux GUI system, X11 or Wayland, depending on underlying conditions
KRNLIMP bool init();
KRNLIMP bool is_wayland();
KRNLIMP bool is_x11();
KRNLIMP void shutdown();

KRNLIMP bool change_gamma(float value);
KRNLIMP void get_display_size(int &width, int &height, bool for_primary_output = false);
KRNLIMP void get_video_mode_list(Tab<String> &list);

// Returns unique, identifiable names of active (connected) monitors.
KRNLIMP void get_monitors(Tab<String> &monitor_names);
// Returns user-friendly info about a monitor. monitor_name == nullptr/empty selects the primary/default monitor.
// friendly_name and monitor_index are optional (may be nullptr). Returns false if the monitor was not found.
KRNLIMP bool get_monitor_info(const char *monitor_name, String *friendly_name, int *monitor_index);
// Returns available resolutions ("W x H") for a single monitor. monitor_name == nullptr/empty selects the primary/default monitor.
KRNLIMP void get_resolutions_from_monitor(const char *monitor_name, Tab<String> &resolutions);

KRNLIMP void *get_main_window_ptr_handle();
KRNLIMP bool is_main_window(void *wnd);
KRNLIMP void destroy_main_window();

KRNLIMP bool init_window(const char *title, int winWidth, int winHeight, const WindowCreationOptions &options);
KRNLIMP void get_window_position(void *w, int &cx, int &cy);

// Get the left top position of the visible frame in absolute, screen coordinates.
// So this function returns 0, 0 when the window is at the left top corner of the desktop.
KRNLIMP void get_window_frame_position(void *w, int &x, int &y);

KRNLIMP bool is_window_maximized(void *w);
KRNLIMP void set_title(const char *title, const char *tooltip = NULL);
KRNLIMP void set_title_utf8(const char *title, const char *tooltip = NULL);

KRNLIMP int get_screen_refresh_rate();
KRNLIMP void set_fullscreen_mode(bool enable);

// Returns with the full, unclipped size of the client area.
KRNLIMP bool get_window_client_size(void *w, int &width, int &height);

KRNLIMP bool get_window_screen_rect(void *w, RECT *rect, RECT *rect_unclipped);

KRNLIMP void process_messages();

KRNLIMP bool get_last_cursor_pos(int *cx, int *cy, void *w);
KRNLIMP void set_cursor(void *w, const char *cursor_name);
KRNLIMP void set_cursor_position(int cx, int cy, void *w);
KRNLIMP void get_absolute_cursor_position(int &cx, int &cy);
KRNLIMP bool get_cursor_delta(int &cx, int &cy, void *w);
KRNLIMP void clip_cursor();
KRNLIMP void unclip_cursor();
KRNLIMP void hide_cursor(bool hide);
KRNLIMP void *get_native_display();
KRNLIMP void *get_native_window(void *w);

// Serialize a window's Wayland requests against the render/present thread. No-op on X11.
KRNLIMP void lock_window(void *w);
KRNLIMP void unlock_window(void *w);

KRNLIMP bool get_clipboard_utf8_text(char *dest, int buf_size);
KRNLIMP bool set_clipboard_utf8_text(const char *text);

} // namespace linux_GUI

#include <supp/dag_undef_KRNLIMP.h>
