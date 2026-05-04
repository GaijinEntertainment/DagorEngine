// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "spirv_extractor.h"

using namespace drv3d_vulkan;

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA

const char unknown_name[] = "<unknown>";

dag::ConstSpan<char> spirv_extractor::getName(const Tab<spirv::ChunkHeader> &chunks, const dag::ConstSpan<uint8_t> &chunk_data,
  uint32_t extension_bits)
{

  auto selected = spirv::find_chunk(chunks, spirv::ChunkType::SHADER_NAME, extension_bits);
  if (selected)
  {
    G_ASSERT(selected->offset + selected->size <= chunk_data.size());
    return dag::ConstSpan<char>(reinterpret_cast<const char *>(chunk_data.data() + selected->offset), selected->size);
  }
  else
    return dag::ConstSpan<char>(unknown_name, sizeof(unknown_name));
}

ShaderDebugInfo spirv_extractor::getDebugInfo(const Tab<spirv::ChunkHeader> &chunks, const dag::ConstSpan<uint8_t> &chunk_data,
  uint32_t extension_bits)
{
  ShaderDebugInfo result;
  auto selected = spirv::find_chunk(chunks, spirv::ChunkType::SPIR_V_DISASSEMBLY, extension_bits);
  if (selected)
  {
    G_ASSERT(selected->offset + selected->size <= chunk_data.size());
    result.spirvDisAsm.setStr(reinterpret_cast<const char *>(chunk_data.data() + selected->offset), selected->size);
  }

  selected = spirv::find_chunk(chunks, spirv::ChunkType::HLSL_DISASSEMBLY, extension_bits);
  if (selected)
  {
    G_ASSERT(selected->offset + selected->size <= chunk_data.size());
    result.dxbcDisAsm.setStr(reinterpret_cast<const char *>(chunk_data.data() + selected->offset), selected->size);
  }

  selected = spirv::find_chunk(chunks, spirv::ChunkType::RECONSTRUCTED_GLSL, extension_bits);
  if (selected)
  {
    G_ASSERT(selected->offset + selected->size <= chunk_data.size());
    result.sourceGLSL.setStr(reinterpret_cast<const char *>(chunk_data.data() + selected->offset), selected->size);
  }

  selected = spirv::find_chunk(chunks, spirv::ChunkType::RECONSTRUCTED_HLSL_DISASSEMBLY, extension_bits);
  if (selected)
  {
    G_ASSERT(selected->offset + selected->size <= chunk_data.size());
    result.reconstructedHLSL.setStr(reinterpret_cast<const char *>(chunk_data.data() + selected->offset), selected->size);
  }

  selected = spirv::find_chunk(chunks, spirv::ChunkType::HLSL_AND_RECONSTRUCTED_HLSL_XDIF, extension_bits);
  if (selected)
  {
    G_ASSERT(selected->offset + selected->size <= chunk_data.size());
    result.reconstructedHLSLAndSourceHLSLXDif.setStr(reinterpret_cast<const char *>(chunk_data.data() + selected->offset),
      selected->size);
  }

  selected = spirv::find_chunk(chunks, spirv::ChunkType::UNPROCESSED_HLSL, extension_bits);
  if (selected)
  {
    G_ASSERT(selected->offset + selected->size <= chunk_data.size());
    result.sourceHLSL.setStr(reinterpret_cast<const char *>(chunk_data.data() + selected->offset), selected->size);
  }

  auto nameData = getName(chunks, chunk_data, extension_bits);
  result.name.setStr(nameData.data(), nameData.size());
  result.debugName = unknown_name;

  return result;
}

#endif

ShaderModuleBlob spirv_extractor::getBlob(const ShaderModuleHeader &header, const ShaderSource &source,
  const ShaderProgramData &bytecode, const Tab<spirv::ChunkHeader> &chunks, dag::ConstSpan<uint8_t> chunk_data,
  uint32_t extension_bits)
{
  ShaderModuleBlob result;
  result.source = source;
  result.offset = bytecode.offset;
  result.sizeSmolv = header.header.smolvSize;
  result.size = bytecode.size;

  G_UNUSED(chunks);
  G_UNUSED(chunk_data);
  G_UNUSED(extension_bits);
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  auto nameData = getName(chunks, chunk_data, extension_bits);
  result.name.setStr(nameData.data(), nameData.size());
#endif

  return result;
}

eastl::optional<ShaderModuleHeader> spirv_extractor::getHeader(VkShaderStageFlags stage, const Tab<spirv::ChunkHeader> &chunks,
  dag::ConstSpan<uint8_t> chunk_data, uint32_t extension_bits)
{
  auto selected = spirv::find_chunk(chunks, spirv::ChunkType::SHADER_HEADER, extension_bits);

  if (selected)
  {
    G_ASSERT(selected->offset + selected->size <= chunk_data.size());
    auto &hdr = *reinterpret_cast<const spirv::ShaderHeader *>(chunk_data.data() + selected->offset);
    if (hdr.verMagic != spirv::HEADER_MAGIC_VER)
    {
      spirv::ShaderHeaderPrev convertedV7 = {};
      bool convertedFromV7 = false;
      // try to fix header to prev version to speedup header change integration
      if (hdr.verMagic == spirv::HEADER_MAGIC_VER_7)
      {
        auto &prev = *reinterpret_cast<const spirv::ShaderHeaderPrevV7 *>(chunk_data.data() + selected->offset);
        convertedFromV7 = true;

        // Direct field copies
        convertedV7.verMagic = spirv::HEADER_MAGIC_VER_8;
        convertedV7.inputAttachmentCount = prev.inputAttachmentCount;
        convertedV7.descriptorCountsCount = prev.descriptorCountsCount;
        convertedV7.registerCount = prev.registerCount;
        convertedV7.pushConstantsCount = prev.pushConstantsCount;
        convertedV7.bindlessSetsUsed = prev.bindlessSetsUsed;
        convertedV7.maxConstantCount = prev.maxConstantCount;
        convertedV7.tRegisterUseMask = 0;
        convertedV7.uRegisterUseMask = prev.uRegisterUseMask;
        convertedV7.bRegisterUseMask = prev.bRegisterUseMask;
        convertedV7.sRegisterUseMask = prev.sRegisterUseMask;
        convertedV7.inputMask = prev.inputMask;
        convertedV7.outputMask = prev.outputMask;
        for (uint8_t i = 0; i < prev.descriptorCountsCount; ++i)
        {
          convertedV7.descriptorCounts[i].type.set(prev.descriptorCounts[i].type);
          convertedV7.descriptorCounts[i].descriptorCount = prev.descriptorCounts[i].descriptorCount;
        }

        // Build register mappings, resource type mask, input attachment pairs and descriptor types
        // from per-register old header data
        convertedV7.resTypeMask = {};

        uint8_t iaCount = 0;
        for (uint8_t i = 0; i < prev.registerCount; ++i)
        {
          // Derive register type, slot and resource type from flat registerIndex range
          // registerIndex stores registerBase + sourceBindingIndex, where registerBase
          // encodes both the register type (B/T/U/S) and resource subtype
          uint8_t regIdx = prev.registerIndex[i];
          uint8_t regType;
          uint8_t slot;
          uint64_t resTypeBits = spirv::ResourceTypeMask::IMG;
          bool isInputAttachment = false;

          if (regIdx < spirv::T_SAMPLED_IMAGE_OFFSET_V7)
          {
            // B_CONST_BUFFER range
            regType = spirv::EngineSlotMapping::REG_B;
            slot = regIdx - spirv::B_CONST_BUFFER_OFFSET_V7;
          }
          else if (regIdx < spirv::T_BUFFER_SAMPLED_IMAGE_OFFSET_V7)
          {
            // T_SAMPLED_IMAGE range -> IMG
            regType = spirv::EngineSlotMapping::REG_T;
            slot = regIdx - spirv::T_SAMPLED_IMAGE_OFFSET_V7;
          }
          else if (regIdx < spirv::T_BUFFER_OFFSET_V7)
          {
            // T_BUFFER_SAMPLED_IMAGE range -> BUF_VIEW
            regType = spirv::EngineSlotMapping::REG_T;
            slot = regIdx - spirv::T_BUFFER_SAMPLED_IMAGE_OFFSET_V7;
            resTypeBits = spirv::ResourceTypeMask::BUF_VIEW;
          }
          else if (regIdx < spirv::T_INPUT_ATTACHMENT_OFFSET_V7)
          {
            // T_BUFFER range -> BUF
            regType = spirv::EngineSlotMapping::REG_T;
            slot = regIdx - spirv::T_BUFFER_OFFSET_V7;
            resTypeBits = spirv::ResourceTypeMask::BUF;
          }
          else if (regIdx < spirv::S_SAMPLER_OFFSET_V7)
          {
            // T_INPUT_ATTACHMENT range
            regType = spirv::EngineSlotMapping::REG_T;
            slot = regIdx - spirv::T_INPUT_ATTACHMENT_OFFSET_V7;
            isInputAttachment = true;
          }
          else if (regIdx < spirv::U_IMAGE_OFFSET_V7)
          {
            // S_SAMPLER range -> sampler
            regType = spirv::EngineSlotMapping::REG_S;
            slot = regIdx - spirv::S_SAMPLER_OFFSET_V7;
          }
          else if (regIdx < spirv::U_BUFFER_IMAGE_OFFSET_V7)
          {
            // U_IMAGE range -> IMG
            regType = spirv::EngineSlotMapping::REG_U;
            slot = regIdx - spirv::U_IMAGE_OFFSET_V7;
          }
          else if (regIdx < spirv::U_BUFFER_OFFSET_V7)
          {
            // U_BUFFER_IMAGE range -> BUF_VIEW
            regType = spirv::EngineSlotMapping::REG_U;
            slot = regIdx - spirv::U_BUFFER_IMAGE_OFFSET_V7;
            resTypeBits = spirv::ResourceTypeMask::BUF_VIEW;
          }
          else if (regIdx < spirv::T_ACCELERATION_STRUCTURE_OFFSET_V7)
          {
            // U_BUFFER range -> BUF
            regType = spirv::EngineSlotMapping::REG_U;
            slot = regIdx - spirv::U_BUFFER_OFFSET_V7;
            resTypeBits = spirv::ResourceTypeMask::BUF;
          }
          else
          {
            // T_ACCELERATION_STRUCTURE range -> AS
            regType = spirv::EngineSlotMapping::REG_T;
            slot = regIdx - spirv::T_ACCELERATION_STRUCTURE_OFFSET_V7;
            resTypeBits = spirv::ResourceTypeMask::AS;
          }

          if (regType == spirv::EngineSlotMapping::REG_T && !isInputAttachment)
          {
            G_ASSERT((convertedV7.tRegisterUseMask & (1 << slot)) == 0);
            convertedV7.tRegisterUseMask |= (1 << slot);
          }

          convertedV7.registerToSlotMapping[i].slot = slot;
          convertedV7.registerToSlotMapping[i].type = regType;
          convertedV7.descriptorTypes[i].set(prev.descriptorTypes[i]);
          convertedV7.missingTableIndex[i] = prev.missingTableIndex[i];

          // Build inverse slot-to-register mapping
          switch (regType)
          {
            case spirv::EngineSlotMapping::REG_B:
              convertedV7.slotToRegisterMapping[spirv::REGISTER_MAPPING_B_OFFSET + slot] = i;
              break;
            case spirv::EngineSlotMapping::REG_T:
              if (!isInputAttachment)
                convertedV7.slotToRegisterMapping[spirv::REGISTER_MAPPING_T_OFFSET_V7 + slot] = i;
              break;
            case spirv::EngineSlotMapping::REG_U:
              convertedV7.slotToRegisterMapping[spirv::REGISTER_MAPPING_U_OFFSET_V7 + slot] = i;
              break;
            case spirv::EngineSlotMapping::REG_S:
              convertedV7.slotToRegisterMapping[spirv::REGISTER_MAPPING_S_OFFSET_V7 + slot] = i;
              break;
            default: break;
          }

          // Build input attachment index-to-register pairs
          if (isInputAttachment && iaCount < spirv::T_INPUT_ATTACHMENT_INDEX_MAX)
          {
            convertedV7.inputAttachmentIndexRegPairs[iaCount].index = prev.inputAttachmentIndex[i];
            convertedV7.inputAttachmentIndexRegPairs[iaCount].flatBinding = slot;
            ++iaCount;
          }

          // Build resource type mask for T and U registers
          if (regType == spirv::EngineSlotMapping::REG_T && !isInputAttachment)
            convertedV7.resTypeMask.t |= resTypeBits << (slot * spirv::ResourceTypeMask::BIT_COUNT);
          else if (regType == spirv::EngineSlotMapping::REG_U)
            convertedV7.resTypeMask.u |= (uint32_t)resTypeBits << (slot * spirv::ResourceTypeMask::BIT_COUNT);
        }
      }
      if (hdr.verMagic == spirv::HEADER_MAGIC_VER_8 || convertedFromV7)
      {
        auto &prev =
          convertedFromV7 ? convertedV7 : *reinterpret_cast<const spirv::ShaderHeaderPrev *>(chunk_data.data() + selected->offset);
        spirv::ShaderHeader converted = {};

        // Scalar fields
        converted.verMagic = spirv::HEADER_MAGIC_VER;
        converted.inputAttachmentCount = prev.inputAttachmentCount;
        converted.descriptorCountsCount = prev.descriptorCountsCount;
        converted.registerCount = prev.registerCount;
        converted.pushConstantsCount = prev.pushConstantsCount;
        converted.bindlessSetsUsed = prev.bindlessSetsUsed;
        converted.maxConstantCount = prev.maxConstantCount;
        converted.tRegisterUseMask = prev.tRegisterUseMask;
        converted.uRegisterUseMask = prev.uRegisterUseMask;
        converted.bRegisterUseMask = prev.bRegisterUseMask;
        converted.sRegisterUseMask = prev.sRegisterUseMask;
        converted.inputMask = prev.inputMask;
        converted.outputMask = prev.outputMask;
        converted.resTypeMask = prev.resTypeMask;

        // Input attachment pairs (same size, direct copy)
        for (uint8_t i = 0; i < spirv::T_INPUT_ATTACHMENT_INDEX_MAX; ++i)
          converted.inputAttachmentIndexRegPairs[i] = prev.inputAttachmentIndexRegPairs[i];

        // Register-indexed arrays: only first registerCount entries are used, copy directly
        for (uint8_t i = 0; i < prev.registerCount; ++i)
        {
          converted.registerToSlotMapping[i] = prev.registerToSlotMapping[i];
          converted.missingTableIndex[i] = prev.missingTableIndex[i];
          converted.descriptorTypes[i] = prev.descriptorTypes[i];
        }

        // Descriptor pool sizes (same size, direct copy)
        for (uint8_t i = 0; i < prev.descriptorCountsCount; ++i)
          converted.descriptorCounts[i] = prev.descriptorCounts[i];

        // slotToRegisterMapping: remap from v7 offsets to current offsets
        // Initialize all slots to invalid
        memset(converted.slotToRegisterMapping, spirv::INVALID_SHADER_REGISTER, sizeof(converted.slotToRegisterMapping));

        // B slots: v7 had 8, current has 12 — copy existing, new slots stay invalid
        for (uint32_t i = 0; i < spirv::B_REGISTER_INDEX_MAX_V7; ++i)
          converted.slotToRegisterMapping[spirv::REGISTER_MAPPING_B_OFFSET + i] =
            prev.slotToRegisterMapping[spirv::REGISTER_MAPPING_B_OFFSET + i];

        // T slots: same count, different offset
        for (uint32_t i = 0; i < spirv::T_REGISTER_INDEX_MAX; ++i)
          converted.slotToRegisterMapping[spirv::REGISTER_MAPPING_T_OFFSET + i] =
            prev.slotToRegisterMapping[spirv::REGISTER_MAPPING_T_OFFSET_V7 + i];

        // U slots: same count, different offset
        for (uint32_t i = 0; i < spirv::U_REGISTER_INDEX_MAX; ++i)
          converted.slotToRegisterMapping[spirv::REGISTER_MAPPING_U_OFFSET + i] =
            prev.slotToRegisterMapping[spirv::REGISTER_MAPPING_U_OFFSET_V7 + i];

        // S slots: same count, different offset
        for (uint32_t i = 0; i < spirv::S_REGISTER_INDEX_MAX; ++i)
          converted.slotToRegisterMapping[spirv::REGISTER_MAPPING_S_OFFSET + i] =
            prev.slotToRegisterMapping[spirv::REGISTER_MAPPING_S_OFFSET_V7 + i];

        return ShaderModuleHeader{converted, selected->hash, stage};
      }
      else
      {
        DAG_FATAL("vulkan: expected shader header ver %08lX got %08lX, check shader dump integrity!", spirv::HEADER_MAGIC_VER,
          hdr.verMagic);
      }
    }
    return ShaderModuleHeader{hdr, selected->hash, stage};
  }
  return {};
}
