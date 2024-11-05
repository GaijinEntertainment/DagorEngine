//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <supp/dag_define_KRNLIMP.h>

KRNLIMP void *win32_get_instance();
KRNLIMP void *win32_get_main_wnd();

#if _TARGET_PC
KRNLIMP void win32_set_window_title(const char *title);
KRNLIMP void win32_set_window_title_utf8(const char *title);

#if _TARGET_PC_LINUX
KRNLIMP void win32_set_window_title_tooltip_utf8(const char *title, const char *tooltip = NULL);
#else
inline void win32_set_window_title_tooltip_utf8(const char *title, const char *) { win32_set_window_title_utf8(title); }
#endif //_TARGET_PC_LINUX
#endif //_TARGET_PC

KRNLIMP void win32_set_thread_name(const char *name);

//! global settings to tweak code to be MS RDP compatible (should be set early and not be changed after that)
extern KRNLIMP bool win32_rdp_compatible_mode; // inited as =false

//! HCURSOR cursor used to hide mouse pointer over app window (may be NULL or handle to invisible cursor)
extern KRNLIMP void *win32_empty_mouse_cursor; // inited as =nullptr
//! initializes once and returns win32_empty_mouse_cursor handle
KRNLIMP void *win32_init_empty_mouse_cursor();

//! HCURSOR cursor used to override mouse pointer over app window (if not NULL will override windows cursor)
extern KRNLIMP void *win32_current_mouse_cursor; // inited as =nullptr

//! value returned by GetDpiForSystem() or 96 if system is not DPI aware
extern KRNLIMP int win32_system_dpi; // inited as =96

#include <supp/dag_undef_KRNLIMP.h>
