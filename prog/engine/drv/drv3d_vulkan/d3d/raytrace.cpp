// Copyright (C) Gaijin Games KFT.  All rights reserved.

// raytrace features implementation
#include <drv/3d/rayTrace/dag_drvRayTrace.h>
#include "vulkan_api.h"

#if !(D3D_HAS_RAY_TRACING && (VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query))
#error "raytrace interface implementation used when stub must be used"
#endif

#include "frontend.h"
#include "globals.h"
#include "buffer.h"
#include "vulkan_device.h"
#include "device_context.h"
#include "raytrace_scratch_buffer.h"
#include "raytrace_as_resource.h"
#include "resource_manager.h"
#include "vk_format_utils.h"

using namespace drv3d_vulkan;

namespace
{
RaytraceBLASBufferRefs getRaytraceGeometryRefs(const RaytraceGeometryDescription &desc)
{
  switch (desc.type)
  {
    case RaytraceGeometryDescription::Type::TRIANGLES:
    {
      const BufferRef &devVbuf = ((GenericBufferInterface *)desc.data.triangles.vertexBuffer)->getBufferRef();
      uint32_t vofs = desc.data.triangles.vertexOffset * desc.data.triangles.vertexStride;

      RaytraceBLASBufferRefs refs = {};
      refs.geometry = devVbuf.buffer;
      refs.geometryOffset = (uint32_t)devVbuf.bufOffset(vofs);
      refs.geometrySize = (uint32_t)devVbuf.visibleDataSize - vofs;

      if (desc.data.triangles.indexBuffer)
      {
        const BufferRef &devIbuf = ((GenericBufferInterface *)desc.data.triangles.indexBuffer)->getBufferRef();
        uint32_t indexSize =
          ((GenericBufferInterface *)desc.data.triangles.indexBuffer)->getIndexType() == VkIndexType::VK_INDEX_TYPE_UINT32 ? 4 : 2;
        refs.index = devIbuf.buffer;
        refs.indexOffset = (uint32_t)devIbuf.bufOffset(desc.data.triangles.indexOffset * indexSize);
        refs.indexSize = desc.data.triangles.indexCount * indexSize;
      }

      if (desc.data.triangles.transformBuffer)
      {
        const BufferRef &devTbuf = ((GenericBufferInterface *)desc.data.triangles.transformBuffer)->getBufferRef();
        refs.transform = devTbuf.buffer;
        // offset must be passed to keep proper discard index reference
        refs.transformOffset = (uint32_t)devTbuf.bufOffset(desc.data.triangles.transformOffset * sizeof(float) * 3 * 4);
      }

      return refs;
    }
    case RaytraceGeometryDescription::Type::AABBS:
    {
      const BufferRef &devBuf = ((GenericBufferInterface *)desc.data.aabbs.buffer)->getBufferRef();
      return {devBuf.buffer, (uint32_t)devBuf.bufOffset(desc.data.aabbs.offset), desc.data.aabbs.stride * desc.data.aabbs.count,
        nullptr, 0, 0};
    }
    default: G_ASSERTF(0, "vulkan: unknown geometry type %u in RaytraceGeometryDescription", (uint32_t)desc.type);
  }
  return {nullptr, 0, 0, nullptr, 0, 0};
}
} // namespace

#define DEF_DESCRIPTOR_RAYTRACE                                                                    \
  DEF_DESCRIPTOR("raytrace", VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_IMAGE_VIEW_TYPE_1D, \
    spirv::T_ACCELERATION_STRUCTURE_OFFSET, false),

bool isRaytraceAcclerationStructure(VkDescriptorType descType) { return descType == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR; }

VkAccelerationStructureBuildSizesInfoKHR getSizeByDriverDesc(RaytraceGeometryDescription *geometry, uint32_t count,
  RaytraceBuildFlags flags)
{
  VulkanDevice &dev = Globals::VK::dev;

  eastl::vector<VkAccelerationStructureGeometryKHR> geometryDef;
  eastl::vector<uint32_t> maxPrimCounts;
  const bool isTopAS = (geometry == nullptr);
  if (!isTopAS)
  {
    geometryDef.reserve(count);
    maxPrimCounts.reserve(count);
    for (uint32_t i = 0; i < count; ++i)
    {
      geometryDef.push_back(RaytraceGeometryDescriptionToVkAccelerationStructureGeometryKHR(geometry[i]));
      maxPrimCounts.push_back(geometry[i].data.triangles.indexCount / 3);
    }
  }
  else
  {
    VkAccelerationStructureGeometryKHR &tlasGeo = geometryDef.emplace_back();
    tlasGeo = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    tlasGeo.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    tlasGeo.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    maxPrimCounts.push_back(count);
  }

  VkAccelerationStructureBuildGeometryInfoKHR buildInfo = //
    {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
  buildInfo.type = isTopAS ? VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR : VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  buildInfo.flags = ToVkBuildAccelerationStructureFlagsKHR(flags);
  buildInfo.geometryCount = uint32_t(geometryDef.size());
  buildInfo.pGeometries = geometryDef.data();

  VkAccelerationStructureBuildSizesInfoKHR size_info = //
    {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
  dev.vkGetAccelerationStructureBuildSizesKHR(dev.get(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo,
    maxPrimCounts.data(), &size_info);

  // TODO: remove this when all code migrate to user scratch buffers
  Globals::rtScratchBuffer.ensureRoomForBuild(size_info.buildScratchSize);

  return size_info;
}

RaytraceBottomAccelerationStructure *d3d::create_raytrace_bottom_acceleration_structure(RaytraceGeometryDescription *desc,
  uint32_t count, RaytraceBuildFlags flags, uint32_t &build_scratch_size_in_bytes, uint32_t *update_scratch_size_in_bytes)
{
  VkAccelerationStructureBuildSizesInfoKHR sizes = getSizeByDriverDesc(desc, count, flags);
  build_scratch_size_in_bytes = sizes.buildScratchSize;
  if (update_scratch_size_in_bytes)
    *update_scratch_size_in_bytes = sizes.updateScratchSize;
  return (RaytraceBottomAccelerationStructure *)RaytraceAccelerationStructure::create(false /*topLevel*/,
    sizes.accelerationStructureSize);
}

RaytraceBottomAccelerationStructure *d3d::create_raytrace_bottom_acceleration_structure(uint32_t size)
{
  return (RaytraceBottomAccelerationStructure *)RaytraceAccelerationStructure::create(false /*topLevel*/, size);
}

void d3d::delete_raytrace_bottom_acceleration_structure(RaytraceBottomAccelerationStructure *as)
{
  Globals::ctx.deleteRaytraceBottomAccelerationStructure(as);
}

RaytraceTopAccelerationStructure *d3d::create_raytrace_top_acceleration_structure(uint32_t elements, RaytraceBuildFlags flags,
  uint32_t &build_scratch_size_in_bytes, uint32_t *update_scratch_size_in_bytes)
{
  VkAccelerationStructureBuildSizesInfoKHR sizes = getSizeByDriverDesc(nullptr, elements, flags);
  build_scratch_size_in_bytes = sizes.buildScratchSize;
  if (update_scratch_size_in_bytes)
    *update_scratch_size_in_bytes = sizes.updateScratchSize;
  return (RaytraceTopAccelerationStructure *)RaytraceAccelerationStructure::create(true /*topLevel*/, sizes.accelerationStructureSize);
}

void d3d::delete_raytrace_top_acceleration_structure(RaytraceTopAccelerationStructure *as)
{
  Globals::ctx.deleteRaytraceTopAccelerationStructure(as);
}

void d3d::set_top_acceleration_structure(ShaderStage stage, uint32_t index, RaytraceTopAccelerationStructure *as)
{
  PipelineState &pipeState = Frontend::State::pipe;
  auto &resBinds = pipeState.getStageResourceBinds((ShaderStage)stage);
  TRegister tReg((RaytraceAccelerationStructure *)as);
  if (resBinds.set<StateFieldTRegisterSet, StateFieldTRegister::Indexed>({index, tReg}))
    pipeState.markResourceBindDirty((ShaderStage)stage);
}

void d3d::build_bottom_acceleration_structure(RaytraceBottomAccelerationStructure *as,
  const ::raytrace::BottomAccelerationStructureBuildInfo &basbi)
{
  BufferRef scratchRef{};
  if (basbi.scratchSpaceBuffer)
  {
    auto scratchSpaceBuffer = (GenericBufferInterface *)basbi.scratchSpaceBuffer;
    G_ASSERTF_RETURN(SBCF_USAGE_ACCELLERATION_STRUCTURE_BUILD_SCRATCH_SPACE & scratchSpaceBuffer->getFlags(), ,
      "vulkan: build_bottom_acceleration_structure: scratchSpaceBuffer must be created with the "
      "SBCF_USAGE_ACCELLERATION_STRUCTURE_BUILD_SCRATCH_SPACE flags set");
    // more validation needed

    scratchRef = scratchSpaceBuffer->getBufferRef();
    scratchRef.addOffset(basbi.scratchSpaceBufferOffsetInBytes);
  }
  else
    scratchRef = BufferRef(Globals::rtScratchBuffer.get());

  BufferRef compactionSizeBuffer{};
  if (RaytraceBuildFlags::ALLOW_COMPACTION == (basbi.flags & RaytraceBuildFlags::ALLOW_COMPACTION))
  {
    auto compactedSizeOutputBuffer = (GenericBufferInterface *)basbi.compactedSizeOutputBuffer;
    G_ASSERTF_RETURN(compactedSizeOutputBuffer, , "vulkan: Compacted size buffer must be provided when compaction is enabled");
    G_ASSERTF_RETURN(compactedSizeOutputBuffer->getFlags() & SBCF_BIND_UNORDERED, , "vulkan: Compacted size buffer must have an UAV");
    G_ASSERTF_RETURN(compactedSizeOutputBuffer->getElementSize() == sizeof(uint64_t), ,
      "vulkan: Compacted size buffer must have element size %u (provided %u)", sizeof(uint64_t),
      compactedSizeOutputBuffer->getElementSize());
    G_ASSERTF_RETURN(basbi.compactedSizeOutputBufferOffsetInBytes % sizeof(uint64_t) == 0, ,
      "vulkan: Compacted size buffer must have offset aligned to %u (provided %u)", sizeof(uint64_t),
      basbi.compactedSizeOutputBufferOffsetInBytes);

    compactionSizeBuffer = compactedSizeOutputBuffer->getBufferRef();
    compactionSizeBuffer.addOffset(basbi.compactedSizeOutputBufferOffsetInBytes);
  }
  else
  {
    G_ASSERTF_RETURN(!basbi.compactedSizeOutputBuffer, ,
      "vulkan: Compacted size buffer should not be provided when compaction is disabled");
  }

  OSSpinlockScopedLock lockedFront(Globals::ctx.getFrontLock());
  CmdRaytraceBuildStructures cmd{Frontend::replay->raytraceStructureBuildStore.size(), 1};
  Frontend::replay->raytraceStructureBuildStore.push_back_uninitialized();

  RaytraceStructureBuildData &bd = Frontend::replay->raytraceStructureBuildStore.back();
  bd.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  bd.flags = ToVkBuildAccelerationStructureFlagsKHR(basbi.flags);
  bd.update = basbi.doUpdate;
  bd.dst = (RaytraceAccelerationStructure *)as;
  bd.scratchBuf = scratchRef;
  bd.blas.geometryCount = basbi.geometryDescCount;
  bd.blas.firstGeometry = uint32_t(Frontend::replay->raytraceGeometryKHRStore.size());
  bd.blas.compactionSizeBuffer = compactionSizeBuffer;

  G_ASSERT(Frontend::replay->raytraceGeometryKHRStore.size() == Frontend::replay->raytraceBuildRangeInfoKHRStore.size());
  for (uint32_t i = 0; i < basbi.geometryDescCount; ++i)
  {
    uint32_t primitiveOffset = 0;
    const auto *ibuf = (const GenericBufferInterface *)basbi.geometryDesc[i].data.triangles.indexBuffer;
    if (ibuf)
    {
      const VkIndexType indexType = ibuf->getIndexType();
      if (indexType == VkIndexType::VK_INDEX_TYPE_UINT32)
      {
        primitiveOffset = basbi.geometryDesc[i].data.triangles.indexOffset * 4;
      }
      else
      {
        G_ASSERT(indexType == VkIndexType::VK_INDEX_TYPE_UINT16);
        primitiveOffset = basbi.geometryDesc[i].data.triangles.indexOffset * 2;
      }
    }
    Frontend::replay->raytraceBuildRangeInfoKHRStore.push_back({
      basbi.geometryDesc[i].data.triangles.indexCount / 3, // primitiveCount
      primitiveOffset,
      0, // firstVertex
      0  // transformOffset
    });
    Frontend::replay->raytraceGeometryKHRStore.push_back(
      RaytraceGeometryDescriptionToVkAccelerationStructureGeometryKHR(basbi.geometryDesc[i]));
    Frontend::replay->raytraceBLASBufferRefsStore.push_back(getRaytraceGeometryRefs(basbi.geometryDesc[i]));
  }
  Globals::ctx.dispatchCommandNoLock(cmd);
}

void d3d::build_bottom_acceleration_structures(::raytrace::BatchedBottomAccelerationStructureBuildInfo *as_array, uint32_t as_count)
{
  auto asSpan = make_span(as_array, as_count);
  for (::raytrace::BatchedBottomAccelerationStructureBuildInfo &itr : asSpan)
    G_ASSERTF_RETURN(itr.basbi.scratchSpaceBuffer, , "vulkan: This API requires providing a scatch buffer for the AS builds");

  OSSpinlockScopedLock lockedFront(Globals::ctx.getFrontLock());
  CmdRaytraceBuildStructures cmd{Frontend::replay->raytraceStructureBuildStore.size(), as_count};
  for (::raytrace::BatchedBottomAccelerationStructureBuildInfo &itr : asSpan)
  {
    const ::raytrace::BottomAccelerationStructureBuildInfo &basbi = itr.basbi;
    auto scratchSpaceBuffer = (GenericBufferInterface *)basbi.scratchSpaceBuffer;
    G_ASSERTF_RETURN(SBCF_USAGE_ACCELLERATION_STRUCTURE_BUILD_SCRATCH_SPACE & scratchSpaceBuffer->getFlags(), ,
      "vulkan: build_bottom_acceleration_structure: scratchSpaceBuffer must be created with the "
      "SBCF_USAGE_ACCELLERATION_STRUCTURE_BUILD_SCRATCH_SPACE flags set");
    // more validation needed

    BufferRef scratchRef = scratchSpaceBuffer->getBufferRef();
    scratchRef.addOffset(basbi.scratchSpaceBufferOffsetInBytes);

    BufferRef compactionSizeBuffer{};
    if (RaytraceBuildFlags::ALLOW_COMPACTION == (basbi.flags & RaytraceBuildFlags::ALLOW_COMPACTION))
    {
      auto compactedSizeOutputBuffer = (GenericBufferInterface *)basbi.compactedSizeOutputBuffer;
      G_ASSERTF_RETURN(compactedSizeOutputBuffer, , "vulkan: Compacted size buffer must be provided when compaction is enabled");
      G_ASSERTF_RETURN(compactedSizeOutputBuffer->getFlags() & SBCF_BIND_UNORDERED, ,
        "vulkan: Compacted size buffer must have an UAV");
      G_ASSERTF_RETURN(compactedSizeOutputBuffer->getElementSize() == sizeof(uint64_t), ,
        "vulkan: Compacted size buffer must have element size %u (provided %u)", sizeof(uint64_t),
        compactedSizeOutputBuffer->getElementSize());
      G_ASSERTF_RETURN(basbi.compactedSizeOutputBufferOffsetInBytes % sizeof(uint64_t) == 0, ,
        "vulkan: Compacted size buffer must have offset aligned to %u (provided %u)", sizeof(uint64_t),
        basbi.compactedSizeOutputBufferOffsetInBytes);

      compactionSizeBuffer = compactedSizeOutputBuffer->getBufferRef();
      compactionSizeBuffer.addOffset(basbi.compactedSizeOutputBufferOffsetInBytes);
    }
    else
    {
      G_ASSERTF_RETURN(!basbi.compactedSizeOutputBuffer, ,
        "vulkan: Compacted size buffer should not be provided when compaction is disabled");
    }

    Frontend::replay->raytraceStructureBuildStore.push_back_uninitialized();

    RaytraceStructureBuildData &bd = Frontend::replay->raytraceStructureBuildStore.back();
    bd.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    bd.flags = ToVkBuildAccelerationStructureFlagsKHR(basbi.flags);
    bd.update = basbi.doUpdate;
    bd.dst = (RaytraceAccelerationStructure *)itr.as;
    bd.scratchBuf = scratchRef;
    bd.blas.geometryCount = basbi.geometryDescCount;
    bd.blas.firstGeometry = uint32_t(Frontend::replay->raytraceGeometryKHRStore.size());
    bd.blas.compactionSizeBuffer = compactionSizeBuffer;

    G_ASSERT(Frontend::replay->raytraceGeometryKHRStore.size() == Frontend::replay->raytraceBuildRangeInfoKHRStore.size());
    for (uint32_t i = 0; i < basbi.geometryDescCount; ++i)
    {
      uint32_t primitiveOffset = 0;
      const auto *ibuf = (const GenericBufferInterface *)basbi.geometryDesc[i].data.triangles.indexBuffer;
      if (ibuf)
      {
        const VkIndexType indexType = ibuf->getIndexType();
        if (indexType == VkIndexType::VK_INDEX_TYPE_UINT32)
        {
          primitiveOffset = basbi.geometryDesc[i].data.triangles.indexOffset * 4;
        }
        else
        {
          G_ASSERT(indexType == VkIndexType::VK_INDEX_TYPE_UINT16);
          primitiveOffset = basbi.geometryDesc[i].data.triangles.indexOffset * 2;
        }
      }
      Frontend::replay->raytraceBuildRangeInfoKHRStore.push_back({
        basbi.geometryDesc[i].data.triangles.indexCount / 3, // primitiveCount
        primitiveOffset,
        0, // firstVertex
        0  // transformOffset
      });
      Frontend::replay->raytraceGeometryKHRStore.push_back(
        RaytraceGeometryDescriptionToVkAccelerationStructureGeometryKHR(basbi.geometryDesc[i]));
      Frontend::replay->raytraceBLASBufferRefsStore.push_back(getRaytraceGeometryRefs(basbi.geometryDesc[i]));
    }
  }
  Globals::ctx.dispatchCommandNoLock(cmd);
}

void d3d::build_top_acceleration_structure(RaytraceTopAccelerationStructure *as,
  const ::raytrace::TopAccelerationStructureBuildInfo &tasbi)
{
  BufferRef scratchRef{};
  if (tasbi.scratchSpaceBuffer)
  {
    auto scratchSpaceBuffer = (GenericBufferInterface *)tasbi.scratchSpaceBuffer;
    G_ASSERTF_RETURN(SBCF_USAGE_ACCELLERATION_STRUCTURE_BUILD_SCRATCH_SPACE & scratchSpaceBuffer->getFlags(), ,
      "vulkan: build_bottom_acceleration_structure: scratchSpaceBuffer must be created with the "
      "SBCF_USAGE_ACCELLERATION_STRUCTURE_BUILD_SCRATCH_SPACE flags set");
    // more validation needed

    scratchRef = scratchSpaceBuffer->getBufferRef();
    scratchRef.addOffset(tasbi.scratchSpaceBufferOffsetInBytes);
  }
  else
    scratchRef = BufferRef(Globals::rtScratchBuffer.get());

  auto vkBuf = (GenericBufferInterface *)tasbi.instanceBuffer;
  const BufferRef &instanceBufRef = vkBuf->getBufferRef();


  OSSpinlockScopedLock lockedFront(Globals::ctx.getFrontLock());
  CmdRaytraceBuildStructures cmd{Frontend::replay->raytraceStructureBuildStore.size(), 1};

  Frontend::replay->raytraceStructureBuildStore.push_back_uninitialized();
  RaytraceStructureBuildData &bd = Frontend::replay->raytraceStructureBuildStore.back();
  bd.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  bd.flags = ToVkBuildAccelerationStructureFlagsKHR(tasbi.flags);
  bd.update = tasbi.doUpdate;
  bd.dst = (RaytraceAccelerationStructure *)as;
  bd.scratchBuf = scratchRef;
  bd.tlas.instanceCount = tasbi.instanceCount;
  bd.tlas.instanceBuffer = instanceBufRef;

  Globals::ctx.dispatchCommandNoLock(cmd);
}

void d3d::build_top_acceleration_structures(::raytrace::BatchedTopAccelerationStructureBuildInfo *as_array, uint32_t as_count)
{
  auto asSpan = make_span(as_array, as_count);
  for (::raytrace::BatchedTopAccelerationStructureBuildInfo &itr : asSpan)
    G_ASSERTF_RETURN(itr.tasbi.scratchSpaceBuffer, , "vulkan: This API requires providing a scatch buffer for the AS builds");

  OSSpinlockScopedLock lockedFront(Globals::ctx.getFrontLock());
  CmdRaytraceBuildStructures cmd{Frontend::replay->raytraceStructureBuildStore.size(), as_count};
  for (::raytrace::BatchedTopAccelerationStructureBuildInfo &itr : asSpan)
  {
    const ::raytrace::TopAccelerationStructureBuildInfo &tasbi = itr.tasbi;
    auto scratchSpaceBuffer = (GenericBufferInterface *)tasbi.scratchSpaceBuffer;
    G_ASSERTF_RETURN(SBCF_USAGE_ACCELLERATION_STRUCTURE_BUILD_SCRATCH_SPACE & scratchSpaceBuffer->getFlags(), ,
      "vulkan: build_bottom_acceleration_structure: scratchSpaceBuffer must be created with the "
      "SBCF_USAGE_ACCELLERATION_STRUCTURE_BUILD_SCRATCH_SPACE flags set");
    // more validation needed

    BufferRef scratchRef = scratchSpaceBuffer->getBufferRef();
    scratchRef.addOffset(tasbi.scratchSpaceBufferOffsetInBytes);
    auto vkBuf = (GenericBufferInterface *)tasbi.instanceBuffer;
    const BufferRef &instanceBufRef = vkBuf->getBufferRef();

    Frontend::replay->raytraceStructureBuildStore.push_back_uninitialized();
    RaytraceStructureBuildData &bd = Frontend::replay->raytraceStructureBuildStore.back();
    bd.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    bd.flags = ToVkBuildAccelerationStructureFlagsKHR(tasbi.flags);
    bd.update = tasbi.doUpdate;
    bd.dst = (RaytraceAccelerationStructure *)itr.as;
    bd.scratchBuf = scratchRef;
    bd.tlas.instanceCount = tasbi.instanceCount;
    bd.tlas.instanceBuffer = instanceBufRef;
  }
  Globals::ctx.dispatchCommandNoLock(cmd);
}

void d3d::write_raytrace_index_entries_to_memory(uint32_t count, const RaytraceGeometryInstanceDescription *desc, void *ptr)
{
  G_STATIC_ASSERT(sizeof(VkAccelerationStructureInstanceKHR) == sizeof(RaytraceGeometryInstanceDescription));
  // just copy everything over, they are bit compatible (except for the struct ptr)
  memcpy(ptr, desc, count * sizeof(VkAccelerationStructureInstanceKHR));
  // now fixup the struct ptr
  auto tptr = reinterpret_cast<VkAccelerationStructureInstanceKHR *>(ptr);
  for (uint32_t i = 0; i < count; ++i)
    tptr[i].accelerationStructureReference =
      ((RaytraceAccelerationStructure *)desc[i].accelerationStructure)->getDeviceAddress(Globals::VK::dev);
}

uint64_t d3d::get_raytrace_acceleration_structure_size(RaytraceAnyAccelerationStructure as)
{
  if (as.top)
    return reinterpret_cast<RaytraceAccelerationStructure *>(as.top)->getMemory().size;
  if (as.bottom)
    return reinterpret_cast<RaytraceAccelerationStructure *>(as.bottom)->getMemory().size;
  return 0;
}

RaytraceAccelerationStructureGpuHandle d3d::get_raytrace_acceleration_structure_gpu_handle(RaytraceAnyAccelerationStructure as)
{
  if (as.top)
    return {reinterpret_cast<RaytraceAccelerationStructure *>(as.top)->getDeviceAddress(Globals::VK::dev)};
  if (as.bottom)
    return {reinterpret_cast<RaytraceAccelerationStructure *>(as.bottom)->getDeviceAddress(Globals::VK::dev)};
  return {0};
}

void d3d::copy_raytrace_acceleration_structure(RaytraceAnyAccelerationStructure dst, RaytraceAnyAccelerationStructure src,
  bool compact)
{
  G_ASSERT_RETURN((dst.bottom && src.bottom) || (dst.top && src.top), );

  auto rsrc = src.top ? reinterpret_cast<RaytraceAccelerationStructure *>(src.top)
                      : reinterpret_cast<RaytraceAccelerationStructure *>(src.bottom);
  auto rdst = dst.top ? reinterpret_cast<RaytraceAccelerationStructure *>(dst.top)
                      : reinterpret_cast<RaytraceAccelerationStructure *>(dst.bottom);

  CmdCopyRaytraceAccelerationStructure cmd{rsrc, rdst, compact};
  Globals::ctx.dispatchCommand(cmd);
}

bool d3d::raytrace::check_vertex_format_support_for_acceleration_structure_build(uint32_t format)
{
  return 0 != (Globals::VK::fmt.buffer_features(VSDTToVulkanFormat(format)) &
                VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR);
}

/// unimplemented

::raytrace::Pipeline d3d::raytrace::create_pipeline(const ::raytrace::PipelineCreateInfo &pci)
{
  G_UNUSED(pci);
  G_ASSERTF(false, "d3d::raytrace::create_pipeline called on API without support");
  return ::raytrace::InvalidPipeline;
}

::raytrace::Pipeline d3d::raytrace::expand_pipeline(const ::raytrace::Pipeline &pipeline, const ::raytrace::PipelineExpandInfo &pei)
{
  G_UNUSED(pipeline);
  G_UNUSED(pei);
  G_ASSERTF(false, "d3d::raytrace::expand_pipeline called on API without support");
  return ::raytrace::InvalidPipeline;
}

void d3d::raytrace::destroy_pipeline(::raytrace::Pipeline &p) { G_UNUSED(p); }

::raytrace::ShaderBindingTableBufferProperties d3d::raytrace::get_shader_binding_table_buffer_properties(
  const ::raytrace::ShaderBindingTableDefinition &sbtci, const ::raytrace::Pipeline &pipeline)
{
  G_UNUSED(sbtci);
  G_UNUSED(pipeline);
  G_ASSERTF(false, "d3d::raytrace::destroy_pipeline called on API without support");
  return {};
}

void d3d::raytrace::dispatch(const ::raytrace::ResourceBindingTable &rbt, const ::raytrace::Pipeline &pipeline,
  const ::raytrace::RayDispatchParameters &rdp, GpuPipeline gpu_pipeline)
{
  G_UNUSED(rbt);
  G_UNUSED(pipeline);
  G_UNUSED(rdp);
  G_UNUSED(gpu_pipeline);
  G_ASSERTF(false, "d3d::raytrace::dispatch called on API without support");
}

void d3d::raytrace::dispatch_indirect(const ::raytrace::ResourceBindingTable &rbt, const ::raytrace::Pipeline &pipeline,
  const ::raytrace::RayDispatchIndirectParameters &rdip, GpuPipeline gpu_pipeline)
{
  G_UNUSED(rbt);
  G_UNUSED(pipeline);
  G_UNUSED(rdip);
  G_UNUSED(gpu_pipeline);
  G_ASSERTF(false, "d3d::raytrace::dispatch_indirect called on API without support");
}

void d3d::raytrace::dispatch_indirect_count(const ::raytrace::ResourceBindingTable &rbt, const ::raytrace::Pipeline &pipeline,
  const ::raytrace::RayDispatchIndirectCountParameters &rdicp, GpuPipeline gpu_pipeline)
{
  G_UNUSED(rbt);
  G_UNUSED(pipeline);
  G_UNUSED(rdicp);
  G_UNUSED(gpu_pipeline);
  G_ASSERTF(false, "d3d::raytrace::dispatch_indirect_count called on API without support");
}
