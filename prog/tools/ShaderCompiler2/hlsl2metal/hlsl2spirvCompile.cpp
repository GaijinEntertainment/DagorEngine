// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hlsl2spirvCompile.h"

#include <array>
#include <sstream>
#include <vector>
#include <unordered_map>

#include <spirv/compiler.h>
#include <drv/shadersMetaData/spirv/compiled_meta_data.h>
#include <osApiWrappers/dag_localConv.h>

#include <debug/dag_debug.h>

#include <spirv.hpp>
#include <SPIRV/disassemble.h>
#include <vulkan/vulkan.h>

#include <spirv-tools/libspirv.hpp>
#include <spirv-tools/optimizer.hpp>

#include "../hlsl2spirv/HLSL2SpirVCommon.h"

#include <EASTL/span.h>

static std::ostream &operator<<(std::ostream &os, const spv_position_t &position)
{
  os << position.line << ", " << position.column << " (" << position.index << ")";
  return os;
}

void applyD3DMacros(std::string &view, bool should_use_half)
{
  std::string macros = "#define SHADER_COMPILER_DXC 1\n"
                       "#define HW_VERTEX_ID uint vertexId: SV_VertexID;\n"
                       "#define HW_BASE_VERTEX_ID error! not supported on this compiler/API\n"
                       "#define HW_BASE_VERTEX_ID_OPTIONAL \n"
                       "#define USE_VERTEX_ID_WITHOUT_BASE_OFFSET(input_struct) \n";

  if (should_use_half)
  {
    macros += "#define half min16float\n"
              "#define half1 min16float1\n"
              "#define half2 min16float2\n"
              "#define half3 min16float3\n"
              "#define half4 min16float4\n";
  }
  else
  {
    macros += "#define half float\n"
              "#define half1 float1\n"
              "#define half2 float2\n"
              "#define half3 float3\n"
              "#define half4 float4\n";
  }
  view = macros + view;
}

Hlsl2SpirvResult hlsl2spirv(const char *source, const char *profile, const char *entry, bool enable_fp16, bool skip_validation,
  CompileResult &compile_result)
{
  Hlsl2SpirvResult result;
  result.failed = true;

  std::string codeCopy(source);

  eastl::vector<eastl::string_view> disabledSpirvOptims = scanDisabledSpirvOptimizations(source);
  applyD3DMacros(codeCopy, enable_fp16);

  auto sourceRange = make_span(codeCopy.c_str(), codeCopy.size());
  auto finalSpirV = spirv::compileHLSL_DXC(sourceRange, entry, profile,
    spirv::CompileFlags::OUTPUT_REFLECTION | spirv::CompileFlags::OVERWRITE_DESCRIPTOR_SETS | spirv::CompileFlags::ENABLE_HALFS |
      spirv::CompileFlags::ENABLE_WAVE_INTRINSICS,
    disabledSpirvOptims);
  result.byteCode = finalSpirV.byteCode;
  auto &spirv = result.byteCode;

  {
    eastl::string flatLogString;
    bool errorOrWarningFound = false;
    for (auto &&msg : finalSpirV.infoLog)
    {
      flatLogString += "DXC_SPV: ";
      flatLogString += msg;
      flatLogString += "\n";
      errorOrWarningFound |= msg.compare(0, 8, "Warning:") == 0;
    }
    errorOrWarningFound |= spirv.empty(); // assume error will surely fail spirv dump generation

    if (errorOrWarningFound)
    {
      flatLogString += "DXC_SPV: Problematic shader dump:\n";
      flatLogString += "======= dump begin\n";
      flatLogString += codeCopy.c_str();
      flatLogString += "======= dump end\n";
    }

    if (spirv.empty())
    {
      compile_result.errors.sprintf("DXC HLSL to Spir-V compilation failed:\n %s", flatLogString.c_str());
      // result.failed == true
      return result;
    }
    else if (flatLogString.length() && errorOrWarningFound)
      debug("%s", flatLogString.c_str());
  }

  result.failed = false;
  result.computeInfo = {finalSpirV.computeShaderInfo.threadGroupSizeX, finalSpirV.computeShaderInfo.threadGroupSizeY,
    finalSpirV.computeShaderInfo.threadGroupSizeZ};
  result.reflection = finalSpirV.reflection;

  return result;
}
