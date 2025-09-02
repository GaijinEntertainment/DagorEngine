//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#if _TARGET_PC_WIN
#define DAG_DLL_EXPORT extern "C" __declspec(dllexport)
#else
#define DAG_DLL_EXPORT extern "C" __attribute__((visibility("default")))
#endif
