// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_progGlobals.h>
#include <supp/_platform.h>
#include <debug/dag_fatal.h>

namespace workcycle_internal
{
void set_title(const char *title, bool utf8);
void set_title_tooltip(const char *title, const char *tooltip, bool utf8);
} // namespace workcycle_internal

void win32_set_window_title(const char *title) { workcycle_internal::set_title(title, false); }

void win32_set_window_title_utf8(const char *title) { workcycle_internal::set_title(title, true); }

void win32_set_window_title_tooltip_utf8(const char *title, const char *tooltip)
{
  workcycle_internal::set_title_tooltip(title, tooltip, true);
}

#define EXPORT_PULL dll_pull_osapiwrappers_setTitle
#include <supp/exportPull.h>
