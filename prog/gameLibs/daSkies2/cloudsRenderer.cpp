// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "cloudsRenderer.h"
#include "cloudsShaderVars.h"

#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_resetDevice.h>
#include <drv/3d/dag_info.h>

#include <3d/dag_textureIDHolder.h>
#include <shaders/dag_shaderVarsUtils.h>

#include <math/dag_frustum.h>

#include <render/set_reprojection.h>
#include <render/viewVecs.h>

#include <EASTL/optional.h>
#include <util/dag_convar.h>

#include <osApiWrappers/dag_miscApi.h>

CONSOLE_BOOL_VAL("clouds", use_compute, true);

void CloudsRenderer::renderTiledDist(CloudsRendererData &data)
{
  if (!data.clouds_tile_distance)
    return;
  TIME_D3D_PROFILE(tiledDist);
  if (data.useCompute)
  {
    TextureInfo ti;
    data.clouds_tile_distance_tmp->getinfo(ti, 0);
    d3d::set_rwtex(STAGE_CS, 0, data.clouds_tile_distance_tmp.getTex2D(), 0, 0);
    clouds_tile_dist_prepare_cs->dispatchThreads(ti.w, ti.h, 1);
    d3d::set_rwtex(STAGE_CS, 0, data.clouds_tile_distance.getTex2D(), 0, 0);
    clouds_tile_dist_min_cs->dispatchThreads(ti.w, ti.h, 1);
    d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
  }
  else
  {
    d3d::set_render_target(data.clouds_tile_distance_tmp.getTex2D(), 0, 0);
    clouds_tile_dist_prepare_ps.render();
    d3d::set_render_target(data.clouds_tile_distance.getTex2D(), 0, 0);
    clouds_tile_dist_min_ps.render();
  }
  if (clouds_tile_dist_count_cs && data.clouds_close_layer_is_outside)
  {
    TIME_D3D_PROFILE(non_empty_tiles);
    TextureInfo ti;
    data.clouds_tile_distance_tmp->getinfo(ti, 0);
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), data.clouds_close_layer_is_outside.getBuf());
    clouds_tile_dist_count_cs->dispatchThreads(ti.w, ti.h, 1);
  }
}

void CloudsRenderer::init()
{
  clouds2_apply.init("clouds2_apply");
  if (d3d::get_driver_desc().shaderModel >= 5.0_sm)
  {
#define INIT(a) a.reset(new_compute_shader(#a, false));
    INIT(clouds2_temporal_cs);
    INIT(clouds2_close_temporal_cs);
    INIT(clouds2_taa_cs);
    INIT(clouds2_taa_cs_has_empty);
    INIT(clouds2_taa_cs_no_empty);
    INIT(clouds_close_layer_outside);
    INIT(clouds_close_layer_clear);
    INIT(clouds_tile_dist_count_cs);
    INIT(clouds_tile_dist_prepare_cs);
    INIT(clouds_tile_dist_min_cs);
    INIT(clouds_create_indirect);
    INIT(clouds2_apply_blur_cs)
#undef INIT
  }
#define INIT(a) a.init(#a);
  INIT(clouds2_temporal_ps)
  INIT(clouds2_close_temporal_ps)
  INIT(clouds2_taa_ps)
  INIT(clouds2_taa_ps_has_empty)
  INIT(clouds2_taa_ps_no_empty)
  INIT(clouds2_direct)
  INIT(clouds_tile_dist_prepare_ps)
  INIT(clouds_tile_dist_min_ps)
  INIT(clouds2_apply_has_empty)
  INIT(clouds2_apply_no_empty)
  INIT(clouds2_apply_blur_ps)
#undef INIT
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Border;
    smpInfo.filter_mode = d3d::FilterMode::Point;
    smpInfo.border_color = d3d::BorderColor::Color::TransparentBlack;
    ShaderGlobal::set_sampler(clouds_depth_gbuf_samplerstateVarId, d3d::request_sampler(smpInfo));
  }
  {
    d3d::SamplerInfo smpInfo;
    // Maybe it's a bug and border address mode should be used instead of wrap, but before the refactor sampler that was used by the
    // texture had wrap address mode
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Wrap;
    smpInfo.filter_mode = d3d::FilterMode::Point;
    ShaderGlobal::set_sampler(clouds_prev_depth_gbuf_samplerstateVarId, d3d::request_sampler(smpInfo));
  }
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.filter_mode = d3d::FilterMode::Point;
    ShaderGlobal::set_sampler(clouds_target_depth_gbuf_samplerstateVarId, d3d::request_sampler(smpInfo));
  }
}

CloudsRenderer::DispatchGroups2D CloudsRenderer::set_dispatch_groups(int w, int h, int w_sz, int h_sz, bool lowres_close_clouds)
{
  union
  {
    Color4 dgf;
    struct
    {
      int x, y, z, w;
    } dg;
  } castDG;
  castDG.dg.x = (w + w_sz - 1) / w_sz;
  castDG.dg.y = (h + h_sz - 1) / h_sz;
  castDG.dg.z = ((lowres_close_clouds ? w / 2 : w) + w_sz - 1) / w_sz;
  castDG.dg.w = ((lowres_close_clouds ? h / 2 : h) + h_sz - 1) / h_sz;
  ShaderGlobal::set_color4(clouds2_dispatch_groupsVarId, castDG.dgf);
  return DispatchGroups2D{castDG.dg.x, castDG.dg.y, castDG.dg.z, castDG.dg.w};
}

int CloudsRenderer::getNotLesserDepthLevel(CloudsRendererData &data, int &depth_levels, Texture *depth)
{
  depth_levels = 1;
  if (!depth)
    return 0;
  depth_levels = depth->level_count();
  if (!data.w)
    return 0;
  TextureInfo depthInfo;
  depth->getinfo(depthInfo, 0);
  int level = 0;
  for (int mw = depthInfo.w, mh = depthInfo.h; mw >= data.w && mh >= data.h && level < depth_levels; mw >>= 1, mh >>= 1, level++)
    ;
  level = max(0, level - 1);
  return level;
}

void CloudsRenderer::setCloudsOriginOffset(float world_size)
{
  ShaderGlobal::set_color4(clouds_origin_offsetVarId, P2D(currentCloudsOffsetCalc(world_size)), 0, 0);
}

void CloudsRenderer::setCloudsOffsetVars(float current_clouds_offset, float world_size)
{
  ShaderGlobal::set_real(clouds_offsetVarId, current_clouds_offset);
  setCloudsOriginOffset(world_size);
}

void CloudsRenderer::render(CloudsRendererData &data, const TextureIDPair &depth, const TextureIDPair &prev_depth,
  const Point2 &wind_change_ofs, float world_size, const TMatrix &view_tm, const TMatrix4 &proj_tm,
  const DPoint3 *world_pos /*= nullptr*/, const bool acquare_new_resource, const bool set_camera_vars)
{
  if (!data.cloudsColorPoolRT)
    return;
  TIME_D3D_PROFILE(render_clouds);
  ScopeReprojection reprojectionScope;
  data.setVars();
  if (set_camera_vars)
    set_viewvecs_to_shader(view_tm, proj_tm);
  ShaderGlobal::set_real(clouds_restart_taaVarId, data.frameValid ? 0 : 2);

  if (acquare_new_resource)
  {
    data.clearTemporalData(get_d3d_full_reset_counter());
    data.frameValid = true;
    data.frameNo++;

    data.nextCloudsColor = nullptr;
    data.prevCloudsColor = nullptr;
    data.prevCloudsWeight = nullptr;
    data.cloudsBlurTextureColor = nullptr;
  }

  ShaderGlobal::set_int(clouds2_current_frameVarId, data.frameNo & 0xFFFF);
  setCloudsOffsetVars(data, world_size);
  const bool canBeInsideClouds = bool(data.clouds_color_close) && depth.getTex();

  ShaderGlobal::set_int(clouds_infinite_skiesVarId, depth.getTex2D() ? 0 : 1);
  ShaderGlobal::set_int(clouds_has_close_sequenceVarId, canBeInsideClouds ? 1 : 0);

  if (data.clouds_close_layer_is_outside)
  {
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), data.clouds_close_layer_is_outside.getBuf());
    if (canBeInsideClouds)
    {
      TIME_D3D_PROFILE(clouds_close_layer_is_outside_calc);
      TMatrix4 globtm;
      d3d::calcglobtm(view_tm, proj_tm, globtm);
      extern void set_frustum_planes(const Frustum &frustum);
      set_frustum_planes(Frustum(globtm));
      clouds_close_layer_outside->dispatch(1, 1, 1);
    }
    else
    {
      TIME_D3D_PROFILE(clouds_close_layer_clear);
      clouds_close_layer_clear->dispatch(1, 1, 1);
    }
  }
  bool useCompute = data.useCompute, taaUseCompute = data.taaUseCompute;
#if DAGOR_DBGLEVEL > 0
  useCompute &= use_compute.get();
  taaUseCompute &= use_compute.get();
#endif

  eastl::optional<ScopeRenderTarget> rtScope;
  if ((!useCompute || !taaUseCompute) && acquare_new_resource)
    rtScope.emplace();

  if (acquare_new_resource)
  {
    data.prevCloudsColor = eastl::move(data.cloudsTextureColor);
    data.nextCloudsColor = data.cloudsColorBlurPoolRT->acquire();
    data.cloudsTextureColor = data.cloudsColorPoolRT->acquire();

    data.prevCloudsWeight = eastl::move(data.cloudsTextureWeight);
    data.cloudsTextureWeight = data.cloudsWeightPoolRT->acquire();
  }
  G_ASSERT(data.nextCloudsColor != nullptr);
  G_ASSERT(data.prevCloudsColor != nullptr);
  G_ASSERT(data.prevCloudsWeight != nullptr);

  renderTiledDist(data);
  int depthLevels = 1;
  int level = getNotLesserDepthLevel(data, depthLevels, depth.getTex2D());
  ShaderGlobal::set_texture(clouds_depth_gbufVarId, depth.getId());
  DispatchGroups2D dg = set_dispatch_groups(data.w, data.h, CLOUD_TRACE_WARP_X, CLOUD_TRACE_WARP_Y, data.lowresCloseClouds);
  {
    if (level != 0 && depthLevels != 1 && depth.getTex2D())
      depth.getTex2D()->texmiplevel(level, level);
    TIME_D3D_PROFILE(current_clouds);
    // when blur is used edges of close object tend to be really exaggerated
    // so we ignoring them is a good idea
    STATE_GUARD(ShaderGlobal::set_int(clouds_ignore_close_objectsVarId, VALUE), data.useBlurredClouds, false);
    if (useCompute)
    {
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), data.nextCloudsColor->getTex2D());
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 1, VALUE, 0, 0), data.cloudsTextureDepth.getTex2D());
      clouds2_temporal_cs->dispatch(dg.x, dg.y, 1); // todo: measure optimal warp size
    }
    else
    {
      d3d::set_render_target(data.nextCloudsColor->getTex2D(), 0);
      d3d::set_render_target(1, data.cloudsTextureDepth.getTex2D(), 0);
      clouds2_temporal_ps.render();
    }
  }

  if (clouds_create_indirect.get() && data.cloudsIndirectBuffer)
  {
    TIME_D3D_PROFILE(create_indirect);
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), data.cloudsIndirectBuffer.getBuf());
    clouds_create_indirect->dispatch(1, 1, 1);
  }
  if (canBeInsideClouds)
  {
    // todo: use indirect
    if (depthLevels != 1 && level + 1 < depthLevels && depth.getTex2D() && data.lowresCloseClouds)
      depth.getTex2D()->texmiplevel(level + 1, level + 1);
    TIME_D3D_PROFILE(current_clouds_near);
    if (useCompute)
    {
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), data.clouds_color_close.getTex2D());
      if (clouds_create_indirect.get() && data.cloudsIndirectBuffer)
      {
        clouds2_close_temporal_cs->dispatch_indirect(data.cloudsIndirectBuffer.getBuf(),
          4 * 4 * (CLOUDS_HAS_CLOSE_LAYER + CLOUDS_APPLY_COUNT_PS));
      }
      else
      {
        clouds2_close_temporal_cs->dispatch(dg.z, dg.w, 1);
      }
    }
    else
    {
      d3d::set_render_target(data.clouds_color_close.getTex2D(), 0);
      if (clouds_create_indirect.get() && data.cloudsIndirectBuffer)
      {
        clouds2_close_temporal_ps.getElem()->setStates();
        d3d::draw_indirect(PRIM_TRILIST, data.cloudsIndirectBuffer.getBuf(), 4 * 4 * CLOUDS_HAS_CLOSE_LAYER);
      }
      else
        clouds2_close_temporal_ps.render();
    }
    d3d::resource_barrier({data.clouds_color_close.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }

  {
    if (depthLevels != 1)
    {
      if (depth.getTex2D())
        depth.getTex2D()->texmiplevel(level, level);
      if (prev_depth.getTex2D())
      {
        G_ASSERT(prev_depth.getTex2D()->level_count() == depth.getTex2D()->level_count());
        prev_depth.getTex2D()->texmiplevel(level, level);
      }
    }
    TIME_D3D_PROFILE(taa_clouds);
    ShaderGlobal::set_texture(clouds_prev_depth_gbufVarId, prev_depth.getId());
    data.prevWorldPos.x -= cloudsOfs.x + wind_change_ofs.x - currentCloudsOfs.x;
    data.prevWorldPos.z -= cloudsOfs.y + wind_change_ofs.y - currentCloudsOfs.y;
    G_UNUSED(world_pos);
    if (set_camera_vars)
      set_reprojection(view_tm, proj_tm, data.prevProjTm, data.prevWorldPos, data.prevGlobTm, data.prevViewVecLT, data.prevViewVecRT,
        data.prevViewVecLB, data.prevViewVecRB, world_pos);
    ShaderGlobal::set_texture(clouds_color_prevVarId, data.prevCloudsColor->getTexId());
    ShaderGlobal::set_texture(clouds_colorVarId, *data.nextCloudsColor);
    ShaderGlobal::set_texture(clouds_prev_taa_weightVarId, data.prevCloudsWeight->getTexId());
    if (taaUseCompute)
    {
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), data.cloudsTextureColor->getTex2D());
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 1, VALUE, 0, 0), data.cloudsTextureWeight->getTex2D());
      if (clouds_create_indirect.get() && data.cloudsIndirectBuffer)
      {
        clouds2_taa_cs_has_empty->dispatch_indirect(data.cloudsIndirectBuffer.getBuf(),
          4 * 4 * (CLOUDS_HAS_EMPTY + CLOUDS_APPLY_COUNT_PS));
        clouds2_taa_cs_no_empty->dispatch_indirect(data.cloudsIndirectBuffer.getBuf(),
          4 * 4 * (CLOUDS_NO_EMPTY + CLOUDS_APPLY_COUNT_PS));
      }
      else
        clouds2_taa_cs->dispatch(dg.x, dg.y, 1); // todo: measure optimal warp size
    }
    else
    {
      d3d::set_render_target(data.cloudsTextureColor->getTex2D(), 0);
      d3d::set_render_target(1, data.cloudsTextureWeight->getTex2D(), 0);
      d3d::clearview(CLEAR_DISCARD_TARGET, 0, 0, 0);
      if (clouds_create_indirect.get() && data.cloudsIndirectBuffer)
      {
        clouds2_taa_ps_has_empty.getElem()->setStates();
        d3d::draw_indirect(PRIM_TRILIST, data.cloudsIndirectBuffer.getBuf(), 4 * 4 * CLOUDS_HAS_EMPTY);
        set_program(clouds2_taa_ps_has_empty.getElem(), clouds2_taa_ps_no_empty.getElem());
        // clouds2_taa_ps_no_empty.getElem()->setStates();
        d3d::draw_indirect(PRIM_TRILIST, data.cloudsIndirectBuffer.getBuf(), 4 * 4 * CLOUDS_NO_EMPTY);
      }
      else
        clouds2_taa_ps.render();

      d3d::resource_barrier({data.cloudsTextureColor->getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    }
    if (depthLevels != 1)
    {
      if (depth.getTex2D())
        depth.getTex2D()->texmiplevel(-1, -1);
      if (prev_depth.getTex2D())
        prev_depth.getTex2D()->texmiplevel(-1, -1);
    }
  }

  if (data.useBlurredClouds)
  {
    ShaderGlobal::set_texture(clouds_colorVarId, *data.cloudsTextureColor);
    if (useCompute)
    {
      STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 0, VALUE, 0, 0), data.nextCloudsColor->getTex2D());
      clouds2_apply_blur_cs->dispatch(dg.x, dg.y, 1);
      d3d::resource_barrier({data.nextCloudsColor->getTex2D(), RB_RW_UAV | RB_STAGE_COMPUTE, 0, 0});
    }
    else
    {
      d3d::set_render_target(data.nextCloudsColor->getTex2D(), 0);
      clouds2_apply_blur_ps.render();
      d3d::resource_barrier({data.nextCloudsColor->getTex2D(), RB_RW_RENDER_TARGET | RB_STAGE_PIXEL, 0, 0});
    }
    ShaderGlobal::set_texture(clouds_colorVarId, *data.nextCloudsColor);
    G_ASSERT(data.cloudsBlurTextureColor == nullptr || !acquare_new_resource);
    data.cloudsBlurTextureColor = data.nextCloudsColor;
  }

  currentCloudsOfs = cloudsOfs;
}

void CloudsRenderer::renderDirect(CloudsRendererData &data)
{
  TIME_D3D_PROFILE(render_clouds_direct);
  G_UNUSED(data);
  if (d3d::get_driver_desc().caps.hasTileBasedArchitecture && VariableMap::isVariablePresent(clouds_direct_sequenceVarId) &&
      !ShaderGlobal::is_var_assumed(clouds_direct_sequenceVarId))
  {
    ShaderGlobal::set_int(clouds_direct_sequenceVarId, 1);
    clouds2_direct.render();
    ShaderGlobal::set_int(clouds_direct_sequenceVarId, 2);
    clouds2_direct.render();
    ShaderGlobal::set_int(clouds_direct_sequenceVarId, 3);
    clouds2_direct.render();
    ShaderGlobal::set_int(clouds_direct_sequenceVarId, 0);
  }
  else
    clouds2_direct.render();
}
/*static*/ void CloudsRenderer::set_program(ShaderElement *oe, ShaderElement *ne)
{
  uint32_t program;
  ShaderStateBlockId state_index;
  shaders::RenderStateId rstate;
  shaders::ConstStateIdx cstate;
  shaders::TexStateIdx tstate;
  int curVariant = get_dynamic_variant_states(ne->native(), program, state_index, rstate, cstate, tstate);
  uint32_t program2;
  ShaderStateBlockId state_index2;
  shaders::RenderStateId rstate2;
  shaders::ConstStateIdx cstate2;
  shaders::TexStateIdx tstate2;
  int curVariant2 = get_dynamic_variant_states(oe->native(), program2, state_index2, rstate2, cstate2, tstate2);
  if (curVariant2 != curVariant || rstate2 != rstate || state_index2 != state_index)
    ne->setStates();
  else
    d3d::set_program(program);
}

void CloudsRenderer::renderFull(CloudsRendererData &data, const TextureIDPair &downsampled_depth, TEXTUREID target_depth,
  const Point4 &target_depth_transform, const TMatrix &view_tm, const TMatrix4 &proj_tm)
{
  TIME_D3D_PROFILE(render_clouds);
  set_viewvecs_to_shader(view_tm, proj_tm);
  ShaderGlobal::set_texture(clouds_target_depth_gbufVarId, target_depth);
  ShaderGlobal::set_color4(clouds_target_depth_gbuf_transformVarId, target_depth_transform);
  ShaderGlobal::set_int(clouds_has_close_sequenceVarId, data.clouds_color_close.getTex2D() ? 1 : 0);
  ShaderGlobal::set_texture(clouds_depth_gbufVarId, downsampled_depth.getId());
  if (!data.cloudsColorPoolRT)
  {
    renderDirect(data);
    return;
  }
  TIME_D3D_PROFILE(apply_bilateral);
  data.setVars();
  ShaderGlobal::set_int(clouds_use_blur_applyVarId, data.useBlurredClouds);
  ShaderGlobal::set_texture(clouds_colorVarId, data.useBlurredClouds ? *data.cloudsBlurTextureColor : *data.cloudsTextureColor);
  int depthLevels;
  int level = getNotLesserDepthLevel(data, depthLevels, downsampled_depth.getTex2D());
  if (level != 0)
    downsampled_depth.getTex2D()->texmiplevel(level, level);
  if (clouds_create_indirect.get() && data.cloudsIndirectBuffer)
  {
    clouds2_apply_has_empty.getElem()->setStates();
    d3d::draw_indirect(PRIM_TRILIST, data.cloudsIndirectBuffer.getBuf(), 4 * 4 * CLOUDS_HAS_EMPTY);
    set_program(clouds2_apply_has_empty.getElem(), clouds2_apply_no_empty.getElem());
    // clouds2_apply_no_close_no_empty.getElem()->setStates();
    d3d::draw_indirect(PRIM_TRILIST, data.cloudsIndirectBuffer.getBuf(), 4 * 4 * CLOUDS_NO_EMPTY);
  }
  else
  {
    clouds2_apply.render();
  }

  if (data.useBlurredClouds)
    data.cloudsBlurTextureColor = nullptr;

  if (level != 0)
    downsampled_depth.getTex2D()->texmiplevel(-1, -1);
}
