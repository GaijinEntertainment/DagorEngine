// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "asmShaderSpirV.h"
#include "../fast_isalnum.h"
#include "../debugSpitfile.h"
#include "HLSL2SpirVCommon.h"
#include "pragmaScanner.h"

#include <ioSys/dag_memIo.h>
#include <ioSys/dag_zlibIo.h>
#include <osApiWrappers/dag_localConv.h>

#include <util/dag_globDef.h>
#include <util/dag_stdint.h>
#include <util/dag_string.h>

#include <debug/dag_debug.h>

#include <spirv/compiler.h>
#include <supp/dag_comPtr.h>

#if _TARGET_PC_WIN
#include <D3Dcompiler.h>
#endif

#include <SPIRV/disassemble.h>
#include <spirv.hpp>

#include <smolv.h>

#include <hlslcc.h>

#include <spirv-tools/libspirv.hpp>
#include <spirv-tools/optimizer.hpp>

#include <string_view>
#include <algorithm>
#include <fstream>
#include <regExp/regExp.h>

#include "../DebugLevel.h"
#include <EASTL/unique_ptr.h>

using namespace std;

static void fix_varaible_names(String &glsl) { glsl.replaceAll("__", "_"); }

eastl::vector<uint8_t> get_SpirV_bytecode(const Tab<spirv::ChunkHeader> &chunks, const Tab<uint8_t> &chunk_store,
  bool write_compressed = false)
{
  DynamicMemGeneralSaveCB mcwr(tmpmem, 0, 128 << 10);
  mcwr.writeInt(write_compressed ? spirv::SPIR_V_BLOB_IDENT : spirv::SPIR_V_BLOB_IDENT_UNCOMPRESSED);
  mcwr.beginBlock();
  if (write_compressed)
  {
    ZlibSaveCB z_cwr(mcwr, ZlibSaveCB::CL_BestCompression);
    z_cwr.writeTab(chunks);
    z_cwr.writeTab(chunk_store);
    z_cwr.finish();
  }
  else
  {
    mcwr.writeTab(chunks);
    mcwr.writeTab(chunk_store);
  }
  mcwr.endBlock();
  mcwr.alignOnDword(mcwr.size());

  eastl::vector<uint8_t> result((const uint8_t *)mcwr.data(), (const uint8_t *)mcwr.data() + mcwr.size());
  return result;
}

ostream &operator<<(ostream &os, spv_message_level_t error_level)
{
  switch (error_level)
  {
    case SPV_MSG_FATAL: os << "fatal"; break;
    case SPV_MSG_INTERNAL_ERROR: os << "internal error"; break;
    case SPV_MSG_ERROR: os << "error"; break;
    case SPV_MSG_WARNING: os << "warning"; break;
    case SPV_MSG_INFO: os << "info"; break;
    case SPV_MSG_DEBUG: os << "debug"; break;
  }
  return os;
}

ostream &operator<<(ostream &os, const spv_position_t &position)
{
  os << position.line << ", " << position.column << " (" << position.index << ")";
  return os;
}

static constexpr uint32_t B_REGISTER_SET = 0;
static constexpr uint32_t T_REGISTER_SET = 1;
static constexpr uint32_t U_REGISTER_SET = 2;

class GLSLCrossDependencyDataVulkan final : public GLSLCrossDependencyDataInterface
{
public:
  struct InterpolationInfo
  {
    uint32_t reg;
    INTERPOLATION_MODE mode;
  };
  vector<InterpolationInfo> interpolationData;
  TESSELLATOR_PARTITIONING tessPartitioning = TESSELLATOR_PARTITIONING_UNDEFINED;
  TESSELLATOR_OUTPUT_PRIMITIVE tessOutPrimitive = TESSELLATOR_OUTPUT_TRIANGLE_CW;
  uint32_t inputsCounter = 0;
  uint32_t outputCounter = 0;

  GLSLCrossDependencyDataVulkan() = default;

  void FragmentColorTarget(const char *name, uint32_t location, uint32_t index) override {}

  uint32_t GetVaryingLocation(const string &name, SHADER_TYPE eShaderType, bool isInput) override
  {
    if (isInput)
      return inputsCounter++;
    if (eShaderType != PIXEL_SHADER)
      return outputCounter++;
    return -1;
  }
  VulkanResourceBinding GetVulkanResourceBinding(string &name, SHADER_TYPE eShaderType, const ResourceBinding &binding,
    bool isDepthSampler, bool allocRoomForCounter = false, uint32_t preferredSet = 0, uint32_t constBufferSize = 0) override
  {
    // still need to put the bindings in separate sets to produce valid vulkan glsl
    uint32_t registerId = -1;
    if (binding.eType == RTYPE_CBUFFER)
    {
      registerId = B_REGISTER_SET;
    }
    else if ((binding.eType == RTYPE_TEXTURE) || (binding.eType == RTYPE_SAMPLER) || (binding.eType == RTYPE_STRUCTURED) ||
             (binding.eType == RTYPE_BYTEADDRESS))
    {
      registerId = T_REGISTER_SET;
    }
    else if ((binding.eType == RTYPE_UAV_RWTYPED) || (binding.eType == RTYPE_UAV_RWSTRUCTURED) ||
             (binding.eType == RTYPE_UAV_RWBYTEADDRESS) || (binding.eType == RTYPE_UAV_APPEND_STRUCTURED) ||
             (binding.eType == RTYPE_UAV_CONSUME_STRUCTURED) || (binding.eType == RTYPE_UAV_RWSTRUCTURED_WITH_COUNTER))
    {
      registerId = U_REGISTER_SET;
    }

    return {registerId, binding.ui32BindPoint};
  }
  INTERPOLATION_MODE GetInterpolationMode(uint32_t regNo) override
  {
    auto at =
      find_if(begin(interpolationData), end(interpolationData), [=](const InterpolationInfo &info) { return info.reg == regNo; });
    if (at != end(interpolationData))
      return at->mode;
    return INTERPOLATION_UNDEFINED;
  }
  void SetInterpolationMode(uint32_t regNo, INTERPOLATION_MODE mode) override
  {
    auto at =
      find_if(begin(interpolationData), end(interpolationData), [=](const InterpolationInfo &info) { return info.reg == regNo; });
    if (at == end(interpolationData))
      interpolationData.emplace_back(InterpolationInfo{regNo, mode});
  }
  void ClearCrossDependencyData() override {}
  void setTessPartitioning(TESSELLATOR_PARTITIONING tp) override { tessPartitioning = tp; }
  TESSELLATOR_PARTITIONING getTessPartitioning() override { return tessPartitioning; }
  void setTessOutPrimitive(TESSELLATOR_OUTPUT_PRIMITIVE top) override { tessOutPrimitive = top; }
  TESSELLATOR_OUTPUT_PRIMITIVE getTessOutPrimitive() override { return tessOutPrimitive; }
  uint32_t getProgramStagesMask() override { return PS_FLAG_VERTEX_SHADER | PS_FLAG_PIXEL_SHADER; }
};

static const EShLanguage EShLangInvalid = EShLangCount;
static EShLanguage profile_to_stage(const char *profile)
{
  switch (profile[0])
  {
    case 'c': return EShLangCompute;
    case 'v': return EShLangVertex;
    case 'p': return EShLangFragment;
    case 'g': return EShLangGeometry;
    case 'd': return EShLangTessEvaluation;
    case 'h': return EShLangTessControl;
    default: return EShLangInvalid;
  }
}

#if _TARGET_PC_WIN
string disassemble_dxbc(ComPtr<ID3DBlob> &blob)
{
  stringstream result;

  ComPtr<ID3DBlob> dasm;
  D3DDisassemble(blob->GetBufferPointer(), blob->GetBufferSize(), 0, NULL, &dasm);
  if (dasm)
  {
    result << reinterpret_cast<const char *>(dasm->GetBufferPointer());
  }

  return result.str();
}
#endif

CompilerMode detectMode(const char *source, CompilerMode mode)
{
  PragmaScanner scanner{source};
  while (auto pragma = scanner())
  {
    using namespace std::string_view_literals;
    auto from = pragma.tokens();

    if (*from == "spir-v"sv)
    {
      ++from;
      if (*from == "compiler"sv)
      {
        ++from;
        if (*from == "hlslcc"sv)
        {
          mode = CompilerMode::HLSLCC;
        }
        else if (*from == "dxc"sv)
        {
          mode = CompilerMode::DXC;
        }
      }
    }
  }
  return mode;
}

bool useBaseVertexPatch(const char *source)
{
  PragmaScanner scanner{source};
  while (auto pragma = scanner())
  {
    using namespace std::string_view_literals;
    auto from = pragma.tokens();

    if (*from == "spir-v"sv)
    {
      ++from;
      if (*from == "no-base-vertex"sv)
        return false;
    }
  }
  return true;
}

CompileResult compileShaderSpirV(const spirv::DXCContext *dxc_ctx, const char *source, const char *profile, const char *entry,
  bool need_disasm, bool hlsl2021, bool enable_fp16, bool skipValidation, bool optimize, int max_constants_no, const char *shader_name,
  CompilerMode mode, uint64_t shader_variant_hash, bool enable_bindless, bool embed_debug_data, bool dump_spirv_only)
{
  CompileResult result;

  if (enable_bindless)
  {
    if (mode != CompilerMode::DXC)
    {
      result.errors.sprintf("Bindless resource are only supported with DXC compiler backend.\n");
      return result;
    }
  }
  else
  {
#if _TARGET_PC_WIN
    mode = detectMode(source, mode);
#endif
  }
#if !_TARGET_PC_WIN
  if (mode != CompilerMode::DXC)
  {
    result.errors.sprintf("HLSLCC available only on windows.");
    return result;
  }
#endif
  // just to have it for debugging
  (void)need_disasm;

  // test_dxc(source, profile, entry);

  const EShLanguage targetStage = profile_to_stage(profile);
  if (targetStage == EShLangInvalid)
  {
    result.errors.sprintf("unknown target profile %s", profile);
    return result;
  }
  if (enable_fp16 && mode != CompilerMode::DXC)
  {
    result.errors.sprintf("Profiles with half specialization are currently suppored only by DXC path.\n"
                          "For switching to DXC please use #pragma spir-v compiler dxc\n");
    return result;
  }
  if (hlsl2021 && mode != CompilerMode::DXC)
  {
    result.errors.sprintf("HLSL21 is currently suppored only by DXC path.\n"
                          "For switching to DXC please use #pragma spir-v compiler dxc\n");
    return result;
  }

#if DAGOR_DBGLEVEL > 1
  optimize = false;
#endif

  spirv::ShaderHeader header = {};
  Tab<spirv::ChunkHeader> chunks;
  Tab<uint8_t> chunkStore;
  std::vector<unsigned int> spirv;

  if (embed_debug_data)
    add_chunk(chunks, chunkStore, spirv::ChunkType::UNPROCESSED_HLSL, 0, source, static_cast<uint32_t>(strlen(source)));

  if (CompilerMode::HLSLCC == mode)
  {
#if _TARGET_PC_WIN
    String shaderCode;
    ComPtr<ID3DBlob> byteCode;
    {
      ComPtr<ID3DBlob> errors;

      if (embed_debug_data)
        add_chunk(chunks, chunkStore, spirv::ChunkType::UNPROCESSED_HLSL, 0, source, static_cast<uint32_t>(strlen(source)));

      const D3D_SHADER_MACRO macros[] = //
        {{"SHADER_COMPILER_HLSLCC", "1"}, {"half", "float"}, {"half1", "float1"}, {"half2", "float2"}, {"half3", "float3"},
          {"half4", "float4"}, {"HW_VERTEX_ID", "uint vertexId: SV_VertexID;"},
          {"HW_BASE_VERTEX_ID", "error! not supported on this compiler/API"}, {"HW_BASE_VERTEX_ID_OPTIONAL", ""}, {nullptr, nullptr}};

      HRESULT hr = D3DCompile(source, strlen(source), NULL, macros, NULL, entry, profile,
        (skipValidation ? D3DCOMPILE_SKIP_VALIDATION : 0) |
          // all other optimization levels may crash hlslcc
          D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR,
        0, &byteCode, &errors);

      if (FAILED(hr))
      {
        if (errors)
          result.errors.sprintf("D3DCompile failed:\n%s", errors->GetBufferPointer());
        else
          result.errors.sprintf("D3DCompile failed with %u and no error log", hr);
        return result;
      }

      GlExtensions ext = {};

      HLSLccSamplerPrecisionInfo spci;
      HLSLccReflection rc;
      GLSLCrossDependencyDataVulkan mapping;
      GLSLShader glslShader;

      stringstream infoLog;

      if (embed_debug_data)
      {
        auto dxbcDisas = disassemble_dxbc(byteCode);
        add_chunk(chunks, chunkStore, spirv::ChunkType::HLSL_DISASSEMBLY, 0, dxbcDisas.data(),
          static_cast<uint32_t>(dxbcDisas.length()));
      }

      int errorCode = TranslateHLSLFromMem((const char *)byteCode->GetBufferPointer(),
        HLSLCC_FLAG_VULKAN_BINDINGS | HLSLCC_FLAG_SEPARABLE_SHADER_OBJECTS | HLSLCC_FLAG_UNIFORM_BUFFER_OBJECT |
          HLSLCC_FLAG_GLSL_EXPLICIT_UNIFORM_OFFSET | HLSLCC_FLAG_GLSL_EMBEDED_ATOMIC_COUNTER | HLSLCC_FLAG_GLSL_CHECK_DATA_LAYOUT |
          HLSLCC_FLAG_MAP_CONST_BUFFER_TO_REGISTER_ARRAY,
        LANG_440, &ext, &mapping, spci, rc, infoLog, &glslShader);

      if (!errorCode)
      {
        if (!infoLog.str().empty())
          result.errors.sprintf("TranslateHLSLFromMem failed!\n%s", infoLog.str().c_str());
        else
          result.errors.sprintf("TranslateHLSLFromMem failed!");
        return result;
      }

      shaderCode = glslShader.sourceCode.c_str();
      if (need_disasm)
        result.disassembly = shaderCode;
    }

    fix_varaible_names(shaderCode);
    if (embed_debug_data)
      add_chunk(chunks, chunkStore, spirv::ChunkType::RECONSTRUCTED_GLSL, 0, shaderCode.data(), shaderCode.length());

    const spirv::CompileFlags glslCompileFlags = optimize ? spirv::CompileFlags::NONE : spirv::CompileFlags::DEBUG_BUILD;
    const char *ptr = shaderCode.str();
    spirv::CompileToSpirVResult finalSpirV = spirv::compileGLSL(make_span(&ptr, 1), targetStage, glslCompileFlags);
    if (finalSpirV.byteCode.empty())
    {
      eastl::string errorMessage;
      for (auto &&msg : finalSpirV.infoLog)
      {
        errorMessage += msg;
        errorMessage += '\n';
      }
      result.errors.sprintf("GLSL to Spir-V compilation failed:\n %s \n GLSL source: %s", errorMessage.c_str(), ptr);
      return result;
    }
    else if (!finalSpirV.infoLog.empty())
    {
      for (auto &&msg : finalSpirV.infoLog)
        debug(msg.c_str());
    }

    spirv = eastl::move(finalSpirV.byteCode);
    header = finalSpirV.header;

    if (profile[0] == 'c')
    {
      ComPtr<ID3D11ShaderReflection> reflector;
      HRESULT hr = D3DReflect(byteCode->GetBufferPointer(), byteCode->GetBufferSize(), IID_PPV_ARGS(&reflector));
      G_ASSERTF(SUCCEEDED(hr), "Unable to reflect the compute shader");
      reflector->GetThreadGroupSize(&result.computeShaderInfo.threadGroupSizeX, &result.computeShaderInfo.threadGroupSizeY,
        &result.computeShaderInfo.threadGroupSizeZ);
    }

    // run a spir-v tools optimize pass over it to have overall better shaders
    if (optimize)
    {
      spvtools::Optimizer optimizer{SPV_ENV_VULKAN_1_0};
      stringstream infoStream;
      optimizer.SetMessageConsumer([&infoStream](spv_message_level_t level, const char * /* source */, const spv_position_t &position,
                                     const char *message) //
        {
          infoStream << "[" << level << "][" << position << "] " << message << endl;
          ;
        });

      optimizer.RegisterPerformancePasses();

      vector<uint32_t> optimized;
      if (optimizer.Run(spirv.data(), spirv.size(), &optimized))
      {
        swap(optimized, spirv);
      }
      else
      {
        debug("Spir-V optimization failed, using unoptimized version");
      }
      string infoMessage = infoStream.str();
      if (!infoMessage.empty())
      {
        debug("Spir-V Optimizer log: %s", infoMessage.c_str());
      }
    }
#endif
  }
  else
  {
    string codeCopy(source);

    // code preprocess to fix SV_VertexID disparity between DX and vulkan
    if (useBaseVertexPatch(source))
    {
      if (!fix_vertex_id_for_DXC(codeCopy, result))
        return result;
    }

    eastl::vector<eastl::string_view> disabledSpirvOptims = scanDisabledSpirvOptimizations(source);

    string macros =
      "#define SHADER_COMPILER_DXC 1\n"
      "#define HW_VERTEX_ID uint vertexId: SV_VertexID;\n"
      "#define HW_BASE_VERTEX_ID [[vk::builtin(\"BaseVertex\")]] uint baseVertexId : DXC_SPIRV_BASE_VERTEX_ID;\n"
      "#define HW_BASE_VERTEX_ID_OPTIONAL [[vk::builtin(\"BaseVertex\")]] uint baseVertexId : DXC_SPIRV_BASE_VERTEX_ID;\n";
    if (enable_bindless)
    {
      macros += "#define BINDLESS_TEXTURE_SET_META_ID " + std::to_string(spirv::bindless::TEXTURE_DESCRIPTOR_SET_META_INDEX) + "\n";
      macros += "#define BINDLESS_SAMPLER_SET_META_ID " + std::to_string(spirv::bindless::SAMPLER_DESCRIPTOR_SET_META_INDEX) + "\n";
      macros += "#define BINDLESS_BUFFER_SET_META_ID " + std::to_string(spirv::bindless::BUFFER_DESCRIPTOR_SET_META_INDEX) + "\n";
    }
    if (enable_fp16)
    {
      macros += "#define SHADER_COMPILER_FP16_ENABLED 1\n";
    }
    else
    {
      // there is a bug(?) in DXC: it can't map half[] -> float[] correctly with disabled 16-bit types flag
      macros += "#define half float\n"
                "#define half1 float1\n"
                "#define half2 float2\n"
                "#define half3 float3\n"
                "#define half4 float4\n";
    }
    codeCopy = macros + codeCopy;

    auto sourceRange = make_span(codeCopy.c_str(), codeCopy.size());

    auto flags = enable_bindless ? spirv::CompileFlags::ENABLE_BINDLESS_SUPPORT : spirv::CompileFlags::NONE;
    flags |= enable_fp16 ? spirv::CompileFlags::ENABLE_HALFS : spirv::CompileFlags::NONE;
    flags |= hlsl2021 ? spirv::CompileFlags::ENABLE_HLSL21 : spirv::CompileFlags::NONE;

    auto finalSpirV = spirv::compileHLSL_DXC(dxc_ctx, sourceRange, entry, profile, flags, disabledSpirvOptims);
    spirv = eastl::move(finalSpirV.byteCode);
    header = finalSpirV.header;

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
      result.errors.sprintf("HLSL to Spir-V compilation failed:\n %s", flatLogString.c_str());
      return result;
    }
    else if (flatLogString.length())
      debug("%s", flatLogString.c_str());

    if (profile[0] == 'c')
    {
      result.computeShaderInfo.threadGroupSizeX = finalSpirV.computeShaderInfo.threadGroupSizeX;
      result.computeShaderInfo.threadGroupSizeY = finalSpirV.computeShaderInfo.threadGroupSizeY;
      result.computeShaderInfo.threadGroupSizeZ = finalSpirV.computeShaderInfo.threadGroupSizeZ;
    }
  }
#if 0
  if (!skipValidation)
  {
    spvtools::SpirvTools tools{SPV_ENV_VULKAN_1_0};

    stringstream infoStream;
    tools.SetMessageConsumer([&infoStream](spv_message_level_t level,
                                           const char * /* source */,
                                           const spv_position_t &position,
                                           const char *message) //
                             {
                               infoStream << "[" << level << "][" << position << "] " << message
                                          << endl;
                               ;
                             });

    bool passed = tools.Validate(spirv);
    string infoMessage = infoStream.str();
    if (!passed)
    {
      debug(infoMessage.c_str());
      string spirvDisas;
      // only indent, friendly names are sometimes not helpful
      if (tools.Disassemble(spirv, &spirvDisas, SPV_BINARY_TO_TEXT_OPTION_INDENT))
      {
        debug(spirvDisas.c_str());
      }
      eastl::string flatInfoLog;
      for (auto && info : finalSpirV.infoLog)
      {
        flatInfoLog += info;
        flatInfoLog += "\n";
      }
      *errmsg = make_error_message("Spir-V validation failed, log: %s\n%s\n%s",
        infoMessage.c_str(), spirvDisas.c_str(), flatInfoLog.c_str());
      return nullptr;
    }

    if (!infoMessage.empty())
    {
      debug("Spir-V Validate log: %s", infoMessage.c_str());
    }
  }
#endif

  if (embed_debug_data)
  {
    spvtools::SpirvTools tools{SPV_ENV_VULKAN_1_0};

    stringstream infoStream;
    tools.SetMessageConsumer([&infoStream](spv_message_level_t level, const char * /* source */, const spv_position_t &position,
                               const char *message) //
      { infoStream << "[" << level << "][" << position << "] " << message << endl; });

    string spirvDisas;
    // only indent, friendly names are sometimes not helpful
    if (tools.Disassemble(spirv, &spirvDisas, SPV_BINARY_TO_TEXT_OPTION_INDENT))
    {
      add_chunk(chunks, chunkStore, spirv::ChunkType::SPIR_V_DISASSEMBLY, 0, spirvDisas.data(),
        static_cast<uint32_t>(spirvDisas.length()));
    }

    string infoMessage = infoStream.str();
    if (!infoMessage.empty())
    {
      debug("Spir-V Disassemble log: %s", infoMessage.c_str());
    }
  }

  if (debug_output_dir)
  {
    spvtools::SpirvTools tools{SPV_ENV_VULKAN_1_0};

    string spirvDisas;
    tools.Disassemble(spirv, &spirvDisas, SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES);

    const String name = String(-1, "%s_%.2s", (mode == CompilerMode::HLSLCC) ? "spirv_hlslcc" : "spirv_dxc", profile);
    if (!dump_spirv_only)
      spitfile(shader_name, entry, name, shader_variant_hash, (void *)spirvDisas.data(), (int)data_size(spirvDisas));
    spitfile(shader_name, entry, name + "_raw", shader_variant_hash, (void *)spirv.data(), (int)data_size(spirv));
  }

  header.verMagic = spirv::HEADER_MAGIC_VER;
  header.maxConstantCount = max_constants_no;
  header.bonesConstantsUsed = -1; // @TODO: remove

  smolv::ByteArray smol;
  smolv::Encode(spirv.data(), spirv.size() * sizeof(unsigned int), smol, 0);
  if (smol.empty())
  {
    debug("Smol-V compression failed, exporting uncompressed spir-v");
    add_chunk(chunks, chunkStore, spirv::ChunkType::SPIR_V, 0, spirv.data(), static_cast<uint32_t>(spirv.size()));
  }
  else
  {
    add_chunk(chunks, chunkStore, spirv::ChunkType::SMOL_V, 0, smol.data(), static_cast<uint32_t>(smol.size()));
  }

  add_chunk(chunks, chunkStore, spirv::ChunkType::SHADER_HEADER, 0, &header, 1);
  add_chunk(chunks, chunkStore, spirv::ChunkType::SHADER_NAME, 0, shader_name, static_cast<uint32_t>(strlen(shader_name)));

  result.bytecode = get_SpirV_bytecode(chunks, chunkStore);
  return result;
}