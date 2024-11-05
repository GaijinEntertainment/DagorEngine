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

__declspec(dllexport) int LibNumberClasses()
{
#ifdef PUBLIC_RELEASE
#if MAX_RELEASE >= 4000
  return 11;
#else
  return 10;
#endif
#else
#if MAX_RELEASE >= 4000
  return 17;
#else
  return 16;
#endif
#endif
}

__declspec(dllexport) ClassDesc *LibClassDesc(int i)
{
  switch (i)
  {
#ifdef PUBLIC_RELEASE
    case 0: return GetMaterCD();
    case 1: return GetMaterCD2();
    case 2: return GetExpUtilCD();
    case 3: return GetDummyCD();
    case 4: return GetTexmapsCD();
    case 5: return GetMatConvUtilCD();
    case 6: return GetObjectPropertiesEditorCD();
    case 8: return GetRBDummyCD();
    case 9: return GetDAGEXPCD();
#if MAX_RELEASE >= 4000
    case 10: return GetDagFreeCamUtilCD();
#endif
#else
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
    case 14: return GetRBDummyCD();
    case 15: return GetDAGEXPCD();
#if MAX_RELEASE >= 4000
    case 16: return GetDagFreeCamUtilCD();
#endif
#endif
    default: return NULL;
  }
}

__declspec(dllexport) ULONG LibVersion() { return VERSION_3DSMAX; }
