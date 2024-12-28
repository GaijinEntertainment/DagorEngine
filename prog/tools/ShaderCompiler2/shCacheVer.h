// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_globDef.h>

// See https://gaijinentertainment.github.io/DagorEngine/dagor-tools/shader-compiler/contributing_to_compiler.html#versioning

#if _CROSS_TARGET_SPIRV
#include "ver_obj_spirv.h"
#elif _CROSS_TARGET_DX12
#include "ver_obj_dxc.h"
#endif

enum
{
  SHADER_CACHE_SIGN = _MAKE4C('Oz'),
  SHADER_CACHE_EOF = _MAKE4C('eof')
};

// Increase this number if changes in the compiler affect all APIs
static const int SHADER_CACHE_COMMON_VER = 23;

static int make_shader_cache_ver(int ver)
{
  char a[4] = {_DUMP4C(ver)};
  int v = 100 * (a[0] - '0') + 10 * (a[1] - '0') + (a[3] - '0');
  v += SHADER_CACHE_COMMON_VER;
  a[0] = v / 100 + '0';
  a[1] = v % 100 / 10 + '0';
  a[3] = v % 10 + '0';
  return MAKE4C(a[0], a[1], a[2], a[3]);
}

static const int SHADER_CACHE_VER = make_shader_cache_ver(
#if _CROSS_TARGET_METAL
#include "ver_obj_metal.h"
#elif _CROSS_TARGET_SPIRV
  VER_OBJ_SPIRV_VAL
#elif _CROSS_TARGET_C1

#elif _CROSS_TARGET_C2

#elif _CROSS_TARGET_DX12
  VER_OBJ_DXC_VAL
#else
#include "ver_obj_pcdx.h"
#endif
);