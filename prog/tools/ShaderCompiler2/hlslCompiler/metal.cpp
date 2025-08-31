// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "asmShaderHLSL2Metal.h"
#include "defines.h"
#include <util/dag_string.h>
#include <drv/3d/dag_platform_pc.h>
#include <util/dag_strUtil.h>

extern bool enableBindless;

DLL_EXPORT bool compile_compute_shader_metal(const char *hlsl_text, unsigned len, const char *entry, const char *profile,
  Tab<uint32_t> &shader_bin, String &out_err)
{
  enableBindless = false;
  bool use_ios = false, use_binary = false;
  bool enableFp16 = str_ends_with_c(profile, "_half");

  dag::Vector<String> params;
  spirv::DXCContext *ctx = spirv::setupDXC("", params);
  CompileResult result = compileShaderMetal(ctx, hlsl_text, profile, entry, false, false, enableFp16, false, true, 4096,
    "nodeBasedShader", use_ios, use_binary, 0, false);
  if (!result.errors.empty())
    out_err.aprintf(0, "%s\n", result.errors.c_str());
  if (result.bytecode.empty())
  {
    spirv::shutdownDXC(ctx);
    return false;
  }

  shader_bin.assign((result.bytecode.size() + sizeof(uint32_t) - 1) / sizeof(uint32_t), 0);
  memcpy(shader_bin.data(), result.bytecode.data(), result.bytecode.size());

  spirv::shutdownDXC(ctx);
  return true;
}

DLL_EXPORT bool compile_compute_shader_metal_bindless(const char *hlsl_text, unsigned len, const char *entry, const char *profile,
  Tab<uint32_t> &shader_bin, String &out_err)
{
  enableBindless = true;
  bool use_ios = false, use_binary = false;
  bool enableFp16 = str_ends_with_c(profile, "_half");

  dag::Vector<String> params;
  spirv::DXCContext *ctx = spirv::setupDXC("", params);
  CompileResult result = compileShaderMetal(ctx, hlsl_text, profile, entry, false, false, enableFp16, false, true, 4096,
    "nodeBasedShader", use_ios, use_binary, 0, true);
  if (!result.errors.empty())
    out_err.aprintf(0, "%s\n", result.errors.c_str());
  if (result.bytecode.empty())
  {
    spirv::shutdownDXC(ctx);
    return false;
  }

  shader_bin.assign((result.bytecode.size() + sizeof(uint32_t) - 1) / sizeof(uint32_t), 0);
  memcpy(shader_bin.data(), result.bytecode.data(), result.bytecode.size());

  spirv::shutdownDXC(ctx);
  return true;
}
