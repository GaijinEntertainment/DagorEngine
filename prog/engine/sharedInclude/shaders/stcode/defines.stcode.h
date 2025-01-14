// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#ifndef __GENERATED_STCODE_FILE
#error This file can only be included in generated stcode
#endif

#if _TARGET_PC_WIN | _TARGET_XBOX
#define CPPSTCODE_DLL_EXPORT extern "C" __declspec(dllexport)
#elif _TARGET_PC_LINUX | _TARGET_PC_MACOSX | _TARGET_ANDROID | _TARGET_C3 | _TARGET_IOS
#define CPPSTCODE_DLL_EXPORT extern "C"
#else
#define CPPSTCODE_DLL_EXPORT extern "C" __attribute__((dllexport))
#endif