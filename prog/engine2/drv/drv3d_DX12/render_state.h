#pragma once

#define MINIMUM_REPRESENTABLE_D32 3e-10
#define MINIMUM_REPRESENTABLE_D24 33e-8
#define MINIMUM_REPRESENTABLE_D16 2e-5

namespace drv3d_dx12
{
BEGIN_BITFIELD_TYPE(PipelineOptionalDynamicStateMask, uint8_t)
  ADD_BITFIELD_MEMBER(hasDepthBoundsTest, 0, 1)
  ADD_BITFIELD_MEMBER(hasStencilTest, 1, 1)
  ADD_BITFIELD_MEMBER(hasBlendConstants, 2, 1)
END_BITFIELD_TYPE()
class PipelineCache;
class PipelineManager;
// Manages IDs handed out for registered shaders::RenderState data structures.
// Internally the state is split in two parts, a static state and a dynamic state.
// The static state is the portion that is baked into graphics pipelines and the
// dynamic state is state that is set during command buffer recording or is a
// software only state (eg scissorEnable - DX12 / VK have scissor always on).
//
// IDs handed out are identifiers of unique states, the system tries to be
// smart to detect duplicates and hand out IDs of compatible states
// (eg if depthTest is off, all depth test related state is ignored on compare).
//
// NOTE: for later xbox port, move all depth biases and depth clip to dynamic state
// RSSetDepthBiasX overrides compiled pipeline values.
class RenderStateSystem
{
public:
  struct StaticStateBits
  {
    // first 32 bits
    uint32_t enableDepthTest : 1;
    uint32_t enableDepthWrite : 1;
    uint32_t enableDepthClip : 1;
    uint32_t enableDepthBounds : 1;
    uint32_t enableStencil : 1;
    uint32_t enableBlending : 1;
    uint32_t enableAlphaToCoverage : 1;

    uint32_t depthFunc : 3;
    uint32_t blendSourceFactor : 4;
    uint32_t blendDestinationFactor : 4;
    uint32_t blendAlphaSourceFactor : 4;
    uint32_t blendAlphaDestinationFactor : 4;
    uint32_t blendFunction : 3;
    uint32_t blendAlphaFunction : 3;

    // second 32 bits
    uint32_t stencilReadMask : 8;
    uint32_t stencilWriteMask : 8;
    uint32_t stencilFunction : 3;
    uint32_t stencilOnFail : 3;
    uint32_t stencilOnDepthFail : 3;
    uint32_t stencilOnPass : 3;
    uint32_t forcedSampleCountShift : 3;
    uint32_t enableConservativeRaster : 1;

    // third 32 bits
    uint32_t viewInstanceCount : 2;
    uint32_t cullMode : 30;

    // fourth 32 bits
    uint32_t colorWriteMask;
  };

  struct StaticState : StaticStateBits
  {
    // fifth 32 bits
    float depthBias;
    // sixth 32 bits
    float depthBiasSloped;

    String toString() const
    {
      String result;
      result.printf(64, "{ enableDepthTest %u", enableDepthTest);
      result.aprintf(64, " enableDepthWrite %u", enableDepthWrite);
      result.aprintf(64, " enableDepthClip %u", enableDepthClip);
      result.aprintf(64, " enableDepthBounds %u", enableDepthBounds);
      result.aprintf(64, " enableStencil %u", enableStencil);
      result.aprintf(64, " enableBlending %u", enableBlending);
      result.aprintf(64, " enableAlphaToCoverage %u", enableAlphaToCoverage);

      result.aprintf(64, " depthFunc %u", depthFunc);
      result.aprintf(64, " blendSourceFactor %u", blendSourceFactor);
      result.aprintf(64, " blendDestinationFactor %u", blendDestinationFactor);
      result.aprintf(64, " blendAlphaSourceFactor %u", blendAlphaSourceFactor);
      result.aprintf(64, " blendAlphaDestinationFactor %u", blendAlphaDestinationFactor);
      result.aprintf(64, " blendFunction %u", blendFunction);
      result.aprintf(64, " blendAlphaFunction %u", blendAlphaFunction);

      result.aprintf(64, " stencilReadMask %u", stencilReadMask);
      result.aprintf(64, " stencilWriteMask %u", stencilWriteMask);
      result.aprintf(64, " stencilFunction %u", stencilFunction);
      result.aprintf(64, " stencilOnFail %u", stencilOnFail);
      result.aprintf(64, " stencilOnDepthFail %u", stencilOnDepthFail);
      result.aprintf(64, " stencilOnPass %u", stencilOnPass);
      result.aprintf(64, " forcedSampleCountShift %u", forcedSampleCountShift);
      result.aprintf(64, " enableConservativeRaster %u", enableConservativeRaster);

      result.aprintf(64, " viewInstanceCount %u", viewInstanceCount);
      result.aprintf(64, " cullMode %u", cullMode);
      result.aprintf(64, " colorWriteMask %u", colorWriteMask);
      result.aprintf(64, " depthBias %x", *reinterpret_cast<const uint32_t *>(&depthBias));
      result.aprintf(64, " depthBiasSloped %x }", *reinterpret_cast<const uint32_t *>(&depthBiasSloped));

      char buf[sizeof(*this) * 2 + 1] = {};
      data_to_str_hex_buf(buf, sizeof(buf), this, sizeof(*this));
      result.aprintf(sizeof(buf) + 4, "= %s", buf);

      return result;
    }

    friend bool operator==(const StaticState &l, const StaticState &r)
    {
      return 0 ==
               memcmp(static_cast<const StaticStateBits *>(&l), static_cast<const StaticStateBits *>(&r), sizeof(StaticStateBits)) &&
             l.depthBias == r.depthBias && l.depthBiasSloped == r.depthBiasSloped;
    }
    static StaticState fromRenderState(const shaders::RenderState &def)
    {
      StaticState result = {};
      if (def.ztest && 0 == def.forcedSampleCount)
      {
        result.enableDepthTest = 1;
        result.depthBias = def.zBias;
        result.depthBiasSloped = def.slopeZBias;
        result.depthFunc = def.zFunc - D3D12_COMPARISON_FUNC_NEVER;
      }
      else
      {
        if (def.ztest && def.forcedSampleCount)
        {
          G_ASSERT(!"When forcedSampleCount is set, then depth test has to be disabled!");
          logwarn("DX12: RenderState with forcedSamplerCount of %u but ztest was set, forcing ztest off!", def.forcedSampleCount);
        }
        result.enableDepthTest = 0;
        result.depthBias = 0.f;
        result.depthBiasSloped = 0.f;
        result.depthFunc = D3D12_COMPARISON_FUNC_ALWAYS - D3D12_COMPARISON_FUNC_NEVER;
      }

      result.enableDepthClip = def.zClip;
      result.enableDepthWrite = def.zwrite;
      result.enableDepthBounds = def.depthBoundsEnable;

      if (def.stencil.func)
      {
        result.enableStencil = 1;
        result.stencilFunction = def.stencil.func - D3D12_COMPARISON_FUNC_NEVER;
        result.stencilOnFail = def.stencil.fail - D3D12_STENCIL_OP_KEEP;
        result.stencilOnDepthFail = def.stencil.zFail - D3D12_STENCIL_OP_KEEP;
        result.stencilOnPass = def.stencil.pass - D3D12_STENCIL_OP_KEEP;
        result.stencilReadMask = def.stencil.readMask;
        result.stencilWriteMask = def.stencil.writeMask;
      }
      else
      {
        result.enableStencil = 0;
        result.stencilReadMask = 0;
        result.stencilWriteMask = 0;
        result.stencilOnFail = 0;
        result.stencilOnDepthFail = 0;
        result.stencilOnPass = 0;
        result.stencilFunction = D3D12_COMPARISON_FUNC_ALWAYS - D3D12_COMPARISON_FUNC_NEVER;
      }

      if (def.ablend)
      {
        result.enableBlending = 1;
        result.blendFunction = def.blendOp - D3D12_BLEND_OP_ADD;
        result.blendSourceFactor = def.ablendSrc - D3D12_BLEND_ZERO;
        result.blendDestinationFactor = def.ablendDst - D3D12_BLEND_ZERO;
        if (def.sepablend)
        {
          result.blendAlphaFunction = def.sepablendOp - D3D12_BLEND_OP_ADD;
          result.blendAlphaSourceFactor = def.sepablendSrc - D3D12_BLEND_ZERO;
          result.blendAlphaDestinationFactor = def.sepablendDst - D3D12_BLEND_ZERO;
        }
        else
        {
          // NOTE: color channel blend mode has its range altered from [D3D11_BLEND_ZERO, ...) to
          // [D3D11_BLEND_ZERO-D3D11_BLEND_ZERO, ...-D3D11_BLEND_ZERO) so index 0 corresponds to
          // D3D11_BLEND_ZERO
          static const uint32_t colorToAlphaChannelMap[] = //
            {
              D3D12_BLEND_ZERO, D3D12_BLEND_ONE,
              D3D12_BLEND_SRC_ALPHA,     // D3D11_BLEND_SRC_COLOR   = 3,
              D3D12_BLEND_INV_SRC_ALPHA, // D3D11_BLEND_INV_SRC_COLOR   = 4,
              D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_DEST_ALPHA, D3D12_BLEND_INV_DEST_ALPHA,
              D3D12_BLEND_DEST_ALPHA,     // D3D11_BLEND_DEST_COLOR  = 9,
              D3D12_BLEND_INV_DEST_ALPHA, // D3D11_BLEND_INV_DEST_COLOR  = 10,
              D3D12_BLEND_SRC_ALPHA_SAT, D3D12_BLEND_ZERO, D3D12_BLEND_ZERO, D3D12_BLEND_BLEND_FACTOR, D3D12_BLEND_INV_BLEND_FACTOR,
              // Those blend modes are unreachable, because shaders::RenderState::ablendSrc has only 4 bits and caps out at 15
              // D3D12_BLEND_SRC1_ALPHA,     // D3D11_BLEND_SRC1_COLOR,
              // D3D12_BLEND_INV_SRC1_ALPHA, // D3D11_BLEND_INV_SRC1_COLOR ,
              // D3D12_BLEND_SRC1_ALPHA,
              // D3D12_BLEND_INV_SRC1_ALPHA // 19
            };
          result.blendAlphaFunction = result.blendFunction;
          result.blendAlphaSourceFactor = colorToAlphaChannelMap[result.blendSourceFactor] - D3D12_BLEND_ZERO;
          result.blendAlphaDestinationFactor = colorToAlphaChannelMap[result.blendDestinationFactor] - D3D12_BLEND_ZERO;
        }
      }
      else
      {
        result.enableBlending = 0;
        result.blendFunction = D3D12_BLEND_OP_ADD - D3D12_BLEND_OP_ADD;
        result.blendSourceFactor = D3D12_BLEND_ONE - D3D12_BLEND_ZERO;
        result.blendDestinationFactor = D3D12_BLEND_ZERO - D3D12_BLEND_ZERO;
        result.blendAlphaFunction = D3D12_BLEND_OP_ADD - D3D12_BLEND_OP_ADD;
        result.blendAlphaSourceFactor = D3D12_BLEND_ONE - D3D12_BLEND_ZERO;
        result.blendAlphaDestinationFactor = D3D12_BLEND_ZERO - D3D12_BLEND_ZERO;
      }

      result.viewInstanceCount = def.viewInstanceCount;
      G_ASSERT(result.viewInstanceCount == 0 || d3d::get_driver_desc().caps.hasBasicViewInstancing);

      result.enableConservativeRaster = def.conservativeRaster;
      result.enableAlphaToCoverage = def.alphaToCoverage;
      result.cullMode = def.cull;

      // decodes back to count by (1u << result.forcedSampleCountShift) >> 1u;
      // this encoding saves one bit
      result.forcedSampleCountShift = 0;
      result.forcedSampleCountShift += def.forcedSampleCount > 0 ? 1 : 0;
      result.forcedSampleCountShift += def.forcedSampleCount > 1 ? 1 : 0;
      result.forcedSampleCountShift += def.forcedSampleCount > 2 ? 1 : 0;
      result.forcedSampleCountShift += def.forcedSampleCount > 4 ? 1 : 0;
      result.forcedSampleCountShift += def.forcedSampleCount > 8 ? 1 : 0;

      result.colorWriteMask = def.colorWr;

      return result;
    }

    static bool has_uniform_color_mask(uint32_t mask)
    {
      // checks if all sets of 4 bits are equal
      return 0 == (((mask ^ (mask >> 16)) & 0xFFFF) | ((mask ^ (mask >> 8)) & 0xFF) | ((mask ^ (mask >> 4)) & 0xF));
    }

    uint32_t adjustColorTargetMask(uint32_t frame_buffer_render_target_mask) const
    {
      return colorWriteMask & frame_buffer_render_target_mask;
    }

    uint32_t calculateMissingShaderOutputMask(uint32_t frame_buffer_render_target_mask, uint32_t pipeline_output_mask) const
    {
      auto finalColorTargetMask = adjustColorTargetMask(frame_buffer_render_target_mask);
      return finalColorTargetMask ^ (finalColorTargetMask & pipeline_output_mask);
    }

    D3D12_BLEND_DESC getBlendDesc(uint32_t frame_buffer_render_target_mask) const
    {
      auto finalColorTargetMask = adjustColorTargetMask(frame_buffer_render_target_mask);
      D3D12_BLEND_DESC result = {};

      result.AlphaToCoverageEnable = enableAlphaToCoverage;
      // only need independent blend for write mask
      result.IndependentBlendEnable = !has_uniform_color_mask(finalColorTargetMask);
      result.RenderTarget[0].RenderTargetWriteMask = finalColorTargetMask & 15;
      result.RenderTarget[0].BlendEnable = enableBlending;
      result.RenderTarget[0].LogicOpEnable = FALSE;
      result.RenderTarget[0].SrcBlend = static_cast<D3D12_BLEND>(D3D12_BLEND_ZERO + blendSourceFactor);
      result.RenderTarget[0].DestBlend = static_cast<D3D12_BLEND>(D3D12_BLEND_ZERO + blendDestinationFactor);
      result.RenderTarget[0].BlendOp = static_cast<D3D12_BLEND_OP>(D3D12_BLEND_OP_ADD + blendFunction);
      result.RenderTarget[0].SrcBlendAlpha = static_cast<D3D12_BLEND>(D3D12_BLEND_ZERO + blendAlphaSourceFactor);
      result.RenderTarget[0].DestBlendAlpha = static_cast<D3D12_BLEND>(D3D12_BLEND_ZERO + blendAlphaDestinationFactor);
      result.RenderTarget[0].BlendOpAlpha = static_cast<D3D12_BLEND_OP>(D3D12_BLEND_OP_ADD + blendAlphaFunction);
      result.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;

      if (result.IndependentBlendEnable)
      {
        for (uint32_t i = 1; i < 8; ++i)
        {
          finalColorTargetMask >>= 4;
          result.RenderTarget[i] = result.RenderTarget[0];
          result.RenderTarget[i].RenderTargetWriteMask = finalColorTargetMask & 15;
        }
      }

      return result;
    }

    D3D12_RASTERIZER_DESC getRasterizerDesc(D3D12_FILL_MODE fill_mode) const
    {
      D3D12_RASTERIZER_DESC result = {};
      result.FillMode = fill_mode;
      if (cullMode == CULL_NONE)
        result.CullMode = D3D12_CULL_MODE_NONE;
      else if (cullMode == CULL_CCW)
        result.CullMode = D3D12_CULL_MODE_BACK;
      else
        result.CullMode = D3D12_CULL_MODE_FRONT;
      result.FrontCounterClockwise = FALSE;
      result.DepthBias = depthBias / MINIMUM_REPRESENTABLE_D16;
      result.DepthBiasClamp = 0.f;
      result.SlopeScaledDepthBias = depthBiasSloped;
      result.DepthClipEnable = enableDepthClip;
      result.MultisampleEnable = FALSE;
      result.AntialiasedLineEnable = FALSE;
      result.ForcedSampleCount = (1u << forcedSampleCountShift) >> 1u;
      result.ConservativeRaster =
        enableConservativeRaster ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

      // The only valid Fill Mode for Conservative Rasterization is D3D11_FILL_SOLID,
      // any other Fill Mode is an invalid parameter for the Rasterizer State.
      // https://microsoft.github.io/DirectX-Specs/d3d/ConservativeRasterization.html#fill-modes-interaction
      if (fill_mode == D3D12_FILL_MODE_WIREFRAME)
        result.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

      return result;
    }

    D3D12_DEPTH_STENCIL_DESC getDepthStencilDesc() const
    {
      D3D12_DEPTH_STENCIL_DESC result = {};
      result.DepthEnable = enableDepthTest;
      result.DepthWriteMask = enableDepthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
      result.DepthFunc = static_cast<D3D12_COMPARISON_FUNC>(D3D12_COMPARISON_FUNC_NEVER + depthFunc);
      result.StencilEnable = enableStencil;
      result.StencilReadMask = stencilReadMask;
      result.StencilWriteMask = stencilWriteMask;
      result.FrontFace.StencilFailOp = static_cast<D3D12_STENCIL_OP>(D3D12_STENCIL_OP_KEEP + stencilOnFail);
      result.FrontFace.StencilDepthFailOp = static_cast<D3D12_STENCIL_OP>(D3D12_STENCIL_OP_KEEP + stencilOnDepthFail);
      result.FrontFace.StencilPassOp = static_cast<D3D12_STENCIL_OP>(D3D12_STENCIL_OP_KEEP + stencilOnPass);
      result.FrontFace.StencilFunc = static_cast<D3D12_COMPARISON_FUNC>(D3D12_COMPARISON_FUNC_NEVER + stencilFunction);
      result.BackFace = result.FrontFace;
      return result;
    }

    bool needsViewInstancing() const { return viewInstanceCount > 0; }

    D3D12_VIEW_INSTANCING_DESC getViewInstancingDesc() const
    {
      // Right now, our render target support is limited, so here we doesn't support rendering to different array slices.
      // We also support mapping a specific view index, to the same viewport.
      // In fact, these conditions make more likely to a GPU to do instancing more efficiently.

      static const D3D12_VIEW_INSTANCE_LOCATION viewInstanceLocations[4] = {{0, 0}, {1, 0}, {2, 0}, {3, 0}};

      // We doesn't support the instancing mask. Our intended use with this doesn't need it.

      D3D12_VIEW_INSTANCING_DESC result;
      result.ViewInstanceCount = viewInstanceCount + 1; // zero based
      result.pViewInstanceLocations = viewInstanceLocations;
      result.Flags = D3D12_VIEW_INSTANCING_FLAG_NONE;
      return result;
    }

    D3D12_DEPTH_STENCIL_DESC1 getDepthStencilDesc1() const
    {
      D3D12_DEPTH_STENCIL_DESC1 result = {};
      result.DepthEnable = enableDepthTest;
      result.DepthWriteMask = enableDepthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
      result.DepthFunc = static_cast<D3D12_COMPARISON_FUNC>(D3D12_COMPARISON_FUNC_NEVER + depthFunc);
      result.StencilEnable = enableStencil;
      result.StencilReadMask = stencilReadMask;
      result.StencilWriteMask = stencilWriteMask;
      result.FrontFace.StencilFailOp = static_cast<D3D12_STENCIL_OP>(D3D12_STENCIL_OP_KEEP + stencilOnFail);
      result.FrontFace.StencilDepthFailOp = static_cast<D3D12_STENCIL_OP>(D3D12_STENCIL_OP_KEEP + stencilOnDepthFail);
      result.FrontFace.StencilPassOp = static_cast<D3D12_STENCIL_OP>(D3D12_STENCIL_OP_KEEP + stencilOnPass);
      result.FrontFace.StencilFunc = static_cast<D3D12_COMPARISON_FUNC>(D3D12_COMPARISON_FUNC_NEVER + stencilFunction);
      result.BackFace = result.FrontFace;
      result.DepthBoundsTestEnable = enableDepthBounds;
      return result;
    }

    PipelineOptionalDynamicStateMask getDynamicStateMask() const
    {
      PipelineOptionalDynamicStateMask mask = {};
      mask.hasDepthBoundsTest = enableDepthBounds;
      mask.hasStencilTest = enableStencil;
      auto srcBlend = static_cast<D3D12_BLEND>(D3D12_BLEND_ZERO + blendSourceFactor);
      auto dstBlend = static_cast<D3D12_BLEND>(D3D12_BLEND_ZERO + blendSourceFactor);
      auto srcABlend = static_cast<D3D12_BLEND>(D3D12_BLEND_ZERO + blendAlphaSourceFactor);
      auto dstABlend = static_cast<D3D12_BLEND>(D3D12_BLEND_ZERO + blendAlphaDestinationFactor);
      mask.hasBlendConstants = (D3D12_BLEND_BLEND_FACTOR == srcBlend) || (D3D12_BLEND_INV_BLEND_FACTOR == srcBlend) ||
                               (D3D12_BLEND_BLEND_FACTOR == dstBlend) || (D3D12_BLEND_INV_BLEND_FACTOR == dstBlend) ||
                               (D3D12_BLEND_BLEND_FACTOR == srcABlend) || (D3D12_BLEND_INV_BLEND_FACTOR == srcABlend) ||
                               (D3D12_BLEND_BLEND_FACTOR == dstABlend) || (D3D12_BLEND_INV_BLEND_FACTOR == dstABlend);
      return mask;
    }
  };
  struct DynamicState
  {
    uint32_t stencilRef : 8;
    uint32_t enableScissor : 1;
  };

  void reset()
  {
    OSSpinlockScopedLock lock(mutex);
    publicStateTable.clear();
    staticStateTable.clear();
  }
  void loadFromCache(PipelineCache &cache, PipelineManager &pipe_man);

  struct PublicStateBasicInfo
  {
    DynamicState dynamicState;
    StaticRenderStateID staticRenderStateID;
  };

  using PublicStateInfo = PublicStateBasicInfo;

  uint32_t registerState(DeviceContext &ctx, const shaders::RenderState &def)
  {
    OSSpinlockScopedLock lock(mutex);
    auto ref = eastl::find_if(begin(publicStateTable), end(publicStateTable),
      [&def](auto &&e) { return renderStateCompare(eastl::get<0>(e), def); });
    if (ref == end(publicStateTable))
    {
      auto staticStateId = registerStaticState(ctx, def);
      auto dynamicState = getDynamicStateFromState(def);
      ref = publicStateTable.insert(end(publicStateTable), eastl::pair(def, PublicStateInfo{dynamicState, staticStateId}));
    }
    return static_cast<uint32_t>(ref - begin(publicStateTable));
  }

  PublicStateInfo getDynamicAndStaticState(uint32_t public_id)
  {
    OSSpinlockScopedLock lock(mutex);
    auto &e = publicStateTable[public_id];
    return e.second;
  }

private:
  // 'smart' compare of two RenderStates, it skips modes for functionality that is turned off
  static bool renderStateCompare(const shaders::RenderState &l, const shaders::RenderState &r)
  {
#define MEMBER_COMPARE(name) \
  if (l.name != r.name)      \
  return false
    MEMBER_COMPARE(ztest);
    MEMBER_COMPARE(zwrite);
    MEMBER_COMPARE(depthBoundsEnable);
    MEMBER_COMPARE(stencil.func);
    MEMBER_COMPARE(ablend);
    MEMBER_COMPARE(sepablend);
    MEMBER_COMPARE(conservativeRaster);
    MEMBER_COMPARE(alphaToCoverage);
    MEMBER_COMPARE(cull);
    MEMBER_COMPARE(forcedSampleCount);
    MEMBER_COMPARE(scissorEnabled);
    MEMBER_COMPARE(colorWr);
    MEMBER_COMPARE(zClip);
    MEMBER_COMPARE(viewInstanceCount);

    if (l.ztest)
    {
      MEMBER_COMPARE(zFunc);
      MEMBER_COMPARE(zBias);
      MEMBER_COMPARE(slopeZBias);
    }

    if (l.stencil.func)
    {
      MEMBER_COMPARE(stencilRef);
      MEMBER_COMPARE(stencil.fail);
      MEMBER_COMPARE(stencil.zFail);
      MEMBER_COMPARE(stencil.pass);
      MEMBER_COMPARE(stencil.readMask);
      MEMBER_COMPARE(stencil.writeMask);
    }

    if (l.ablend)
    {
      MEMBER_COMPARE(blendOp);
      MEMBER_COMPARE(ablendSrc);
      MEMBER_COMPARE(ablendDst);
      if (l.sepablend)
      {
        MEMBER_COMPARE(sepablendOp);
        MEMBER_COMPARE(sepablendSrc);
        MEMBER_COMPARE(sepablendDst);
      }
    }

#undef MEMBER_COMPARE
    return true;
  }
  static DynamicState getDynamicStateFromState(const shaders::RenderState &def)
  {
    DynamicState result;

    // also a function to overwrite this in d3d interface...
    result.stencilRef = def.stencilRef;
    result.enableScissor = 0 != def.scissorEnabled;

    return result;
  }
  StaticRenderStateID registerStaticState(DeviceContext &ctx, const shaders::RenderState &def);
  OSSpinlock mutex;
  eastl::vector<StaticState> staticStateTable;
  eastl::vector<eastl::pair<shaders::RenderState, PublicStateInfo>> publicStateTable;
};
} // namespace drv3d_dx12