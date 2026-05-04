// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "include/shaderBlobDisassembler/disasm.h"

#include <generic/dag_span.h>
#include <debug/dag_assert.h>

#if _TARGET_PC_WIN

#if HAS_PS4_PS5_TRANSCODE
#include <asmShadersPS4.h>
#include <asmShaderPS5.h>
#endif

#include <asmShaderHLSL2Metal.h>
#include <asmShaderSpirV.h>
#include <asmShaderDXIL.h>
#include <asmShaders11.h>

#elif _TARGET_PC_MACOSX

#include <asmShaderHLSL2Metal.h>

#elif _TARGET_PC_LINUX

#include <asmShaderSpirV.h>

#else

#error "Shader blob disassembler is not supported on this platform"

#endif

namespace shader_blob_disasm
{

eastl::string disassembleShaderBlob(dag::ConstSpan<uint8_t> bytecode, dag::ConstSpan<uint8_t> metadata, Target target)
{
  // @TODO: better error reporting?
#if !_TARGET_PC_WIN || !HAS_PS4_PS5_TRANSCODE
  G_ASSERT_RETURN(target != Target::PS4, {});
  G_ASSERT_RETURN(target != Target::PS5, {});
#endif

#if _TARGET_PC_MACOSX
  G_ASSERT_RETURN(target == Target::METAL, {});
#elif _TARGET_PC_LINUX
  G_ASSERT_RETURN(target == Target::SPIRV, {});
#endif

  switch (target)
  {
#if _TARGET_PC_WIN || _TARGET_PC_LINUX
    case Target::SPIRV: return disassembleShaderSpirV(bytecode, metadata);
#endif
#if _TARGET_PC_WIN || _TARGET_PC_MACOSX
    case Target::METAL: return disassembleShaderMetal(bytecode, metadata);
#endif

    default: G_ASSERTF_RETURN(0, {}, "Shader blob disasm for %s is not supported yet", TARGET_NAMES[size_t(target)]); break;
  }

  return {};
}

} // namespace shader_blob_disasm
