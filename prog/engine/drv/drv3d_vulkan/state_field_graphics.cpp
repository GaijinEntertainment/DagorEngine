// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "pipeline.h"
#include "state_field_graphics.h"
#include <cstring>
#include <validation.h>
#include "texture.h"
#include "globals.h"
#include "dummy_resources.h"
#include "pipeline/manager.h"
#include "pipeline_cache.h"
#include "backend.h"
#include "execution_pod_state.h"
#include "execution_context.h"
#include "vk_to_string.h"
#include "frontend.h"
#include "device_context.h"
#include "wrapped_command_buffer.h"

namespace drv3d_vulkan
{

// front fields
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsProgram, program, FrontGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsDepthBounds, depthBounds, FrontGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsScissorRect, scissorRect, FrontGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsInputLayoutOverride, inputLayout, FrontGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsRenderState, renderState, FrontGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsConditionalRenderingState, conditionalRenderingState, FrontGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsStencilRefOverride, stencilRefOverride, FrontGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsVertexBuffers, vertexBuffers, FrontGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsIndexBuffer, indexBuffers, FrontGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldStageResourceBinds<STAGE_PS>, bindsPS, FrontGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldStageResourceBinds<STAGE_VS>, bindsVS, FrontGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsPolygonLine, polygonLine, FrontGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsBlendConstantFactor, blendConstantFactor, FrontGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsVertexBufferStrides, vertexBufferStrides, FrontGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsViewport, viewport, FrontGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsQueryState, queryState, FrontGraphicsStateStorage)
// back fields
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsRenderPassClass, rpClass, BackGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsBasePipeline, basePipeline, BackGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsConditionalRenderingScopeCloser, conditionalRenderCloser, BackScopeStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsRenderPassScopeCloser, renderPassCloser, BackScopeStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsNativeRenderPassScopeCloser, nativeRenderPassCloser, BackScopeStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsFlush, flush, BackGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsConditionalRenderingScopeOpener, conditionalRenderingScopeOpener,
  BackGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsPipeline, pipeline, BackGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsPipelineLayout, layout, BackGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsFramebuffer, framebuffer, BackGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsInPass, inPass, BackGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsScissor, scissor, BackDynamicGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsDepthBias, depthBias, BackDynamicGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsDepthBounds, depthBounds, BackGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsStencilRef, stencilRef, BackDynamicGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsStencilRefOverride, stencilRefOverride, BackDynamicGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsStencilMask, stencilMask, BackDynamicGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsDynamicRenderStateIndex, dynamicRenderStateIndex, BackGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsBlendConstantFactor, blendConstantFactor, BackGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsIndexBuffer, indexBuffer, BackGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsViewport, viewport, BackGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsVertexBuffersBindArray, vertexBuffersArray, BackGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsPrimitiveTopology, primTopo, BackGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsRenderPassScopeOpener, renderPassOpener, BackGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsRenderPassEarlyScopeOpener, renderPassEarlyOpener, BackGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsQueryScopeOpener, queryOpener, BackGraphicsStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsQueryScopeCloser, graphicsQueryCloser, BackScopeStateStorage);
VULKAN_TRACKED_STATE_FIELD_REF(StateFieldGraphicsRenderPassArea, renderPassArea, BackGraphicsStateStorage);

// front fields

template <>
void StateFieldGraphicsPolygonLine::dumpLog(const FrontGraphicsStateStorage &) const
{
  debug("PolygonLine: %s", data ? "yes" : "no");
}

template <>
void StateFieldGraphicsPolygonLine::applyTo(FrontGraphicsStateStorage &, ExecutionState &target) const
{
  target.get<BackGraphicsState, BackGraphicsState>().pipelineState.polygonLine = data;
  target.set<StateFieldGraphicsPipeline, StateFieldGraphicsPipeline::Invalidate, BackGraphicsState>(1);
}

template <>
void StateFieldGraphicsPolygonLine::transit(FrontGraphicsStateStorage &, DeviceContext &target) const
{
  CmdSetPolygonLineEnable cmd{data};
  target.dispatchCommandNoLock(cmd);
}

template <>
void StateFieldGraphicsRenderState::applyTo(FrontGraphicsStateStorage &, ExecutionState &target) const
{
  DriverRenderState driverRS = Backend::renderStateSystem.get(data);
  target.get<BackGraphicsState, BackGraphicsState>().pipelineState.renderState = driverRS;
  target.set<StateFieldGraphicsDynamicRenderStateIndex, LinearStorageIndex, BackGraphicsState>(driverRS.dynamicIdx);
  target.set<StateFieldGraphicsPipeline, StateFieldGraphicsPipeline::Invalidate, BackGraphicsState>(1);
}

template <>
void StateFieldGraphicsRenderState::transit(FrontGraphicsStateStorage &, DeviceContext &target) const
{
  CmdSetRenderState cmd{data};
  target.dispatchCommandNoLock(cmd);
}

template <>
void StateFieldGraphicsRenderState::dumpLog(const FrontGraphicsStateStorage &) const
{
  debug("RenderStateID: %u", (uint32_t)data);
}
//////

template <>
void StateFieldGraphicsInputLayoutOverride::applyTo(FrontGraphicsStateStorage &, ExecutionState &target) const
{
  if (!handle)
    return;

  target.get<BackGraphicsState, BackGraphicsState>().pipelineState.inputLayout = handle;
  target.set<StateFieldGraphicsPipeline, StateFieldGraphicsPipeline::Invalidate, BackGraphicsState>(1);
}

template <>
void StateFieldGraphicsInputLayoutOverride::transit(FrontGraphicsStateStorage &, DeviceContext &target) const
{
  CmdSetInputLayout cmd{handle};
  target.dispatchCommandNoLock(cmd);
}

template <>
void StateFieldGraphicsInputLayoutOverride::dumpLog(const FrontGraphicsStateStorage &) const
{
  if (!handle)
    debug("InputLayout override: none");
  else
    debug("InputLayout override: %u", handle.get());
}

template <>
void StateFieldGraphicsConditionalRenderingState::applyTo(FrontGraphicsStateStorage &, ExecutionState &target) const
{
  target.set<StateFieldGraphicsConditionalRenderingScopeOpener, ConditionalRenderingState, BackGraphicsState>(data);
  target.set<StateFieldGraphicsConditionalRenderingScopeCloser, ConditionalRenderingState, BackScopeState>(data);
}

template <>
void StateFieldGraphicsConditionalRenderingScopeOpener::applyTo(BackGraphicsStateStorage &state, ExecutionContext &target)
{
  G_UNUSED(state);

  if (data != ConditionalRenderingState{})
  {
    G_ASSERTF(state.inPass.data == InPassStateFieldType::NORMAL_PASS,
      "vulkan: conditional rendering must be inside render pass, state %u caller %s", (int)state.inPass.data,
      target.getCurrentCmdCaller());

#if VK_EXT_conditional_rendering
    VkDeviceSize bufOffset = data.buffer.bufOffset(data.offset);
    target.verifyResident(data.buffer.buffer);
    Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT, VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT},
      data.buffer.buffer, {bufOffset, sizeof(uint32_t)});

    VkConditionalRenderingBeginInfoEXT crbi = //
      {VK_STRUCTURE_TYPE_CONDITIONAL_RENDERING_BEGIN_INFO_EXT, nullptr, data.buffer.getHandle(), bufOffset, 0};
    VULKAN_LOG_CALL(Backend::cb.wCmdBeginConditionalRenderingEXT(&crbi));
#endif
  }
}

template <>
void StateFieldGraphicsConditionalRenderingScopeCloser::applyTo(BackScopeStateStorage &, ExecutionContext &)
{
  // we should close on next field apply if follow up code will open scope
  bool askingForOpener = data != ConditionalRenderingState{};
  bool stageWillPerformOpener = stage == ActiveExecutionStage::GRAPHICS;

  bool scopeWillOpen = askingForOpener && stageWillPerformOpener;
  if (eastl::exchange(shouldClose, scopeWillOpen))
    VULKAN_LOG_CALL(Backend::cb.wCmdEndConditionalRenderingEXT());
}

template <>
void StateFieldGraphicsNativeRenderPassScopeCloser::dumpLog(const BackScopeStateStorage &) const
{
  debug("native rpCloser shouldClose: %s , activePass: %s", shouldClose ? "yes" : "no", data ? "yes" : "no");
}

template <>
void StateFieldGraphicsNativeRenderPassScopeCloser::applyTo(BackScopeStateStorage &, ExecutionContext &target)
{
  if (Globals::cfg.bits.fatalOnNRPSplit &&
      (shouldClose && stage != ActiveExecutionStage::GRAPHICS &&
        (Backend::State::pipe.getRO<StateFieldRenderPassSubpassIdx, uint32_t, FrontGraphicsState, FrontRenderPassState>()) !=
          StateFieldRenderPassSubpassIdx::InvalidSubpass))
  {
    // this is application error most of cases, check caller code and logic and fix it
    // stack of fatal should point to operation that was executed and command caller to application code
    DAG_FATAL(
      "application interrupted native render pass via non draw operation. move non draw operation outside of render pass! context: %s",
      target.getCurrentCmdCaller());
  }

  // we should close on next field apply if follow up code will open scope
  bool askingForOpener = data;
  bool stageWillPerformOpener = stage == ActiveExecutionStage::GRAPHICS;

  bool scopeWillOpen = askingForOpener && stageWillPerformOpener;
  if (eastl::exchange(shouldClose, scopeWillOpen))
    target.endNativePass();
}

template <>
void StateFieldGraphicsRenderPassScopeCloser::dumpLog(const BackScopeStateStorage &) const
{
  debug("rpCloser shouldClose: %s , activePass: %s", shouldClose ? "yes" : "no", data ? "yes" : "no");
}

template <>
void StateFieldGraphicsRenderPassScopeCloser::applyTo(BackScopeStateStorage &state, ExecutionContext &target)
{

  // we should close on next field apply if follow up code will open scope
  bool askingForOpener = data;
  bool stageWillPerformOpener = stage == ActiveExecutionStage::GRAPHICS;
  bool nativeWillBeUsed = state.nativeRenderPassCloser.shouldClose;

  bool scopeWillOpen = askingForOpener && stageWillPerformOpener && !nativeWillBeUsed;
  if (eastl::exchange(shouldClose, scopeWillOpen))
    target.endPass("rp_scope_close");
}

template <>
void StateFieldGraphicsConditionalRenderingState::transit(FrontGraphicsStateStorage &, DeviceContext &target) const
{
  CmdSetConditionalRender cmd{data.buffer, data.offset};
  target.dispatchCommandNoLock(cmd);
}

template <>
void StateFieldGraphicsConditionalRenderingState::dumpLog(const FrontGraphicsStateStorage &) const
{
  if (data == ConditionalRenderingState{})
    debug("conditional rendering: inactive");
  else
    debug("conditional rendering: active on buffer %" PRIu64 ", offset %d", generalize(data.buffer.getHandle()), data.offset);
}

template <>
void StateFieldGraphicsConditionalRenderingScopeOpener::dumpLog(const BackGraphicsStateStorage &) const
{
  if (data == ConditionalRenderingState{})
    debug("conditional rendering opener: inactive");
  else
    debug("conditional rendering opener: active on buffer %" PRIu64 ", offset %d", generalize(data.buffer.getHandle()), data.offset);
}

template <>
void StateFieldGraphicsConditionalRenderingScopeCloser::dumpLog(const BackScopeStateStorage &) const
{
  if (shouldClose)
    debug("conditional rendering closer: pending close on next apply");
  else
    debug("conditional rendering closer: no close pendin");
}

template <>
void StateFieldGraphicsProgram::applyTo(FrontGraphicsStateStorage &, ExecutionState &target) const
{
  ShaderProgramDatabase &spdb = Globals::shaderProgramDatabase;
  if (handle != ProgramID::Null())
  {
    InputLayoutID layoutID = spdb.getGraphicsProgInputLayout(handle);
    if (layoutID != InputLayoutID::Null())
    {
      VariatedGraphicsPipeline &newPipeline = Globals::pipelines.get<VariatedGraphicsPipeline>(handle);
      target.get<BackGraphicsState, BackGraphicsState>().pipelineState.inputLayout = layoutID;
      target.set<StateFieldGraphicsBasePipeline, VariatedGraphicsPipeline *, BackGraphicsState>(&newPipeline);
      target.set<StateFieldGraphicsPipelineLayout, GraphicsPipelineLayout *, BackGraphicsState>(newPipeline.getLayout());
      target.set<StateFieldGraphicsPrimitiveTopology, StateFieldGraphicsPrimitiveTopology::TessOverride, BackGraphicsState>(
        newPipeline.hasTesselationStage() ? true : false);
      target.set<StateFieldGraphicsPipeline, StateFieldGraphicsPipeline::Invalidate, BackGraphicsState>(1);
      return;
    }
  }

  // if invalid graphics program will be really used, it will trigger error at usage time
  target.get<BackGraphicsState, BackGraphicsState>().pipelineState.inputLayout = InputLayoutID::Null();
  target.set<StateFieldGraphicsBasePipeline, VariatedGraphicsPipeline *, BackGraphicsState>(nullptr);
  target.set<StateFieldGraphicsPipelineLayout, GraphicsPipelineLayout *, BackGraphicsState>(nullptr);
  target.set<StateFieldGraphicsPrimitiveTopology, StateFieldGraphicsPrimitiveTopology::TessOverride, BackGraphicsState>(false);
  target.set<StateFieldGraphicsPipeline, StateFieldGraphicsPipeline::Invalidate, BackGraphicsState>(1);
}

template <>
void StateFieldGraphicsProgram::transit(FrontGraphicsStateStorage &, DeviceContext &target) const
{
  CmdSetGraphicsProgram cmd{handle};
  target.dispatchCommandNoLock(cmd);
}

template <>
void StateFieldGraphicsProgram::dumpLog(const FrontGraphicsStateStorage &) const
{
  if (!handle)
  {
    debug("Program: none");
    return;
  }

  debug("Program: %u\n  ", handle.get());
  // we can't read debug info here, as method can be called in frontend
}

// back fields impl

template <>
void StateFieldGraphicsRenderPassScopeOpener::dumpLog(const BackGraphicsStateStorage &) const
{
  debug("rpOpener: need to open pass %s", data ? "yes" : "no");
}

template <>
void StateFieldGraphicsRenderPassScopeOpener::applyTo(BackGraphicsStateStorage &state, ExecutionContext &target) const
{
  TIME_PROFILE(vulkan_render_pass_scope_opener);
  G_ASSERTF(data, "vulkan: can't use graphics stage without render pass");

  if (state.nativeRenderPass.ptr)
  {
    if (state.inPass.data == InPassStateFieldType::NATIVE_PASS)
      target.nextNativeSubpass();
    else
    {
      if (Globals::cfg.bits.fatalOnNRPSplit && state.inPass.data != InPassStateFieldType::NONE)
        DAG_FATAL("vulkan: can't start new pass without closing old");
      target.beginNativePass();
    }
    return;
  }
  if (Globals::cfg.bits.fatalOnNRPSplit && state.inPass.data != InPassStateFieldType::NONE)
    DAG_FATAL("vulkan: can't start new pass without closing old");

  VulkanDevice &vkDev = Globals::VK::dev;

  RenderPassClass *nextRenderPassClass = state.rpClass.ptr;
  FramebufferState &fbState = target.getFramebufferState();
  RenderPassClass::Identifier passClassIdent = fbState.renderPassClass;

  if (nextRenderPassClass && (nextRenderPassClass->getIdentifier() != passClassIdent))
    nextRenderPassClass = nullptr;

  if (!nextRenderPassClass)
  {
    nextRenderPassClass = Globals::passes.getPassClass(passClassIdent);
    state.framebuffer.handle = VulkanNullHandle();
  }

  fbState.frameBufferInfo.extent = fbState.frameBufferInfo.makeDrawArea(state.viewport.data.rect2D.extent);
  VulkanFramebufferHandle nextFramebuffer = nextRenderPassClass->getFrameBuffer(vkDev, fbState.frameBufferInfo);

  target.beginPassInternal(nextRenderPassClass, nextFramebuffer, state.viewport.data.rect2D);
}

template <>
void StateFieldGraphicsRenderPassEarlyScopeOpener::dumpLog(const BackGraphicsStateStorage &) const
{
  debug("earlyRpOpener: need to open pass %s", data ? "yes" : "no");
}

template <>
void StateFieldGraphicsRenderPassEarlyScopeOpener::applyTo(BackGraphicsStateStorage &, ExecutionContext &target) const
{
  G_ASSERTF(data, "vulkan: can't use graphics stage without render pass");

  // keep splitting buffers on some targets as sync there is broken on driver level
  if (Globals::desc.issues.hasRenderPassClearDataRace)
    target.interruptFrameCoreForRenderPassStart();
}

template <>
void StateFieldGraphicsRenderPassClass::applyTo(BackGraphicsStateStorage &, ExecutionContext &) const
{}

template <>
void StateFieldGraphicsRenderPassClass::dumpLog(const BackGraphicsStateStorage &) const
{
  if (!ptr)
    debug("rpClass: <none>");
  else
    debug("rpClass: 0x%p", ptr);
}

template <>
void StateFieldGraphicsBasePipeline::applyTo(BackGraphicsStateStorage &state, ExecutionContext &target)
{
  if (!state.flush.needPipeline)
    return;

  if (!ptr)
  {
    target.reportMissingPipelineComponent("base");
    return;
  }

  auto &regs = ptr->getLayout()->registers;
  if (regs.fs().header.inputAttachmentCount)
  {
    G_ASSERTF(state.nativeRenderPass.ptr,
      "vulkan: trying to use pipeline with input attachment while render pass is not bound. Additional info %s",
      ptr->printDebugInfoBuffered());
    const auto &header = regs.fs().header;
    uint32_t iaCnt = regs.fs().header.inputAttachmentCount;
    for (uint32_t i = 0; i < header.registerCount && iaCnt > 0; ++i)
    {
      if (header.inputAttachmentIndex[i] == spirv::INVALID_INPUT_ATTACHMENT_INDEX)
        continue;
      state.nativeRenderPass.ptr->bindInputAttachments(target, Backend::State::exec.getResBinds(STAGE_PS),
        header.inputAttachmentIndex[i], header.registerIndex[i], ptr);
      --iaCnt;
    }
  }
}

template <>
void StateFieldGraphicsBasePipeline::dumpLog(const BackGraphicsStateStorage &) const
{
  if (!ptr)
    debug("basePipeline: none");
  else
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
    debug("basePipeline:\n  %s", ptr->printDebugInfoBuffered());
#else
    debug("basePipeline: %p", ptr);
#endif
}

template <>
void StateFieldGraphicsPipeline::applyTo(BackGraphicsStateStorage &state, ExecutionContext &target)
{
  // TODO: refactor this checks to separate nested field
  if (!state.basePipeline.ptr)
  {
    if (state.flush.needPipeline)
      target.reportMissingPipelineComponent("variant compile");
    return;
  }

  if (!state.flush.needPipeline)
  {
    ptr = nullptr;
    return;
  }

  VulkanDevice &vkDev = Globals::VK::dev;

  // can be rebinded if non nullptr
  if (!ptr)
  {
    GraphicsPipelineVariantDescription varDsc = {Backend::State::exec.get<BackGraphicsState, BackGraphicsState>().pipelineState,
      state.rpClass.ptr ? state.rpClass.ptr->getIdentifier() : RenderPassClass::Identifier(),
      state.nativeRenderPass.ptr ? state.nativeRenderPass.ptr->getHash() : 0,
      state.nativeRenderPass.ptr ? state.nativeRenderPass.ptr->getCurrentSubpass() : 0, state.primTopo.data};

    VariatedGraphicsPipeline::CompilationContext compCtx = {vkDev, Backend::renderStateSystem, Globals::pipeCache.getHandle(),
      state.nativeRenderPass.ptr, Backend::State::pod.asyncPipelineCompileFeedbackPtr, Backend::State::pod.nonDrawCompile};

    ptr = state.basePipeline.ptr->getVariant(compCtx, varDsc);
  }

  if (oldPtr == ptr && ptr)
    return;

  target.renderAllowed = ptr != nullptr;
  if (ptr)
  {
    VULKAN_LOG_CALL(Backend::cb.wCmdBindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, ptr->getHandle()));
  }
}

template <>
void StateFieldGraphicsPipeline::dumpLog(const BackGraphicsStateStorage &) const
{
  if (!ptr)
    debug("pipeline: was changed");
  else
    debug("pipeline: ptr %p handle %016llX", ptr, generalize(ptr->getHandle()));
}

template <>
void StateFieldGraphicsPipelineLayout::applyTo(BackGraphicsStateStorage &state, ExecutionContext &target) const
{
  if (!ptr)
  {
    if (state.flush.needPipeline)
      target.reportMissingPipelineComponent("layout");
    return;
  }

  Backend::bindless.bindSets(VK_PIPELINE_BIND_POINT_GRAPHICS, ptr->handle);
  Backend::State::exec.getResBinds(STAGE_VS).invalidateState();
  Backend::State::exec.getResBinds(STAGE_PS).invalidateState();
}

template <>
void StateFieldGraphicsPipelineLayout::dumpLog(const BackGraphicsStateStorage &) const
{
  if (!ptr)
  {
    debug("graphicsPipelineLayout: none");
    return;
  }

  String stages(30, "VS:1 TCS:%d TES:%d GS:%d FS:1", (int)ptr->hasTC(), (int)ptr->hasTE(), (int)ptr->hasGS());

  debug("graphicsPipelineLayout: ptr %p handle %llu %s", ptr, generalize(ptr->handle), stages.c_str());

  debug("  vs:");
  ptr->registers.vs().dumpLogGeneralInfo();

  debug("  fs:");
  ptr->registers.fs().dumpLogGeneralInfo();

  if (ptr->hasGS())
  {
    debug("  gs:");
    ptr->registers.gs().dumpLogGeneralInfo();
  }

  if (ptr->hasTC())
  {
    debug("  tcs:");
    ptr->registers.tc().dumpLogGeneralInfo();
  }

  if (ptr->hasTE())
  {
    debug("  tes:");
    ptr->registers.te().dumpLogGeneralInfo();
  }
}

template <>
void StateFieldGraphicsFramebuffer::applyTo(BackGraphicsStateStorage &, ExecutionContext &) const
{}

template <>
void StateFieldGraphicsFramebuffer::dumpLog(const BackGraphicsStateStorage &) const
{
  debug("framebuffer: %016llX", generalize(handle));
}

template <>
void StateFieldGraphicsInPass::applyTo(BackGraphicsStateStorage &, ExecutionContext &) const
{}

template <>
void StateFieldGraphicsInPass::dumpLog(const BackGraphicsStateStorage &) const
{
  debug("inPass: %s", data == InPassStateFieldType::NONE ? "NONE" : "NORMAL_PASS");
}

template <>
void StateFieldGraphicsScissorRect::applyTo(FrontGraphicsStateStorage &, ExecutionState &target) const
{
  target.set<StateFieldGraphicsScissor, VkRect2D, BackGraphicsState, BackDynamicGraphicsState>(data);
}

template <>
void StateFieldGraphicsScissorRect::dumpLog(const FrontGraphicsStateStorage &) const
{
  debug("scissorRect: ofs (%i, %i) ext (%u, %u)", data.offset.x, data.offset.y, data.extent.width, data.extent.height);
}

template <>
void StateFieldGraphicsScissorRect::transit(FrontGraphicsStateStorage &, DeviceContext &target) const
{
  CmdSetScissorRect cmd{data};
  target.dispatchCommandNoLock(cmd);
}

template <>
void StateFieldGraphicsScissor::applyTo(BackDynamicGraphicsStateStorage &, ExecutionContext &) const
{
  VULKAN_LOG_CALL(Backend::cb.wCmdSetScissor(0, 1, enabled ? &rect : &viewRect));
}

template <>
void StateFieldGraphicsScissor::dumpLog(const BackDynamicGraphicsStateStorage &) const
{
  debug("scissorRect: ofs (%i, %i) ext (%u, %u)", rect.offset.x, rect.offset.y, rect.extent.width, rect.extent.height);
  debug("scissorEnabled: %s", enabled ? "yes" : "no");
}

template <>
void StateFieldGraphicsDepthBias::applyTo(BackDynamicGraphicsStateStorage &, ExecutionContext &) const
{
  // FIXME: restore dyn mask feature
  // if (dynamicStateMask.hasDepthBias)
  VULKAN_LOG_CALL(Backend::cb.wCmdSetDepthBias(data.bias / MINIMUM_REPRESENTABLE_D16, 0.f, data.slopedBias));
}

template <>
void StateFieldGraphicsDepthBias::dumpLog(const BackDynamicGraphicsStateStorage &) const
{
  debug("depthBias: %f", data.bias);
  debug("depthBiasSloped: %f", data.slopedBias);
}

template <>
void StateFieldGraphicsDepthBounds::applyTo(BackGraphicsStateStorage &, ExecutionContext &) const
{
  // FIXME: restore dyn mask feature
  // if (dynamicStateMask.hasDepthBoundsTest)
  VULKAN_LOG_CALL(Backend::cb.wCmdSetDepthBounds(data.min, data.max));
}

template <>
void StateFieldGraphicsDepthBounds::applyTo(FrontGraphicsStateStorage &, ExecutionState &target) const
{
  using FieldName = StateFieldGraphicsDepthBounds;
  target.set<FieldName, FieldName::DataType, BackGraphicsState>(data);
}

template <>
void StateFieldGraphicsDepthBounds::transit(FrontGraphicsStateStorage &, DeviceContext &target) const
{
  CmdSetDepthBoundsRange cmd{data.min, data.max};
  target.dispatchCommandNoLock(cmd);
}

template <>
void StateFieldGraphicsDepthBounds::dumpLog(const BackGraphicsStateStorage &) const
{
  debug("depthBounds: min %f max %f", data.min, data.max);
}

template <>
void StateFieldGraphicsDepthBounds::dumpLog(const FrontGraphicsStateStorage &) const
{
  debug("depthBounds: min %f max %f", data.min, data.max);
}

template <>
void StateFieldGraphicsStencilRef::applyTo(BackDynamicGraphicsStateStorage &state, ExecutionContext &) const
{
  if (state.stencilRefOverride.data == STENCIL_REF_OVERRIDE_DISABLE)
    VULKAN_LOG_CALL(Backend::cb.wCmdSetStencilReference(VK_STENCIL_FACE_BOTH_BIT, data));
}

template <>
void StateFieldGraphicsStencilRef::dumpLog(const BackDynamicGraphicsStateStorage &) const
{
  debug("stencilRef: %u", data);
}

template <>
void StateFieldGraphicsStencilRefOverride::applyTo(FrontGraphicsStateStorage &, ExecutionState &target) const
{
  target.set<StateFieldGraphicsStencilRefOverride, uint16_t, BackGraphicsState, BackDynamicGraphicsState>(data);
}

template <>
void StateFieldGraphicsStencilRefOverride::transit(FrontGraphicsStateStorage &, DeviceContext &target) const
{
  CmdSetStencilRefOverride cmd{data};
  target.dispatchCommandNoLock(cmd);
}

template <>
void StateFieldGraphicsStencilRefOverride::applyTo(BackDynamicGraphicsStateStorage &state, ExecutionContext &) const
{
  auto newStencilRef = state.stencilRef.data;
  if (data != STENCIL_REF_OVERRIDE_DISABLE)
    newStencilRef = data;
  VULKAN_LOG_CALL(Backend::cb.wCmdSetStencilReference(VK_STENCIL_FACE_BOTH_BIT, newStencilRef));
}

template <>
void StateFieldGraphicsStencilRefOverride::dumpLog(const BackDynamicGraphicsStateStorage &) const
{
  if (data != STENCIL_REF_OVERRIDE_DISABLE)
    debug("stencilRefOverride: %u", data);
  else
    debug("stencilRefOverride: disabled");
}

template <>
void StateFieldGraphicsStencilRefOverride::dumpLog(const FrontGraphicsStateStorage &) const
{
  if (data != STENCIL_REF_OVERRIDE_DISABLE)
    debug("stencilRefOverride: %u", data);
  else
    debug("stencilRefOverride: disabled");
}

template <>
void StateFieldGraphicsStencilMask::applyTo(BackDynamicGraphicsStateStorage &, ExecutionContext &) const
{
  VULKAN_LOG_CALL(Backend::cb.wCmdSetStencilWriteMask(VK_STENCIL_FACE_BOTH_BIT, data));
  VULKAN_LOG_CALL(Backend::cb.wCmdSetStencilCompareMask(VK_STENCIL_FACE_BOTH_BIT, data));
}

template <>
void StateFieldGraphicsStencilMask::dumpLog(const BackDynamicGraphicsStateStorage &) const
{
  debug("stencilMask: %u", data);
}

template <>
void StateFieldGraphicsDynamicRenderStateIndex::applyTo(BackGraphicsStateStorage &state, ExecutionContext &) const
{
  auto dynamicStateDesc = Backend::renderStateSystem.getDynamic(data);

  // FIXME: restore dyn mask feature
  // GraphicsPipelineDynamicStateMask dynamicStateMask(0);
  // if (pipeline)
  //   dynamicStateMask = pipeline->getDynamicStateMask();

  using DBias = StateFieldGraphicsDepthBias;
  state.dynamic.set<DBias, DBias::DataType>({dynamicStateDesc.depthBias, dynamicStateDesc.slopedDepthBias});
  state.dynamic.set<StateFieldGraphicsStencilMask>(dynamicStateDesc.stencilMask);
  state.dynamic.set<StateFieldGraphicsStencilRef>(dynamicStateDesc.stencilRef);
  state.dynamic.set<StateFieldGraphicsScissor>(dynamicStateDesc.enableScissor);
}

template <>
void StateFieldGraphicsDynamicRenderStateIndex::dumpLog(const BackGraphicsStateStorage &) const
{
  debug("dynamicRenderStateIndex: %u", data);
}

template <>
void StateFieldGraphicsBlendConstantFactor::applyTo(BackGraphicsStateStorage &, ExecutionContext &) const
{
  // FIXME: restore dyn mask feature
  // if (dynamicStateMask.hasBlendConstants)
  const float values[] = //
    {data.r / 255.f, data.g / 255.f, data.b / 255.f, data.a / 255.f};
  VULKAN_LOG_CALL(Backend::cb.wCmdSetBlendConstants(values));
}

template <>
void StateFieldGraphicsBlendConstantFactor::transit(FrontGraphicsStateStorage &, DeviceContext &target) const
{
  CmdSetBlendConstantFactor cmd{data};
  target.dispatchCommandNoLock(cmd);
}

template <>
void StateFieldGraphicsBlendConstantFactor::applyTo(FrontGraphicsStateStorage &, ExecutionState &target) const
{
  target.set<StateFieldGraphicsBlendConstantFactor, E3DCOLOR, BackGraphicsState>(data);
}

template <>
void StateFieldGraphicsBlendConstantFactor::dumpLog(const BackGraphicsStateStorage &) const
{
  debug("blendConstantFactor: (%u %u %u %u)", data.r, data.g, data.b, data.a);
}

template <>
void StateFieldGraphicsBlendConstantFactor::dumpLog(const FrontGraphicsStateStorage &) const
{
  debug("blendConstantFactor: (%u %u %u %u)", data.r, data.g, data.b, data.a);
}

template <>
void StateFieldGraphicsIndexBuffer::applyTo(BackGraphicsStateStorage &, ExecutionContext &target) const
{
  if (!data.buffer)
    return;

  target.verifyResident(data.buffer);
  Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_INDEX_READ_BIT}, data.buffer,
    {data.bufOffset(0), data.visibleDataSize});
  VULKAN_LOG_CALL(Backend::cb.wCmdBindIndexBuffer(data.getHandle(), data.bufOffset(0), indexType));
}

template <>
void StateFieldGraphicsIndexBuffer::dumpLog(const BackGraphicsStateStorage &) const
{
  if (data.buffer)
    debug("IB: ptr %p handle %llu ofs %u range %u name %s", data.buffer, generalize(data.getHandle()), data.bufOffset(0),
      data.visibleDataSize, data.buffer->getDebugName());
  else
    debug("IB: none");
}

template <>
void StateFieldGraphicsIndexBuffer::dumpLog(const FrontGraphicsStateStorage &) const
{
  if (data.buffer)
    // do not dereference buffer here, it can be dead one
    debug("IB: ptr %p", data.buffer);
  else
    debug("IB: none");
}

template <>
void StateFieldGraphicsIndexBuffer::transit(FrontGraphicsStateStorage &, DeviceContext &target) const
{
  CmdSetIndexBuffer cmd{data, indexType};
  target.dispatchCommandNoLock(cmd);
}

template <>
void StateFieldGraphicsIndexBuffer::applyTo(FrontGraphicsStateStorage &, ExecutionState &target) const
{
  target.set<StateFieldGraphicsIndexBuffer, BufferRef, BackGraphicsState>(data);
  target.set<StateFieldGraphicsIndexBuffer, VkIndexType, BackGraphicsState>(indexType);
}

VkViewport viewport_state_to_vk_viewport(ViewportState data)
{
  VkViewport vp;
  VulkanDevice &vkDev = Globals::VK::dev;
#if VK_AMD_negative_viewport_height
  if (vkDev.hasExtension<NegativeViewportHeightAMD>())
  {
    // if the amd extension is used, the negated height means
    // use abs(height) and do a y flip like dx11
    vp.y = data.rect2D.offset.y;
  }
  else
#endif
  {
    // negative height means we need to offset by the height to transform the rect correctly
    vp.y = data.rect2D.offset.y + data.rect2D.extent.height;
  }
  // turn on y flip, its an extension but we require the support so no checks
  vp.height = -(float)data.rect2D.extent.height;
  vp.x = data.rect2D.offset.x;
  vp.width = data.rect2D.extent.width;
  vp.minDepth = data.minZ;
  vp.maxDepth = data.maxZ;

  return vp;
}

void StateFieldGraphicsViewport::set(const RestoreFromFramebuffer &)
{
  // this approach is bad, consider removing it when getter for viewport is no longer supported in d3d::
  PipelineState &pipeState = Frontend::State::pipe;
  Driver3dRenderTarget &rt = pipeState.get<FrontFramebufferState, FrontFramebufferState, FrontGraphicsState>().asDriverRT();

  data.rect2D.offset.x = 0;
  data.rect2D.offset.y = 0;

  data.rect2D.extent = Globals::swapchain.getMode().extent;

  if (!rt.isBackBufferColor())
  {
    if (rt.isColorUsed())
    {
      for (uint32_t i = 0; i < Driver3dRenderTarget::MAX_SIMRT; ++i)
      {
        if (rt.isColorUsed(i))
        {
          const VkExtent3D &size = cast_to_texture_base(*rt.color[i].tex).getMipmapExtent(rt.color[i].level);
          data.rect2D.extent.width = size.width;
          data.rect2D.extent.height = size.height;
          break;
        }
      }
    }
    else if (rt.isDepthUsed() && rt.depth.tex)
    {
      const VkExtent3D &size = cast_to_texture_base(*rt.depth.tex).getMipmapExtent(rt.depth.level);
      data.rect2D.extent.width = size.width;
      data.rect2D.extent.height = size.height;
    }
  }
}

template <>
void StateFieldGraphicsViewport::dumpLog(const FrontGraphicsStateStorage &) const
{
  VkViewport vp = viewport_state_to_vk_viewport(data);
  debug("viewport: ofs(%i, %i) extend(%f, %f) depth(%f, %f)", vp.x, vp.y, vp.width, vp.height, vp.minDepth, vp.maxDepth);
}

template <>
void StateFieldGraphicsViewport::applyTo(FrontGraphicsStateStorage &state, ExecutionState &target) const
{
  ViewportState vp = data;
  FramebufferState &fbs = target.get<BackGraphicsState, BackGraphicsState>().framebufferState;
  ExecutionContext &ctx = target.getExecutionContext();
  RenderPassResource *rpRes = state.renderpass.getRO<StateFieldRenderPassResource, RenderPassResource *>();

  // need to check viewport for selected clear targets, as sometimes clear for depth
  // is requested with a bigger color target bound which provides a vp that would be
  // too large
  VkExtent2D maxExt = rpRes ? rpRes->getMaxActiveAreaExtent() : fbs.frameBufferInfo.makeDrawArea(vp.rect2D.extent);
  vp.rect2D.extent.width = min(vp.rect2D.extent.width, maxExt.width - vp.rect2D.offset.x);
  vp.rect2D.extent.height = min(vp.rect2D.extent.height, maxExt.height - vp.rect2D.offset.y);

  // force viewport to full internal bb extent
  // if we are about to write in pre rotate pass
  if (ctx.swapchain.inPreRotate())
  {
    vp.rect2D.offset = {0, 0};
    maxExt = ctx.swapchain.getColorImage()->getMipExtents2D(0);
    vp.rect2D.extent = maxExt;
    vp.maxZ = 1.0f;
    vp.minZ = 0.0f;
  }

  if (!rpRes)
  {
    RegionDifference dif =
      classify_rect2d_diff(target.getRO<StateFieldGraphicsRenderPassArea, VkRect2D, BackGraphicsState>(), vp.rect2D);
    if (dif == RegionDifference::SUPERSET)
      target.interruptRenderPass("ViewportExtend");
  }
  target.set<StateFieldGraphicsViewport, ViewportState, BackGraphicsState>(vp);
}

template <>
void StateFieldGraphicsViewport::transit(FrontGraphicsStateStorage &, DeviceContext &target) const
{
  CmdSetViewport cmd{data};
  target.dispatchCommandNoLock(cmd);
}

template <>
void StateFieldGraphicsViewport::applyTo(BackGraphicsStateStorage &state, ExecutionContext &) const
{
  VkViewport translatedViewport = viewport_state_to_vk_viewport(data);
  // update scissor copy of viewport that is used when scissor is off
  state.dynamic.set<StateFieldGraphicsScissor>(data);
  VULKAN_LOG_CALL(Backend::cb.wCmdSetViewport(0, 1, &translatedViewport));
}

template <>
void StateFieldGraphicsViewport::dumpLog(const BackGraphicsStateStorage &) const
{
  VkViewport vp = viewport_state_to_vk_viewport(data);
  debug("viewport: ofs(%i, %i) extend(%f, %f) depth(%f, %f)", vp.x, vp.y, vp.width, vp.height, vp.minDepth, vp.maxDepth);
}

template <>
void StateFieldGraphicsVertexBufferBind::dumpLog(uint32_t index, const FrontGraphicsStateStorage &) const
{
  if (!bRef)
  {
    debug("vertexBuffer%u: empty", index);
    return;
  }

  // do not dereference buffer pointer here, it can be a dead one
  debug("vertexBuffer%u: ptr %p raw offset %u", index, bRef.buffer, offset);
}

template <>
void StateFieldGraphicsVertexBufferBind::transit(uint32_t index, FrontGraphicsStateStorage &, DeviceContext &target) const
{
  CmdSetVertexBuffer cmd{index, bRef, offset};
  target.dispatchCommandNoLock(cmd);
}

template <>
void StateFieldGraphicsVertexBufferBind::applyTo(uint32_t index, FrontGraphicsStateStorage &, ExecutionState &target) const
{
  target.set<StateFieldGraphicsVertexBuffersBindArray, Indexed, BackGraphicsState>({index, *this});
}

bool StateFieldGraphicsVertexBufferBind::diff(const StateFieldGraphicsVertexBufferBind &v) const
{
  return (bRef != v.bRef) || (offset != v.offset);
}

void StateFieldGraphicsVertexBufferBind::set(const StateFieldGraphicsVertexBufferBind &v)
{
  bRef = v.bRef;
  offset = v.offset;
}

template <>
void StateFieldGraphicsVertexBuffersBindArray::applyTo(BackGraphicsStateStorage &, ExecutionContext &target) const
{
  if (!countMask)
    return;

  uint32_t cnt = 0;
  for (uint32_t i = countMask; i != 0; i >>= 1)
  {
    // by default no allowance to have empty slots in bind-vector (bubble)
    if ((i & 1) == 0)
      D3D_ERROR("vulkan: vb binding filtered due empty slot %u (mask %u)", cnt + 1, countMask);
    else
      ++cnt;
  }

  for (uint32_t i = 0; i < cnt; ++i)
  {
    //    G_ASSERTF(offsets[i] >= resPtrs[i]->bufOffsetLoc(0),
    //      "vulkan: old discard index is referenced/invalid offset (zero offset %u specified %u) for vb[%u] %p:%s",
    //      resPtrs[i]->bufOffsetLoc(0), offsets[i], i, resPtrs[i], resPtrs[i]->getDebugName());
    target.verifyResident(resPtrs[i]);
    Backend::sync.addBufferAccess({VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT}, resPtrs[i],
      {offsets[i], resPtrs[i]->getBlockSize() - (offsets[i] % resPtrs[i]->getBlockSize())});
  }

  VULKAN_LOG_CALL(Backend::cb.wCmdBindVertexBuffers(0, cnt, ary(buffers), offsets));
}

template <>
void StateFieldGraphicsVertexBuffersBindArray::dumpLog(const BackGraphicsStateStorage &) const
{
  debug("vertexBuffersArray: mask %u");
  for (uint32_t i = 0; i < MAX_VERTEX_INPUT_STREAMS; ++i)
  {
    if (countMask & (1 << i))
      debug("vertexBuffersArray:   idx %u handle %llu offset %u", i, generalize(buffers[i]), offsets[i]);
    else
      debug("vertexBuffersArray:   idx %u empty", i);
  }
}

template <>
void StateFieldGraphicsVertexBufferStride::dumpLog(uint32_t index, const FrontGraphicsStateStorage &) const
{
  debug("vertexBuffer%u: stride %u", index, data);
}

template <>
void StateFieldGraphicsVertexBufferStride::transit(uint32_t index, FrontGraphicsStateStorage &, DeviceContext &target) const
{
  CmdSetVertexStride cmd{index, data};
  target.dispatchCommandNoLock(cmd);
}

template <>
void StateFieldGraphicsVertexBufferStride::applyTo(uint32_t index, FrontGraphicsStateStorage &, ExecutionState &target) const
{
  target.get<BackGraphicsState, BackGraphicsState>().pipelineState.strides[index] = data;
  target.set<StateFieldGraphicsPipeline, StateFieldGraphicsPipeline::Invalidate, BackGraphicsState>(1);
}

///

template <>
void StateFieldGraphicsPrimitiveTopology::applyTo(BackGraphicsStateStorage &state, ExecutionContext &ctx) const
{
#if DAGOR_DBGLEVEL > 0
  if (!state.basePipeline.ptr || !state.flush.needPipeline)
    return;

  VkPrimitiveTopology actual = useTessOverride ? VK_PRIMITIVE_TOPOLOGY_PATCH_LIST : data;
  bool hasGS = state.basePipeline.ptr->hasGeometryStage();
  bool hasTesselation = state.basePipeline.ptr->hasTesselationStage();

  if (!validate_primitive_topology(actual, hasGS, hasTesselation))
  {
    D3D_ERROR("Invalid primitive topology %s for draw call encountered. Geometry stage active %s, "
              "Tessellation stage active %s. Caller: %s",
      formatPrimitiveTopology(actual), hasGS ? "yes" : "no", hasTesselation ? "yes" : "no", ctx.getCurrentCmdCaller());
  }
#else
  G_UNUSED(state);
  G_UNUSED(ctx);
#endif
}

template <>
void StateFieldGraphicsPrimitiveTopology::dumpLog(const BackGraphicsStateStorage &) const
{
  debug("primTopo: %s tessOverride: %s", formatPrimitiveTopology(data), useTessOverride ? "yes" : "no");
}

void StateFieldGraphicsFlush::syncTLayoutsToRenderPass(BackGraphicsStateStorage &state, ExecutionContext &target) const
{
  if (state.inPass.data == InPassStateFieldType::NONE && !state.nativeRenderPass.ptr)
    target.ensureStateForDepthAttachment(state.viewport.data.rect2D);
  else if (state.nativeRenderPass.ptr)
    target.ensureStateForNativeRPDepthAttachment();
}

void StateFieldGraphicsFlush::applyDescriptors(BackGraphicsStateStorage &state, ExecutionContext &target) const
{
  VariatedGraphicsPipeline *ptr = state.basePipeline.ptr;
  GraphicsPipelineLayout &layout = *ptr->getLayout();
  VulkanPipelineLayoutHandle layoutHandle = layout.handle;
  auto &regs = layout.registers;

  VulkanDevice &vkDev = target.vkDev;
  const ResourceDummySet &dummyResTbl = Globals::dummyResources.getTable();
  size_t frameIndex = Backend::gpuJob->index;
  PipelineStageStateBase &vsResBinds = Backend::State::exec.getResBinds(STAGE_VS);

  vsResBinds.apply(vkDev, dummyResTbl, frameIndex, regs.vs(), ExtendedShaderStage::VS,
    [layoutHandle](VulkanDescriptorSetHandle set, const uint32_t *offsets, uint32_t offset_count) //
    {
      VULKAN_LOG_CALL(Backend::cb.wCmdBindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, layoutHandle,
        ShaderStageTraits<VK_SHADER_STAGE_VERTEX_BIT>::register_index + PipelineBindlessConfig::bindlessSetCount, 1, ary(&set),
        offset_count, offsets));
    });

  Backend::State::exec.getResBinds(STAGE_PS).apply(vkDev, dummyResTbl, frameIndex, regs.fs(), ExtendedShaderStage::PS,
    [layoutHandle](VulkanDescriptorSetHandle set, const uint32_t *offsets, uint32_t offset_count) //
    {
      VULKAN_LOG_CALL(Backend::cb.wCmdBindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, layoutHandle,
        ShaderStageTraits<VK_SHADER_STAGE_FRAGMENT_BIT>::register_index + PipelineBindlessConfig::bindlessSetCount, 1, ary(&set),
        offset_count, offsets));
    });

  if (layout.hasGS())
  {
    vsResBinds.applyNoDiff(vkDev, dummyResTbl, frameIndex, regs.gs(), ExtendedShaderStage::GS,
      [layoutHandle](VulkanDescriptorSetHandle set, const uint32_t *offsets, uint32_t offset_count) //
      {
        VULKAN_LOG_CALL(Backend::cb.wCmdBindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, layoutHandle,
          ShaderStageTraits<VK_SHADER_STAGE_GEOMETRY_BIT>::register_index + PipelineBindlessConfig::bindlessSetCount, 1, ary(&set),
          offset_count, offsets));
      });
  }

  if (layout.hasTC())
  {
    vsResBinds.applyNoDiff(vkDev, dummyResTbl, frameIndex, regs.tc(), ExtendedShaderStage::TC,
      [layoutHandle](VulkanDescriptorSetHandle set, const uint32_t *offsets, uint32_t offset_count) //
      {
        VULKAN_LOG_CALL(Backend::cb.wCmdBindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, layoutHandle,
          ShaderStageTraits<VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT>::register_index + PipelineBindlessConfig::bindlessSetCount, 1,
          ary(&set), offset_count, offsets));
      });
  }

  if (layout.hasTE())
  {
    vsResBinds.applyNoDiff(vkDev, dummyResTbl, frameIndex, regs.te(), ExtendedShaderStage::TE,
      [layoutHandle](VulkanDescriptorSetHandle set, const uint32_t *offsets, uint32_t offset_count) //
      {
        VULKAN_LOG_CALL(Backend::cb.wCmdBindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, layoutHandle,
          ShaderStageTraits<VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT>::register_index + PipelineBindlessConfig::bindlessSetCount, 1,
          ary(&set), offset_count, offsets));
      });
  }

  if (Globals::cfg.debugLevel > 0)
  {
    ptr->setNameForDescriptorSet(regs.vs().getSetForNaming(), spirv::graphics::vertex::REGISTERS_SET_INDEX);
    ptr->setNameForDescriptorSet(regs.fs().getSetForNaming(), spirv::graphics::fragment::REGISTERS_SET_INDEX);
    if (layout.hasGS())
      ptr->setNameForDescriptorSet(regs.gs().getSetForNaming(), spirv::graphics::geometry::REGISTERS_SET_INDEX);
    if (layout.hasTC())
      ptr->setNameForDescriptorSet(regs.tc().getSetForNaming(), spirv::graphics::control::REGISTERS_SET_INDEX);
    if (layout.hasTE())
      ptr->setNameForDescriptorSet(regs.te().getSetForNaming(), spirv::graphics::evaluation::REGISTERS_SET_INDEX);
  }
}

void StateFieldGraphicsFlush::applyBarriers(BackGraphicsStateStorage &state, ExecutionContext &target) const
{
  VariatedGraphicsPipeline *ptr = state.basePipeline.ptr;
  GraphicsPipelineLayout &layout = *ptr->getLayout();
  auto &regs = layout.registers;

  target.trackStageResAccesses(regs.vs().header, ExtendedShaderStage::VS);
  target.trackStageResAccesses(regs.fs().header, ExtendedShaderStage::PS);
  if (layout.hasGS())
    target.trackStageResAccesses(regs.gs().header, ExtendedShaderStage::GS);
  if (layout.hasTC())
    target.trackStageResAccesses(regs.tc().header, ExtendedShaderStage::TC);
  if (layout.hasTE())
    target.trackStageResAccesses(regs.te().header, ExtendedShaderStage::TE);
}

template <>
void StateFieldGraphicsFlush::applyTo(BackGraphicsStateStorage &state, ExecutionContext &target) const
{
  syncTLayoutsToRenderPass(state, target);

  if (state.basePipeline.ptr && needPipeline)
  {
    applyBarriers(state, target);
    applyDescriptors(state, target);
  }
}

template <>
void StateFieldGraphicsFlush::dumpLog(const BackGraphicsStateStorage &) const
{
  debug("flush: %s", needPipeline ? "draw" : "rp-actions");
}

template <>
void StateFieldGraphicsQueryState::applyTo(FrontGraphicsStateStorage &, ExecutionState &target) const
{
  target.set<StateFieldGraphicsQueryScopeOpener, GraphicsQueryState, BackGraphicsState>(data);
  target.set<StateFieldGraphicsQueryScopeCloser, GraphicsQueryState, BackScopeState>(data);
}

template <>
void StateFieldGraphicsQueryScopeOpener::applyTo(BackGraphicsStateStorage &state, ExecutionContext &target)
{
  G_UNUSED(state);

  if (data != GraphicsQueryState{})
    target.beginQuery(data.pool, data.index, 0);
}

template <>
void StateFieldGraphicsQueryScopeCloser::applyTo(BackScopeStateStorage &, ExecutionContext &target)
{
  // we should close on next field apply if follow up code will open scope
  bool askingForOpener = data != GraphicsQueryState{};
  bool stageWillPerformOpener = stage == ActiveExecutionStage::GRAPHICS;

  bool scopeWillOpen = askingForOpener && stageWillPerformOpener;
  if (eastl::exchange(shouldClose, scopeWillOpen))
    target.endQuery(data.pool, data.index);
}

template <>
void StateFieldGraphicsQueryState::transit(FrontGraphicsStateStorage &, DeviceContext &target) const
{
  CmdSetGraphicsQuery cmd{data.pool, data.index};
  target.dispatchCommandNoLock(cmd);
}

template <>
void StateFieldGraphicsQueryState::dumpLog(const FrontGraphicsStateStorage &) const
{
  if (data == GraphicsQueryState{})
    debug("graphics query: inactive");
  else
    debug("graphics query: active on pool %" PRIu64 ", index %d", generalize(data.pool), data.index);
}

template <>
void StateFieldGraphicsQueryScopeOpener::dumpLog(const BackGraphicsStateStorage &) const
{
  if (data == GraphicsQueryState{})
    debug("graphics query opener: inactive");
  else
    debug("graphics query opener: active on pool %" PRIu64 ", index %d", generalize(data.pool), data.index);
}

template <>
void StateFieldGraphicsQueryScopeCloser::dumpLog(const BackScopeStateStorage &) const
{
  if (shouldClose)
    debug("graphics query closer: pending close on next apply");
  else
    debug("graphics query closer: no close pendin");
}

template <>
void StateFieldGraphicsRenderPassArea::applyTo(BackGraphicsStateStorage &, ExecutionContext &) const
{}

template <>
void StateFieldGraphicsRenderPassArea::dumpLog(const BackGraphicsStateStorage &) const
{
  debug("renderPassArea: ofs (%i, %i) ext (%u, %u)", data.offset.x, data.offset.y, data.extent.width, data.extent.height);
}

} // namespace drv3d_vulkan

using namespace drv3d_vulkan;
