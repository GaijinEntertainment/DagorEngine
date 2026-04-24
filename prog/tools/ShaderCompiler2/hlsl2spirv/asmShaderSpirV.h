// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "../compileResult.h"
#include <spirv/compiler_dxc.h>
#include <generic/dag_span.h>

CompileResult compileShaderSpirV(const spirv::DXCContext *dxc_ctx, const char *source, const char *profile, const char *entry,
  bool need_disasm, bool hlsl2021, bool enable_fp16, bool skipValidation, bool optimize, int max_constants_no, const char *shader_name,
  uint64_t shader_variant_hash, bool enable_bindless, bool embed_debug_data, bool dump_spirv_only,
  bool validate_global_consts_offset_order);

eastl::string disassembleShaderSpirV(dag::ConstSpan<uint8_t> bytecode, dag::ConstSpan<uint8_t> metadata);
