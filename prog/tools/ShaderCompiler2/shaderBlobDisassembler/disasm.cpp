// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "include/shaderBlobDisassembler/disasm.h"

#include <generic/dag_span.h>
#include <debug/dag_assert.h>
#include <debug/dag_debug.h>
#include <shaderBlobUnpack/shaderBlobUnpack.h>

#if !_TARGET_PC_WIN && !_TARGET_PC_MACOSX && !_TARGET_PC_LINUX
#error "Shader blob disassembler is not supported on this platform"
#endif

#if _TARGET_PC_WIN || _TARGET_PC_LINUX
#include <spirv-tools/libspirv.hpp>
#include <sstream>
#endif

#if _TARGET_PC_WIN
#include <windows.h>
#include <d3dcompiler.h>
#include <dxil/compiler.h>
#endif

namespace shader_blob_disasm
{

using namespace shader_blob;

#if _TARGET_PC_WIN || _TARGET_PC_LINUX

static void append_spirv_header(eastl::string &out, const spirv::ShaderHeader &header)
{
  out.append_sprintf("  magic=%u\n", header.verMagic);
  out.append_sprintf("  inputAttachmentCount=%u\n", header.inputAttachmentCount);
  out.append_sprintf("  descriptorCountsCount=%u\n", header.descriptorCountsCount);
  out.append_sprintf("  registerCount=%u\n", header.registerCount);
  out.append_sprintf("  pushConstantsCount=%u\n", header.pushConstantsCount);
  out.append_sprintf("  bindlessSetsUsed=%u\n", header.bindlessSetsUsed);
  out.append_sprintf("  implicitCbufRegCount=%u\n", header.implicitCbufRegCount);
  out.append_sprintf("  tRegisterUseMask=0x%x\n", header.tRegisterUseMask);
  out.append_sprintf("  uRegisterUseMask=0x%x\n", header.uRegisterUseMask);
  out.append_sprintf("  bRegisterUseMask=0x%x\n", header.bRegisterUseMask);
  out.append_sprintf("  sRegisterUseMask=0x%x\n", header.sRegisterUseMask);
  out.append_sprintf("  inputMask=0x%x\n", header.inputMask);
  out.append_sprintf("  outputMask=0x%x\n", header.outputMask);
  out.append_sprintf("  smolvSize=%u\n", header.smolvSize);
  out.append_sprintf("  resTypeMask.u=0x%x\n", header.resTypeMask.u);
  out.append_sprintf("  resTypeMask.t=0x%llx\n", header.resTypeMask.t);
  out.append("  inputAttachmentIndexRegPairs=[");
  for (auto [index, flatBinding] : header.inputAttachmentIndexRegPairs)
    out.append_sprintf(" {index=%d, flatBinding=%d}", index, flatBinding);
  out.append(" ]\n");
  out.append("  registerToSlotMapping=[");
  for (auto [slot, type] : header.registerToSlotMapping)
    out.append_sprintf(" {slot=%d, type=%d}", slot, type);
  out.append(" ]\n");
  out.append("  slotToRegisterMapping=[");
  for (uint8_t reg : header.slotToRegisterMapping)
    out.append_sprintf(" %d", reg);
  out.append(" ]\n");
  out.append("  missingTableIndex=[");
  for (uint8_t id : header.missingTableIndex)
    out.append_sprintf(" %d", id);
  out.append(" ]\n");
  out.append("  descriptorTypes=[");
  for (auto type : header.descriptorTypes)
    out.append_sprintf(" %d", type.value);
  out.append(" ]\n");
  out.append("  descriptorCounts=[");
  for (auto [type, count] : header.descriptorCounts)
    out.append_sprintf(" {type=%d, count=%d}", type.value, count);
  out.append(" ]\n");
}

static void disasm_spirv_words(dag::ConstSpan<uint8_t> code, eastl::string &out)
{
  spvtools::SpirvTools tools{SPV_ENV_VULKAN_1_0};

  std::stringstream infoStream;
  tools.SetMessageConsumer(
    [&infoStream](spv_message_level_t level, const char *, const spv_position_t &position, const char *message) {
      infoStream << "[" << int(level) << "][" << position.line << ":" << position.column << "] " << message << std::endl;
    });

  std::vector<uint32_t> data(code.size() / sizeof(uint32_t));
  memcpy(data.data(), code.data(), data.size() * sizeof(uint32_t));
  std::string disas;

  if (tools.Disassemble(data, &disas, SPV_BINARY_TO_TEXT_OPTION_INDENT))
    out.assign(disas.data(), disas.size());

  std::string infoMessage = infoStream.str();
  if (!infoMessage.empty())
    debug("Spir-V Disassemble log: %s", infoMessage.c_str());
}

static eastl::string disassembleSpirV(dag::ConstSpan<uint8_t> bytecode, dag::ConstSpan<uint8_t> metadata)
{
  UnpackedShader unpacked;
  eastl::string error;
  if (!unpack(Api::SPIRV, metadata, bytecode, Stage::VS, unpacked, &error))
  {
    logerr("Invalid SpirV shader blob format: %s.", error.c_str());
    return {};
  }

  eastl::string result;
  for (uint32_t stageIndex = 0; stageIndex < unpacked.stages.size(); ++stageIndex)
  {
    const StageBlob &stage = unpacked.stages[stageIndex];
    const spirv::ChunkSetRef *chunks = unpacked.spirvChunks(stageIndex);
    G_ASSERT_CONTINUE(chunks);

    eastl::string existingChunksDisasm;
    for (const spirv::ChunkHeader &chunkHeader : chunks->chunks)
    {
      if (!existingChunksDisasm.empty())
        existingChunksDisasm.append(" ");
      existingChunksDisasm.append(spirv::chunk_type_name(chunkHeader.type));
    }

    eastl::string headerDisasm;
    headerDisasm.append("\n");
    append_spirv_header(headerDisasm, *stage.spirvHeader);

    eastl::string spirvDisasm;
    bool hasEmbeddedDisasm = false;
    auto embedded = spirv::find_chunk_data(*chunks, spirv::ChunkType::SPIR_V_DISASSEMBLY, 0);
    if (!embedded.empty())
    {
      hasEmbeddedDisasm = true;
      spirvDisasm.assign(reinterpret_cast<const char *>(embedded.data()), embedded.size());
    }
    else
      disasm_spirv_words(stage.bytecode, spirvDisasm);

    eastl::string unprocessedHlsl;
    auto hlsl = spirv::find_chunk_data(*chunks, spirv::ChunkType::UNPROCESSED_HLSL, 0);
    if (!hlsl.empty())
      unprocessedHlsl.assign(reinterpret_cast<const char *>(hlsl.data()), hlsl.size());

    if (unpacked.stages.size() > 1)
      result.append_sprintf("Stage: %s\n", stage_name(stage.stage));
    auto optStr = [](auto const &str) -> char const * { return str.empty() ? "<not present>" : str.c_str(); };
    result.append_sprintf("Found chunks: %s\n"
                          "Header: %s\n"
                          "Disasm%s: %s\n"
                          "Hlsl: %s\n",
      optStr(existingChunksDisasm), optStr(headerDisasm), hasEmbeddedDisasm ? " (embedded)" : "", optStr(spirvDisasm),
      optStr(unprocessedHlsl));
  }
  return result;
}

#endif // _TARGET_PC_WIN || _TARGET_PC_LINUX

#if _TARGET_PC_WIN

static eastl::string disasm_dxbc(dag::ConstSpan<uint8_t> code)
{
  eastl::string result;
  ID3DBlob *disassembly = nullptr;
  if (SUCCEEDED(D3DDisassemble(code.data(), code.size(), 0, nullptr, &disassembly)) && disassembly)
  {
    result.assign(reinterpret_cast<const char *>(disassembly->GetBufferPointer()), disassembly->GetBufferSize());
    disassembly->Release();
  }
  return result;
}

static eastl::string disassembleDX11(dag::ConstSpan<uint8_t> bytecode, dag::ConstSpan<uint8_t> metadata)
{
  UnpackedShader unpacked;
  eastl::string error;
  if (!unpack(Api::DX11, metadata, bytecode, Stage::VS, unpacked, &error))
  {
    logerr("Invalid DX11 shader blob format: %s.", error.c_str());
    return {};
  }

  eastl::string result;
  const dx11::SimpleHeader *header = unpacked.stages.front().dx11Header;
  result.append_sprintf("Header:\n  maxConstants=%d\n  maxRtvUsed=%d\n  uavReadMask=0x%x\n  uavWriteMask=0x%x\n",
    header->maxConstantRegUsed, header->maxRtvUsed, dx11::get_uav_read_mask(*header), dx11::get_uav_write_mask(*header));
  for (const StageBlob &stage : unpacked.stages)
  {
    if (unpacked.stages.size() > 1)
      result.append_sprintf("Stage: %s\n", stage_name(stage.stage));
    eastl::string disasm = disasm_dxbc(stage.bytecode);
    result.append_sprintf("Size: %u bytes\nDisasm: %s\n", unsigned(stage.bytecode.size()),
      disasm.empty() ? "<failed>" : disasm.c_str());
  }
  return result;
}

static void append_dxil_header(eastl::string &out, const dxil::ShaderHeader &header)
{
  out.append_sprintf("  shaderType=%u\n", header.shaderType);
  out.append_sprintf("  implicitCbufRegCount=%u\n", header.implicitCbufRegCount);
  out.append_sprintf("  bonesConstantsUsed=%u\n", header.bonesConstantsUsed);
  out.append_sprintf("  tRegisterUseMask=0x%x\n", header.resourceUsageTable.tRegisterUseMask);
  out.append_sprintf("  sRegisterUseMask=0x%x\n", header.resourceUsageTable.sRegisterUseMask);
  out.append_sprintf("  bindlessUsageMask=0x%x\n", header.resourceUsageTable.bindlessUsageMask);
  out.append_sprintf("  bRegisterUseMask=0x%x\n", header.resourceUsageTable.bRegisterUseMask);
  out.append_sprintf("  uRegisterUseMask=0x%x\n", header.resourceUsageTable.uRegisterUseMask);
  out.append_sprintf("  rootConstantDwords=%u\n", header.resourceUsageTable.rootConstantDwords);
  out.append_sprintf("  specialConstantsMask=%u\n", header.resourceUsageTable.specialConstantsMask);
  out.append_sprintf("  sRegisterCompareUseMask=0x%x\n", header.sRegisterCompareUseMask);
  out.append_sprintf("  inOutSemanticMask=0x%x\n", header.inOutSemanticMask);
  out.append_sprintf("  inputPrimitive=%u\n", header.inputPrimitive);
}

static eastl::string disassembleDX12(dag::ConstSpan<uint8_t> bytecode, dag::ConstSpan<uint8_t> metadata)
{
  UnpackedShader unpacked;
  eastl::string error;
  if (!unpack(Api::DX12, metadata, bytecode, Stage::VS, unpacked, &error))
  {
    logerr("Invalid DX12 shader blob format: %s.", error.c_str());
    return {};
  }

  // DXIL text disasm needs dxcompiler; when it is not around fall back to
  // D3DDisassemble, which handles DXBC blobs only
  static HMODULE dxcLib = LoadLibraryA("dxcompiler.dll");

  eastl::string result;
  for (const StageBlob &stage : unpacked.stages)
  {
    eastl::string headerDisasm;
    append_dxil_header(headerDisasm, *stage.dxilHeader);

    eastl::string disasm;
    if (dxcLib)
      disasm = ::dxil::disassemble({stage.bytecode.data(), size_t(stage.bytecode.size())}, dxcLib);
    if (disasm.empty())
      disasm = disasm_dxbc(stage.bytecode);

    result.append_sprintf("Stage: %s (%u bytes)\nHeader:\n%sDisasm: %s\n", stage_name(stage.stage), unsigned(stage.bytecode.size()),
      headerDisasm.c_str(), disasm.empty() ? "<no disassembler available, raw blob only>" : disasm.c_str());
  }
  return result;
}

#endif // _TARGET_PC_WIN

eastl::string disassembleShaderBlob(dag::ConstSpan<uint8_t> bytecode, dag::ConstSpan<uint8_t> metadata)
{
  auto detectedApi = detect_api(metadata);
  G_ASSERTF_RETURN(detectedApi, {}, "Api detection for blob disasm failed: %s", detectedApi.error().c_str());

  switch (*detectedApi)
  {
#if _TARGET_PC_WIN
    case Api::DX11: return disassembleDX11(bytecode, metadata);
    case Api::DX12: return disassembleDX12(bytecode, metadata);
#endif
#if _TARGET_PC_WIN || _TARGET_PC_LINUX
    case Api::SPIRV: return disassembleSpirV(bytecode, metadata);
#endif

    default: G_ASSERTF_RETURN(0, {}, "Shader blob disasm for %s is not supported on this platform", api_name(*detectedApi)); break;
  }

  return {};
}

} // namespace shader_blob_disasm
