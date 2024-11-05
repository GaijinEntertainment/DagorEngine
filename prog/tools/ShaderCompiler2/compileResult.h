// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

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
  eastl::vector<uint8_t> bytecode;
  eastl::string disassembly;
  eastl::string errors;
  ComputeShaderInfo computeShaderInfo;
};
