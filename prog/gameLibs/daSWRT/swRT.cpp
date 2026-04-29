// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daSWRT/swBVH.h>
#include <generic/dag_relocatableFixedVector.h>
#include <vecmath/dag_vecMath.h>
#include <util/dag_stlqsort.h>
#include <math/dag_TMatrix.h>
#include <EASTL/unique_ptr.h>
#include <shaders/dag_shaders.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_tex3d.h>
#include <3d/dag_resPtr.h>
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

#include "swCommon.h"
#include "shaders/swBVHDefine.hlsli"

#define GLOBAL_VARS_LIST       \
  VAR(swrt_shadow_target)      \
  VAR(swrt_shadow_target_size) \
  VAR(swrt_checkerboard_frame) \
  VAR(swrt_dir_to_sun_basis)   \
  VAR(swrt_shadow_planes)      \
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
  mat.reset();
  clearAll();

  del_it(checkerboard_shadows_swrt_cs);
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
  CS(shadows_swrt_cs);
  CS(mask_shadow_swrt_cs);
#undef CS

  rt.init("draw_collision_swrt");
}

void RenderSWRT::drawRT()
{
  if (!topBuf)
    return;

  if (!mat)
    mat.reset(new_shader_material_by_name_optional("draw_collision_swrt"));
  if (mat)
  {
    mat->addRef();
    elem = mat->make_elem();
  }
  else
    return;
  TIME_D3D_PROFILE(bvh_rt);
  rt.render();
}

uint32_t RenderSWRT::getTileBufferSizeDwords(uint32_t w, uint32_t h)
{
  // 2 dwords per tile when LIMIT_AT_END is enabled in shaders (first + last leaf), 1 dword otherwise
  return 2 * (uint32_t(w + SWRT_TILE_SIZE - 1) / SWRT_TILE_SIZE * (uint32_t(h + SWRT_TILE_SIZE - 1) / SWRT_TILE_SIZE));
}

void RenderSWRT::renderShadowFrustumTiles(int w, int h, const Point3 &to_sun_direction, float sun_angle_size)
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

  mask_shadow_swrt_cs->dispatchThreads((w + SWRT_TILE_SIZE - 1) / SWRT_TILE_SIZE, (h + SWRT_TILE_SIZE - 1) / SWRT_TILE_SIZE, 1);
}

void RenderSWRT::renderShadows(const Point3 &to_sun_direction, float sun_size, uint32_t checkerboad_frame, Sbuffer *shadow_mask_buf,
  BaseTexture *shadow_target, bool set_target_var)
{
  if (!shadows_swrt_cs || !shadow_target)
    return;

  TMatrix orthoSun;
  view_matrix_from_look(to_sun_direction, orthoSun);
  float4 basis[3] = {float4(orthoSun.getcol(0), tanf(sun_size)), float4(orthoSun.getcol(1), 0), float4(orthoSun.getcol(2), sun_size)};
  ShaderGlobal::set_float4_array(swrt_dir_to_sun_basisVarId, basis, countof(basis));

  TextureInfo ti;
  shadow_target->getinfo(ti, 0);
  const int w = ti.w, h = ti.h;
  const int srcW = checkerboad_frame != NO_CHECKER_BOARD ? w * 2 : w; // fixme!! source is not w*2
  ShaderGlobal::set_int4(swrt_shadow_target_sizeVarId, w, h, srcW - 1, h - 1);
  ShaderGlobal::set_buffer(swrt_shadow_maskVarId, shadow_mask_buf);
  ShaderGlobal::set_texture(swrt_shadow_targetVarId, shadow_target);
  ShaderGlobal::set_int(swrt_checkerboard_frameVarId, checkerboad_frame);

  DA_PROFILE_GPU;
  renderShadowFrustumTiles(srcW, h, to_sun_direction, sun_size);
  d3d::resource_barrier({shadow_mask_buf, RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});
  if (checkerboad_frame != NO_CHECKER_BOARD)
    checkerboard_shadows_swrt_cs->dispatchThreads(w, h, 1);
  else
    shadows_swrt_cs->dispatchThreads(w, h, 1);

  d3d::resource_barrier({shadow_target, RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});

  ShaderGlobal::set_buffer(swrt_shadow_maskVarId, nullptr);
  if (!set_target_var)
    ShaderGlobal::set_texture(swrt_shadow_targetVarId, nullptr);
}
