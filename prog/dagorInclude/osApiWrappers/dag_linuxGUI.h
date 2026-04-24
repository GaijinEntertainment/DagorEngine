//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#ifndef _TARGET_PC_LINUX
#error using linux specific implementation with wrong platform
#endif

#include <generic/dag_tab.h>
#include <util/dag_string.h>

#include <supp/dag_define_KRNLIMP.h>

namespace linux_GUI
{

struct RECT
{
  int left, top, right, bottom;
};

// Init linux GUI system, X11 or Wayland, depending on underlying conditions
KRNLIMP bool init();
KRNLIMP bool is_wayland();
KRNLIMP bool is_x11();
KRNLIMP void shutdown();

KRNLIMP bool change_gamma(float value);
KRNLIMP void get_display_size(int &width, int &height, bool for_primary_output = false);
KRNLIMP void get_video_mode_list(Tab<String> &list);

KRNLIMP void *get_main_window_ptr_handle();
KRNLIMP bool is_main_window(void *wnd);
KRNLIMP void destroy_main_window();

KRNLIMP bool init_window(const char *title, int winWidth, int winHeight);
KRNLIMP void get_window_position(void *w, int &cx, int &cy);
KRNLIMP void set_title(const char *title, const char *tooltip = NULL);
KRNLIMP void set_title_utf8(const char *title, const char *tooltip = NULL);

KRNLIMP int get_screen_refresh_rate();
KRNLIMP void set_fullscreen_mode(bool enable);
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

KRNLIMP bool get_clipboard_utf8_text(char *dest, int buf_size);
KRNLIMP bool set_clipboard_utf8_text(const char *text);

} // namespace linux_GUI

#include <supp/dag_undef_KRNLIMP.h>
