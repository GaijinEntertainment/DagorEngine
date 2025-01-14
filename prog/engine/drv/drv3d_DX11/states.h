// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <limits.h>
#include <generic/dag_staticTab.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_renderStates.h>
#include <drv/3d/dag_samplerHandle.h>
#include "sampler.h"

namespace drv3d_dx11
{

constexpr inline int BLEND_FACTORS_COUNT = 4;

enum
{
  VIEWMOD_NOCHANGES = 0,
  VIEWMOD_FULL = 1,
  VIEWMOD_CUSTOM = 2
};

struct ViewData
{
  int x, y;
  int w, h;
  float minz, maxz;

  ViewData() : x(0), y(0), w(1), h(1), minz(0.0f), maxz(0.0f) {}
};

struct RasterizerState
{
  bool wire;
  bool scissorEnable;
  bool cullFlip;
  bool depthClip;
  uint32_t cullMode; // CULL_NONE
  float depthBias;
  float slopeDepthBias;
  uint32_t forcedSampleCount;
  bool conservativeRaster;

  //    bool  depthBoundsEnable;          //= false
  //    float depthBoundsZmin;
  //    float depthBoundsZmax;

  ViewData viewport;
  int16_t scissor_x;
  int16_t scissor_y;
  int16_t scissor_w;
  int16_t scissor_h;


  /*
      .DepthBias = false;
      .DepthClipEnable = true;
      .MultisampleEnable = false;
      .AntialiasedLineEnable = false;
      */

  ID3D11RasterizerState *getStateObject();

  RasterizerState() :
    //         depthBoundsEnable(false), depthBoundsZmin(0.0f),depthBoundsZmax(1.0f),
    wire(false),
    scissorEnable(false),
    cullFlip(false),
    cullMode(CULL_NONE),
    depthBias(0.0f),
    slopeDepthBias(0.0f),
    depthClip(true),
    forcedSampleCount(0),
    conservativeRaster(false)
  {
    G_STATIC_ASSERT(CULL_NONE > 0);

    viewport.x = 0;
    viewport.y = 0;
    viewport.w = 0;
    viewport.h = 0;
    viewport.minz = 0.0f;
    viewport.maxz = 1.0f;

    scissor_x = 0;
    scissor_y = 0;
    scissor_w = 0;
    scissor_h = 0;
  }

  D3D11_CULL_MODE getCullMode()
  {
    if (cullMode == CULL_NONE)
      return D3D11_CULL_NONE;
    return cullFlip ? ((cullMode == CULL_CCW) ? D3D11_CULL_FRONT : D3D11_CULL_BACK)
                    : ((cullMode == CULL_CCW) ? D3D11_CULL_BACK : D3D11_CULL_FRONT);
  }

  struct Key
  {
    int depthBias;
    float slopeDepthBias;
    union
    {
      uint32_t k;
      struct
      {
        uint32_t wire : 1;
        uint32_t cullMode : 2; // 1 2 3
        uint32_t scissorEnable : 1;
        uint32_t cullFlip : 1;
        uint32_t depthClip : 1;
        uint32_t multisampleEnable : 1;
        uint32_t antialiasedLineEnable : 1;
        uint32_t forcedSampleCount : 5; // 0 - 16
        uint32_t conservativeRaster : 1;
      } s;
    } u;
    inline uint32_t getHash() const { return hash32shiftmult(u.k); }
  };


  Key makeKey(int depth_bias_int)
  {
    Key k;
    k.depthBias = depth_bias_int;
    k.slopeDepthBias = slopeDepthBias;
    k.u.k = 0;
    k.COPY_KEY(wire);
    k.COPY_KEY(cullMode);
    k.COPY_KEY(scissorEnable);
    k.COPY_KEY(cullFlip);
    k.COPY_KEY(depthClip);
    k.u.s.multisampleEnable = 0;
    k.u.s.antialiasedLineEnable = 0;
    k.COPY_KEY(forcedSampleCount);
    k.COPY_KEY(conservativeRaster);
    return k;
  };
};

struct BlendState
{
  struct BlendParams
  {
    bool ablendEnable;
    bool sepAblendEnable;
    uint8_t ablendOp; // D3D11_BLEND_OP 3 bits
    uint8_t ablendOpA;
    uint8_t ablendSrc; // D3D11_BLEND 5 bits
    uint8_t ablendDst;
    uint8_t ablendSrcA;
    uint8_t ablendDstA;
  };

  uint8_t alphaToCoverage;
  bool independentBlendEnabled;
  BlendParams params[shaders::RenderState::NumIndependentBlendParameters]; //-V730_NOINIT
  uint8_t writeMask[Driver3dRenderTarget::MAX_SIMRT];                      // D3D11_COLOR_WRITE_ENABLE 4 bits

  struct Key
  {
  private:
    friend struct BlendState;

    union BlendKey
    {
      struct
      {
        uint32_t ablendEnable : 1;
        uint32_t sepAblendEnable : 1;
        uint32_t ablendOp : 3; // D3D11_BLEND_OP 3 bits
        uint32_t ablendOpA : 3;
        uint32_t ablendSrc : 5; // D3D11_BLEND 5 bits
        uint32_t ablendDst : 5;
        uint32_t ablendSrcA : 5;
        uint32_t ablendDstA : 5;
      };
      uint32_t bits;
    };

    union
    {
      struct
      {
        uint32_t writeMask0 : 4;
        uint32_t writeMask1 : 4;
        uint32_t writeMask2 : 4;
        uint32_t writeMask3 : 4;
        uint32_t writeMask4 : 4;
        uint32_t writeMask5 : 4;
        uint32_t writeMask6 : 4;
        uint32_t writeMask7 : 4;
      };
      uint32_t writeMaskBits;
    };

    BlendKey blendKeys[shaders::RenderState::NumIndependentBlendParameters];

    uint32_t alphaToCoverage : 1;

  public:
    Key()
    {
      // makes sure all the bits of the key set to zero
      memset(this, 0, sizeof(Key));
    }
    inline uint32_t getHash() const { return hash32shiftmult(writeMaskBits + blendKeys[0].bits); }
  };

  static_assert(sizeof(Key) == 4 + shaders::RenderState::NumIndependentBlendParameters * sizeof(Key::BlendKey) + 4);

  ID3D11BlendState *getStateObject();

  BlendState() : alphaToCoverage(0), independentBlendEnabled(false)
  {
    for (auto &blendParams : params)
    {
      blendParams.ablendEnable = false;
      blendParams.sepAblendEnable = false;
      blendParams.ablendOp = BLENDOP_ADD;
      blendParams.ablendSrc = BLEND_ONE;
      blendParams.ablendDst = BLEND_ZERO;
      blendParams.ablendOpA = BLENDOP_ADD;
      blendParams.ablendSrcA = BLEND_ONE;
      blendParams.ablendDstA = BLEND_ZERO;
    }

    for (size_t i = 0; i < countof(writeMask); ++i)
      writeMask[i] = D3D11_COLOR_WRITE_ENABLE_ALL;
  }

  Key makeKey()
  {
    Key key;
    key.alphaToCoverage = alphaToCoverage;

    for (uint32_t i = 0; i < shaders::RenderState::NumIndependentBlendParameters; i++)
    {
      auto &blendKey = key.blendKeys[i];
      const auto &blendParams = params[i];

      blendKey.ablendEnable = blendParams.ablendEnable;
      blendKey.sepAblendEnable = blendParams.sepAblendEnable;
      blendKey.ablendOp = blendParams.ablendOp;
      blendKey.ablendOpA = blendParams.ablendOpA;
      blendKey.ablendSrc = blendParams.ablendSrc;
      blendKey.ablendDst = blendParams.ablendDst;
      blendKey.ablendSrcA = blendParams.ablendSrcA;
      blendKey.ablendDstA = blendParams.ablendDstA;
    }

    key.writeMask0 = writeMask[0];
    key.writeMask1 = writeMask[1];
    key.writeMask2 = writeMask[2];
    key.writeMask3 = writeMask[3];
    key.writeMask4 = writeMask[4];
    key.writeMask5 = writeMask[5];
    key.writeMask6 = writeMask[6];
    key.writeMask7 = writeMask[7];
    return key;
  };
};

struct DepthStencilState
{
  bool depthEnable;   //= false
  bool depthWrite;    //= false
  bool stencilEnable; //= false

  uint8_t depthFunc;   // D3D11_COMPARISON_FUNC
  uint8_t stencilMask; // 0xff

  uint8_t stencilFunc;      // D3D11_COMPARISON_FUNC
  uint8_t stencilFail;      // D3D11_STENCIL_OP
  uint8_t stencilDepthFail; //
  uint8_t stencilDepthPass; //

  uint8_t stencilBackFunc;      // D3D11_COMPARISON_FUNC - 4 bit
  uint8_t stencilBackFail;      // D3D11_STENCIL_OP - 4 bit
  uint8_t stencilBackDepthFail; //
  uint8_t stencilBackDepthPass; //

  typedef uint64_t Key;

  ID3D11DepthStencilState *getStateObject();

  DepthStencilState() :
    depthEnable(false),
    depthWrite(false),
    stencilEnable(false),
    depthFunc(CMPF_GREATEREQUAL),
    stencilMask(0xff),
    stencilFunc(CMPF_NEVER),
    stencilFail(STNCLOP_KEEP),
    stencilDepthFail(STNCLOP_KEEP),
    stencilDepthPass(STNCLOP_KEEP),
    stencilBackFunc(CMPF_NEVER),
    stencilBackFail(STNCLOP_KEEP),
    stencilBackDepthFail(STNCLOP_KEEP),
    stencilBackDepthPass(STNCLOP_KEEP)
  {}

  Key makeKey(bool is_depth_bounds_test_enabled)
  {
    union
    {
      Key k;
      struct
      {
        uint32_t depthEnable : 1;
        uint32_t depthWrite : 1;
        uint32_t stencilEnable : 1;
        uint32_t depthFunc : 4;
        uint32_t stencilFail : 4;
        uint32_t stencilDepthFail : 4;
        uint32_t stencilDepthPass : 4;
        uint32_t stencilFunc : 4;
        uint32_t stencilBackFail : 4;
        uint32_t stencilBackDepthFail : 4;
        uint32_t stencilBackDepthPass : 4;
        uint32_t stencilBackFunc : 4;
        uint32_t stencilMask : 8;
      } s;
    } u;

    u.k = 0;
    COPY_KEY(depthEnable);
    COPY_KEY(depthWrite);
    COPY_KEY(stencilEnable);
    COPY_KEY(depthFunc);
    COPY_KEY(stencilFail);
    COPY_KEY(stencilDepthFail);
    COPY_KEY(stencilDepthPass);
    COPY_KEY(stencilFunc);
    COPY_KEY(stencilBackFail);
    COPY_KEY(stencilBackDepthFail);
    COPY_KEY(stencilBackDepthPass);
    COPY_KEY(stencilBackFunc);
    COPY_KEY(stencilMask);
    return u.k;
  };
};

struct SamplerState
{
  enum class SamplerSource : uint8_t
  {
    None = 0,
    Texture = 1,
    Sampler = 2,
    Latest = 3
  };


  ID3D11ShaderResourceView *viewObject = nullptr;
  ID3D11SamplerState *stateObject = nullptr;
  SamplerKey latestSamplerKey = {};

  BaseTexture *texture = nullptr;
  GenericBuffer *buffer = nullptr;
  d3d::SamplerHandle samplerHandle = d3d::INVALID_SAMPLER_HANDLE;
  SamplerSource samplerSource = SamplerSource::None;

  // return true if modified
  bool setTex(BaseTexture *tex, unsigned shader_stage, bool use_sampler)
  {
    if (use_sampler)
    {
      samplerSource = tex ? SamplerSource::Texture : SamplerSource::None;
      samplerHandle = d3d::INVALID_SAMPLER_HANDLE;
    }

    if (buffer)
    {
      buffer = NULL;
      texture = tex;
      viewObject = NULL;
      stateObject = NULL;
      BaseTex *bt = (BaseTex *)tex;
      if (bt)
      {
        bt->unsetModified(shader_stage);
      }
      return true;
    }

    if (texture == tex)
    {
      if (tex != NULL)
      {
        BaseTex *bt = (BaseTex *)tex;
        if (!bt->getModified(shader_stage))
        {
          return false;
        }
        bt->unsetModified(shader_stage);
      }
      else // tex == NULL
      {
        return stateObject != nullptr;
      }
    }

    texture = tex;
    // will be filled on flush
    viewObject = NULL;
    stateObject = NULL;
    return true;
  }

  bool setBuffer(GenericBuffer *buf)
  {
    if (texture)
    {
      texture = NULL;
      buffer = buf;
      viewObject = NULL;
      stateObject = NULL;
      return true;
    }

    if (buffer == buf)
    {
      return false;
    }

    buffer = buf;
    // will be filled on flush
    viewObject = NULL;
    stateObject = NULL;
    return true;
  }

  bool setSampler(d3d::SamplerHandle sampler)
  {
    bool unchanged = ((samplerSource == SamplerSource::Sampler) && (sampler == samplerHandle)) ||
                     ((samplerSource == SamplerSource::None) && (sampler == d3d::INVALID_SAMPLER_HANDLE));
    if (unchanged)
    {
      return false;
    }

    stateObject = nullptr;
    samplerSource = sampler == d3d::INVALID_SAMPLER_HANDLE ? SamplerSource::None : SamplerSource::Sampler;
    samplerHandle = sampler;
    return true;
  }
};

struct TextureFetchState
{
  struct Samplers
  {
    StaticTab<SamplerState, MAX_RESOURCES> resources;
    uint32_t modifiedMask = 0;
    G_STATIC_ASSERT(MAX_RESOURCES <= sizeof(decltype(modifiedMask)) * CHAR_BIT);
    bool flush(unsigned shader_stage, bool force, ID3D11ShaderResourceView **views, ID3D11SamplerState **states, int &first, int &max);
    ID3D11SamplerState *getStateObject(const BaseTex *basetex);
    ID3D11SamplerState *getStateObject(const SamplerKey &key);
  };
  carray<Samplers, STAGE_MAX_EXT> resources;

  struct UAV
  {
    carray<ID3D11UnorderedAccessView *, MAX_UAV> uavs;
    uint32_t uavModifiedMask;
    uint32_t uavsSet;
    bool uavsUsed;
    UAV() : uavModifiedMask(0), uavsSet(0), uavsUsed(false) { mem_set_0(uavs); }
  };
  carray<UAV, STAGE_MAX_EXT> uavState;
  uint8_t lastHDGBitsApplied;

  TextureFetchState() { lastHDGBitsApplied = 0; }

  bool setUav(int stage, unsigned slot, ID3D11UnorderedAccessView *uav)
  {
    G_ASSERT(slot < MAX_UAV);
    if (uav)
      uavState[stage].uavsUsed = true;

    uint32_t slot_mask = (1 << slot);
    ID3D11UnorderedAccessView *curr = uavState[stage].uavs[slot];
    if (curr == uav)
      return false;

    uavState[stage].uavModifiedMask |= slot_mask;
    uavState[stage].uavs[slot] = uav;

    if (stage == STAGE_CS && d3d::get_driver_desc().caps.hasAsyncCompute)
    {
      UAV &asyncCsState = uavState[STAGE_CS_ASYNC_STATE];
      if (uav)
        asyncCsState.uavsUsed = true;
      asyncCsState.uavModifiedMask |= slot_mask;
      asyncCsState.uavs[slot] = uav;
    }

    return true;
  }

  void flush(bool force, uint32_t hdg_bits);
  void flush_cs(bool force, bool async = false);
};

struct MiniRenderStateUnsafe
{
  BaseTexture *tex0; // Must be protected with resources CS while stored.

  Driver3dRenderTarget rt;
  shaders::DriverRenderStateId renderState;

  RasterizerState nextRasterizerState;
  uint8_t stencilRef;
  float blendFactor[BLEND_FACTORS_COUNT];
  int l, t, w, h;
  float zn, zf;

  void store();
  void restore();
};

} // namespace drv3d_dx11
