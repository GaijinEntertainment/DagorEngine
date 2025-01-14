// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "pipeline.h"
#include "state_field_compute.h"
#include "globals.h"
#include "dummy_resources.h"
#include "pipeline/manager.h"
#include "compute_state.h"
#include "execution_state.h"
#include "device_context.h"
#include "backend.h"
#include "execution_context.h"
#include "wrapped_command_buffer.h"

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
    D3D_ERROR("vulkan: trying to use invalid compute program\n%s", target.getExecutionContext().getCurrentCmdCaller());
    return;
  }

  auto *pipe = &Globals::pipelines.get<ComputePipeline>(handle);
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
void StateFieldComputePipeline::applyTo(BackComputeStateStorage &, ExecutionContext &) const
{
  VULKAN_LOG_CALL(Backend::cb.wCmdBindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, ptr->getHandleForUse()));
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
void StateFieldComputeLayout::applyTo(BackComputeStateStorage &, ExecutionContext &) const
{
  Backend::bindless.bindSets(VK_PIPELINE_BIND_POINT_COMPUTE, ptr->handle);
  Backend::State::exec.getResBinds(STAGE_CS).invalidateState();
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
  auto &regRef = ptr->getLayout()->registers.cs();
  VulkanPipelineLayoutHandle layoutHandle = ptr->getLayout()->handle;

  target.trackStageResAccessesNonParallel(regRef.header, ExtendedShaderStage::CS);

  Backend::State::exec.getResBinds(STAGE_CS).apply(target.vkDev, Globals::dummyResources.getTable(), Backend::gpuJob->index, regRef,
    ExtendedShaderStage::CS,
    [layoutHandle](VulkanDescriptorSetHandle set, const uint32_t *offsets, uint32_t offset_count) //
    {
      VULKAN_LOG_CALL(Backend::cb.wCmdBindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, layoutHandle,
        ShaderStageTraits<VK_SHADER_STAGE_COMPUTE_BIT>::register_index + PipelineBindlessConfig::bindlessSetCount, 1, ary(&set),
        offset_count, offsets));
    });

  if (Globals::cfg.debugLevel > 0)
    ptr->setNameForDescriptorSet(regRef.getSetForNaming(), spirv::compute::REGISTERS_SET_INDEX);

  Backend::sync.completeNeeded();
}

template <>
void StateFieldComputeFlush::dumpLog(const BackComputeStateStorage &) const
{}

} // namespace drv3d_vulkan

using namespace drv3d_vulkan;
