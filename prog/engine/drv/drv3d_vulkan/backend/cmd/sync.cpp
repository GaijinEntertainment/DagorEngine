// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "sync.h"
#include "backend/context.h"
#include "util/fault_report.h"
#include "stacked_profile_events.h"
#include "predicted_latency_waiter.h"
#include "execution_sync.h"
#include "wrapped_command_buffer.h"

using namespace drv3d_vulkan;

// barrier is not always describing DST OP
// while we can convert it only to DST OP for following sync step
// and DST OP is not always well convertible by pure bit-scan like logic
// and in general user slection of this RB bits are quite error prone (at least from vulkan POV)
// so do precise barriers handling for now

// ignore flags that don't make any sense for now
struct FilteredResourceBarrier
{
  uint64_t v;
  FilteredResourceBarrier(ResourceBarrier in) { v = in & ~(RB_FLAG_SPLIT_BARRIER_END); }
};

bool d3d_resource_barrier_to_layout(bool is_depth, VkImageLayout &layout, FilteredResourceBarrier barrier)
{
  switch (barrier.v)
  {
    case RB_STAGE_ALL_SHADERS | RB_RO_SRV: layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; break;
    case RB_STAGE_PIXEL | RB_STAGE_VERTEX | RB_STAGE_COMPUTE | RB_RO_SRV: // fallthru
    case RB_STAGE_PIXEL | RB_SOURCE_STAGE_PIXEL | RB_RO_SRV:              // fallthru
    case RB_STAGE_PIXEL | RB_RO_SRV: layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; break;
    case RB_STAGE_VERTEX | RB_RO_SRV: layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; break;
    case RB_STAGE_COMPUTE | RB_RO_SRV: layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; break;
    case RB_STAGE_PIXEL | RB_STAGE_COMPUTE | RB_RO_SRV: layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; break;
    case RB_RW_UAV | RB_STAGE_PIXEL:                    // fallthru
    case RB_RW_UAV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE: // fallthru
    case RB_RW_UAV | RB_STAGE_COMPUTE: layout = VK_IMAGE_LAYOUT_GENERAL; break;
    case RB_STAGE_VERTEX | RB_STAGE_PIXEL | RB_RO_SRV: layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; break;
    case RB_RW_RENDER_TARGET:
      layout = is_depth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      break;
    case RB_RW_RENDER_TARGET | RB_RO_SRV | RB_STAGE_PIXEL:
      G_ASSERTF(is_depth, "vulkan: RO DS barrier used for non DS texture, fix caller!");
      layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
      break;
    case RB_RO_BLIT_SOURCE: // fallthru
    case RB_RO_COPY_SOURCE: layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL; break;
    default: return false;
  }
  return true;
}

bool d3d_resource_barrier_to_laddr(bool is_tex, bool is_depth, LogicAddress &laddr, FilteredResourceBarrier barrier)
{
  G_UNUSED(is_tex);
  switch (barrier.v)
  {
    case RB_STAGE_ALL_SHADERS | RB_RO_SRV:
      laddr.access = VK_ACCESS_SHADER_READ_BIT;
      laddr.stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT |
                    VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
      break;
    case RB_STAGE_PIXEL | RB_SOURCE_STAGE_PIXEL | RB_RO_SRV: // fallthru
    case RB_STAGE_PIXEL | RB_RO_SRV:
      laddr.access = VK_ACCESS_SHADER_READ_BIT;
      laddr.stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      break;
    case RB_STAGE_VERTEX | RB_RO_SRV:
      laddr.access = VK_ACCESS_SHADER_READ_BIT;
      laddr.stage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
      break;
    case RB_STAGE_VERTEX | RB_STAGE_PIXEL | RB_RO_SRV:
      laddr.access = VK_ACCESS_SHADER_READ_BIT;
      laddr.stage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      break;
    case RB_STAGE_COMPUTE | RB_RO_SRV:
      laddr.access = VK_ACCESS_SHADER_READ_BIT;
      laddr.stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
      break;
    case RB_STAGE_PIXEL | RB_STAGE_VERTEX | RB_STAGE_COMPUTE | RB_RO_SRV:
      laddr.access = VK_ACCESS_SHADER_READ_BIT;
      laddr.stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      break;
    case RB_STAGE_PIXEL | RB_STAGE_COMPUTE | RB_RO_SRV:
      laddr.access = VK_ACCESS_SHADER_READ_BIT;
      laddr.stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      break;
    case RB_RW_UAV | RB_STAGE_PIXEL:
      laddr.access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      laddr.stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      break;
    case RB_RW_UAV | RB_STAGE_ALL_SHADERS:
      laddr.access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      laddr.stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT |
                    VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
      break;
    case RB_RW_UAV | RB_STAGE_COMPUTE:
      laddr.access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      laddr.stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
      break;
    case RB_RW_UAV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE:
      laddr.access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      laddr.stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      break;
    case RB_RW_RENDER_TARGET:
      G_ASSERTF(is_tex, "vulkan: render target barrier used for non texture res, fix caller!");
      laddr = LogicAddress::forAttachmentWithLayout(
        is_depth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
      break;
    case RB_RW_RENDER_TARGET | RB_RO_SRV | RB_STAGE_PIXEL:
      G_ASSERTF(is_tex, "vulkan: render target barrier used for non texture res, fix caller!");
      G_ASSERTF(is_depth, "vulkan: RO DS barrier used for non DS texture, fix caller!");
      laddr = LogicAddress::forAttachmentWithLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
      laddr.merge(LogicAddress::forImageOnExecStage(ExtendedShaderStage::PS, RegisterType::T));
      break;
    case RB_RO_BLIT_SOURCE: // fallthru
    case RB_RO_COPY_SOURCE:
      laddr.access = VK_ACCESS_TRANSFER_READ_BIT;
      laddr.stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      break;
    default: return false;
  }
  return true;
}

TSPEC void BEContext::execCmd(const CmdFlushDraws &)
{
  Backend::profilerStack.finish(); // ends frame core

  TIME_PROFILE(vulkan_CmdFlushDraws)
  FrameInfo &frame = Backend::gpuJob.get();
  flush(frame.frameDone);
  Globals::timelines.get<TimelineManager::GpuExecute>().advance();
  Backend::gpuJob.restart();
}

TSPEC void BEContext::execCmd(const CmdFlushAndWait &cmd)
{
  Backend::profilerStack.finish(); // ends frame core

  TIME_PROFILE(vulkan_CmdFlushAndWait);
  FrameInfo &frame = Backend::gpuJob.get();
  // avoid slowdown when we can't treat 2 screen updates as proper frame
  Backend::latencyWaiter.reset();
  flush(frame.frameDone);
  Backend::gpuJob.end();
  while (Globals::timelines.get<TimelineManager::GpuExecute>().advance())
    ; // VOID
  Backend::gpuJob.start();

  // we can't wait fence from 2 threads, so
  // shortcut to avoid submiting yet another fence
  if (cmd.userFence)
    cmd.userFence->setSignaledExternally();
}

TSPEC void BEContext::execCmd(const CmdImageBarrier &cmd)
{
  // tracking on non bindless resources is precise enough, handle only bindless case
  if (!cmd.img->isUsedInBindless())
    return;
  beginCustomStage("imageBarrier");

  LogicAddress laddr;
  VkImageLayout layout;
  FilteredResourceBarrier filteredBarrier = FilteredResourceBarrier(cmd.state);

  bool isDepth = cmd.img->getUsage() & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  if (!(d3d_resource_barrier_to_layout(isDepth, layout, filteredBarrier) &&
        d3d_resource_barrier_to_laddr(true /*is_tex*/, isDepth, laddr, filteredBarrier)))
  {
    D3D_ERROR("vulkan: unhandled barrier %llX for img %p:%s, range %u-%u caller %s", cmd.state, cmd.img, cmd.img->getDebugName(),
      cmd.res_index, cmd.res_range, getCurrentCmdCaller());
    return;
  }

  if (cmd.res_range == 0)
    Backend::sync.addImageAccess(laddr, cmd.img, layout, {0, cmd.img->getMipLevels(), 0, cmd.img->getArrayLayers()});
  else if (cmd.img->getArrayLayers() == 1)
    Backend::sync.addImageAccess(laddr, cmd.img, layout, {cmd.res_index, cmd.res_range, 0, 1});
  else if (cmd.img->getMipLevels() == 1)
    Backend::sync.addImageAccess(laddr, cmd.img, layout, {0, 1, cmd.res_index, cmd.res_range});
  else
  {
    for (uint32_t i = cmd.res_index; i < cmd.res_range + cmd.res_index; i++)
    {
      uint32_t mip_level = i % cmd.img->getMipLevels();
      uint32_t layer = i / cmd.img->getMipLevels();

      Backend::sync.addImageAccess(laddr, cmd.img, layout, {mip_level, 1, layer, 1});
    }
  }
}

TSPEC void BEContext::execCmd(const CmdBufferBarrier &cmd)
{
  // tracking on non bindless resources is precise enough, handle only bindless case
  if (!cmd.bRef.buffer->isUsedInBindless())
    return;
  beginCustomStage("bufferBarrier");

  LogicAddress laddr;
  if (!d3d_resource_barrier_to_laddr(false /*is_tex*/, false /*is_depth*/, laddr, FilteredResourceBarrier(cmd.state)))
  {
    D3D_ERROR("vulkan: unhandled barrier %llX for buf %p:%s caller %s", cmd.state, cmd.bRef.buffer, cmd.bRef.buffer->getDebugName(),
      getCurrentCmdCaller());
    return;
  }

  verifyResident(cmd.bRef.buffer);
  Backend::sync.addBufferAccess(laddr, cmd.bRef.buffer, {cmd.bRef.bufOffset(0), cmd.bRef.visibleDataSize});
}

TSPEC void BEContext::execCmd(const CmdDelaySyncCompletion &cmd)
{
  if (cmd.enable)
  {
    beginCustomStage("DelayedSyncStart");
    Backend::sync.setCompletionDelay(cmd.enable);
    Backend::cb.startReorder();
  }
  else
  {
    Backend::sync.setCompletionDelay(cmd.enable);
    finishReorderAndPerformSync();
  }
}

TSPEC void BEContext::execCmd(const CmdQueueSwitch &cmd)
{
  if (!Globals::cfg.bits.allowUserProvidedMultiQueue)
    return;
  if ((int)frameCoreQueue == cmd.queue)
    return;

  VulkanCommandBufferHandle frameCoreOld = frameCore;
  beginCustomStage("QueueSwitch");
  G_UNUSED(frameCoreOld);
  G_ASSERTF(frameCoreOld == frameCore, "vulkan: should have not interrupted frame core here!");
  switchFrameCoreForQueueChange((DeviceQueueType)cmd.queue);
  onFrameCoreReset();
}

TSPEC void BEContext::execCmd(const CmdQueueSignal &cmd)
{
  if (!Globals::cfg.bits.allowUserProvidedMultiQueue)
    return;
  G_ASSERTF(scratch.userQueueSignals.size() == cmd.signalIdx,
    "vulkan: back to front queue signal sequencing broken expected %u, got %u", scratch.userQueueSignals.size(), cmd.signalIdx);
  scratch.userQueueSignals.push_back({lastBufferIdxOnQueue[cmd.queue], 0});
}

TSPEC void BEContext::execCmd(const CmdQueueWait &cmd)
{
  if (!Globals::cfg.bits.allowUserProvidedMultiQueue)
    return;

  // interrupt command buffer on wait, to avoid waiting for previous work together with next work
  VulkanCommandBufferHandle frameCoreOld = frameCore;
  beginCustomStage("QueueWait");
  if (frameCoreOld == frameCore)
    interruptFrameCore();
  ExecutionScratch::UserQueueSignal &usignal = scratch.userQueueSignals[cmd.signalIdx];
  // already waited for this signal on target queue
  if (usignal.waitedOnQueuesMask & (1 << cmd.queue))
    return;
  addQueueDep(scratch.userQueueSignals[cmd.signalIdx].bufferIdx, lastBufferIdxOnQueue[cmd.queue]);
  // mark waited
  usignal.waitedOnQueuesMask |= (1 << cmd.queue);
}

TSPEC void BEContext::execCmd(const CmdCompleteSync &)
{
  // someone started reorder, so we are surely inside proper sync range, no need to complete it
  if (Backend::cb.isInReorder())
    return;

  Backend::sync.completeNeeded();
}

TSPEC void FaultReportDump::dumpCmd(const CmdQueueSwitch &v) { VK_CMD_DUMP_PARAM(queue); }

TSPEC void FaultReportDump::dumpCmd(const CmdQueueSignal &v)
{
  VK_CMD_DUMP_PARAM(signalIdx);
  VK_CMD_DUMP_PARAM(queue);
}

TSPEC void FaultReportDump::dumpCmd(const CmdQueueWait &v)
{
  VK_CMD_DUMP_PARAM(signalIdx);
  VK_CMD_DUMP_PARAM(queue);
}
