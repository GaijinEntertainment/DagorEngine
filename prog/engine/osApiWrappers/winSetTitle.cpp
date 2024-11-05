// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_progGlobals.h>
#include <supp/_platform.h>
#include <debug/dag_fatal.h>

void win32_set_window_title(const char *title)
{
  HWND hwnd = (HWND)win32_get_main_wnd();
  if (hwnd)
    SetWindowText(hwnd, title);
}
void win32_set_window_title_utf8(const char *title)
{
  HWND hwnd = (HWND)win32_get_main_wnd();
  if (hwnd)
  {
    wchar_t wTitle[4096];
    int used = MultiByteToWideChar(CP_UTF8, 0, title, i_strlen(title) + 1, wTitle, 4096);
    if (used && used < 4096)
      SetWindowTextW(hwnd, wTitle);
    else
      DAG_FATAL("title is incorrect or too big (srclen=%d, title=%s, err=%08X)", i_strlen(title), title, GetLastError());
  }
}

#define EXPORT_PULL dll_pull_osapiwrappers_setTitle
#include <supp/exportPull.h>
