// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "asmShaderSpirV.h"
#include <sstream>
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

#include <spirv.hpp>

#include <smolv.h>

#include <spirv-tools/libspirv.hpp>
#include <spirv-tools/optimizer.hpp>

#include <string_view>
#include <algorithm>
#include <fstream>
#include <regExp/regExp.h>

#include "../DebugLevel.h"
#include <EASTL/unique_ptr.h>

using namespace std;

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

CompileResult compileShaderSpirV(const SpirVCompileInputs &inputs)
{
  CompileResult result;

  spirv::ShaderHeader header = {};
  Tab<spirv::ChunkHeader> chunks;
  Tab<uint8_t> chunkStore;
  std::vector<unsigned int> spirv;

  if (inputs.embedDebugData)
    add_chunk(chunks, chunkStore, spirv::ChunkType::UNPROCESSED_HLSL, 0, inputs.source, static_cast<uint32_t>(strlen(inputs.source)));

  string codeCopy(inputs.source);

  // code preprocess to fix SV_VertexID disparity between DX and vulkan
  if (useBaseVertexPatch(inputs.source))
  {
    if (!fix_vertex_id_for_DXC(codeCopy, result))
      return result;
  }

  eastl::vector<eastl::string_view> disabledSpirvOptims = scanDisabledSpirvOptimizations(inputs.source);

  string macros = "#define SHADER_COMPILER_DXC 1\n"
                  "#define HW_VERTEX_ID uint vertexId: SV_VertexID;\n"
                  "#define HW_BASE_VERTEX_ID [[vk::builtin(\"BaseVertex\")]] uint baseVertexId : DXC_SPIRV_BASE_VERTEX_ID;\n"
                  "#define HW_BASE_VERTEX_ID_OPTIONAL [[vk::builtin(\"BaseVertex\")]] uint baseVertexId : DXC_SPIRV_BASE_VERTEX_ID;\n";
  if (inputs.enableBindless)
  {
    macros += "#define BINDLESS_TEXTURE_SET_META_ID " + std::to_string(spirv::bindless::TEXTURE_DESCRIPTOR_SET_META_INDEX) + "\n";
    macros += "#define BINDLESS_SAMPLER_SET_META_ID " + std::to_string(spirv::bindless::SAMPLER_DESCRIPTOR_SET_META_INDEX) + "\n";
    macros += "#define BINDLESS_BUFFER_SET_META_ID " + std::to_string(spirv::bindless::BUFFER_DESCRIPTOR_SET_META_INDEX) + "\n";
  }

  if (inputs.enableFp16)
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

  // format for profile is *s_X_Y
  bool allowWaveIntrisics = strlen(inputs.profile) > 3 && inputs.profile[3] >= '6';
  if (allowWaveIntrisics)
  {
    macros += "#define WAVE_INTRINSICS 1\n";
  }
  codeCopy = macros + codeCopy;

  auto sourceRange = make_span(codeCopy.c_str(), codeCopy.size());

  auto flags = inputs.enableBindless ? spirv::CompileFlags::ENABLE_BINDLESS_SUPPORT : spirv::CompileFlags::NONE;
  flags |= inputs.enableFp16 ? spirv::CompileFlags::ENABLE_HALFS : spirv::CompileFlags::NONE;
  flags |= inputs.hlsl2021 ? spirv::CompileFlags::ENABLE_HLSL21 : spirv::CompileFlags::NONE;
  flags |= allowWaveIntrisics ? spirv::CompileFlags::ENABLE_WAVE_INTRINSICS : spirv::CompileFlags::NONE;
  flags |=
    inputs.validateGlobalConstsOffsetOrder ? spirv::CompileFlags::VALIDATE_GLOBAL_CONSTS_OFFSET_ORDER : spirv::CompileFlags::NONE;
  flags |= inputs.noConversionWarnings ? spirv::CompileFlags::NO_CONVERSION_WARNINGS : spirv::CompileFlags::NONE;
  flags |= inputs.useScalarLayout ? spirv::CompileFlags::USE_SCALAR_LAYOUT : spirv::CompileFlags::NONE;

  auto finalSpirV = spirv::compileHLSL_DXC(inputs.dxcCtx, sourceRange, inputs.entry, inputs.profile, inputs.implicitCbufRegCount,
    flags, disabledSpirvOptims);
  spirv = eastl::move(finalSpirV.byteCode);
  header = finalSpirV.header;

  eastl::string flatLogString;
  bool errorFound = false;
  bool warningFound = false;
  for (auto &&msg : finalSpirV.infoLog)
  {
    flatLogString += "DXC_SPV: ";
    flatLogString += msg;
    flatLogString += "\n";
    warningFound |= msg.compare(0, 8, "Warning:") == 0 || msg.find("warning: ") != eastl::string::npos;
  }
  errorFound |= spirv.empty(); // assume error will surely fail spirv dump generation

  if (errorFound || warningFound)
  {
    flatLogString += "DXC_SPV: Problematic shader dump:\n";
    flatLogString += "======= dump begin\n";
    flatLogString += codeCopy.c_str();
    flatLogString += "======= dump end\n";
  }

  if (errorFound || (inputs.warningsAsErrors && warningFound))
  {
    result.errors.sprintf("HLSL to Spir-V compilation failed:\n %s", flatLogString.c_str());
    return result;
  }
  else if (flatLogString.length())
    debug("%s", flatLogString.c_str());

  if (inputs.profile[0] == 'c')
  {
    result.computeShaderInfo.threadGroupSizeX = finalSpirV.computeShaderInfo.threadGroupSizeX;
    result.computeShaderInfo.threadGroupSizeY = finalSpirV.computeShaderInfo.threadGroupSizeY;
    result.computeShaderInfo.threadGroupSizeZ = finalSpirV.computeShaderInfo.threadGroupSizeZ;
  }
  result.psOutputInfo = finalSpirV.psOutput;

#if 0
  if (!inputs.skipValidation)
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

  if (inputs.embedDebugData)
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

    const String name = String(-1, "%s_%.2s", "spirv_dxc", inputs.profile);
    if (!inputs.dumpSpirvOnly)
      spitfile(inputs.shaderName, inputs.entry, name, inputs.shaderVariantHash, (void *)spirvDisas.data(), (int)data_size(spirvDisas));
    spitfile(inputs.shaderName, inputs.entry, name + "_raw", inputs.shaderVariantHash, (void *)spirv.data(), (int)data_size(spirv));
  }

  header.verMagic = spirv::HEADER_MAGIC_VER;
  header.implicitCbufRegCount = inputs.implicitCbufRegCount;

  smolv::ByteArray smol;
  smolv::Encode(spirv.data(), spirv.size() * sizeof(unsigned int), smol, 0);
  if (smol.empty())
    debug("Smol-V compression failed, exporting uncompressed spir-v");

  header.smolvSize = uint32_t(smol.size());
  add_chunk(chunks, chunkStore, spirv::ChunkType::SHADER_HEADER, 0, &header, 1);

  result.metadata = get_SpirV_bytecode(chunks, chunkStore);

  if (header.smolvSize)
  {
    result.bytecode = {smol.data(), smol.data() + smol.size()};
    G_ASSERT(result.bytecode.size() == smol.size());
  }
  else
    result.bytecode = {(const uint8_t *)spirv.data(), (const uint8_t *)spirv.data() + spirv.size() * sizeof(unsigned int)};
  return result;
}
