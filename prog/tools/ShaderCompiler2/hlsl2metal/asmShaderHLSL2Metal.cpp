// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "asmShaderHLSL2Metal.h"

#include "asmShaderHLSL2MetalDXC.h"

CompileResult compileShaderMetal(const spirv::DXCContext *dxc_ctx, const char *source, const char *profile, const char *entry,
  bool need_disasm, bool hlsl2021, bool enable_fp16, bool skipValidation, bool optimize, int max_constants_no, const char *shader_name,
  bool use_ios_token, bool use_binary_msl, uint64_t shader_variant_hash, bool enableBindless)
{
  return compileShaderMetalDXC(dxc_ctx, source, profile, entry, need_disasm, hlsl2021, enable_fp16, skipValidation, optimize,
    max_constants_no, shader_name, use_ios_token, use_binary_msl, shader_variant_hash, enableBindless);
}
