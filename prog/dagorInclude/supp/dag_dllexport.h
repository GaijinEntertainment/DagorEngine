//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#if _TARGET_PC_WIN | _TARGET_XBOX
#define DAG_DLL_EXPORT extern "C" __declspec(dllexport)
#elif _TARGET_C1 | _TARGET_C2

#else
#define DAG_DLL_EXPORT extern "C" __attribute__((visibility("default")))
#endif
