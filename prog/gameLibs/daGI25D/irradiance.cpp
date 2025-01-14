// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daGI25D/irradiance.h>
#include <math/dag_frustum.h>
#include <math/dag_math3d.h>
#include <math/dag_Point2.h>
#include <math/dag_cube_matrix.h>
#include <math/integer/dag_IBBox2.h>
#include <math/integer/dag_IBBox3.h>
#include <3d/dag_quadIndexBuffer.h>

#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_tex3d.h>
#include <perfMon/dag_statDrv.h>

#include <math/dag_hlsl_floatx.h>
#include "global_vars.h"
#include "shaders/dagi_volmap_consts_25d.hlsli"
#include <textureUtil/textureUtil.h>
#include <frustumCulling/frustumPlanes.h>


#include <util/dag_convar.h>
CONSOLE_BOOL_VAL("render", gi_25d_force_update_volmap, false);

namespace dagi25d
{

// must match distance in raycast_light in dagi_volmap_25d
int Irradiance::getSceneCoordTraceDist() const
{
  return TRACE_DIST_IN_COORD + 1;
} //+1 as we can "step out" by up to one voxel if probe is inside voxel

int Irradiance::getXZResolution() const { return GI_25D_RESOLUTION_X; }
int Irradiance::getYResolution() const { return GI_25D_RESOLUTION_Y; }
float Irradiance::getXZWidth() const { return GI_25D_RESOLUTION_X * voxelSizeXZ; }

float Irradiance::getMaxDist() const { return GI_25D_RESOLUTION_X * voxelSizeXZ * sqrtf(2) / 2; }

void Irradiance::init(bool scalar_ao, float xz_size, float y_size)
{
  init_global_vars();
  voxelSizeXZ = xz_size, voxelSizeY = y_size;
  // gi_compute_light_25d_cs.reset(new_compute_shader("gi_compute_light_25d_cs"));
  if (!gi_mark_intersected_25d_cs)
  {
    warpSize = d3d::get_driver_desc().minWarpSize;
    gi_mark_intersected_25d_cs.reset(
      new_compute_shader(warpSize == 64 ? "gi_mark_intersected_25d_cs_4" : "gi_mark_intersected_25d_cs_2"));
    gi_compute_light_25d_cs_4_4.reset(new_compute_shader("gi_compute_light_25d_cs_4_4"));
    gi_compute_light_25d_cs_4_8.reset(new_compute_shader("gi_compute_light_25d_cs_4_8"));
    gi_compute_light_25d_cs_8_4.reset(new_compute_shader("gi_compute_light_25d_cs_8_4"));
    debug("gi25d irradiance warp is size %d", warpSize);
  }
  // const uint32_t sceneFmt = TEXCF_SRGBREAD | TEXFMT_R8G8B8A8;
  const uint32_t sceneFmt = scalar_ao ? TEXFMT_R8 : TEXFMT_R11G11B10F;
  volmap = dag::create_voltex(GI_25D_RESOLUTION_X, GI_25D_RESOLUTION_Z, GI_25D_RESOLUTION_Y * 6, sceneFmt | TEXCF_UNORDERED, 1,
    "gi_25d_volmap");
  d3d::clear_rwtexf(volmap.getVolTex(), ResourceClearValue{}.asFloat, 0, 0);
  volmap->texaddr(TEXADDR_WRAP);
  volmap->texaddrw(TEXADDR_CLAMP);
  d3d::resource_barrier({volmap.getVolTex(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

  // resetting gi_25d_volmap_tc, to not sample based on tc from previous scene
  ShaderGlobal::set_color4(gi_25d_volmap_tcVarId, 0.0f, 0.0f, 0.0f, 1.0f);

  intersection = dag::buffers::create_ua_sr_byte_address(
    (GI_25D_RESOLUTION_X * GI_25D_RESOLUTION_Z * GI_25D_RESOLUTION_Y + 31) / 32 + // intersected
      GI_25D_RESOLUTION_X * GI_25D_RESOLUTION_Z                                   // floor
    ,
    "gi_25d_intersection");
  invalidate();
}

void Irradiance::invalidate()
{
  toroidalOrigin += IPoint2{10000, -10000};
  if (volmap.getVolTex())
    d3d::resource_barrier({volmap.getVolTex(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
}

void Irradiance::close()
{
  gi_mark_intersected_25d_cs.reset();
  gi_compute_light_25d_cs_8_4.reset();
  gi_compute_light_25d_cs_4_8.reset();
  gi_compute_light_25d_cs_4_4.reset();
  volmap.close();
  intersection.close();
}

void Irradiance::setVars()
{
  volmap.setVar();
  ShaderGlobal::set_color4(gi_25d_volmap_sizeVarId, voxelSizeXZ, voxelSizeY, 0, 0);
  const IPoint2 fullRes(GI_25D_RESOLUTION_X, GI_25D_RESOLUTION_Z);
  const IPoint2 ofs = toroidalOrigin; //(fullRes + toroidalOrigin%GI_25D_RESOLUTION_X)%GI_25D_RESOLUTION_X;
  Point2 origin = voxelSizeXZ * Point2(toroidalOrigin - IPoint2(GI_25D_RESOLUTION_X, GI_25D_RESOLUTION_X) / 2);
  ShaderGlobal::set_color4(gi_25d_volmap_originVarId, origin.x, origin.y, ofs.x, ofs.y);
}

Irradiance::UpdateResult Irradiance::updateOrigin(const Point3 &baseOrigin)
{
  if (!gi_compute_light_25d_cs_4_8)
    return NO_CHANGE;
  const float scaleXZ = 1.f / (voxelSizeXZ * GI_25D_RESOLUTION_X);
  const float scaleY = 1.f / (voxelSizeY * GI_25D_RESOLUTION_Y);
  ShaderGlobal::set_color4(gi_25d_volmap_tcVarId, scaleXZ, scaleY, -baseOrigin.x * scaleXZ, -baseOrigin.z * scaleXZ);

  Point3 origin = baseOrigin;
  IPoint2 newTexelsOrigin = ipoint2(floor(Point2::xz(origin) / voxelSizeXZ) + Point2(0.5, 0.5));
  static constexpr int ALIGN_TEXEL = 4;                                                                          // align to 4
  newTexelsOrigin = ((newTexelsOrigin + IPoint2(ALIGN_TEXEL / 2, ALIGN_TEXEL / 2)) / ALIGN_TEXEL) * ALIGN_TEXEL; // align to
                                                                                                                 // ALIGN_TEXEL
  if (gi_25d_force_update_volmap)
    toroidalOrigin.x += 1000;
  IPoint2 imove = (toroidalOrigin - newTexelsOrigin);
  IPoint2 amove = abs(imove);
  static constexpr int THRESHOLD = 4;
  if (amove.x < THRESHOLD && amove.y < THRESHOLD)
    return NO_CHANGE;
  const IPoint2 realRes(GI_25D_RESOLUTION_X, GI_25D_RESOLUTION_X);
  IPoint2 res = realRes;
  IPoint2 stInvalid;
  if (max(amove.x, amove.y) >= GI_25D_RESOLUTION_X) // clear whole scene
  {
    toroidalOrigin = newTexelsOrigin;
    setVars();
    const IPoint2 lt = toroidalOrigin - realRes / 2;
    ShaderGlobal::set_color4(gi_25d_volmap_invalid_start_widthVarId, lt.x, lt.y, realRes.x, realRes.y);
    stInvalid = lt;
  }
  else
  {
    if (amove.x > amove.y)
    {
      imove.y = amove.y = 0;
      res.x = amove.x;
      newTexelsOrigin.y = toroidalOrigin.y;
    }
    else
    {
      imove.x = amove.x = 0;
      res.y = amove.y;
      newTexelsOrigin.x = toroidalOrigin.x;
    }
    toroidalOrigin = newTexelsOrigin;
    setVars();

    const IPoint2 lt = toroidalOrigin - realRes / 2;
    const IPoint2 rb = lt + realRes + imove;
    stInvalid = IPoint2(imove.x >= 0 ? lt.x : rb.x, imove.y >= 0 ? lt.y : rb.y);

    // debug("move 2.5d %@ stInvalid = %@, res = %@", imove, stInvalid, res);
  }

  // debug("move 2.5d %@ stInvalid = %@, res = %@", imove, stInvalid, res);
  // if (((stInvalid.x|stInvalid.y) & 3) != 0)
  //   logerr("movement should be aligned");
  ShaderGlobal::set_color4(gi_25d_volmap_invalid_start_widthVarId, stInvalid.x, stInvalid.y, res.x, res.y);
  {
    TIME_D3D_PROFILE(intersection_25d);

    d3d::set_rwbuffer(STAGE_CS, 0, intersection.getBuf());
    const int zThreads = (warpSize == 64) ? 4 : 2;
    gi_mark_intersected_25d_cs->dispatch((res.x + 3) / 4, (res.y + 3) / 4, (GI_25D_RESOLUTION_Y + zThreads - 1) / zThreads);
    d3d::set_rwbuffer(STAGE_CS, 0, 0);
    d3d::resource_barrier({intersection.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});
  }
  {
    TIME_D3D_PROFILE(volmap_25d);
    d3d::set_rwtex(STAGE_CS, 0, volmap.getVolTex(), 0, 0, false);

    IPoint2 optimal(4, 4);
    if (warpSize == 64)
    {
      // calculate optimal permutation
      uint totalThreadXZSize = warpSize / 2;
      int currentRedundance = 1024;
      for (int i = 2; i <= 3; ++i)
      {
        const int resGroup = 1 << i, compResGroup = (totalThreadXZSize + resGroup - 1) / resGroup;
        auto checkRedundancy = [&currentRedundance, &optimal, res](IPoint2 resGroup) {
          IPoint2 red = (resGroup - IPoint2(res.x % resGroup.x, res.y % resGroup.y));
          red.x %= resGroup.x;
          red.y %= resGroup.y;
          const int newRed = red.x + red.y;
          if (newRed < currentRedundance || (newRed == currentRedundance && optimal.x + optimal.y > resGroup.x + resGroup.y))
          {
            optimal = resGroup;
            currentRedundance = newRed;
          }
        };
        checkRedundancy(IPoint2(resGroup, compResGroup));
        checkRedundancy(IPoint2(compResGroup, resGroup));
      }
    }
    // debug("res= %@, optimal = %@ red = %d", res, optimal,currentRedundance);//8 permutations!
    // gi_compute_light_25d_cs_intersected->dispatch((res.x+optimal.x-1)/optimal.x,(res.y+optimal.y-1)/optimal.y,
    // (GI_25D_RESOLUTION_Y+1)/2);
    if (optimal.x == 4 && optimal.y == 4)
    {
      gi_compute_light_25d_cs_4_4->dispatch((res.x + 3) / 4, (res.y + 3) / 4, (GI_25D_RESOLUTION_Y + 1) / 2);
    }
    else if (optimal.x == 4 && optimal.y == 8)
      gi_compute_light_25d_cs_4_8->dispatch((res.x + optimal.x - 1) / optimal.x, (res.y + optimal.y - 1) / optimal.y,
        (GI_25D_RESOLUTION_Y + 1) / 2);
    else if (optimal.x == 8 && optimal.y == 4)
      gi_compute_light_25d_cs_8_4->dispatch((res.x + optimal.x - 1) / optimal.x, (res.y + optimal.y - 1) / optimal.y,
        (GI_25D_RESOLUTION_Y + 1) / 2);
    else
      G_ASSERTF(0, "Internal error");

    d3d::set_rwtex(STAGE_CS, 0, 0, 0, 0, false);

    d3d::resource_barrier({volmap.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
  }
  const uint32_t updatedTexels = res.x * res.y;
  if (updatedTexels >= GI_25D_RESOLUTION_X * GI_25D_RESOLUTION_X)
    return UPDATE_TELEPORTED;
  if (updatedTexels >= (GI_25D_RESOLUTION_X * GI_25D_RESOLUTION_X) / 4)
    return UPDATE_BIG;
  return UPDATE_MOVED;
}

void Irradiance::drawDebug(IrradianceDebugType type)
{
  if (!debug_rasterize_probes.shader)
    debug_rasterize_probes.init("debug_render_volmap_25d", NULL, 0, "debug_render_volmap_25d");
  if (!debug_rasterize_probes.shader)
    return;
  TIME_D3D_PROFILE(debugVolmap25d);
  ShaderGlobal::set_int(debug_volmap_typeVarId, (int)type);

  d3d::settm(TM_WORLD, TMatrix::IDENT);
  mat44f globtm;
  d3d::getglobtm(globtm);
  set_frustum_planes(Frustum(globtm));

  debug_rasterize_probes.shader->setStates(0, true);

  d3d::setvsrc(0, 0, 0);
  index_buffer::use_box();

  d3d::drawind_instanced(PRIM_TRILIST, 0, 12, 0, GI_25D_RESOLUTION_X * GI_25D_RESOLUTION_Z * GI_25D_RESOLUTION_Y);

  d3d::setind(0);
}

} // namespace dagi25d