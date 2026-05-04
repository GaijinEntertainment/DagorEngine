// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "render_work.h"
#include "backend/context.h"
#include <ska_hash_map/flat_hash_map2.hpp>
#include <EASTL/sort.h>
#include <gui/dag_visualLog.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <perfMon/dag_statDrv.h>
#include "util/backtrace.h"
#include "memory_heap_resource.h"
#include "backend.h"
#include "execution_async_compile_state.h"
#include "execution_scratch.h"
#include "pipeline_cache.h"
#include "execution_timings.h"
#include "stacked_profile_events.h"
#include "execution_sync.h"
#include "bindless.h"
#include "vk_to_string.h"
#include "wrapped_command_buffer.h"
#include "resource_manager.h"

using namespace drv3d_vulkan;

namespace
{
template <typename T>
const char *asBooleanString(const T &value)
{
  return value ? "true" : "false";
}
} // namespace

TSPEC String FaultReportDump::paramToString(const float &value) { return String(16, "%f", value); }

TSPEC String FaultReportDump::paramToString(const uint32_t &value) { return String(16, "%u", value); }

TSPEC String FaultReportDump::paramToString(const int64_t &value) { return String(16, "%i", value); }

TSPEC String FaultReportDump::paramToString(const uint64_t &value) { return String(16, "%u", value); }

TSPEC String FaultReportDump::paramToString(const int &value) { return String(16, "%i", value); }

TSPEC String FaultReportDump::paramToString(const bool &value) { return String(16, "%s", asBooleanString(value)); }

TSPEC String FaultReportDump::paramToString(const VkExtent2D &value) { return String(32, "{%u, %u}", value.width, value.height); }

TSPEC String FaultReportDump::paramToString(const E3DCOLOR &value)
{
  return String(32, "E3DCOLOR{ r = %u, g = %u,  b = %u, a = %u }", value.r, value.g, value.b, value.a);
}

TSPEC String FaultReportDump::paramToString(const ViewportState &value)
{
  return String(32, "ViewportState{ x = %i, y = %i, w = %u, h = %u, min-z = %f, max-z = %f }", value.rect2D.offset.x,
    value.rect2D.offset.y, value.rect2D.extent.width, value.rect2D.extent.height, value.minZ, value.maxZ);
}

TSPEC String FaultReportDump::paramToString(const VulkanBufferHandle &buffer)
{
  addRef(currentCommand, FaultReportDump::GlobalTag::TAG_VK_HANDLE, generalize(buffer));
  return String(32, "VulkanBufferHandle{ 0x%X }", buffer.value);
}

TSPEC String FaultReportDump::paramToString(const VulkanQueryPoolHandle &buffer)
{
  return String(32, "VulkanQueryPoolHandle{ 0x%X }", buffer.value);
}

TSPEC String FaultReportDump::paramToString(const BufferRef &value)
{
  addRef(currentCommand, FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)value.buffer);
  return String(32, "BufferRef{ buffer = 0x%p{0x%X}, offset = %u }", value.buffer, value.buffer ? value.buffer->getHandle().value : 0,
    value.offset);
}

TSPEC String FaultReportDump::paramToString(const VkRect2D &value)
{
  return String(32, "VkRect2D{ x = %i, y = %i, w = %u, h = %u }", value.offset.x, value.offset.y, value.extent.width,
    value.extent.height);
}

TSPEC String FaultReportDump::paramToString(const ProgramID &value)
{
  addRef(currentCommand, FaultReportDump::GlobalTag::TAG_PIPE, (uint64_t)value.get());
  return String(32, "ProgramID{ %u }", value.get());
}

TSPEC String FaultReportDump::paramToString(const VkImageSubresourceRange &value)
{
  return String(32,
    "VkImageSubresourceRange{ aspectMask = %u, baseMipLevel = %u, levelCount = %u, "
    "baseArrayLayer = %u, layerCount = %u }",
    value.aspectMask, value.baseMipLevel, value.levelCount, value.baseArrayLayer, value.layerCount);
}

TSPEC String FaultReportDump::paramToString(const VkClearDepthStencilValue &value)
{
  return String(32, "VkClearDepthStencilValue{ depth = %f, stencil = %u }", value.depth, value.stencil);
}

TSPEC String FaultReportDump::paramToString(const VkImageBlit &value)
{
  return String(32,
    "VkImageBlit{ VkImageSubresourceLayers{ aspectMask = %u, mipLevel = %u, "
    "baseArrayLayer = %u, layerCount = %u },VkOffset3D[0]{ x = %u, y = %u, z = %u "
    "},VkOffset3D[1]{ x = %u, y = %u, z = %u },VkImageSubresourceLayers{ aspectMask = %u, "
    "mipLevel = %u, baseArrayLayer = %u, layerCount = %u },VkOffset3D[0]{ x = %u, y = %u, z "
    "= %u },VkOffset3D[1]{ x = %u, y = %u, z = %u } }",
    value.srcSubresource.aspectMask, value.srcSubresource.mipLevel, value.srcSubresource.baseArrayLayer,
    value.srcSubresource.layerCount, value.srcOffsets[0].x, value.srcOffsets[0].y, value.srcOffsets[0].z, value.srcOffsets[1].x,
    value.srcOffsets[1].y, value.srcOffsets[1].z, value.dstSubresource.aspectMask, value.dstSubresource.mipLevel,
    value.dstSubresource.baseArrayLayer, value.dstSubresource.layerCount, value.dstOffsets[0].x, value.dstOffsets[0].y,
    value.dstOffsets[0].z, value.dstOffsets[1].x, value.dstOffsets[1].y, value.dstOffsets[1].z);
}

TSPEC String FaultReportDump::paramToString(const VkClearColorValue &value)
{
  return String(64,
    "\nVkClearColorValue.float32 { %f, %f, %f, %f }\n"
    "VkClearColorValue.int32 { %i, %i, %i, %i }\n"
    "VkClearColorValue.uint32 { %u, %u, %u, %u }\n",
    value.float32[0], value.float32[1], value.float32[2], value.float32[3], value.int32[0], value.int32[1], value.int32[2],
    value.int32[3], value.uint32[0], value.uint32[1], value.uint32[2], value.uint32[3]);
}

#define ENUM_TO_NAME(Name) \
  case Name: enumName = #Name; break

TSPEC String FaultReportDump::paramToString(const VkPrimitiveTopology &top) { return String(32, "%s", formatPrimitiveTopology(top)); }

TSPEC String FaultReportDump::paramToString(const VkCompareOp &value)
{
  const char *enumName = "<unknown>";
  switch (value)
  {
    ENUM_TO_NAME(VK_COMPARE_OP_NEVER);
    ENUM_TO_NAME(VK_COMPARE_OP_LESS);
    ENUM_TO_NAME(VK_COMPARE_OP_EQUAL);
    ENUM_TO_NAME(VK_COMPARE_OP_LESS_OR_EQUAL);
    ENUM_TO_NAME(VK_COMPARE_OP_GREATER);
    ENUM_TO_NAME(VK_COMPARE_OP_NOT_EQUAL);
    ENUM_TO_NAME(VK_COMPARE_OP_GREATER_OR_EQUAL);
    ENUM_TO_NAME(VK_COMPARE_OP_ALWAYS);
    default: break; // silence stupid -Wswitch
  }
  return String(32, "%s", enumName);
}

TSPEC String FaultReportDump::paramToString(const VkBlendOp &value)
{
  const char *enumName = "<unknown>";
  switch (value)
  {
    ENUM_TO_NAME(VK_BLEND_OP_ADD);
    ENUM_TO_NAME(VK_BLEND_OP_SUBTRACT);
    ENUM_TO_NAME(VK_BLEND_OP_REVERSE_SUBTRACT);
    ENUM_TO_NAME(VK_BLEND_OP_MIN);
    ENUM_TO_NAME(VK_BLEND_OP_MAX);
    default: break; // silence stupid -Wswitch
  }
  return String(32, "%s", enumName);
}

TSPEC String FaultReportDump::paramToString(const VkBlendFactor &value)
{
  const char *enumName = "<unknown>";
  switch (value)
  {
    ENUM_TO_NAME(VK_BLEND_FACTOR_ZERO);
    ENUM_TO_NAME(VK_BLEND_FACTOR_ONE);
    ENUM_TO_NAME(VK_BLEND_FACTOR_SRC_COLOR);
    ENUM_TO_NAME(VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR);
    ENUM_TO_NAME(VK_BLEND_FACTOR_DST_COLOR);
    ENUM_TO_NAME(VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR);
    ENUM_TO_NAME(VK_BLEND_FACTOR_SRC_ALPHA);
    ENUM_TO_NAME(VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    ENUM_TO_NAME(VK_BLEND_FACTOR_DST_ALPHA);
    ENUM_TO_NAME(VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA);
    ENUM_TO_NAME(VK_BLEND_FACTOR_CONSTANT_COLOR);
    ENUM_TO_NAME(VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR);
    ENUM_TO_NAME(VK_BLEND_FACTOR_CONSTANT_ALPHA);
    ENUM_TO_NAME(VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA);
    ENUM_TO_NAME(VK_BLEND_FACTOR_SRC_ALPHA_SATURATE);
    ENUM_TO_NAME(VK_BLEND_FACTOR_SRC1_COLOR);
    ENUM_TO_NAME(VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR);
    ENUM_TO_NAME(VK_BLEND_FACTOR_SRC1_ALPHA);
    ENUM_TO_NAME(VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA);
    default: break; // silence stupid -Wswitch
  }
  return String(32, "%s", enumName);
}

TSPEC String FaultReportDump::paramToString(const VkStencilOp &value)
{
  const char *enumName = "<unknown>";
  switch (value)
  {
    ENUM_TO_NAME(VK_STENCIL_OP_KEEP);
    ENUM_TO_NAME(VK_STENCIL_OP_ZERO);
    ENUM_TO_NAME(VK_STENCIL_OP_REPLACE);
    ENUM_TO_NAME(VK_STENCIL_OP_INCREMENT_AND_CLAMP);
    ENUM_TO_NAME(VK_STENCIL_OP_DECREMENT_AND_CLAMP);
    ENUM_TO_NAME(VK_STENCIL_OP_INVERT);
    ENUM_TO_NAME(VK_STENCIL_OP_INCREMENT_AND_WRAP);
    ENUM_TO_NAME(VK_STENCIL_OP_DECREMENT_AND_WRAP);
    default: break; // silence stupid -Wswitch
  }
  return String(32, "%s", enumName);
}

TSPEC String FaultReportDump::paramToString(const VkIndexType &value)
{
  const char *enumName = "<unknown>";
  switch (value)
  {
    ENUM_TO_NAME(VK_INDEX_TYPE_UINT16);
    ENUM_TO_NAME(VK_INDEX_TYPE_UINT32);
    default: break; // silence stupid -Wswitch
  }
  return String(32, "%s", enumName);
}

#undef ENUM_TO_NAME

TSPEC String FaultReportDump::paramToString(const ImageViewState &value)
{
  return String(32,
    "ImageViewState{ sampleStencil = %u, isRenderTarget = %u, isCubemap = %u, isArray = "
    "%u, format = %s, mipMapBase = %u, mipMapCount = %u, arrayBase = %u, arrayCount = %u }",
    asBooleanString(value.sampleStencil), asBooleanString(value.isRenderTarget), asBooleanString(value.isCubemap),
    asBooleanString(value.isArray), value.getFormat().getNameString(), value.getMipBase(), value.getMipCount(), value.getArrayBase(),
    value.getArrayCount());
}

TSPEC String FaultReportDump::paramToString(const VulkanImageViewHandle &value) { return String(32, "%u", value.value); }

TSPEC String FaultReportDump::paramToStringPtr(const RenderPassResource *value)
{
  addRef(currentCommand, FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)value);
  return String(32, "0x%p", value);
}

TSPEC String FaultReportDump::paramToStringPtr(const Buffer *value)
{
  addRef(currentCommand, FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)value);
  return String(32, "0x%p", value);
}

TSPEC String FaultReportDump::paramToStringPtr(const Image *value)
{
  addRef(currentCommand, FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)value);
  return String(32, "0x%p", value);
}

TSPEC String FaultReportDump::paramToStringPtr(const InputLayout *value) { return String(32, "0x%p", value); }

TSPEC String FaultReportDump::paramToStringPtr(const ComputePipeline *value) { return String(32, "0x%p", value); }

TSPEC String FaultReportDump::paramToStringPtr(const GraphicsProgram *value) { return String(32, "0x%p", value); }

TSPEC String FaultReportDump::paramToStringPtr(const ThreadedFence *value) { return String(32, "0x%p", value); }

#if VULKAN_HAS_RAYTRACING

TSPEC String FaultReportDump::paramToStringPtr(const RaytraceAccelerationStructure *value)
{
  addRef(currentCommand, FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)value);
  return String(32, "0x%p", value);
}

TSPEC String FaultReportDump::paramToStringPtr(const RaytracePipeline *value) { return String(32, "0x%p", value); }

TSPEC String FaultReportDump::paramToString(const RaytraceBuildFlags &value) { return String(32, "0x%X", (uint32_t)value); }

#endif

TSPEC String FaultReportDump::paramToString(const ShaderModuleUse &smu)
{
  // TODO: debug print header...
  return String(32, "{<header>, %u}", /*smu.header, */ smu.blobId);
}

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA

TSPEC String FaultReportDump::paramToString(const ShaderDebugInfo &sdi)
{
  return String(128,
    "shader debug attachment\n"
    ">> name = %s\n"
    ">> spirvDisAsm = %s\n"
    ">> dxbcDisAsm = %s\n"
    ">> sourceGLSL = %s\n"
    ">> reconstructedHLSL = %s\n"
    ">> reconstructedHLSLAndSourceHLSLXDif = %s\n"
    ">> sourceHLSL = %s\n"
    ">> debugName = %s",
    sdi.name, sdi.spirvDisAsm, sdi.dxbcDisAsm, sdi.sourceGLSL, sdi.reconstructedHLSL, sdi.reconstructedHLSLAndSourceHLSLXDif,
    sdi.sourceHLSL, sdi.debugName);
}
#endif

TSPEC String FaultReportDump::paramToString(const shaders::DriverRenderStateId &val)
{
  uint32_t handle = (uint32_t)val;
  return String(32, "shaders::DriverRenderStateId %u", handle);
}

TSPEC String FaultReportDump::paramToString(const InputLayoutID &val) { return String(32, "InputLayoutID %u", val.get()); }

TSPEC String FaultReportDump::paramToString(const FSRUpscalingArgs &args)
{
  return String(0,
    "FSRUpscaling\n"
    ">>colorTexture: 0x%p\n"
    ">>depthTexture: 0x%p\n"
    ">>motionVectors: 0x%p\n"
    ">>exposureTexture: 0x%p\n"
    ">>reactiveTexture: 0x%p\n"
    ">>transparencyAndCompositionTexture: 0x%p\n"
    ">>outputTexture: 0x%p\n",
    args.colorTexture, args.depthTexture, args.motionVectors, args.exposureTexture, args.reactiveTexture,
    args.transparencyAndCompositionTexture, args.outputTexture);
}

TSPEC String FaultReportDump::paramToString(const nv::DlssParams<Image> &args)
{
  return String(0,
    "nv::DlssParams\n"
    ">>colorTexture: 0x%p\n"
    ">>depthTexture: 0x%p\n"
    ">>motionVectors: 0x%p\n"
    ">>exposureTexture: 0x%p\n"
    ">>outputTexture: 0x%p\n",
    args.inColor, args.inDepth, args.inMotionVectors, args.inExposure, args.outColor);
}

TSPEC String FaultReportDump::paramToString(const nv::DlssGParams<Image> &args)
{
  return String(0,
    "nv::DlssGParams\n"
    ">>inHUDless: 0x%p\n"
    ">>inUI: 0x%p\n"
    ">>inDepth: 0x%p\n"
    ">>inMotionVectors: 0x%p\n",
    args.inHUDless, args.inUI, args.inDepth, args.inMotionVectors);
}

TSPEC String FaultReportDump::paramToString(const VkBufferImageCopy &val)
{
  return String(128,
    "VkBufferImageCopy\n"
    ">>bufferOffset = %llu\n"
    ">>bufferRowLength = %u\n"
    ">>bufferImageHeight = %u\n"
    ">>imageOffset %u %u %u\n"
    ">>imageExtent %u %u %u\n"
    ">>imageSubresource = mip %u baseLayer %u layers %u aspect %lu",
    val.bufferOffset, val.bufferRowLength, val.bufferImageHeight, val.imageOffset.x, val.imageOffset.y, val.imageOffset.z,
    val.imageExtent.width, val.imageExtent.height, val.imageExtent.depth, val.imageSubresource.mipLevel,
    val.imageSubresource.baseArrayLayer, val.imageSubresource.layerCount, val.imageSubresource.aspectMask);
}

TSPEC String FaultReportDump::paramToString(const URegister &uReg)
{
  if (uReg.image)
    addRef(currentCommand, FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)uReg.image);
  if (uReg.buffer.buffer)
    addRef(currentCommand, FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)uReg.buffer.buffer);
  return String(32, "img %p buf %p", uReg.image, uReg.buffer.buffer);
}

TSPEC String FaultReportDump::paramToString(const TRegister &tReg)
{
  switch (tReg.type)
  {
    case TRegister::TYPE_IMG:
      addRef(currentCommand, FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)tReg.img.ptr);
      return String(64, "img %p %s %s", tReg.img.ptr);
    case TRegister::TYPE_BUF:
      addRef(currentCommand, FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)tReg.buf.buffer);
      return String(64, "buf %p\n", tReg.buf.buffer);
#if VULKAN_HAS_RAYTRACING
    case TRegister::TYPE_AS:
      addRef(currentCommand, FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)tReg.rtas);
      return String(64, "as %p", tReg.rtas);
#endif
    default: return String(32, "empty");
  }
}

TSPEC String FaultReportDump::paramToString(const SRegister &sReg)
{
  addRef(currentCommand, FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)sReg.resPtr);
  return String(32, "spl res %p", sReg.resPtr);
}

TSPEC String FaultReportDump::paramToString(const RenderPassArea &area)
{
  return String(32, "RPArea [%u, %u] - [%u, %u] Z [%f, %f]", area.left, area.top, area.width, area.height, area.minZ, area.maxZ);
}

TSPEC String FaultReportDump::paramToString(const AliasedResourceMemory &mem_alias)
{
  return String(64,
    "AliasedResourceMemory {"
    "deviceMemory=%s, "
    "handle=0x%016llX, "
    "offset=0x%016llX, "
    "size=0x%016llX}",
    mem_alias.deviceMemory ? "y" : "n", mem_alias.handle, mem_alias.offset, mem_alias.size);
}

TSPEC String FaultReportDump::paramToString(const lowlatency::LatencyMarkerType &value) { return String(16, "%u", (int)value); }
