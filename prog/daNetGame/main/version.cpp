// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "version.h"
#include <stdio.h>
#include <util/dag_compilerDefs.h>

#define STR(x)  #x
#define XSTR(x) STR(x)

#ifdef BUILD_NUMBER
static const int g_build_number = BUILD_NUMBER;
#else
static const int g_build_number = -1;
#endif
int get_build_number() { return g_build_number; }

#if defined(VERSION_DOT)
static const char version_str_internal[] = XSTR(VERSION_DOT);
const char *get_exe_version_str() { return version_str_internal; }
#else
const char *get_exe_version_str() { return "0.0.0.0"; }
#endif

static uint32_t exe_version_internal = ~0u;
exe_version32_t get_exe_version32()
{
  if (DAGOR_UNLIKELY(exe_version_internal == ~0u))
  {
    uint8_t v1 = 0, v2 = 0, v3 = 0, v4 = 0;
    sscanf(get_exe_version_str(), "%hhu.%hhu.%hhu.%hhu", &v1, &v2, &v3, &v4);
    exe_version_internal = (unsigned(v1) << 24) | (unsigned(v2) << 16) | (unsigned(v3) << 8) | unsigned(v4);
  }
  return exe_version_internal;
}
