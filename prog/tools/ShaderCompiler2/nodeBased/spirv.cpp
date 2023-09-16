#include "asmShaderSpirV.h"
#include <util/dag_string.h>
#include <3d/dag_drv3d_pc.h>

bool enableBindless = false;

bool compile_compute_shader_spirv(const char *hlsl_text, unsigned len, const char *entry, const char *profile,
  Tab<uint32_t> &shader_bin, String &out_err)
{
  enableBindless = false;
  CompileResult result =
    compileShaderSpirV(hlsl_text, profile, entry, false, false, true, 4096, 0, "nodeBasedShader", CompilerMode::DEFAULT);
  if (result.bytecode.empty())
    return false;

  shader_bin.assign((result.bytecode.size() + sizeof(uint32_t) - 1) / sizeof(uint32_t), 0);
  memcpy(shader_bin.data(), result.bytecode.data(), result.bytecode.size());
  if (!result.errors.empty())
    out_err = result.errors.c_str();
  return true;
}

bool compile_compute_shader_spirv_bindless(const char *hlsl_text, unsigned len, const char *entry, const char *profile,
  Tab<uint32_t> &shader_bin, String &out_err)
{
  enableBindless = true;
  CompileResult result =
    compileShaderSpirV(hlsl_text, profile, entry, false, false, true, 4096, 0, "nodeBasedShader", CompilerMode::DEFAULT);
  if (result.bytecode.empty())
    return false;

  shader_bin.assign((result.bytecode.size() + sizeof(uint32_t) - 1) / sizeof(uint32_t), 0);
  memcpy(shader_bin.data(), result.bytecode.data(), result.bytecode.size());
  if (!result.errors.empty())
    out_err = result.errors.c_str();
  return true;
}