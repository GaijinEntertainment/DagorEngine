// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "misc.h"
#include "backend/context.h"
#include "util/fault_report.h"
#include "execution_async_compile_state.h"
#include "execution_timings.h"
#if _TARGET_ANDROID
#include <unistd.h>
#endif

using namespace drv3d_vulkan;

TSPEC void BEContext::execCmd(const CmdCompileGraphicsPipeline &cmd)
{
  // skip compilation of pipelines with subpass inputs, as we can't supply them using this algorithm
  ProgramID prog = Backend::State::pipe.getRO<StateFieldGraphicsProgram, ProgramID, FrontGraphicsState>();
  if (prog != ProgramID::Null())
  {
    auto &pipe = Globals::pipelines.get<VariatedGraphicsPipeline>(prog);
    auto &regs = pipe.getLayout()->registers;
    if (regs.fs().header.inputAttachmentCount)
      return;
  }

  Backend::State::asyncCompileState.nonDrawCompile = true;
  Backend::State::exec.set<StateFieldActiveExecutionStage>(ActiveExecutionStage::GRAPHICS);
  flushGrahpicsState(cmd.top);
  Backend::State::asyncCompileState.nonDrawCompile = false;
  invalidateActiveGraphicsPipeline();
}

TSPEC void BEContext::execCmd(const CmdCompileComputePipeline &) {}

TSPEC void BEContext::execCmd(const CmdRecordFrameTimings &cmd)
{
  // profile ref ticks can be inconsistent between threads, skip such data
  uint64_t now = profile_ref_ticks();
  cmd.timingData->frontendToBackendUpdateScreenLatency = (int64_t)(now > cmd.kickoffTime ? now - cmd.kickoffTime : 0);

  cmd.timingData->gpuWaitDuration = Backend::timings.gpuWaitDuration;
  cmd.timingData->backendFrontendWaitDuration = Backend::timings.workWaitDuration;
  // time blocked in present are counted from last frame
  cmd.timingData->presentDuration = Backend::timings.presentWaitDuration;
}

TSPEC void BEContext::execCmd(const CmdCleanupPendingReferences &cmd)
{
  // apply queued discards before syncing state and scrapping resources
  // otherwise chain of discards may be corrupted
  // by removal of buffer in its tail
  applyQueuedDiscards();
  Backend::State::pendingCleanups.cleanupAllNonUsed(cmd.state, Backend::State::pipe);
}

TSPEC void BEContext::execCmd(const CmdShutdownPendingReferences &) { Backend::State::pendingCleanups.shutdown(); }

TSPEC void BEContext::execCmd(const CmdRegisterFrameEventsCallback &cmd)
{
  cmd.callback->beginFrameEvent();
  scratch.frameEventCallbacks.push_back(cmd.callback);
}

TSPEC void BEContext::execCmd(const CmdGetWorkerCpuCore &cmd)
{
#if _TARGET_ANDROID
  *cmd.core = sched_getcpu();
  *cmd.threadId = gettid();
#endif
  G_UNUSED(cmd);
}

TSPEC void BEContext::execCmd(const CmdAsyncPipeFeedbackBegin &cmd)
{
  for (int i = 0; i < Backend::State::asyncCompileState.asyncPipelineCompileFeedbackStack.size(); i++)
    interlocked_add(*Backend::State::asyncCompileState.asyncPipelineCompileFeedbackStack[i],
      Backend::State::asyncCompileState.currentSkippedCount);
  Backend::State::asyncCompileState.currentSkippedCount = 0;
  Backend::State::asyncCompileState.asyncPipelineCompileFeedbackStack.push_back(cmd.feedbackPtr);
}

TSPEC void BEContext::execCmd(const CmdAsyncPipeFeedbackEnd &)
{
  uint32_t stackSize = Backend::State::asyncCompileState.asyncPipelineCompileFeedbackStack.size();
  G_ASSERT(stackSize > 0);
  interlocked_add(*Backend::State::asyncCompileState.asyncPipelineCompileFeedbackStack[stackSize - 1],
    Backend::State::asyncCompileState.currentSkippedCount);
  Backend::State::asyncCompileState.asyncPipelineCompileFeedbackStack.pop_back();
}

TSPEC void BEContext::execCmd(const CmdUpdateAliasedMemoryInfo &cmd) { Backend::aliasedMemory.update(cmd.id, cmd.info); }

TSPEC void BEContext::execCmd(const CmdImgActivate &cmd)
{
  // can't read contents of image if other resource was using its memory
  // and even if we not reading them, followup barriers can break content on some drivers
  cmd.img->layout.resetTo(VK_IMAGE_LAYOUT_UNDEFINED);
}

TSPEC void BEContext::execCmd(const CmdSetLatencyMarker &cmd)
{
  if (cmd.type == lowlatency::LatencyMarkerType::RENDERSUBMIT_END)
  {
    needRenderSubmitLatencyMarkerBeforePresent = true;
    return;
  }

  if (auto *lowLatencyModule = GpuLatency::getInstance())
    lowLatencyModule->setMarker(cmd.frame_id, cmd.type);
}

TSPEC void FaultReportDump::dumpCmd(const CmdSetLatencyMarker &v)
{
  VK_CMD_DUMP_PARAM(frame_id);
  VK_CMD_DUMP_PARAM(type);
}

TSPEC void BEContext::execCmd(const CmdRestartPipelineCompiler &)
{
  Backend::pipelineCompiler.shutdown();
  Backend::pipelineCompiler.init();
}
