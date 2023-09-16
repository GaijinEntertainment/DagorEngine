#include "hlsl2spirvCompile.h"

#include <array>
#include <sstream>
#include <vector>
#include <unordered_map>

#include <spirv/compiler.h>
#include <spirv/compiled_meta_data.h>
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
  std::string macros =
    "#define SHADER_COMPILER_DXC 1\n"
    "#define HW_VERTEX_ID uint vertexId: SV_VertexID;\n"
    "#define HW_BASE_VERTEX_ID [[vk::builtin(\"BaseVertex\")]] uint baseVertexId : DXC_SPIRV_BASE_VERTEX_ID;\n"
    "#define HW_BASE_VERTEX_ID_OPTIONAL [[vk::builtin(\"BaseVertex\")]] uint baseVertexId : DXC_SPIRV_BASE_VERTEX_ID;\n";

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


static const std::array limits = {
  spirv::B_CONST_BUFFER_OFFSET,               //
  spirv::T_SAMPLED_IMAGE_OFFSET,              //
  spirv::T_SAMPLED_IMAGE_WITH_COMPARE_OFFSET, //
  spirv::T_BUFFER_SAMPLED_IMAGE_OFFSET,       //
  spirv::T_BUFFER_OFFSET,                     //
  spirv::T_INPUT_ATTACHMENT_OFFSET,           //
  spirv::U_IMAGE_OFFSET,                      //
  spirv::U_BUFFER_IMAGE_OFFSET,               //
  spirv::U_BUFFER_OFFSET,                     //
  spirv::U_BUFFER_WITH_COUNTER_OFFSET,        //
  spirv::T_ACCELERATION_STRUCTURE_OFFSET      //
};

RegisterData processRegisterIndex(eastl::span<uint8_t> indexes, uint8_t register_count)
{
  RegisterData data;

  std::unordered_map<uint32_t, std::vector<uint8_t> *> limitMapper = {
    {spirv::B_CONST_BUFFER_OFFSET, &data.bRegistersConstBuffer}, //
    // yes, next 3 reference same vector as textures in shader will be
    // listed the same way they are added to this vector
    {spirv::T_SAMPLED_IMAGE_OFFSET, &data.tRegistersForImages},              //
    {spirv::T_SAMPLED_IMAGE_WITH_COMPARE_OFFSET, &data.tRegistersForImages}, //
    {spirv::T_BUFFER_SAMPLED_IMAGE_OFFSET, &data.tRegistersForImages},       //
    //
    {spirv::T_BUFFER_OFFSET, &data.tRegistersBuffer},                               //
    {spirv::T_INPUT_ATTACHMENT_OFFSET, &data.tRegistersInputAttachement},           //
    {spirv::U_IMAGE_OFFSET, &data.uRegistersImage},                                 //
    {spirv::U_BUFFER_IMAGE_OFFSET, &data.uRegistersBufferImage},                    //
    {spirv::U_BUFFER_OFFSET, &data.uRegistersBuffer},                               //
    {spirv::U_BUFFER_WITH_COUNTER_OFFSET, &data.uRegistersBufferWithCounter},       //
    {spirv::T_ACCELERATION_STRUCTURE_OFFSET, &data.tRegistersAccelerationStructure} //
  };


  for (int i = 0; i < register_count; i++)
  {
    for (int j = 0; j < limits.size(); j++)
    {
      if (j == limits.size() - 1)
      {
        const uint8_t limit = limits[j];
        limitMapper[limit]->push_back(indexes[i] - limit);
        break;
      }

      if (indexes[i] >= limits[j])
        continue;

      const uint8_t limit = limits[j - 1];
      limitMapper[limit]->push_back(indexes[i] - limit);
      break;
    }
  }
  return data;
}

Hlsl2SpirvResult hlsl2spirv(const char *source, const char *profile, const char *entry, bool skip_validation,
  CompileResult &compile_result)
{
  Hlsl2SpirvResult result;
  result.failed = true;

  std::string codeCopy(source);

  // code preprocess to fix SV_VertexID disparity between DX and vulkan/metal
  if (!fix_vertex_id_for_DXC(codeCopy, compile_result))
    return result;

  bool is_half = dd_stricmp("ps_5_0_half", profile) == 0 || dd_stricmp("vs_5_0_half", profile) == 0;
  applyD3DMacros(codeCopy, is_half);

  auto sourceRange = make_span(codeCopy.c_str(), codeCopy.size());
  auto finalSpirV = spirv::compileHLSL_DXC(sourceRange, entry, profile, spirv::CompileFlags::NONE);
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
      compile_result.errors.sprintf("HLSL to Spir-V compilation failed:\n %s", flatLogString.c_str());
      // result.failed == true
      return result;
    }
    else if (flatLogString.length())
      debug("%s", flatLogString.c_str());
  }

  if (!skip_validation)
  {
    spvtools::SpirvTools tools{SPV_ENV_UNIVERSAL_1_4};

    std::stringstream infoStream;
    tools.SetMessageConsumer([&infoStream](spv_message_level_t level, const char * /* source */, const spv_position_t &position,
                               const char *message) //
      { infoStream << "[" << level << "][" << position << "] " << message << std::endl; });

    bool passed = tools.Validate(spirv);
    std::string infoMessage = infoStream.str();
    debug(infoMessage.c_str());
    std::string spirvDisas;
    // only indent, friendly names are sometimes not helpful
    if (tools.Disassemble(spirv, &spirvDisas, SPV_BINARY_TO_TEXT_OPTION_INDENT))
    {
      debug(spirvDisas.c_str());
    }

    if (!passed)
    {
      eastl::string flatInfoLog;
      for (auto &&info : finalSpirV.infoLog)
      {
        flatInfoLog += info;
        flatInfoLog += "\n";
      }

      if (!infoMessage.empty())
      {
        debug("Spir-V Validate log: %s", infoMessage.c_str());
      }
    }
  }

  result.failed = false;
  result.computeInfo = {finalSpirV.computeShaderInfo.threadGroupSizeX, finalSpirV.computeShaderInfo.threadGroupSizeY,
    finalSpirV.computeShaderInfo.threadGroupSizeZ};
  result.registers = processRegisterIndex(finalSpirV.header.registerIndex, finalSpirV.header.registerCount);

  return result;
}