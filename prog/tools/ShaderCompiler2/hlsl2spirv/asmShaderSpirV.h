#pragma once

#include "../compileResult.h"

enum class CompilerMode
{
  HLSLCC,
  DXC,
  DEFAULT = DXC
};
CompileResult compileShaderSpirV(const char *source, const char *profile, const char *entry, bool need_disasm, bool skipValidation,
  bool optimize, int max_constants_no, int bones_const_used, const char *shader_name, CompilerMode mode, uint64_t shader_variant_hash);
