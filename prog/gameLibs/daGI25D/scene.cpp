// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daGI25D/scene.h>
#include <math/dag_math3d.h>
#include <math/dag_Point2.h>
#include <math/dag_cube_matrix.h>
#include <math/integer/dag_IBBox2.h>
#include <math/integer/dag_IBBox3.h>

#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_tex3d.h>
#include <shaders/dag_computeShaders.h>
#include <perfMon/dag_statDrv.h>

#include <math/dag_hlsl_floatx.h>
#include "global_vars.h"
#include "shaders/dagi_voxels_consts_25d.hlsli"

#include <util/dag_convar.h>
CONSOLE_BOOL_VAL("render", gi_25d_force_update_scene, false);
CONSOLE_BOOL_VAL("render", gi_25d_force_update_scene_partial, false);

namespace dagi25d
{

int Scene::getXZResolution() const { return VOXEL_25D_RESOLUTION_X; }
int Scene::getYResolution() const { return VOXEL_25D_RESOLUTION_Y; }

float Scene::getMaxDist() const { return VOXEL_25D_RESOLUTION_X * voxelSizeXZ * sqrtf(2) / 2; }

void Scene::init(bool scalar_ao, float xz_size, float y_size)
{
  init_global_vars();
  voxelSizeXZ = xz_size, voxelSizeY = y_size;
  if (!ssgi_clear_scene_25d_cs)
    ssgi_clear_scene_25d_cs.reset(new_compute_shader("ssgi_clear_scene_25d_cs"));
  if (!ssgi_clear_scene_25d_full_cs)
    ssgi_clear_scene_25d_full_cs.reset(new_compute_shader("ssgi_clear_scene_25d_full_cs"));

  int required_size = VOXEL_25D_RESOLUTION_X * VOXEL_25D_RESOLUTION_Z * VOXEL_25D_RESOLUTION_Y; // alpha
  if (!scalar_ao)
    required_size += VOXEL_25D_RESOLUTION_X * VOXEL_25D_RESOLUTION_Z * VOXEL_25D_RESOLUTION_Y * 4; // color
  required_size += VOXEL_25D_RESOLUTION_X * VOXEL_25D_RESOLUTION_Z * 4;                            // floor HT

  sceneAlpha = dag::buffers::create_ua_sr_byte_address((required_size + 3) / sizeof(uint), "scene_25d_buf");
  invalidate();
  setVars();
}

void Scene::close()
{
  sceneAlpha.close();
  ssgi_clear_scene_25d_cs.reset();
  debug_rasterize_voxels.close();
}

void Scene::debugRayCast(SceneDebugType debug_scene)
{
  if (!debug_rasterize_voxels.shader)
    debug_rasterize_voxels.init("ssgi_debug_rasterize_voxels_25d", NULL, 0, "ssgi_debug_rasterize_voxels_25d");
  ShaderGlobal::set_int(ssgi_debug_rasterize_sceneVarId, static_cast<int>(debug_scene));
  TIME_D3D_PROFILE(ssgi_debug_rasterize_voxels);
  debug_rasterize_voxels.shader->setStates(0, true);
  d3d::setvsrc(0, 0, 0);
  d3d::draw(PRIM_TRILIST, 0, 1);
}

void Scene::setVars()
{
  sceneAlpha.setVar();
  ShaderGlobal::set_color4(scene_25d_voxels_sizeVarId, voxelSizeXZ, voxelSizeY, 0, voxelSizeXZ * 0.9 * 0.5);
  const IPoint2 fullRes(VOXEL_25D_RESOLUTION_X, VOXEL_25D_RESOLUTION_Z);
  const IPoint2 ofs = toroidalOrigin; //(fullRes + toroidalOrigin%VOXEL_25D_RESOLUTION_X)%VOXEL_25D_RESOLUTION_X;
  Point2 origin = voxelSizeXZ * Point2(toroidalOrigin - IPoint2(VOXEL_25D_RESOLUTION_X, VOXEL_25D_RESOLUTION_X) / 2);
  ShaderGlobal::set_color4(scene_25d_voxels_originVarId, origin.x, origin.y, ofs.x, ofs.y);
}

static constexpr int MOVE_THRESHOLD = 4;
static constexpr int ALIGN_TEXEL = 4; // align to 4

int Scene::moveDistThreshold() const { return MOVE_THRESHOLD + ALIGN_TEXEL / 2; }

UpdateResult Scene::updateOrigin(const Point3 &baseOrigin, const voxelize_scene_fun_cb &voxelize_cb, bool &update_pending)
{
  update_pending = false;
  if (!ssgi_clear_scene_25d_cs)
    return UpdateResult::NO_CHANGE;
  Point3 origin = baseOrigin;
  IPoint2 newTexelsOrigin = ipoint2(floor(Point2::xz(origin) / voxelSizeXZ) + Point2(0.5, 0.5));
  newTexelsOrigin = ((newTexelsOrigin + IPoint2(ALIGN_TEXEL / 2, ALIGN_TEXEL / 2)) / ALIGN_TEXEL) * ALIGN_TEXEL; // align to
                                                                                                                 // ALIGN_TEXEL
  IPoint2 targetOrigin = newTexelsOrigin;
  if (gi_25d_force_update_scene)
    toroidalOrigin = newTexelsOrigin - IPoint2(1000, 10000);
  else if (gi_25d_force_update_scene_partial && abs(toroidalOrigin.x - newTexelsOrigin.x) < MOVE_THRESHOLD)
    newTexelsOrigin.x += MOVE_THRESHOLD * 2;
  IPoint2 imove = (toroidalOrigin - newTexelsOrigin);
  IPoint2 amove = abs(imove);
  if (amove.x < MOVE_THRESHOLD && amove.y < MOVE_THRESHOLD)
    return UpdateResult::NO_CHANGE;
  const IPoint2 realRes(VOXEL_25D_RESOLUTION_X, VOXEL_25D_RESOLUTION_X);
  IPoint2 res = realRes;
  const bool invalidateAll = gi_25d_force_update_scene.get() || max(amove.x, amove.y) >= VOXEL_25D_RESOLUTION_X;
  if (!invalidateAll) // clear whole scene
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
  }
  toroidalOrigin = newTexelsOrigin;
  setVars();
  const IPoint2 lt = toroidalOrigin - realRes / 2;

  IPoint2 stInvalid = lt;
  if (((stInvalid.x | stInvalid.y | res.x | res.y) & 3) != 0)
    logerr("movement should be aligned %@", lt);
  if (invalidateAll) // clear whole scene
  {
    // float v[4]={0};
    // d3d::clear_rwtexf(sceneVoxels.getVolTex(), v, 0, 0);//not working on xbox1!
    TIME_D3D_PROFILE(ssgi_clear_scene_25d_full);
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), sceneAlpha.getBuf());
    const IPoint2 invalidLT = stInvalid - lt;
    ShaderGlobal::set_color4(scene_25d_voxels_invalid_coord_boxVarId, invalidLT.x, invalidLT.y, invalidLT.x + res.x,
      invalidLT.y + res.y);
    ShaderGlobal::set_color4(scene_25d_voxels_invalid_start_widthVarId, stInvalid.x, stInvalid.y, res.x, res.y);
    ssgi_clear_scene_25d_full_cs->dispatch((res.x + 7) / 8, (res.y + 7) / 8, VOXEL_25D_RESOLUTION_Y); // clear
  }
  else
  {
    const IPoint2 rb = lt + realRes + imove;
    stInvalid = IPoint2(imove.x >= 0 ? lt.x : rb.x, imove.y >= 0 ? lt.y : rb.y);
    if (((stInvalid.x | stInvalid.y | res.x | res.y) & 3) != 0)
      logerr("movement should be aligned %@", lt);

    // debug("move 2.5d %@ stInvalid = %@, res = %@", imove, stInvalid, res);
    // scene_25d_voxels_origin_coord_box@f4 = (scene_25d_voxels_invalid_start_width.x - scene_25d_voxels_origin.z + 128,//128 is
    // VOXEL_25D_RESOLUTION_X/2
    //                                         scene_25d_voxels_invalid_start_width.y - scene_25d_voxels_origin.w + 128,//128 is
    //                                         VOXEL_25D_RESOLUTION_X/2 scene_25d_voxels_invalid_start_width.x +
    //                                         scene_25d_voxels_invalid_start_width.z - scene_25d_voxels_origin.z + 128,//128 is
    //                                         VOXEL_25D_RESOLUTION_X/2 scene_25d_voxels_invalid_start_width.y +
    //                                         scene_25d_voxels_invalid_start_width.w - scene_25d_voxels_origin.w + 128);//128 is
    //                                         VOXEL_25D_RESOLUTION_X/2
    const IPoint2 invalidLT = stInvalid - lt;
    ShaderGlobal::set_color4(scene_25d_voxels_invalid_coord_boxVarId, invalidLT.x, invalidLT.y, invalidLT.x + res.x,
      invalidLT.y + res.y);
    ShaderGlobal::set_color4(scene_25d_voxels_invalid_start_widthVarId, stInvalid.x, stInvalid.y, res.x, res.y);
    {
      TIME_D3D_PROFILE(ssgi_clear_scene_25d_w);
      STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_CS, 0, VALUE), sceneAlpha.getBuf());
      ssgi_clear_scene_25d_cs->dispatch((max(amove.x, amove.y) * VOXEL_25D_RESOLUTION_X + 3) / 4, 1,
        1); // clear
    }
  }

  d3d::resource_barrier({sceneAlpha.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});

  {
    TIME_D3D_PROFILE(voxelize_scene_albedo_initial);

    SCOPE_RENDER_TARGET;
    SCOPE_VIEW_PROJ_MATRIX;

    d3d::set_render_target();
    BBox3 sceneBox;
    sceneBox[0] = Point3::xVy(point2(stInvalid) * voxelSizeXZ, 0);
    sceneBox[1] = Point3::xVy(point2(stInvalid + res) * voxelSizeXZ, +voxelSizeY * VOXEL_25D_RESOLUTION_Y);

    const int voxels_25d_no = ShaderGlobal::get_int(get_shader_variable_id("voxels_25d_const_no"));
    STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_PS, voxels_25d_no, VALUE), sceneAlpha.getBuf());
    voxelize_cb(sceneBox, 0.5 * Point3(voxelSizeXZ, voxelSizeY, voxelSizeXZ)); // decrease voxel size twice, as we supersample alpha
                                                                               // twice
    d3d::resource_barrier({sceneAlpha.getBuf(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE});
  }

  update_pending = (toroidalOrigin != targetOrigin); // only 1 axis moved, the other axis will be moved in the next frame
  const uint32_t updatedTexels = res.x * res.y;
  if (updatedTexels >= VOXEL_25D_RESOLUTION_X * VOXEL_25D_RESOLUTION_X)
    return UpdateResult::TELEPORTED;
  if (updatedTexels >= (VOXEL_25D_RESOLUTION_X * VOXEL_25D_RESOLUTION_X) / 4)
    return UpdateResult::BIG;
  return UpdateResult::MOVED;
}

} // namespace dagi25d