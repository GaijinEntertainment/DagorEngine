#include "pipeline.h"
#include "device.h"
#include "state_field_compute.h"

namespace drv3d_vulkan
{

VULKAN_TRACKED_STATE_FIELD_REF(StateFieldStageResourceBinds<STAGE_CS>, binds, FrontComputeStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldComputeProgram, prog, FrontComputeStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldComputePipeline, pipeline, BackComputeStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldComputeLayout, layout, BackComputeStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldComputeFlush, flush, BackComputeStateStorage);

template <>
void StateFieldComputeProgram::applyTo(FrontComputeStateStorage &, ExecutionState &target) const
{
  if (!handle)
  {
    // check only if we about to use compute pipeline, otherwise ignore error,
    // it still can happen, but at least we don't handle false positives and still see it sometimes
    if (target.getRO<StateFieldActiveExecutionStage, ActiveExecutionStage>() != ActiveExecutionStage::COMPUTE)
      return;
    // in this case caller somehow managed to draw with program
    // that was deleted right before draw
    // trace back and fix this at caller site
    // but do not crash and use debug program as fallback
    logerr("vulkan: trying to use invalid compute program\n%s", target.getExecutionContext().getCurrentCmdCaller());
    return;
  }

  auto *pipe = &get_device().pipeMan.get<ComputePipeline>(handle);
  target.set<StateFieldComputePipeline, ComputePipeline *, BackComputeState>(pipe);
  target.set<StateFieldComputeLayout, ComputePipelineLayout *, BackComputeState>(pipe->getLayout());
}

template <>
void StateFieldComputeProgram::transit(FrontComputeStateStorage &, DeviceContext &target) const
{
  CmdSetComputeProgram cmd{handle};
  target.dispatchCommandNoLock(cmd);
}

template <>
void StateFieldComputeProgram::dumpLog(const FrontComputeStateStorage &) const
{
  if (!handle)
  {
    debug("ComputeProgram: none");
    return;
  }

  debug("ComputeProgram: %u ", handle.get());

  // we can't read debug info here, as method can be called in frontend
}

template <>
void StateFieldComputePipeline::applyTo(BackComputeStateStorage &, ExecutionContext &target) const
{
  ptr->bind(target.vkDev, target.frameCore);
}

template <>
void StateFieldComputePipeline::dumpLog(const BackComputeStateStorage &) const
{
  debug("ComputePipeline = %p", ptr);
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  if (ptr)
    ptr->printDebugInfo();
#endif
}

template <>
void StateFieldComputeLayout::applyTo(BackComputeStateStorage &, ExecutionContext &target) const
{
  target.back.contextState.bindlessManagerBackend.bindSets(target, VK_PIPELINE_BIND_POINT_COMPUTE, ptr->handle);
  target.back.executionState.getResBinds(STAGE_CS).invalidateState();
}

template <>
void StateFieldComputeLayout::dumpLog(const BackComputeStateStorage &) const
{
  debug("ComputeLayout = %p", ptr);
}

template <>
void StateFieldComputeFlush::applyTo(BackComputeStateStorage &state, ExecutionContext &target) const
{
  ComputePipeline *ptr = state.pipeline.ptr;
  ContextState &ctxState = target.back.contextState;
  auto &regRef = ptr->getLayout()->registers.cs();
  VulkanPipelineLayoutHandle layoutHandle = ptr->getLayout()->handle;

  target.trackStageResAccessesNonParallel(regRef.header, STAGE_CS);

  target.back.executionState.getResBinds(STAGE_CS).apply(target.vkDev, target.device.getDummyResourceTable(), ctxState.frame->index,
    regRef, target, STAGE_CS,
    [&target, layoutHandle](VulkanDescriptorSetHandle set, const uint32_t *offsets, uint32_t offset_count) //
    {
      VULKAN_LOG_CALL(target.vkDev.vkCmdBindDescriptorSets(target.frameCore, VK_PIPELINE_BIND_POINT_COMPUTE, layoutHandle,
        ShaderStageTraits<VK_SHADER_STAGE_COMPUTE_BIT>::register_index + PipelineBindlessConfig::bindlessSetCount, 1, ary(&set),
        offset_count, offsets));
    });

  target.back.syncTrack.completeNeeded(target.frameCore, target.vkDev);
}

template <>
void StateFieldComputeFlush::dumpLog(const BackComputeStateStorage &) const
{}

} // namespace drv3d_vulkan

using namespace drv3d_vulkan;
