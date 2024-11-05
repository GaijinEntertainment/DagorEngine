// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "../compileResult.h"

CompileResult compileShaderMetal(const char *source, const char *profile, const char *entry, bool need_disasm, bool enable_fp16,
  bool skipValidation, bool optimize, int max_constants_no, const char *shader_name, bool use_ios_token, bool use_binary_msl,
  uint64_t shader_variant_hash);
