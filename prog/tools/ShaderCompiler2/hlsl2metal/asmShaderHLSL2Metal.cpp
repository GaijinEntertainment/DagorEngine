#include "asmShaderHLSL2Metal.h"

#include "asmShaderHLSL2MetalDXC.h"

CompileResult compileShaderMetal(const char *source, const char *profile, const char *entry, bool need_disasm, bool skipValidation,
  bool optimize, int max_constants_no, int bones_const_used, const char *shader_name, bool use_ios_token, bool use_binary_msl,
  uint64_t shader_variant_hash)
{
  return compileShaderMetalDXC(source, profile, entry, need_disasm, skipValidation, optimize, max_constants_no, bones_const_used,
    shader_name, use_ios_token, use_binary_msl, shader_variant_hash);
}
