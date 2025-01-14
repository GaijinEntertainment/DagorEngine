// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "device_execution_tracker.h"
#include <osApiWrappers/dag_critSec.h>
#include "globals.h"
#include "resource_manager.h"
#include <util/dag_hash.h>
#include "driver_config.h"
#include "util/fault_report.h"
#include "backend.h"
#include "execution_state.h"
#include "execution_context.h"
#include "wrapped_command_buffer.h"

using namespace drv3d_vulkan;

void DeviceExecutionTracker::init()
{
  maxMarkerCount = MARKER_ALLOC_SZ;
  markerCount = 0;
  prevJobHash = 0;
  allocatedBuffers = false;
}

void DeviceExecutionTracker::shutdown()
{
  if (allocatedBuffers)
  {
    WinAutoLock lk(Globals::Mem::mutex);
    ResourceManager &rm = Globals::Mem::res;
    rm.free(hostMarkers);
    rm.free(deviceMarkers);
    hostMarkers = nullptr;
    deviceMarkers = nullptr;
    allocatedBuffers = false;
  }
  maxMarkerCount = 0;
  markerCount = 0;
}

void DeviceExecutionTracker::dumpFaultData(FaultReportDump &dump) const
{
  if (!allocatedBuffers || !markerCount)
    return;
  deviceMarkers->markNonCoherentRangeLoc(0, (markerCount - 1) * sizeof(Marker), false);

  FaultReportDump::RefId mark = dump.addTagged(FaultReportDump::GlobalTag::TAG_GPU_JOB_HASH, currentMarker.hash,
    String(8, "job final hash %016llX markers count %u max %u", currentMarker.hash, markerCount, maxMarkerCount));
  dump.addRef(mark, FaultReportDump::GlobalTag::TAG_GPU_JOB_ITEM, currentMarker.jobId);

  for (int i = 0; i < markerCount; ++i)
  {
    Marker &dmarker = *(Marker *)deviceMarkers->ptrOffsetLoc(i * sizeof(Marker));
    mark = dump.addTagged(FaultReportDump::GlobalTag::TAG_GPU_EXEC_MARKER, (uint64_t)&dmarker,
      String(8, "hash %016llX job %u", dmarker.hash, dmarker.jobId));
    dump.addRef(mark, FaultReportDump::GlobalTag::TAG_GPU_JOB_ITEM, dmarker.jobId);

    Marker &hmarker = *(Marker *)hostMarkers->ptrOffsetLoc(i * sizeof(Marker));
    mark = dump.addTagged(FaultReportDump::GlobalTag::TAG_CPU_EXEC_MARKER, (uint64_t)&hmarker,
      String(8, "hash %016llX job %u", hmarker.hash, hmarker.jobId));
    dump.addRef(mark, FaultReportDump::GlobalTag::TAG_GPU_JOB_ITEM, hmarker.jobId);
#if DAGOR_DBGLEVEL > 0
    dump.addRef(mark, FaultReportDump::GlobalTag::TAG_CALLER_HASH, hmarker.caller);
#endif
  }
}

void DeviceExecutionTracker::addMarker(const void *data, size_t data_sz)
{
  if (Globals::cfg.bits.enableDeviceExecutionTracker == 0)
    return;

  if (markerCount >= maxMarkerCount)
    return;

  const size_t msz = sizeof(Marker);
  // late init as we setup resource manager after first frame init
  if (!allocatedBuffers)
  {
    hostMarkers = Buffer::create(msz * maxMarkerCount, //
      DeviceMemoryClass::HOST_RESIDENT_HOST_READ_WRITE_BUFFER, 1, BufferMemoryFlags::DEDICATED);
    deviceMarkers = Buffer::create(msz * maxMarkerCount, //
      DeviceMemoryClass::HOST_RESIDENT_HOST_READ_ONLY_BUFFER, 1, BufferMemoryFlags::DEDICATED);
    allocatedBuffers = true;
  }

  const uint8_t *dt = (const uint8_t *)data;
  for (int i = 0; i < data_sz; ++i)
    currentMarker.hash = fnv1a_step<64>(dt[i], currentMarker.hash);
#if DAGOR_DBGLEVEL > 0
  currentMarker.caller = Backend::State::exec.getExecutionContext().getCurrentCmdCallerHash();
#endif

  if (Globals::cfg.executionTrackerBreakpoint != 0 && Globals::cfg.executionTrackerBreakpoint == currentMarker.hash)
  {
    generateFaultReport();
    DAG_FATAL("vulkan: reached execution hash breakpoint %016llX at cmd %p:%u", currentMarker.hash,
      Backend::State::exec.getExecutionContext().cmd, Backend::State::exec.getExecutionContext().cmdIndex);
  }

  const VkDeviceSize bufOffset = markerCount * msz;
  memcpy(hostMarkers->ptrOffsetLoc(bufOffset), &currentMarker, msz);
  // FIXME: do single flush at frame end
  hostMarkers->markNonCoherentRangeLoc(bufOffset, msz, true);

  const VkPipelineStageFlags syncStages = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
  const VkMemoryBarrier syncMem = {VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
    VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT};
  const VkBufferCopy markerCopy = {hostMarkers->bufOffsetLoc(bufOffset), deviceMarkers->bufOffsetLoc(bufOffset), msz};

  // FMB-COPY-FMB pattern, should force GPU to make this operation "isolated" from async stuff on timeline
  // which is what is required, otherwise it will not represent our serial-like
  Backend::cb.wCmdPipelineBarrier(syncStages, syncStages, 0, 1, &syncMem, 0, nullptr, 0, nullptr);
  Backend::cb.wCmdCopyBuffer(hostMarkers->getHandle(), deviceMarkers->getHandle(), 1, &markerCopy);
  Backend::cb.wCmdPipelineBarrier(syncStages, syncStages, 0, 1, &syncMem, 0, nullptr, 0, nullptr);

  ++markerCount;
}

void DeviceExecutionTracker::restart(size_t job_id)
{
  if (Globals::cfg.bits.enableDeviceExecutionTracker == 0)
    return;

  interlocked_release_store(prevJobHash, currentMarker.hash);
  currentMarker.jobId = job_id;
  currentMarker.hash = FNV1Params<64>::offset_basis;
  markerCount = 0;
  if (allocatedBuffers)
  {
    const size_t msz = sizeof(Marker);
    // clear on CPU for simplicity
    memset(deviceMarkers->ptrOffsetLoc(0), 0, msz * maxMarkerCount);
    memset(hostMarkers->ptrOffsetLoc(0), 0, msz * maxMarkerCount);
    deviceMarkers->markNonCoherentRangeLoc(0, msz * maxMarkerCount, true);
    hostMarkers->markNonCoherentRangeLoc(0, msz * maxMarkerCount, true);
  }
}

void DeviceExecutionTracker::verify()
{
  if (Globals::cfg.bits.enableDeviceExecutionTracker == 0 || markerCount == 0)
    return;

  deviceMarkers->markNonCoherentRangeLoc(0, (markerCount - 1) * sizeof(Marker), false);

  uint64_t firstFailedHash = 0;
  uint64_t firstFailedCaller = 0;

  for (int i = markerCount - 1; i > 0; --i)
  {
    Marker &dmarker = *(Marker *)deviceMarkers->ptrOffsetLoc(i * sizeof(Marker));
    Marker &hmarker = *(Marker *)hostMarkers->ptrOffsetLoc(i * sizeof(Marker));
    if (dmarker.hash != hmarker.hash)
    {
      firstFailedHash = hmarker.hash;
#if DAGOR_DBGLEVEL > 0
      firstFailedCaller = hmarker.caller;
#endif
    }
    else
      break;
  }
  if (firstFailedHash != 0)
    D3D_ERROR(
      "vulkan: GPU execution tracker verify failed. First hash %016llX (caller %016llX) not reached on GPU, frame hash %016llX",
      firstFailedHash, firstFailedCaller, currentMarker.hash);
}
