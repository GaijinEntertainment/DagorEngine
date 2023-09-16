#include "asmShadersPS4.h"
#include <util/dag_string.h>
#include <3d/dag_drv3d_pc.h>

bool compile_compute_shader_pssl(const char *pssl_text, unsigned len, const char *entry, const char *profile,
  Tab<uint32_t> &shader_bin, String &out_err)
{
  len = static_cast<uint32_t>(strlen(pssl_text)) + 1;
  CompileResult result = compileShaderPS4(entry, pssl_text, len, profile, entry, 3, false, 0, 0, 0, nullptr, false, 4096, 0);
  if (result.bytecode.empty())
    return false;

  shader_bin.assign((result.bytecode.size() + sizeof(uint32_t) - 1) / sizeof(uint32_t), 0);
  memcpy(shader_bin.data(), result.bytecode.data(), result.bytecode.size());
  if (!result.errors.empty())
    out_err = result.errors.c_str();
  return true;
}