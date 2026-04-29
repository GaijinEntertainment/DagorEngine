// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_span.h>

#include <EASTL/vector.h>
#include <EASTL/string.h>

struct ComputeShaderInfo
{
  uint32_t threadGroupSizeX = 0;
  uint32_t threadGroupSizeY = 0;
  uint32_t threadGroupSizeZ = 0;
#if _CROSS_TARGET_DX12
  bool scarlettWave32 = false;
#endif
};

struct CompileResult
{
  eastl::vector<uint8_t> metadata;
  eastl::vector<uint8_t> bytecode;
  eastl::string disassembly;
  eastl::string errors;
  eastl::string logs;
  ComputeShaderInfo computeShaderInfo;
};

struct ShaderStageData
{
  dag::ConstSpan<uint8_t> metadata;
  dag::ConstSpan<unsigned> bytecode;

  bool empty() const { return bytecode.empty() && metadata.empty(); }

  bool operator!=(const ShaderStageData &rhs) const
  {
    if (metadata.size() != rhs.metadata.size())
      return true;
    if (metadata.data() != rhs.metadata.data() && !mem_eq(metadata, rhs.metadata.data()))
      return true;

    if (bytecode.size() != rhs.bytecode.size())
      return true;
    if (bytecode.data() != rhs.bytecode.data() && !mem_eq(bytecode, rhs.bytecode.data()))
      return true;

    return false;
  }

  uint32_t type() const
  {
    uint32_t type = 0;
    if (metadata.size() >= 4)
      memcpy(&type, metadata.data(), sizeof(type));
    return type;
  }
};
