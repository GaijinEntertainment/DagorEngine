// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "const_register_type.h"
#include "device.h"
#include "device_context.h"
#include "resource_manager/raytrace_acceleration_structure.h"
#include "shader.h"
#include "texture.h"
#include "viewport_state.h"

#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <generic/dag_bitset.h>
#include <generic/dag_enumerate.h>
#include <generic/dag_variantArray.h>
#include <math/dag_adjpow2.h>
#include <math/dag_lsbVisitor.h>
#include <perfMon/dag_graphStat.h>


namespace drv3d_dx12
{
struct FrontendState
{
  struct Vacant
  {
    friend bool operator==(const Vacant &, const Vacant &) { return true; }
    friend bool operator!=(const Vacant &, const Vacant &) { return false; }
  };
  using TRegisterStates = dag::VariantArray<dxil::MAX_T_REGISTERS, Vacant, BaseTex *, GenericBufferInterface *
#if D3D_HAS_RAY_TRACING
    ,
    RaytraceAccelerationStructure *
#endif
    >;
  struct URegisterTexture
  {
    BaseTex *tex;
    ImageViewState view;

    friend bool operator==(const URegisterTexture &lhs, const URegisterTexture &rhs)
    {
      return lhs.tex == rhs.tex && lhs.view == rhs.view;
    }
    friend bool operator!=(const URegisterTexture &lhs, const URegisterTexture &rhs) { return !(lhs == rhs); }
  };
  using URegisterStates = dag::VariantArray<dxil::MAX_U_REGISTERS, Vacant, URegisterTexture, GenericBufferInterface *>;

  struct StageResourcesState
  {
    Sbuffer *bRegisterBuffers[dxil::MAX_B_REGISTERS] = {};
    uint32_t bRegisterOffsets[dxil::MAX_B_REGISTERS] = {};
    uint32_t bRegisterSizes[dxil::MAX_B_REGISTERS] = {};

    TRegisterStates tRegisterResources;

    d3d::SamplerHandle sRegisterSamplers[dxil::MAX_S_REGISTERS] = {};

    URegisterStates uRegisterResources;

    ShaderStageResouceUsageMask dirtyMasks{~0ul};

    void markDirtyB(uint32_t index, bool set) { or_bit(dirtyMasks.bRegisterMask, index, set); }
    void markDirtyT(uint32_t index, bool set) { or_bit(dirtyMasks.tRegisterMask, index, set); }
    void markDirtyS(uint32_t index, bool set) { or_bit(dirtyMasks.sRegisterMask, index, set); }
    void markDirtyU(uint32_t index, bool set) { or_bit(dirtyMasks.uRegisterMask, index, set); }
    void allDirty() { dirtyMasks = ShaderStageResouceUsageMask{~0ul}; }
  };
  struct DirtyState
  {
    enum Bits
    {
      COMPUTE_PROGRAM,
      GRAPHICS_PROGRAM,

      GRAPHICS_STATIC_STATE,
      GRAPHICS_STENCIL_REFERENCE,
      GRAPHICS_SCISSOR_TEST_ENABLE,

      DEPTH_BOUNDS_RANGE,
#if !_TARGET_XBOXONE
      VARIABLE_RATE_SHADING,
      VARIABLE_RATE_SHADING_TEXTURE,
#endif

      BLEND_CONSTANT,

      VIEWPORT,
      VIEWPORT_FROM_RENDER_TARGETS,

      POLYGON_LINE_ENABLED,

      SCISSOR_RECT,

      INPUT_LAYOUT,

      // has to be the last to validate against invalid stages
      COMPUTE_CONST_REGISTERS,
      PIXEL_CONST_REGISTERS,
      VERTEX_CONST_REGISTERS,
      // no raytrace const registers, intentionally not supported (yet?)

      COUNT,
      INVALID = COUNT
    };
    using Type = eastl::bitset<COUNT>;
  };
  struct ResourceDirtyState
  {
    enum Bits
    {
      FRAMEBUFFER,

      INDEX_BUFFER,

      VERTEX_BUFFER_0,
      VERTEX_BUFFER_1,
      VERTEX_BUFFER_2,
      VERTEX_BUFFER_3,

      COUNT,
      INVALID = COUNT
    };
    using Type = eastl::bitset<COUNT>;
  };
  struct ToggleState
  {
    enum Bits
    {
      BLEND,
      SEPARATE_BLEND,

      POLYGON_LINE,

      MRT_CLEAR,
      CLEAR_COLOR,
      CLEAR_DEPTH,
      CLEAR_STENCIL,

      USE_BACKBUFFER_SRGB,
      FORCE_SET_BACKBUFFER,

      COUNT,
      INVALID = COUNT
    };
    using Type = eastl::bitset<COUNT>;
  };

  // returns set
  bool markDirty(DirtyState::Bits bit, bool set)
  {
    DirtyState::Type n;
    n.set(bit, set);
    dirtyState |= n;
    return set;
  }

  bool markResourceDirty(ResourceDirtyState::Bits bit, bool set)
  {
    ResourceDirtyState::Type n;
    n.set(bit, set);
    resourceDirtyState |= n;
    return set;
  }

  static Driver3dRenderTarget default_render_target_state()
  {
    Driver3dRenderTarget result;
    result.setBackbufColor();
    result.removeDepth();
    return result;
  }

  E3DCOLOR clearColors[Driver3dRenderTarget::MAX_SIMRT] = {};
  float clearDepth = 0.f;
  uint8_t clearStencil = 0;
  uint8_t currentMrtClearTarget = 0;
  uint8_t nextMrtClearTarget = 0;

  Driver3dRenderTarget renderTargets = default_render_target_state();
  Driver3dRenderTarget activeRenderTargets = default_render_target_state();

  GraphicsProgramID graphicsProgram = GraphicsProgramID::Null();
  InputLayoutID externalInputLayout = InputLayoutID::Null();

  Sbuffer *vertexBuffers[MAX_VERTEX_INPUT_STREAMS] = {};
  uint32_t vertexBufferOffsets[MAX_VERTEX_INPUT_STREAMS] = {};
  uint32_t vertexStries[MAX_VERTEX_INPUT_STREAMS] = {};

  Ibuffer *indexBuffer = nullptr;

  ViewportState viewports[Viewport::MAX_VIEWPORT_COUNT] = {};
  uint32_t viewportCount = 0;
  D3D12_RECT scissorRects[Viewport::MAX_VIEWPORT_COUNT] = {};
  uint32_t scissorCount = 0;

  E3DCOLOR blendConstant = E3DCOLOR(0);

  float depthBoundsFrom = 0.f;
  float depthBoundsTo = 1.f;

  StaticRenderStateID graphicsStaticState = StaticRenderStateID::Null();
  RenderStateSystem::DynamicState graphicsDynamicState = {};

  DirtyState::Type dirtyState = ~DirtyState::Type(0);
  ResourceDirtyState::Type resourceDirtyState = ~ResourceDirtyState::Type{0};

  // some state is toggled on by default, all those bits are put here
  // for better readability
  static constexpr uint32_t toggle_init_mask = (1u << ToggleState::FORCE_SET_BACKBUFFER);
  ToggleState::Type toggleBits = ToggleState::Type(toggle_init_mask);

  ProgramID computeProgram = ProgramID::Null();
  // current sizes of the register space sections (eg what needs uploading)
  uint32_t registerSpaceSizes[STAGE_MAX] = {MIN_COMPUTE_CONST_REGISTERS, PIXEL_SHADER_REGISTERS, VERTEX_SHADER_MIN_REGISTERS};
  // one big chunk to provide memory for register backing
  ConstRegisterType totalRegisterSpace[MAX_COMPUTE_CONST_REGISTERS + VERTEX_SHADER_MAX_REGISTERS + PIXEL_SHADER_REGISTERS] = {};

  StageResourcesState stageResources[STAGE_MAX_EXT];
#if !_TARGET_XBOXONE
  BaseTexture *shadingRateTexture = nullptr;
  D3D12_SHADING_RATE constantShadingRate = D3D12_SHADING_RATE_1X1;
  D3D12_SHADING_RATE_COMBINER vertexShadingRateCombiner = D3D12_SHADING_RATE_COMBINER_PASSTHROUGH;
  D3D12_SHADING_RATE_COMBINER pixelShadingRateCombiner = D3D12_SHADING_RATE_COMBINER_PASSTHROUGH;
#endif

  mutable OSSpinlock resourceBindingGuard;

  static void updateRenderTargetBindingStates(const Driver3dRenderTarget &rts, bool in_use)
  {
    for (auto i : LsbVisitor{rts.used & Driver3dRenderTarget::COLOR_MASK})
    {
      if (auto tex = cast_to_texture_base(rts.getColor(i).tex))
      {
        tex->setRtvBinding(i, in_use);
      }
    }
    if (rts.isDepthUsed())
    {
      auto tex = cast_to_texture_base(rts.getDepth().tex);
      G_ASSERTF(tex, "Depth is used, but depth dexture is set to null");
      tex->setDsvBinding(in_use);
    }
  }

  void preRecovery()
  {
    OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};
    dirtyState = ~DirtyState::Type(0);
    resourceDirtyState = ~ResourceDirtyState::Type{0};
    for (auto &stage : stageResources)
      stage.allDirty();
  }

  eastl::span<ConstRegisterType> getRegisterSection(uint32_t stage)
  {
    // of those break, assumptions about which bits indicate what are incorrect
    G_STATIC_ASSERT(STAGE_CS == 0);
    G_STATIC_ASSERT(STAGE_PS == 1);
    G_STATIC_ASSERT(STAGE_VS == 2);
    // if those break, all the shift optimizations need to be reworked
    G_STATIC_ASSERT(MAX_COMPUTE_CONST_REGISTERS == 4096);
    G_STATIC_ASSERT(VERTEX_SHADER_MAX_REGISTERS == 4096);
    // segmentation [0-4095:cs][4096-8191:vs][8192-8263:ps]
    uint32_t offset = ((stage & 2) << 11)    // turns 2 into 4096
                      + ((stage & 1) << 13); // turns 1 into 8192

    decltype(eastl::dynamic_extent) size = (((stage + 1) & 1) << 12) // maps 0 (CS) and 2 (VS) to 4096
                                           | ((-static_cast<int32_t>(stage & 1)) & PIXEL_SHADER_REGISTERS); // maps 1 (PS) to
                                                                                                            // PIXEL_SHADER_REGISTERS
    return {&totalRegisterSpace[offset], size};
  }

  bool registerMemoryUpdate(uint32_t stage, uint32_t offset, eastl::span<const ConstRegisterType> src)
  {
    auto dstBase = getRegisterSection(stage);
    G_ASSERTF_RETURN(offset < dstBase.size(), false, "const register offset out of range %u is larger than %u", offset,
      dstBase.size());
    auto dst = dstBase.last(dstBase.size() - offset);
    G_ASSERTF_RETURN(src.size() <= dst.size(), false, "const register update overflow, %u > %u", src.size(), dst.size());
    // search for first difference
    auto range = eastl::mismatch(src.begin(), src.end(), dst.begin());
    return range.second != eastl::copy(range.first, src.end(), range.second);
  }

  template <typename A, typename U, typename D>
  void flushResources(A &ctx, uint32_t stage, U update_srv_buffer, D is_const_ds) DAG_TS_REQUIRES(resourceBindingGuard)
  {
    StageResourcesState &target = stageResources[stage];
    const auto updateMask = target.dirtyMasks;
    target.dirtyMasks = {};

    for (auto i : updateMask.bRegisterMask)
    {
      if (auto gbuf = (GenericBufferInterface *)target.bRegisterBuffers[i])
      {
        gbuf->updateDeviceBuffer([](auto &buf) { buf.resourceId.markUsedAsConstOrVertexBuffer(); });
        ConstBufferSetupInformationStream info;
        info.buffer = {get_any_buffer_ref(gbuf), target.bRegisterOffsets[i], target.bRegisterSizes[i]};
#if DX12_VALIDATE_STREAM_CB_USAGE_WITHOUT_INITIALIZATION
        info.lastDiscardFrameIdx = gbuf->getDiscardFrame();
        info.isStreamBuffer = gbuf->isStreamBuffer();
#endif
        ctx.setConstBuffer(stage, i, info, gbuf->getResName());
      }
      else
      {
        ctx.setConstBuffer(stage, i, {}, "");
      }
    }

    // have to check all slots regardless of mask, a state of a texture might has changed
    for (auto i : updateMask.tRegisterMask)
    {
      if (auto maybeBuf = target.tRegisterResources.getIf<GenericBufferInterface *>(i))
      {
        auto gbuf = *maybeBuf;
        gbuf->updateDeviceBuffer(update_srv_buffer);
        G_ASSERT(!gbuf->isStreamBuffer());
        ctx.setSRVBuffer(stage, i, gbuf->getDeviceBuffer());
      }
      else if (auto maybeTex = target.tRegisterResources.getIf<BaseTex *>(i))
      {
        auto texture = *maybeTex;
        // if it is right now a stub texture then use it explicitly to avoid a race
        if (texture->isStub())
        {
          texture = texture->getStubTex();
        }

        ImageViewState view = texture->getViewInfo();
        ctx.setSRVTexture(stage, i, texture, view, is_const_ds(texture, view));
      }
#if D3D_HAS_RAY_TRACING
      else if (auto maybeAs = target.tRegisterResources.getIf<RaytraceAccelerationStructure *>(i))
      {
        ctx.setRaytraceAccelerationStructure(stage, i, *maybeAs);
      }
#endif
      else
      {
        ctx.setSRVNull(stage, i);
      }
    }

    // have to check all slots regardless of mask, a state of a texture might has changed
    for (auto i : updateMask.sRegisterMask)
    {
      if (auto sampler = target.sRegisterSamplers[i]; sampler != d3d::INVALID_SAMPLER_HANDLE)
      {
        ctx.setSamplerHandle(stage, i, sampler);
      }
      else if (auto maybeTexture = target.tRegisterResources.getIf<BaseTex *>(i))
      {
        ctx.setSampler(stage, i, (*maybeTexture)->getDeviceSampler());
      }
      else
      {
        ctx.setSampler(stage, i, {});
      }
    }

    for (auto i : updateMask.uRegisterMask)
    {
      if (auto maybeBuf = target.uRegisterResources.getIf<GenericBufferInterface *>(i))
      {
        auto gbuf = *maybeBuf;
        gbuf->updateDeviceBuffer([](auto &buf) { buf.resourceId.markUsedAsUAVBuffer(); });
        G_ASSERT(!gbuf->isStreamBuffer());
        ctx.setUAVBuffer(stage, i, gbuf->getDeviceBuffer());
      }
      else if (auto maybeTex = target.uRegisterResources.getIf<URegisterTexture>(i))
      {
        ctx.setUAVTexture(stage, i, maybeTex->tex->getDeviceImage(), maybeTex->view);
      }
      else
      {
        ctx.setUAVNull(stage, i);
      }
    }
  }

  bool setBackbufferSrgb(bool value)
  {
    OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};
    bool old = toggleBits.test(ToggleState::USE_BACKBUFFER_SRGB);
    if (old != value)
    {
      toggleBits.set(ToggleState::USE_BACKBUFFER_SRGB, value);
      if (renderTargets.isBackBufferColor())
      {
        resourceDirtyState.set(ResourceDirtyState::FRAMEBUFFER);
        toggleBits.set(ToggleState::FORCE_SET_BACKBUFFER);
      }
    }
    return old;
  }

  template <typename T>
  void getRenderTargets(T clb) const
  {
    OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};
    // ensure that the input is of const ref and disallows modifications
    clb(const_cast<const Driver3dRenderTarget &>(renderTargets));
  }

  Extent2D getFramebufferExtent(DeviceContext &ctx) const
  {
    OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};
    return getFramebufferExtentInternal(ctx);
  }

  bool isConstDepthStencilTarget(BaseTex *texture, ImageViewState view) const
  {
    if (renderTargets.depth.tex != texture)
      return false;
    if (!renderTargets.isDepthUsed() || !renderTargets.isDepthReadOnly())
      return false;
    if (0 == (TEXCF_RTARGET & texture->cflg))
      return false;

    return view.getMipRange().isInside(MipMapIndex::make(renderTargets.depth.level)) &&
           view.getArrayRange().isInside(ArrayLayerIndex::make(renderTargets.depth.layer));
  }

  BaseTexture *getColorTarget(uint32_t index) const
  {
    OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};
    return renderTargets.color[index].tex;
  }

  void setRenderTargets(const Driver3dRenderTarget &rt)
  {
    OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};
    resourceDirtyState.set(ResourceDirtyState::FRAMEBUFFER, true);
    updateRenderTargetBindingStates(renderTargets, false);
    updateRenderTargetBindingStates(rt, true);
    renderTargets = rt;
    for (auto i : LsbVisitor{renderTargets.used & Driver3dRenderTarget::COLOR_MASK})
    {
      if (auto tex = cast_to_texture_base(renderTargets.getColor(i).tex))
      {
        tex->dirtyBoundUavsNoLock();
        tex->dirtyBoundSrvsNoLock();
      }
    }
  }

  void setColorTarget(uint32_t index, BaseTex *texture, uint32_t mip_level, uint32_t face_index)
  {
    OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};
    resourceDirtyState.set(ResourceDirtyState::FRAMEBUFFER, true);
    if (renderTargets.isColorUsed(index))
    {
      auto ot = cast_to_texture_base(renderTargets.getColor(index).tex);
      if (ot)
      {
        ot->setRtvBinding(index, false);
      }
    }
    if (texture)
    {
      texture->setRtvBinding(index, true);
      texture->dirtyBoundSrvsNoLock();
      texture->dirtyBoundUavsNoLock();
    }
    if (toggleBits.test(ToggleState::MRT_CLEAR))
    {
      renderTargets.setColor(nextMrtClearTarget, texture, mip_level, face_index);
      currentMrtClearTarget = nextMrtClearTarget++;
    }
    else
    {
      renderTargets.setColor(index, texture, mip_level, face_index);
    }
  }
  void removeColorTarget(uint32_t index)
  {
    OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};
    resourceDirtyState.set(ResourceDirtyState::FRAMEBUFFER, true);
    if (renderTargets.isColorUsed(index))
    {
      auto ot = cast_to_texture_base(renderTargets.getColor(index).tex);
      if (ot)
      {
        ot->setRtvBinding(index, false);
      }
    }
    renderTargets.removeColor(index);
  }
  void resetColorTargetsToBackBuffer()
  {
    OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};
    resourceDirtyState.set(ResourceDirtyState::FRAMEBUFFER, true);
    for (auto i : LsbVisitor{renderTargets.used & Driver3dRenderTarget::COLOR_MASK})
    {
      if (auto tex = cast_to_texture_base(renderTargets.getColor(i).tex))
      {
        tex->setRtvBinding(i, false);
      }
    }
    renderTargets.setBackbufColor();
  }
  void setDepthStencilTarget(BaseTex *texture, uint32_t face_index, bool read_only)
  {
    OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};
    resourceDirtyState.set(ResourceDirtyState::FRAMEBUFFER, true);
    if (renderTargets.isDepthUsed())
    {
      auto ot = cast_to_texture_base(renderTargets.getDepth().tex);
      G_ASSERTF(ot, "Depth is used in RT state, but depth texture is null");
      ot->setDsvBinding(false);
    }
    texture->setDsvBinding(true);
    renderTargets.setDepth(texture, face_index, read_only);
  }
  void removeDepthStencilTarget()
  {
    OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};
    resourceDirtyState.set(ResourceDirtyState::FRAMEBUFFER, true);
    if (renderTargets.isDepthUsed())
    {
      auto ot = cast_to_texture_base(renderTargets.getDepth().tex);
      G_ASSERTF(ot, "Depth is used in RT state, but depth texture is null");
      ot->setDsvBinding(false);
    }
    renderTargets.removeDepth();
  }

  void setVertexBuffer(uint32_t stream, Sbuffer *buffer, uint32_t offset, uint32_t stride)
  {
    G_STATIC_ASSERT(MAX_VERTEX_INPUT_STREAMS <= 4);
    G_ASSERT(stream < MAX_VERTEX_INPUT_STREAMS);
    OSSpinlockScopedLock resourceBindingLock(resourceBindingGuard);
    markResourceDirty(ResourceDirtyState::Bits(ResourceDirtyState::VERTEX_BUFFER_0 + stream),
      (vertexBuffers[stream] != buffer) || (vertexBufferOffsets[stream] != offset) || (vertexStries[stream] != stride));
    vertexBuffers[stream] = buffer;
    vertexBufferOffsets[stream] = offset;
    vertexStries[stream] = stride;
  }

  void setGraphicsProgram(GraphicsProgramUsageInfo program_info)
  {
    markDirty(DirtyState::GRAPHICS_PROGRAM, graphicsProgram != program_info.programId);
    graphicsProgram = program_info.programId;

    markDirty(DirtyState::INPUT_LAYOUT, program_info.input != externalInputLayout);
    externalInputLayout = program_info.input;
  }

  void setInputLayout(InputLayoutID layout)
  {
    markDirty(DirtyState::INPUT_LAYOUT, externalInputLayout != layout);
    externalInputLayout = layout;
  }

  void setStageBRegisterBuffer(uint32_t stage, uint32_t index, Sbuffer *buffer, uint32_t offset, uint32_t size)
  {
    G_ASSERT(stage < countof(stageResources));
    StageResourcesState &target = stageResources[stage];
    G_ASSERT(index < countof(target.bRegisterBuffers));
    OSSpinlockScopedLock resourceBindingLock(resourceBindingGuard);
    target.markDirtyB(index,
      target.bRegisterBuffers[index] != buffer || target.bRegisterOffsets[index] != offset || target.bRegisterSizes[index] != size);
    target.bRegisterBuffers[index] = buffer;
    target.bRegisterOffsets[index] = offset;
    target.bRegisterSizes[index] = size;
  }

  void setStageTRegisterBuffer(uint32_t stage, uint32_t index, GenericBufferInterface *buffer)
  {
    G_ASSERT(stage < countof(stageResources));
    StageResourcesState &target = stageResources[stage];

    OSSpinlockScopedLock resourceBindingLock(resourceBindingGuard);

    if (auto maybeTex = target.tRegisterResources.getIf<BaseTex *>(index))
      (*maybeTex)->setSrvBinding(stage, index, false);

    target.markDirtyT(index, target.tRegisterResources[index] != buffer);

    if (buffer)
      target.tRegisterResources.emplace<GenericBufferInterface *>(index, buffer);
    else
      target.tRegisterResources.emplace<Vacant>(index);
  }

  void setStageURegisterBuffer(uint32_t stage, uint32_t index, GenericBufferInterface *buffer)
  {
    G_ASSERT(stage < countof(stageResources));
    StageResourcesState &target = stageResources[stage];

    GenericBufferInterface *prevBuf = nullptr;
    {
      OSSpinlockScopedLock resourceBindingLock(resourceBindingGuard);

      if (auto maybeTex = target.uRegisterResources.getIf<URegisterTexture>(index))
        maybeTex->tex->setUavBinding(stage, index, false);
      else if (auto maybeBuf = target.uRegisterResources.getIf<GenericBufferInterface *>(index))
        prevBuf = *maybeBuf;

      target.markDirtyU(index, target.uRegisterResources[index] != buffer);

      if (buffer)
        target.uRegisterResources.emplace<GenericBufferInterface *>(index, buffer);
      else
        target.uRegisterResources.emplace<Vacant>(index);
    }
    if (buffer != prevBuf && prevBuf)
    {
      auto flags = prevBuf->getFlags();
      markBufferStagesDirty(prevBuf, SBCF_BIND_VERTEX & flags, SBCF_BIND_CONSTANT & flags, SBCF_BIND_SHADER_RES & flags,
        SBCF_BIND_UNORDERED & flags);
    }
  }

  void setStageSRVTexture(uint32_t stage, uint32_t index, BaseTex *texture, bool use_sampler)
  {
    G_ASSERT(stage < countof(stageResources));
    StageResourcesState &target = stageResources[stage];

    OSSpinlockScopedLock resourceBindingLock(resourceBindingGuard);

    if (texture)
    {
      texture->dirtyBoundUavsNoLock();
      texture->dirtyBoundRtvsNoLock();
    }

    if (auto maybeTex = target.tRegisterResources.getIf<BaseTex *>(index))
      (*maybeTex)->setSrvBinding(stage, index, false);

    const bool changed = target.tRegisterResources[index] != texture;
    target.markDirtyT(index, changed);
    if (use_sampler)
    {
      target.markDirtyS(index, changed);
      target.sRegisterSamplers[index] = {};
    }

    if (texture)
      target.tRegisterResources.emplace<BaseTex *>(index, texture);
    else
      target.tRegisterResources.emplace<Vacant>(index);

    if (texture)
      texture->setSrvBinding(stage, index, true);
  }

  void setStageSampler(uint32_t stage, uint32_t index, d3d::SamplerHandle handle)
  {
    G_ASSERT(stage < countof(stageResources));
    StageResourcesState &target = stageResources[stage];
    G_ASSERT(index < countof(target.sRegisterSamplers));
    OSSpinlockScopedLock resourceBindingLock(resourceBindingGuard);
    target.markDirtyS(index, target.sRegisterSamplers[index] != handle);
    target.sRegisterSamplers[index] = handle;
  }

  void setStageUAVTexture(uint32_t stage, uint32_t index, BaseTex *texture, ImageViewState view)
  {
    G_ASSERT(stage < countof(stageResources));
    StageResourcesState &target = stageResources[stage];

    OSSpinlockScopedLock resourceBindingLock(resourceBindingGuard);
    if (texture)
    {
      texture->dirtyBoundSrvsNoLock();
      texture->dirtyBoundRtvsNoLock();
    }

    if (auto maybeUTex = target.uRegisterResources.getIf<URegisterTexture>(index))
    {
      G_ASSERT_RETURN(maybeUTex->tex, );
      maybeUTex->tex->setUavBinding(stage, index, false);
    }

    target.markDirtyU(index, target.uRegisterResources[index] != URegisterTexture{texture, view});

    if (texture)
      target.uRegisterResources.emplace<URegisterTexture>(index, texture, view);
    else
      target.uRegisterResources.emplace<Vacant>(index);

    if (texture)
      texture->setUavBinding(stage, index, true);
  }

  void setIndexBuffer(Ibuffer *buffer)
  {
    OSSpinlockScopedLock resourceBindingLock(resourceBindingGuard);
    markResourceDirty(ResourceDirtyState::INDEX_BUFFER, indexBuffer != buffer);
    indexBuffer = buffer;
  }

  void setScissorRects(dag::ConstSpan<D3D12_RECT> rects)
  {
    if (rects.size() != scissorCount || !eastl::equal(rects.begin(), rects.end(), scissorRects))
    {
      markDirty(DirtyState::SCISSOR_RECT, true);
      eastl::copy_n(rects.begin(), rects.size(), scissorRects);
      scissorCount = rects.size();
    }
  }

  void setScissorTestEnable(bool enable)
  {
    markDirty(DirtyState::GRAPHICS_SCISSOR_TEST_ENABLE, (graphicsDynamicState.enableScissor != 0) != enable);
    graphicsDynamicState.enableScissor = enable;
  }

  void setPolygonLine(bool enable)
  {
    // need to take the lock to guard toggleBits (TODO may move out of bitset)
    OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};
    markDirty(DirtyState::POLYGON_LINE_ENABLED, toggleBits.test(ToggleState::POLYGON_LINE) != enable);
    toggleBits.set(ToggleState::POLYGON_LINE, enable);
  }

  Extent2D getFramebufferExtentInternal(DeviceContext &ctx) const
  {
    if (!renderTargets.isBackBufferColor())
    {
      if (renderTargets.isColorUsed())
      {
        auto &color = renderTargets.color[__ctz_unsafe(renderTargets.used & Driver3dRenderTarget::COLOR_MASK)];
        const Extent3D &size = cast_to_texture_base(*color.tex).getMipmapExtent(color.level);
        return {size.width, size.height};
      }
      if (renderTargets.isDepthUsed() && renderTargets.depth.tex)
      {
        const Extent3D &size = cast_to_texture_base(*renderTargets.depth.tex).getMipmapExtent(renderTargets.depth.level);
        return {size.width, size.height};
      }
    }
    return ctx.getSwapchainExtent();
  }

  void calculateViewport(DeviceContext &ctx)
  {
    if (!dirtyState.test(DirtyState::VIEWPORT_FROM_RENDER_TARGETS))
      return;
    dirtyState.reset(DirtyState::VIEWPORT_FROM_RENDER_TARGETS);

    auto extent = getFramebufferExtentInternal(ctx);
    if (auto &vp = viewports[0];
        vp.x != 0 || vp.y != 0 || vp.width != extent.width || vp.height != extent.height || viewportCount != 1)
    {
      vp.x = 0;
      vp.y = 0;
      vp.width = extent.width;
      vp.height = extent.height;
      viewportCount = 1;
      markDirty(DirtyState::VIEWPORT, true);
    }
  }

  template <typename T>
  void getViewport(DeviceContext &ctx, T clb)
  {
    OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};
    calculateViewport(ctx);
    clb(viewports[0]);
  }

  void setUpdateViewportFromRenderTarget() { markDirty(DirtyState::VIEWPORT_FROM_RENDER_TARGETS, true); }

  void setViewports(dag::ConstSpan<ViewportState> states)
  {
    if (states.size() != viewportCount || !eastl::equal(states.begin(), states.end(), viewports))
    {
      markDirty(DirtyState::VIEWPORT, true);
      eastl::copy_n(states.begin(), states.size(), viewports);
      viewportCount = states.size();
    }

    dirtyState.reset(DirtyState::VIEWPORT_FROM_RENDER_TARGETS);
  }

  void setBlendConstant(E3DCOLOR cnst)
  {
    markDirty(DirtyState::BLEND_CONSTANT, blendConstant != cnst);
    blendConstant = cnst;
  }

  void setDepthBoundsRange(float from, float to)
  {
    markDirty(DirtyState::DEPTH_BOUNDS_RANGE, (depthBoundsFrom != from) || (depthBoundsTo != to));
    depthBoundsFrom = from;
    depthBoundsTo = to;
  }

  void setStencilReference(uint8_t ref)
  {
    markDirty(DirtyState::GRAPHICS_STENCIL_REFERENCE, graphicsDynamicState.stencilRef != ref);
    graphicsDynamicState.stencilRef = ref;
  }

  void onFrameEnd(DeviceContext &ctx)
  {
    // have to lock the context mutex here or we have a ordering issue of locking multiple mutexes
    ScopedCommitLock ctxLock{ctx};
    OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};
    dirtyState = ~DirtyState::Type(0);
    resourceDirtyState = ~ResourceDirtyState::Type{0};
    toggleBits.set(ToggleState::FORCE_SET_BACKBUFFER);
    activeRenderTargets = default_render_target_state();
    renderTargets = default_render_target_state();
    for (auto &&res : stageResources)
      res.allDirty();
  }

  void setComputeProgram(ProgramID p)
  {
    markDirty(DirtyState::COMPUTE_PROGRAM, computeProgram != p);
    computeProgram = p;
  }

  uint32_t setComputeConstRegisterCount(uint32_t cnt)
  {
    if (cnt)
      cnt = clamp<uint32_t>(get_bigger_pow2(cnt), MIN_COMPUTE_CONST_REGISTERS, MAX_COMPUTE_CONST_REGISTERS);
    else
      cnt = MIN_COMPUTE_CONST_REGISTERS; // TODO update things to allow 0 (eg shader can tell how many it needs)
    markDirty(DirtyState::COMPUTE_CONST_REGISTERS, registerSpaceSizes[STAGE_CS] < cnt);
    registerSpaceSizes[STAGE_CS] = cnt;
    return cnt;
  }
  uint32_t setVertexConstRegisterCount(uint32_t cnt)
  {
    if (cnt)
      cnt = clamp<uint32_t>(get_bigger_pow2(cnt), VERTEX_SHADER_MIN_REGISTERS, VERTEX_SHADER_MAX_REGISTERS);
    else
      cnt = VERTEX_SHADER_MIN_REGISTERS; // TODO update things to allow 0 (eg shader can tell how many it needs)
    markDirty(DirtyState::VERTEX_CONST_REGISTERS, registerSpaceSizes[STAGE_VS] < cnt);
    registerSpaceSizes[STAGE_VS] = cnt;
    return cnt;
  }
  void setConstRegisters(int stage, uint32_t offset, eastl::span<const ConstRegisterType> blob)
  {
    auto ds = static_cast<DirtyState::Bits>(DirtyState::COMPUTE_CONST_REGISTERS + stage);
    G_ASSERT(ds < DirtyState::INVALID);
    markDirty(ds, registerMemoryUpdate(stage, offset, blob));
  }

  void handleRenderTargetUpdate(DeviceContext &ctx)
  {
    auto swapchainColor = ctx.getSwapchainColorTexture();
    if (!swapchainColor)
      return;

    ImageViewState newViews[Driver3dRenderTarget::MAX_SIMRT + 1] = {};
    Image *newImages[Driver3dRenderTarget::MAX_SIMRT + 1] = {};

    if (0 != (renderTargets.used & Driver3dRenderTarget::COLOR0))
    {
      ImageViewState &targetView = newViews[0];
      Image *&targetImage = newImages[0];
      if (renderTargets.color[0].tex)
      {
        BaseTex *base = cast_to_texture_base(renderTargets.color[0].tex);
        targetView = base->getViewInfoRenderTarget(MipMapIndex::make(renderTargets.color[0].level),
          ArrayLayerIndex::make(renderTargets.color[0].layer), false);
        targetImage = base->getDeviceImage();
      }
      else
      {
        bool useSrgb = toggleBits.test(ToggleState::USE_BACKBUFFER_SRGB);
        auto baseFormat = ctx.getSwapchainColorFormat();
        targetImage = nullptr;
        targetView.setFormat(useSrgb ? baseFormat.getSRGBVariant() : baseFormat.getLinearVariant());
        targetView.isArray = 0;
        targetView.isCubemap = 0;
        targetView.setSingleMipMapRange(MipMapIndex::make(0));
        targetView.setSingleArrayRange(ArrayLayerIndex::make(0));
        targetView.setRTV();
        targetImage = swapchainColor->getDeviceImage();

        auto swapchainSecondaryColor = ctx.getSwapchainSecondaryColorTexture();
        if (swapchainSecondaryColor && (renderTargets.used & Driver3dRenderTarget::COLOR_MASK) == Driver3dRenderTarget::COLOR0)
        {
          baseFormat = ctx.getSwapchainSecondaryColorFormat();
          newViews[1] = targetView;
          newViews[1].setFormat(useSrgb ? baseFormat.getSRGBVariant() : baseFormat.getLinearVariant());
          newImages[1] = swapchainSecondaryColor->getDeviceImage();
        }
      }
    }

    for (auto i : LsbVisitor{renderTargets.used & Driver3dRenderTarget::COLOR_MASK & ~1u})
    {
      ImageViewState &targetView = newViews[i];
      Image *&targetImage = newImages[i];

      BaseTex *base = cast_to_texture_base(renderTargets.color[i].tex);
      targetView = base->getViewInfoRenderTarget(MipMapIndex::make(renderTargets.color[i].level),
        ArrayLayerIndex::make(renderTargets.color[i].layer), false);
      targetImage = base->getDeviceImage();
    }

    if (renderTargets.isDepthUsed())
    {
      ImageViewState &targetView = newViews[Driver3dRenderTarget::MAX_SIMRT];
      Image *&targetImage = newImages[Driver3dRenderTarget::MAX_SIMRT];
      BaseTex *depthTex = cast_to_texture_base(renderTargets.depth.tex);
      G_ASSERTF(depthTex, "Depth is used in RT state, but depth texture is null");
      targetView = depthTex->getViewInfoRenderTarget(MipMapIndex::make(renderTargets.depth.level),
        ArrayLayerIndex::make(renderTargets.depth.layer), renderTargets.isDepthReadOnly());
      targetImage = depthTex->getDeviceImage();
    }
    ctx.setFramebuffer(newImages, newViews, renderTargets.isDepthReadOnly());
    activeRenderTargets = renderTargets;
  }

  void flushRenderTargets(DeviceContext &ctx)
  {
    calculateViewport(ctx);

    if (!resourceDirtyState.test(ResourceDirtyState::FRAMEBUFFER))
    {
      return;
    }

    const bool changedTargets = (renderTargets != activeRenderTargets) || toggleBits.test(ToggleState::FORCE_SET_BACKBUFFER);
    if (changedTargets)
    {
      handleRenderTargetUpdate(ctx);
      toggleBits.reset(ToggleState::FORCE_SET_BACKBUFFER);
      Stat3D::updateRenderTarget();
    }

    toggleBits.reset(ToggleState::CLEAR_COLOR);
    toggleBits.reset(ToggleState::CLEAR_DEPTH);
    toggleBits.reset(ToggleState::CLEAR_STENCIL);

    resourceDirtyState.reset(ResourceDirtyState::FRAMEBUFFER);
  }

  void validate_draw_call_produces_anything()
  {
#if DX12_VALIDATE_DRAW_CALL_USEFULNESS
    // here we see if a draw call does actually outputs something
    // we check if the pixel shader writes something and match
    // it to the provided render target
    // we also check uav uses
    auto outputMask = graphicsProgramioMask.usedOutputs;
    auto targetMask = activeRenderTargets.used & Driver3dRenderTarget::COLOR_MASK;
    auto writtenMask = outputMask & targetMask;
    if (!writtenMask)
    {
      // no writing to color, check if depth is available
      if (0 == (Driver3dRenderTarget::DEPTH & activeRenderTargets.used))
      {
        // no color and depth, see if uavs are touched by the ps or vs
        if (!stageResources[STAGE_PS].useMask.uRegisterMask.any() && !stageResources[STAGE_VS].useMask.uRegisterMask.any())
        {
          // no one is writing to it, complain!
          // This can be very spamy!
          D3D_ERROR("Draw call is not producing any output, ps output mask 0x%02X, render target "
                    "color slot mask 0x%02X, no depth target is set and no UAVs are accessed by any "
                    "shader",
            outputMask, targetMask);
        }
      }
    }
#endif
  }

  enum class GraphicsMode
  {
    DRAW,
    DRAW_INDEXED,
    DISPATCH_MESH,
    DRAW_OR_DRAW_INDEXED,
    DRAW_UP = DRAW,
    DRAW_INDEXED_UP = DRAW,
    ALL = DRAW_INDEXED
  };

  bool flushGraphics(Device &device, GraphicsMode mode)
  {
    if (graphicsProgram == GraphicsProgramID::Null())
      return false;
    auto &ctx = device.getContext();
    DirtyState::Type unchangedMask;
    ResourceDirtyState::Type unchangedResourceMask;
    // keep compute dirty flags
    // TODO: move those defs somewhere else
    unchangedMask.set(DirtyState::COMPUTE_CONST_REGISTERS);
    unchangedMask.set(DirtyState::COMPUTE_PROGRAM);

    if (dirtyState.test(DirtyState::GRAPHICS_PROGRAM))
    {
      ctx.setGraphicsPipeline(graphicsProgram);

      Stat3D::updateProgram();
    }

    OSSpinlockScopedLock resourceBindingLock(resourceBindingGuard);
    flushRenderTargets(ctx);

    if (dirtyState.test(DirtyState::VIEWPORT))
      ctx.updateViewports(make_span_const(viewports, viewportCount));

    if (dirtyState.test(DirtyState::DEPTH_BOUNDS_RANGE))
      ctx.setDepthBoundsRange(depthBoundsFrom, depthBoundsTo);

    if (dirtyState.test(DirtyState::GRAPHICS_STENCIL_REFERENCE))
      ctx.setStencilRef(graphicsDynamicState.stencilRef);

    if (dirtyState.test(DirtyState::BLEND_CONSTANT))
      ctx.setBlendConstant(blendConstant);

    if (dirtyState.test(DirtyState::POLYGON_LINE_ENABLED))
      ctx.setPolygonLine(toggleBits.test(ToggleState::POLYGON_LINE));

    if (dirtyState.test(DirtyState::GRAPHICS_STATIC_STATE))
      ctx.setStaticRenderState(graphicsStaticState);

    if (dirtyState.test(DirtyState::GRAPHICS_SCISSOR_TEST_ENABLE))
      ctx.setScissorEnable(graphicsDynamicState.enableScissor);

    if (graphicsDynamicState.enableScissor)
    {
      if (dirtyState.test(DirtyState::SCISSOR_RECT))
        ctx.setScissorRects(make_span_const(scissorRects, scissorCount));
    }
    else
    {
      unchangedMask.set(DirtyState::SCISSOR_RECT);
    }

    // DRAW_OR_DRAW_INDEXED is basically like DRAW_INDEXED. But it does not require an index buffer.
    if (resourceDirtyState.test(ResourceDirtyState::INDEX_BUFFER) &&
        ((GraphicsMode::DRAW_INDEXED == mode) || (mode == GraphicsMode::DRAW_OR_DRAW_INDEXED && indexBuffer != nullptr)))
    {
      G_ASSERTF(indexBuffer != nullptr, "flush with index buffer, but index buffer was null!");
      G_ANALYSIS_ASSUME(indexBuffer != nullptr);
      // can not be null, render commands would be invalid
      auto gbuf = (GenericBufferInterface *)indexBuffer;
      if (!gbuf)
      {
        return false;
      }
      gbuf->updateDeviceBuffer([](auto &buf) { buf.resourceId.markUsedAsIndexBuffer(); });
      ctx.setIndexBuffer(get_any_buffer_ref(gbuf), get_index_format(gbuf));
    }
    else
    {
      // make sure that possible change to index buffer is not lost
      unchangedResourceMask.set(ResourceDirtyState::INDEX_BUFFER);
    }

    flushResources(
      ctx, STAGE_VS, //
      [](auto &buf) { buf.resourceId.markUsedAsNonPixelShaderResource(); },
      [=](BaseTex *tex, ImageViewState view) { return isConstDepthStencilTarget(tex, view); });

    flushResources(
      ctx, STAGE_PS, //
      [](auto &buf) { buf.resourceId.markUsedAsPixelShaderResource(); },
      [=](BaseTex *tex, ImageViewState view) { return isConstDepthStencilTarget(tex, view); });

    if (dirtyState.test(DirtyState::VERTEX_CONST_REGISTERS))
    {
      if (!stageResources[STAGE_VS].bRegisterBuffers[0])
      {
        ctx.pushConstRegisterData(STAGE_VS, getRegisterSection(STAGE_VS).first(registerSpaceSizes[STAGE_VS]));
      }
      else
      {
        unchangedMask.set(DirtyState::VERTEX_CONST_REGISTERS);
      }
    }
    if (dirtyState.test(DirtyState::PIXEL_CONST_REGISTERS))
    {
      if (!stageResources[STAGE_PS].bRegisterBuffers[0])
      {
        ctx.pushConstRegisterData(STAGE_PS, getRegisterSection(STAGE_PS).first(registerSpaceSizes[STAGE_PS]));
      }
      else
      {
        // happens a lot, not sure why
        unchangedMask.set(DirtyState::PIXEL_CONST_REGISTERS);
      }
    }

    if ((GraphicsMode::DRAW == mode) || (GraphicsMode::DRAW_INDEXED == mode) || (mode == GraphicsMode::DRAW_OR_DRAW_INDEXED))
    {
      if (dirtyState.test(DirtyState::INPUT_LAYOUT))
      {
        ctx.bindVertexDecl(externalInputLayout);
      }

      constexpr uint32_t vertexBufferMask = 1u << ResourceDirtyState::VERTEX_BUFFER_0 | 1u << ResourceDirtyState::VERTEX_BUFFER_1 |
                                            1u << ResourceDirtyState::VERTEX_BUFFER_2 | 1u << ResourceDirtyState::VERTEX_BUFFER_3;

      const uint32_t vertexBufferBits = (resourceDirtyState.to_uint32() & vertexBufferMask) >> ResourceDirtyState::VERTEX_BUFFER_0;

      for (auto i : LsbVisitor{vertexBufferBits})
      {
        if (auto gbuf = (GenericBufferInterface *)vertexBuffers[i])
        {
          gbuf->updateDeviceBuffer([](auto &buf) { buf.resourceId.markUsedAsConstOrVertexBuffer(); });
          ctx.bindVertexBuffer(i, {get_any_buffer_ref(gbuf), vertexBufferOffsets[i]}, vertexStries[i]);
        }
      }
    }

#if !_TARGET_XBOXONE
    if (auto shadingTier = device.getVariableShadingRateTier())
    {
      if (dirtyState.test(DirtyState::VARIABLE_RATE_SHADING))
      {
        ctx.setVariableRateShading(constantShadingRate, vertexShadingRateCombiner, pixelShadingRateCombiner);
        // a few warnings about wrong configurations
        if (shadingRateTexture)
        {
          if (DAGOR_UNLIKELY(D3D12_SHADING_RATE_COMBINER_PASSTHROUGH == pixelShadingRateCombiner))
          {
            // sort of valid usage, but when no rate texture is needed it should be set to null
            logwarn("DX12: VRS: Pixel Shading Rate Combiner is set to PASSTHROUGH, but a sampling "
                    "rate texture is set, with this combiner the texture is not used");
          }
        }
        else if (shadingTier == 1)
        {
          // on T1 combiners have no effect
          if (DAGOR_UNLIKELY(D3D12_SHADING_RATE_COMBINER_PASSTHROUGH != pixelShadingRateCombiner))
          {
            logwarn("DX12: VRS: Device is VRS Tier 1 and Pixel Shading Rate Combiner is not "
                    "PASSTHROUGH, which is invalid and will be ignored");
          }
          if (DAGOR_UNLIKELY(D3D12_SHADING_RATE_COMBINER_PASSTHROUGH != vertexShadingRateCombiner))
          {
            logwarn("DX12: VRS: Device is VRS Tier 1 and Vertex Shading Rate Combiner is not "
                    "PASSTHROUGH, which is invalid and will be ignored");
          }
        }
        else
        {
          if (DAGOR_UNLIKELY(D3D12_SHADING_RATE_COMBINER_OVERRIDE == pixelShadingRateCombiner))
          {
            // this is turning VRS off in a wired way
            logwarn("DX12: VRS: Pixel Shading Rate Combiner is set to OVERRIDE, but no sampling rate "
                    "texture is set, it will override to 1x1");
          }
          if (DAGOR_UNLIKELY(D3D12_SHADING_RATE_COMBINER_MIN == pixelShadingRateCombiner))
          {
            // this is turning VRS off in a wired way
            logwarn("DX12: VRS: Pixel Shading Rate Combiner is set to MIN, but no sampling rate "
                    "texture is set, this will min to 1x1");
          }
          if (DAGOR_UNLIKELY(D3D12_SHADING_RATE_COMBINER_MAX == pixelShadingRateCombiner))
          {
            logwarn("DX12: VRS: Pixel Shading Rate Combiner is set to MAX, but no sampling rate "
                    "texture is set, consider using PASSTHROUGH instead");
          }
          if (DAGOR_UNLIKELY(D3D12_SHADING_RATE_COMBINER_SUM == pixelShadingRateCombiner))
          {
            logwarn("DX12: VRS: Pixel Shading Rate Combiner is set to SUM, but no sampling rate "
                    "texture is set, this is effectively adding one to the sampling rate of the "
                    "previous stage");
          }
        }
      }
      if (dirtyState.test(DirtyState::VARIABLE_RATE_SHADING_TEXTURE) && shadingTier > 1)
      {
        ctx.setVariableRateShadingTexture(shadingRateTexture ? cast_to_texture_base(shadingRateTexture)->getDeviceImage() : nullptr);
      }
    }
#endif

    dirtyState &= unchangedMask;
    resourceDirtyState &= unchangedResourceMask;

    validate_draw_call_produces_anything();
    return true;
  }
  void flushCompute(DeviceContext &ctx)
  {
    if (computeProgram == ProgramID::Null())
      return;
    DirtyState::Type computeMask;
    computeMask.set(DirtyState::COMPUTE_PROGRAM);
    if (dirtyState.test(DirtyState::COMPUTE_PROGRAM))
    {
      ctx.setComputePipeline(computeProgram);
    }

    OSSpinlockScopedLock resourceBindingLock(resourceBindingGuard);
    // check returns always false, compute shader can not conflict with framebuffer
    // also no check needed for const depth stencil target, pass has to end
    flushResources(
      ctx, STAGE_CS,                                                        //
      [](auto &buf) { buf.resourceId.markUsedAsNonPixelShaderResource(); }, //
      [](BaseTex *, ImageViewState) { return false; });


    if (dirtyState.test(DirtyState::COMPUTE_CONST_REGISTERS))
    {
      if (!stageResources[STAGE_CS].bRegisterBuffers[0])
      {
        ctx.pushConstRegisterData(STAGE_CS, getRegisterSection(STAGE_CS).first(registerSpaceSizes[STAGE_CS]));
        computeMask.set(DirtyState::COMPUTE_CONST_REGISTERS);
      }
    }

    dirtyState &= ~computeMask;
  }

#if D3D_HAS_RAY_TRACING
  void setStageTRegisterRaytraceAccelerationStructure(uint32_t stage, uint32_t index, RaytraceAccelerationStructure *as)
  {
    G_ASSERT(stage < countof(stageResources));
    StageResourcesState &target = stageResources[stage];

    OSSpinlockScopedLock resourceBindingLock(resourceBindingGuard);

    if (auto maybeTex = target.tRegisterResources.getIf<BaseTex *>(index))
      (*maybeTex)->setSrvBinding(stage, index, false);

    target.markDirtyT(index, target.tRegisterResources[index] != as);

    if (as)
      target.tRegisterResources.emplace<RaytraceAccelerationStructure *>(index, as);
    else
      target.tRegisterResources.emplace<Vacant>(index);
  }
#endif

  void setClearColor(uint32_t index, E3DCOLOR color) { clearColors[index] = color; }

  void setClearDepthStencil(float depth, uint8_t stencil)
  {
    clearDepth = depth;
    clearStencil = stencil;
  }

  void beginMrtClear(uint32_t start)
  {
    // need to take the lock to guard toggleBits (TODO may move out of bitset)
    OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};
    toggleBits.set(ToggleState::MRT_CLEAR);
    currentMrtClearTarget = nextMrtClearTarget = start;
  }

  void endMrtClear(DeviceContext &ctx)
  {
    // need to take the lock here to keep consistent ordering
    ScopedCommitLock lock{ctx};
    // need to take the lock to guard toggleBits (TODO may move out of bitset)
    OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};
    toggleBits.reset(ToggleState::MRT_CLEAR);
    const uint32_t clearMask = (toggleBits.test(ToggleState::CLEAR_COLOR) ? CLEAR_TARGET : 0) |
                               (toggleBits.test(ToggleState::CLEAR_DEPTH) ? CLEAR_ZBUFFER : 0) |
                               (toggleBits.test(ToggleState::CLEAR_STENCIL) ? CLEAR_STENCIL : 0);
    if (clearMask)
    {
      flushRenderTargets(ctx);
      ctx.clearRenderTargets(viewports[0], clearMask, clearColors, clearDepth, clearStencil);
    }
    toggleBits.reset(ToggleState::CLEAR_COLOR);
    toggleBits.reset(ToggleState::CLEAR_DEPTH);
    toggleBits.reset(ToggleState::CLEAR_STENCIL);
  }

  void clearView(DeviceContext &ctx, uint32_t mask, E3DCOLOR color, float depth, uint8_t stencil)
  {
    // need to take the lock here to keep consistent ordering
    ScopedCommitLock lock{ctx};
    // need to take the lock to guard toggleBits (TODO may move out of bitset)
    OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};
    resourceDirtyState.set(ResourceDirtyState::FRAMEBUFFER);
    if (toggleBits.test(ToggleState::MRT_CLEAR))
    {
      if (currentMrtClearTarget == 0)
      {
        if (mask & CLEAR_ZBUFFER)
        {
          clearDepth = depth;
          toggleBits.set(ToggleState::CLEAR_DEPTH);
        }
        if (mask & CLEAR_STENCIL)
        {
          clearStencil = stencil;
          toggleBits.set(ToggleState::CLEAR_STENCIL);
        }
      }
      if (mask & CLEAR_TARGET)
      {
        clearColors[currentMrtClearTarget] = color;
        toggleBits.set(ToggleState::CLEAR_COLOR);
      }
    }
    else
    {
      if (mask & CLEAR_TARGET)
      {
        eastl::fill(eastl::begin(clearColors), eastl::end(clearColors), color);
      }
      flushRenderTargets(ctx);
      ctx.clearRenderTargets(viewports[0], mask, clearColors, depth, stencil);
    }
  }

  void notifySwapchainChange()
  {
    OSSpinlockScopedLock resourceBindingLock(resourceBindingGuard);
    if (renderTargets.isBackBufferColor())
    {
      resourceDirtyState.set(ResourceDirtyState::FRAMEBUFFER);
      toggleBits.set(ToggleState::FORCE_SET_BACKBUFFER);
    }
  }

  void markBufferStagesDirty(GenericBufferInterface *buffer, bool check_vb, bool check_const, bool check_srv, bool check_uav)
  {
    OSSpinlockScopedLock resourceBindingLock(resourceBindingGuard);
    markBufferStagesDirtyNoLock(buffer, check_vb, check_const, check_srv, check_uav);
  }

  void markBufferStagesDirtyNoLock(GenericBufferInterface *buffer, bool check_vb, bool check_const, bool check_srv, bool check_uav)
  {
    if (indexBuffer == buffer)
    {
      markResourceDirty(ResourceDirtyState::INDEX_BUFFER, true);
    }
    if (check_vb)
    {
      for (auto [i, vBuffer] : enumerate(vertexBuffers, ResourceDirtyState::VERTEX_BUFFER_0))
      {
        if (vBuffer == buffer)
        {
          markResourceDirty(ResourceDirtyState::Bits(i), true);
        }
      }
    }
    for (auto &stage : stageResources)
    {
      if (check_const)
      {
        for (auto [j, bBuffer] : enumerate(stage.bRegisterBuffers))
        {
          if (bBuffer == buffer)
          {
            stage.markDirtyB(j, true);
          }
        }
      }
      if (check_srv)
      {
        for (auto [j, tBuffer] : enumerate(stage.tRegisterResources))
        {
          if (tBuffer == buffer)
          {
            stage.markDirtyT(j, true);
          }
        }
      }
      if (check_uav)
      {
        for (auto [j, uBuffer] : enumerate(stage.uRegisterResources))
        {
          if (uBuffer == buffer)
          {
            stage.markDirtyU(j, true);
          }
        }
      }
    }
  }

  void notifyDelete(GenericBufferInterface *buffer)
  {
    OSSpinlockScopedLock resourceBindingLock(resourceBindingGuard);
    if (indexBuffer == buffer)
    {
      markResourceDirty(ResourceDirtyState::INDEX_BUFFER, true);
      indexBuffer = nullptr;
    }
    for (auto [i, vBuffer] : enumerate(vertexBuffers, ResourceDirtyState::VERTEX_BUFFER_0))
    {
      if (vBuffer == buffer)
      {
        markResourceDirty(ResourceDirtyState::Bits(i), true);
        vertexBuffers[i] = nullptr;
        vertexBufferOffsets[i] = 0;
        vertexStries[i] = 0;
      }
    }
    for (auto [i, stage] : enumerate(stageResources))
    {
      for (uint32_t j = 0; j < dxil::MAX_B_REGISTERS; ++j)
      {
        if (stage.bRegisterBuffers[j] == buffer)
        {
          stage.markDirtyB(j, true);
          stage.bRegisterBuffers[j] = nullptr;
          stage.bRegisterOffsets[j] = 0;
          stage.bRegisterSizes[j] = 0;
        }
      }
      for (uint32_t j = 0; j < dxil::MAX_T_REGISTERS; ++j)
      {
        if (stage.tRegisterResources[j] == buffer)
        {
          stage.markDirtyT(j, true);
          stage.tRegisterResources.emplace<Vacant>(j);
        }
      }
      for (uint32_t j = 0; j < dxil::MAX_U_REGISTERS; ++j)
      {
        if (stage.uRegisterResources[j] == buffer)
        {
          stage.markDirtyU(j, true);
          stage.uRegisterResources.emplace<Vacant>(j);
        }
      }
    }
  }

  void removeTRegisterBuffer(GenericBufferInterface *buffer)
  {
    OSSpinlockScopedLock resourceBindingLock(resourceBindingGuard);
    // for (auto [i, stage] : enumerate(stageResources))
    for (auto &stage : stageResources)
    {
      for (uint32_t j = 0; j < dxil::MAX_T_REGISTERS; ++j)
      {
        if (stage.tRegisterResources[j] == buffer)
        {
          // logdbg("Active texture buffer (stage %u, slot %u) bound as UAV...", i, j);
          stage.markDirtyT(j, true);
          stage.tRegisterResources.emplace<Vacant>(j);
        }
      }
    }
  }

  void setDynamicAndStaticState(const RenderStateSystem::PublicStateInfo &state)
  {
    markDirty(DirtyState::GRAPHICS_STATIC_STATE, state.staticRenderStateID != graphicsStaticState);
    markDirty(DirtyState::GRAPHICS_STENCIL_REFERENCE, state.dynamicState.stencilRef != graphicsDynamicState.stencilRef);
    markDirty(DirtyState::GRAPHICS_SCISSOR_TEST_ENABLE, state.dynamicState.enableScissor != graphicsDynamicState.enableScissor);
    graphicsDynamicState = state.dynamicState;
    graphicsStaticState = state.staticRenderStateID;
  };

#if !_TARGET_XBOXONE
  void setVariableShadingRate(D3D12_SHADING_RATE constant_rate, D3D12_SHADING_RATE_COMBINER vertex_combiner,
    D3D12_SHADING_RATE_COMBINER pixel_combiner)
  {
    markDirty(DirtyState::VARIABLE_RATE_SHADING, constantShadingRate != constant_rate ||
                                                   vertexShadingRateCombiner != vertex_combiner ||
                                                   pixelShadingRateCombiner != pixel_combiner);
    constantShadingRate = constant_rate;
    vertexShadingRateCombiner = vertex_combiner;
    pixelShadingRateCombiner = pixel_combiner;
  }
  void setVariableShadingRateTexture(BaseTexture *rate_texture)
  {
    markDirty(DirtyState::VARIABLE_RATE_SHADING_TEXTURE, shadingRateTexture != rate_texture);
    shadingRateTexture = rate_texture;
  }
#endif

  void dirtySRVNoLock([[maybe_unused]] BaseTex *texture, uint32_t stage, const Bitset<dxil::MAX_T_REGISTERS> &slots)
  {
    auto &st = stageResources[stage];
    G_ASSERT(slots.any());
    for (auto i : slots)
    {
      st.markDirtyT(i, true);
    }
  }

  void dirtySRV(BaseTex *texture, uint32_t stage, const Bitset<dxil::MAX_T_REGISTERS> &slots)
  {
    OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};
    dirtySRVNoLock(texture, stage, slots);
  }

  void dirtySampler([[maybe_unused]] BaseTex *texture, uint32_t stage, const Bitset<dxil::MAX_T_REGISTERS> &slots)
  {
    OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};
    auto &st = stageResources[stage];
    G_ASSERT(slots.any());
    for (auto i : slots)
    {
      st.markDirtyS(i, true);
    }
  }

  void dirtySRVandSamplerNoLock([[maybe_unused]] BaseTex *texture, uint32_t stage, const Bitset<dxil::MAX_T_REGISTERS> &slots)
  {
    auto &st = stageResources[stage];
    G_ASSERT(slots.any());
    for (auto i : slots)
    {
      st.markDirtyT(i, true);
      st.markDirtyS(i, true);
    }
  }

  void dirtyUAVNoLock([[maybe_unused]] BaseTex *texture, uint32_t stage, const Bitset<dxil::MAX_U_REGISTERS> &slots)
  {
    auto &st = stageResources[stage];
    G_ASSERT(slots.any());
    for (auto i : slots)
    {
      st.markDirtyU(i, true);
    }
  }

  void dirtyRenderTargetNoLock([[maybe_unused]] BaseTex *texture, const Bitset<Driver3dRenderTarget::MAX_SIMRT> &slots)
  {
    if (slots.any())
      resourceDirtyState.set(ResourceDirtyState::FRAMEBUFFER, true);
    toggleBits.set(ToggleState::FORCE_SET_BACKBUFFER);
  }

  void markTextureStagesDirtyNoLock(BaseTex *texture, const eastl::bitset<dxil::MAX_T_REGISTERS> *srvs,
    const eastl::bitset<dxil::MAX_U_REGISTERS> *uavs, eastl::bitset<Driver3dRenderTarget::MAX_SIMRT> rtvs, [[maybe_unused]] bool dsv)
  {
    for (uint32_t s = 0; s < STAGE_MAX_EXT; ++s)
    {
      if (srvs[s].any())
        dirtySRVandSamplerNoLock(texture, s, srvs[s]);
      if (uavs[s].any())
        dirtyUAVNoLock(texture, s, uavs[s]);
    }
    dirtyRenderTargetNoLock(texture, rtvs);
  }

  void notifyDelete(BaseTex *texture, const Bitset<dxil::MAX_T_REGISTERS> *srvs, const Bitset<dxil::MAX_U_REGISTERS> *uavs,
    const Bitset<Driver3dRenderTarget::MAX_SIMRT> &rtvs, bool dsv)
  {
    OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};

    for (auto [s, st] : enumerate(stageResources))
    {
      for (auto i : srvs[s])
      {
        st.tRegisterResources.emplace<Vacant>(i); // no need to notify S
      }
      st.dirtyMasks.tRegisterMask |= srvs[s];

      for (auto i : uavs[s])
      {
        st.uRegisterResources.emplace<Vacant>(i);
      }
      st.dirtyMasks.uRegisterMask |= uavs[s];
    }

    for (auto i : LsbVisitor{renderTargets.used & Driver3dRenderTarget::COLOR_MASK})
    {
      if (renderTargets.color[i].tex == texture)
      {
        renderTargets.removeColor(i);
        resourceDirtyState.set(ResourceDirtyState::FRAMEBUFFER, true);
      }
    }

    for (auto i : LsbVisitor{activeRenderTargets.used & Driver3dRenderTarget::COLOR_MASK})
    {
      if (activeRenderTargets.color[i].tex == texture)
      {
        activeRenderTargets.removeColor(i);
        resourceDirtyState.set(ResourceDirtyState::FRAMEBUFFER, true);
      }
    }

    for (auto i : rtvs)
    {
      renderTargets.removeColor(i);
      toggleBits.set(ToggleState::FORCE_SET_BACKBUFFER);
      resourceDirtyState.set(ResourceDirtyState::FRAMEBUFFER, true);
    }

    if (dsv)
    {
      renderTargets.removeDepth();
      toggleBits.set(ToggleState::FORCE_SET_BACKBUFFER);
      resourceDirtyState.set(ResourceDirtyState::FRAMEBUFFER, true);
    }
  }
};

OSSpinlock &get_resource_binding_guard();
} // namespace drv3d_dx12
