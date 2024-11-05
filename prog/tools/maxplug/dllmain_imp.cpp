// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <max.h>
#include "dagor.h"
// #include "resource.h"

HINSTANCE hInstance;

TCHAR *GetString(int id)
{
  static TCHAR buf[256];

  if (hInstance)
    return LoadString(hInstance, id, buf, sizeof(buf)) ? buf : NULL;
  return NULL;
}

void load_dagorpath_cfg();

BOOL WINAPI DllMain(HINSTANCE hinstDLL, ULONG fdwReason, LPVOID lpvReserved)
{
  static int controlsInit = FALSE;
  hInstance = hinstDLL;
  if (!controlsInit)
  {
    controlsInit = TRUE;
#if !defined(MAX_RELEASE_R13) || MAX_RELEASE < MAX_RELEASE_R13
    InitCustomControls(hInstance);
#endif
    InitCommonControls();
  }
  load_dagorpath_cfg();
  return TRUE;
}

__declspec(dllexport) const TCHAR *LibDescription() { return _T("Dagor Utilities - for internal usage only :-)"); }

__declspec(dllexport) int LibNumberClasses() { return 1; }

__declspec(dllexport) ClassDesc *LibClassDesc(int i)
{
  switch (i)
  {
    case 0: return GetDAGIMPCD();
    default: return NULL;
  }
}

__declspec(dllexport) ULONG LibVersion() { return VERSION_3DSMAX; }
