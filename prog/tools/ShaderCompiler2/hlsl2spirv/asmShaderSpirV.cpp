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

static constexpr char const *SPIRV_CHUNK_TYPE_NAMES[] = {"SHADER_HEADER", "SMOL_V", "MARK_V", "SPIR_V", "SPIR_V_DISASSEMBLY",
  "HLSL_DISASSEMBLY", "RECONSTRUCTED_GLSL", "RECONSTRUCTED_HLSL_DISASSEMBLY", "HLSL_AND_RECONSTRUCTED_HLSL_XDIF", "UNPROCESSED_HLSL",
  "SHADER_NAME"};

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

CompileResult compileShaderSpirV(const spirv::DXCContext *dxc_ctx, const char *source, const char *profile, const char *entry,
  bool need_disasm, bool hlsl2021, bool enable_fp16, bool skipValidation, bool optimize, int max_constants_no, const char *shader_name,
  uint64_t shader_variant_hash, bool enable_bindless, bool embed_debug_data, bool dump_spirv_only,
  bool validate_global_consts_offset_order)
{
  CompileResult result;
  // just to have it for debugging
  (void)need_disasm;

#if DAGOR_DBGLEVEL > 1
  optimize = false;
#endif

  spirv::ShaderHeader header = {};
  Tab<spirv::ChunkHeader> chunks;
  Tab<uint8_t> chunkStore;
  std::vector<unsigned int> spirv;

  if (embed_debug_data)
    add_chunk(chunks, chunkStore, spirv::ChunkType::UNPROCESSED_HLSL, 0, source, static_cast<uint32_t>(strlen(source)));

  string codeCopy(source);

  // code preprocess to fix SV_VertexID disparity between DX and vulkan
  if (useBaseVertexPatch(source))
  {
    if (!fix_vertex_id_for_DXC(codeCopy, result))
      return result;
  }

  eastl::vector<eastl::string_view> disabledSpirvOptims = scanDisabledSpirvOptimizations(source);

  string macros = "#define SHADER_COMPILER_DXC 1\n"
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

  // format for profile is *s_X_Y
  bool allowWaveIntrisics = strlen(profile) > 3 && profile[3] >= '6';
  if (allowWaveIntrisics)
  {
    macros += "#define WAVE_INTRINSICS 1\n";
  }
  codeCopy = macros + codeCopy;

  auto sourceRange = make_span(codeCopy.c_str(), codeCopy.size());

  auto flags = enable_bindless ? spirv::CompileFlags::ENABLE_BINDLESS_SUPPORT : spirv::CompileFlags::NONE;
  flags |= enable_fp16 ? spirv::CompileFlags::ENABLE_HALFS : spirv::CompileFlags::NONE;
  flags |= hlsl2021 ? spirv::CompileFlags::ENABLE_HLSL21 : spirv::CompileFlags::NONE;
  flags |= allowWaveIntrisics ? spirv::CompileFlags::ENABLE_WAVE_INTRINSICS : spirv::CompileFlags::NONE;
  flags |= validate_global_consts_offset_order ? spirv::CompileFlags::VALIDATE_GLOBAL_CONSTS_OFFSET_ORDER : spirv::CompileFlags::NONE;

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

    const String name = String(-1, "%s_%.2s", "spirv_dxc", profile);
    if (!dump_spirv_only)
      spitfile(shader_name, entry, name, shader_variant_hash, (void *)spirvDisas.data(), (int)data_size(spirvDisas));
    spitfile(shader_name, entry, name + "_raw", shader_variant_hash, (void *)spirv.data(), (int)data_size(spirv));
  }

  header.verMagic = spirv::HEADER_MAGIC_VER;
  header.maxConstantCount = max_constants_no;

  smolv::ByteArray smol;
  smolv::Encode(spirv.data(), spirv.size() * sizeof(unsigned int), smol, 0);
  if (smol.empty())
    debug("Smol-V compression failed, exporting uncompressed spir-v");

  header.smolvSize = uint32_t(smol.size());
  add_chunk(chunks, chunkStore, spirv::ChunkType::SHADER_HEADER, 0, &header, 1);
  add_chunk(chunks, chunkStore, spirv::ChunkType::SHADER_NAME, 0, shader_name, static_cast<uint32_t>(strlen(shader_name)));

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

eastl::string disassembleShaderSpirV(dag::ConstSpan<uint8_t> bytecode, dag::ConstSpan<uint8_t> metadata)
{
#define FORMAT_FAIL(fmt_, ...)                                            \
  do                                                                      \
  {                                                                       \
    logerr("Invalid SpirV shader blob format: " fmt_ ".", ##__VA_ARGS__); \
    return {};                                                            \
  } while (0)

  Tab<spirv::ChunkHeader> chunks{};
  Tab<uint8_t> storage{};

  {
    InPlaceMemLoadCB reader{metadata.data(), int(metadata.size())};

    int const blobIdent = reader.readInt();
    if (blobIdent != spirv::SPIR_V_BLOB_IDENT && blobIdent != spirv::SPIR_V_BLOB_IDENT_UNCOMPRESSED)
      FORMAT_FAIL("invalid blob ident %d", blobIdent);
    bool const readCompressed = blobIdent == spirv::SPIR_V_BLOB_IDENT;

    int const dataSize = reader.beginBlock();

    if (readCompressed)
    {
      ZlibLoadCB compressedReader{reader, dataSize};
      compressedReader.readTab(chunks);
      compressedReader.readTab(storage);
    }
    else
    {
      reader.readTab(chunks);
      reader.readTab(storage);
    }

    reader.endBlock();
  }

  // @TODO: disasm more information
  eastl::string existingChunksDisasm{};
  eastl::string headerDisasm{};
  eastl::string spirvDisasm{};
  eastl::string unprocessedHlsl{};
  eastl::string shaderName{};
  eastl::string warnings{};
  bool hasEmbeddedDisasm = false;

  auto disasmSpirv = [](dag::ConstSpan<uint32_t> spirv, eastl::string &out) {
    spvtools::SpirvTools tools{SPV_ENV_VULKAN_1_0};

    std::stringstream infoStream;
    tools.SetMessageConsumer([&infoStream](spv_message_level_t level, const char *, const spv_position_t &position,
                               const char *message) { infoStream << "[" << level << "][" << position << "] " << message << endl; });

    std::vector<uint32_t> data(spirv.size());
    memcpy(data.data(), spirv.data(), spirv.size() * elem_size(spirv));
    std::string disas;

    if (tools.Disassemble(data, &disas, SPV_BINARY_TO_TEXT_OPTION_INDENT))
    {
      out.resize(disas.size());
      strncpy(out.data(), disas.data(), out.size());
    }

    string infoMessage = infoStream.str();
    if (!infoMessage.empty())
    {
      debug("Spir-V Disassemble log: %s", infoMessage.c_str());
    }
  };

  spirv::ShaderHeader header;

  for (auto const &chunkHeader : chunks)
  {
    if (!existingChunksDisasm.empty())
      existingChunksDisasm.append(" ");
    existingChunksDisasm.append(SPIRV_CHUNK_TYPE_NAMES[uint32_t(chunkHeader.type)]);

    switch (chunkHeader.type)
    {
      case spirv::ChunkType::SHADER_HEADER:
      {
        if (chunkHeader.size != sizeof(spirv::ShaderHeader))
          FORMAT_FAIL("invalid shader header");
        memcpy(&header, storage.data() + chunkHeader.offset, sizeof(header));
        // @TODO: this would benefit greatly from some struct printing lib functionality (friend injection or wait for c++26
        // reflection).
        headerDisasm.append("\n");
        if (header.verMagic != spirv::HEADER_MAGIC_VER)
        {
          warnings.append_sprintf("\n  WARNING: header version mismatch, %d in blob, %d in exe", header.verMagic,
            spirv::HEADER_MAGIC_VER);
        }
        headerDisasm.append_sprintf("  magic=%u\n", header.verMagic);
        headerDisasm.append_sprintf("  inputAttachmentCount=%u\n", header.inputAttachmentCount);
        headerDisasm.append_sprintf("  descriptorCountsCount=%u\n", header.descriptorCountsCount);
        headerDisasm.append_sprintf("  registerCount=%u\n", header.registerCount);
        headerDisasm.append_sprintf("  pushConstantsCount=%u\n", header.pushConstantsCount);
        headerDisasm.append_sprintf("  bindlessSetsUsed=%u\n", header.bindlessSetsUsed);
        headerDisasm.append_sprintf("  maxConstantCount=%u\n", header.maxConstantCount);
        headerDisasm.append_sprintf("  tRegisterUseMask=0x%x\n", header.tRegisterUseMask);
        headerDisasm.append_sprintf("  uRegisterUseMask=0x%x\n", header.uRegisterUseMask);
        headerDisasm.append_sprintf("  bRegisterUseMask=0x%x\n", header.bRegisterUseMask);
        headerDisasm.append_sprintf("  sRegisterUseMask=0x%x\n", header.sRegisterUseMask);
        headerDisasm.append_sprintf("  inputMask=0x%x\n", header.inputMask);
        headerDisasm.append_sprintf("  outputMask=0x%x\n", header.sRegisterUseMask);
        headerDisasm.append_sprintf("  smolvSize=%u\n", header.smolvSize);
        headerDisasm.append_sprintf("  resTypeMask.u=0x%x\n", header.resTypeMask.u);
        headerDisasm.append_sprintf("  resTypeMask.t=0x%llx\n", header.resTypeMask.t);
        headerDisasm.append("  inputAttachmentIndexRegPairs=[");
        for (auto [index, flatBinding] : header.inputAttachmentIndexRegPairs)
          headerDisasm.append_sprintf(" {index=%d, flatBinding=%d}", index, flatBinding);
        headerDisasm.append(" ]\n");
        headerDisasm.append("  registerToSlotMapping=[");
        for (auto [slot, type] : header.registerToSlotMapping)
          headerDisasm.append_sprintf(" {slot=%d, type=%d}", slot, type);
        headerDisasm.append(" ]\n");
        headerDisasm.append("  slotToRegisterMapping=[");
        for (uint8_t reg : header.slotToRegisterMapping)
          headerDisasm.append_sprintf(" %d", reg);
        headerDisasm.append(" ]\n");
        headerDisasm.append("  missingTableIndex=[");
        for (uint8_t id : header.missingTableIndex)
          headerDisasm.append_sprintf(" %d", id);
        headerDisasm.append(" ]\n");
        headerDisasm.append("  descriptorTypes=[");
        for (auto type : header.descriptorTypes)
          headerDisasm.append_sprintf(" %d", type.value);
        headerDisasm.append(" ]\n");
        headerDisasm.append("  descriptorCounts=[");
        for (auto [type, count] : header.descriptorCounts)
          headerDisasm.append_sprintf(" {type=%d, count=%d}", type.value, count);
        headerDisasm.append(" ]\n");
        break;
      }
      case spirv::ChunkType::SMOL_V:
      case spirv::ChunkType::MARK_V:
      case spirv::ChunkType::SPIR_V: break;
      case spirv::ChunkType::SPIR_V_DISASSEMBLY:
      {
        hasEmbeddedDisasm = true;
        spirvDisasm.assign((char const *)storage.data() + chunkHeader.offset, chunkHeader.size);

        break;
      }
      case spirv::ChunkType::UNPROCESSED_HLSL:
      {
        if (!unprocessedHlsl.empty())
          break;

        unprocessedHlsl.assign((char const *)storage.data() + chunkHeader.offset, chunkHeader.size);

        break;
      }
      case spirv::ChunkType::SHADER_NAME:
      {
        if (!shaderName.empty())
          break;

        shaderName.assign((char const *)storage.data() + chunkHeader.offset, chunkHeader.size);

        break;
      }

      default:
      {
        // @TODO: disasm more chunk types
        break;
      }
    }
  }

  if (!hasEmbeddedDisasm)
  {
    if (headerDisasm.empty())
    {
      FORMAT_FAIL("shader header not found");
    }

    if (header.smolvSize)
    {
      Tab<uint32_t> spirv(smolv::GetDecodedBufferSize(bytecode.data(), bytecode.size()) / sizeof(uint32_t));
      smolv::Decode(bytecode.data(), bytecode.size(), spirv.data(), spirv.size() * sizeof(uint32_t));

      disasmSpirv(spirv, spirvDisasm);
    }
    else
    {
      disasmSpirv(dag::ConstSpan<uint32_t>{(uint32_t const *)bytecode.data(), intptr_t(bytecode.size() / sizeof(uint32_t))},
        spirvDisasm);
    }
  }

  auto optStr = [](auto const &str) -> char const * { return str.empty() ? "<not present>" : str.c_str(); };

  return eastl::string{eastl::string::CtorSprintf{},
    "Found chunks: %s\n"
    "Shader: %s\n"
    "Warnings: %s\n"
    "Header: %s\n"
    "Disasm%s: %s\n"
    "Hlsl: %s\n", //
    optStr(existingChunksDisasm), optStr(shaderName), optStr(warnings), optStr(headerDisasm), hasEmbeddedDisasm ? " (embedded)" : "",
    optStr(spirvDisasm), optStr(unprocessedHlsl)};
}
