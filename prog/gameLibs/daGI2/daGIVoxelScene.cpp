// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_convar.h>
#include <perfMon/dag_statDrv.h>
#include "daGIVoxelScene.h"

CONSOLE_INT_VAL("gi", gi_voxels_lit_scene_debug, 0, 0, 2);
CONSOLE_INT_VAL("gi", gi_voxel_scene_update_from_gbuf_speed_min, 128, 64, 32 << 10);
CONSOLE_INT_VAL("gi", gi_voxel_scene_update_from_gbuf_speed_count, 8192, 0, 256 << 10);

#define GLOBAL_VARS_LIST                  \
  VAR(dagi_voxel_lit_scene_stable_update) \
  VAR(dagi_lit_voxel_scene_luma_only)     \
  VAR(dagi_voxel_lit_scene_update_count)  \
  VAR(dagi_lit_voxel_scene_res)           \
  VAR(dagi_lit_voxel_scene_res_np2)       \
  VAR(dagi_lit_voxel_scene_to_sdf_clips)  \
  VAR(dagi_lit_scene_to_atlas_decode)     \
  VAR(dagi_lit_voxel_scene_to_sdf)        \
  VAR(dagi_lit_voxel_scene_alpha_samplerstate)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_LIST
#undef VAR

DaGIVoxelScene::~DaGIVoxelScene() { ShaderGlobal::set_int4(dagi_lit_voxel_scene_resVarId, 0, 0, 0, 0); }


void DaGIVoxelScene::init(uint32_t sdfW, uint32_t sdfH, float scale, uint32_t first_sdf_clip, uint32_t voxel_scene_clips, Aniso ani,
  Radiance rad)
{
#define VAR(a)     \
  if (!(a##VarId)) \
    logerr("mandatory shader variable is missing: %s", #a);
  GLOBAL_VARS_LIST
#undef VAR

  const bool aniso = ani == Aniso::On;
  const bool luma = rad == Radiance::Luma;
  voxel_scene_clips = max<int>(voxel_scene_clips, 1);
  const int w = (uint32_t(sdfW * scale) + 3) & ~3, d = (uint32_t(sdfH * scale) + 3) & ~3;
  if (w == resW && d == resD && clips == voxel_scene_clips && anisotropy == aniso && lumaOnly == luma)
    return;
  debug("daGI: init voxel scene res %dx%dx%d, clips %d, aniso = %d, luma %d", w, w, d, voxel_scene_clips, aniso, luma);
  resW = w;
  resD = d;
  clips = voxel_scene_clips;
  anisotropy = aniso;
  lumaOnly = luma;
  firstSdfClip = first_sdf_clip;
  const uint32_t anisotropyCnt = (anisotropy ? 6 : 1);
  fullAtlasResD = anisotropyCnt * (resD + 2) * clips;
  const uint32_t oneClipSizeWithBorderInTexels = anisotropyCnt * (resD + 2);
  const uint32_t oneAxisSliceSizeWithBorderInTexels = (anisotropy ? resD + 2 : 0);
  dagi_lit_voxel_scene.close();
  dagi_lit_voxel_scene_alpha.close();
  const uint32_t fmt = lumaOnly ? TEXFMT_R16F : TEXFMT_R11G11B10F;
  dagi_lit_voxel_scene = dag::create_voltex(resW, resW, fullAtlasResD, TEXCF_UNORDERED | fmt, 1, "dagi_lit_voxel_scene");
  dagi_lit_voxel_scene_alpha =
    dag::create_voltex(resW, resW, fullAtlasResD, TEXCF_UNORDERED | TEXFMT_R8, 1, "dagi_lit_voxel_scene_alpha");
  dagi_lit_voxel_scene_alpha_samplerstateVarId.set_sampler(d3d::request_sampler({}));
  ShaderGlobal::set_int4(dagi_lit_voxel_scene_resVarId, resW, resD, clips, anisotropy ? 1 : 0);

  constexpr int max_pos = (1 << 30) - 1;
  ShaderGlobal::set_int4(dagi_lit_voxel_scene_res_np2VarId, resW, resD, is_pow_of2(resW) ? 0 : (max_pos / resW) * resW,
    is_pow_of2(resD) ? 0 : (max_pos / resD) * resD);
  ShaderGlobal::set_color4(dagi_lit_voxel_scene_to_sdfVarId, float(resW) / sdfW, float(resD) / sdfH, 0, 0);

  ShaderGlobal::set_int4(dagi_lit_voxel_scene_to_sdf_clipsVarId, firstSdfClip, fullAtlasResD, oneClipSizeWithBorderInTexels,
    oneAxisSliceSizeWithBorderInTexels);
  ShaderGlobal::set_int(dagi_lit_voxel_scene_luma_onlyVarId, lumaOnly);
  ShaderGlobal::set_color4(dagi_lit_scene_to_atlas_decodeVarId,
    Color4(resD, oneClipSizeWithBorderInTexels, oneAxisSliceSizeWithBorderInTexels, 1) / fullAtlasResD);
  cleared = false;
  if (!dagi_voxel_scene_reset_cs)
  {
    dagi_voxel_scene_reset_cs.reset(new_compute_shader("dagi_voxel_scene_reset_cs"));
    dagi_voxel_lit_scene_from_gbuf_cs.reset(new_compute_shader("dagi_voxel_lit_scene_from_gbuf_cs"));
  }
}

void DaGIVoxelScene::update()
{
  if (cleared)
    return;
  if (dagi_voxel_scene_reset_cs)
  {
    DA_PROFILE_GPU;
    dagi_voxel_scene_reset_cs->dispatchThreads(resW, resW, fullAtlasResD);
    d3d::resource_barrier({dagi_lit_voxel_scene_alpha.getVolTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
    d3d::resource_barrier({dagi_lit_voxel_scene.getVolTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
  }
  cleared = true;
}

void DaGIVoxelScene::rbNone()
{
  d3d::resource_barrier({dagi_lit_voxel_scene_alpha.getVolTex(), RB_NONE, 0, 0});
  d3d::resource_barrier({dagi_lit_voxel_scene.getVolTex(), RB_NONE, 0, 0});
}

void DaGIVoxelScene::rbFinish()
{
  d3d::resource_barrier({dagi_lit_voxel_scene_alpha.getVolTex(),
    RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 0});
  d3d::resource_barrier({dagi_lit_voxel_scene.getVolTex(),
    RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 0});
}


void DaGIVoxelScene::updateFromGbuf()
{
  if (!dagi_voxel_lit_scene_from_gbuf_cs)
    return;
  DA_PROFILE_GPU;
  float useTemporalSpeed = (temporalStable ? 0.5f : 1.f) * temporalSpeed; // decrease amount
  const uint32_t pixelCount =
    gi_voxel_scene_update_from_gbuf_speed_min + gi_voxel_scene_update_from_gbuf_speed_count * useTemporalSpeed;
  ShaderGlobal::set_int(dagi_voxel_lit_scene_stable_updateVarId, temporalStable ? 1 : 0);
  ShaderGlobal::set_int4(dagi_voxel_lit_scene_update_countVarId, pixelCount, gbuf_update_frame++, 0, 0);
  dagi_voxel_lit_scene_from_gbuf_cs->dispatchThreads(pixelCount, 1, 1);

  // fixme: this barrier is not needed here. Instead, we need to call it _before update (from gbuf or on move)
  d3d::resource_barrier({dagi_lit_voxel_scene_alpha.getVolTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
  d3d::resource_barrier({dagi_lit_voxel_scene.getVolTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
}

void DaGIVoxelScene::debugRender()
{
  if (!gi_voxels_lit_scene_debug)
    return;
  if (!dagi_lit_scene_voxels_debug.getElem())
    dagi_lit_scene_voxels_debug.init("dagi_lit_scene_voxels_debug");
  if (!dagi_world_scene_voxel_scene_debug.getElem())
    dagi_world_scene_voxel_scene_debug.init("dagi_world_scene_voxel_scene_debug");

  DA_PROFILE_GPU;
  if (gi_voxels_lit_scene_debug == 2)
    dagi_lit_scene_voxels_debug.render();
  else if (gi_voxels_lit_scene_debug == 1)
    dagi_world_scene_voxel_scene_debug.render();
}