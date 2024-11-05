// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "../DebugLevel.h"
#include "../compileResult.h"

CompileResult compileShaderDX11(const char *shaderName, const char *source, const char **args, const char *profile, const char *entry,
  bool need_disasm, DebugLevel hlsl_debug_level, bool skip_validation, bool embed_source, unsigned flags, int max_constants_no);
