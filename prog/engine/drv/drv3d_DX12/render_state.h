// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "bitfield.h"
#include "driver.h"
#include "dynamic_array.h"
#include "tagged_handles.h"

#include <drv/3d/dag_consts.h>
#include <drv/3d/dag_decl.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_renderStates.h>
#include <drv/shadersMetaData/dxil/compiled_shader_header.h>
#include <EASTL/vector.h>
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
  struct DynamicState
  {
    uint32_t stencilRef : 8;
    uint32_t enableScissor : 1;
  };
  struct StaticStateBits
  {
    // first 32 bits
    uint32_t enableDepthTest : 1;
    uint32_t enableDepthWrite : 1;
    uint32_t enableDepthClip : 1;
    uint32_t enableDepthBounds : 1;
    uint32_t enableStencil : 1;
    uint32_t enableIndependentBlend : 1;
    uint32_t enableAlphaToCoverage : 1;

    uint32_t depthFunc : 3;
    uint32_t forcedSampleCountShift : 3;
    uint32_t enableConservativeRaster : 1;
    uint32_t viewInstanceCount : 2;
    uint32_t cullMode : 16; // 2 bit used, 14 free

    // second 32 bits
    uint32_t stencilReadMask : 8;
    uint32_t stencilWriteMask : 8;
    uint32_t stencilFunction : 3;
    uint32_t stencilOnFail : 3;
    uint32_t stencilOnDepthFail : 3;
    uint32_t stencilOnPass : 7; // 3 bit used, 4 free

    // third 32 bits
    uint32_t colorWriteMask;

    StaticStateBits() :
      enableDepthTest{0},
      enableDepthWrite{0},
      enableDepthClip{0},
      enableDepthBounds{0},
      enableStencil{0},
      enableIndependentBlend{0},
      enableAlphaToCoverage{0},
      depthFunc{0},
      forcedSampleCountShift{0},
      enableConservativeRaster{0},
      viewInstanceCount{0},
      cullMode{0},
      stencilReadMask{0},
      stencilWriteMask{0},
      stencilFunction{0},
      stencilOnFail{0},
      stencilOnDepthFail{0},
      stencilOnPass{0},
      colorWriteMask{0}
    {}
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
    struct BlendParams
    {
      BlendFactors blendFactors;
      BlendFactors blendAlphaFactors;
      uint8_t blendFunction : 3;
      uint8_t blendAlphaFunction : 3;
      uint8_t enableBlending : 2; // 1 bit used, 1 free
    };

    // 3 * 32 bits (4 * 3 * 8 bits)
    BlendParams blendParams[shaders::RenderState::NumIndependentBlendParameters]{};

    static_assert(sizeof(BlendParams) == 3);
    static_assert(sizeof(blendParams) == shaders::RenderState::NumIndependentBlendParameters * sizeof(BlendParams));

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
      result.aprintf(64, " depthBias %x", *reinterpret_cast<const uint32_t *>(&depthBias));
      result.aprintf(64, " depthBiasSloped %x", *reinterpret_cast<const uint32_t *>(&depthBiasSloped));

      for (uint32_t i = 0; i < shaders::RenderState::NumIndependentBlendParameters; i++)
      {
        result.aprintf(64, " blendParams[%d].enableBlending %u", i, blendParams[i].enableBlending);
        result.aprintf(64, " blendParams[%d].blendFactors.Source %u", i, blendParams[i].blendFactors.Source);
        result.aprintf(64, " blendParams[%d].blendFactors.Destination %u", i, blendParams[i].blendFactors.Destination);
        result.aprintf(64, " blendParams[%d].blendAlphaFactors.Source %u", i, blendParams[i].blendAlphaFactors.Source);
        result.aprintf(64, " blendParams[%d].blendAlphaFactors.Destination %u", i, blendParams[i].blendAlphaFactors.Destination);
        result.aprintf(64, " blendParams[%d].blendFunction %u", i, blendParams[i].blendFunction);
        result.aprintf(64, " blendParams[%d].blendAlphaFunction %u", i, blendParams[i].blendAlphaFunction);
      }

      char buf[sizeof(*this) * 2 + 1] = {};
      data_to_str_hex_buf(buf, sizeof(buf), this, sizeof(*this));
      result.aprintf(sizeof(buf) + 4, "= %s", buf);

      return result;
    }

    eastl::string toStringForPipelineName() const
    {
      eastl::string result;

      // the string is every long
      result.reserve(0x4000);

      result = "[";

      result.append_sprintf("enableDepthTest=%u,", enableDepthTest);
      result.append_sprintf("enableDepthWrite=%u,", enableDepthWrite);
      result.append_sprintf("enableDepthClip=%u,", enableDepthClip);
      result.append_sprintf("enableDepthBounds=%u,", enableDepthBounds);
      result.append_sprintf("enableStencil=%u,", enableStencil);
      result.append_sprintf("enableIndependentBlend=%u,", enableIndependentBlend);
      result.append_sprintf("enableAlphaToCoverage=%u,", enableAlphaToCoverage);

      result.append_sprintf("depthFunc=%u,", depthFunc);
      result.append_sprintf("stencilReadMask=%u,", stencilReadMask);
      result.append_sprintf("stencilWriteMask=%u,", stencilWriteMask);
      result.append_sprintf("stencilFunction=%u,", stencilFunction);
      result.append_sprintf("stencilOnFail=%u,", stencilOnFail);
      result.append_sprintf("stencilOnDepthFail=%u,", stencilOnDepthFail);
      result.append_sprintf("stencilOnPass=%u,", stencilOnPass);
      result.append_sprintf("forcedSampleCountShift=%u,", forcedSampleCountShift);
      result.append_sprintf("enableConservativeRaster=%u,", enableConservativeRaster);

      result.append_sprintf("viewInstanceCount=%u,", viewInstanceCount);
      result.append_sprintf("cullMode=%u,", cullMode);
      result.append_sprintf("colorWriteMask=%08x,", colorWriteMask);
      result.append_sprintf("depthBias=%08x,", *reinterpret_cast<const uint32_t *>(&depthBias));
      result.append_sprintf("depthBiasSloped=%08x,", *reinterpret_cast<const uint32_t *>(&depthBiasSloped));

      result += "blendParams";
      for (uint32_t i = 0; i < shaders::RenderState::NumIndependentBlendParameters; i++)
      {
        auto &param = blendParams[i];
        result.append_sprintf("[%d][", i);
        result.append_sprintf("enableBlending=%u,", param.enableBlending);
        result.append_sprintf("blendFactors.Source=%u,", param.blendFactors.Source);
        result.append_sprintf("blendFactors.Destination=%u,", param.blendFactors.Destination);
        result.append_sprintf("blendAlphaFactors.Source=%u,", param.blendAlphaFactors.Source);
        result.append_sprintf("blendAlphaFactors.Destination=%u,", param.blendAlphaFactors.Destination);
        result.append_sprintf("blendFunction=%u,", param.blendFunction);
        result.append_sprintf("blendAlphaFunction=%u", param.blendAlphaFunction);
        result += "]";
      }

      result += "]";

      return result;
    }

    friend bool operator==(const StaticState &l, const StaticState &r)
    {
      const auto blendBytesToCompare = l.enableIndependentBlend ? sizeof(blendParams) : sizeof(BlendParams);
      return 0 ==
               memcmp(static_cast<const StaticStateBits *>(&l), static_cast<const StaticStateBits *>(&r), sizeof(StaticStateBits)) &&
             0 == memcmp(l.blendParams, r.blendParams, blendBytesToCompare) && l.depthBias == r.depthBias &&
             l.depthBiasSloped == r.depthBiasSloped;
    }
    friend bool operator!=(const StaticState &l, const StaticState &r) { return !(l == r); }

    inline uint32_t distance(const StaticState &other) const
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

      for (uint32_t i = 0; i < countof(blendParams); ++i)
      {
        CMP(blendParams[i].blendFactors.Source);
        CMP(blendParams[i].blendFactors.Destination);
        CMP(blendParams[i].blendAlphaFactors.Source);
        CMP(blendParams[i].blendAlphaFactors.Destination);
        CMP(blendParams[i].blendFunction);
        CMP(blendParams[i].blendAlphaFunction);
        CMP(blendParams[i].enableBlending);
      }
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

      result.enableIndependentBlend = def.independentBlendEnabled;
      for (uint32_t i = 0; i < shaders::RenderState::NumIndependentBlendParameters; i++)
      {
        if (def.blendParams[i].ablend)
        {
          result.blendParams[i].enableBlending = 1;
          result.blendParams[i].blendFunction = def.blendParams[i].blendOp - D3D12_BLEND_OP_ADD;
          result.blendParams[i].blendFactors.Source = def.blendParams[i].ablendFactors.src - D3D12_BLEND_ZERO;
          result.blendParams[i].blendFactors.Destination = def.blendParams[i].ablendFactors.dst - D3D12_BLEND_ZERO;
          if (def.blendParams[i].sepablend)
          {
            result.blendParams[i].blendAlphaFunction = def.blendParams[i].sepablendOp - D3D12_BLEND_OP_ADD;
            result.blendParams[i].blendAlphaFactors.Source = def.blendParams[i].sepablendFactors.src - D3D12_BLEND_ZERO;
            result.blendParams[i].blendAlphaFactors.Destination = def.blendParams[i].sepablendFactors.dst - D3D12_BLEND_ZERO;
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
            result.blendParams[i].blendAlphaFunction = result.blendParams[i].blendFunction;
            result.blendParams[i].blendAlphaFactors.Source =
              colorToAlphaChannelMap[result.blendParams[i].blendFactors.Source] - D3D12_BLEND_ZERO;
            result.blendParams[i].blendAlphaFactors.Destination =
              colorToAlphaChannelMap[result.blendParams[i].blendFactors.Destination] - D3D12_BLEND_ZERO;
          }
        }
        else
        {
          result.blendParams[i].enableBlending = 0;
          result.blendParams[i].blendFunction = D3D12_BLEND_OP_ADD - D3D12_BLEND_OP_ADD;
          result.blendParams[i].blendFactors.Source = D3D12_BLEND_ONE - D3D12_BLEND_ZERO;
          result.blendParams[i].blendFactors.Destination = D3D12_BLEND_ZERO - D3D12_BLEND_ZERO;
          result.blendParams[i].blendAlphaFunction = D3D12_BLEND_OP_ADD - D3D12_BLEND_OP_ADD;
          result.blendParams[i].blendAlphaFactors.Source = D3D12_BLEND_ONE - D3D12_BLEND_ZERO;
          result.blendParams[i].blendAlphaFactors.Destination = D3D12_BLEND_ZERO - D3D12_BLEND_ZERO;
        }
      }

      result.viewInstanceCount = def.viewInstanceCount;

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

      result.IndependentBlendEnable = !has_uniform_color_mask(finalColorTargetMask) || enableIndependentBlend;
      const auto RTCount = result.IndependentBlendEnable ? Driver3dRenderTarget::MAX_SIMRT : 1;
      for (uint32_t i = 0; i < RTCount; ++i)
      {
        const auto blendParamsId = i < shaders::RenderState::NumIndependentBlendParameters && enableIndependentBlend ? i : 0;
        const auto &blendParameters = blendParams[blendParamsId];

        result.RenderTarget[i].RenderTargetWriteMask = finalColorTargetMask & 15;
        result.RenderTarget[i].BlendEnable = blendParameters.enableBlending;
        result.RenderTarget[i].LogicOpEnable = FALSE;
        result.RenderTarget[i].SrcBlend = static_cast<D3D12_BLEND>(D3D12_BLEND_ZERO + blendParameters.blendFactors.Source);
        result.RenderTarget[i].DestBlend = static_cast<D3D12_BLEND>(D3D12_BLEND_ZERO + blendParameters.blendFactors.Destination);
        result.RenderTarget[i].BlendOp = static_cast<D3D12_BLEND_OP>(D3D12_BLEND_OP_ADD + blendParameters.blendFunction);
        result.RenderTarget[i].SrcBlendAlpha = static_cast<D3D12_BLEND>(D3D12_BLEND_ZERO + blendParameters.blendAlphaFactors.Source);
        result.RenderTarget[i].DestBlendAlpha =
          static_cast<D3D12_BLEND>(D3D12_BLEND_ZERO + blendParameters.blendAlphaFactors.Destination);
        result.RenderTarget[i].BlendOpAlpha = static_cast<D3D12_BLEND_OP>(D3D12_BLEND_OP_ADD + blendParameters.blendAlphaFunction);
        result.RenderTarget[i].LogicOp = D3D12_LOGIC_OP_NOOP;

        finalColorTargetMask >>= 4;
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
      mask.hasBlendConstants = false;
      uint32_t numBlendParamsToCheck = enableIndependentBlend ? shaders::RenderState::NumIndependentBlendParameters : 1;
      for (uint32_t i = 0; i < numBlendParamsToCheck; i++)
      {
        auto srcBlend = static_cast<D3D12_BLEND>(D3D12_BLEND_ZERO + blendParams[i].blendFactors.Source);
        auto dstBlend = static_cast<D3D12_BLEND>(D3D12_BLEND_ZERO + blendParams[i].blendFactors.Destination);
        auto srcABlend = static_cast<D3D12_BLEND>(D3D12_BLEND_ZERO + blendParams[i].blendAlphaFactors.Source);
        auto dstABlend = static_cast<D3D12_BLEND>(D3D12_BLEND_ZERO + blendParams[i].blendAlphaFactors.Destination);
        mask.hasBlendConstants = (D3D12_BLEND_BLEND_FACTOR == srcBlend) || (D3D12_BLEND_INV_BLEND_FACTOR == srcBlend) ||
                                 (D3D12_BLEND_BLEND_FACTOR == dstBlend) || (D3D12_BLEND_INV_BLEND_FACTOR == dstBlend) ||
                                 (D3D12_BLEND_BLEND_FACTOR == srcABlend) || (D3D12_BLEND_INV_BLEND_FACTOR == srcABlend) ||
                                 (D3D12_BLEND_BLEND_FACTOR == dstABlend) || (D3D12_BLEND_INV_BLEND_FACTOR == dstABlend) ||
                                 bool(mask.hasBlendConstants);
        if (mask.hasBlendConstants)
          break;
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

  static bool is_compatible(const Driver3dDesc &desc, const StaticState &state)
  {
    // either support the feature or don't use it
    if ((desc.caps.hasDepthBoundsTest || 0 == state.enableDepthBounds) &&
        (desc.caps.hasConservativeRassterization || 0 == state.enableConservativeRaster) &&
        (desc.caps.hasBasicViewInstancing || 0 == state.viewInstanceCount))
    {
      return true;
    }

    if (!(desc.caps.hasDepthBoundsTest || 0 == state.enableDepthBounds))
    {
      logdbg("DX12: ...render state is not compatible, uses depth bounds test...");
    }
    if (!(desc.caps.hasConservativeRassterization || 0 == state.enableConservativeRaster))
    {
      logdbg("DX12: ...render state is not compatible, uses conservative raster...");
    }
    if (!(desc.caps.hasBasicViewInstancing || 0 == state.viewInstanceCount))
    {
      logdbg("DX12: ...render state is not compatible, uses view instancing...");
    }
    return false;
  }

  DynamicArray<StaticRenderStateIDWithHash> loadStaticStatesFromBlk(DeviceContext &ctx, const Driver3dDesc &desc, const DataBlock *blk,
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
    const auto numBlendParamsToCompare = l.independentBlendEnabled ? shaders::RenderState::NumIndependentBlendParameters : 1;
    for (uint32_t i = 0; i < numBlendParamsToCompare; i++)
    {
      MEMBER_COMPARE(blendParams[i].ablend);
      MEMBER_COMPARE(blendParams[i].sepablend);
    }
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

    for (uint32_t i = 0; i < numBlendParamsToCompare; i++)
    {
      if (l.blendParams[i].ablend)
      {
        MEMBER_COMPARE(blendParams[i].blendOp);
        MEMBER_COMPARE(blendParams[i].ablendFactors.src);
        MEMBER_COMPARE(blendParams[i].ablendFactors.dst);
        if (l.blendParams[i].sepablend)
        {
          MEMBER_COMPARE(blendParams[i].sepablendOp);
          MEMBER_COMPARE(blendParams[i].sepablendFactors.src);
          MEMBER_COMPARE(blendParams[i].sepablendFactors.dst);
        }
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
