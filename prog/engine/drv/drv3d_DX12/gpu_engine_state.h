// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "driver.h"
#include "pipeline.h"
#include "tagged_handles.h"
#include "frame_buffer.h"

#include <drv/3d/rayTrace/dag_drvRayTrace.h>
#include <EASTL/bitset.h>
#include <generic/dag_enumerate.h>


namespace drv3d_dx12
{
class Image;

enum class ActivePipeline
{
  Graphics,
  Compute,
  Raytrace
};

struct GraphicsState
{
  enum
  {
    SCISSOR_RECT_DIRTY,
    SCISSOR_ENABLED,
    SCISSOR_ENABLED_DIRTY,
    VIEWPORT_DIRTY,
    USE_WIREFRAME,

    INDEX_BUFFER_DIRTY,
    VERTEX_BUFFER_0_DIRTY,
    VERTEX_BUFFER_1_DIRTY,
    VERTEX_BUFFER_2_DIRTY,
    VERTEX_BUFFER_3_DIRTY,
    // state
    INDEX_BUFFER_STATE_DIRTY,
    VERTEX_BUFFER_STATE_0_DIRTY,
    VERTEX_BUFFER_STATE_1_DIRTY,
    VERTEX_BUFFER_STATE_2_DIRTY,
    VERTEX_BUFFER_STATE_3_DIRTY,

    PREDICATION_BUFFER_STATE_DIRTY,

    COUNT
  };

  BasePipeline *basePipeline = nullptr;
  PipelineVariant *pipeline = nullptr;
  D3D12_PRIMITIVE_TOPOLOGY_TYPE topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;

  InputLayoutID inputLayoutIdent = InputLayoutID::Null();
  StaticRenderStateID staticRenderStateIdent = StaticRenderStateID::Null();
  FramebufferLayoutID framebufferLayoutID = FramebufferLayoutID::Null();
  FramebufferState framebufferState;
  D3D12_RECT scissorRects[Viewport::MAX_VIEWPORT_COUNT] = {};
  uint32_t scissorRectCount = 0;
  D3D12_VIEWPORT viewports[Viewport::MAX_VIEWPORT_COUNT] = {};
  uint32_t viewportCount = 0;
  eastl::bitset<COUNT> statusBits{0};

  BufferResourceReferenceAndAddressRange indexBuffer;
  DXGI_FORMAT indexBufferFormat = DXGI_FORMAT_R16_UINT;
  BufferResourceReferenceAndAddressRange vertexBuffers[MAX_VERTEX_INPUT_STREAMS];
  uint32_t vertexBufferStrides[MAX_VERTEX_INPUT_STREAMS] = {};
  BufferResourceReferenceAndOffset predicationBuffer;
  BufferResourceReferenceAndOffset activePredicationBuffer;

  void invalidateResourceStates()
  {
    statusBits.set(INDEX_BUFFER_STATE_DIRTY);
    statusBits.set(VERTEX_BUFFER_STATE_0_DIRTY);
    statusBits.set(VERTEX_BUFFER_STATE_1_DIRTY);
    statusBits.set(VERTEX_BUFFER_STATE_2_DIRTY);
    statusBits.set(VERTEX_BUFFER_STATE_3_DIRTY);
    statusBits.set(PREDICATION_BUFFER_STATE_DIRTY);
    framebufferState.dirtyAllTexturesState();
  }

  void dirtyBufferState(BufferGlobalId ident)
  {
    if (indexBuffer.resourceId == ident)
    {
      statusBits.set(INDEX_BUFFER_STATE_DIRTY);
    }
    for (auto [i, vertexBuffer] : enumerate(vertexBuffers, VERTEX_BUFFER_STATE_0_DIRTY))
    {
      if (vertexBuffer.resourceId == ident)
      {
        statusBits.set(i);
      }
    }
    if (predicationBuffer.resourceId == ident)
    {
      statusBits.set(PREDICATION_BUFFER_STATE_DIRTY);
    }
  }

  void dirtyTextureState(Image *texture) { framebufferState.dirtyTextureState(texture); }

  void onFlush()
  {
    G_STATIC_ASSERT(4 == MAX_VERTEX_INPUT_STREAMS);
    framebufferLayoutID = FramebufferLayoutID::Null();
    statusBits.set(INDEX_BUFFER_DIRTY);
    statusBits.set(INDEX_BUFFER_STATE_DIRTY);
    for (uint32_t i = 0; i < MAX_VERTEX_INPUT_STREAMS; ++i)
    {
      statusBits.set(VERTEX_BUFFER_0_DIRTY + i);
      statusBits.set(VERTEX_BUFFER_STATE_0_DIRTY + i);
    }
    statusBits.set(PREDICATION_BUFFER_STATE_DIRTY);
    activePredicationBuffer = {};

    framebufferState.dirtyAllTexturesState();
  }

  void onFrameStateInvalidate(D3D12_CPU_DESCRIPTOR_HANDLE null_ct)
  {
    pipeline = nullptr;

    statusBits.set(GraphicsState::VIEWPORT_DIRTY);
    statusBits.set(GraphicsState::SCISSOR_RECT_DIRTY);

    framebufferState.clear(null_ct);

    basePipeline = nullptr;
    framebufferLayoutID = FramebufferLayoutID::Null();

    indexBuffer = {};
    statusBits.set(INDEX_BUFFER_DIRTY);
    statusBits.set(INDEX_BUFFER_STATE_DIRTY);

    for (uint32_t i = 0; i < MAX_VERTEX_INPUT_STREAMS; ++i)
    {
      statusBits.set(VERTEX_BUFFER_0_DIRTY + i);
      statusBits.set(VERTEX_BUFFER_STATE_0_DIRTY + i);
      vertexBuffers[i] = {};
    }

    predicationBuffer = {};
    activePredicationBuffer = {};
    statusBits.set(PREDICATION_BUFFER_STATE_DIRTY);

    framebufferState.dirtyAllTexturesState();
  }
};

struct ComputeState
{
  ComputePipeline *pipeline = nullptr;

  void onFlush() {}
  void onFrameStateInvalidate() { pipeline = nullptr; }
};

#if D3D_HAS_RAY_TRACING
struct RaytraceState
{
  RaytracePipeline *pipeline = nullptr;

  void onFlush() {}

  void onFrameStateInvalidate() { pipeline = nullptr; }
};
#endif

} // namespace drv3d_dx12
