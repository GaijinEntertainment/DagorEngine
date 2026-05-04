// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if EXECUTION_SYNC_DEBUG_CAPTURE > 0
#include "execution_sync_capture.h"
#include "backend.h"
#include "frontend.h"
#include "timelines.h"
#include "backend_interop.h"
#include "backend/context.h"
#include <memory/dag_framemem.h>

using namespace drv3d_vulkan;

void ExecutionSyncCapture::addSyncStep()
{
  if (currentLocalSyncStepOpIdx == 0)
    return;
  currentLocalSyncStepOpIdx = 0;
  BEContext &ctx = Backend::ctx;
  DeviceQueueType syncStepQueue = ctx.frameCoreQueue;
  steps.push_back({currentVisNode++, currentVisPin++, currentVisPin++, Backend::sync.getCaller(),
    (uint32_t)ctx.lastBufferIdxOnQueue[(int)syncStepQueue], (uint32_t)syncStepQueue, currentSyncStep, 0, currentVisPin++,
    currentVisPin++, currentVisPin++, 0, 0, 0});
  currentSyncStep++;
}

void ExecutionSyncCapture::addOp(SyncOpUid uid, LogicAddress laddr, Resource *res, SyncOpCaller caller)
{
  if (uid.v == ~0)
    return;
  ops.push_back({currentVisNode++, currentVisPin++, currentVisPin++, currentVisPin++, uid.v, currentSyncStep,
    currentLocalSyncStepOpIdx++, laddr, res, caller, String(res->getDebugName()), 0});
}

void ExecutionSyncCapture::addLink(SyncOpUid src_op_uid, SyncOpUid dst_op_uid)
{
  if (src_op_uid.v == ~0 || dst_op_uid.v == ~0)
    return;
  links.push_back({src_op_uid.v, dst_op_uid.v});
}

void ExecutionSyncCapture::onBackendReplayStart()
{
  WinAutoLock lk(Globals::Mem::mutex);

  dag::Vector<ResourceMemoryId, framemem_allocator> aliasableMemIds;

  auto nameFilterCb = [&aliasableMemIds, this](auto *obj) {
    obj->filteredOutForSyncCapture = false;
    if (resNameFilter.empty())
      return;

    if (!strstr(obj->getDebugName(), resNameFilter.c_str()))
      obj->filteredOutForSyncCapture = true;
    else if (obj->mayAlias())
      aliasableMemIds.push_back(obj->getMemoryId());
  };

  auto aliasFilterCb = [&aliasableMemIds](auto *obj) {
    if (obj->filteredOutForSyncCapture)
      return;

    if (!obj->mayAlias())
      return;

    const AliasedResourceMemory &lmem = Backend::aliasedMemory.get(obj->getMemoryId());
    for (ResourceMemoryId i : aliasableMemIds)
    {
      const AliasedResourceMemory &rmem = Backend::aliasedMemory.get(i);
      if (rmem.intersects(lmem))
      {
        obj->filteredOutForSyncCapture = false;
        break;
      }
    }
  };

  Globals::Mem::res.iterateAllocated<Image>(nameFilterCb);
  Globals::Mem::res.iterateAllocated<Buffer>(nameFilterCb);

  if (!aliasableMemIds.empty())
  {
    Globals::Mem::res.iterateAllocated<Image>(aliasFilterCb);
    Globals::Mem::res.iterateAllocated<Buffer>(aliasFilterCb);
  }
}

void ExecutionSyncCapture::reset()
{
  if (Backend::interop.syncCaptureRequest.load(std::memory_order_acquire))
  {
    resNameFilter = Backend::interop.syncCaptureNameFilter;
    Frontend::syncCapture = Backend::syncCapture;
    --Backend::interop.syncCaptureRequest;
  }
  ops.clear();
  steps.clear();
  links.clear();
  bufferLinks.clear();
  currentLocalSyncStepOpIdx = 0;
  currentSyncStep = 0;
  currentVisNode = 1;
  currentVisPin = 1;
}

#endif // EXECUTION_SYNC_DEBUG_CAPTURE > 0
