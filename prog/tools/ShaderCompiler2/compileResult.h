#pragma once

#include <EASTL/vector.h>
#include <EASTL/string.h>

struct ComputeShaderInfo
{
  uint32_t threadGroupSizeX = 0;
  uint32_t threadGroupSizeY = 0;
  uint32_t threadGroupSizeZ = 0;
};

struct CompileResult
{
  eastl::vector<uint8_t> bytecode;
  eastl::string disassembly;
  eastl::string errors;
  ComputeShaderInfo computeShaderInfo;
};
