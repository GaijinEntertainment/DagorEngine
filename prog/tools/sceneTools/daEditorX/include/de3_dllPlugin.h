//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#if _TARGET_STATIC_LIB
#define DE3_DLL_PLUGIN_LINKAGE static
#define DE3_DLL_PLUGIN_CALLCONV
#else
#define DE3_DLL_PLUGIN_LINKAGE  extern "C"
#define DE3_DLL_PLUGIN_CALLCONV __fastcall
#endif
