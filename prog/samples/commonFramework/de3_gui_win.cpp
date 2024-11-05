// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "de3_gui_dialogs.h"
#include <windows.h>
#include <CommDlg.h>
#include <util/dag_string.h>

enum
{
  MAXIMUM_PATH_LEN = 2048
};

#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "user32.lib")

static inline char *make_ms_slashes(char *path)
{
  for (char *c = path; *c; ++c)
    if (*c == '/')
      *c = '\\';
  return path;
}
static inline char *make_slashes(char *path)
{
  for (char *c = path; *c; ++c)
    if (*c == '\\')
      *c = '/';
  return path;
}

static String select_file(void *hwnd, const char fn[], const char mask[], const char initial_dir[], bool save_nopen,
  const char def_ext[], const char title[])
{
  bool result = false;
  OPENFILENAME ofn;
  char _fn[MAXIMUM_PATH_LEN];
  strcpy(_fn, fn);
  ::make_ms_slashes(_fn);

  char _dir[MAXIMUM_PATH_LEN];
  strcpy(_dir, initial_dir);
  ::make_ms_slashes(_dir);

  char _mask[MAXIMUM_PATH_LEN] = "";
  strcpy(_mask, mask);
  strcat(_mask, "|");
  int _len = (int)strlen(_mask);
  for (int i = 0; i < _len; ++i)
  {
    if (_mask[i] == '|')
      _mask[i] = '\0';
  }

  ZeroMemory(&ofn, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = (HWND)hwnd;
  ofn.lpstrFile = _fn;
  ofn.nMaxFile = MAXIMUM_PATH_LEN;
  ofn.lpstrFilter = _mask;
  ofn.nFilterIndex = 0;
  ofn.lpstrFileTitle = (LPSTR)title;
  ofn.lpstrInitialDir = (LPSTR)_dir;
  if (strlen(def_ext))
    ofn.lpstrDefExt = (LPSTR)def_ext;

  if (!save_nopen)
  {
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    result = GetOpenFileName(&ofn);
  }
  else
  {
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_EXTENSIONDIFFERENT;
    result = GetSaveFileName(&ofn);
  }

  if (result)
  {
    ::make_slashes(_fn);
    return String(_fn);
  }

  return String();
}


String de3_imgui_file_open_dlg(const char *caption, const char *filter, const char *def_ext, const char *init_path,
  const char *init_fn)
{
  return select_file(NULL, init_fn, filter, init_path, false, def_ext, caption);
}

String de3_imgui_file_save_dlg(const char *caption, const char *filter, const char *def_ext, const char *init_path,
  const char *init_fn)
{
  return select_file(NULL, init_fn, filter, init_path, true, def_ext, caption);
}
