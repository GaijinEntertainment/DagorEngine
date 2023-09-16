#include "asmShaderHLSL2Metal.h"
#include <util/dag_string.h>
#include <3d/dag_drv3d_pc.h>

bool compile_compute_shader_metal(const char *hlsl_text, unsigned len, const char *entry, const char *profile,
  Tab<uint32_t> &shader_bin, String &out_err)
{
  bool use_ios = false, use_binary = false;
  CompileResult result =
    compileShaderMetal(hlsl_text, profile, entry, false, false, true, 4096, 0, "nodeBasedShader", use_ios, use_binary);
  if (!result.errors.empty())
    out_err = result.errors.c_str();
  if (result.bytecode.empty())
    return false;

  shader_bin.assign((result.bytecode.size() + sizeof(uint32_t) - 1) / sizeof(uint32_t), 0);
  memcpy(shader_bin.data(), result.bytecode.data(), result.bytecode.size());
  return true;
}
