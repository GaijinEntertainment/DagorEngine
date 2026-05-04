//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <supp/dag_dllexport.h>

#if _TARGET_STATIC_LIB
#define DE3_DLL_PLUGIN_LINKAGE static
#define DE3_DLL_PLUGIN_CALLCONV
#else
#define DE3_DLL_PLUGIN_LINKAGE  DAG_DLL_EXPORT
#define DE3_DLL_PLUGIN_CALLCONV __fastcall
#endif
