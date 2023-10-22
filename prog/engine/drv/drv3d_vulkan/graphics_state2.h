// defines full draw/dispatch ready state that can be applied to execution context
#pragma once
#include "util/tracked_state.h"
#include "state_field_graphics.h"
#include "dynamic_graphics_state.h"
#include "state_field_resource_binds.h"
#include "front_framebuffer_state.h"
#include "front_render_pass_state.h"
#include "render_pass_resource.h"

namespace drv3d_vulkan
{

struct FrontGraphicsStateStorage
{
  StateFieldGraphicsProgram program;
  StateFieldGraphicsPolygonLine polygonLine;
  StateFieldGraphicsBlendConstantFactor blendConstantFactor;
  StateFieldGraphicsVertexBufferStrides vertexBufferStrides;
  StateFieldGraphicsDepthBounds depthBounds;
  StateFieldGraphicsScissorRect scissorRect;
  StateFieldGraphicsInputLayoutOverride inputLayout;
  StateFieldGraphicsRenderState renderState;
  StateFieldGraphicsConditionalRenderingState conditionalRenderingState;
  StateFieldGraphicsStencilRefOverride stencilRefOverride;
  StateFieldGraphicsVertexBuffers vertexBuffers;
  StateFieldGraphicsIndexBuffer indexBuffers;
  StateFieldStageResourceBinds<STAGE_PS> bindsPS;
  StateFieldStageResourceBinds<STAGE_VS> bindsVS;
  StateFieldGraphicsViewport viewport;
  StateFieldGraphicsQueryState queryState;
  FrontRenderPassState renderpass;
  FrontFramebufferState framebuffer;

  void reset() {}
  void dumpLog() const { debug("FrontGraphicsStateStorage end"); }

  VULKAN_TRACKED_STATE_STORAGE_CB_DEFENITIONS();
};

struct BackGraphicsStateStorage
{
  StateFieldGraphicsPipelineLayout layout;
  StateFieldGraphicsRenderPassClass rpClass;
  StateFieldGraphicsBasePipeline basePipeline;
  StateFieldGraphicsPipeline pipeline;
  StateFieldGraphicsFramebuffer framebuffer;
  StateFieldGraphicsInPass inPass;
  StateFieldGraphicsDepthBounds depthBounds;
  StateFieldGraphicsDynamicRenderStateIndex dynamicRenderStateIndex;
  StateFieldGraphicsBlendConstantFactor blendConstantFactor;
  StateFieldGraphicsIndexBuffer indexBuffer;
  StateFieldGraphicsViewport viewport;
  StateFieldGraphicsVertexBuffersBindArray vertexBuffersArray;
  StateFieldGraphicsPrimitiveTopology primTopo;
  StateFieldGraphicsFlush flush;
  StateFieldGraphicsConditionalRenderingScopeOpener conditionalRenderingScopeOpener;
  StateFieldGraphicsRenderPassScopeOpener renderPassOpener;
  StateFieldGraphicsRenderPassEarlyScopeOpener renderPassEarlyOpener;
  StateFieldRenderPassResource nativeRenderPass;
  StateFieldGraphicsQueryScopeOpener queryOpener;

  BackDynamicGraphicsState dynamic;

  void reset() {}
  void dumpLog() const { debug("BackGraphicsStateStorage end"); }

  VULKAN_TRACKED_STATE_STORAGE_CB_DEFENITIONS();
};

class FrontGraphicsState
  : public TrackedState<FrontGraphicsStateStorage, FrontRenderPassState, FrontFramebufferState, StateFieldGraphicsViewport,
      StateFieldGraphicsProgram, StateFieldGraphicsPolygonLine, StateFieldGraphicsVertexBufferStrides,
      StateFieldGraphicsBlendConstantFactor, StateFieldGraphicsDepthBounds, StateFieldGraphicsScissorRect,
      StateFieldGraphicsInputLayoutOverride, StateFieldGraphicsRenderState, StateFieldGraphicsConditionalRenderingState,
      StateFieldGraphicsQueryState, StateFieldGraphicsStencilRefOverride, StateFieldGraphicsVertexBuffers,
      StateFieldGraphicsIndexBuffer, StateFieldStageResourceBinds<STAGE_PS>, StateFieldStageResourceBinds<STAGE_VS>>
{
public:
  VULKAN_TRACKED_STATE_DEFAULT_NESTED_FIELD_CB();
};

class BackGraphicsState
  : public TrackedState<BackGraphicsStateStorage, StateFieldGraphicsRenderPassEarlyScopeOpener, StateFieldRenderPassResource,
      StateFieldGraphicsInPass, StateFieldGraphicsViewport, StateFieldGraphicsPipelineLayout, StateFieldGraphicsBasePipeline,
      StateFieldGraphicsFlush, StateFieldGraphicsQueryScopeOpener, StateFieldGraphicsRenderPassScopeOpener,
      StateFieldGraphicsFramebuffer, StateFieldGraphicsRenderPassClass, StateFieldGraphicsConditionalRenderingScopeOpener,

      StateFieldGraphicsPrimitiveTopology, StateFieldGraphicsDynamicRenderStateIndex, BackDynamicGraphicsState,
      StateFieldGraphicsPipeline, StateFieldGraphicsDepthBounds, StateFieldGraphicsBlendConstantFactor, StateFieldGraphicsIndexBuffer,
      StateFieldGraphicsVertexBuffersBindArray>
{
public:
  template <typename T>
  void reset(T &);

  GraphicsPipelineStateDescription pipelineState; //-V730_NOINIT
  FramebufferState framebufferState;
  RenderPassResource::BakedAttachmentSharedData nativeRenderPassAttachments;

  BackGraphicsState &getValue() { return *this; }

  VULKAN_TRACKED_STATE_DEFAULT_NESTED_FIELD_CB_NO_RESET();
};

} // namespace drv3d_vulkan
