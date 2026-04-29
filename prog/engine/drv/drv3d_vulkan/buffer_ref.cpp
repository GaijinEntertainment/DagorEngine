// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "buffer_ref.h"
#include "globals.h"
#include "device_context.h"
#include "backend/cmd/buffers.h"
#include "buffer_props.h"
#include "frontend.h"
#include "frontend_pod_state.h"

using namespace drv3d_vulkan;

BufferRef BufferRef::discard(DeviceMemoryClass memory_class, FormatStore view_format, uint32_t bufFlags, uint32_t dynamic_size)
{
  BufferRef ret;

  if (bufFlags & SBCF_FRAMEMEM)
  {
    D3D_CONTRACT_ASSERTF(dynamic_size, "vulkan: framemem discard size must be > 0");
    ret = Frontend::frameMemBuffers.acquire(dynamic_size, bufFlags);
    if (ret.buffer != buffer)
      Frontend::frameMemBuffers.swapRefCounters(buffer, ret.buffer);
    if (buffer)
      Globals::ctx.dispatchCmd<CmdNotifyBufferDiscard>({*this, ret, bufFlags});
  }
  else
  {
    Buffer *buf = buffer;
    uint32_t reallocated_size = buf->getBlockSize();
    D3D_CONTRACT_ASSERTF(dynamic_size <= reallocated_size,
      "vulkan: discard size (%u) more than max buffer size (%u) requested for buffer %p:%s", dynamic_size, reallocated_size, buf,
      buf->getDebugName());

    if (dynamic_size > 0)
    {
      dynamic_size = Globals::VK::bufProps.alignSize(dynamic_size);
      D3D_CONTRACT_ASSERTF(!buf->hasView() || (dynamic_size == reallocated_size),
        "vulkan: variable sized discard for sampled buffers is not allowed asked %u bytes while buffer %p:%s size is %u", dynamic_size,
        buf, buf->getDebugName(), reallocated_size);
      reallocated_size = dynamic_size;
    }

    // lock ahead of time to avoid errors on frontFrameIndex increments from other thread
    // this is valid pattern for background thread data uploads,
    // but we must not corrupt memory if same happens for other reasons
    OSSpinlockScopedLock frontLock(Globals::ctx.getFrontLock());
    uint32_t frontFrameIndex = Frontend::State::pod.frameIndex;
    if (!buf->onDiscard(frontFrameIndex, reallocated_size))
    {
      uint32_t discardCount = buf->getDiscardBlocks();
      if (discardCount > 1)
        discardCount += Buffer::pending_discard_frames;
      else
        discardCount = Buffer::pending_discard_frames;

      buf = Buffer::create(buffer->getBlockSize(), memory_class, discardCount, buffer->getDescription().memFlags);
      // must mark range used as we will give this range to user without calling onDiscard
      buf->markDiscardUsageRange(frontFrameIndex, reallocated_size);
      if (buffer->hasView())
        buf->addBufferView(view_format);

      Globals::Dbg::naming.setBufName(buf, buffer->getDebugName());
    }
    ret = BufferRef{buf};
    Globals::ctx.dispatchCmdNoLock<CmdNotifyBufferDiscard>({*this, ret, bufFlags});
  }

  G_ASSERTF(ret.buffer, "vulkan: discard failed for %p:%s", buffer, buffer->getDebugName());
  return ret;
}
