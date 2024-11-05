// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_platform_pc.h>
#include <util/dag_string.h>
#include <asmShaderDXIL.h>

#include "../DebugLevel.h"
#include "defines.h"

extern DebugLevel hlslDebugLevel;
extern bool hlslEmbedSource;

namespace
{
static constexpr bool useScarlettWaveSize32 = false;
bool compile_compute_shader(const char *hlsl_text, unsigned len, const char *entry, const char *profile, Tab<uint32_t> &shader_bin,
  String &out_err, dx12::dxil::Platform platform)
{
  // having a len param and then possibly with invalid value is not nice...
  if (len & 0x80000000) // possibly a negative number, recount to be sure
    len = static_cast<unsigned>(strlen(hlsl_text));

  CompileResult result = dx12::dxil::compileShader(make_span(hlsl_text, len), profile, entry, false, false, false, false, true, false,
    nullptr, 4096, "", platform, useScarlettWaveSize32, false, hlslDebugLevel, hlslEmbedSource);
  if (result.bytecode.empty())
    return false;

  shader_bin.assign((result.bytecode.size() + sizeof(uint32_t) - 1) / sizeof(uint32_t), 0);
  memcpy(shader_bin.data(), result.bytecode.data(), result.bytecode.size());
  if (!result.errors.empty())
    out_err = result.errors.c_str();
  return true;
}
} // namespace

DLL_EXPORT bool compile_compute_shader_dx12(const char *hlsl_text, unsigned len, const char *entry, const char *profile,
  Tab<uint32_t> &shader_bin, String &out_err)
{
  return compile_compute_shader(hlsl_text, len, entry, profile, shader_bin, out_err, dx12::dxil::Platform::PC);
}

DLL_EXPORT bool compile_compute_shader_dx12_xbox_one(const char *hlsl_text, unsigned len, const char *entry, const char *profile,
  Tab<uint32_t> &shader_bin, String &out_err)
{
  return compile_compute_shader(hlsl_text, len, entry, profile, shader_bin, out_err, dx12::dxil::Platform::XBOX_ONE);
}

DLL_EXPORT bool compile_compute_shader_dx12_xbox_scarlett(const char *hlsl_text, unsigned len, const char *entry, const char *profile,
  Tab<uint32_t> &shader_bin, String &out_err)
{
  return compile_compute_shader(hlsl_text, len, entry, profile, shader_bin, out_err, dx12::dxil::Platform::XBOX_SCARLETT);
}