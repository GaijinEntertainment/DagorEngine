#include "state_field_execution_context.h"
#include "device.h"
#include "back_scope_state.h"

namespace drv3d_vulkan
{

VULKAN_TRACKED_STATE_FIELD_REF(StateFieldActiveExecutionStage, activeExecutionStage, ExecutionStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldExecutionStageCloser, stageCloser, BackScopeStateStorage);

template <>
void StateFieldExecutionStageCloser::dumpLog(const BackScopeStateStorage &) const
{
  debug("stageCloser: last transit %s -> %s", formatActiveExecutionStage(data.from), formatActiveExecutionStage(data.to));
}

template <>
void StateFieldExecutionStageCloser::applyTo(BackScopeStateStorage &, ExecutionContext &target) const
{
  if (ActiveExecutionStage::FRAME_BEGIN == data.to || ActiveExecutionStage::FRAME_BEGIN == data.from ||
      ActiveExecutionStage::CUSTOM == data.from)
    return;

  constexpr VkAccessFlags stateDstAccess[] = {
    // FRAME_BEGIN
    0,
    // GRAPHICS
    //  graphics may use indirect draw, an index buffer, vertex buffers, uniforms, texture and/or
    //  storage objects
    VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
      VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
    // COMPUTE
    //  compute may use indirect dispatch, uniforms, texture and/or storage objects
    VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
    // CUSTOM
    VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
#if D3D_HAS_RAY_TRACING
    // RAYTRACE
    //  raytrace has only uniforms, texture and/or storage objects
    VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT
#endif
  };

  constexpr VkPipelineStageFlags stateDstStage[] = {
    // FRAME_BEGIN
    0,
    // GRAPHICS
    //  graphics may use indirect draw, an index buffer, vertex buffers, uniforms, texture and/or
    //  storage objects
    VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
    // COMPUTE
    //  compute may use indirect dispatch, uniforms, texture and/or storage objects
    VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    // CUSTOM
    //  must finish all work before using custom stage, otherwise
    //  some untracked pending oprations can be left not synchronised for following work
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
#if D3D_HAS_RAY_TRACING
    // RAYTRACE
    //  raytrace has only uniforms, texture and/or storage objects
    VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV
#endif
  };

  constexpr VkPipelineStageFlags stateSrcStage[] = {
    // FRAME_BEGIN
    0,
    // GRAPHICS
    //  those all the stages that could modify storage images/buffers
    //  framebuffer flushes are handled by render passes
    VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    // COMPUTE
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    // CUSTOM
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
#if D3D_HAS_RAY_TRACING
    // RAYTRACE
    VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV | VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV
#endif
  };

  // add execution and memory barrier and flush everything pending too
  VulkanDevice &vkDev = get_device().getVkDevice();
  ExecutionContext::PrimaryPipelineBarrier barrier(vkDev, stateSrcStage[(uint32_t)data.from], stateDstStage[(uint32_t)data.to]);

  barrier.addMemory({VK_ACCESS_SHADER_WRITE_BIT, stateDstAccess[(uint32_t)data.to]});
  barrier.submit(target.frameCore);
}

template <>
void StateFieldActiveExecutionStage::applyTo(ExecutionStateStorage &state, ExecutionContext &)
{
  auto from = oldData;
  auto to = data;
  oldData = data;

  // graphics to graphics is synced with render passes
  // also access to render targets is handled with image state tracking
  if (ActiveExecutionStage::GRAPHICS == to && ActiveExecutionStage::GRAPHICS == from)
    return;

  switch (data)
  {
    case ActiveExecutionStage::COMPUTE: state.compute.disableApply(false); state.graphics.disableApply(true);
#if D3D_HAS_RAY_TRACING
      state.raytrace.disableApply(true);
#endif
      break;
    case ActiveExecutionStage::GRAPHICS:
      // needed to properly restart render pass after interrupt if it was not changed since interruption
      state.graphics.makeFieldDirty<StateFieldGraphicsRenderPassScopeOpener>();
      state.compute.disableApply(true);
      state.graphics.disableApply(false);
#if D3D_HAS_RAY_TRACING
      state.raytrace.disableApply(true);
#endif
      break;
#if D3D_HAS_RAY_TRACING
    case ActiveExecutionStage::RAYTRACE:
      state.compute.disableApply(true);
      state.graphics.disableApply(true);
      state.raytrace.disableApply(false);
      break;
#endif
    case ActiveExecutionStage::FRAME_BEGIN:
    case ActiveExecutionStage::CUSTOM: state.compute.disableApply(true); state.graphics.disableApply(true);
#if D3D_HAS_RAY_TRACING
      state.raytrace.disableApply(true);
#endif
      break;
    default: G_ASSERTF(false, "vulkan: wrong active execution stage selected"); return;
  }

  // any scopes must be closed on stage change
  state.scopes.set<StateFieldExecutionStageCloser, StateFieldExecutionStageCloser::Transit>({from, to});
  state.scopes.set<StateFieldGraphicsNativeRenderPassScopeCloser, ActiveExecutionStage>(to);
  state.scopes.set<StateFieldGraphicsRenderPassScopeCloser, ActiveExecutionStage>(to);
  state.scopes.set<StateFieldGraphicsConditionalRenderingScopeCloser, ActiveExecutionStage>(to);
}

template <>
void StateFieldActiveExecutionStage::dumpLog(const ExecutionStateStorage &) const
{
  debug("activeExecutionStage: %s", formatActiveExecutionStage(data));
}

} // namespace drv3d_vulkan

using namespace drv3d_vulkan;
