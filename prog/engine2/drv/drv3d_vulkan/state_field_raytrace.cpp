#include "pipeline.h"
#include "device.h"

#if D3D_HAS_RAY_TRACING
#include "state_field_raytrace.h"

namespace drv3d_vulkan
{

VULKAN_TRACKED_STATE_FIELD_REF(StateFieldStageResourceBinds<STAGE_RAYTRACE>, binds, FrontRaytraceStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldRaytraceProgram, prog, FrontRaytraceStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldRaytracePipeline, pipeline, BackRaytraceStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldRaytraceLayout, layout, BackRaytraceStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldRaytraceFlush, flush, BackRaytraceStateStorage);

template <>
void StateFieldRaytraceProgram::applyTo(FrontRaytraceStateStorage &, ExecutionState &target) const
{
  if (!handle)
    return;

  auto *pipe = &get_device().pipeMan.get<RaytracePipeline>(handle);
  target.set<StateFieldRaytracePipeline, RaytracePipeline *, BackRaytraceState>(pipe);
  target.set<StateFieldRaytraceLayout, RaytracePipelineLayout *, BackRaytraceState>(pipe->getLayout());
}

template <>
void StateFieldRaytraceProgram::transit(FrontRaytraceStateStorage &, DeviceContext &target) const
{
  CmdSetRaytraceProgram cmd{handle};
  target.dispatchCommandNoLock(cmd);
}

template <>
void StateFieldRaytraceProgram::dumpLog(const FrontRaytraceStateStorage &) const
{
  if (!handle)
  {
    debug("RaytraceProgram: none");
    return;
  }

  debug("RaytraceProgram: %u ", handle.get());

  // we can't read debug info here, as method can be called in frontend
}

template <>
void StateFieldRaytracePipeline::dumpLog(const BackRaytraceStateStorage &) const
{
  debug("RaytracePipeline = %p", ptr);
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  if (ptr)
    ptr->printDebugInfo();
#endif
}

template <>
void StateFieldRaytraceLayout::applyTo(BackRaytraceStateStorage &, ExecutionContext &target) const
{
  target.back.contextState.bindlessManagerBackend.bindSets(target, VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, ptr->handle);
  ContextState &ctxState = target.back.contextState;
  ctxState.stageState[STAGE_RAYTRACE].invalidateState();
}

template <>
void StateFieldRaytraceLayout::dumpLog(const BackRaytraceStateStorage &) const
{
  debug("RaytraceLayout = %p", ptr);
}

template <>
void StateFieldRaytracePipeline::applyTo(BackRaytraceStateStorage &, ExecutionContext &target) const
{
  // NOTE: kept as a reference
#if 0 // VK_NV_ray_tracing
  ContextState& ctxState = target.back.contextState;
  ptr->bind(target.vkDev, target.getFrameCmdBuffer());
#else
  G_UNUSED(target);
#endif
}

template <>
void StateFieldRaytraceFlush::applyTo(BackRaytraceStateStorage &state, ExecutionContext &target) const
{
  // NOTE: kept as a reference
#if 0 // VK_NV_ray_tracing
  RaytracePipeline* ptr = state.pipeline.ptr;
  ContextState& ctxState = target.back.contextState;
  auto& regRef = ptr->getLayout()->registers.rs();
  VulkanPipelineLayoutHandle layoutHandle = ptr->getLayout()->handle;

  target.ensureBoundImageStates(regRef.header, STAGE_RAYTRACE);

  ctxState.stageState[STAGE_RAYTRACE].apply(
    target.vkDev, target.device.getDummyResourceTable(),
    ctxState.frame->index,
    regRef, target,
    [&target, layoutHandle](VulkanDescriptorSetHandle set, const uint32_t *offsets, uint32_t offset_count) //
    {
      VULKAN_LOG_CALL(target.vkDev.vkCmdBindDescriptorSets(
        target.getFrameCmdBuffer(), VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, layoutHandle,
        ShaderStageTraits<VK_SHADER_STAGE_RAYGEN_BIT_NV>::register_index + PipelineBindlessConfig::bindlessSetCount, 1,
        ary(&set), offset_count, offsets));
    });
  );
#else
  G_UNUSED(state);
  G_UNUSED(target);
#endif
}

template <>
void StateFieldRaytraceFlush::dumpLog(const BackRaytraceStateStorage &) const
{}

} // namespace drv3d_vulkan

using namespace drv3d_vulkan;

#endif // D3D_HAS_RAY_TRACING