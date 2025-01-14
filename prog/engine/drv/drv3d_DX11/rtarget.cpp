// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/vector_map.h>
#include <generic/dag_tab.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <debug/dag_debug.h>
#include <math/integer/dag_IPoint2.h>
#include <convertHelper.h>

#include <vecmath/dag_vecMath.h>
#include <math/dag_vecMathCompatibility.h>

#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_renderStates.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_texture.h>

#include "driver.h"
#include "texture.h"
#include "clear.h"
#if HAS_NVAPI
#include <nvapi.h>
#endif

using namespace drv3d_dx11;

// RenderState &g_frameState = g_render_state; //ok for strict aliasing?
// #define g_frameState g_render_state
// #include "frameStateTM.inc.cpp"
// #undef g_frameState

namespace drv3d_dx11
{
extern bool get_render_state_target_size(int &w, int &h);
static bool check_rect(const RectInt &rect)
{
  if (rect.left < 0 || rect.top < 0 || rect.right <= rect.left || rect.bottom <= rect.top)
  {
    G_ASSERTF(0, "Invalid rect: %d, %d - %d, %d", rect.left, rect.top, rect.right, rect.bottom);
    return false;
  }
  return true;
}

ID3D11RenderTargetView *getRenderTargetView(const Driver3dRenderTarget &rtState, int i)
{
  G_ASSERT(rtState.isColorUsed(i));
  /*const*/ BaseTexture *btex = rtState.color[i].tex;
  /*const*/ BaseTex *tex = (BaseTex *)((btex != NULL) ? btex : g_driver_state.backBufferColorTex);
  return (ID3D11RenderTargetView *)tex->getRtView(rtState.color[i].face, rtState.color[i].level, 1,
    btex == NULL && g_render_state.srgb_bb_write ? true : false);
}

ID3D11DepthStencilView *getDepthStencilView(const Driver3dRenderTarget &rtState)
{
  G_ASSERT(rtState.isDepthUsed());
  BaseTex *tex = NULL;
  if (rtState.depth.tex)
  {
    tex = (BaseTex *)rtState.depth.tex;
#if DAGOR_DBGLEVEL > 0
    tex->wasUsed = 1;
#endif
  }

  G_ASSERT(tex != NULL);
  bool depthReadOnly = featureLevelsSupported >= D3D_FEATURE_LEVEL_11_0 && rtState.isDepthReadOnly();
  int slice = (tex && (tex->restype() == RES3D_ARRTEX || tex->restype() == RES3D_CUBETEX)) ? rtState.depth.face : 0;
  return tex ? (ID3D11DepthStencilView *)tex->getRtView(slice, 0, 1, depthReadOnly) : NULL;
}


bool init_rendertargets(RenderState &rs)
{
  rs.nextRtState.reset();
  rs.nextRtState.setBackbufColor();
  rs.nextRtState.removeDepth();
  rs.currRtState = rs.nextRtState;
  return true;
}

// set mrt
// set viewport
// set scissor
// set raster state
void flush_rendertargets(RenderState &rs)
{
  for (int i = 0; i < Driver3dRenderTarget::MAX_SIMRT; i++)
  {
    if (rs.nextRtState.isColorUsed(i))
    {
      BaseTexture *btex = rs.nextRtState.color[i].tex;
      if (btex)
        ((BaseTex *)btex)->wasCopiedToStage = 0;
    }
  }
  if (rs.rtModified || rs.texFetchState.uavState[STAGE_PS].uavModifiedMask)
  {
    Driver3dRenderTarget &rt = rs.nextRtState;

    if ((rt.used & Driver3dRenderTarget::TOTAL_MASK) == 0 && !rs.texFetchState.uavState[STAGE_PS].uavsUsed)
    {
      D3D_ERROR("no render target set!");
      // TODO correct error
      return;
    }

    rs.rtModified = false;
    rs.texFetchState.uavState[STAGE_PS].uavModifiedMask = 0;

    // D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT
    ID3D11RenderTargetView *color[Driver3dRenderTarget::MAX_SIMRT];
    int maxTarget = -1;
#if DAGOR_DBGLEVEL > 0
    int testWidth = 0;
    int testHeight = 0;
#endif
    for (int i = 0; i < countof(color); i++)
    {
      // TODO: reset aliased bind points
      if (rt.isColorUsed(i))
      {
        color[i] = getRenderTargetView(rt, i);
        maxTarget = i;

#if DAGOR_DBGLEVEL > 0
        int w, h;
        d3d::get_render_target_size(w, h, rt.color[i].tex, rt.color[i].level);
        G_ASSERTF((testWidth == 0 && testHeight == 0) || (w == testWidth && h == testHeight), "%dx%d != %dx%d", testWidth, testHeight,
          w, h);
        testWidth = w;
        testHeight = h;
#endif
      }
      else
        color[i] = NULL;
    }

    ID3D11DepthStencilView *depth = rt.isDepthUsed() ? getDepthStencilView(rt) : NULL;

#if DAGOR_DBGLEVEL > 0
    if (rt.isDepthUsed() && rt.depth.tex) // Skip test of auto-created depth.
    {
      int w, h;
      d3d::get_render_target_size(w, h, rt.depth.tex, rt.depth.level);
      G_ASSERTF((testWidth == 0 && testHeight == 0) || (w == testWidth && h == testHeight), "%dx%d != %dx%d", testWidth, testHeight, w,
        h);
    }
#endif

    ContextAutoLock contextLock;
    if (!rs.texFetchState.uavState[STAGE_PS].uavsUsed)
    {
      dx_context->OMSetRenderTargets(maxTarget + 1, color, depth);
      rs.maxUsedTarget = maxTarget;
    }
    else
    {
      int first_uav = -1, last_uav = -1;
      for (int i = 0; i < rs.texFetchState.uavState[STAGE_PS].uavs.size(); i++)
        if (rs.texFetchState.uavState[STAGE_PS].uavs[i])
        {
          if (first_uav < 0)
            first_uav = i;

          last_uav = i;
        }

      const bool haveUav = first_uav >= 0;
      if (haveUav)
      {
        G_ASSERTF(last_uav < 8, "last_uav=%d", last_uav);
        rs.maxUsedTarget = max(maxTarget, last_uav);

        // On conflict, RTs are prioritized over UAVs. So, the actual UAV slots that we will set should not intersect with RT slots.
        // This is expected to be a normal situation because of UAVs set from stcode, so no logs.
        // See https://youtrack.gaijin.team/issue/RE-2419
        first_uav = max(first_uav, maxTarget + 1);
        last_uav = max(last_uav, maxTarget + 1);
      }
      else
        rs.maxUsedTarget = maxTarget;

      static UINT initialCounts[MAX_UAV] = {0}; // should be -1 or 0, however, we don't use counters at all
      dx_context->OMSetRenderTargetsAndUnorderedAccessViews(maxTarget + 1, color, depth, haveUav ? first_uav : 0,
        haveUav ? last_uav - first_uav + 1 : 0, haveUav ? &rs.texFetchState.uavState[STAGE_PS].uavs[first_uav] : NULL,
        haveUav ? initialCounts : NULL);
    }

    Stat3D::updateRenderTarget();
    rs.currRtState = rs.nextRtState;
  }
}


void flush_null_rendertargets(RenderState &rs)
{
  if ((rs.rtModified || rs.texFetchState.uavState[STAGE_PS].uavModifiedMask) && rs.maxUsedTarget >= -1)
  {
    ID3D11RenderTargetView *color[Driver3dRenderTarget::MAX_SIMRT];
    for (int i = 0; i <= rs.maxUsedTarget; ++i)
      color[i] = nullptr;

    ContextAutoLock contextLock;
    dx_context->OMSetRenderTargets(rs.maxUsedTarget + 1, color, NULL);
    Stat3D::updateRenderTarget();
    rs.maxUsedTarget = -2;
  }
}
void flush_null_cs_rendertargets(RenderState &rs)
{
  if (rs.maxUsedTarget >= -1) // normal rt is up to -1 AND depth. so <-1 means we already had called flush_null_rendertargets
  {
    ID3D11RenderTargetView *color[Driver3dRenderTarget::MAX_SIMRT];
    for (int i = 0; i <= rs.maxUsedTarget; ++i)
      color[i] = nullptr;

    ContextAutoLock contextLock;
    dx_context->OMSetRenderTargets(rs.maxUsedTarget + 1, color, NULL);
    Stat3D::updateRenderTarget();
    rs.maxUsedTarget = -2;
    rs.rtModified = true; // rt IS modified after that. Ensure we call flush_all on normal RT correctly
  }
}

void close_rendertargets() {}

void resolve_msaa_and_gen_mips(const Driver3dRenderTarget &rtState)
{
  static int lastFrameDip = 0;
  if (lastFrameDip == g_render_state.currentFrameDip)
    return;
  lastFrameDip = g_render_state.currentFrameDip;

  for (auto mrtNo : LsbVisitor{rtState.used & Driver3dRenderTarget::COLOR_MASK})
  {
    if (auto tex = (BaseTex *)rtState.color[mrtNo].tex; tex && tex->tex.resolvedTex)
    {
      tex->dirtyRt |= (1 << (rtState.color[mrtNo].face));
    }
  }
}

void remove_texture_from_samplers(BaseTexture *tex)
{
  ResAutoLock resLock;
  const auto &resources = g_render_state.texFetchState.resources;
  for (const TextureFetchState::Samplers &samplers : resources)
    for (const SamplerState &ss : samplers.resources)
      if (ss.texture == tex)
        d3d::set_tex(&samplers - resources.data(), &ss - samplers.resources.data(), NULL);
}

void remove_view_from_uav(ID3D11UnorderedAccessView *view)
{
  if (!view)
    return;
  ResAutoLock resLock;
  const auto &resources = g_render_state.texFetchState.uavState;
  for (const TextureFetchState::UAV &uav : resources)
    for (const auto &uaView : uav.uavs)
      if (uaView == view)
        d3d::set_rwtex(&uav - resources.data(), &uaView - uav.uavs.data(), nullptr, 0, 0);
}

void remove_view_from_uav_ignore_slot(int shader_stage, int ignore_slot, ID3D11UnorderedAccessView *view)
{
  if (!view)
    return;
  ResAutoLock resLock;
  const auto &resources = g_render_state.texFetchState.uavState;
  const auto &uav = resources[shader_stage];
  for (int slot = 0; slot < uav.uavs.size(); slot++)
  {
    const auto &uaView = uav.uavs[slot];
    if (uaView == view && ignore_slot != slot)
      d3d::set_rwtex(shader_stage, slot, nullptr, 0, 0);
  }
}

void remove_buffer_from_slot(Sbuffer *buf)
{
  ResAutoLock resLock;
  const auto &resources = g_render_state.texFetchState.resources;
  for (const TextureFetchState::Samplers &samplers : resources)
    for (const SamplerState &ss : samplers.resources)
      if (ss.buffer == buf)
        d3d::set_buffer(&samplers - resources.data(), &ss - samplers.resources.data(), nullptr);
}
}; // namespace drv3d_dx11


static float fullScrTri[3][4] = {{-1, +1, -1, +1}, {-1, -3, -1, -3}, {+3, +1, +3, +1}};

namespace drv3d_dx11
{

extern shaders::DriverRenderStateId current_render_state;
extern shaders::DriverRenderStateId stretch_prepare_render_state;
extern eastl::vector_map<int, shaders::DriverRenderStateId> clear_view_states;

}; // namespace drv3d_dx11

bool d3d::set_render_target(const Driver3dRenderTarget &rt)
{
  CHECK_MAIN_THREAD();


  RenderState &rs = g_render_state;
  resolve_msaa_and_gen_mips(rs.nextRtState);
  rs.nextRtState = rt;
  rs.modified = rs.rtModified = true;
  rs.viewModified = VIEWMOD_FULL;
  return true;
}

void d3d::get_render_target(Driver3dRenderTarget &out_rt)
{
  CHECK_MAIN_THREAD();
  out_rt = g_render_state.nextRtState;
}

bool d3d::set_render_target()
{
  CHECK_MAIN_THREAD();
  RenderState &rs = g_render_state;

  resolve_msaa_and_gen_mips(rs.nextRtState);
  rs.modified = rs.rtModified = true;
  rs.viewModified = VIEWMOD_FULL;
  rs.nextRtState.setBackbufColor();
  rs.nextRtState.removeDepth();
  return true;
}

bool d3d::set_depth(BaseTexture *tex, DepthAccess access)
{
  RenderState &rs = g_render_state;

  rs.modified = rs.rtModified = true;
  if (tex == NULL)
    rs.nextRtState.removeDepth();
  else
  {
#if DAGOR_DBGLEVEL > 0
    if (!is_depth_format_flg(((BaseTex *)tex)->cflg))
    {
      D3D_ERROR("can't set_depth with non depth texture <%s>", ((BaseTex *)tex)->getResName());
      return false;
    }
#endif
    BaseTex *bt = (BaseTex *)tex;
    if (bt->needs_clear)
      bt->clear();
    if (access == DepthAccess::RW)
      remove_texture_from_samplers(tex);
    rs.nextRtState.setDepth(tex, access == DepthAccess::SampledRO);
  }
  return true;
}

bool d3d::set_depth(BaseTexture *tex, int face, DepthAccess access)
{
  RenderState &rs = g_render_state;

  rs.modified = rs.rtModified = true;
  if (tex == NULL)
    rs.nextRtState.removeDepth();
  else
  {
    BaseTex *bt = (BaseTex *)tex;
#if DAGOR_DBGLEVEL > 0
    if (!is_depth_format_flg(bt->cflg))
    {
      D3D_ERROR("can't set_depth with non depth texture");
      return false;
    }
#endif
    if (bt->needs_clear)
      bt->clear();
    if (access == DepthAccess::RW && bt->maxMipLevel <= face && face <= bt->minMipLevel)
      remove_texture_from_samplers(tex);
    rs.nextRtState.setDepth(tex, face, access == DepthAccess::SampledRO);
  }
  return true;
}

bool d3d::set_render_target(int rt_index, BaseTexture *tex, uint8_t level)
{
  if (tex)
    CHECK_MAIN_THREAD();

  RenderState &rs = g_render_state;
  rs.modified = rs.rtModified = true;
  resolve_msaa_and_gen_mips(rs.nextRtState);

  G_ASSERT(rt_index >= 0 && rt_index < Driver3dRenderTarget::MAX_SIMRT);

  if (tex == NULL)
  {
    rs.nextRtState.removeColor(rt_index);
    if (rt_index == 0)
      rs.viewModified = VIEWMOD_FULL;
    return true;
  }

  BaseTex *bt = (BaseTex *)tex;
  if (bt->needs_clear)
    bt->clear();
  uint32_t cflg = bt->cflg;
  if (!(cflg & TEXCF_RTARGET))
  {
    D3D_ERROR("can't set_render_target with non rtarget texture %s", tex->getTexName());
    return false;
  }
  if (is_depth_format_flg(cflg))
  {
    D3D_ERROR("can't set_render_target with depth texture %s, use set_depth instead", tex->getTexName());
    return false;
  }

  if (bt->maxMipLevel <= level && level <= bt->minMipLevel)
    remove_texture_from_samplers(tex);

  rs.nextRtState.setColor(rt_index, tex, level, 0);

  if (rt_index == 0)
  {
    rs.nextRtState.removeDepth();
    rs.viewModified = VIEWMOD_FULL;
  }

  return true;
}

bool d3d::set_render_target(int rt_index, BaseTexture *tex, int fc, uint8_t level)
{
  CHECK_MAIN_THREAD();
  RenderState &rs = g_render_state;
  rs.modified = rs.rtModified = true;
  resolve_msaa_and_gen_mips(rs.nextRtState);

  G_ASSERT(rt_index >= 0 && rt_index < Driver3dRenderTarget::MAX_SIMRT);

  if (tex == NULL)
  {
    rs.nextRtState.removeColor(rt_index);
    if (rt_index == 0)
      rs.viewModified = VIEWMOD_FULL;
    return true;
  }

  uint32_t cflg = ((BaseTex *)tex)->cflg;
  BaseTex *bt = (BaseTex *)tex;
  if (bt->needs_clear)
    bt->clear();
  if (!(cflg & TEXCF_RTARGET))
  {
    D3D_ERROR("can't set_render_target with non rtarget texture %s", tex->getTexName());
    return false;
  }
  if (is_depth_format_flg(cflg))
  {
    D3D_ERROR("can't set_render_target with depth texture %s, use set_depth instead", tex->getTexName());
    return false;
  }

  if (bt->maxMipLevel <= level && level <= bt->minMipLevel)
    remove_texture_from_samplers(tex);

  rs.nextRtState.setColor(rt_index, tex, level, fc);

  if (rt_index == 0)
  {
    rs.nextRtState.removeDepth();
    rs.viewModified = VIEWMOD_FULL;
  }
  return true;
}

bool d3d::get_target_size(int &w, int &h)
{
  Driver3dRenderTarget &rt = g_render_state.nextRtState;
  return get_render_state_target_size(w, h);
}

bool d3d::get_render_target_size(int &w, int &h, BaseTexture *rt_tex, uint8_t level)
{
  w = h = 0;

  if (rt_tex == NULL)
  {
    Driver3dRenderTarget &rt = g_render_state.currRtState;
    if (rt.isBackBufferColor())
    {
      d3d::get_screen_size(w, h);
      return true;
    }
    w = g_driver_state.width;
    h = g_driver_state.height;
    return true;
  }

  BaseTex *bt = (BaseTex *)rt_tex;
  if (!(bt->cflg & TEXCF_RTARGET))
  {
    return false;
  }

  G_ASSERT(level < bt->mipLevels);
  w = max(1, bt->width >> level);
  h = max(1, bt->height >> level);

  return true;
}

bool d3d::clearview(int write_mask, E3DCOLOR c, float z_value, uint32_t stencil_value)
{
  CHECK_MAIN_THREAD();

  RenderState &rs = g_render_state;
  // Disable flushing of render state to avoid unnecessary validation of render state.
  // Anyway we don't render here and can flush this part later.
  bool rasterizerModified = rs.rasterizerModified;
  bool depthStencilModified = rs.depthStencilModified;
  rs.rasterizerModified = rs.depthStencilModified = false;
  flush_all(false);
  rs.rasterizerModified = rasterizerModified;
  rs.depthStencilModified = depthStencilModified;
  rs.modified = rasterizerModified || depthStencilModified;

  int targetW = 1, targetH = 1;

  get_render_state_target_size(targetW, targetH);
  G_ASSERT(rs.nextRtState.isColorUsed() || rs.nextRtState.isDepthUsed());

  RasterizerState &rz = rs.nextRasterizerState;

  bool fastClear = rs.nextRasterizerState.viewport.x == 0 && rs.nextRasterizerState.viewport.y == 0 &&
                   rs.nextRasterizerState.viewport.w == targetW && rs.nextRasterizerState.viewport.h == targetH;

  if (fastClear)
  {
    if (write_mask & CLEAR_TARGET)
    {
      float color[4] = {
        normalizeColor(c.r),
        normalizeColor(c.g),
        normalizeColor(c.b),
        normalizeColor(c.a),
      };
      for (auto i : LsbVisitor{rs.currRtState.used & Driver3dRenderTarget::COLOR_MASK})
      {
        ID3D11RenderTargetView *rtView = getRenderTargetView(rs.currRtState, i);
        ContextAutoLock contextLock;
        dx_context->ClearRenderTargetView(rtView, color);
      }
    }

    if (rs.currRtState.isDepthUsed() && (write_mask & (CLEAR_ZBUFFER | CLEAR_STENCIL)))
    {
      ID3D11DepthStencilView *depthView = getDepthStencilView(rs.currRtState);
      ContextAutoLock contextLock;
      dx_context->ClearDepthStencilView(depthView,
        ((write_mask & CLEAR_ZBUFFER) ? D3D11_CLEAR_DEPTH : 0) | ((write_mask & CLEAR_STENCIL) ? D3D11_CLEAR_STENCIL : 0), z_value,
        stencil_value);
    }
  }
  else
  {
    float color[4] = {
      normalizeColor(c.r),
      normalizeColor(c.g),
      normalizeColor(c.b),
      normalizeColor(c.a),
    };

    ResAutoLock resLock;
    MiniRenderStateUnsafe savedRs;
    savedRs.store();

    clear_slow(write_mask, color, z_value, stencil_value);

    savedRs.restore();
  }

  return true;
}

#include <renderPassGeneric.cpp.inl>

void d3d::clear_render_pass(const RenderPassTarget &target, const RenderPassArea &area, const RenderPassBind &bind)
{
  TextureInfo info;
  target.resource.tex->getinfo(info, target.resource.mip_level);

  float colorClearValue[4]{};
  if (bind.slot != RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL)
  {
    resource_clear_value_to_float4(info.cflg, target.clearValue, colorClearValue);
  }

  const bool fastClear = area.top == 0 && area.left == 0 && area.height == info.h && area.width == info.w;
  if (fastClear)
  {
    const bool isDepth = bind.slot == RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL;
    BaseTex *texture = (BaseTex *)target.resource.tex;
    ID3D11View *view =
      texture->getRtView(target.resource.layer, target.resource.mip_level, 1, isDepth ? false : g_render_state.srgb_bb_write);

    if (bind.slot == RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL)
    {
      ContextAutoLock contextLock;
      dx_context->ClearDepthStencilView((ID3D11DepthStencilView *)view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
        target.clearValue.asDepth, target.clearValue.asStencil);
    }
    else
    {
      ContextAutoLock contextLock;
      dx_context->ClearRenderTargetView((ID3D11RenderTargetView *)view, colorClearValue);
    }
  }
  else
  {
    ResAutoLock resLock;
    MiniRenderStateUnsafe savedRs;
    savedRs.store();

    int writeMask;
    if (bind.slot == RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL)
    {
      d3d::set_depth(target.resource.tex, DepthAccess::RW);
      writeMask = CLEAR_STENCIL | CLEAR_ZBUFFER;
    }
    else
    {
      d3d::set_render_target(target.resource.tex, 0);
      writeMask = CLEAR_TARGET;
    }
    d3d::setview(area.left, area.top, area.width, area.height, area.minZ, area.maxZ);
    clear_slow(writeMask, colorClearValue, target.clearValue.asDepth, target.clearValue.asStencil);

    savedRs.restore();
  }
}

static void stretch_prepare(BaseTexture *from)
{
  d3d::set_pixel_shader(g_default_copy_ps);
  d3d::set_vertex_shader(g_default_copy_vs);
  d3d::setvdecl(g_default_pos_vdecl);

  d3d::set_tex(STAGE_PS, 0, from, false);
  G_ASSERT(g_default_clamp_sampler != d3d::INVALID_SAMPLER_HANDLE);
  d3d::set_sampler(STAGE_PS, 0, g_default_clamp_sampler);
  if (!stretch_prepare_render_state)
  {
    shaders::RenderState state;
    state.independentBlendEnabled = 0;
    for (auto &blendParam : state.blendParams)
    {
      blendParam.ablend = 0;
      blendParam.sepablend = 0;
    }
    state.ztest = 0;
    state.zwrite = 0;
    state.zBias = 0.0f;
    state.slopeZBias = 0.0f;
    state.colorWr = WRITEMASK_ALL;
    state.cull = CULL_NONE;
    stretch_prepare_render_state = d3d::create_render_state(state);
  }
  d3d::set_render_state(stretch_prepare_render_state);
}

static bool try_copy_tex(BaseTexture *from, BaseTexture *to, RectInt *from_rect, RectInt *to_rect)
{
  if ((from_rect && !check_rect(*from_rect)) || (to_rect && !check_rect(*to_rect)))
    return false;

  BaseTex *fromBase = (BaseTex *)from;
  BaseTex *toBase = (BaseTex *)to;

  if (fromBase->format != toBase->format || (fromBase->cflg & TEXCF_SAMPLECOUNT_MASK) != (toBase->cflg & TEXCF_SAMPLECOUNT_MASK))
    return false;

  int srcw, srch;
  d3d::get_render_target_size(srcw, srch, from, 0);
  int dstw, dsth;
  d3d::get_render_target_size(dstw, dsth, to, 0);

  if (srcw == dstw && srch == dsth && !from_rect && !to_rect)
  {
    ContextAutoLock autolock;
#if DAGOR_DBGLEVEL > 0
    ((BaseTex *)to)->wasUsed = 1;
#endif
    disable_conditional_render_unsafe();
    VALIDATE_GENERIC_RENDER_PASS_CONDITION(!g_render_state.isGenericRenderPassActive,
      "DX11: try_copy_tex uses CopyResource inside a generic render pass");
    dx_context->CopyResource(((BaseTex *)to)->tex.texRes, ((BaseTex *)from)->tex.texRes);
    return true;
  }
  else
  {
    int dl = 0, dt = 0, dw = dstw, dh = dsth;
    int sl = 0, st = 0, sw = srcw, sh = srch;
    if (to_rect)
    {
      dl = to_rect->left;
      dt = to_rect->top;
      dw = min(int(dstw), int(to_rect->right - to_rect->left));
      dh = min(int(dsth), int(to_rect->bottom - to_rect->top));
    }
    if (from_rect)
    {
      sl = from_rect->left;
      st = from_rect->top;
      sw = min(int(srcw), int(from_rect->right - from_rect->left));
      sh = min(int(srch), int(from_rect->bottom - from_rect->top));
    }

    if (sw == dw && sh == dh)
    {
      ContextAutoLock autolock;
      D3D11_BOX srcBox;
      srcBox.left = sl;
      srcBox.top = st;
      srcBox.front = 0;
      srcBox.right = sl + sw;
      srcBox.bottom = st + sh;
      srcBox.back = 1;
#if DAGOR_DBGLEVEL > 0
      ((BaseTex *)to)->wasUsed = 1;
#endif
      disable_conditional_render_unsafe();
      VALIDATE_GENERIC_RENDER_PASS_CONDITION(!g_render_state.isGenericRenderPassActive,
        "DX11: try_copy_tex uses CopySubresourceRegion inside a generic render pass");
      dx_context->CopySubresourceRegion(((BaseTex *)to)->tex.texRes, 0, dl, dt, 0, ((BaseTex *)from)->tex.texRes, 0, &srcBox);
      return true;
    }
  }
  return false;
}

bool d3d::stretch_rect(BaseTexture *from, BaseTexture *to, RectInt *from_rect, RectInt *to_rect)
{
  CHECK_THREAD;

  if ((from_rect && !check_rect(*from_rect)) || (to_rect && !check_rect(*to_rect)))
    return false;

  if (from)
  {
    BaseTex *bt = (BaseTex *)from;
    if (bt->needs_clear)
      bt->clear();
  }

  ResAutoLock resLock;
  MiniRenderStateUnsafe rs;
  rs.store();

  if (!from) // Get current backbuffer texture.
  {
    ContextAutoLock contextLock;
    G_ASSERT_RETURN(swap_chain, false);
    ID3D11Texture2D *backBuffer = NULL;
    HRESULT hr = swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&backBuffer); // -V522
    G_ASSERT(SUCCEEDED(hr));
    G_ASSERT(backBuffer);

    BaseTex *backBufferTex = (BaseTex *)g_driver_state.backBufferColorTex;
    G_ASSERT(backBufferTex);

    if (backBufferTex->tex.tex2D)
      backBufferTex->tex.tex2D->Release();
    backBufferTex->tex.tex2D = backBuffer;
  }

  if (!from_rect && !to_rect && from && to)
  {
    if (from->restype() != RES3D_TEX || to->restype() != RES3D_TEX)
    {
      DEBUG_CTX("wrong tex formats");
      rs.restore();
      return false;
    }
    if (try_copy_tex(from, to, NULL, NULL))
      return true;
    d3d::set_render_target((Texture *)to, 0);
    stretch_prepare(from);
    d3d::draw_up(PRIM_TRILIST, 1, fullScrTri, sizeof(fullScrTri[0]));

    rs.restore();
    return true;
  }
  else if (!from_rect && !to_rect && from)
  {

    if (from->restype() != RES3D_TEX)
    {
      DEBUG_CTX("n/a");
      rs.restore();
      return false;
    }
    d3d::set_render_target();
    stretch_prepare(from);
    d3d::draw_up(PRIM_TRILIST, 1, fullScrTri, sizeof(fullScrTri[0]));
    rs.restore();
    return true;
  }
  else if (from)
  {
    if (from->restype() != RES3D_TEX)
    {
      DEBUG_CTX("n/a");
      rs.restore();
      return false;
    }
    if (to && try_copy_tex(from, to, from_rect, to_rect))
      return true;

    d3d::set_render_target();
    if (to)
      d3d::set_render_target((Texture *)to, 0);
    stretch_prepare(from);
    if (to_rect || from_rect)
    {
      BaseTex *src_bt = (BaseTex *)from;
      int srcw = src_bt->width, srch = src_bt->height;
      int dstw, dsth;
      d3d::get_render_target_size(dstw, dsth, to, 0);
      Point2 toLT, toRB, fromLT, fromRB;
      if (from_rect)
      {
        fromLT = Point2(from_rect->left / (srcw - 0.f), 1. - from_rect->top / (srch - 0.f));
        fromRB = Point2(from_rect->right / (srcw - 0.f), 1. - from_rect->bottom / (srch - 0.f));
      }
      else
        fromLT = Point2(0, 1), fromRB = Point2(1, 0);
      if (to_rect)
      {
        toLT = Point2(to_rect->left / (dstw - 0.f), 1. - to_rect->top / (dsth - 0.f));
        toRB = Point2(to_rect->right / (dstw - 0.f), 1. - to_rect->bottom / (dsth - 0.f));
      }
      else
        toLT = Point2(0, 1), toRB = Point2(1, 0);
      Point4 quad[4];
      quad[0] = Point4(toLT.x, toLT.y, fromLT.x, fromLT.y) * 2. - Point4(1., 1., 1., 1.);
      quad[1] = Point4(toRB.x, toLT.y, fromRB.x, fromLT.y) * 2. - Point4(1., 1., 1., 1.);
      quad[2] = Point4(toLT.x, toRB.y, fromLT.x, fromRB.y) * 2. - Point4(1., 1., 1., 1.);
      quad[3] = Point4(toRB.x, toRB.y, fromRB.x, fromRB.y) * 2. - Point4(1., 1., 1., 1.);

      d3d::draw_up(PRIM_TRISTRIP, 2, quad, sizeof(Point4));
    }
    else
      d3d::draw_up(PRIM_TRILIST, 1, fullScrTri, sizeof(fullScrTri[0]));
    rs.restore();
    return true;
  }
  else if (to)
  {
    if (try_copy_tex(g_driver_state.backBufferColorTex, to, from_rect, to_rect))
      return true;
    return stretch_rect(g_driver_state.backBufferColorTex, to, from_rect, to_rect);
    /*D3D_ERROR("n/a: can not stretch from backbuffer of format %d to texture of format %d",
      getbasetex(g_driver_state.backBufferColorTex)->format, getbasetex(to)->format);
    debug_dump_stack("d3d::stretch_rect()");
    return false;*/
  }


  debug("n/a: from=%p to=%p from_rect=%p to_rect=%p", from, to, from_rect, to_rect);
  debug_dump_stack("d3d::stretch_rect()");
  return false;
}

bool d3d::copy_from_current_render_target(BaseTexture *to_tex)
{
  CHECK_THREAD;

  Driver3dRenderTarget rt;
  d3d::get_render_target(rt);
  d3d::stretch_rect(rt.isBackBufferColor() ? 0 : rt.color[0].tex, to_tex);
  d3d::set_render_target(rt);

  return true;
}

Texture *d3d::get_backbuffer_tex() { return g_driver_state.backBufferColorTex; }
Texture *d3d::get_secondary_backbuffer_tex() { return nullptr; }

/*bool d3d::copy_from_current_render_target(BaseTexture *to_tex)
{
   return d3d::stretch_rect(nextRtState.color[0].tex, to_tex);
}*/

#include <legacyCaptureImpl.cpp.inl>
