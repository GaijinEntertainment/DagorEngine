// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "spirv_extractor.h"

using namespace drv3d_vulkan;

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA

const char unknown_name[] = "<unknown>";

dag::ConstSpan<char> spirv_extractor::getName(const Tab<spirv::ChunkHeader> &chunks, const Tab<uint8_t> &chunk_data,
  uint32_t extension_bits)
{

  auto selected = spirv::find_chunk(chunks, spirv::ChunkType::SHADER_NAME, extension_bits);
  if (selected)
  {
    return dag::ConstSpan<char>(reinterpret_cast<const char *>(chunk_data.data() + selected->offset), selected->size);
  }
  else
    return dag::ConstSpan<char>(unknown_name, sizeof(unknown_name));
}

ShaderDebugInfo spirv_extractor::getDebugInfo(const Tab<spirv::ChunkHeader> &chunks, const Tab<uint8_t> &chunk_data,
  uint32_t extension_bits)
{
  ShaderDebugInfo result;
  auto selected = spirv::find_chunk(chunks, spirv::ChunkType::SPIR_V_DISASSEMBLY, extension_bits);
  if (selected)
  {
    result.spirvDisAsm.setStr(reinterpret_cast<const char *>(chunk_data.data() + selected->offset), selected->size);
  }

  selected = spirv::find_chunk(chunks, spirv::ChunkType::HLSL_DISASSEMBLY, extension_bits);
  if (selected)
  {
    result.dxbcDisAsm.setStr(reinterpret_cast<const char *>(chunk_data.data() + selected->offset), selected->size);
  }

  selected = spirv::find_chunk(chunks, spirv::ChunkType::RECONSTRUCTED_GLSL, extension_bits);
  if (selected)
  {
    result.sourceGLSL.setStr(reinterpret_cast<const char *>(chunk_data.data() + selected->offset), selected->size);
  }

  selected = spirv::find_chunk(chunks, spirv::ChunkType::RECONSTRUCTED_HLSL_DISASSEMBLY, extension_bits);
  if (selected)
  {
    result.reconstructedHLSL.setStr(reinterpret_cast<const char *>(chunk_data.data() + selected->offset), selected->size);
  }

  selected = spirv::find_chunk(chunks, spirv::ChunkType::HLSL_AND_RECONSTRUCTED_HLSL_XDIF, extension_bits);
  if (selected)
  {
    result.reconstructedHLSLAndSourceHLSLXDif.setStr(reinterpret_cast<const char *>(chunk_data.data() + selected->offset),
      selected->size);
  }

  selected = spirv::find_chunk(chunks, spirv::ChunkType::UNPROCESSED_HLSL, extension_bits);
  if (selected)
  {
    result.sourceHLSL.setStr(reinterpret_cast<const char *>(chunk_data.data() + selected->offset), selected->size);
  }

  auto nameData = getName(chunks, chunk_data, extension_bits);
  result.name.setStr(nameData.data(), nameData.size());
  result.debugName = unknown_name;

  return result;
}

#endif

ShaderModuleBlob spirv_extractor::getBlob(const Tab<spirv::ChunkHeader> &chunks, const Tab<uint8_t> &chunk_data,
  uint32_t extension_bits)
{
  ShaderModuleBlob result;
  // First find is ok here, the shader compiler has to write them in priority order.
  // look for raw entry first
  auto selected = spirv::find_chunk(chunks, spirv::ChunkType::SPIR_V, extension_bits);

  if (!selected)
  {
    // try smol-v
    selected = spirv::find_chunk(chunks, spirv::ChunkType::SMOL_V, extension_bits);
  }

  if (!selected)
  {
    // try mark-v
#if 0
    // no support yet
    selected = spirv::find_chunk(chunks, spirv::ChunkType::MARK_V, extension_bits)
#endif
  }

  if (selected)
  {
#if 0
  if (spirv::ChunkType::MARK_V == selected->type)
  {
  }
  else
#endif
    if (spirv::ChunkType::SMOL_V == selected->type)
    {
      result.blob.resize(smolv::GetDecodedBufferSize(chunk_data.data() + selected->offset, selected->size) / sizeof(uint32_t));
      smolv::Decode(chunk_data.data() + selected->offset, selected->size, result.blob.data(), result.blob.size() * sizeof(uint32_t));
      result.hash = spirv::HashValue::calculate(result.blob.data(), result.blob.size());
    }
    else if (spirv::ChunkType::SPIR_V == selected->type)
    {
      auto srcFrom = reinterpret_cast<const uint32_t *>(chunk_data.data() + selected->offset);
      auto srcTo = reinterpret_cast<const uint32_t *>(chunk_data.data() + selected->offset + selected->size);
      result.blob.insert(begin(result.blob), srcFrom, srcTo);
      result.hash = selected->hash;
    }
  }

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  auto nameData = getName(chunks, chunk_data, extension_bits);
  result.name.setStr(nameData.data(), nameData.size());
#endif

  return result;
}

eastl::optional<ShaderModuleHeader> spirv_extractor::getHeader(VkShaderStageFlags stage, const Tab<spirv::ChunkHeader> &chunks,
  const Tab<uint8_t> &chunk_data, uint32_t extension_bits)
{
  auto selected = spirv::find_chunk(chunks, spirv::ChunkType::SHADER_HEADER, extension_bits);

  if (selected)
  {
    auto &hdr = *reinterpret_cast<const spirv::ShaderHeader *>(chunk_data.data() + selected->offset);
    if (hdr.verMagic != spirv::HEADER_MAGIC_VER)
    {
      // try to fix header to prev version to speedup header change integration
      if (hdr.verMagic == spirv::HEADER_MAGIC_VER_PREV)
      {
        auto &oldHdr = *reinterpret_cast<const spirv::ShaderHeaderPrev *>(chunk_data.data() + selected->offset);
        spirv::ShaderHeader convertedHeader;
        memset(&convertedHeader, 0, sizeof(convertedHeader));
        convertedHeader.verMagic = spirv::HEADER_MAGIC_VER;
        for (int i = 0; i < spirv::REGISTER_ENTRIES_OLD; ++i)
        {
          convertedHeader.descriptorTypes[i] = oldHdr.descriptorTypes[i];
          convertedHeader.imageViewTypes[i] = oldHdr.imageViewTypes[i];
          convertedHeader.inputAttachmentIndex[i] = oldHdr.inputAttachmentIndex[i];
          convertedHeader.registerIndex[i] = oldHdr.registerIndex[i];
          convertedHeader.imageCheckIndices[i] = oldHdr.imageCheckIndices[i];
          convertedHeader.bufferCheckIndices[i] = oldHdr.bufferCheckIndices[i];
          convertedHeader.missingTableIndex[i] = oldHdr.missingTableIndex[i];
        }
        for (int i = 0; i < spirv::SHADER_HEADER_DECRIPTOR_COUNT_SIZE; ++i)
          convertedHeader.descriptorCounts[i] = oldHdr.descriptorCounts[i];
        for (int i = 0; i < spirv::T_REGISTER_INDEX_MAX; ++i)
        {
          convertedHeader.bufferViewCheckIndices[i] = oldHdr.bufferViewCheckIndices[i];
          convertedHeader.accelerationStructureCheckIndices[i] = oldHdr.accelerationStructureCheckIndices[i];
        }
        for (int i = 0; i < spirv::B_REGISTER_INDEX_MAX; ++i)
          convertedHeader.constBufferCheckIndices[i] = oldHdr.constBufferCheckIndices[i];

        convertedHeader.maxConstantCount = oldHdr.maxConstantCount;
        convertedHeader.bonesConstantsUsed = oldHdr.bonesConstantsUsed;
        convertedHeader.tRegisterUseMask = oldHdr.tRegisterUseMask;
        convertedHeader.uRegisterUseMask = oldHdr.uRegisterUseMask;
        convertedHeader.bRegisterUseMask = oldHdr.bRegisterUseMask;
        convertedHeader.sRegisterUseMask = oldHdr.sRegisterUseMask;
        convertedHeader.inputMask = oldHdr.inputMask;
        convertedHeader.outputMask = oldHdr.outputMask;
        convertedHeader.inputAttachmentCount = oldHdr.inputAttachmentCount;
        convertedHeader.imageCount = oldHdr.imageCount;
        convertedHeader.bufferCount = oldHdr.bufferCount;
        convertedHeader.bufferViewCount = oldHdr.bufferViewCount;
        convertedHeader.constBufferCount = oldHdr.constBufferCount;
        convertedHeader.accelerationStructureCount = oldHdr.accelerationStructureCount;
        convertedHeader.descriptorCountsCount = oldHdr.descriptorCountsCount;
        convertedHeader.registerCount = oldHdr.registerCount;
        convertedHeader.pushConstantsCount = oldHdr.pushConstantsCount;
        return ShaderModuleHeader{convertedHeader, selected->hash, stage};
      }
      DAG_FATAL("vulkan: expected shader header ver %08lX got %08lX, check shader dump integrity!", spirv::HEADER_MAGIC_VER,
        hdr.verMagic);
    }
    return ShaderModuleHeader{hdr, selected->hash, stage};
  }
  return {};
}