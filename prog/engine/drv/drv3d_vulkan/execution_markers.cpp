// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "execution_markers.h"
#include "globals.h"
#include "debug_naming.h"
#include "device_context.h"
#include "device_queue.h"
#include "wrapped_command_buffer.h"

using namespace drv3d_vulkan;

void ExecutionMarkers::init()
{
#if VK_AMD_buffer_marker
  if (Globals::VK::dev.hasExtension<BufferMarkerAMD>())
  {
    executionMarkerBuffer = Buffer::create(MAX_DEBUG_MARKER_BUFFER_ENTRIES * sizeof(uint32_t),
      DeviceMemoryClass::HOST_RESIDENT_HOST_READ_ONLY_BUFFER, 1, BufferMemoryFlags::DEDICATED);
    Globals::Dbg::naming.setBufName(executionMarkerBuffer, "amd marker buffer");
  }
#endif
}

void ExecutionMarkers::shutdown()
{
#if VK_AMD_buffer_marker
  if (executionMarkerBuffer)
    Globals::ctx.destroyBuffer(executionMarkerBuffer);
  executionMarkerBuffer = nullptr;
#endif
}


void ExecutionMarkers::markAsPassedIfLessOrEquel(uint32_t id)
{
  for (auto &&val : commandDebugData)
  {
    if (val.commandIndex <= id)
    {
      val.passed = true;
    }
  }
}

void ExecutionMarkers::dumpFault(FaultReportDump &dump)
{
  for (auto &&data : commandDebugData)
  {
    FaultReportDump::RefId mark =
      dump.addTagged(FaultReportDump::GlobalTag::TAG_MARKER, data.commandIndex, String(8, "%s", data.passed ? "yes" : "no"));
    dump.addRef(mark, FaultReportDump::GlobalTag::TAG_CMD, (uint64_t)data.commandPtr);
    dump.addRef(mark, FaultReportDump::GlobalTag::TAG_WORK_ITEM, (uint64_t)data.workItemIndex);
  }
}

void ExecutionMarkers::check()
{
  if (commandDebugData.empty())
  {
    debug("No debug event mapping data");
    return;
  }
  for (auto &&data : commandDebugData)
    data.passed = false;

#if VK_AMD_buffer_marker
  if (executionMarkerBuffer)
  {
    auto ptr = reinterpret_cast<const uint32_t *>(executionMarkerBuffer->ptrOffsetLoc(0));
    debug("Checking execution makers - VK_AMD_buffer_marker:");
    debug("Next id would be: %08lX", commandIndex);
    uint32_t maxValue = 0;
    for (uint32_t i = 0; i < MAX_DEBUG_MARKER_BUFFER_ENTRIES; ++i)
    {
      maxValue = max(maxValue, ptr[i]);
    }
    debug("Max committed value is: %08lX", maxValue);
    debug("Missing %08lX commits", commandIndex - maxValue);
    markAsPassedIfLessOrEquel(maxValue);
    return;
  }
#endif
#if VK_NV_device_diagnostic_checkpoints
  VulkanDevice &vkDev = Globals::VK::dev;
  if (vkDev.hasExtension<DiagnosticCheckpointsNV>())
  {
    VulkanQueueHandle grQueue = Globals::VK::queue[DeviceQueueType::GRAPHICS].getHandle();

    debug("Checking execution makers - VK_NV_device_diagnostic_checkpoints:");
    debug("Next id would be: %08lX", commandIndex);
    uint32_t count = 0;
    VULKAN_LOG_CALL(vkDev.vkGetQueueCheckpointDataNV(grQueue, &count, nullptr));
    if (0 == count)
    {
      debug("No marker data available");
      return;
    }
    debug("Found %u markers", count);
    eastl::vector<VkCheckpointDataNV> checkpoints;
    checkpoints.resize(count);
    // this extension mechanism for output data is awkward
    for (auto &&checkpoint : checkpoints)
    {
      checkpoint.sType = VK_STRUCTURE_TYPE_CHECKPOINT_DATA_NV;
      checkpoint.pNext = nullptr;
      // checkpoint.stage = 0;
      checkpoint.pCheckpointMarker = nullptr;
    }
    VULKAN_LOG_CALL(vkDev.vkGetQueueCheckpointDataNV(grQueue, &count, checkpoints.data()));
    uint32_t bottomId = 0;
    uint32_t topId = 0;
    // this extension usually reports last passed bottom and the next in line that then failed
    for (auto &&checkpoint : checkpoints)
    {
      debug("Marker %u has passed stage %u", (uintptr_t)checkpoint.pCheckpointMarker, checkpoint.stage);
      if (checkpoint.stage == VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT)
        topId = (uint32_t)(uintptr_t)checkpoint.pCheckpointMarker;
      else if (checkpoint.stage == VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT)
        bottomId = (uint32_t)(uintptr_t)checkpoint.pCheckpointMarker;
    }
    debug("Last executed command is: %08lX", bottomId);
    debug("Next command would be: %08lX", topId);
    debug("Missing %u commits", commandIndex - bottomId);
    markAsPassedIfLessOrEquel(bottomId);
    return;
  }
#endif
}

bool ExecutionMarkers::hasPassed(size_t id, const void *ptr)
{
  CommandDebugInfo *closestMatch = nullptr;
  for (auto &&val : commandDebugData)
  {
    if (val.workItemIndex != id || val.commandPtr < ptr)
      continue;
    if (val.commandPtr == ptr)
    {
      debug("0x%p -> %u", ptr, val.commandIndex);
      return val.passed;
    }

    if (closestMatch)
    {
      // find command that is behind the the cmd and is closer to the  cmd
      if (val.commandPtr < closestMatch->commandPtr)
        closestMatch = &val;
    }
    else
    {
      closestMatch = &val;
    }
  }
  if (closestMatch)
  {
    debug("0x%p -> %u", ptr, closestMatch->commandIndex);
    return closestMatch->passed;
  }

  if (!commandDebugData.empty())
    debug("can't find debug data for work item %u and cmd %p", id, ptr);
  return true;
}

void ExecutionMarkers::write(VkPipelineStageFlagBits stage, size_t work_idx, const void *cmd)
{
  G_UNUSED(stage);

#if VK_AMD_buffer_marker | VK_NV_device_diagnostic_checkpoints
  VulkanDevice &vkDev = Globals::VK::dev;
#endif

#if VK_AMD_buffer_marker
  if (executionMarkerBuffer)
  {
    uint32_t indexValue = commandIndex++;
    if (commandDebugData.empty())
      commandDebugData.resize(MAX_DEBUG_MARKER_BUFFER_ENTRIES);

    auto &dbgInfo = commandDebugData[indexValue % commandDebugData.size()];
    dbgInfo.workItemIndex = work_idx;
    dbgInfo.commandPtr = cmd;
    dbgInfo.commandIndex = indexValue;

    VULKAN_LOG_CALL(Backend::cb.wCmdWriteBufferMarkerAMD(stage, executionMarkerBuffer->getHandle(),
      executionMarkerBuffer->bufOffsetLoc((indexValue % MAX_DEBUG_MARKER_BUFFER_ENTRIES) * sizeof(uint32_t)), indexValue));

    return;
  }
#endif
#if VK_NV_device_diagnostic_checkpoints
  if (vkDev.hasExtension<DiagnosticCheckpointsNV>())
  {
    uint32_t indexValue = commandIndex++;
    if (commandDebugData.empty())
      commandDebugData.resize(MAX_DEBUG_MARKER_BUFFER_ENTRIES);

    auto &dbgInfo = commandDebugData[indexValue % commandDebugData.size()];
    dbgInfo.workItemIndex = work_idx;
    dbgInfo.commandPtr = cmd;
    dbgInfo.commandIndex = indexValue;

    VULKAN_LOG_CALL(Backend::cb.wCmdSetCheckpointNV((void *)(uintptr_t)indexValue));
    return;
  }
#endif
}
