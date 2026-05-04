// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_progGlobals.h>
#include <supp/_platform.h>
#include <debug/dag_fatal.h>
#include <osApiWrappers/dag_linuxGUI.h>

void win32_set_window_title(const char *title) { linux_GUI::set_title(title); }

void win32_set_window_title_utf8(const char *title) { linux_GUI::set_title_utf8(title); }

void win32_set_window_title_tooltip_utf8(const char *title, const char *tooltip) { linux_GUI::set_title_utf8(title, tooltip); }

#define EXPORT_PULL dll_pull_osapiwrappers_setTitle
#include <supp/exportPull.h>
