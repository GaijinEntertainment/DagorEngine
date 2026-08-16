// Copyright (C) Gaijin Games KFT.  All rights reserved.

// user provided GPU sync logic implementation
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_enhanced_barrier.h>
#include "globals.h"
#include "global_lock.h"
#include "device_context.h"
#include "driver_config.h"
#include "command.h"
#include "physical_device_set.h"
#include "backend/cmd/sync.h"
#include "buffer.h"
#include "texture.h"

using namespace drv3d_vulkan;

namespace
{

DeviceQueueType gpu_pipeline_to_device_queue_type(GpuPipeline pipe)
{
  if (pipe == GpuPipeline::ASYNC_COMPUTE)
    return DeviceQueueType::COMPUTE;
  else if (pipe == GpuPipeline::TRANSFER)
    // reuse upload queue because it will be free from any workload at time user logic is processed
    return DeviceQueueType::TRANSFER_UPLOAD;
  else
    return DeviceQueueType::GRAPHICS;
}

// use alloc less approach for fence handle
// pack replay ID and local frame fence index
struct PrefixedFenceHandle
{
  // underlying type must fit to external
  static_assert(sizeof(GPUFENCEHANDLE) >= sizeof(uint32_t));
  static constexpr uint32_t fenceBits = 8;
  static constexpr uint32_t fenceMask = (1 << fenceBits) - 1;
  static constexpr uint32_t frameMask = ~fenceMask;

  GPUFENCEHANDLE val;
  GPUFENCEHANDLE toDriver() { return val; }
  uint32_t toIdx() { return val & fenceMask; }

  // +1 to avoid collision with BAD_GPUFENCEHANDLE
  GPUFENCEHANDLE getThisFramePrefix() { return (((Frontend::replay->id + 1) << fenceBits) & frameMask); }
  bool verify() { return getThisFramePrefix() == (val & frameMask); }

  PrefixedFenceHandle(uint32_t idx) : val(getThisFramePrefix() | idx)
  {
    G_ASSERTF(idx < fenceMask, "vulkan: too much user fences per frame, adjust masking");
  }
  PrefixedFenceHandle(GPUFENCEHANDLE user_fence) : val(user_fence) {}
};

} // anonymous namespace

void d3d::resource_barrier(const ResourceBarrierDesc &desc, GpuPipeline /*gpu_pipeline = GpuPipeline::GRAPHICS*/)
{
  if (Globals::lock.isAcquired() && Globals::VK::phy.hasBindless)
  {
    OSSpinlockScopedLock frontLock(Globals::ctx.getFrontLock());

    desc.enumerateTextureBarriers([](BaseTexture *tex, ResourceBarrier state, unsigned res_index, unsigned res_range) {
      if (!tex)
      {
        D3D_ERROR("vulkan: no texture in user barrier specified");
        return;
      }
      // ignore split barrier flags for now, process only end part
      if (state & RB_FLAG_SPLIT_BARRIER_BEGIN)
        return;

      // RB_NONE is "hack" to skip next sync
      // but it is non efficient as we must track src op for proper sync on vulkan
      // and also sync step must be reordered/delayed, as other operations must be RB_NONE-d too
      // yet if it is done, no RB_NONE is NOT needed at all
      // because if operations can be batch completed, reordered/delayed sync step will verify it and
      // make proper batch-fashion barriers
      if (state == RB_NONE)
        return;

      Image *image = cast_to_texture_base(tex)->image;
      uint32_t stop_index = (res_range == 0) ? image->getMipLevels() * image->getArrayLayers() : res_range + res_index;
      G_UNUSED(stop_index);
      G_ASSERTF((image->getMipLevels() * image->getArrayLayers()) >= stop_index,
        "Out of bound subresource index range (%d, %d+%d) in resource barrier for image %s (0, %d)", res_index, res_index, res_range,
        image->getDebugName(), image->getMipLevels() * image->getArrayLayers());

      Globals::ctx.dispatchCmdNoLock<CmdImageBarrier>({image, state, res_index, res_range});
    });

    desc.enumerateBufferBarriers([](Sbuffer *buf, ResourceBarrier state) {
      // ignore global UAV write flushes
      // they will not work properly in current sync logic and should be already tracked
      // only REAL acceses can be processed properly/without risk, not dummy/assumed global ones
      // reasons:
      //  1. drivers are buggy on global memory barriers (even validator can simply ignore them)
      //  2. execution on GPU can be async task based, not linear FIFO, giving different results on "wide" barriers
      if (!buf && (state & RB_FLUSH_UAV))
        return;
      // RB_NONE is "hack" to skip next sync
      // but it is non efficient as we must track src op for proper sync on vulkan
      // and also sync step must be reordered/delayed, as other operations must be RB_NONE-d too
      // yet if it is done, no RB_NONE is NOT needed at all
      // because if operations can be batch completed, reordered/delayed sync step will verify it and
      // make proper batch-fashion barriers
      if (state == RB_NONE)
        return;

      // ignore split barrier flags for now, process only end part
      if (state & RB_FLAG_SPLIT_BARRIER_BEGIN)
        return;

      if (!buf)
      {
        D3D_ERROR("vulkan: no buffer in user barrier specified");
        return;
      }
      auto gbuf = (GenericBufferInterface *)buf;
      G_ASSERT(gbuf->getBufferRef().buffer);
      Globals::ctx.dispatchCmdNoLock<CmdBufferBarrier>({gbuf->getBufferRef(), state}); //-V522
    });
  }
}

namespace
{

// front lock must be held: appends into the current replay's barrier store
bool queue_enhanced_image_barrier(const d3d::TextureBarrier &barrier, BaseTexture *texture)
{
  if (!texture)
  {
    D3D_ERROR("vulkan: enhanced_texture_barrier with null texture");
    return false;
  }
  Image *image = cast_to_texture_base(texture)->image;
  if (!image)
  {
    D3D_ERROR("vulkan: image is missing in barrier for texture %p:%s", texture, texture->getName());
    return false;
  }
  if (image->isSyncTracked())
  {
    D3D_ERROR("vulkan: enhanced_texture_barrier called on tracked texture <%s>, create it with TEXCF_NO_STATE_TRACKING",
      image->getDebugName());
    return false;
  }
  Frontend::replay->enhancedImageBarriers.push_back({image, barrier});
  return true;
}

bool queue_enhanced_buffer_barrier(const d3d::BufferBarrier &barrier, Sbuffer *buffer)
{
  if (!buffer)
  {
    D3D_ERROR("vulkan: enhanced_buffer_barrier with null buffer");
    return false;
  }
  BufferRef bRef = ((GenericBufferInterface *)buffer)->getBufferRef();
  if (!bRef.buffer)
    return false;
  if (bRef.buffer->isSyncTracked())
  {
    D3D_ERROR("vulkan: enhanced_buffer_barrier called on tracked buffer <%s>, create it with SBCF_NO_STATE_TRACKING",
      buffer->getBufName());
    return false;
  }
  Frontend::replay->enhancedBufferBarriers.push_back({bRef, barrier});
  return true;
}

} // anonymous namespace

void d3d::enhanced_texture_barrier(const d3d::TextureBarrier &barrier, BaseTexture *texture)
{
  VERIFY_GLOBAL_LOCK_ACQUIRED();
  OSSpinlockScopedLock frontLock(Globals::ctx.getFrontLock());
  const uint32_t idx = Frontend::replay->enhancedImageBarriers.size();
  if (queue_enhanced_image_barrier(barrier, texture))
    Globals::ctx.dispatchCmdNoLock<CmdEnhancedBarrierBatch>({idx, 1, 0, 0});
}

void d3d::enhanced_buffer_barrier(const d3d::BufferBarrier &barrier, Sbuffer *buffer)
{
  VERIFY_GLOBAL_LOCK_ACQUIRED();
  OSSpinlockScopedLock frontLock(Globals::ctx.getFrontLock());
  const uint32_t idx = Frontend::replay->enhancedBufferBarriers.size();
  if (queue_enhanced_buffer_barrier(barrier, buffer))
    Globals::ctx.dispatchCmdNoLock<CmdEnhancedBarrierBatch>({0, 0, idx, 1});
}

void d3d::enhanced_barrier_batch(dag::ConstSpan<d3d::TextureBarrierBatchItem> texture_barriers,
  dag::ConstSpan<d3d::BufferBarrierBatchItem> buffer_barriers)
{
  VERIFY_GLOBAL_LOCK_ACQUIRED();
  OSSpinlockScopedLock frontLock(Globals::ctx.getFrontLock());

  const uint32_t imageIndex = Frontend::replay->enhancedImageBarriers.size();
  uint32_t imageCount = 0;
  for (const d3d::TextureBarrierBatchItem &item : texture_barriers)
    imageCount += queue_enhanced_image_barrier(item.barrier, item.texture) ? 1 : 0;

  const uint32_t bufferIndex = Frontend::replay->enhancedBufferBarriers.size();
  uint32_t bufferCount = 0;
  for (const d3d::BufferBarrierBatchItem &item : buffer_barriers)
    bufferCount += queue_enhanced_buffer_barrier(item.barrier, item.buffer) ? 1 : 0;

  if (imageCount || bufferCount)
    Globals::ctx.dispatchCmdNoLock<CmdEnhancedBarrierBatch>({imageIndex, imageCount, bufferIndex, bufferCount});
}

void drv3d_vulkan::d3d_command_change_queue(GpuPipeline gpu_pipeline)
{
  VERIFY_GLOBAL_LOCK_ACQUIRED();

  Globals::ctx.dispatchCmd<CmdQueueSwitch>({(int)gpu_pipeline_to_device_queue_type(gpu_pipeline)});
}

GPUFENCEHANDLE d3d::insert_fence(GpuPipeline gpu_pipeline)
{
  // ignore out of gpu lock fence inserts, because they point to unknown time (can interrupt pass or so)
  // and if they are used - usually for lock transfer tracking, which are handled by frame-frame global sync
  if (!Globals::lock.isAcquired())
    return BAD_GPUFENCEHANDLE;

  uint32_t rawSignalIdx = Frontend::replay->userSignalCount++;
  Globals::ctx.dispatchCmd<CmdQueueSignal>({rawSignalIdx, (int)gpu_pipeline_to_device_queue_type(gpu_pipeline)});
  return PrefixedFenceHandle{rawSignalIdx}.toDriver();
}

void d3d::insert_wait_on_fence(GPUFENCEHANDLE &fence, GpuPipeline gpu_pipeline) //-V669
{
  if (fence == BAD_GPUFENCEHANDLE)
    return;

  VERIFY_GLOBAL_LOCK_ACQUIRED();
  PrefixedFenceHandle userFence(fence);

  // old signal from different job!
  if (!userFence.verify())
    return;
  Globals::ctx.dispatchCmd<CmdQueueWait>({userFence.toIdx(), (int)gpu_pipeline_to_device_queue_type(gpu_pipeline)});
}