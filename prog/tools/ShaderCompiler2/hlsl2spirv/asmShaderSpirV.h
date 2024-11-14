// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "../compileResult.h"

enum class CompilerMode
{
  HLSLCC,
  DXC,
  DEFAULT = DXC
};
CompileResult compileShaderSpirV(const char *source, const char *profile, const char *entry, bool need_disasm, bool enable_fp16,
  bool skipValidation, bool optimize, int max_constants_no, const char *shader_name, CompilerMode mode, uint64_t shader_variant_hash,
  bool enable_bindless, bool embed_debug_data);
