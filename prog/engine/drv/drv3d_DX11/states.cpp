// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/vector_map.h>
#include <generic/dag_tab.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <debug/dag_debug.h>
#include <debug/dag_fatal.h>
#include <math/dag_mathUtils.h>
#include <util/dag_string.h>
#include <osApiWrappers/dag_spinlock.h>
#include <util/dag_generationReferencedData.h>
#include <convertHelper.h>

#include "sampler.h"
#include "driver.h"
#include "clear.h"
#if HAS_NVAPI
#include <nvapi.h>
#endif
#include <AmdDxExtDepthBoundsApi.h>
#include <d3d11_3.h>
#include <perfMon/dag_statDrv.h>

using namespace drv3d_dx11;

static RenderState &g_frameState = g_render_state; // ok for strict aliasing?
// #define g_frameState g_render_state
#include "frameStateTM.inc.cpp"
#include <drv/3d/dag_sampler.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_renderStates.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_bindless.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_variableRateShading.h>
#include <drv/3d/dag_streamOutput.h>
#include <drv/3d/dag_shader.h>
#include <gpuConfig.h>
// #undef g_frameState


#define MINIMUM_REPRESENTABLE_D32 3e-10
#define MINIMUM_REPRESENTABLE_D24 33e-8
#define MINIMUM_REPRESENTABLE_D16 2e-5
#define MAX_RASTERIZER_STATES     128

#if DAGOR_DBGLEVEL > 1
#define CHECK_SHADOW_BUFFERS_CONSISTENCY 1
#endif

namespace drv3d_dx11
{
static bool is_depth_bounds_test_enabled = false;
static bool was_depth_bounds_test_enabled = false;
static float depth_bounds_test_min = 0.f;
static float depth_bounds_test_max = 1.f;

struct DriverRenderState
{
  ID3D11BlendState *blendState;
  ID3D11RasterizerState *rasterState;
  ID3D11DepthStencilState *depthStencilState;
  bool enableDepthBounds;
  bool scissorEnabled;
  ID3D11RasterizerState *wireRasterState;
  shaders::RenderState sourceShaderRenderState;
};

// We can use ConcurrentElementPool<DriverRenderState> here to avoid using mutex, but since it's cached in engine
// (shaders/renderStates.cpp) there no much point as it's created rarely
static OSSpinlock render_states_mutex;
static GenerationReferencedData<shaders::DriverRenderStateId, DriverRenderState> render_states;
shaders::DriverRenderStateId current_render_state;
shaders::DriverRenderStateId stretch_prepare_render_state;
eastl::vector_map<int, shaders::DriverRenderStateId> clear_view_states;

KeyMapWideHashed<COMProxyPtr<ID3D11BlendState>, BlendState::Key, 128> blend_state_cache; // currently we have ~70 in game, so we use
                                                                                         // hash
KeyMapWideHashed<COMProxyPtr<ID3D11RasterizerState>, RasterizerState::Key, 32> rasterizer_state_cache;
static int rasterizer_state_cacheCount = 0;
KeyMap<COMProxyPtr<ID3D11DepthStencilState>, DepthStencilState::Key, 16> depth_stencil_state_cache; // currently we have ~10 depth
                                                                                                    // sencil states. Keep it linear.
KeyMapWideHashed<COMProxyPtr<ID3D11SamplerState>, SamplerKey, 128> texture_fetch_state_cache;
void get_states_mem_used(String &str)
{
  str.printf(128, "blend_state_cache.size() =%d\n", blend_state_cache.size());
  str.aprintf(128, "depth_stencil_state_cache.size() =%d\n", depth_stencil_state_cache.size());
  str.aprintf(128, "texture_fetch_state_cache.size() =%d\n", texture_fetch_state_cache.size());
  // too much info
  // for (int i = 0; i < texture_fetch_state_cache.BUCKETS; ++i)
  //   str.aprintf(128, "texture_fetch_state_cache[%d] = %d\n", i, texture_fetch_state_cache.table[i].size());
}

bool get_render_state_target_size(int &w, int &h)
{
  RenderState &rs = g_render_state;
  if (rs.nextRtState.isColorUsed())
  {
    for (int i = 0; i < rs.nextRtState.MAX_SIMRT; ++i)
    {
      if (rs.nextRtState.isColorUsed(i))
      {
        return d3d::get_render_target_size(w, h, rs.nextRtState.color[i].tex, rs.nextRtState.color[i].level);
      }
    }
  }
  else if (rs.nextRtState.isDepthUsed())
  {
    return d3d::get_render_target_size(w, h, rs.nextRtState.depth.tex, rs.nextRtState.depth.level);
  }

  w = g_driver_state.width;
  h = g_driver_state.height;
  return true;
}

void resolve_msaa_and_gen_mips(BaseTex *tex)
{
  if (!tex || !tex->dirtyRt)
    return;

  if (tex->tex.resolvedTex)
  {
    tex->resolve(tex->tex.resolvedTex, tex->format);
  }

  tex->dirtyRt = 0;
}

static BOOL toBOOL(bool b) { return b ? TRUE : FALSE; }

static DriverRenderState create_default_rstate()
{
  DriverRenderState state;
  state.enableDepthBounds = false;
  state.rasterState = RasterizerState().getStateObject();
  state.blendState = BlendState().getStateObject();
  state.depthStencilState = DepthStencilState().getStateObject();
  return state;
}

bool init_states(RenderState &rs)
{
  OSSpinlockScopedLock lock(render_states_mutex);
  current_render_state = render_states.emplaceOne(create_default_rstate());
  return true;
}

static DriverRenderState shader_render_state_to_driver_render_state(const shaders::RenderState &state);

void flush_states(RenderState &rs)
{
  if (dagor_d3d_force_driver_reset)
    return;
  if (rs.rasterizerModified || rs.depthStencilModified)
  {
    rs.rasterizerModified = false;

    OSSpinlockScopedLock lock(render_states_mutex);
    const auto currentState = render_states.get(current_render_state);
    ID3D11RasterizerState *s = currentState->rasterState;

    if (g_render_state.nextRasterizerState.wire)
      s = render_states.get(current_render_state)->wireRasterState;
#if DAGOR_DBGLEVEL > 0
    if (currentState->sourceShaderRenderState.forcedSampleCount > 0 && rs.nextRtState.isDepthUsed())
    {
      D3D_ERROR("Forced sample count is used when depth RT is set! FSC will be disabled");
      shaders::RenderState fixedState = currentState->sourceShaderRenderState;
      fixedState.forcedSampleCount = 0;
      s = render_states.get(render_states.emplaceOne(shader_render_state_to_driver_render_state(fixedState)))->rasterState;
    }
#endif
    if (s != NULL)
    {
      ContextAutoLock contextLock;
      dx_context->RSSetState(s);
    }
  }


  RasterizerState &rz = rs.nextRasterizerState;
  int tw = 1, th = 1;
  if (rs.viewModified || rs.scissorModified)
    get_render_state_target_size(tw, th);

  if (rs.viewModified)
  {
    if (rs.viewModified == VIEWMOD_FULL)
    {
      rz.viewport.x = 0;
      rz.viewport.y = 0;
      rz.viewport.w = tw;
      rz.viewport.h = th;
      rz.viewport.minz = 0.0f;
      rz.viewport.maxz = 1.0f;
    }

    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = (float)rz.viewport.x;
    viewport.TopLeftY = (float)rz.viewport.y;
    viewport.Width = (float)rz.viewport.w;
    viewport.Height = (float)rz.viewport.h;
    viewport.MinDepth = rz.viewport.minz;
    viewport.MaxDepth = rz.viewport.maxz;
    rs.viewModified = VIEWMOD_NOCHANGES;

    ContextAutoLock contextLock;
    dx_context->RSSetViewports(1, &viewport);
  }

  if (rs.scissorModified)
  {
    int x = 0;
    int y = 0;
    int w = tw;
    int h = th;

    if (render_states.get(current_render_state)->scissorEnabled)
    {
      x = rz.scissor_x;
      y = rz.scissor_y;
      w = rz.scissor_w;
      h = rz.scissor_h;
    }

    D3D11_RECT rect[1];
    rect[0].left = x;
    rect[0].right = x + w; // - rect[0].left;
    rect[0].top = y;
    rect[0].bottom = y + h; // - rect[0].top;

    ContextAutoLock contextLock;
    dx_context->RSSetScissorRects(1, rect);

    rs.scissorModified = false;
  }

  if (rs.alphaBlendModified)
  {
    rs.alphaBlendModified = false;
    ID3D11BlendState *s = render_states.get(current_render_state)->blendState;
    if (s != NULL)
    {
      ContextAutoLock contextLock;
      dx_context->OMSetBlendState(s, rs.blendFactor, 0xffffffff);
    }
    if (get_gpu_driver_cfg().flushBeforeSepAblendAndBlendFactorForRT1 && rs.nextRtState.isColorUsed(1))
    {
      const auto sourceState = render_states.get(current_render_state)->sourceShaderRenderState;

      bool ablendUsed = sourceState.blendParams[0].ablend;
      if (sourceState.independentBlendEnabled)
      {
        for (int i = 1; i < sourceState.NumIndependentBlendParameters && !ablendUsed; ++i)
          ablendUsed = sourceState.blendParams[i].ablend;
      }
      if (ablendUsed)
      {
        ContextAutoLock lock;
        dx_context->Dispatch(0, 0, 0);
      }
    }
  }

  if (rs.depthStencilModified)
  {
    rs.depthStencilModified = false;
    ID3D11DepthStencilState *s = render_states.get(current_render_state)->depthStencilState;
    if (s != NULL)
    {
      ContextAutoLock contextLock;
      dx_context->OMSetDepthStencilState(s, rs.stencilRef);
    }

    if (is_depth_bounds_test_enabled || was_depth_bounds_test_enabled)
    {
      ContextAutoLock contextLock;
#if HAS_NVAPI
      if (nv_dbt_supported)
        NvAPI_D3D11_SetDepthBoundsTest(dx_device, is_depth_bounds_test_enabled, depth_bounds_test_min, depth_bounds_test_max);
#endif

      if (ati_dbt_supported)
        amdDepthBoundsExtension->SetDepthBounds(is_depth_bounds_test_enabled, depth_bounds_test_min, depth_bounds_test_max);

      was_depth_bounds_test_enabled = is_depth_bounds_test_enabled;
    }
  }

  flush_samplers(rs);
}


void flush_samplers(RenderState &rs)
{
  if (dagor_d3d_force_driver_reset)
    return;
  // Removed RT may become a new sampler.
  if (rs.texFetchState.resources[STAGE_PS].anyModifiedMask() | rs.texFetchState.resources[STAGE_VS].anyModifiedMask() ||
      rs.rtModified || rs.texFetchState.lastHDGBitsApplied != rs.hdgBits)
  {
    rs.texFetchState.flush(rs.rtModified, rs.hdgBits); // todo: optimize rtModified case
  }
}

void flush_cs_objects(RenderState &rs, bool async)
{
  // rs.csState.srvModifiedMask = rs.csState.uavModifiedMask = 0xFFFFFFF;
  const uint32_t stage = async ? (uint32_t)STAGE_CS_ASYNC_STATE : (uint32_t)STAGE_CS;

  uint32_t uavModifiedMask = rs.texFetchState.uavState[stage].uavModifiedMask;
  uint32_t resourcesModifiedMask = rs.texFetchState.resources[stage].anyModifiedMask();

  if (uavModifiedMask | resourcesModifiedMask)
    rs.texFetchState.flush_cs(false, async);
}

void close_states()
{
  blend_state_cache.clear();
  rasterizer_state_cache.clear();
  rasterizer_state_cacheCount = 0;
  depth_stencil_state_cache.clear();
  texture_fetch_state_cache.clear();
}

ID3D11RasterizerState *RasterizerState::getStateObject()
{
  int depthBiasInt = (int)(depthBias / MINIMUM_REPRESENTABLE_D16);
  Key k = makeKey(depthBiasInt);
  COMProxyPtr<ID3D11RasterizerState> *proxy = rasterizer_state_cache.find(k);
  if (proxy != NULL)
  {
    G_ASSERT(proxy->obj != NULL);
    proxy->obj->AddRef();
    return proxy->obj;
  }

  COMProxyPtr<ID3D11RasterizerState> p;
  D3D11_RASTERIZER_DESC_NEXT rd;
  ZeroMemory(&rd, sizeof(rd));
  rd.FillMode = wire ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
  rd.CullMode = getCullMode();
  rd.FrontCounterClockwise = FALSE;
  rd.DepthBias = depthBiasInt;
  rd.DepthBiasClamp = 0.f; // Disable.
  rd.SlopeScaledDepthBias = slopeDepthBias;
  rd.DepthClipEnable = toBOOL(depthClip);
  rd.ScissorEnable = toBOOL(scissorEnable);
  rd.MultisampleEnable = true;
  rd.AntialiasedLineEnable = false;
  rd.ForcedSampleCount = forcedSampleCount;
  rd.ConservativeRaster = conservativeRaster ? D3D11_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D11_CONSERVATIVE_RASTERIZATION_MODE_OFF;

  {
    HRESULT hr;
    if (dx_device3) // It is not documented whether the vanilla ID3D11DeviceContext supports downcasted ID3D11RasterizerState, but it
                    // does.
      hr = dx_device3->CreateRasterizerState2(&rd, (ID3D11RasterizerState2 **)&p.obj);
    else if (dx_device1)
      hr = dx_device1->CreateRasterizerState1((D3D11_RASTERIZER_DESC1 *)&rd, (ID3D11RasterizerState1 **)&p.obj);
    else
      hr = dx_device->CreateRasterizerState((D3D11_RASTERIZER_DESC *)&rd, &p.obj);

    if (hr == S_OK)
    {
      if (rasterizer_state_cacheCount < MAX_RASTERIZER_STATES || (depthBiasInt == 0 && slopeDepthBias == 0.f))
      {
        if (rasterizer_state_cache.add(k, p))
        {
          rasterizer_state_cacheCount++;
          p.obj->AddRef();
          return p.obj;
        }
      }
      else
        return p.obj; // To be released by the caller.
    }
  }
  return NULL;
}

ID3D11BlendState *BlendState::getStateObject()
{
  Key k = makeKey();
  COMProxyPtr<ID3D11BlendState> *proxy = blend_state_cache.find(k);
  if (proxy != NULL)
  {
    G_ASSERT(proxy->obj != NULL);
    return proxy->obj;
  }

  COMProxyPtr<ID3D11BlendState> p;
  D3D11_BLEND_DESC desc;
  ZeroMemory(&desc, sizeof(desc));

  desc.AlphaToCoverageEnable = toBOOL(alphaToCoverage);
  desc.IndependentBlendEnable = independentBlendEnabled;
  for (int i = 1; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
  {
    desc.IndependentBlendEnable |= (writeMask[0] != writeMask[i]);
  }

  for (int i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
  {
    uint32_t blendParamsId = independentBlendEnabled && (i < shaders::RenderState::NumIndependentBlendParameters) ? i : 0;
    const auto &blendParams = params[blendParamsId];

    // available dagor blend modes directly corresponds to D3D11_BLEND
    D3D11_BLEND ablendSrcASet, ablendDstASet;
    if (!blendParams.sepAblendEnable)
    {
      static D3D11_BLEND blendRemap[20] = {
        D3D11_BLEND_ZERO, D3D11_BLEND_ZERO, D3D11_BLEND_ONE,
        D3D11_BLEND_SRC_ALPHA,     // D3D11_BLEND_SRC_COLOR   = 3,
        D3D11_BLEND_INV_SRC_ALPHA, // D3D11_BLEND_INV_SRC_COLOR   = 4,
        D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_DEST_ALPHA, D3D11_BLEND_INV_DEST_ALPHA,
        D3D11_BLEND_DEST_ALPHA,     // D3D11_BLEND_DEST_COLOR  = 9,
        D3D11_BLEND_INV_DEST_ALPHA, // D3D11_BLEND_INV_DEST_COLOR  = 10,
        D3D11_BLEND_SRC_ALPHA_SAT, D3D11_BLEND_ZERO, D3D11_BLEND_ZERO, D3D11_BLEND_BLEND_FACTOR, D3D11_BLEND_INV_BLEND_FACTOR,
        D3D11_BLEND_SRC1_ALPHA,     // D3D11_BLEND_SRC1_COLOR,
        D3D11_BLEND_INV_SRC1_ALPHA, // D3D11_BLEND_INV_SRC1_COLOR ,
        D3D11_BLEND_SRC1_ALPHA,
        D3D11_BLEND_INV_SRC1_ALPHA // 19
      };
      ablendSrcASet = blendRemap[blendParams.ablendSrc];
      ablendDstASet = blendRemap[blendParams.ablendDst];
    }
    else
    {
      ablendSrcASet = static_cast<D3D11_BLEND>(blendParams.ablendSrcA);
      ablendDstASet = static_cast<D3D11_BLEND>(blendParams.ablendDstA);
    }

    D3D11_RENDER_TARGET_BLEND_DESC &rtb = desc.RenderTarget[i];
    rtb.BlendEnable = toBOOL(blendParams.ablendEnable);
    if (rtb.BlendEnable)
    {
      rtb.SrcBlend = static_cast<D3D11_BLEND>(blendParams.ablendSrc); // same as D3D11_BLEND
      rtb.DestBlend = static_cast<D3D11_BLEND>(blendParams.ablendDst);
      rtb.BlendOp = static_cast<D3D11_BLEND_OP>(blendParams.ablendOp); // same as D3D11_BLEND_OP
      rtb.SrcBlendAlpha = ablendSrcASet;
      rtb.DestBlendAlpha = ablendDstASet;
      rtb.BlendOpAlpha = static_cast<D3D11_BLEND_OP>(blendParams.sepAblendEnable ? blendParams.ablendOpA : blendParams.ablendOp);
    }
    else
    {
      rtb.SrcBlend = D3D11_BLEND_ONE; // same as D3D11_BLEND
      rtb.DestBlend = D3D11_BLEND_ZERO;
      rtb.BlendOp = D3D11_BLEND_OP_ADD; // same as D3D11_BLEND_OP
      rtb.SrcBlendAlpha = D3D11_BLEND_ONE;
      rtb.DestBlendAlpha = D3D11_BLEND_ZERO;
      rtb.BlendOpAlpha = D3D11_BLEND_OP_ADD;
    }
    rtb.RenderTargetWriteMask = writeMask[i]; // D3D11_COLOR_WRITE_ENABLE
  };

  {
    HRESULT hr = dx_device->CreateBlendState(&desc, &p.obj);
    if (hr == S_OK)
    {
      blend_state_cache.add(k, p);
      return p.obj;
    }
  }

  return NULL;
}

ID3D11DepthStencilState *DepthStencilState::getStateObject()
{
  Key k = makeKey(is_depth_bounds_test_enabled);
  COMProxyPtr<ID3D11DepthStencilState> *proxy = depth_stencil_state_cache.find(k);
  if (proxy != NULL)
  {
    G_ASSERT(proxy->obj != NULL);
    return proxy->obj;
  }

  COMProxyPtr<ID3D11DepthStencilState> p;
  D3D11_DEPTH_STENCIL_DESC desc;
  ZeroMemory(&desc, sizeof(desc));
  desc.DepthEnable = toBOOL(depthEnable);
  desc.DepthWriteMask = depthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
  desc.DepthFunc = (D3D11_COMPARISON_FUNC)depthFunc;
  desc.StencilEnable = toBOOL(stencilEnable);
  desc.StencilReadMask = stencilReadMask;
  desc.StencilWriteMask = stencilWriteMask;

  desc.FrontFace.StencilFailOp = (D3D11_STENCIL_OP)stencilFail;
  desc.FrontFace.StencilDepthFailOp = (D3D11_STENCIL_OP)stencilDepthFail;
  desc.FrontFace.StencilPassOp = (D3D11_STENCIL_OP)stencilDepthPass;
  desc.FrontFace.StencilFunc = (D3D11_COMPARISON_FUNC)stencilFunc;

  desc.BackFace.StencilFailOp = (D3D11_STENCIL_OP)stencilBackFail;
  desc.BackFace.StencilDepthFailOp = (D3D11_STENCIL_OP)stencilBackDepthFail;
  desc.BackFace.StencilPassOp = (D3D11_STENCIL_OP)stencilBackDepthPass;
  desc.BackFace.StencilFunc = (D3D11_COMPARISON_FUNC)stencilBackFunc;

  {
    HRESULT hr = dx_device->CreateDepthStencilState(&desc, &p.obj);

    if (hr == S_OK)
    {
      depth_stencil_state_cache.add(k, p);
      return p.obj;
    }
  }

  return NULL;
}

ID3D11SamplerState *TextureFetchState::Resources::getStateObject(const SamplerKey &key)
{
  uint8_t anisotropyLevel =
    override_max_anisotropy_level > 0 ? min(override_max_anisotropy_level, uint8_t(key.anisotropyLevel)) : key.anisotropyLevel;

  COMProxyPtr<ID3D11SamplerState> *proxy = texture_fetch_state_cache.find(key);
  if (proxy != NULL)
  {
    G_ASSERT(proxy->obj != NULL);
    return proxy->obj;
  }

  COMProxyPtr<ID3D11SamplerState> p;
  D3D11_SAMPLER_DESC desc;

  int texFilter = key.texFilter;
  int mipFilter = key.mipFilter;

  G_ASSERT(texFilter < 5);
  G_ASSERT(mipFilter < 4);
  if (texFilter >= TEXFILTER_NONE) // 4
    texFilter = TEXFILTER_POINT;   // 4->1
  if (mipFilter >= TEXMIPMAP_LINEAR)
    mipFilter = TEXMIPMAP_LINEAR;
  if (anisotropyLevel > 1)
    texFilter = TEXFILTER_BEST; // 4->1

  // to avoid issues with shader dump, we keeped enum values intact when removing default modes
  // so table is intact, still contain old entries, yet they are not used now
  // [TEXMIPMAP_][TEXFILTER_]
  static const uint8_t filterMode[4][5] = {
    // TEXMIPMAP_DEFAULT
    D3D11_FILTER_MIN_MAG_MIP_LINEAR,                  // TEXFILTER_DEFAULT
    D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR,            // TEXFILTER_POINT
    D3D11_FILTER_MIN_MAG_MIP_LINEAR,                  // TEXFILTER_LINEAR
    D3D11_FILTER_ANISOTROPIC,                         // TEXFILTER_BEST
    D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // TEXFILTER_COMPARE
                                                      // TEXMIPMAP_NONE
    D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,            // TEXFILTER_DEFAULT
    D3D11_FILTER_MIN_MAG_MIP_POINT,                   // TEXFILTER_POINT
    D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,            // TEXFILTER_LINEAR
    D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,            // TEXFILTER_BEST
    D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // TEXFILTER_COMPARE
                                                      // TEXMIPMAP_POINT
    D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,            // TEXFILTER_DEFAULT
    D3D11_FILTER_MIN_MAG_MIP_POINT,                   // TEXFILTER_POINT
    D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,            // TEXFILTER_LINEAR
    D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,            // TEXFILTER_BEST
    D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // TEXFILTER_COMPARE
                                                      // TEXMIPMAP_LINEAR
    D3D11_FILTER_MIN_MAG_MIP_LINEAR,                  // TEXFILTER_DEFAULT
    D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR,            // TEXFILTER_POINT
    D3D11_FILTER_MIN_MAG_MIP_LINEAR,                  // TEXFILTER_LINEAR
    D3D11_FILTER_ANISOTROPIC,                         // TEXFILTER_BEST
    D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,       // TEXFILTER_COMPARE
  };

  desc.Filter = (D3D11_FILTER)filterMode[mipFilter][texFilter];
  desc.AddressU = (D3D11_TEXTURE_ADDRESS_MODE)key.addrU;
  desc.AddressV = (D3D11_TEXTURE_ADDRESS_MODE)key.addrV;
  desc.AddressW = (D3D11_TEXTURE_ADDRESS_MODE)key.addrW;
  desc.MipLODBias = key.lodBias;
  desc.MaxAnisotropy = anisotropyLevel;
  desc.ComparisonFunc = texFilter == TEXFILTER_COMPARE ? D3D11_COMPARISON_LESS_EQUAL : D3D11_COMPARISON_ALWAYS;
  desc.BorderColor[0] = normalizeColor(key.borderColor.r);
  desc.BorderColor[1] = normalizeColor(key.borderColor.g);
  desc.BorderColor[2] = normalizeColor(key.borderColor.b);
  desc.BorderColor[3] = normalizeColor(key.borderColor.a);
  // set in resview
  desc.MinLOD = 0;
  desc.MaxLOD = FLT_MAX;

  {
    HRESULT hr = dx_device->CreateSamplerState(&desc, &p.obj);
    if (hr == S_OK)
    {
      texture_fetch_state_cache.add(key, p);
      return p.obj;
    }
  }

  return NULL;
}

bool TextureFetchState::Resources::flush(unsigned shader_stage, bool force, ID3D11ShaderResourceView **views,
  ID3D11SamplerState **states, SlotRange &view_range_out, SlotRange &state_range_out)
{
  if ((anyModifiedMask() == 0 && !force) || resources.empty()) // Resources CS locked in TextureFetchState::flush.
    return false;

  view_range_out = SlotRange{};
  state_range_out = SlotRange{};

  if (force)
  {
    modifiedMask = (1ULL << resources.size()) - 1;
    samplersModifiedMask = modifiedMask;
  }
  int nextCnt = 0;
  unsigned i = 0, bit = 1;
  G_STATIC_ASSERT(sizeof(modifiedMask) == sizeof(bit));
  bool btWasModified = false;

  const uint32_t relevantSamplersMask = samplersModifiedMask & SAMPLERS_SLOT_MASK;

  // Necessary for access to g_sampler_keys; lock once.
  SamplerKeysAutoLock lock;

  for (ResourceSlotState &ss : resources)
  {
    BaseTex *bt = (BaseTex *)ss.texture;

    bool slotModified = (modifiedMask & bit) != 0;
    bool samplerSlotModified = (relevantSamplersMask & bit) != 0;
    bool textureStateModified = (bt && bt->getModified(shader_stage));

    if (slotModified || textureStateModified)
    {
      if (bt)
      {
        btWasModified |= bt->getModified(shader_stage);
#if DAGOR_DBGLEVEL > 0
        bt->wasUsed = 1;
#endif
        if (bt->dirtyRt)
          resolve_msaa_and_gen_mips(bt);
        const uint32_t maxMip = bt->maxMipLevel;
        // WARNING! if we set both sampler LOD and resView, we efficiently move min mip TWICE
        ss.viewObject = bt->getResView(0, maxMip, bt->minMipLevel - maxMip + 1);
      }
      else if (ss.buffer != NULL)
      {
        ss.viewObject = ss.buffer->getResView();
      }
      else
      {
        ss.viewObject = NULL;
      }

      update(view_range_out, i);
    }
#if CHECK_SHADOW_BUFFERS_CONSISTENCY
    else
    {
      if (bt)
      {
        const uint32_t maxMip = bt->maxMipLevel;
        ID3D11ShaderResourceView *view = bt->getResView(0, maxMip, bt->minMipLevel - maxMip + 1);
        G_ASSERT(ss.viewObject == view);
        // WARNING! if we set both sampler LOD and resView, we efficiently move min mip TWICE
      }
      else if (ss.buffer != NULL)
      {
        G_ASSERT(ss.viewObject == ss.buffer->getResView());
      }
      else
      {
        G_ASSERTF(ss.viewObject == NULL, "slot = %d", i);
      }
    }
#endif

    if (samplerSlotModified)
    {
      if (ss.samplerHandle != d3d::INVALID_SAMPLER_HANDLE)
      {
        // See `SamplerKeysAutoLock lock;` above.
        ss.stateObject = getStateObject(g_sampler_keys[uint64_t(ss.samplerHandle) - 1]);
      }
      else
      {
        ss.stateObject = nullptr;
      }

      update(state_range_out, i);
    }

    if (bt || ss.buffer != NULL)
      nextCnt = i + 1;

    views[i] = ss.viewObject;
    states[i] = ss.stateObject;
    if (dagor_d3d_force_driver_reset)
      return false;

    ++i;
    bit = bit << 1;
  }

  resources.resize(nextCnt);

  if (view_range_out.maxUsed < 0)
    view_range_out.firstUsed = 0;
  G_ASSERT(view_range_out.maxUsed < MAX_RESOURCES);
  if (state_range_out.maxUsed < 0)
    state_range_out.firstUsed = 0;
  G_ASSERT(state_range_out.maxUsed < MAX_CS_SAMPLERS && state_range_out.maxUsed < MAX_PS_SAMPLERS &&
           state_range_out.maxUsed < MAX_VS_SAMPLERS);
  modifiedMask = samplersModifiedMask = 0;
  // Reset modifications at the end of the flush because one texture can be set twice
  // or more to the different samplers
  if (btWasModified)
    for (int resNo = view_range_out.firstUsed; resNo <= min<int>(view_range_out.maxUsed, resources.size() - 1); ++resNo)
      if (resources[resNo].texture)
        ((BaseTex *)resources[resNo].texture)->unsetModified(shader_stage);
  return true;
}

void TextureFetchState::flush(bool force, uint32_t hdg_bits)
{
  ResAutoLock resLock;

  ID3D11ShaderResourceView *views[MAX_RESOURCES];
  ID3D11SamplerState *states[MAX_RESOURCES];

  G_STATIC_ASSERT(MAX_VS_SAMPLERS <= MAX_PS_SAMPLERS);
  SlotRange viewRange, samplerRange;
  if (resources[STAGE_PS].flush(STAGE_PS, force, views, states, viewRange, samplerRange))
  {
    if (!dagor_d3d_force_driver_reset)
    {
      ContextAutoLock contextLock;
      if (samplerRange.maxUsed >= 0)
      {
        dx_context->PSSetSamplers(samplerRange.firstUsed, samplerRange.maxUsed + 1 - samplerRange.firstUsed,
          &states[samplerRange.firstUsed]);
      }
      dx_context->PSSetShaderResources(viewRange.firstUsed, viewRange.maxUsed + 1 - viewRange.firstUsed, &views[viewRange.firstUsed]);
    }
  }

  if (lastHDGBitsApplied != hdg_bits && hdg_bits)
    force = true;

  if (resources[STAGE_VS].flush(STAGE_VS, force, views, states, viewRange, samplerRange))
  {
    if (!dagor_d3d_force_driver_reset)
    {
      ContextAutoLock contextLock;
      if (samplerRange.maxUsed >= 0)
      {
        dx_context->VSSetSamplers(samplerRange.firstUsed, samplerRange.maxUsed + 1 - samplerRange.firstUsed,
          &states[samplerRange.firstUsed]);
      }
      dx_context->VSSetShaderResources(viewRange.firstUsed, viewRange.maxUsed + 1 - viewRange.firstUsed, &views[viewRange.firstUsed]);
    }
  }
  else
    viewRange = samplerRange = {};

  const bool hasVsResources = (viewRange.firstUsed <= viewRange.maxUsed) || (samplerRange.firstUsed <= samplerRange.maxUsed);
  if ((hasVsResources || lastHDGBitsApplied != hdg_bits) && !dagor_d3d_force_driver_reset)
  {
    ContextAutoLock contextLock;

#define SET_SMP_RES(XS)                                                                              \
  if (hdg_bits & HAS_##XS)                                                                           \
  {                                                                                                  \
    if (smp_cnt > 0)                                                                                 \
      dx_context->XS##SetSamplers(samplerRange.firstUsed, smp_cnt, &states[samplerRange.firstUsed]); \
    dx_context->XS##SetShaderResources(viewRange.firstUsed, res_cnt, &views[viewRange.firstUsed]);   \
  }
#define CLR_SMP_RES(XS)                                          \
  if ((lastHDGBitsApplied & HAS_##XS) && !(hdg_bits & HAS_##XS)) \
  {                                                              \
    dx_context->XS##SetSamplers(0, MAX_VS_SAMPLERS, states);     \
    dx_context->XS##SetShaderResources(0, MAX_RESOURCES, views); \
  }

    if (hasVsResources)
    {
      int res_cnt = viewRange.maxUsed + 1 - viewRange.firstUsed;
      int smp_cnt = samplerRange.maxUsed + 1 - samplerRange.firstUsed;
      SET_SMP_RES(HS);
      SET_SMP_RES(DS);
      SET_SMP_RES(GS);
    }
    if (lastHDGBitsApplied != hdg_bits)
    {
      memset(states, 0, sizeof(states));
      memset(views, 0, sizeof(views));
      CLR_SMP_RES(HS);
      CLR_SMP_RES(DS);
      CLR_SMP_RES(GS);
    }
    lastHDGBitsApplied = hdg_bits;
#undef SET_SMP_RES
#undef CLR_SMP_RES
  }
}

template <typename obj_type>
void cs_state_flush_helper(int mask, int size, obj_type *src[], obj_type *dst[], int &first, int &count)
{
  memcpy(dst, src, size * sizeof(src[0]));
  first = size;
  int last = 0;
  uint32_t maskSized = mask & ((1 << size) - 1);
  if (maskSized)
  {
    first = __bsf_unsafe(maskSized);
    last = __bsr_unsafe(maskSized);
  }
  count = last - first + 1;
  G_ASSERT(count > 0);
}

void TextureFetchState::flush_cs(bool force, bool async)
{
  const uint32_t stage = async ? (uint32_t)STAGE_CS_ASYNC_STATE : (uint32_t)STAGE_CS;
  UAV &stageUavState = uavState[stage];

  if (stageUavState.uavModifiedMask)
  {
    int first, count;
    ID3D11UnorderedAccessView *res[MAX_UAV];
    cs_state_flush_helper<ID3D11UnorderedAccessView>(stageUavState.uavModifiedMask, MAX_UAV, stageUavState.uavs.data(), res, first,
      count);
    static UINT initialCounts[MAX_UAV] = {0}; // should be -1 or 0, however, we don't use counters at all
    {
      ContextAutoLock contextLock;
      dx_context->CSSetUnorderedAccessViews(first, count, res + first, initialCounts);
    }
    stageUavState.uavModifiedMask = 0;
  }

  ResAutoLock resLock;
  Resources &stageResources = resources[stage];
  SlotRange viewRange, samplerRange;
  ID3D11ShaderResourceView *views[MAX_RESOURCES];
  ID3D11SamplerState *states[MAX_RESOURCES];

  if (stageResources.flush(stage, force, views, states, viewRange, samplerRange))
  {
    if (!dagor_d3d_force_driver_reset)
    {
      {
        ContextAutoLock contextLock;
        if (samplerRange.maxUsed >= 0)
        {
          dx_context->CSSetSamplers(samplerRange.firstUsed, samplerRange.maxUsed + 1 - samplerRange.firstUsed,
            &states[samplerRange.firstUsed]);
        }
        dx_context->CSSetShaderResources(viewRange.firstUsed, viewRange.maxUsed + 1 - viewRange.firstUsed,
          &views[viewRange.firstUsed]);
      }
    }
  }
}

void MiniRenderStateUnsafe::store()
{
  RenderState &rs = g_render_state;
  tex0 = rs.texFetchState.resources[STAGE_PS].resources.size() > 0 ? rs.texFetchState.resources[STAGE_PS].resources[0].texture : NULL;
  d3d::get_render_target(rt);
  nextRasterizerState = rs.nextRasterizerState;
  stencilRef = rs.stencilRef;
  memcpy(blendFactor, rs.blendFactor, sizeof(blendFactor));
  renderState = current_render_state;
  d3d::getview(l, t, w, h, zn, zf);
}

void MiniRenderStateUnsafe::restore()
{
  RenderState &rs = g_render_state;

  rs.nextRasterizerState = nextRasterizerState;
  rs.stencilRef = stencilRef;
  memcpy(rs.blendFactor, blendFactor, sizeof(blendFactor));
  d3d::set_render_state(renderState);
  rs.modified = true;
  rs.rasterizerModified = true;
  rs.alphaBlendModified = true;
  rs.depthStencilModified = true;

  d3d::set_tex(STAGE_PS, 0, tex0);
  d3d::set_sampler(STAGE_PS, 0, d3d::request_sampler({}));
  d3d::set_program(BAD_PROGRAM);
  if (memcmp(&rt, &g_render_state.nextRtState, sizeof(rt)) != 0)
  {
    d3d::set_render_target(rt);
    d3d::setview(l, t, w, h, zn, zf);
  }
}

} // namespace drv3d_dx11

bool d3d::setview(int x, int y, int w, int h, float minz, float maxz)
{
  CHECK_THREAD;
  ViewData &vd = g_render_state.nextRasterizerState.viewport;
  vd.x = x;
  vd.y = y;
  vd.w = w;
  vd.h = h;
  vd.minz = minz;
  vd.maxz = maxz;
  g_render_state.viewModified = VIEWMOD_CUSTOM;
  g_render_state.modified = true;

  D3D_CONTRACT_ASSERT(x > D3D11_VIEWPORT_BOUNDS_MIN && x <= D3D11_VIEWPORT_BOUNDS_MAX);
  D3D_CONTRACT_ASSERT(y > D3D11_VIEWPORT_BOUNDS_MIN && y <= D3D11_VIEWPORT_BOUNDS_MAX);
  D3D_CONTRACT_ASSERT(w >= 0);
  D3D_CONTRACT_ASSERT(h >= 0);
  D3D_CONTRACT_ASSERTF(x + w <= D3D11_VIEWPORT_BOUNDS_MAX, "x=%d w= %d", x, w);
  D3D_CONTRACT_ASSERTF(y + h <= D3D11_VIEWPORT_BOUNDS_MAX, "y=%d h= %d", y, h);

  return true;
}

bool d3d::setviews(dag::ConstSpan<Viewport> viewports)
{
  G_UNUSED(viewports);
  G_ASSERTF(false, "Not implemented");
  return false;
}

bool d3d::getview(int &x, int &y, int &w, int &h, float &minz, float &maxz)
{
  CHECK_THREAD;
  ViewData &vd = g_render_state.nextRasterizerState.viewport;
  x = vd.x;
  y = vd.y;
  w = vd.w;
  h = vd.h;

  if (g_render_state.viewModified == VIEWMOD_FULL)
  {
    x = y = 0;
    get_render_state_target_size(w, h);
  }

  minz = vd.minz;
  maxz = vd.maxz;

  return true;
}

bool d3d::setscissors(dag::ConstSpan<ScissorRect> scissorRects)
{
  G_UNUSED(scissorRects);
  G_ASSERTF(false, "Not implemented");
  return false;
}

bool d3d::setwire(bool w)
{
  CHECK_THREAD;
  g_render_state.nextRasterizerState.wire = w;
  g_render_state.modified = g_render_state.rasterizerModified = true;
  return true;
}

static bool enable_depth_bounds(bool on)
{
  if (!nv_dbt_supported && !ati_dbt_supported)
    return false;

  if (is_depth_bounds_test_enabled == on)
    return true;

  g_render_state.modified = g_render_state.depthStencilModified = true;
  is_depth_bounds_test_enabled = on;
  return true;
}


bool d3d::set_depth_bounds(float zmin, float zmax)
{
  if (!nv_dbt_supported && !ati_dbt_supported)
    return false;

  zmin = clamp(zmin, 0.f, 1.f);
  zmax = clamp(zmax, zmin, 1.f);

  if (depth_bounds_test_min == zmin && depth_bounds_test_max == zmax)
    return true;

  depth_bounds_test_min = zmin;
  depth_bounds_test_max = zmax;

  g_render_state.modified = g_render_state.depthStencilModified = true;
  return true;
}

bool d3d::setstencil(uint32_t ref)
{
  CHECK_THREAD;
  if (g_render_state.stencilRef != ref)
  {
    g_render_state.stencilRef = ref;
    g_render_state.modified = g_render_state.depthStencilModified = true;
  }
  return true;
}

bool d3d::set_blend_factor(E3DCOLOR c)
{
  CHECK_THREAD;
  g_render_state.blendFactor[0] = normalizeColor(c.r);
  g_render_state.blendFactor[1] = normalizeColor(c.g);
  g_render_state.blendFactor[2] = normalizeColor(c.b);
  g_render_state.blendFactor[3] = normalizeColor(c.a);
  g_render_state.modified = g_render_state.alphaBlendModified = 1;
  return true;
}

DriverRenderState drv3d_dx11::shader_render_state_to_driver_render_state(const shaders::RenderState &state)
{
  BlendState blendState;
  blendState.independentBlendEnabled = state.independentBlendEnabled;

  auto translateParam = [](BlendState::BlendParams &dst, const auto &src) {
    dst.ablendEnable = src.ablend;
    dst.sepAblendEnable = src.sepablend;
    dst.ablendOp = src.blendOp;
    dst.ablendOpA = src.sepablendOp;
    dst.ablendSrc = src.ablendFactors.src;
    dst.ablendDst = src.ablendFactors.dst;
    dst.ablendSrcA = src.sepablendFactors.src;
    dst.ablendDstA = src.sepablendFactors.dst;
  };

  if (state.dualSourceBlendEnabled)
  {
    translateParam(blendState.params[0], state.dualSourceBlend.params);
  }
  else
  {
    for (uint32_t i = 0; i < shaders::RenderState::NumIndependentBlendParameters; i++)
      translateParam(blendState.params[i], state.blendParams[i]);
  }
  blendState.alphaToCoverage = state.alphaToCoverage;
  uint32_t writeMask = state.colorWr;
  for (size_t i = 0; i < Driver3dRenderTarget::MAX_SIMRT; i++, writeMask >>= 4)
    blendState.writeMask[i] = writeMask & 0xF;

  RasterizerState rasterState;
  rasterState.depthClip = state.zClip;
  rasterState.cullMode = state.cull;
  rasterState.depthBias = state.zBias;
  rasterState.slopeDepthBias = state.slopeZBias;
  rasterState.forcedSampleCount = state.forcedSampleCount;
  rasterState.conservativeRaster = state.conservativeRaster;
  rasterState.scissorEnable = state.scissorEnabled;

  DepthStencilState depthStencilState;
  depthStencilState.depthEnable = state.ztest;
  depthStencilState.depthWrite = state.zwrite;
  depthStencilState.depthFunc = state.zFunc;
  depthStencilState.stencilEnable = state.stencil.func > 0;
  if (depthStencilState.stencilEnable)
  {
    depthStencilState.stencilFunc = depthStencilState.stencilBackFunc = state.stencil.func;
    depthStencilState.stencilWriteMask = state.stencil.writeMask;
    depthStencilState.stencilReadMask = state.stencil.readMask;
    depthStencilState.stencilFail = depthStencilState.stencilBackFail = state.stencil.fail;
    depthStencilState.stencilDepthFail = depthStencilState.stencilBackDepthFail = state.stencil.zFail;
    depthStencilState.stencilDepthPass = depthStencilState.stencilBackDepthPass = state.stencil.pass;
  }

  DriverRenderState renderState;
  renderState.enableDepthBounds = state.depthBoundsEnable;
  renderState.blendState = blendState.getStateObject();
  renderState.rasterState = rasterState.getStateObject();
  renderState.depthStencilState = depthStencilState.getStateObject();
  renderState.scissorEnabled = state.scissorEnabled;

  RasterizerState wireRasterState;
  wireRasterState.depthClip = state.zClip;
  wireRasterState.cullMode = state.cull;
  wireRasterState.depthBias = state.zBias;
  wireRasterState.slopeDepthBias = state.slopeZBias;
  wireRasterState.forcedSampleCount = state.forcedSampleCount;
  wireRasterState.conservativeRaster = 0;
  wireRasterState.scissorEnable = state.scissorEnabled;
  wireRasterState.wire = true;
  renderState.wireRasterState = wireRasterState.getStateObject();

  renderState.sourceShaderRenderState = state;

  return renderState;
}


shaders::DriverRenderStateId d3d::create_render_state(const shaders::RenderState &state)
{
  DriverRenderState renderState = shader_render_state_to_driver_render_state(state);
  OSSpinlockScopedLock lock(render_states_mutex);
  return render_states.emplaceOne(renderState);
}


void drv3d_dx11::recreate_render_states()
{
  OSSpinlockScopedLock lock(render_states_mutex);
  for (int renderStateNo = 0; renderStateNo < render_states.totalSize(); renderStateNo++)
  {
    shaders::DriverRenderStateId renderStateRef = render_states.getRefByIdx(renderStateNo);
    if (renderStateRef)
    {
      DriverRenderState *renderState = render_states.get(renderStateRef);
      *renderState = shader_render_state_to_driver_render_state(renderState->sourceShaderRenderState);
    }
  }
}


bool d3d::set_render_state(shaders::DriverRenderStateId state_id)
{
  OSSpinlockScopedLock lock(render_states_mutex);

  DriverRenderState driverState = *render_states.get(state_id);
  ::enable_depth_bounds(driverState.enableDepthBounds);

  if (current_render_state != state_id)
  {
    DriverRenderState previousState = *render_states.get(current_render_state);
    const bool rasterUpdated = driverState.rasterState != previousState.rasterState;
    const bool blendUpdated = driverState.blendState != previousState.blendState;
    ;
    const bool depthStencilUpdated = driverState.depthStencilState != previousState.depthStencilState;
    g_render_state.rasterizerModified |= rasterUpdated;
    g_render_state.alphaBlendModified |= blendUpdated;
    g_render_state.depthStencilModified |= depthStencilUpdated;
    g_render_state.modified |= rasterUpdated || blendUpdated || depthStencilUpdated;
    current_render_state = state_id;
  }
  return true;
}

void d3d::clear_render_states()
{
  OSSpinlockScopedLock lock(render_states_mutex);
  render_states.clear();
  current_render_state = render_states.emplaceOne(create_default_rstate());
  clear_view_states.clear();
  stretch_prepare_render_state.reset();
}

static void check_content_loaded(BaseTexture *tex)
{
  BaseTex *bt = (BaseTex *)tex;

  G_ASSERTF(!bt || bt->getTID() == BAD_TEXTUREID || bt->getTID().checkMarkerBit() || tql::check_texture_id_valid(bt->getTID()),
    "Broken texture (%p->tidXored=0x%x tid=0x%x)", bt, bt->tidXored, bt->getTID());

  if (bt && bt->ddsxNotLoaded)
  {
#if DAGOR_DBGLEVEL > 0
    DAG_FATAL("ddsxTex <%s> allocated, but contents was not loaded before usage in d3d::settex()", bt->getName());
#else
    D3D_ERROR("ddsxTex <%s> allocated, but contents was not loaded before usage in d3d::settex()", bt->getName());
#endif
  }
}


static const int max_samples[STAGE_MAX] = {MAX_CS_SAMPLERS, MAX_PS_SAMPLERS, MAX_VS_SAMPLERS};

bool d3d::set_tex(unsigned shader_stage, unsigned slot, BaseTexture *tex)
{
  if (shader_stage >= STAGE_MAX)
  {
    D3D_CONTRACT_ERROR("invalid shader stage number %d", shader_stage);
    return false;
  }
  check_content_loaded(tex);

  if (slot >= g_device_desc.maxsimtex || slot >= MAX_RESOURCES)
  {
    D3D_CONTRACT_ERROR("invalid PS sampler number %d", slot);
    return false;
  }

  if (BaseTex *bt = (BaseTex *)tex)
  {
    if (bt->needs_clear)
      bt->clear();
    if (bt->cflg & TEXCF_UNORDERED)
      for (uint32_t mip = bt->maxMipLevel; mip <= bt->minMipLevel; ++mip)
        for (bool asUint : {false, true})
          remove_view_from_uav(bt->getExistingUaView(0, mip, asUint));
  }

  ResAutoLock resLock;
  TextureFetchState::Resources &resources = g_render_state.texFetchState.resources[shader_stage];
  resources.resources.resize(max(resources.resources.size(), slot + 1));
  resources.modifiedMask |= resources.resources[slot].setTex(tex, shader_stage) ? (1 << slot) : 0;

  g_render_state.modified = true;

  if (shader_stage == STAGE_CS && d3d::get_driver_desc().caps.hasAsyncCompute)
  {
    TextureFetchState::Resources &resources = g_render_state.texFetchState.resources[STAGE_CS_ASYNC_STATE];
    resources.resources.resize(max(resources.resources.size(), slot + 1));
    resources.modifiedMask |= resources.resources[slot].setTex(tex, shader_stage) ? (1 << slot) : 0;
  }
  return true;
}


void d3d::set_sampler(unsigned shader_stage, unsigned slot, d3d::SamplerHandle sampler)
{
  ResAutoLock resLock;
  TextureFetchState::Resources &resources = g_render_state.texFetchState.resources[shader_stage];
  resources.resources.resize(max(resources.resources.size(), slot + 1));
  resources.samplersModifiedMask |= resources.resources[slot].setSampler(sampler) ? (1 << slot) : 0;

  if (shader_stage == STAGE_CS && d3d::get_driver_desc().caps.hasAsyncCompute)
  {
    TextureFetchState::Resources &resources = g_render_state.texFetchState.resources[STAGE_CS_ASYNC_STATE];
    resources.resources.resize(max(resources.resources.size(), slot + 1));
    resources.samplersModifiedMask |= resources.resources[slot].setSampler(sampler) ? (1 << slot) : 0;
  }

  g_render_state.modified = true;
}


uint32_t d3d::register_bindless_sampler(SamplerHandle)
{
  G_ASSERTF(false, "d3d::register_bindless_sampler called on API without support");
  return 0;
}

uint32_t d3d::allocate_bindless_resource_range(D3DResourceType, uint32_t)
{
  G_ASSERTF(false, "d3d::allocate_bindless_resource_range called on API without support");
  return 0;
}

uint32_t d3d::resize_bindless_resource_range(D3DResourceType, uint32_t, uint32_t, uint32_t)
{
  G_ASSERTF(false, "d3d::resize_bindless_resource_range called on API without support");
  return 0;
}

void d3d::free_bindless_resource_range(D3DResourceType, uint32_t, uint32_t)
{
  G_ASSERTF(false, "d3d::free_bindless_resource_range called on API without support");
}

bool d3d::update_bindless_resource(D3DResourceType, uint32_t, D3dResource *)
{
  G_ASSERTF(false, "d3d::update_bindless_resource called on API without support");
  return false;
}

void d3d::update_bindless_resources_to_null(D3DResourceType, uint32_t, uint32_t)
{
  G_ASSERTF(false, "d3d::update_bindless_resource_to_null called on API without support");
}

bool d3d::clear_rwtexi(BaseTexture *tex, const unsigned val[4], uint32_t face, uint32_t mip_level)
{
  BaseTex *base = 0;
  if (tex)
  {
    base = (BaseTex *)tex;
    D3D_CONTRACT_ASSERT(base);
  }
  if (!base)
    return false;

#if DAGOR_DBGLEVEL > 0
  if (!(base->cflg & TEXCF_UNORDERED))
    D3D_CONTRACT_ERROR("can't clear_rwtex with non UAV texture (use TEXCF_UNORDERED) <%s>", base->getName());
#endif
  ID3D11UnorderedAccessView *uav = base->getUaView(face, mip_level, false);
  ContextAutoLock contextLock;
  dx_context->ClearUnorderedAccessViewUint(uav, val);
  return true;
}
bool d3d::clear_rwtexf(BaseTexture *tex, const float val[4], uint32_t face, uint32_t mip_level)
{
  BaseTex *base = 0;
  if (tex)
  {
    base = (BaseTex *)tex;
    D3D_CONTRACT_ASSERT(base);
  }
  if (!base)
    return false;
#if DAGOR_DBGLEVEL > 0
  if (!(base->cflg & TEXCF_UNORDERED))
    D3D_CONTRACT_ERROR("can't clear_rwtex with non UAV texture (use TEXCF_UNORDERED) <%s>", base->getName());
#endif
  ID3D11UnorderedAccessView *uav = base->getUaView(face, mip_level, false);
  ContextAutoLock lock;
  dx_context->ClearUnorderedAccessViewFloat(uav, val);
  return true;
}

bool d3d::zero_rwbufi(Sbuffer *buffer)
{
  if (buffer == nullptr)
    return false;

  D3D_CONTRACT_ASSERT(buffer->getFlags() & SBCF_BIND_UNORDERED);
  GenericBuffer *vb = (GenericBuffer *)buffer;

  ContextAutoLock contextLock;
  VALIDATE_GENERIC_RENDER_PASS_CONDITION(!g_render_state.isGenericRenderPassActive,
    "DX11: zero_rwbufi uses ClearUnorderedAccessViewUint inside a generic render pass");

  uint32_t values[4] = {0, 0, 0, 0};
  dx_context->ClearUnorderedAccessViewUint(vb->uav, values);

  return true;
}

bool d3d::clear_rt(const RenderTarget &rt, const ResourceClearValue &clear_val)
{
  if (!rt.tex)
    return false;

  BaseTex *base = (BaseTex *)rt.tex;
  ID3D11View *view = base->getRtView(rt.layer, rt.mip_level, 1, g_render_state.srgb_bb_write);
  if (is_depth_format_flg(base->cflg))
  {
    ContextAutoLock contextLock;
    dx_context->ClearDepthStencilView((ID3D11DepthStencilView *)view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, clear_val.asDepth,
      clear_val.asStencil);
  }
  else
  {
    // To preserve int bit pattern need to clear with shader
    // https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-clearrendertargetview#remarks
    if (base->isSampledAsFloat())
    {
      ContextAutoLock contextLock;
      dx_context->ClearRenderTargetView((ID3D11RenderTargetView *)view, clear_val.asFloat);
    }
    else
    {
      ResAutoLock resLock;
      MiniRenderStateUnsafe savedRs;
      savedRs.store();

      d3d::set_render_target(base, rt.mip_level);
      d3d::setview(0, 0, base->width, base->height, 0, 1);
      clear_slow(CLEAR_TARGET, clear_val.asFloat, 0.0f, 0u);

      savedRs.restore();
    }
  }

  return true;
}

bool d3d::discard_tex(BaseTexture *tex)
{
  CHECK_THREAD;
  if (!tex)
  {
    D3D_CONTRACT_ERROR("discard_tex called with null texture");
    return false;
  }
  auto texture = static_cast<BaseTex *>(tex);
  return texture->markTexelsUninitialized();
}

bool d3d::set_rwtex(unsigned shader_stage, unsigned slot, BaseTexture *tex, uint32_t face, uint32_t mip_level, bool as_uint)
{
  if (featureLevelsSupported < D3D_FEATURE_LEVEL_11_1 && shader_stage > STAGE_PS)
  {
    D3D_CONTRACT_ERROR("invalid stage %d", shader_stage);
    return false;
  }
  if (shader_stage == STAGE_VS)
    shader_stage = STAGE_PS;

  BaseTex *base = 0;
  ID3D11UnorderedAccessView *uav = nullptr;
  if (tex)
  {
    base = (BaseTex *)tex;
    if (base->maxMipLevel <= mip_level && mip_level <= base->minMipLevel)
      remove_texture_from_samplers(tex);
    uint32_t cflg = base->cflg;
    if (!(base->cflg & TEXCF_UNORDERED))
      D3D_CONTRACT_ERROR("can't setrwtex with non UAV texture (use TEXCF_UNORDERED) <%s>", base->getName());
    if (base->needs_clear)
      base->clear();
    uav = base->getUaView(face, mip_level, as_uint);
    remove_view_from_uav_ignore_slot(shader_stage, slot, uav);
  }

  if (slot >= MAX_UAV)
  {
    D3D_CONTRACT_ERROR("invalid UAV slot: %d", slot);
    return false;
  }
  g_render_state.texFetchState.setUav(shader_stage, slot, uav);
  g_render_state.modified = true;
  return true;
}


void d3d::set_variable_rate_shading(unsigned, unsigned, VariableRateShadingCombiner, VariableRateShadingCombiner)
{
  // could be supported with nvapi? it is mentioned on the nv vrworks page.
}
void d3d::set_variable_rate_shading_texture(BaseTexture *)
{
  // could be supported with nvapi? it is mentioned on the nv vrworks page.
}

void d3d::set_stream_output_buffer(int, const StreamOutputBufferSetup &) {}

void d3d::resource_barrier(const ResourceBarrierDesc &, GpuPipeline) {}
