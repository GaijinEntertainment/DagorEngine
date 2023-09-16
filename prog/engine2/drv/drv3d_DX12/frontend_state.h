#pragma once

#include <3d/dag_drv3d.h>
#include <EASTL/bitset.h>

namespace drv3d_dx12
{
struct FrontendState
{
  struct StageResourcesState
  {
    Sbuffer *bRegisterBuffers[dxil::MAX_B_REGISTERS] = {};
    uint32_t bRegisterOffsets[dxil::MAX_B_REGISTERS] = {};
    uint32_t bRegisterSizes[dxil::MAX_B_REGISTERS] = {};
    BaseTex *tRegisterTextures[dxil::MAX_T_REGISTERS] = {};
    ImageViewState tRegisterTextureViews[dxil::MAX_T_REGISTERS] = {};
    d3d::SamplerHandle sRegisterSamplers[dxil::MAX_S_REGISTERS] = {};
    Sbuffer *tRegisterBuffers[dxil::MAX_T_REGISTERS] = {};
#if D3D_HAS_RAY_TRACING
    RaytraceAccelerationStructure *tRegisterRaytraceAccelerataionStructures[dxil::MAX_T_REGISTERS] = {};
#endif
    BaseTex *uRegisterTextures[dxil::MAX_U_REGISTERS] = {};
    ImageViewState uRegisterTextureViews[dxil::MAX_U_REGISTERS] = {};
    Sbuffer *uRegisterBuffers[dxil::MAX_U_REGISTERS] = {};

    ShaderStageResouceUsageMask useMask{~0ul};
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
      RAYTRACE_PROGRAM,
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
    typedef eastl::bitset<COUNT> Type;
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
    typedef eastl::bitset<COUNT> Type;
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
    result.setBackbufDepth();
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
  GraphicsProgramIOMask graphicsProgramioMask{};
  InputLayoutID externalInputLayout = InputLayoutID::Null();
  InternalInputLayoutID internalInputLayout = InternalInputLayoutID::Null();

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
#if D3D_HAS_RAY_TRACING
  ProgramID raytraceProgram = ProgramID::Null();
#endif
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

  OSSpinlock resourceBindingGuard;

  static void updateRenderTargetBindingStates(DeviceContext &ctx, const Driver3dRenderTarget &rts, bool in_use)
  {
    if (rts.isColorUsed())
    {
      for (uint32_t i = 0; i < Driver3dRenderTarget::MAX_SIMRT; ++i)
      {
        if (rts.isColorUsed(i))
        {
          auto tex = cast_to_texture_base(rts.getColor(i).tex);
          if (tex)
          {
            tex->setRTVBinding(i, in_use);
          }
        }
      }
    }
    if (rts.isDepthUsed())
    {
      auto tex = cast_to_texture_base(rts.getDepth().tex);
      if (!tex)
      {
        tex = ctx.getSwapchainDepthStencilTextureAnySize();
      }
      tex->setDSVBinding(in_use);
    }
  }

  void preRecovery()
  {
    OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};
    dirtyState = ~DirtyState::Type(0);
    resourceDirtyState = ~ResourceDirtyState::Type{0};
    for (int i = 0; i < STAGE_MAX_EXT; i++)
      stageResources[i].allDirty();
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

  template <typename A, typename D, typename U>
  void flushResources(A &ctx, uint32_t stage, D is_const_ds, U update_srv_buffer)
  {
    StageResourcesState &target = stageResources[stage];
    auto updateMask = target.dirtyMasks & target.useMask;
    if (updateMask.bRegisterMask.any())
    {
      const size_t last = eastl::min(updateMask.bRegisterMask.find_last(), size_t(dxil::MAX_B_REGISTERS - 1));
      for (size_t i = updateMask.bRegisterMask.find_first(); i <= last; ++i)
      {
        if (updateMask.bRegisterMask.test(i))
        {
          Sbuffer *buffer = target.bRegisterBuffers[i];
          if (buffer)
          {
            auto gbuf = (GenericBufferInterface *)buffer;
            gbuf->updateDeviceBuffer([](auto &buf) { buf.resourceId.markUsedAsConstOrVertexBuffer(); });
            ctx.setConstBuffer(stage, i, {get_any_buffer_ref(gbuf), target.bRegisterOffsets[i], target.bRegisterSizes[i]});
          }
          else
          {
            ctx.setConstBuffer(stage, i, {});
          }
        }
      }
    }
    if (updateMask.tRegisterMask.any())
    {
      const size_t last = eastl::min(updateMask.tRegisterMask.find_last(), size_t(dxil::MAX_T_REGISTERS - 1));
      const size_t first = updateMask.tRegisterMask.find_first();
      // have to check all slots regardless of mask, a state of a texture might has changed
      for (size_t i = first; i <= last; ++i)
      {
        if (updateMask.tRegisterMask.test(i))
        {
          if (target.tRegisterBuffers[i])
          {
            auto gbuf = (GenericBufferInterface *)target.tRegisterBuffers[i];
            gbuf->updateDeviceBuffer(update_srv_buffer);
            G_ASSERT(!gbuf->isStreamBuffer());
            ctx.setSRVBuffer(stage, i, gbuf->getDeviceBuffer());
          }
          else if (target.tRegisterTextures[i])
          {
            BaseTex *texture = target.tRegisterTextures[i];
            // if it is right now a stub texture then use it explicitly to avoid a race
            if (texture->isStub())
            {
              texture = texture->getStubTex();
            }

            ImageViewState view = texture->getViewInfo();
            ctx.setSRVTexture(stage, i, texture, view, is_const_ds(texture, view));
          }
#if D3D_HAS_RAY_TRACING
          else if (target.tRegisterRaytraceAccelerataionStructures[i])
          {
            ctx.setRaytraceAccelerationStructure(stage, i, target.tRegisterRaytraceAccelerataionStructures[i]);
          }
#endif
          else
          {
            ctx.setSRVNull(stage, i);
          }
        }
      }
    }

    if (updateMask.sRegisterMask.any())
    {
      const size_t last = eastl::min(updateMask.sRegisterMask.find_last(), size_t(dxil::MAX_T_REGISTERS - 1));
      const size_t first = updateMask.sRegisterMask.find_first();
      // have to check all slots regardless of mask, a state of a texture might has changed
      for (size_t i = first; i <= last; ++i)
      {
        if (updateMask.sRegisterMask.test(i))
        {
          if (target.sRegisterSamplers[i])
          {
            ctx.setSamplerHandle(stage, i, target.sRegisterSamplers[i]);
          }
          else if (target.tRegisterTextures[i])
          {
            ctx.setSampler(stage, i, target.tRegisterTextures[i]->getDeviceSampler());
          }
          else
          {
            ctx.setSampler(stage, i, {});
          }
        }
      }
    }

    if (updateMask.uRegisterMask.any())
    {
      const size_t last = eastl::min(updateMask.uRegisterMask.find_last(), size_t(dxil::MAX_U_REGISTERS - 1));
      for (size_t i = updateMask.uRegisterMask.find_first(); i <= last; ++i)
      {
        if (updateMask.uRegisterMask.test(i))
        {
          if (target.uRegisterBuffers[i])
          {
            auto gbuf = (GenericBufferInterface *)target.uRegisterBuffers[i];
            gbuf->updateDeviceBuffer([](auto &buf) { buf.resourceId.markUsedAsUAVBuffer(); });
            G_ASSERT(!gbuf->isStreamBuffer());
            ctx.setUAVBuffer(stage, i, gbuf->getDeviceBuffer());
          }
          else if (target.uRegisterTextures[i])
          {
            BaseTex *texture = target.uRegisterTextures[i];
            Image *image = texture->getDeviceImage();
            ctx.setUAVTexture(stage, i, image, target.uRegisterTextureViews[i]);
          }
          else
          {
            ctx.setUAVNull(stage, i);
          }
        }
      }
    }
    target.dirtyMasks ^= updateMask;
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
  void getRenderTargets(T clb)
  {
    OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};
    // ensure that the input is of const ref and disallows modifications
    clb(const_cast<const Driver3dRenderTarget &>(renderTargets));
  }

  Extent2D getFramebufferExtent(DeviceContext &ctx)
  {
    OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};
    return getFramebufferExtentInternal(ctx);
  }

  void markDirtyIfMatchesInTRegister(uint32_t stage, BaseTex *texture, MipMapIndex mip, ArrayLayerIndex face,
    eastl::bitset<dxil::MAX_T_REGISTERS> slots)
  {
    G_UNUSED(texture); // can be used to validate
    StageResourcesState &target = stageResources[stage];
    auto start = slots.find_first();
    auto stop = slots.find_last();
    for (; start <= stop; ++start)
    {
      if (slots.test(start))
      {
        auto tex = target.tRegisterTextures[start];
        ImageViewState view = tex->getViewInfo();
        auto mipRange = view.getMipRange();
        auto arrayRange = view.getArrayRange();
        target.markDirtyT(start, mipRange.isInside(mip) && arrayRange.isInside(face));
      }
    }
  }

  bool isConstDepthStencilTarget(BaseTex *texture, ImageViewState view)
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

  void removeImageFromRenderTargets(DeviceContext &ctx, BaseTexture *texture, ImageViewState view)
  {
    const auto mipRange = view.getMipRange();
    const auto arrayRange = view.getArrayRange();

    for (uint32_t i = 0; i < Driver3dRenderTarget::MAX_SIMRT && 0 != ((renderTargets.used & Driver3dRenderTarget::COLOR_MASK) >> i);
         ++i)
    {
      if (renderTargets.isColorUsed(i) && renderTargets.color[i].tex == texture &&
          mipRange.isInside(MipMapIndex::make(renderTargets.color[i].level)) &&
          arrayRange.isInside(ArrayLayerIndex::make(renderTargets.color[i].layer)))
      {
        removeColorTarget(i);
      }
    }

    if (renderTargets.isDepthUsed())
    {
      if (renderTargets.depth.tex == texture && mipRange.isInside(MipMapIndex::make(renderTargets.depth.level)) &&
          arrayRange.isInside(ArrayLayerIndex::make(renderTargets.depth.layer)))
      {
        removeDepthStencilTarget(ctx);
      }
    }
  }

  BaseTexture *getColorTarget(uint32_t index)
  {
    OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};
    return renderTargets.color[index].tex;
  }

  void setRenderTargets(DeviceContext &ctx, const Driver3dRenderTarget &rt)
  {
    OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};
    resourceDirtyState.set(ResourceDirtyState::FRAMEBUFFER, true);
    updateRenderTargetBindingStates(ctx, renderTargets, false);
    updateRenderTargetBindingStates(ctx, rt, true);
    renderTargets = rt;
    for (uint32_t i = 0; i < Driver3dRenderTarget::MAX_SIMRT; ++i)
    {
      if (renderTargets.isColorUsed(i))
      {
        const Driver3dRenderTarget::RTState rtState = renderTargets.getColor(i);
        if (rtState.tex)
        {
          cast_to_texture_base(rtState.tex)->dirtyBoundUAVsNoLock();
          cast_to_texture_base(rtState.tex)->dirtyBoundSRVsNoLock();
        }
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
        ot->setRTVBinding(index, false);
      }
    }
    if (texture)
    {
      texture->setRTVBinding(index, true);
      texture->dirtyBoundSRVsNoLock();
      texture->dirtyBoundUAVsNoLock();
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
        ot->setRTVBinding(index, false);
      }
    }
    renderTargets.removeColor(index);
  }
  void resetColorTargetsToBackBuffer()
  {
    OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};
    resourceDirtyState.set(ResourceDirtyState::FRAMEBUFFER, true);
    if (renderTargets.isColorUsed())
    {
      for (uint32_t i = 0; i < Driver3dRenderTarget::MAX_SIMRT; ++i)
      {
        if (renderTargets.isColorUsed(i))
        {
          auto tex = cast_to_texture_base(renderTargets.getColor(i).tex);
          if (tex)
          {
            tex->setRTVBinding(i, false);
          }
        }
      }
    }
    renderTargets.setBackbufColor();
  }
  void setDepthStencilTarget(DeviceContext &ctx, BaseTex *texture, uint32_t face_index, bool read_only)
  {
    OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};
    resourceDirtyState.set(ResourceDirtyState::FRAMEBUFFER, true);
    if (renderTargets.isDepthUsed())
    {
      auto ot = cast_to_texture_base(renderTargets.getDepth().tex);
      if (!ot)
      {
        ot = ctx.getSwapchainDepthStencilTextureAnySize();
      }
      ot->setDSVBinding(false);
    }
    texture->setDSVBinding(true);
    renderTargets.setDepth(texture, face_index, read_only);
  }
  void removeDepthStencilTarget(DeviceContext &ctx)
  {
    OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};
    resourceDirtyState.set(ResourceDirtyState::FRAMEBUFFER, true);
    if (renderTargets.isDepthUsed())
    {
      auto ot = cast_to_texture_base(renderTargets.getDepth().tex);
      if (!ot)
      {
        ot = ctx.getSwapchainDepthStencilTextureAnySize();
      }
      ot->setDSVBinding(false);
    }
    renderTargets.removeDepth();
  }
  void resetDepthStencilToBackBuffer(DeviceContext &ctx)
  {
    OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};
    resourceDirtyState.set(ResourceDirtyState::FRAMEBUFFER, true);
    if (renderTargets.isDepthUsed())
    {
      auto ot = cast_to_texture_base(renderTargets.getDepth().tex);
      if (ot)
      {
        ot->setDSVBinding(false);
      }
    }
    renderTargets.setBackbufDepth();
    // can be null when the window just got destroyed and the splashsceen thread still draws stuff...
    auto swDST = ctx.getSwapchainDepthStencilTextureAnySize();
    if (swDST)
    {
      swDST->setDSVBinding(true);
    }
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
    stageResources[STAGE_VS].useMask = program_info.vertexShaderStageResourceUses;
    stageResources[STAGE_PS].useMask = program_info.pixelShaderStageResourceUses;
    graphicsProgramioMask = program_info.ioMask;

    externalInputLayout = program_info.input;
    // only need to update input layout if the internal layout changed, external is not important
    // here
    markDirty(DirtyState::INPUT_LAYOUT, program_info.internalInputLayoutID != internalInputLayout);
    internalInputLayout = program_info.internalInputLayoutID;
  }

  void setInputLayout(InputLayoutID layout)
  {
    if (externalInputLayout != layout)
    {
      markDirty(DirtyState::INPUT_LAYOUT, true);
      externalInputLayout = layout;
      internalInputLayout = InternalInputLayoutID::Null();
    }
  }

  void setStageBRegisterBuffer(uint32_t stage, uint32_t index, Sbuffer *buffer, uint32_t offset, uint32_t size)
  {
    G_ASSERT(stage < array_size(stageResources));
    StageResourcesState &target = stageResources[stage];
    G_ASSERT(index < array_size(target.bRegisterBuffers));
    OSSpinlockScopedLock resourceBindingLock(resourceBindingGuard);
    target.markDirtyB(index,
      target.bRegisterBuffers[index] != buffer || target.bRegisterOffsets[index] != offset || target.bRegisterSizes[index] != size);
    target.bRegisterBuffers[index] = buffer;
    target.bRegisterOffsets[index] = offset;
    target.bRegisterSizes[index] = size;
  }

  void setStageTRegisterBuffer(uint32_t stage, uint32_t index, Sbuffer *buffer)
  {
    G_ASSERT(stage < array_size(stageResources));
    StageResourcesState &target = stageResources[stage];
    G_ASSERT(index < array_size(target.tRegisterBuffers));
    OSSpinlockScopedLock resourceBindingLock(resourceBindingGuard);
    if (target.tRegisterTextures[index])
    {
      target.tRegisterTextures[index]->setSRVBinding(stage, index, false);
      target.tRegisterTextures[index] = nullptr;
    }
    target.markDirtyT(index, target.tRegisterBuffers[index] != buffer);
    target.tRegisterBuffers[index] = buffer;
#if D3D_HAS_RAY_TRACING
    target.tRegisterRaytraceAccelerataionStructures[index] = nullptr;
#endif
  }

  void setStageURegisterBuffer(uint32_t stage, uint32_t index, Sbuffer *buffer)
  {
    G_ASSERT(stage < array_size(stageResources));
    StageResourcesState &target = stageResources[stage];
    G_ASSERT(index < array_size(target.uRegisterBuffers));
    GenericBufferInterface *prevBuf = nullptr;
    {
      OSSpinlockScopedLock resourceBindingLock(resourceBindingGuard);
      if (target.uRegisterTextures[index])
      {
        target.uRegisterTextures[index]->setUAVBinding(stage, index, false);
        target.uRegisterTextures[index] = nullptr;
      }
      target.markDirtyU(index, target.uRegisterBuffers[index] != buffer);
      if (target.uRegisterBuffers[index])
        prevBuf = (GenericBufferInterface *)target.uRegisterBuffers[index];
      target.uRegisterBuffers[index] = buffer;
    }
    if (buffer != prevBuf)
    {
      if (prevBuf)
      {
        auto flags = prevBuf->getFlags();
        notifyDiscard(target.uRegisterBuffers[index], SBCF_BIND_VERTEX & flags, SBCF_BIND_CONSTANT & flags,
          SBCF_BIND_SHADER_RES & flags, SBCF_BIND_UNORDERED & flags);
      }
    }
  }

  void setStageSRVTexture(uint32_t stage, uint32_t index, BaseTex *texture)
  {
    G_ASSERT(stage < array_size(stageResources));
    StageResourcesState &target = stageResources[stage];
    G_ASSERT(index < array_size(target.tRegisterTextures));
    OSSpinlockScopedLock resourceBindingLock(resourceBindingGuard);
    if (texture)
    {
      texture->dirtyBoundUAVsNoLock();
      texture->dirtyBoundRTVsNoLock();
    }
    target.markDirtyT(index, target.tRegisterTextures[index] != texture);
    target.markDirtyS(index, target.tRegisterTextures[index] != texture);
    if (target.tRegisterTextures[index])
    {
      target.tRegisterTextures[index]->setSRVBinding(stage, index, false);
    }
    if (texture)
    {
      texture->setSRVBinding(stage, index, true);
    }
    target.tRegisterTextures[index] = texture;
    target.tRegisterBuffers[index] = nullptr;
    target.sRegisterSamplers[index] = {};
#if D3D_HAS_RAY_TRACING
    target.tRegisterRaytraceAccelerataionStructures[index] = nullptr;
#endif
  }

  void setStageSampler(uint32_t stage, uint32_t index, d3d::SamplerHandle handle)
  {
    G_ASSERT(stage < array_size(stageResources));
    StageResourcesState &target = stageResources[stage];
    G_ASSERT(index < array_size(target.sRegisterSamplers));
    OSSpinlockScopedLock resourceBindingLock(resourceBindingGuard);
    target.markDirtyS(index, target.sRegisterSamplers[index] != handle);
    target.sRegisterSamplers[index] = handle;
  }

  void setStageUAVTexture(uint32_t stage, uint32_t index, BaseTex *texture, ImageViewState view)
  {
    G_ASSERT(stage < array_size(stageResources));
    StageResourcesState &target = stageResources[stage];
    G_ASSERT(index < array_size(target.uRegisterTextures));
    OSSpinlockScopedLock resourceBindingLock(resourceBindingGuard);
    if (texture)
    {
      texture->dirtyBoundSRVsNoLock();
      texture->dirtyBoundRTVsNoLock();
    }
    target.markDirtyU(index, target.uRegisterTextures[index] != texture || target.uRegisterTextureViews[index] != view);
    if (target.uRegisterTextures[index])
    {
      target.uRegisterTextures[index]->setUAVBinding(stage, index, false);
    }
    if (texture)
    {
      texture->setUAVBinding(stage, index, true);
    }
    target.uRegisterTextures[index] = texture;
    target.uRegisterTextureViews[index] = view;
    target.uRegisterBuffers[index] = nullptr;
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
    auto result = ctx.getSwapchainExtent();
    if (!renderTargets.isBackBufferColor())
    {
      if (renderTargets.isColorUsed())
      {
        for (uint32_t i = 0; i < Driver3dRenderTarget::MAX_SIMRT; ++i)
        {
          if (renderTargets.isColorUsed(i))
          {
            const Extent3D &size = cast_to_texture_base(*renderTargets.color[i].tex).getMipmapExtent(renderTargets.color[i].level);
            result.width = size.width;
            result.height = size.height;
            break;
          }
        }
      }
      else if (renderTargets.isDepthUsed() && renderTargets.depth.tex)
      {
        const Extent3D &size = cast_to_texture_base(*renderTargets.depth.tex).getMipmapExtent(renderTargets.depth.level);
        result.width = size.width;
        result.height = size.height;
      }
    }
    return result;
  }

  void calculateViewport(DeviceContext &ctx)
  {
    if (!dirtyState.test(DirtyState::VIEWPORT_FROM_RENDER_TARGETS))
      return;

    auto extent = getFramebufferExtentInternal(ctx);
    ViewportState newViewport = viewports[0];
    newViewport.x = 0;
    newViewport.y = 0;
    newViewport.width = extent.width;
    newViewport.height = extent.height;

    setViewports({&newViewport, 1});
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

  void setComputeProgram(ProgramID p, ShaderStageResouceUsageMask resource_mask)
  {
    markDirty(DirtyState::COMPUTE_PROGRAM, computeProgram != p);
    computeProgram = p;
    stageResources[STAGE_CS].useMask = resource_mask;
  }

  uint32_t setComputeConstRegisterCount(uint32_t cnt)
  {
    if (cnt)
      cnt = clamp<uint32_t>(nextPowerOfTwo(cnt), MIN_COMPUTE_CONST_REGISTERS, MAX_COMPUTE_CONST_REGISTERS);
    else
      cnt = MIN_COMPUTE_CONST_REGISTERS; // TODO update things to allow 0 (eg shader can tell how many it needs)
    markDirty(DirtyState::COMPUTE_CONST_REGISTERS, registerSpaceSizes[STAGE_CS] < cnt);
    registerSpaceSizes[STAGE_CS] = cnt;
    return cnt;
  }
  uint32_t setVertexConstRegisterCount(uint32_t cnt)
  {
    if (cnt)
      cnt = clamp<uint32_t>(nextPowerOfTwo(cnt), VERTEX_SHADER_MIN_REGISTERS, VERTEX_SHADER_MAX_REGISTERS);
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

    for (uint32_t i = 1; i < Driver3dRenderTarget::MAX_SIMRT && (0 != ((renderTargets.used & Driver3dRenderTarget::COLOR_MASK) >> i));
         ++i)
    {
      if (renderTargets.isColorUsed(i))
      {
        ImageViewState &targetView = newViews[i];
        Image *&targetImage = newImages[i];

        BaseTex *base = cast_to_texture_base(renderTargets.color[i].tex);
        targetView = base->getViewInfoRenderTarget(MipMapIndex::make(renderTargets.color[i].level),
          ArrayLayerIndex::make(renderTargets.color[i].layer), false);
        targetImage = base->getDeviceImage();
      }
    }

    if (renderTargets.isDepthUsed())
    {
      ImageViewState &targetView = newViews[Driver3dRenderTarget::MAX_SIMRT];
      Image *&targetImage = newImages[Driver3dRenderTarget::MAX_SIMRT];
      BaseTex *depthTex = cast_to_texture_base(renderTargets.depth.tex);
      if (depthTex && depthTex != ctx.getSwapchainDepthStencilTextureAnySize())
      {
        targetView = depthTex->getViewInfoRenderTarget(MipMapIndex::make(renderTargets.depth.level),
          ArrayLayerIndex::make(renderTargets.depth.layer), renderTargets.isDepthReadOnly());
      }
      else
      {
        depthTex = ctx.getSwapchainDepthStencilTexture(getFramebufferExtentInternal(ctx));
        targetView.setFormat(ctx.getSwapchainDepthStencilFormat());
        targetView.isArray = 0;
        targetView.isCubemap = 0;
        targetView.setSingleMipMapRange(MipMapIndex::make(0));
        targetView.setSingleArrayRange(ArrayLayerIndex::make(0));
        targetView.setDSV(renderTargets.isDepthReadOnly());
      }

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
          logerr("Draw call is not producing any output, ps output mask 0x%02X, render target "
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
    DRAW_UP = DRAW,
    DRAW_INDEXED_UP = DRAW,
    ALL = DRAW_INDEXED
  };

  bool flushGraphics(Device &device, ShaderProgramDatabase &shaders, GraphicsMode mode)
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
    unchangedMask.set(DirtyState::RAYTRACE_PROGRAM);

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

    if (resourceDirtyState.test(ResourceDirtyState::INDEX_BUFFER) && (GraphicsMode::DRAW_INDEXED == mode))
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
      ctx, STAGE_VS,
      [=](BaseTex *tex, ImageViewState view) //
      { return isConstDepthStencilTarget(tex, view); },
      [](auto &buf) { buf.resourceId.markUsedAsNonPixelShaderResource(); });

    flushResources(
      ctx, STAGE_PS,
      [=](BaseTex *tex, ImageViewState view) //
      { return isConstDepthStencilTarget(tex, view); },
      [](auto &buf) { buf.resourceId.markUsedAsPixelShaderResource(); });

    if (dirtyState.test(DirtyState::VERTEX_CONST_REGISTERS))
    {
      if (stageResources[STAGE_VS].useMask.bRegisterMask.test(0) && !stageResources[STAGE_VS].bRegisterBuffers[0])
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
      if (stageResources[STAGE_PS].useMask.bRegisterMask.test(0) && !stageResources[STAGE_PS].bRegisterBuffers[0])
      {
        ctx.pushConstRegisterData(STAGE_PS, getRegisterSection(STAGE_PS).first(registerSpaceSizes[STAGE_PS]));
      }
      else
      {
        // happens a lot, not sure why
        unchangedMask.set(DirtyState::PIXEL_CONST_REGISTERS);
      }
    }

    if ((GraphicsMode::DRAW == mode) || (GraphicsMode::DRAW_INDEXED == mode))
    {
      if (dirtyState.test(DirtyState::INPUT_LAYOUT))
      {
        if (InternalInputLayoutID::Null() == internalInputLayout)
          internalInputLayout = shaders.getInternalInputLayout(ctx, externalInputLayout, graphicsProgramioMask,
            ShaderProgramDatabase::Validation::Do, ShaderProgramDatabase::ContextLockState::Locked);

        ctx.bindVertexDecl(internalInputLayout);
      }

      for (uint32_t i = 0; i < MAX_VERTEX_INPUT_STREAMS; ++i)
      {
        if (resourceDirtyState.test(uint32_t(ResourceDirtyState::VERTEX_BUFFER_0) + i) && vertexBuffers[i])
        {
          auto gbuf = (GenericBufferInterface *)vertexBuffers[i];
          gbuf->updateDeviceBuffer([](auto &buf) { buf.resourceId.markUsedAsConstOrVertexBuffer(); });
          ctx.bindVertexBuffer(i, {get_any_buffer_ref(gbuf), vertexBufferOffsets[i]}, vertexStries[i]);
        }
      }
    }

#if !_TARGET_XBOXONE
    if (dirtyState.test(DirtyState::VARIABLE_RATE_SHADING) && (device.getVariableShadingRateTier() > 0))
    {
      ctx.setVariableRateShading(constantShadingRate, vertexShadingRateCombiner, pixelShadingRateCombiner);
      // a few warnings about wrong configurations
      if (shadingRateTexture)
      {
        if (D3D12_SHADING_RATE_COMBINER_PASSTHROUGH == pixelShadingRateCombiner)
        {
          // sort of valid usage, but when no rate texture is needed it should be set to null
          logwarn("DX12: VRS: Pixel Shading Rate Combiner is set to PASSTHROUGH, but a sampling "
                  "rate texture is set, with this combiner the texture is not used");
        }
      }
      else if (device.getVariableShadingRateTier() == 1)
      {
        // on T1 combiners have no effect
        if (D3D12_SHADING_RATE_COMBINER_PASSTHROUGH != pixelShadingRateCombiner)
        {
          logwarn("DX12: VRS: Device is VRS Tier 1 and Pixel Shading Rate Combiner is not "
                  "PASSTHROUGH, which is invalid and will be ignored");
        }
        if (D3D12_SHADING_RATE_COMBINER_PASSTHROUGH != vertexShadingRateCombiner)
        {
          logwarn("DX12: VRS: Device is VRS Tier 1 and Vertex Shading Rate Combiner is not "
                  "PASSTHROUGH, which is invalid and will be ignored");
        }
      }
      else
      {
        if (D3D12_SHADING_RATE_COMBINER_OVERRIDE == pixelShadingRateCombiner)
        {
          // this is turning VRS off in a wired way
          logwarn("DX12: VRS: Pixel Shading Rate Combiner is set to OVERRIDE, but no sampling rate "
                  "texture is set, it will override to 1x1");
        }
        if (D3D12_SHADING_RATE_COMBINER_MIN == pixelShadingRateCombiner)
        {
          // this is turning VRS off in a wired way
          logwarn("DX12: VRS: Pixel Shading Rate Combiner is set to MIN, but no sampling rate "
                  "texture is set, this will min to 1x1");
        }
        if (D3D12_SHADING_RATE_COMBINER_MAX == pixelShadingRateCombiner)
        {
          logwarn("DX12: VRS: Pixel Shading Rate Combiner is set to MAX, but no sampling rate "
                  "texture is set, consider using PASSTHROUGH instead");
        }
        if (D3D12_SHADING_RATE_COMBINER_SUM == pixelShadingRateCombiner)
        {
          logwarn("DX12: VRS: Pixel Shading Rate Combiner is set to SUM, but no sampling rate "
                  "texture is set, this is effectively adding one to the sampling rate of the "
                  "previous stage");
        }
      }
    }
    if (dirtyState.test(DirtyState::VARIABLE_RATE_SHADING_TEXTURE) && (device.getVariableShadingRateTier() > 1))
    {
      ctx.setVariableRateShadingTexture(shadingRateTexture ? cast_to_texture_base(shadingRateTexture)->getDeviceImage() : nullptr);
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

    // check returns always false, compute shader can not conflict with framebuffer
    // also no check needed for const depth stencil target, pass has to end
    flushResources(
      ctx, STAGE_CS, [](BaseTex *, ImageViewState) { return false; },
      [](auto &buf) { buf.resourceId.markUsedAsNonPixelShaderResource(); });

    if (dirtyState.test(DirtyState::COMPUTE_CONST_REGISTERS))
    {
      if (stageResources[STAGE_CS].useMask.bRegisterMask.test(0) && !stageResources[STAGE_CS].bRegisterBuffers[0])
      {
        ctx.pushConstRegisterData(STAGE_CS, getRegisterSection(STAGE_CS).first(registerSpaceSizes[STAGE_CS]));
        computeMask.set(DirtyState::COMPUTE_CONST_REGISTERS);
      }
    }

    dirtyState &= ~computeMask;
  }

#if D3D_HAS_RAY_TRACING
  void setRaytraceProgram(ProgramID p, ShaderStageResouceUsageMask resource_mask)
  {
    markDirty(DirtyState::RAYTRACE_PROGRAM, raytraceProgram != p);
    raytraceProgram = p;
    stageResources[STAGE_RAYTRACE].useMask = resource_mask;
  }

  void setStageTRegisterRaytraceAccelerationStructure(uint32_t stage, uint32_t index, RaytraceAccelerationStructure *as)
  {
    G_ASSERT(stage < array_size(stageResources));
    StageResourcesState &target = stageResources[stage];
    G_ASSERT(index < array_size(target.tRegisterRaytraceAccelerataionStructures));
    OSSpinlockScopedLock resourceBindingLock(resourceBindingGuard);
    if (target.tRegisterTextures[index])
    {
      target.tRegisterTextures[index]->setSRVBinding(stage, index, false);
      target.tRegisterTextures[index] = nullptr;
    }
    target.markDirtyT(index, target.tRegisterRaytraceAccelerataionStructures[index] != as);
    target.tRegisterRaytraceAccelerataionStructures[index] = as;
    target.tRegisterBuffers[index] = nullptr;
  }

  void flushRaytrace(DeviceContext &ctx)
  {
    if (raytraceProgram == ProgramID::Null())
      return;
    DirtyState::Type raytraceMask;
    raytraceMask.set(DirtyState::RAYTRACE_PROGRAM);

    if (dirtyState.test(DirtyState::RAYTRACE_PROGRAM))
      ctx.setRaytracePipeline(raytraceProgram);

    // check returns always false, raytrace shader can not conflict with framebuffer
    // also no check needed for const depth stencil target, pass has to end
    flushResources(
      ctx, STAGE_RAYTRACE, [](BaseTex *, ImageViewState) { return false; },
      [](auto &buf) { buf.resourceId.markUsedAsNonPixelShaderResource(); });

    dirtyState &= ~raytraceMask;
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

  void notifyDiscard(Sbuffer *buffer, bool check_vb, bool check_const, bool check_tex, bool check_storage)
  {
    OSSpinlockScopedLock resourceBindingLock(resourceBindingGuard);
    if (indexBuffer == buffer)
    {
      markResourceDirty(ResourceDirtyState::INDEX_BUFFER, true);
    }
    if (check_vb)
    {
      for (uint32_t i = 0; i < MAX_VERTEX_INPUT_STREAMS; ++i)
      {
        if (vertexBuffers[i] == buffer)
        {
          markResourceDirty(ResourceDirtyState::Bits(ResourceDirtyState::VERTEX_BUFFER_0 + i), true);
        }
      }
    }
    for (uint32_t i = 0; i < STAGE_MAX_EXT; ++i)
    {
      auto &stage = stageResources[i];
      if (check_const)
      {
        for (uint32_t j = 0; j < dxil::MAX_B_REGISTERS; ++j)
        {
          if (stage.bRegisterBuffers[j] == buffer)
          {
            stage.markDirtyB(j, true);
          }
        }
      }
      if (check_tex)
      {
        for (uint32_t j = 0; j < dxil::MAX_T_REGISTERS; ++j)
        {
          if (stage.tRegisterBuffers[j] == buffer)
          {
            stage.markDirtyT(j, true);
          }
        }
      }
      if (check_storage)
      {
        for (uint32_t j = 0; j < dxil::MAX_U_REGISTERS; ++j)
        {
          if (stage.uRegisterBuffers[j] == buffer)
          {
            stage.markDirtyU(j, true);
          }
        }
      }
    }
  }

  void notifyDelete(Sbuffer *buffer)
  {
    OSSpinlockScopedLock resourceBindingLock(resourceBindingGuard);
    if (indexBuffer == buffer)
    {
      debug("Active index buffer <%s> was deleted...", buffer->getBufName());
      markResourceDirty(ResourceDirtyState::INDEX_BUFFER, true);
      indexBuffer = nullptr;
    }
    for (uint32_t i = 0; i < MAX_VERTEX_INPUT_STREAMS; ++i)
    {
      if (vertexBuffers[i] == buffer)
      {
        debug("Active vertex buffer <%s> (slot %u) deleted...", buffer->getBufName(), i);
        markResourceDirty(ResourceDirtyState::Bits(ResourceDirtyState::VERTEX_BUFFER_0 + i), true);
        vertexBuffers[i] = nullptr;
        vertexBufferOffsets[i] = 0;
        vertexStries[i] = 0;
      }
    }
    for (uint32_t i = 0; i < STAGE_MAX_EXT; ++i)
    {
      auto &stage = stageResources[i];
      for (uint32_t j = 0; j < dxil::MAX_B_REGISTERS; ++j)
      {
        if (stage.bRegisterBuffers[j] == buffer)
        {
          debug("Active const buffer <%s> (stage %u, slot %u) deleted...", buffer->getBufName(), i, j);
          stage.markDirtyB(j, true);
          stage.bRegisterBuffers[j] = nullptr;
          stage.bRegisterOffsets[j] = 0;
          stage.bRegisterSizes[j] = 0;
        }
      }
      for (uint32_t j = 0; j < dxil::MAX_T_REGISTERS; ++j)
      {
        if (stage.tRegisterBuffers[j] == buffer)
        {
          debug("Active texture buffer <%s> (stage %u, slot %u) deleted...", buffer->getBufName(), i, j);
          stage.markDirtyT(j, true);
          stage.tRegisterBuffers[j] = nullptr;
          stage.tRegisterTextures[j] = nullptr;
#if D3D_HAS_RAY_TRACING
          stage.tRegisterRaytraceAccelerataionStructures[j] = nullptr;
#endif
        }
      }
      for (uint32_t j = 0; j < dxil::MAX_U_REGISTERS; ++j)
      {
        if (stage.uRegisterBuffers[j] == buffer)
        {
          debug("Active storage buffer <%s> (stage %u, slot %u) deleted...", buffer->getBufName(), i, j);
          stage.markDirtyU(j, true);
          stage.uRegisterBuffers[j] = nullptr;
          stage.uRegisterTextures[j] = nullptr;
        }
      }
    }
  }

  void removeTRegisterBuffer(Sbuffer *buffer)
  {
    OSSpinlockScopedLock resourceBindingLock(resourceBindingGuard);
    for (uint32_t i = 0; i < STAGE_MAX_EXT; ++i)
    {
      auto &stage = stageResources[i];
      for (uint32_t j = 0; j < dxil::MAX_T_REGISTERS; ++j)
      {
        if (stage.tRegisterBuffers[j] == buffer)
        {
          // debug("Active texture buffer (stage %u, slot %u) bound as UAV...", i, j);
          stage.markDirtyT(j, true);
          stage.tRegisterBuffers[j] = nullptr;
          stage.tRegisterTextures[j] = nullptr;
#if D3D_HAS_RAY_TRACING
          stage.tRegisterRaytraceAccelerataionStructures[j] = nullptr;
#endif
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

  void replaceTexture(DeviceContext &ctx, const TextureReplacer &replacer)
  {
    eastl::pair<Image *, HostDeviceSharedMemoryRegion> result;
    {
      OSSpinlockScopedLock resourceBindingLock(resourceBindingGuard);
      result = replacer.update();
    }
    if (result.first)
      ctx.destroyImage(result.first);
    if (result.second)
      ctx.freeMemory(result.second);
  }

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

  void dirtySRVNoLock(BaseTex *texture, uint32_t stage, eastl::bitset<dxil::MAX_T_REGISTERS> slots)
  {
    G_UNUSED(texture); // can be used to validate
    auto &st = stageResources[stage];
    G_ASSERT(slots.any());
    auto start = slots.find_first();
    auto stop = slots.find_last();
    for (; start <= stop; ++start)
    {
      st.markDirtyT(start, slots.test(start));
    }
  }

  void dirtySRV(BaseTex *texture, uint32_t stage, eastl::bitset<dxil::MAX_T_REGISTERS> slots)
  {
    OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};
    dirtySRVNoLock(texture, stage, slots);
  }

  void dirtySampler(BaseTex *texture, uint32_t stage, eastl::bitset<dxil::MAX_T_REGISTERS> slots)
  {
    OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};
    G_UNUSED(texture); // can be used to validate
    auto &st = stageResources[stage];
    G_ASSERT(slots.any());
    auto start = slots.find_first();
    auto stop = slots.find_last();
    for (; start <= stop; ++start)
    {
      st.markDirtyS(start, slots.test(start));
    }
  }

  void dirtySRVandSamplerNoLock(BaseTex *texture, uint32_t stage, eastl::bitset<dxil::MAX_T_REGISTERS> slots)
  {
    G_UNUSED(texture); // can be used to validate
    auto &st = stageResources[stage];
    G_ASSERT(slots.any());
    auto start = slots.find_first();
    auto stop = slots.find_last();
    for (; start <= stop; ++start)
    {
      st.markDirtyT(start, slots.test(start));
      st.markDirtyS(start, slots.test(start));
    }
  }

  void dirtyUAVNoLock(BaseTex *texture, uint32_t stage, eastl::bitset<dxil::MAX_U_REGISTERS> slots)
  {
    G_UNUSED(texture); // can be used to validate
    auto &st = stageResources[stage];
    G_ASSERT(slots.any());
    auto start = slots.find_first();
    auto stop = slots.find_last();
    for (; start <= stop; ++start)
    {
      st.markDirtyU(start, slots.test(start));
    }
  }

  void dirtyRendertTargetNoLock(BaseTex *texture, eastl::bitset<Driver3dRenderTarget::MAX_SIMRT> slots)
  {
    G_UNUSED(texture); // can be used to validate
    if (slots.any() /*|| dsv*/)
      resourceDirtyState.set(ResourceDirtyState::FRAMEBUFFER, true);
    toggleBits.set(ToggleState::FORCE_SET_BACKBUFFER);
  }

  /*
    void dirtyAll(BaseTex *texture, const eastl::bitset<dxil::MAX_T_REGISTERS> *srvs,
                  const eastl::bitset<dxil::MAX_U_REGISTERS> *uavs,
                  eastl::bitset<Driver3dRenderTarget::MAX_SIMRT> rtvs, bool dsv)
    {
      G_UNUSED(texture); // can be used to validate
      OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};
      for (uint32_t s = 0; s < STAGE_MAX_EXT; ++s)
      {
        auto &st = stageResources[s];
        if (srvs[s].any())
        {
          auto start = srvs[s].find_first();
          auto stop = srvs[s].find_last();
          for (; start <= stop; ++start)
          {
            st.markDirtyT(start, srvs[s].test(start));
            st.markDirtyS(start, srvs[s].test(start));
          }
        }
        if (uavs[s].any())
        {
          auto start = uavs[s].find_first();
          auto stop = uavs[s].find_last();
          for (; start <= stop; ++start)
          {
            st.markDirtyU(start, uavs[s].test(start));
          }
        }
      }
      if (rtvs.any() || dsv)
      {
        resourceDirtyState.set(ResourceDirtyState::FRAMEBUFFER, true);
        toggleBits.set(ToggleState::FORCE_SET_BACKBUFFER);
      }
    }*/

  void notifyDelete(BaseTex *texture, const eastl::bitset<dxil::MAX_T_REGISTERS> *srvs,
    const eastl::bitset<dxil::MAX_U_REGISTERS> *uavs, eastl::bitset<Driver3dRenderTarget::MAX_SIMRT> rtvs, bool dsv)
  {
    G_UNUSED(texture); // can be used to validate
    OSSpinlockScopedLock resourceBindingLock{resourceBindingGuard};
    for (uint32_t s = 0; s < STAGE_MAX_EXT; ++s)
    {
      auto &st = stageResources[s];
      if (srvs[s].any())
      {
        auto start = srvs[s].find_first();
        auto stop = srvs[s].find_last();
        for (; start <= stop; ++start)
        {
          st.markDirtyT(start, srvs[s].test(start));
          // no need to notify S
          st.tRegisterTextures[s] = nullptr;
        }
      }
      if (uavs[s].any())
      {
        auto start = uavs[s].find_first();
        auto stop = uavs[s].find_last();
        for (; start <= stop; ++start)
        {
          st.markDirtyU(start, uavs[s].test(start));
          st.uRegisterTextures[s] = nullptr;
        }
      }
    }
    for (uint32_t i = 0; i < Driver3dRenderTarget::MAX_SIMRT; ++i)
    {
      if (renderTargets.color[i].tex == texture)
      {
        renderTargets.removeColor(i);
        resourceDirtyState.set(ResourceDirtyState::FRAMEBUFFER, true);
      }
      if (activeRenderTargets.color[i].tex == texture)
      {
        activeRenderTargets.removeColor(i);
        resourceDirtyState.set(ResourceDirtyState::FRAMEBUFFER, true);
      }
    }

    if (rtvs.any())
    {
      auto start = rtvs.find_first();
      auto stop = rtvs.find_last();
      for (; start <= stop; ++start)
      {
        if (rtvs.test(start))
        {
          renderTargets.removeColor(start);
          toggleBits.set(ToggleState::FORCE_SET_BACKBUFFER);
          resourceDirtyState.set(ResourceDirtyState::FRAMEBUFFER, true);
        }
      }
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