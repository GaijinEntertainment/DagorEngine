// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daSWRT/swBVH.h>
#include <generic/dag_relocatableFixedVector.h>
#include <vecmath/dag_vecMath.h>
#include <util/dag_stlqsort.h>
#include <math/dag_TMatrix.h>
#include <shaders/dag_shaders.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_tex3d.h>
#include <3d/dag_resPtr.h>
#include <drv/3d/dag_info.h>
#include <shaders/dag_postFxRenderer.h>
#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_statDrv.h>
#include <generic/dag_carray.h>
#include <render/noiseTex.h>
#include <util/dag_parallelFor.h>
#include <osApiWrappers/dag_spinlock.h>
#include <shaders/dag_computeShaders.h>
#include <util/dag_convar.h>
#include <math/dag_viewMatrix.h>
#include <math/dag_hlsl_floatx.h>

#include <daBVH/swCommon.h>
#include <daBVH/swBVHDefine.hlsli>

#define GLOBAL_VARS_LIST        \
  VAR(swrt_shadow_target)       \
  VAR(swrt_shadow_target_size)  \
  VAR(swrt_shadow_tc_size)      \
  VAR(swrt_checkerboard_frame)  \
  VAR(swrt_frame_no)            \
  VAR(swrt_caster_reach)        \
  VAR(swrt_caster_static_depth) \
  VAR(swrt_shadow_quarter)      \
  VAR(swrt_shadow_ray)          \
  VAR(swrt_box_dist_ref)        \
  VAR(swrt_blas_leaf_skip_dist) \
  VAR(swrt_tile_16)             \
  VAR(swrt_dir_to_sun_basis)    \
  VAR(swrt_shadow_planes)       \
  VAR(swrt_shadow_mask)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_LIST
#undef VAR

using namespace build_bvh;

void RenderSWRT::clearAll()
{
  clearTLASSourceData();
  clearBLASBuiltData();
  clearBLASSourceData();
  clearTLASBuffers();
  clearBLASBuffers();
}

void RenderSWRT::close()
{
  if (inited)
    release_blue_noise();
  clearAll();

  del_it(checkerboard_shadows_swrt_cs);
  del_it(quarter_shadows_swrt_cs);
  del_it(shadows_swrt_cs);
  del_it(mask_shadow_swrt_cs);

  inited = false;
}

RenderSWRT::~RenderSWRT() { close(); }

void RenderSWRT::init()
{
  close();
  init_and_get_blue_noise();
  inited = true;
#define CS(a) a = new_compute_shader(#a)
  CS(checkerboard_shadows_swrt_cs);
  CS(quarter_shadows_swrt_cs);
  CS(shadows_swrt_cs);
  CS(mask_shadow_swrt_cs);
#undef CS

  bool optionalDebug = d3d::get_driver_code() == d3d::dx11; // dx11 shader compiler is failing on debug shader: draw_collision_swrt
  rt.init("draw_collision_swrt", optionalDebug);
}

void RenderSWRT::drawRT()
{
  if (!topBuf)
    return;

  TIME_D3D_PROFILE(bvh_rt);
  rt.render(); // optional
}

void RenderSWRT::setShadowBoxDistRef(float metres) { ShaderGlobal::set_float(swrt_box_dist_refVarId, metres); }

void RenderSWRT::setShadowBlasLeafBoxes(float skip_dist) { ShaderGlobal::set_float(swrt_blas_leaf_skip_distVarId, skip_dist); }

static bool shadows_take_leaf_boxes()
{
  if (ShaderGlobal::is_var_assumed(swrt_blas_leaf_skip_distVarId))
    return ShaderGlobal::get_interval_assumed_value(swrt_blas_leaf_skip_distVarId) >= 0; // records on as 0, off -1
  return ShaderGlobal::get_float(swrt_blas_leaf_skip_distVarId) >= 0;
}

uint32_t RenderSWRT::getTileBufferSizeDwords(uint32_t w, uint32_t h)
{
  // 2 dwords per 8x8 tile at every rate: the first and the last leaf
  return 2 * (uint32_t(w + SWRT_TILE_SIZE - 1) / SWRT_TILE_SIZE * (uint32_t(h + SWRT_TILE_SIZE - 1) / SWRT_TILE_SIZE));
}

IPoint2 RenderSWRT::getShadowTargetSize(int src_w, int src_h, bool checkerboard, bool quarter)
{
  // ceil, so the last target texel still maps to the last source pixel
  return IPoint2(checkerboard ? (src_w + 1) / 2 : src_w, checkerboard && quarter ? (src_h + 1) / 2 : src_h);
}

uint32_t RenderSWRT::getShadowTargetFormat(bool checkerboard, bool quarter)
{
  return checkerboard && quarter ? TEXFMT_G16R16F : TEXFMT_R8;
}

void RenderSWRT::renderShadowFrustumTiles(int w, int h, const Point3 &to_sun_direction, float sun_angle_size, int tile_size)
{
  if (!mask_shadow_swrt_cs)
    return;
  DA_PROFILE_GPU;
  // validate for vertical sun
  float2 sinCosA = float2(sinf(sun_angle_size), cosf(sun_angle_size));
  const float lenXZ = sqrtf(to_sun_direction.x * to_sun_direction.x + to_sun_direction.z * to_sun_direction.z);
  float3 lN, rN, tN, bN, nN;
  if (lenXZ > 1e-2f)
  {
    float2 to_sun_light_dir_xz(to_sun_direction.x / lenXZ, to_sun_direction.z / lenXZ);

    lN = -float3(to_sun_light_dir_xz.y * sinCosA.y - to_sun_light_dir_xz.x * sinCosA.x, 0,
      -to_sun_light_dir_xz.y * sinCosA.x - to_sun_light_dir_xz.x * sinCosA.y);
    rN = -float3(-to_sun_light_dir_xz.y * sinCosA.y - to_sun_light_dir_xz.x * sinCosA.x, 0,
      -to_sun_light_dir_xz.y * sinCosA.x + to_sun_light_dir_xz.x * sinCosA.y);
    nN = float3(to_sun_light_dir_xz.x, 0, to_sun_light_dir_xz.y);
    float3 directLeftN = float3(to_sun_light_dir_xz.y, 0, -to_sun_light_dir_xz.x); // no rotation with sun size

    // validate for horizontal sun
    float3 ortoSun = fabsf(to_sun_direction.y) < 1e-6f ? float3(0, -1, 0) : normalize(cross(directLeftN, to_sun_direction));
    bN = (makeTM(directLeftN, sun_angle_size) % -ortoSun);
    tN = (makeTM(directLeftN, -sun_angle_size) % ortoSun);
  }
  else
  {
    // code is checking only 4 first planes though
    lN = float3(sinCosA.y, sinCosA.x, 0);
    rN = float3(-sinCosA.y, sinCosA.x, 0);
    bN = float3(0, sinCosA.x, sinCosA.y);
    nN = float3(0, sinCosA.x, -sinCosA.y);
    tN = to_sun_direction;
  }
  float4 planes[4] = {
    float4(lN.x, rN.x, bN.x, nN.x), float4(lN.y, rN.y, bN.y, nN.y), float4(lN.z, rN.z, bN.z, nN.z), float4(tN.x, tN.y, tN.z, 0)};
  ShaderGlobal::set_float4_array(swrt_shadow_planesVarId, planes, countof(planes));

  ShaderGlobal::set_int(swrt_tile_16VarId, tile_size == 16 ? 1 : 0);
  mask_shadow_swrt_cs->dispatchThreads((w + tile_size - 1) / tile_size, (h + tile_size - 1) / tile_size, 1);
}

void RenderSWRT::renderShadows(const Point3 &to_sun_direction, float sun_size, uint32_t frame_no, uint32_t checkerboad_frame,
  int src_w, int src_h, Sbuffer *shadow_mask_buf, BaseTexture *shadow_target, bool set_target_var,
  const daSWRT::ShadowCasterReach &caster_reach, const IPoint2 &full_res)
{
  // All four compute shaders are required and created together:
  // renderShadowFrustumTiles writes swrt_shadow_mask via mask_shadow_swrt_cs, which the per-pixel pass below reads.
  if (!shadows_swrt_cs || !checkerboard_shadows_swrt_cs || !quarter_shadows_swrt_cs || !mask_shadow_swrt_cs || !shadow_target)
    return;
  // only the leaf box variant reads it
  if (gpuBlasMode == daSWRT::BlasMode::BoxesOnly && !shadows_take_leaf_boxes())
  {
    LOGERR_ONCE("SWRT: a boxes-only BLAS needs setShadowBlasLeafBoxes >= 0; no SWRT shadows");
    ShaderGlobal::set_int4(swrt_shadow_target_sizeVarId, 0, 0, 0, 0);
    ShaderGlobal::set_texture(swrt_shadow_targetVarId, BAD_TEXTUREID);
    return;
  }

  TMatrix orthoSun;
  view_matrix_from_look(to_sun_direction, orthoSun);
  const bool quarter = shadowQuarter && checkerboad_frame != NO_CHECKER_BOARD;
  // the quarter rate traces hard, parallel rays: no sun disk widening
  const float cone = quarter ? 0.f : sun_size;
  float4 basis[3] = {float4(orthoSun.getcol(0), tanf(cone)), float4(orthoSun.getcol(1), 0), float4(orthoSun.getcol(2), cone)};
  ShaderGlobal::set_float4_array(swrt_dir_to_sun_basisVarId, basis, countof(basis));
  ShaderGlobal::set_float4(swrt_caster_reachVarId, caster_reach.csmDistance, caster_reach.csmReach, caster_reach.staticDistance,
    caster_reach.staticTopHeight);
  ShaderGlobal::set_float(swrt_caster_static_depthVarId, caster_reach.staticDepth);

  const Point3 rayDir = normalize(to_sun_direction);
  auto safeInv = [](float v) { return fabsf(v) < 1e-32f ? (v < 0 ? -1e32f : 1e32f) : 1.f / v; };
  float4 shadowRay[2] = {float4(rayDir, 0), float4(safeInv(rayDir.x), safeInv(rayDir.y), safeInv(rayDir.z), 0)};
  ShaderGlobal::set_float4_array(swrt_shadow_rayVarId, shadowRay, countof(shadowRay));

  TextureInfo ti;
  shadow_target->getinfo(ti, 0);
  const int w = ti.w, h = ti.h;
  // src_w, src_h: the half-res depth size; the checkerboard target halves the width, the quarter one both
  ShaderGlobal::set_int4(swrt_shadow_target_sizeVarId, w, h, src_w - 1, src_h - 1);
  // a half-res texel p holds the full-res texels 2p, 2p+1: its uv is (p + 0.5) / (full / 2), not / src
  const float tcW = full_res.x > 0 ? full_res.x * 0.5f : src_w, tcH = full_res.y > 0 ? full_res.y * 0.5f : src_h;
  ShaderGlobal::set_float4(swrt_shadow_tc_sizeVarId, tcW, tcH, 0, 0);
  ShaderGlobal::set_buffer(swrt_shadow_maskVarId, shadow_mask_buf);
  ShaderGlobal::set_texture(swrt_shadow_targetVarId, shadow_target);
  ShaderGlobal::set_int(swrt_checkerboard_frameVarId, checkerboad_frame);
  ShaderGlobal::set_int(swrt_shadow_quarterVarId, quarter ? 1 : 0);
  ShaderGlobal::set_int(swrt_frame_noVarId, frame_no);

  DA_PROFILE_GPU;
  {
    TIME_D3D_PROFILE(swrt_shadows_tiles);
    renderShadowFrustumTiles(src_w, src_h, to_sun_direction, cone, quarter ? 16 : SWRT_TILE_SIZE);
  }
  d3d::resource_barrier({shadow_mask_buf, RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});
  {
    TIME_D3D_PROFILE(swrt_shadows_pixels);
    if (quarter)
      quarter_shadows_swrt_cs->dispatchThreads(w, h, 1);
    else if (checkerboad_frame != NO_CHECKER_BOARD)
      checkerboard_shadows_swrt_cs->dispatchThreads(w, h, 1);
    else
      shadows_swrt_cs->dispatchThreads(w, h, 1);
  }

  d3d::resource_barrier({shadow_target, RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});

  ShaderGlobal::set_buffer(swrt_shadow_maskVarId, nullptr);
  if (!set_target_var)
    ShaderGlobal::set_texture(swrt_shadow_targetVarId, nullptr);
}
