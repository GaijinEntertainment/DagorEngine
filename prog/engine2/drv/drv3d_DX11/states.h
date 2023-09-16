#ifndef __DRV3D_DX11_STATES_H
#define __DRV3D_DX11_STATES_H
#pragma once
#include <limits.h>
#include <generic/dag_staticTab.h>

namespace drv3d_dx11
{

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
/*
  enum
  {
     ABLEND_MRT_MASK0 = (1<<0),
     ABLEND_MRT_MASK1 = (1<<1),
     ABLEND_MRT_MASK2 = (1<<2),
     ABLEND_MRT_MASK3 = (1<<3),
     ABLEND_MRT_ALL = (ABLEND_MRT_MASK0 | ABLEND_MRT_MASK1 | ABLEND_MRT_MASK2 | ABLEND_MRT_MASK3),
     ABLEND_MRT_ILLEGAL = (1<<7)
  };
*/
struct BlendState
{
  bool ablendEnable; // target 0
  bool sepAblendEnable;
  uint8_t ablendEnableMrt; // 1bit for every RT
  uint8_t alphaToCoverage;

  uint8_t ablendOp; // D3D11_BLEND_OP 3 bits
  uint8_t ablendOpA;
  uint8_t ablendSrc; // D3D11_BLEND_   5 bits
  uint8_t ablendDst;
  uint8_t ablendSrcA;
  uint8_t ablendDstA;

  uint8_t writeMask[Driver3dRenderTarget::MAX_SIMRT]; // D3D11_COLOR_WRITE_ENABLE 4 bits

  typedef uint64_t Key;

  ID3D11BlendState *getStateObject();

  BlendState() :
    ablendEnable(false),
    sepAblendEnable(false),
    ablendEnableMrt(0x0),
    alphaToCoverage(0), // alphaToOne(0),
    ablendOp(BLENDOP_ADD),
    ablendSrc(BLEND_ONE),
    ablendDst(BLEND_ZERO),
    ablendOpA(BLENDOP_ADD),
    ablendSrcA(BLEND_ONE),
    ablendDstA(BLEND_ZERO)

  {
    for (size_t i = 0; i < countof(writeMask); ++i)
      writeMask[i] = D3D11_COLOR_WRITE_ENABLE_ALL;
  }

  Key makeKey()
  {
    union
    {
      Key k;
      struct
      {
        uint64_t ablendEnableMrt : 7; // should be 8, for 8 mrt
        uint64_t ablendEnable : 1;
        uint64_t sepAblendEnable : 1;
        uint64_t alphaToCoverage : 1;
        uint64_t ablendOp_ablendOpA : 5; // D3D11_BLEND_OP 3 bits per parameter, but each param has only 5 variants (5 bits for all
                                         // combinations)
        uint64_t ablendSrc_Dst_SrcA_DstA : 17; // D3D11_BLEND_   5 bits per parameter, but each param has only 17 variants (17 bits for
                                               // all combinations)
        uint64_t writeMask0 : 4;
        uint64_t writeMask1 : 4;
        uint64_t writeMask2 : 4;
        uint64_t writeMask3 : 4;
        uint64_t writeMask4 : 4;
        uint64_t writeMask5 : 4;
        uint64_t writeMask6 : 4;
        uint64_t writeMask7 : 4;
      } s;
    } u;
    G_STATIC_ASSERT(sizeof(u) == sizeof(Key));
    u.k = 0;
    COPY_KEY(ablendEnable);
    COPY_KEY(sepAblendEnable);
    COPY_KEY(alphaToCoverage);
    u.s.ablendEnableMrt = uint32_t(ablendEnableMrt & (uint32_t(1 << 8) - 1));
    const uint32_t BLEND_OP_COUNT = 5; // https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_blend_op
    u.s.ablendOp_ablendOpA = ablendOp * BLEND_OP_COUNT + ablendOpA;
    const uint32_t BLEND_COUNT = 17; // https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_blend
    u.s.ablendSrc_Dst_SrcA_DstA = ((ablendSrc * BLEND_COUNT + ablendDst) * BLEND_COUNT + ablendSrcA) * BLEND_COUNT + ablendDstA;
    u.s.writeMask0 = writeMask[0]; // MAX_SIMRT=4
    u.s.writeMask1 = writeMask[1];
    u.s.writeMask2 = writeMask[2];
    u.s.writeMask3 = writeMask[3];
    u.s.writeMask4 = writeMask[4];
    u.s.writeMask5 = writeMask[5];
    u.s.writeMask6 = writeMask[6];
    u.s.writeMask7 = writeMask[7];
    return u.k;
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
  ID3D11ShaderResourceView *viewObject = nullptr;
  ID3D11SamplerState *stateObject = nullptr;
  BaseTexture *texture = nullptr;
  GenericBuffer *buffer = nullptr;

  struct Key
  {
    uint32_t borderColor;
    uint32_t lodBias;
    union
    {
      uint32_t k;
      struct
      {
        uint32_t anisotropyLevel : 5;
        uint32_t addrU : 3;
        uint32_t addrV : 3;
        uint32_t addrW : 3;
        uint32_t texFilter : 3;
        uint32_t mipFilter : 3;
      } s;
    } u;

    void copy(const BaseTex *tex, int anisotropy_level)
    {
      borderColor = uint32_t(tex->borderColor);
      lodBias = *((uint32_t *)(&tex->lodBias));
      u.k = tex->key & tex->SAMPLER_KEY_MASK; //
    };
    inline uint32_t getHash() const { return hash32shiftmult(u.k); }
  };

  static Key makeKey(const BaseTex *tex, int anisotropy_level)
  {
    Key k;
    ZeroMemory(&k, sizeof(k));
    k.copy(tex, anisotropy_level);
    return k;
  };

  // return true if modified
  bool setTex(BaseTexture *tex, unsigned shader_stage)
  {
    if (buffer)
    {
      buffer = NULL;
      texture = tex;
      viewObject = NULL;
      stateObject = NULL;
      BaseTex *bt = (BaseTex *)tex;
      if (bt)
        bt->unsetModified(shader_stage);
      return true;
    }
    if (texture == tex)
    {
      if (tex != NULL)
      {
        BaseTex *bt = (BaseTex *)tex;
        if (!bt->getModified(shader_stage))
          return false;
        bt->unsetModified(shader_stage);
      }
      else // tex = NULL
        return false;
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
      return false;

    buffer = buf;
    // will be filled on flush
    viewObject = NULL;
    stateObject = NULL;
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

} // namespace drv3d_dx11


#endif
