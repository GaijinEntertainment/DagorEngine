//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#if _TARGET_PC_WIN | _TARGET_XBOX | _TARGET_C1 | _TARGET_C2
#define DAG_DLL_EXPORT extern "C" __declspec(dllexport)
#else
#define DAG_DLL_EXPORT extern "C" __attribute__((visibility("default")))
#endif
