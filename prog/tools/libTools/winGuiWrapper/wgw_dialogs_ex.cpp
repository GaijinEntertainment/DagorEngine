// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include <windows.h>
#include <shlobj.h>

#include <winGuiWrapper/wgw_dialogs.h>
#include <libTools/util/strUtil.h>

#include <debug/dag_debug.h>

#pragma comment(lib, "shell32.lib")

namespace wingw
{
bool _is_init = false;

void init()
{
  if (_is_init)
    return;

  G_ASSERT((CoInitialize(NULL) != S_OK) && "p2dlg ERROR: COM not initialized!");

  _is_init = true;
}

//-----------------------------------------
//        directory dialog
//-----------------------------------------


static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT msg, LPARAM l_param, LPARAM data)
{
  switch (msg)
  {
    case BFFM_INITIALIZED:
    {
      const char *toSelect = (const char *)data;
      wchar_t selPath[MAXIMUM_PATH_LEN];
      const int pathLen = (int)::strlen(toSelect);
      ::SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)toSelect);
      break;
    }

    case BFFM_VALIDATEFAILED: message_box(MBS_EXCL, "Invalid folder", "Folder %s not exists.", (char *)l_param); return 1;
  }

  return 0;
}


String dir_select_dlg(void *hwnd, const char caption[], const char init_path[])
{
  init();

  static BROWSEINFO bi;
  bi.hwndOwner = (HWND)hwnd;

  LPSTR lpBuffer;
  LPITEMIDLIST pidlRoot, pidlBrowse;
  LPMALLOC pMalloc = NULL;
  HRESULT hRes = CoGetMalloc(1, &pMalloc);

  char _path[MAXIMUM_PATH_LEN];
  strcpy(_path, init_path);
  ::make_ms_slashes(_path);

  if (SUCCEEDED(hRes))
  {
    lpBuffer = (LPSTR)pMalloc->Alloc(MAX_PATH);
    SHGetSpecialFolderLocation(bi.hwndOwner, CSIDL_DESKTOP, &pidlRoot);

    bi.pidlRoot = pidlRoot;
    bi.pszDisplayName = lpBuffer;
    bi.lpszTitle = caption;
    bi.ulFlags = BIF_NEWDIALOGSTYLE | BIF_RETURNONLYFSDIRS | BIF_SHAREABLE | BIF_EDITBOX | BIF_VALIDATE;
    bi.lpfn = BrowseCallbackProc;
    bi.lParam = LPARAM(_path);

    pidlBrowse = SHBrowseForFolder(&bi);
    if ((pidlBrowse != NULL) && (SHGetPathFromIDList(pidlBrowse, lpBuffer)))
    {
      if (strlen(lpBuffer) < sizeof(_path))
      {
        strcpy(_path, lpBuffer);
        ::make_slashes(_path);
        return String(_path);
      }
    }

    pMalloc->Free(lpBuffer);
  }
  else
  {
    debug("CFileEditBox: CoGetMalloc FAILED!!!");
  }

  return String();
}

} // namespace wingw