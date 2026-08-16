// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "bitfield.h"
#include "driver.h"
#include "dynamic_array.h"
#include "tagged_handles.h"
#include "format_store.h"

#include <drv/3d/dag_decl.h>
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_renderStates.h>
#include <drv/shadersMetaData/dxil/compiled_shader_header.h>
#include <EASTL/bit.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_spinlock.h>
#include <util/dag_string.h>
#include <util/dag_strUtil.h>


#define MINIMUM_REPRESENTABLE_D32 3e-10
#define MINIMUM_REPRESENTABLE_D24 33e-8
#define MINIMUM_REPRESENTABLE_D16 2e-5

namespace drv3d_dx12
{
struct StaticRenderStateIDWithHash
{
  StaticRenderStateID id;
  dxil::HashValue hash;
};

class DeviceContext;

struct PipelineOptionalDynamicStateMask
{
  bool hasDepthBoundsTest : 1 = false;
  bool hasStencilTest : 1 = false;
  bool hasBlendConstants : 1 = false;
};

class PipelineCache;
class PipelineManager;

// At namespace scope because getViewInstancingDesc hands out a pointer to it, so it has to
// outlive the call and be one entity across translation units.
inline constexpr D3D12_VIEW_INSTANCE_LOCATION VIEW_INSTANCE_LOCATIONS[4] = {{0, 0}, {1, 0}, {2, 0}, {3, 0}};

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
  struct DynamicState
  {
    uint32_t stencilRef : 8 = 0;
    uint32_t enableScissor : 1 = 0;
  };

  struct StaticStateBits
  {
    // first 32 bits
    uint32_t enableDepthTest : 1 = 0;
    uint32_t enableDepthWrite : 1 = 0;
    uint32_t enableDepthClip : 1 = 0;
    uint32_t enableDepthBounds : 1 = 0;
    uint32_t enableStencil : 1 = 0;
    uint32_t enableIndependentBlend : 1 = 0;
    uint32_t enableAlphaToCoverage : 1 = 0;

    uint32_t depthFunc : 3 = 0;
    uint32_t forcedSampleCountShift : 3 = 0;
    uint32_t enableConservativeRaster : 1 = 0;
    uint32_t viewInstanceCount : 2 = 0;
    uint32_t cullMode : 15 = 0; // 2 bit used, 13 free
    uint32_t enableDualSourceBlending : 1 = 0;

    // second 32 bits
    uint32_t stencilReadMask : 8 = 0;
    uint32_t stencilWriteMask : 8 = 0;
    uint32_t stencilFunction : 3 = 0;
    uint32_t stencilOnFail : 3 = 0;
    uint32_t stencilOnDepthFail : 3 = 0;
    uint32_t stencilOnPass : 7 = 0; // 3 bit used, 4 free

    // third 32 bits
    uint32_t colorWriteMask = 0;

    bool operator==(const StaticStateBits &) const = default;
  };

  // Struct should not have alignment intervals,
  // because we calculate hash for whole structure
  G_STATIC_ASSERT(sizeof(StaticStateBits) == 3 * sizeof(uint32_t));

  struct StaticState : StaticStateBits
  {
    // fourth 32 bits
    float depthBias = 0.0;
    // fifth 32 bits
    float depthBiasSloped = 0.0;

    struct BlendFactors
    {
      uint8_t Source : 4;
      uint8_t Destination : 4;
    };
    struct ExtendedBlendFactors
    {
      uint8_t Source;
      uint8_t Destination;
    };
    struct BlendParams
    {
      BlendFactors blendFactors;
      BlendFactors blendAlphaFactors;
      uint8_t blendFunction : 3;
      uint8_t blendAlphaFunction : 3;
      uint8_t enableBlending : 2; // 1 bit used, 1 free
    };
    struct ExtendedBlendParams
    {
      ExtendedBlendFactors blendFactors;
      ExtendedBlendFactors blendAlphaFactors;
      uint8_t blendFunction : 3;
      uint8_t blendAlphaFunction : 3;
      uint8_t enableBlending : 2;
    };

    // 3 * 32 bits (4 * 3 * 8 bits)
    union
    {
      BlendParams blendParams[shaders::RenderState::NumIndependentBlendParameters]{};
      struct
      {
        ExtendedBlendParams params;
        uint8_t pad_[sizeof(blendParams) - sizeof(params)];
      } dualSourceBlend;
    };

    static_assert(sizeof(BlendParams) == 3);
    static_assert(sizeof(ExtendedBlendParams) == 5);
    static_assert(sizeof(blendParams) == shaders::RenderState::NumIndependentBlendParameters * sizeof(BlendParams));
    static_assert(sizeof(dualSourceBlend) == sizeof(blendParams));

    String toString() const
    {
      String result;

      result.aprintf(64, " enableDepthTest %u", enableDepthTest);
      result.aprintf(64, " enableDepthWrite %u", enableDepthWrite);
      result.aprintf(64, " enableDepthClip %u", enableDepthClip);
      result.aprintf(64, " enableDepthBounds %u", enableDepthBounds);
      result.aprintf(64, " enableStencil %u", enableStencil);
      result.aprintf(64, " enableIndependentBlend %u", enableIndependentBlend);
      result.aprintf(64, " enableAlphaToCoverage %u", enableAlphaToCoverage);
      result.aprintf(64, " enableDualSourceBlending %u", enableDualSourceBlending);

      result.aprintf(64, " depthFunc %u", depthFunc);
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
      result.aprintf(64, " depthBias %x", eastl::bit_cast<uint32_t>(depthBias));
      result.aprintf(64, " depthBiasSloped %x", eastl::bit_cast<uint32_t>(depthBiasSloped));

      auto appendParams = [&result](const auto &param, int i) {
        result.aprintf(64, " blendParams[%d].enableBlending %u", i, param.enableBlending);
        result.aprintf(64, " blendParams[%d].blendFactors.Source %u", i, param.blendFactors.Source);
        result.aprintf(64, " blendParams[%d].blendFactors.Destination %u", i, param.blendFactors.Destination);
        result.aprintf(64, " blendParams[%d].blendAlphaFactors.Source %u", i, param.blendAlphaFactors.Source);
        result.aprintf(64, " blendParams[%d].blendAlphaFactors.Destination %u", i, param.blendAlphaFactors.Destination);
        result.aprintf(64, " blendParams[%d].blendFunction %u", i, param.blendFunction);
        result.aprintf(64, " blendParams[%d].blendAlphaFunction %u", i, param.blendAlphaFunction);
      };

      if (enableDualSourceBlending)
      {
        appendParams(dualSourceBlend.params, 0);
      }
      else
      {
        for (uint32_t i = 0; i < shaders::RenderState::NumIndependentBlendParameters; i++)
          appendParams(blendParams[i], i);
      }

      char buf[sizeof(*this) * 2 + 1] = {};
      data_to_str_hex_buf(buf, sizeof(buf), this, sizeof(*this));
      result.aprintf(sizeof(buf) + 4, "= %s", buf);

      return result;
    }

    // Unused blendParams entries are not normalized on creation, so only the
    // bytes selected by the blend mode take part in the comparison.
    size_t usedBlendParamsSize() const
    {
      if (enableIndependentBlend)
        return sizeof(blendParams);
      return enableDualSourceBlending ? sizeof(ExtendedBlendParams) : sizeof(BlendParams);
    }

    bool operator==(const StaticState &other) const
    {
      return static_cast<const StaticStateBits &>(*this) == static_cast<const StaticStateBits &>(other) &&
             depthBias == other.depthBias && depthBiasSloped == other.depthBiasSloped &&
             0 == memcmp(blendParams, other.blendParams, usedBlendParamsSize());
    }

    uint32_t distance(const StaticState &other) const
    {
      uint32_t d = 0;
#define CMP(name)         \
  if (name != other.name) \
  {                       \
    ++d;                  \
  }
      CMP(enableDepthTest);
      CMP(enableDepthWrite);
      CMP(enableDepthClip);
      CMP(enableDepthBounds);
      CMP(enableStencil);
      CMP(enableIndependentBlend);
      CMP(enableAlphaToCoverage);
      CMP(enableDualSourceBlending);

      CMP(depthFunc);
      CMP(forcedSampleCountShift);
      CMP(enableConservativeRaster);
      CMP(viewInstanceCount);
      CMP(cullMode);

      CMP(stencilReadMask);
      CMP(stencilWriteMask);
      CMP(stencilFunction);
      CMP(stencilOnFail);
      CMP(stencilOnDepthFail);
      CMP(stencilOnPass);

      CMP(colorWriteMask);

      CMP(depthBias);
      CMP(depthBiasSloped);

#define CMP_BLEND_PARAMS(params_)               \
  do                                            \
  {                                             \
    CMP(params_.blendFactors.Source);           \
    CMP(params_.blendFactors.Destination);      \
    CMP(params_.blendAlphaFactors.Source);      \
    CMP(params_.blendAlphaFactors.Destination); \
    CMP(params_.blendFunction);                 \
    CMP(params_.blendAlphaFunction);            \
    CMP(params_.enableBlending);                \
  } while (0)

      if (enableDualSourceBlending)
        CMP_BLEND_PARAMS(dualSourceBlend.params);
      else
      {
        for (uint32_t i = 0; i < countof(blendParams); ++i)
          CMP_BLEND_PARAMS(blendParams[i]);
      }
#undef CMP_BLEND_PARAMS
#undef CMP
      return d;
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
        result.stencilFunction = D3D12_COMPARISON_FUNC_ALWAYS - D3D12_COMPARISON_FUNC_NEVER;
      }

      result.enableIndependentBlend = def.independentBlendEnabled;
      result.enableDualSourceBlending = def.dualSourceBlendEnabled;

      auto fillBlendState = [](auto &dst, const auto &src) {
        if (src.ablend)
        {
          dst.enableBlending = 1;
          dst.blendFunction = src.blendOp - D3D12_BLEND_OP_ADD;
          dst.blendFactors.Source = src.ablendFactors.src - D3D12_BLEND_ZERO;
          dst.blendFactors.Destination = src.ablendFactors.dst - D3D12_BLEND_ZERO;
          if (src.sepablend)
          {
            dst.blendAlphaFunction = src.sepablendOp - D3D12_BLEND_OP_ADD;
            dst.blendAlphaFactors.Source = src.sepablendFactors.src - D3D12_BLEND_ZERO;
            dst.blendAlphaFactors.Destination = src.sepablendFactors.dst - D3D12_BLEND_ZERO;
          }
          else
          {
            // NOTE: color channel blend mode has its range altered from [D3D11_BLEND_ZERO, ...) to
            // [D3D11_BLEND_ZERO-D3D11_BLEND_ZERO, ...-D3D11_BLEND_ZERO) so index 0 corresponds to
            // D3D11_BLEND_ZERO
            constexpr uint32_t colorToAlphaChannelMap[] = //
              {
                D3D12_BLEND_ZERO, D3D12_BLEND_ONE,
                D3D12_BLEND_SRC_ALPHA,     // D3D11_BLEND_SRC_COLOR   = 3,
                D3D12_BLEND_INV_SRC_ALPHA, // D3D11_BLEND_INV_SRC_COLOR   = 4,
                D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_DEST_ALPHA, D3D12_BLEND_INV_DEST_ALPHA,
                D3D12_BLEND_DEST_ALPHA,     // D3D11_BLEND_DEST_COLOR  = 9,
                D3D12_BLEND_INV_DEST_ALPHA, // D3D11_BLEND_INV_DEST_COLOR  = 10,
                D3D12_BLEND_SRC_ALPHA_SAT, D3D12_BLEND_ZERO, D3D12_BLEND_ZERO, D3D12_BLEND_BLEND_FACTOR, D3D12_BLEND_INV_BLEND_FACTOR,
                D3D12_BLEND_SRC1_ALPHA,     // D3D11_BLEND_SRC1_COLOR,
                D3D12_BLEND_INV_SRC1_ALPHA, // D3D11_BLEND_INV_SRC1_COLOR ,
                D3D12_BLEND_SRC1_ALPHA,
                D3D12_BLEND_INV_SRC1_ALPHA // 19
              };
            dst.blendAlphaFunction = dst.blendFunction;
            dst.blendAlphaFactors.Source = colorToAlphaChannelMap[dst.blendFactors.Source] - D3D12_BLEND_ZERO;
            dst.blendAlphaFactors.Destination = colorToAlphaChannelMap[dst.blendFactors.Destination] - D3D12_BLEND_ZERO;
          }
        }
        else
        {
          dst.blendFunction = D3D12_BLEND_OP_ADD - D3D12_BLEND_OP_ADD;
          dst.blendFactors.Source = D3D12_BLEND_ONE - D3D12_BLEND_ZERO;
          dst.blendFactors.Destination = D3D12_BLEND_ZERO - D3D12_BLEND_ZERO;
          dst.blendAlphaFunction = D3D12_BLEND_OP_ADD - D3D12_BLEND_OP_ADD;
          dst.blendAlphaFactors.Source = D3D12_BLEND_ONE - D3D12_BLEND_ZERO;
          dst.blendAlphaFactors.Destination = D3D12_BLEND_ZERO - D3D12_BLEND_ZERO;
        }
      };

      if (def.dualSourceBlendEnabled)
        fillBlendState(result.dualSourceBlend.params, def.dualSourceBlend.params);
      else
      {
        for (uint32_t i = 0; i < shaders::RenderState::NumIndependentBlendParameters; ++i)
          fillBlendState(result.blendParams[i], def.blendParams[i]);
      }

      result.viewInstanceCount = def.viewInstanceCount;

      result.enableConservativeRaster = def.conservativeRaster;
      result.enableAlphaToCoverage = def.alphaToCoverage;
      result.cullMode = def.cull;

      // decodes back to count by (1u << result.forcedSampleCountShift) >> 1u;
      // this encoding saves one bit
      result.forcedSampleCountShift += def.forcedSampleCount > 0 ? 1 : 0;
      result.forcedSampleCountShift += def.forcedSampleCount > 1 ? 1 : 0;
      result.forcedSampleCountShift += def.forcedSampleCount > 2 ? 1 : 0;
      result.forcedSampleCountShift += def.forcedSampleCount > 4 ? 1 : 0;
      result.forcedSampleCountShift += def.forcedSampleCount > 8 ? 1 : 0;

      result.colorWriteMask = def.colorWr;

      return result;
    }

    static constexpr bool has_uniform_color_mask(uint32_t mask)
    {
      // checks if all sets of 4 bits are equal
      return 0 == (((mask ^ (mask >> 16)) & 0xFFFF) | ((mask ^ (mask >> 8)) & 0xFF) | ((mask ^ (mask >> 4)) & 0xF));
    }

    uint32_t adjustColorTargetMask(uint32_t frame_buffer_render_target_mask) const
    {
      return colorWriteMask & frame_buffer_render_target_mask;
    }

    uint32_t calculateMissingShaderOutputMask(uint32_t frame_buffer_render_target_mask, eastl::span<const FormatStore> rt_formats,
      uint32_t pipeline_output_mask) const
    {
      auto finalColorTargetMask = adjustColorTargetMask(frame_buffer_render_target_mask);

      // https://microsoft.github.io/DirectX-Specs/d3d/D3D12R9G9B9E5Format.html#interaction-with-rendertargetwritemask
      uint32_t alphaMask = 0x8;
      const auto r9g9b9e5Store = FormatStore::fromDXGIFormat(DXGI_FORMAT_R9G9B9E5_SHAREDEXP);
      for (const auto format : rt_formats)
      {
        if (format == r9g9b9e5Store)
          finalColorTargetMask &= ~alphaMask;
        alphaMask <<= 4;
      }

      return finalColorTargetMask ^ (finalColorTargetMask & pipeline_output_mask);
    }

    D3D12_BLEND_DESC getBlendDesc(uint32_t frame_buffer_render_target_mask) const
    {
      auto finalColorTargetMask = adjustColorTargetMask(frame_buffer_render_target_mask);
      D3D12_BLEND_DESC result = {
        .AlphaToCoverageEnable = 0 != enableAlphaToCoverage,
        // dual source blending requires blending to be enabled on render target 0 only,
        // so it must never use the independent blend path that replicates its params
        .IndependentBlendEnable =
          !enableDualSourceBlending && (!has_uniform_color_mask(finalColorTargetMask) || enableIndependentBlend),
      };
      const auto RTCount = result.IndependentBlendEnable ? Driver3dRenderTarget::MAX_SIMRT : 1;

      auto fillRtBlendDesc = [&finalColorTargetMask](D3D12_RENDER_TARGET_BLEND_DESC &dst, const auto &src) {
        dst = {
          .BlendEnable = 0 != src.enableBlending,
          .LogicOpEnable = FALSE,
          .SrcBlend = static_cast<D3D12_BLEND>(D3D12_BLEND_ZERO + src.blendFactors.Source),
          .DestBlend = static_cast<D3D12_BLEND>(D3D12_BLEND_ZERO + src.blendFactors.Destination),
          .BlendOp = static_cast<D3D12_BLEND_OP>(D3D12_BLEND_OP_ADD + src.blendFunction),
          .SrcBlendAlpha = static_cast<D3D12_BLEND>(D3D12_BLEND_ZERO + src.blendAlphaFactors.Source),
          .DestBlendAlpha = static_cast<D3D12_BLEND>(D3D12_BLEND_ZERO + src.blendAlphaFactors.Destination),
          .BlendOpAlpha = static_cast<D3D12_BLEND_OP>(D3D12_BLEND_OP_ADD + src.blendAlphaFunction),
          .LogicOp = D3D12_LOGIC_OP_NOOP,
          .RenderTargetWriteMask = static_cast<UINT8>(finalColorTargetMask & 15),
        };
      };

      for (uint32_t i = 0; i < RTCount; ++i)
      {
        const auto blendParamsId = i < shaders::RenderState::NumIndependentBlendParameters && enableIndependentBlend ? i : 0;

        if (enableDualSourceBlending)
          fillRtBlendDesc(result.RenderTarget[i], dualSourceBlend.params);
        else
          fillRtBlendDesc(result.RenderTarget[i], blendParams[blendParamsId]);

        finalColorTargetMask >>= 4;
      }

      return result;
    }

    D3D12_RASTERIZER_DESC getRasterizerDesc(D3D12_FILL_MODE fill_mode) const
    {
      // The only valid Fill Mode for Conservative Rasterization is D3D11_FILL_SOLID,
      // any other Fill Mode is an invalid parameter for the Rasterizer State.
      // https://microsoft.github.io/DirectX-Specs/d3d/ConservativeRasterization.html#fill-modes-interaction
      const bool conservativeRaster = enableConservativeRaster && fill_mode != D3D12_FILL_MODE_WIREFRAME;
      return {
        .FillMode = fill_mode,
        .CullMode = cullMode == CULL_NONE  ? D3D12_CULL_MODE_NONE
                    : cullMode == CULL_CCW ? D3D12_CULL_MODE_BACK
                                           : D3D12_CULL_MODE_FRONT,
        .FrontCounterClockwise = FALSE,
        .DepthBias = static_cast<INT>(depthBias / MINIMUM_REPRESENTABLE_D16),
        .DepthBiasClamp = 0.f,
        .SlopeScaledDepthBias = depthBiasSloped,
        .DepthClipEnable = 0 != enableDepthClip,
        .MultisampleEnable = FALSE,
        .AntialiasedLineEnable = FALSE,
        .ForcedSampleCount = (1u << forcedSampleCountShift) >> 1u,
        .ConservativeRaster =
          conservativeRaster ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
      };
    }

    D3D12_DEPTH_STENCIL_DESC getDepthStencilDesc() const
    {
      const auto desc = getDepthStencilDesc1();
      return {
        .DepthEnable = desc.DepthEnable,
        .DepthWriteMask = desc.DepthWriteMask,
        .DepthFunc = desc.DepthFunc,
        .StencilEnable = desc.StencilEnable,
        .StencilReadMask = desc.StencilReadMask,
        .StencilWriteMask = desc.StencilWriteMask,
        .FrontFace = desc.FrontFace,
        .BackFace = desc.BackFace,
      };
    }

    bool needsViewInstancing() const { return viewInstanceCount > 0; }

    D3D12_VIEW_INSTANCING_DESC getViewInstancingDesc() const
    {
      // Right now, our render target support is limited, so here we doesn't support rendering to different array slices.
      // We also support mapping a specific view index, to the same viewport.
      // In fact, these conditions make more likely to a GPU to do instancing more efficiently.

      // We doesn't support the instancing mask. Our intended use with this doesn't need it.

      return {
        .ViewInstanceCount = viewInstanceCount + 1u, // zero based
        .pViewInstanceLocations = VIEW_INSTANCE_LOCATIONS,
        .Flags = D3D12_VIEW_INSTANCING_FLAG_NONE,
      };
    }

    D3D12_DEPTH_STENCIL_DESC1 getDepthStencilDesc1() const
    {
      const D3D12_DEPTH_STENCILOP_DESC stencilOp = {
        .StencilFailOp = static_cast<D3D12_STENCIL_OP>(D3D12_STENCIL_OP_KEEP + stencilOnFail),
        .StencilDepthFailOp = static_cast<D3D12_STENCIL_OP>(D3D12_STENCIL_OP_KEEP + stencilOnDepthFail),
        .StencilPassOp = static_cast<D3D12_STENCIL_OP>(D3D12_STENCIL_OP_KEEP + stencilOnPass),
        .StencilFunc = static_cast<D3D12_COMPARISON_FUNC>(D3D12_COMPARISON_FUNC_NEVER + stencilFunction),
      };
      return {
        .DepthEnable = 0 != enableDepthTest,
        .DepthWriteMask = enableDepthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO,
        .DepthFunc = static_cast<D3D12_COMPARISON_FUNC>(D3D12_COMPARISON_FUNC_NEVER + depthFunc),
        .StencilEnable = 0 != enableStencil,
        .StencilReadMask = static_cast<UINT8>(stencilReadMask),
        .StencilWriteMask = static_cast<UINT8>(stencilWriteMask),
        .FrontFace = stencilOp,
        .BackFace = stencilOp,
        .DepthBoundsTestEnable = 0 != enableDepthBounds,
      };
    }

    PipelineOptionalDynamicStateMask getDynamicStateMask() const
    {
      PipelineOptionalDynamicStateMask mask = {
        .hasDepthBoundsTest = enableDepthBounds == 1,
        .hasStencilTest = enableStencil == 1,
      };
      auto usesBlendFactor = [](const auto &params) {
        auto isBlendFactor = [](uint32_t stored_factor) {
          const auto factor = static_cast<D3D12_BLEND>(D3D12_BLEND_ZERO + stored_factor);
          return (D3D12_BLEND_BLEND_FACTOR == factor) || (D3D12_BLEND_INV_BLEND_FACTOR == factor);
        };
        return isBlendFactor(params.blendFactors.Source) || isBlendFactor(params.blendFactors.Destination) ||
               isBlendFactor(params.blendAlphaFactors.Source) || isBlendFactor(params.blendAlphaFactors.Destination);
      };

      if (enableDualSourceBlending)
        mask.hasBlendConstants = usesBlendFactor(dualSourceBlend.params);
      else
      {
        const uint32_t numBlendParamsToCheck = enableIndependentBlend ? shaders::RenderState::NumIndependentBlendParameters : 1;
        for (uint32_t i = 0; i < numBlendParamsToCheck && !mask.hasBlendConstants; i++)
          mask.hasBlendConstants = usesBlendFactor(blendParams[i]);
      }
      return mask;
    }
  };

  // Struct should not have alignment intervals,
  // because we calculate hash for whole structure
  G_STATIC_ASSERT(sizeof(StaticState) == sizeof(StaticStateBits) + sizeof(StaticState::depthBias) +
                                           sizeof(StaticState::depthBiasSloped) + sizeof(StaticState::blendParams));

  void reset()
  {
    OSSpinlockScopedLock lock(mutex);
    publicStateTable.clear();
    staticStateTable.clear();
  }

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
      ref = publicStateTable.insert(end(publicStateTable),
        eastl::pair(def, PublicStateInfo{.dynamicState = dynamicState, .staticRenderStateID = staticStateId}));
    }
    return static_cast<uint32_t>(ref - begin(publicStateTable));
  }

  PublicStateInfo getDynamicAndStaticState(uint32_t public_id)
  {
    OSSpinlockScopedLock lock(mutex);
    auto &e = publicStateTable[public_id];
    return e.second;
  }

  static bool is_compatible(const DriverDesc &desc, const StaticState &state)
  {
    // either support the feature or don't use it
    const bool depthBoundsOk = desc.caps.hasDepthBoundsTest || 0 == state.enableDepthBounds;
    const bool conservativeRasterOk = desc.caps.hasConservativeRassterization || 0 == state.enableConservativeRaster;
    const bool viewInstancingOk = desc.caps.hasBasicViewInstancing || 0 == state.viewInstanceCount;

    if (!depthBoundsOk)
      logdbg("DX12: ...render state is not compatible, uses depth bounds test...");
    if (!conservativeRasterOk)
      logdbg("DX12: ...render state is not compatible, uses conservative raster...");
    if (!viewInstancingOk)
      logdbg("DX12: ...render state is not compatible, uses view instancing...");

    return depthBoundsOk && conservativeRasterOk && viewInstancingOk;
  }

  DynamicArray<StaticRenderStateIDWithHash> loadStaticStatesFromBlk(DeviceContext &ctx, const DriverDesc &desc, const DataBlock *blk,
    const char *default_format);

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
    MEMBER_COMPARE(independentBlendEnabled);
    MEMBER_COMPARE(dualSourceBlendEnabled);
    const auto numBlendParamsToCompare =
      (l.independentBlendEnabled && !l.dualSourceBlendEnabled) ? shaders::RenderState::NumIndependentBlendParameters : 1;
    MEMBER_COMPARE(conservativeRaster);
    MEMBER_COMPARE(alphaToCoverage);
    MEMBER_COMPARE(cull);
    MEMBER_COMPARE(forcedSampleCount);
    MEMBER_COMPARE(scissorEnabled);
    MEMBER_COMPARE(colorWr);
    MEMBER_COMPARE(zClip);
    MEMBER_COMPARE(viewInstanceCount);

    // fromRenderState forces ztest off when forcedSampleCount is set, so
    // depth func and biases do not reach the static state in that case
    if (l.ztest && 0 == l.forcedSampleCount)
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

#define BLEND_PARAMS_COMPARE(field_)                 \
  do                                                 \
  {                                                  \
    MEMBER_COMPARE(field_.ablend);                   \
    MEMBER_COMPARE(field_.sepablend);                \
    if (l.field_.ablend)                             \
    {                                                \
      MEMBER_COMPARE(field_.blendOp);                \
      MEMBER_COMPARE(field_.ablendFactors.src);      \
      MEMBER_COMPARE(field_.ablendFactors.dst);      \
      if (l.field_.sepablend)                        \
      {                                              \
        MEMBER_COMPARE(field_.sepablendOp);          \
        MEMBER_COMPARE(field_.sepablendFactors.src); \
        MEMBER_COMPARE(field_.sepablendFactors.dst); \
      }                                              \
    }                                                \
  } while (0)

    for (uint32_t i = 0; i < numBlendParamsToCompare; i++)
    {
      if (l.dualSourceBlendEnabled)
        BLEND_PARAMS_COMPARE(dualSourceBlend.params);
      else
        BLEND_PARAMS_COMPARE(blendParams[i]);
    }

#undef BLEND_PARAMS_COMPARE
#undef MEMBER_COMPARE
    return true;
  }
  static DynamicState getDynamicStateFromState(const shaders::RenderState &def)
  {
    // also a function to overwrite this in d3d interface...
    return {
      .stencilRef = def.stencilRef,
      .enableScissor = 0 != def.scissorEnabled,
    };
  }
  StaticRenderStateID registerStaticState(DeviceContext &ctx, const StaticState &def);
  StaticRenderStateID registerStaticState(DeviceContext &ctx, const shaders::RenderState &def)
  {
    return registerStaticState(ctx, StaticState::fromRenderState(def));
  }
  OSSpinlock mutex;
  dag::Vector<StaticState> staticStateTable;
  dag::Vector<eastl::pair<shaders::RenderState, PublicStateInfo>> publicStateTable;
};
} // namespace drv3d_dx12
