// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include <windows.h>
#include <CommDlg.h>

#include <winGuiWrapper/wgw_dialogs.h>
#include <winGuiWrapper/wgw_busy.h>
#include <libTools/util/strUtil.h>
#include <propPanel2/comWnd/search_replace_dialog.h>

#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "user32.lib")

namespace wingw
{
//-----------------------------------------
//        file dialog
//-----------------------------------------

String select_file(void *hwnd, const char fn[], const char mask[], const char initial_dir[], bool save_nopen, const char def_ext[],
  const char title[])
{
  bool _busy = wingw::set_busy(false);

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
  ofn.lpstrTitle = title;
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

  wingw::set_busy(_busy);

  if (result)
  {
    ::make_slashes(_fn);
    return String(_fn);
  }

  return String();
}


String file_open_dlg(void *hwnd, const char caption[], const char filter[], const char def_ext[], const char init_path[],
  const char init_fn[])
{
  return select_file(hwnd, init_fn, filter, init_path, false, def_ext, caption);
}


String file_save_dlg(void *hwnd, const char caption[], const char filter[], const char def_ext[], const char init_path[],
  const char init_fn[])
{
  return select_file(hwnd, init_fn, filter, init_path, true, def_ext, caption);
}


//-----------------------------------------
//        color dialog
//-----------------------------------------

bool select_color(void *hwnd, E3DCOLOR &value)
{
  CHOOSECOLOR pclr;
  static COLORREF acrCustClr[16];
  ZeroMemory(&pclr, sizeof(pclr));

  pclr.Flags = CC_FULLOPEN | CC_RGBINIT;
  pclr.lpCustColors = (LPDWORD)acrCustClr;
  pclr.rgbResult = RGB(value.r, value.g, value.b);
  pclr.lStructSize = sizeof(CHOOSECOLOR);
  pclr.hwndOwner = (HWND)hwnd;

  if (ChooseColor(&pclr))
  {
    value = E3DCOLOR(GetRValue(pclr.rgbResult), GetGValue(pclr.rgbResult), GetBValue(pclr.rgbResult));
    return true;
  }

  return false;
}

//-----------------------------------------
//        message box
//-----------------------------------------

int message_box(int flags, const char *caption, const char *text, const DagorSafeArg *arg, int anum)
{
  bool _busy = wingw::set_busy(false);

  int ret = 0;

  String msg;
  msg.vprintf(2048, text, arg, anum);

  ret = ::MessageBox(FindWindow("EDITOR_LAYOUT_DE_WINDOW", NULL), msg, caption, flags);

  wingw::set_busy(_busy);

  return ret;
}

//-----------------------------------------

bool search_dialog(void *hwnd, SimpleString &search_text, int &mask)
{
  // SearchReplaceDialog srDialog(hwnd, mask, search_text, search_text);
  // if (srDialog.showDialog() == DIALOG_ID_OK)
  //   return true;

  return false;
}


bool search_replace_dialog(void *hwnd, SimpleString &search_text, SimpleString &replace_text, int &mask)
{
  // SearchReplaceDialog srDialog(hwnd, mask, search_text, replace_text, false);
  // if (srDialog.showDialog() == DIALOG_ID_OK)
  //   return true;

  return false;
}
} // namespace wingw
