// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_span.h>
#include <EASTL/string.h>

namespace shader_blob_disasm
{

enum class Target
{
  DX11,
  DX12,
  SPIRV,
  METAL,
  PS4,
  PS5,

  COUNT
};

inline constexpr char const *TARGET_NAMES[size_t(Target::COUNT)] = {
  "dx11",
  "dx12",
  "spirv",
  "metal",
  "ps4",
  "ps5",
};

eastl::string disassembleShaderBlob(dag::ConstSpan<uint8_t> bytecode, dag::ConstSpan<uint8_t> metadata, Target target);

} // namespace shader_blob_disasm
