//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/unique_ptr.h>
#include <supp/dag_define_KRNLIMP.h>

KRNLIMP void *os_dll_load(const char *filename);
#if _TARGET_PC_LINUX
KRNLIMP void *os_dll_load_deep_bind(const char *filename);
#else
inline void *os_dll_load_deep_bind(const char *filename) { return os_dll_load(filename); }
#endif
KRNLIMP const char *os_dll_get_last_error_str();
KRNLIMP void *os_dll_get_symbol(void *handle, const char *function);
KRNLIMP bool os_dll_close(void *handle);

// Gets the path to the loaded dll that contains a given address and puts it into the buffer, and returns it's start addr.
// Otherwise, returns nullptr, and the buffer is not touched.
KRNLIMP const char *os_dll_get_dll_name_from_addr(char *out_buf, size_t out_buf_size, const void *addr);

struct DagorDynLibCloser
{
  void operator()(void *handle) { handle ? (void)os_dll_close(handle) : (void)0; }
};

using DagorDynLibHolder = eastl::unique_ptr<void, DagorDynLibCloser>;

#include <supp/dag_undef_KRNLIMP.h>

#if _TARGET_PC_WIN | _TARGET_XBOX
#define DAGOR_OS_DLL_SUFFIX ".dll"
#elif _TARGET_PC_LINUX | _TARGET_ANDROID
#define DAGOR_OS_DLL_SUFFIX ".so"
#elif _TARGET_PC_MACOSX | _TARGET_IOS
#define DAGOR_OS_DLL_SUFFIX ".dylib"
#elif _TARGET_C1 | _TARGET_C2

#elif _TARGET_C3

#endif
