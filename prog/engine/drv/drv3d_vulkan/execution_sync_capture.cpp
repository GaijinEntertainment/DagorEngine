// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if EXECUTION_SYNC_DEBUG_CAPTURE > 0
#include "execution_sync_capture.h"
#include "backend.h"
#include "frontend.h"
#include "timelines.h"
#include "backend_interop.h"
#include "execution_context.h"

using namespace drv3d_vulkan;

void ExecutionSyncCapture::addSyncStep()
{
  currentLocalSyncStepOpIdx = 0;
  ExecutionContext &ctx = Backend::State::exec.getExecutionContext();
  DeviceQueueType syncStepQueue = ctx.frameCoreQueue;
  steps.push_back({currentVisNode++, currentVisPin++, currentVisPin++, Backend::sync.getCaller(),
    (uint32_t)ctx.lastBufferIdxOnQueue[(int)syncStepQueue], (uint32_t)syncStepQueue, currentSyncStep, 0, currentVisPin++,
    currentVisPin++, currentVisPin++, 0, 0, 0});
  currentSyncStep++;
}
void ExecutionSyncCapture::addOp(ExecutionSyncTracker::OpUid uid, LogicAddress laddr, Resource *res,
  ExecutionSyncTracker::OpCaller caller)
{
  ops.push_back({currentVisNode++, currentVisPin++, currentVisPin++, currentVisPin++, uid.v, currentSyncStep,
    currentLocalSyncStepOpIdx++, laddr, res, caller, String(res->getDebugName()), 0});
}
void ExecutionSyncCapture::addLink(ExecutionSyncTracker::OpUid src_op_uid, ExecutionSyncTracker::OpUid dst_op_uid)
{
  links.push_back({src_op_uid.v, dst_op_uid.v});
}
void ExecutionSyncCapture::reset()
{
  if (Backend::interop.syncCaptureRequest.load(std::memory_order_acquire))
  {
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
