// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daFx/dafx.h>
#include <de3_dynRenderService.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>

extern void dafx_sparksfx_set_context(dafx::ContextId ctx);
extern void dafx_modfx_set_context(dafx::ContextId ctx);
extern void dafx_compound_set_context(dafx::ContextId ctx);
extern void dafx_modfx_register_cpu_overrides(dafx::ContextId ctx);

inline void set_up_dafx_context(dafx::ContextId &dafx_ctx, dafx::CullingId &dafx_cull, dafx::CullingId &dafx_fom_cull)
{
  G_ASSERT(!dafx_ctx);

  dafx::Config cfg;
  cfg.use_async_thread = false;
  cfg.qualityMask = 1 << 2; // high quality
  cfg.sim_lods_enabled = true;
  cfg.gen_sim_lods_for_invisible_instances = true;

  dafx_ctx = dafx::create_context(cfg);
  if (dafx_ctx)
  {
    dafx::set_debug_flags(dafx_ctx, dafx::DEBUG_DISABLE_CULLING);

    eastl::vector<dafx::CullingDesc> descs;
    descs.push_back(dafx::CullingDesc("highres", dafx::SortingType::BACK_TO_FRONT));
    descs.push_back(dafx::CullingDesc("lowres", dafx::SortingType::BACK_TO_FRONT));
    descs.push_back(dafx::CullingDesc("underwater", dafx::SortingType::BACK_TO_FRONT));
    descs.push_back(dafx::CullingDesc("distortion", dafx::SortingType::BY_SHADER));
    dafx_cull = dafx::create_culling_state(dafx_ctx, descs);
    descs.clear();
    descs.push_back(dafx::CullingDesc("fom", dafx::SortingType::NONE));
    dafx_fom_cull = dafx::create_culling_state(dafx_ctx, descs);
    dafx_sparksfx_set_context(dafx_ctx);
    dafx_modfx_set_context(dafx_ctx);
    dafx_compound_set_context(dafx_ctx);
    dafx_modfx_register_cpu_overrides(dafx_ctx);
  }
}

inline void set_up_dafx_effect(dafx::ContextId dafx_ctx, BaseEffectObject *fx, bool force_recreate, bool enabled, const TMatrix *tm,
  bool is_emitter_tm)
{
  if (!fx || !dafx_ctx)
    return;

  TMatrix ident = TMatrix::IDENT;
  dafx::flush_command_queue(dafx_ctx);
  fx->setParam(_MAKE4C('PFXR'), &force_recreate);
  fx->setParam(_MAKE4C('PFXE'), &enabled);
  fx->setParam(HUID_EMITTER_TM, &ident);

  if (tm)
  {
    fx->setParam(is_emitter_tm ? HUID_EMITTER_TM : HUID_TM, const_cast<TMatrix *>(tm));
    Point4 p4 = Point4::xyz0(tm->getcol(3));
    fx->setParam(_MAKE4C('PFXP'), &p4);
  }

  dafx::flush_command_queue(dafx_ctx);
}

inline void set_dafx_wind_params(dafx::ContextId dafx_ctx, Point2 &wind_dir, float wind_power, float wind_scroll)
{
  dafx::set_global_value(dafx_ctx, "wind_dir", &wind_dir, 8);
  dafx::set_global_value(dafx_ctx, "wind_power", &wind_power, 4);
  dafx::set_global_value(dafx_ctx, "wind_scroll", &wind_scroll, 4);
}

inline void set_dafx_camera_velocity(dafx::ContextId dafx_ctx, Point3 &cam_vel)
{
  dafx::set_global_value(dafx_ctx, "camera_velocity", &cam_vel, 12);
}

inline void act_dafx(dafx::ContextId dafx_ctx, float dt)
{
  if (!dafx_ctx)
    return;

  dafx::set_global_value(dafx_ctx, "dt", &dt, 4);
  dafx::start_update(dafx_ctx, dt);
  dafx::finish_update(dafx_ctx);
}

inline void before_render_dafx(dafx::ContextId dafx_ctx, dafx::CullingId dafx_cull, dafx::Stats &dafx_stats, TMatrix4 &prevGlobTm)
{
  if (!dafx_ctx)
    return;

  TMatrix4 globtm;
  d3d::getglobtm(globtm);

  TMatrix4 viewTmCur;
  d3d::gettm(TM_VIEW, &viewTmCur);

  TMatrix itm;
  itm = orthonormalized_inverse(tmatrix(viewTmCur));

  Driver3dPerspective persp;
  d3d::getpersp(persp);

  int targetWidth, targetHeight;
  d3d::get_render_target_size(targetWidth, targetHeight, NULL, 0);

  IDynRenderService *renderService = EDITORCORE->queryEditorInterface<IDynRenderService>();
  if (renderService)
  {
    Texture *renderBuffer = renderService->getRenderBuffer();
    if (renderBuffer)
    {
      TextureInfo info;
      renderBuffer->getinfo(info);
      targetWidth = info.w;
      targetHeight = info.h;
    }
  }

  Point4 znZfar =
    Point4(persp.zn, persp.zf, safediv(persp.zn, persp.zn * persp.zf), safediv(persp.zf - persp.zn, persp.zn * persp.zf));
  Point2 targetSize(targetWidth, targetHeight);
  Point2 targetSizeRcp(safeinv((float)targetWidth), safeinv((float)targetHeight));

  static int sunLightDirVarId = ::get_shader_variable_id("from_sun_direction", true);
  Color4 sunLightDir =
    VariableMap::isVariablePresent(sunLightDirVarId) ? ShaderGlobal::get_color4(sunLightDirVarId) : Color4(0, 1, 0, 0);
  static int sunColorVarId = ::get_shader_variable_id("sun_color_0", true);
  Color4 sunColor = VariableMap::isVariablePresent(sunColorVarId) ? ShaderGlobal::get_color4(sunColorVarId) : Color4(1, 1, 1, 1);
  static int skyColorVarId = ::get_shader_variable_id("sky_color", true);
  Color4 skyColor = VariableMap::isVariablePresent(skyColorVarId) ? ShaderGlobal::get_color4(skyColorVarId) : Color4(1, 1, 1, 1);

  dafx::set_global_value(dafx_ctx, "proj_hk", &persp.hk, 4);
  dafx::set_global_value(dafx_ctx, "target_size", &targetSize, 8);
  dafx::set_global_value(dafx_ctx, "target_size_rcp", &targetSizeRcp, 8);
  dafx::set_global_value(dafx_ctx, "zn_zfar", &znZfar, 16);
  dafx::set_global_value(dafx_ctx, "globtm", &globtm, 64);
  dafx::set_global_value(dafx_ctx, "globtm_sim", &prevGlobTm, 64);
  dafx::set_global_value(dafx_ctx, "view_dir_x", &itm.getcol(0), 12);
  dafx::set_global_value(dafx_ctx, "view_dir_y", &itm.getcol(1), 12);
  dafx::set_global_value(dafx_ctx, "view_dir_z", &itm.getcol(2), 12);
  dafx::set_global_value(dafx_ctx, "world_view_pos", &itm.getcol(3), 12);
  dafx::set_global_value(dafx_ctx, "from_sun_direction", &sunLightDir, 12);
  dafx::set_global_value(dafx_ctx, "sun_color", &sunColor, 12);
  dafx::set_global_value(dafx_ctx, "sky_color", &skyColor, 12);

  static int prevTargetWidth = 0;
  static int prevTargetHeight = 0;

  if (prevTargetWidth != targetWidth || prevTargetHeight != targetHeight)
  {
    prevTargetWidth = targetWidth;
    prevTargetHeight = targetHeight;
    TEXTUREID downsampledDepthId = get_managed_texture_id("downsampled_far_depth_tex");
    BaseTexture *downsampledDepthTex = downsampledDepthId != BAD_TEXTUREID ? acquire_managed_tex(downsampledDepthId) : nullptr;
    if (downsampledDepthTex)
    {
      TextureInfo ti;
      downsampledDepthTex->getinfo(ti);

      Point2 depthSize(ti.w, ti.h);
      Point2 depthSizeRcp(safeinv((float)ti.w), safeinv((float)ti.h));
      IPoint2 offset(0, 0);

      dafx::set_global_value(dafx_ctx, "depth_size", &depthSize, 8);
      dafx::set_global_value(dafx_ctx, "depth_size_rcp", &depthSizeRcp, 8);
      dafx::set_global_value(dafx_ctx, "depth_tci_offset", &offset, 8);
      release_managed_tex(downsampledDepthId);
    }
  }

  act_dafx(dafx_ctx, 0.0f);
  memset(&dafx_stats, 0, sizeof(dafx::Stats));

  dafx::update_culling_state(dafx_ctx, dafx_cull, Frustum(globtm), Point3::xyz(itm.getcol(3)));
  dafx::before_render(dafx_ctx, {"highres", "lowres", "underwater", "distortion"});

  prevGlobTm = globtm;
}

inline void render_dafx_fom(dafx::ContextId dafx_ctx, dafx::CullingId dafx_fom_cull)
{
  TMatrix4 fomGlobtm;
  d3d::getglobtm(fomGlobtm);

  TMatrix4 fomViewTm;
  d3d::gettm(TM_VIEW, &fomViewTm);

  TMatrix fomViewItm;
  fomViewItm = orthonormalized_inverse(tmatrix(fomViewTm));

  Point3 viewPos = Point3::xyz(fomViewItm.getcol(3));

  // Prepare
  dafx::update_culling_state(dafx_ctx, dafx_fom_cull, Frustum(fomGlobtm), viewPos);
  dafx::before_render(dafx_ctx, {"fom"});

  // Render
  TMatrix4 prevGtm;
  Point3 prevPos;
  Point3 prevViewDirs[3];
  dafx::get_global_value(dafx_ctx, "globtm", &prevGtm, 64);
  dafx::get_global_value(dafx_ctx, "world_view_pos", &prevPos, 12);
  dafx::get_global_value(dafx_ctx, "view_dir_x", &prevViewDirs[0], 12);
  dafx::get_global_value(dafx_ctx, "view_dir_y", &prevViewDirs[1], 12);
  dafx::get_global_value(dafx_ctx, "view_dir_z", &prevViewDirs[2], 12);

  dafx::set_global_value(dafx_ctx, "globtm", &fomGlobtm, 64);
  dafx::set_global_value(dafx_ctx, "view_dir_x", fomViewItm[0], 12);
  dafx::set_global_value(dafx_ctx, "view_dir_y", fomViewItm[1], 12);
  dafx::set_global_value(dafx_ctx, "view_dir_z", fomViewItm[2], 12);
  dafx::set_global_value(dafx_ctx, "world_view_pos", &viewPos, 12);
  dafx::flush_global_values(dafx_ctx);

  dafx::render(dafx_ctx, dafx_fom_cull, "fom", 0.f);

  dafx::set_global_value(dafx_ctx, "globtm", &prevGtm, 64);
  dafx::set_global_value(dafx_ctx, "world_view_pos", &prevPos, 12);
  dafx::set_global_value(dafx_ctx, "view_dir_x", &prevViewDirs[0], 12);
  dafx::set_global_value(dafx_ctx, "view_dir_y", &prevViewDirs[1], 12);
  dafx::set_global_value(dafx_ctx, "view_dir_z", &prevViewDirs[2], 12);
  dafx::flush_global_values(dafx_ctx);
}

inline void calc_dafx_stats(dafx::ContextId dafx_ctx, dafx::Stats &dafx_stats)
{
  // there could be multiple renders per 1 update
  dafx::Stats stats;
  dafx::get_stats(dafx_ctx, stats);
  dafx_stats.activeInstances = max(dafx_stats.activeInstances, stats.activeInstances);
  dafx_stats.cpuElemProcessed = max(dafx_stats.cpuElemProcessed, stats.cpuElemProcessed);
  dafx_stats.gpuElemProcessed = max(dafx_stats.gpuElemProcessed, stats.gpuElemProcessed);
  dafx_stats.visibleTriangles = max(dafx_stats.visibleTriangles, stats.visibleTriangles);
  dafx_stats.renderedTriangles = max(dafx_stats.renderedTriangles, stats.renderedTriangles);
  dafx_stats.renderDrawCalls = max(dafx_stats.renderDrawCalls, stats.renderDrawCalls);
  dafx::clear_stats(dafx_ctx);
}