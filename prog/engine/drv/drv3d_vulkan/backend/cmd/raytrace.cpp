// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "raytrace.h"
#include "backend/context.h"
#include "util/fault_report.h"
#include "buffer_ref.h"
#include "execution_sync.h"
#include "wrapped_command_buffer.h"

using namespace drv3d_vulkan;

#if VULKAN_HAS_RAYTRACING
TSPEC void BEContext::execCmd(const CmdTraceRays &)
{
  Backend::State::exec.set<StateFieldActiveExecutionStage>(ActiveExecutionStage::RAYTRACE);
  G_ASSERTF(false, "CmdTraceRays called on API without support");
}

#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query

void BEContext::accumulateRaytraceBuildAccesses(const RaytraceStructureBuildData &build_data)
{
  if (build_data.type == VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR)
  {
    auto &tlbd = build_data.tlas;
    verifyResident(tlbd.instanceBuffer.buffer);
    Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_SHADER_READ_BIT},
      tlbd.instanceBuffer.buffer, {tlbd.instanceBuffer.bufOffset(0), sizeof(VkAccelerationStructureInstanceKHR) * tlbd.instanceCount});
  }
  else
  {
    auto &blbd = build_data.blas;
    for (const RaytraceBLASBufferRefs &bufferRefs :
      make_span(data->raytraceBLASBufferRefsStore.data() + blbd.firstGeometry, blbd.geometryCount))
    {
      verifyResident(bufferRefs.geometry);
      Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_SHADER_READ_BIT},
        bufferRefs.geometry, {bufferRefs.geometryOffset, bufferRefs.geometrySize});

      if (bufferRefs.index)
      {
        verifyResident(bufferRefs.index);
        Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_SHADER_READ_BIT},
          bufferRefs.index, {bufferRefs.indexOffset, bufferRefs.indexSize});
      }

      if (bufferRefs.transform)
      {
        verifyResident(bufferRefs.transform);
        Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_SHADER_READ_BIT},
          bufferRefs.transform, {bufferRefs.transformOffset, RT_TRANSFORM_SIZE});
      }
    }
    if (blbd.compactionSizeBuffer)
      verifyResident(blbd.compactionSizeBuffer.buffer);
  }

  verifyResident(build_data.scratchBuf.buffer);
  Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                                  VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR},
    build_data.scratchBuf.buffer, {build_data.scratchBuf.bufOffset(0), build_data.scratchBuf.visibleDataSize});

  if (build_data.update)
    Backend::sync.addAccelerationStructureAccess(
      {VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR},
      build_data.dst);
  else
    Backend::sync.addAccelerationStructureAccess(
      {VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR}, build_data.dst);
}

void BEContext::accumulateAssumedRaytraceStructureReads(const RaytraceStructureBuildData &build_data)
{
  if (build_data.type == VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR)
  {
    // there is no processed references to BLAS from TLAS in build data/resource, so for speed
    // do assumed read on CS/RT stage, if we do no read really - that barrier will fail!
    // set seal for safety here, to not conflict with followup writes without reads
    Backend::sync.addAccelerationStructureAccess(LogicAddress::forBLASIndirectReads(), build_data.dst);
  }
}

void initBuildInfo(VkAccelerationStructureBuildGeometryInfoKHR &dst, const RaytraceStructureBuildData &build_data)
{
  dst = //
    {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};

  dst.mode = build_data.update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  dst.flags = build_data.flags;
  dst.type = build_data.type;
  dst.dstAccelerationStructure = build_data.dst->getHandle();
  if (build_data.update)
    dst.srcAccelerationStructure = build_data.dst->getHandle();
  dst.scratchData.deviceAddress = build_data.scratchBuf.devOffset(0);
}

void BEContext::buildAccelerationStructure(const RaytraceStructureBuildData &build_data)
{
  if (build_data.type == VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR)
  {
    VkAccelerationStructureBuildGeometryInfoKHR buildInfo;
    initBuildInfo(buildInfo, build_data);

    VkAccelerationStructureGeometryInstancesDataKHR instancesData = //
      {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR};
    instancesData.data.deviceAddress = build_data.tlas.instanceBuffer.devOffset(0);

    VkAccelerationStructureGeometryKHR tlasGeo = //
      {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    tlasGeo.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    tlasGeo.geometry.instances = instancesData;

    buildInfo.geometryCount = 1;
    buildInfo.pGeometries = &tlasGeo;

    VkAccelerationStructureBuildRangeInfoKHR rangeInfo = {};
    rangeInfo.primitiveOffset = 0;
    rangeInfo.primitiveCount = build_data.tlas.instanceCount;

    const VkAccelerationStructureBuildRangeInfoKHR *build_range = &rangeInfo;
    VULKAN_LOG_CALL(Backend::cb.wCmdBuildAccelerationStructuresKHR(1, &buildInfo, &build_range));
  }
  else
  {
    VkAccelerationStructureBuildGeometryInfoKHR buildInfo;
    initBuildInfo(buildInfo, build_data);

    buildInfo.geometryCount = build_data.blas.geometryCount;
    buildInfo.pGeometries = data->raytraceGeometryKHRStore.data() + build_data.blas.firstGeometry;

    const VkAccelerationStructureBuildRangeInfoKHR *build_range = &data->raytraceBuildRangeInfoKHRStore[build_data.blas.firstGeometry];
    VULKAN_LOG_CALL(Backend::cb.wCmdBuildAccelerationStructuresKHR(1, &buildInfo, &build_range));

    if (build_data.blas.compactionSizeBuffer)
    {
      Backend::sync.addAccelerationStructureAccess(
        {VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR}, build_data.dst);

      constexpr VkDeviceSize querySize = sizeof(uint64_t);
      verifyResident(build_data.blas.compactionSizeBuffer.buffer);
      Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT},
        build_data.blas.compactionSizeBuffer.buffer, {build_data.blas.compactionSizeBuffer.bufOffset(0), querySize});
    }
  }
}

void BEContext::queryAccelerationStructureCompationSizes(const RaytraceStructureBuildData &build_data)
{
  if (build_data.type != VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR)
    return;
  BufferRef compactSizeBuf = build_data.blas.compactionSizeBuffer;
  if (!compactSizeBuf)
    return;

  // checked in previous build step
  //  verifyResident(compactSizeBuf.buffer);

  VkAccelerationStructureKHR dstAc = build_data.dst->getHandle();

  // do blocking-like size query for now, if not efficient - redo with per frame query pool copy
  constexpr VkDeviceSize querySize = sizeof(uint64_t);
  VULKAN_LOG_CALL(Backend::cb.wCmdResetQueryPool(Globals::rtSizeQueryPool.getPool(), 0, 1));
  VULKAN_LOG_CALL(Backend::cb.wCmdWriteAccelerationStructuresPropertiesKHR(1, &dstAc,
    VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, Globals::rtSizeQueryPool.getPool(), 0));
  VULKAN_LOG_CALL(Backend::cb.wCmdCopyQueryPoolResults(Globals::rtSizeQueryPool.getPool(), 0, 1, compactSizeBuf.getHandle(),
    compactSizeBuf.bufOffset(0), querySize, VK_QUERY_RESULT_WAIT_BIT | VK_QUERY_RESULT_64_BIT));

  Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_HOST_BIT, VK_ACCESS_HOST_READ_BIT}, compactSizeBuf.buffer,
    {compactSizeBuf.bufOffset(0), querySize});
}

#endif

TSPEC void BEContext::execCmd(const CmdRaytraceBuildStructures &cmd)
{
  beginCustomStage("RaytraceBuildAccelerationStructures");
  DA_PROFILE_TAG(count, "%u", cmd.count);
  auto dataRange = make_span(&data->raytraceStructureBuildStore[cmd.index], cmd.count);
  for (RaytraceStructureBuildData &itr : dataRange)
    accumulateRaytraceBuildAccesses(itr);
  Backend::sync.completeNeeded();
  for (RaytraceStructureBuildData &itr : dataRange)
    buildAccelerationStructure(itr);
  Backend::sync.completeNeeded();
  for (RaytraceStructureBuildData &itr : dataRange)
    queryAccelerationStructureCompationSizes(itr);
  Backend::sync.completeNeeded();
  for (RaytraceStructureBuildData &itr : dataRange)
    accumulateAssumedRaytraceStructureReads(itr);
}

TSPEC void BEContext::execCmd(const CmdCopyRaytraceAccelerationStructure &cmd)
{
#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
  beginCustomStage("RaytraceCopyAccelerationStructure");
  Backend::sync.addAccelerationStructureAccess(
    {VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR}, cmd.src);
  Backend::sync.addAccelerationStructureAccess(
    {VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR}, cmd.dst);
  Backend::sync.completeNeeded();

  VkCopyAccelerationStructureInfoKHR ci{VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR, nullptr};
  ci.mode = cmd.compact ? VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR : VK_COPY_ACCELERATION_STRUCTURE_MODE_CLONE_KHR;
  ci.src = cmd.src->getHandle();
  ci.dst = cmd.dst->getHandle();
  VULKAN_LOG_CALL(Backend::cb.wCmdCopyAccelerationStructureKHR(&ci));
#endif
}

TSPEC void FaultReportDump::dumpCmd(const CmdCopyRaytraceAccelerationStructure &v)
{
  VK_CMD_DUMP_PARAM_PTR(src);
  VK_CMD_DUMP_PARAM_PTR(dst);
}
#endif
