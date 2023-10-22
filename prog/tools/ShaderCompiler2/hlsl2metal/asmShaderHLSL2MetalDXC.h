#pragma once

#include "../compileResult.h"

CompileResult compileShaderMetalDXC(const char *source, const char *profile, const char *entry, bool need_disasm, bool skipValidation,
  bool optimize, int max_constants_no, int bones_const_used, const char *shader_name, bool use_ios_token, bool use_binary_msl,
  uint64_t shader_variant_hash);
