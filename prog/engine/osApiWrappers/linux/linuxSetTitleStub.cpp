// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_progGlobals.h>

void win32_set_window_title(const char *) {}

void win32_set_window_title_utf8(const char *) {}

void win32_set_window_title_tooltip_utf8(const char *, const char *) {}

#define EXPORT_PULL dll_pull_osapiwrappers_setTitle
#include <supp/exportPull.h>
