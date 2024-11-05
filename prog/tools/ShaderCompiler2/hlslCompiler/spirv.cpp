// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "asmShaderSpirV.h"
#include "defines.h"
#include <util/dag_string.h>
#include <drv/3d/dag_platform_pc.h>
#include <util/dag_strUtil.h>

extern bool enableBindless;

DLL_EXPORT bool compile_compute_shader_spirv(const char *hlsl_text, unsigned len, const char *entry, const char *profile,
  Tab<uint32_t> &shader_bin, String &out_err)
{
  enableBindless = false;
  bool enableFp16 = str_ends_with_c(profile, "_half");
  CompileResult result =
    compileShaderSpirV(hlsl_text, profile, entry, false, enableFp16, false, true, 4096, "nodeBasedShader", CompilerMode::DEFAULT, 0);
  if (result.bytecode.empty())
    return false;

  shader_bin.assign((result.bytecode.size() + sizeof(uint32_t) - 1) / sizeof(uint32_t), 0);
  memcpy(shader_bin.data(), result.bytecode.data(), result.bytecode.size());
  if (!result.errors.empty())
    out_err = result.errors.c_str();
  return true;
}

DLL_EXPORT bool compile_compute_shader_spirv_bindless(const char *hlsl_text, unsigned len, const char *entry, const char *profile,
  Tab<uint32_t> &shader_bin, String &out_err)
{
  enableBindless = true;
  bool enableFp16 = str_ends_with_c(profile, "_half");
  CompileResult result =
    compileShaderSpirV(hlsl_text, profile, entry, false, enableFp16, false, true, 4096, "nodeBasedShader", CompilerMode::DEFAULT, 0);
  if (result.bytecode.empty())
    return false;

  shader_bin.assign((result.bytecode.size() + sizeof(uint32_t) - 1) / sizeof(uint32_t), 0);
  memcpy(shader_bin.data(), result.bytecode.data(), result.bytecode.size());
  if (!result.errors.empty())
    out_err = result.errors.c_str();
  return true;
}