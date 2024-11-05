// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <windows.h>

// Since the AVX API is not declared in the Windows 7 SDK headers and
// since we don't have the proper libs to work with, we will declare
// the API as function pointers and get them with GetProcAddress calls
// from kernel32.dll.  We also need to set some #defines.

#pragma warning(disable : 4191) // function type cast

#define XSTATE_AVX      (XSTATE_GSSE)
#define XSTATE_MASK_AVX (XSTATE_MASK_GSSE)

typedef DWORD64(WINAPI *PGETENABLEDXSTATEFEATURES)();
typedef BOOL(WINAPI *PINITIALIZECONTEXT)(PVOID Buffer, DWORD ContextFlags, PCONTEXT *Context, PDWORD ContextLength);
typedef BOOL(WINAPI *PGETXSTATEFEATURESMASK)(PCONTEXT Context, PDWORD64 FeatureMask);
typedef PVOID(WINAPI *LOCATEXSTATEFEATURE)(PCONTEXT Context, DWORD FeatureId, PDWORD Length);
typedef BOOL(WINAPI *SETXSTATEFEATURESMASK)(PCONTEXT Context, DWORD64 FeatureMask);

PGETENABLEDXSTATEFEATURES pfnGetEnabledXStateFeatures;

bool isAvxXStateFeatureEnabled()
{
  // If this function was called before and we were not running on
  // at least Windows 7 SP1, then bail.
  if (pfnGetEnabledXStateFeatures == (PGETENABLEDXSTATEFEATURES)-1)
    return false;

  // Get the addresses of the AVX XState functions.
  if (pfnGetEnabledXStateFeatures == NULL)
  {
    HMODULE hm = GetModuleHandleA("kernel32.dll");
    pfnGetEnabledXStateFeatures = (PGETENABLEDXSTATEFEATURES)GetProcAddress(hm, "GetEnabledXStateFeatures");
    if (pfnGetEnabledXStateFeatures == NULL)
      return false;
  }

  DWORD64 FeatureMask = pfnGetEnabledXStateFeatures();
  return FeatureMask & XSTATE_MASK_AVX;
}

#define EXPORT_PULL dll_pull_osapiwrappers_winXSaveFeatures
#include <supp/exportPull.h>
