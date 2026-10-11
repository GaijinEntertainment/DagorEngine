// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <string_view>
#include <vector>

#include "../compileResult.h"

#include <spirv/compiler.h>

struct Hlsl2SpirvResult
{
  bool failed = false;

  std::vector<unsigned int> byteCode;

  ComputeShaderInfo computeInfo;

  eastl::vector<spirv::ReflectionInfo> reflection;
};


Hlsl2SpirvResult hlsl2spirv(const spirv::DXCContext *dxc_ctx, const char *source, const char *profile, const char *entry,
  bool hlsl2021, bool enable_fp16, bool skip_validation, int implicit_cbuf_size, CompileResult &compile_result, bool enable_bindless);
