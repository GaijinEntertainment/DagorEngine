// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <max.h>
#include "dagor.h"

HINSTANCE hInstance;

TCHAR *GetString(int id)
{
  static TCHAR buf[256];

  if (hInstance)
    return LoadString(hInstance, id, buf, sizeof(buf) / sizeof(*buf)) ? buf : NULL;
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
    InitCommonControls();
  }
  load_dagorpath_cfg();
  return TRUE;
}

__declspec(dllexport) const TCHAR *LibDescription() { return _T("Dagor Utilities - for internal usage only :-)"); }

__declspec(dllexport) int LibNumberClasses() { return 18; }

__declspec(dllexport) ClassDesc *LibClassDesc(int i)
{
  switch (i)
  {
    case 0: return GetFontUtilCD();
    case 1: return GetMaterCD();
    case 2: return GetMaterCD2();
    case 3: return GetExpUtilCD();
    case 4: return GetDagUtilCD();
    case 5: return GetDummyCD();
    case 6: return GetTexmapsCD();
    case 7: return GetVPnormCD();
    case 8: return GetVPzbufCD();
    case 9: return GetTexAnimIOCD();
    case 10: return GetPolyBumpCD();
    case 11: return GetMatConvUtilCD();
    case 12: return GetObjectPropertiesEditorCD();
    case 13: return GetRBDummyCD();
    case 14: return GetDAGEXPCD();
    case 15: return GetDagFreeCamUtilCD();
    case 16: return GetDAGIMPCD();
    case 17: return GetImpUtilCD();
    default: return NULL;
  }
}

__declspec(dllexport) ULONG LibVersion() { return VERSION_3DSMAX; }
