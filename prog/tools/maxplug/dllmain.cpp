// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <string>
#include <unordered_map>

#include <max.h>
#include "dagor.h"

HINSTANCE hInstance;

const TCHAR *GetString(int id)
{
  if (!hInstance)
    return NULL;

  // Callers such as ClassDesc::ClassName() keep the pointer, so each string gets its own entry
  // whose address never moves, rather than a buffer shared by every id. The cache outlives the
  // static destruction of the module on purpose, so those pointers never dangle.
  static auto &cache = *new std::unordered_map<int, std::wstring>;
  const auto it = cache.find(id);
  if (it != cache.end())
    return it->second.c_str();

  // A zero buffer size makes LoadString return the resource itself, which is not null terminated.
  TCHAR *res = NULL;
  const int len = LoadString(hInstance, id, reinterpret_cast<TCHAR *>(&res), 0);
  if (len <= 0)
    return NULL;

  return cache.emplace(id, std::wstring(res, len)).first->second.c_str();
}

void load_dagorpath_cfg();

BOOL WINAPI DllMain(HINSTANCE hinstDLL, ULONG fdwReason, LPVOID lpvReserved)
{
  if (fdwReason != DLL_PROCESS_ATTACH)
    return TRUE;

  hInstance = hinstDLL;
  InitCommonControls();
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
