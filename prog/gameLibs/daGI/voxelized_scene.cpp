#include <math/dag_frustum.h>
#include <math/dag_math3d.h>
#include <math/dag_Point2.h>
#include <math/dag_cube_matrix.h>
#include <math/integer/dag_IBBox2.h>
#include <math/integer/dag_IBBox3.h>
#include <daGI/daGI.h>

#include <3d/dag_drv3d.h>
#include <3d/dag_tex3d.h>
#include <shaders/dag_computeShaders.h>
#include <perfMon/dag_statDrv.h>
#include <render/toroidal_update.h>

#include <math/dag_hlsl_floatx.h>
#include "shaders/dagi_voxels_consts.hlsli"
#include "shaders/dagi_scene_common_types.hlsli"
#include "load_data.h"
#include "global_vars.h"
#include <util/dag_convar.h>
#include <textureUtil/textureUtil.h>

enum
{
  VOXELS_RES_X = VOXEL_RESOLUTION_X,
  VOXELS_RES_Y = VOXEL_RESOLUTION_Z,
  VOXELS_RES_Z = VOXEL_RESOLUTION_Y
};
CONSOLE_BOOL_VAL("render", gi_force_revoxelization, false);
CONSOLE_BOOL_VAL("render", gi_force_part_revoxelization, false);

IPoint3 GI3D::getSceneRes(unsigned) const { return IPoint3(VOXELS_RES_X, VOXELS_RES_Y, VOXELS_RES_Z); }

void GI3D::debugSceneVoxelsRayCast(DebugSceneType debug_scene, int cascade)
{
  if (!debug_rasterize_voxels.shader)
    debug_rasterize_voxels.init("ssgi_debug_rasterize_voxels", NULL, 0, "ssgi_debug_rasterize_voxels");
  cascade = clamp<int>(cascade, 0, sceneCascades.size() - 1);
  ShaderGlobal::set_int(ssgi_debug_rasterize_sceneVarId, debug_scene);
  ShaderGlobal::set_int(ssgi_debug_rasterize_scene_cascadeVarId, cascade);
  TIME_D3D_PROFILE(ssgi_debug_rasterize_voxels);
  debug_rasterize_voxels.shader->setStates(0, true);
  d3d::setvsrc(0, 0, 0);
  d3d::draw(PRIM_TRILIST, 0, 1);
}

static carray<int, 4> scene_voxels_originVarId, scene_voxels_bmaxVarId, scene_voxels_toroidal_ofsVarId;

void GI3D::initSceneVoxelsCommon()
{
  if (ssgi_mark_scene_voxels_from_gbuf_cs.get())
    return;
  sceneCascades.resize(3);
  for (int i = 0; i < sceneCascades.size(); ++i)
    sceneCascades[i].scale = 1 << i;
  ssgi_mark_scene_voxels_from_gbuf_cs.reset(new_compute_shader("ssgi_mark_scene_voxels_from_gbuf_cs"));
  ssgi_temporal_scene_voxels_cs.reset(new_compute_shader("ssgi_temporal_scene_voxels_cs"));
  ssgi_clear_scene_cs.reset(new_compute_shader("ssgi_clear_scene_cs"));
  ssgi_temporal_scene_voxels_create_cs.reset(new_compute_shader("ssgi_temporal_scene_voxels_create_cs"));
  ssgi_invalidate_voxels_cs.reset(new_compute_shader("ssgi_ivalidate_voxels_cs"));
  G_ASSERT(ssgi_clear_scene_cs);
  G_ASSERT(ssgi_invalidate_voxels_cs);
  for (int si = 0; si < sceneCascades.size(); ++si)
  {
    scene_voxels_originVarId[si] = get_shader_variable_id(String(128, "scene_voxels_origin%d", si));
    scene_voxels_bmaxVarId[si] = get_shader_variable_id(String(128, "scene_voxels_bmax%d", si));
    scene_voxels_toroidal_ofsVarId[si] = get_shader_variable_id(String(128, "scene_voxels_toroidal_ofs%d", si));
  }

  invalidBoxesSB = dag::buffers::create_ua_sr_structured(sizeof(bbox3f), MAX_BOXES_INV_PER_FRAME, "invalidBoxes");
}

void GI3D::closeSceneVoxels()
{
  if (!sceneVoxelsAlpha.getVolTex())
    return;
  sceneVoxelsColor.close();
  sceneVoxelsAlpha.close();
  selectedGbufVoxelsCount.close();
  selectedGbufVoxels.close();
}

void GI3D::initSceneVoxels()
{
  closeSceneVoxels();
  initSceneVoxelsCommon();
  if (sceneVoxelsAlpha)
    return;
#if SCENE_VOXELS_COLOR == SCENE_VOXELS_R11G11B10
  uint32_t sceneFmt = TEXFMT_R11G11B10F;
#define SCENE_CLEAR_FUNC clear_float3_voltex_via_cs
#elif SCENE_VOXELS_COLOR == SCENE_VOXELS_SRGB8_A8
  uint32_t sceneFmt = TEXCF_SRGBREAD | TEXFMT_R8G8B8A8;
#define SCENE_CLEAR_FUNC clear_uint4_voltex_via_cs
#elif SCENE_VOXELS_COLOR == SCENE_VOXELS_HSV_A8
  uint32_t sceneFmt = TEXFMT_R8G8B8A8;
#define SCENE_CLEAR_FUNC clear_uint4_voltex_via_cs
#endif
  if (getQuality() != ONLY_AO)
  {
    sceneVoxelsColor = dag::create_voltex(VOXELS_RES_X, VOXELS_RES_Y, VOXELS_RES_Z * sceneCascades.size(), sceneFmt | TEXCF_UNORDERED,
      1, "scene_voxels_data");
    texture_util::get_shader_helper()->SCENE_CLEAR_FUNC(sceneVoxelsColor.getVolTex());
#undef SCENE_CLEAR_FUNC
    sceneVoxelsColor->texaddr(TEXADDR_WRAP);
    d3d::resource_barrier({sceneVoxelsColor.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
  }

#if SCENE_VOXELS_COLOR == SCENE_VOXELS_R11G11B10
  sceneVoxelsAlpha = dag::create_voltex(VOXELS_RES_X, VOXELS_RES_Y, VOXELS_RES_Z * sceneCascades.size(), TEXFMT_L8 | TEXCF_UNORDERED,
    1, "scene_voxels_alpha");
  texture_util::get_shader_helper()->clear_float_voltex_via_cs(sceneVoxelsAlpha.getVolTex());
  sceneVoxelsAlpha->texaddr(TEXADDR_WRAP);
  d3d::resource_barrier({sceneVoxelsAlpha.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
#endif
  selectedGbufVoxelsCount = dag::create_sbuffer(4, 4, SBCF_UA_INDIRECT, 0, "voxelsCount");
  selectedGbufVoxels = dag::buffers::create_ua_sr_structured(sizeof(GBUF_VOXEL), MAX_MARKED_SCENE_VOXELS, "selectedGbufVoxels");
  sceneVoxelsAlpha.setVar();
  sceneVoxelsColor.setVar();
  invalidateVoxelScene();
}

void GI3D::invalidateVoxelScene()
{
  for (auto &s : sceneCascades)
    s.invalidate();
  setSceneVoxelVars();
}

void GI3D::updateVoxelSceneBox(const BBox3 &sceneBox) { sceneVoxelBox = sceneBox; }

void GI3D::updateVoxelMinSize(float voxel_resolution)
{
  if (sceneVoxelSize.x != voxel_resolution)
  {
    invalidateVoxelScene();
    sceneVoxelSize = Point3(voxel_resolution, voxel_resolution, voxel_resolution);
    ShaderGlobal::set_color4(scene_voxels_sizeVarId, sceneVoxelSize.x, sceneVoxelSize.y, 1.f / (sceneCascades.size() * VOXELS_RES_Z),
      sceneCascades.size());
  }
  setSceneVoxelVars();
}

void GI3D::setSceneVoxelVars()
{
  for (int si = 0; si < sceneCascades.size(); ++si)
  {
    auto &s = sceneCascades[si];
    const Point3 voxelSize = sceneVoxelSize * s.scale;
    s.origin = mul(voxelSize, Point3(s.toroidalOrigin - IPoint3::xzy(VOXEL_RESOLUTION / 2)));
    ShaderGlobal::set_color4(scene_voxels_originVarId[si], s.origin.x, s.origin.y, s.origin.z, s.scale);
    const Point3 bmax = s.origin + mul(voxelSize, Point3(IPoint3::xzy(VOXEL_RESOLUTION)));
    ShaderGlobal::set_color4(scene_voxels_bmaxVarId[si], bmax.x, bmax.y, bmax.z, voxelSize.x * 0.95);
    IPoint3 ofs = s.toroidalOrigin;
    ShaderGlobal::set_color4(scene_voxels_toroidal_ofsVarId[si], ofs.x, ofs.y, ofs.z, 0);
  }
  // todo: set voxel_origin and size, and others from updateVoxelSceneBox
}

void GI3D::afterResetScene()
{
  invalidateCount = true;
  invalidateVoxelScene();
  if (sceneVoxelsColor.getVolTex())
    d3d::resource_barrier({sceneVoxelsColor.getVolTex(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
  if (sceneVoxelsAlpha.getVolTex())
    d3d::resource_barrier({sceneVoxelsAlpha.getVolTex(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
  sceneVoxelsAlpha.setVar();
  sceneVoxelsColor.setVar();
}

void GI3D::fillRestrictedUpdateBox(TMatrix4 globtm)
{
  if (!get_restricted_update_bbox_cb)
    return;

  TIME_D3D_PROFILE(fill_restricted_update_box);
  const Frustum frustum(globtm);

  const int cascade = 0; // Use only first cascade so that bbox will not be too large.
  const Point3 voxelSize = sceneVoxelSize * sceneCascades[cascade].scale;
  const Point3 bmin = sceneCascades[cascade].origin;
  const Point3 bmax = bmin + mul(voxelSize, Point3(IPoint3::xzy(VOXEL_RESOLUTION)));
  const bbox3f cascade_bbox = v_ldu_bbox3(BBox3(bmin, bmax));

  bbox3f resultBbox = get_restricted_update_bbox_cb(cascade_bbox, frustum, true);
  bool hasPhysObjInCascade = !v_bbox3_is_empty(resultBbox);
  ShaderGlobal::set_int(has_physobj_in_cascadeVarId, hasPhysObjInCascade);
  ShaderGlobal::set_color4(ssgi_restricted_update_bbox_minVarId, resultBbox.bmin);
  ShaderGlobal::set_color4(ssgi_restricted_update_bbox_maxVarId, resultBbox.bmax);
  lastHasPhysObjInCascade = hasPhysObjInCascade;
  lastRestrictedUpdateGiBox = resultBbox;
}

void GI3D::markVoxelsFromRT(float speed)
{
  if (getBouncingMode() == TILL_CONVERGED || !sceneVoxelsColor.getVolTex())
    return;
  TIME_D3D_PROFILE(mark_voxels_from_gbuf);
  if (invalidateCount)
  {
    uint32_t v[4] = {0, 0, 0, 0};
    d3d::clear_rwbufi(selectedGbufVoxelsCount.get(), v);
    d3d::resource_barrier({selectedGbufVoxelsCount.get(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});
    invalidateCount = false;
  }
  TMatrix4 globtm;
  d3d::getglobtm(globtm);

  fillRestrictedUpdateBox(globtm);

  globtm = globtm.transpose();
  ShaderGlobal::set_color4(globtm_psf_0VarId, Color4(globtm[0]));
  ShaderGlobal::set_color4(globtm_psf_1VarId, Color4(globtm[1]));
  ShaderGlobal::set_color4(globtm_psf_2VarId, Color4(globtm[2]));
  ShaderGlobal::set_color4(globtm_psf_3VarId, Color4(globtm[3]));
  const uint32_t groups =
    clamp((uint32_t)(speed * MAX_MARKED_SCENE_VOXELS_GROUPS), (uint32_t)1, (uint32_t)MAX_MARKED_SCENE_VOXELS_GROUPS);
  ShaderGlobal::set_int(ssgi_total_scene_mark_dipatchVarId, groups * MARK_VOXELS_WARP_SIZE);

  SCOPE_RENDER_TARGET;
  d3d::set_render_target();
  d3d::set_rwbuffer(STAGE_CS, 0, selectedGbufVoxelsCount.get());
  d3d::set_rwbuffer(STAGE_CS, 1, selectedGbufVoxels.get());
  ssgi_mark_scene_voxels_from_gbuf_cs->dispatch(groups, 1, 1);
  d3d::set_rwbuffer(STAGE_CS, 1, 0);

  {
    TIME_D3D_PROFILE(fill_voxels_from_gbuf);
    d3d::resource_barrier({selectedGbufVoxelsCount.get(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});
    ssgi_temporal_scene_voxels_create_cs->dispatch(1, 1, 1);

    d3d::resource_barrier({sceneVoxelsColor.getVolTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
    d3d::resource_barrier({sceneVoxelsAlpha.getVolTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});

    d3d::set_rwtex(STAGE_CS, 0, sceneVoxelsColor.getVolTex(), 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, sceneVoxelsAlpha.getVolTex(), 0, 0);
    d3d::resource_barrier({selectedGbufVoxels.get(), RB_RO_SRV | RB_STAGE_COMPUTE});
    d3d::set_buffer(STAGE_CS, 15, selectedGbufVoxels.get());
    d3d::resource_barrier({selectedGbufVoxelsCount.get(), RB_RO_INDIRECT_BUFFER});
    ssgi_temporal_scene_voxels_cs->dispatch_indirect(selectedGbufVoxelsCount.get(), 0);
    d3d::set_rwtex(STAGE_CS, 0, 0, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, 0, 0, 0);
    d3d::resource_barrier({sceneVoxelsColor.getVolTex(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
    d3d::resource_barrier({sceneVoxelsAlpha.getVolTex(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
  }
  d3d::set_buffer(STAGE_CS, 15, 0);

// debug
#if GI3D_VERBOSE_DEBUG
  uint32_t bins[4];
  load_data(bins, selectedGbufVoxelsCount.get(), sizeof(bins));
  debug("gbuf voxels = %d %d %d %d", bins[0], bins[1], bins[2], bins[3]);
#endif
}

IPoint3 GI3D::getTexelOrigin(int scene, const Point3 &baseOrigin) const
{
  Point3 origin = baseOrigin;
  const Point3 voxelSize = sceneVoxelSize * sceneCascades[scene].scale;
  if (!sceneVoxelBox.isempty())
  {
    Point3 voxelSceneSize = mul(voxelSize, Point3::xzy(VOXEL_RESOLUTION));
    origin = Point3(min(max(sceneVoxelBox[0] + voxelSceneSize / 2, baseOrigin), sceneVoxelBox[1] - voxelSceneSize / 2));
    Point3 sceneWidth = sceneVoxelBox.width();
    if (sceneWidth.x < voxelSceneSize.x)
      origin.x = sceneVoxelBox.center().x;
    if (sceneWidth.y < voxelSceneSize.y)
      origin.y = sceneVoxelBox.center().y;
    if (sceneWidth.z < voxelSceneSize.z)
      origin.z = sceneVoxelBox.center().z;
  }
  return ipoint3(floor(div(origin, voxelSize)) + Point3(0.5, 0.5, 0.5));
}

float GI3D::getSceneDist3D() const
{
  if (!sceneCascades.size())
    return 0;
  return sceneVoxelSize.x * sceneCascades.back().scale *
         sqrtf(float(VOXEL_RESOLUTION_X) * VOXEL_RESOLUTION_X + float(VOXEL_RESOLUTION_Y) * VOXEL_RESOLUTION_Y +
               float(VOXEL_RESOLUTION_Z) * VOXEL_RESOLUTION_Z) *
         0.5f;
}

static constexpr int MOVE_THRESHOLD = 4;
GI3D::SceneCascade::STATUS GI3D::updateOriginScene(int scene, const Point3 &baseOrigin, const render_scene_fun_cb &preclear_cb,
  const render_scene_fun_cb &voxelize_cb, bool repeat_last)
{
  if (!ssgi_clear_scene_cs || !sceneVoxelsAlpha.getVolTex())
    return SceneCascade::NOT_MOVED;
  const IPoint3 realRes(VOXEL_RESOLUTION_X, VOXEL_RESOLUTION_Y, VOXEL_RESOLUTION_Z);
  const Point3 voxelSize = sceneVoxelSize * sceneCascades[scene].scale;
  if (!repeat_last)
  {
    IPoint3 newTexelsOrigin = getTexelOrigin(scene, baseOrigin);
    IPoint3 &toroidalSceneOrigin = sceneCascades[scene].toroidalOrigin;
    if (gi_force_revoxelization.get())
      toroidalSceneOrigin.x -= 10000;
    else if (gi_force_part_revoxelization.get() && abs(toroidalSceneOrigin.x - newTexelsOrigin.x) < MOVE_THRESHOLD)
    {
      newTexelsOrigin.x += MOVE_THRESHOLD * 2;
    }
    // newTexelsOrigin.y = toroidalSceneOrigin.y;

    IPoint3 imove = (toroidalSceneOrigin - newTexelsOrigin);
    IPoint3 amove = abs(imove);
    const int maxMove = max(max(amove.x, amove.y), amove.z);
    if (maxMove < MOVE_THRESHOLD)
    {
      sceneCascades[scene].lastStatus = SceneCascade::NOT_MOVED;
      return sceneCascades[scene].lastStatus;
    }
    // debug("toroidalSceneOrigin = %@, imove = %@, amove = %@", toroidalSceneOrigin, imove, amove);
    IPoint3 res = realRes;
    IPoint3 moveDir(0, 0, 0);
    if (amove.x >= realRes.x || amove.y >= realRes.y || amove.z >= realRes.z) // clear whole scene
    {
      toroidalSceneOrigin = newTexelsOrigin;
      imove = IPoint3(0, 0, 0);
      sceneCascades[scene].lastStatus = SceneCascade::TELEPORTED;
    }
    else
    {
      enum AXIS
      {
        X,
        Y,
        Z
      } axis = X;
      if (amove.x == maxMove)
        axis = X;
      else if (amove.z == maxMove)
        axis = Z;
      else if (amove.y == maxMove)
        axis = Y;
      switch (axis)
      {
        case X:
          imove.y = imove.z = 0;
          res.x = amove.x;
          moveDir.x = imove.x < 0;
          break;
        case Y:
          imove.x = imove.z = 0;
          res.y = amove.y;
          moveDir.y = imove.y < 0;
          break;
        case Z:
          imove.y = imove.x = 0;
          res.z = amove.z;
          moveDir.z = imove.z < 0;
          break;
      };
      newTexelsOrigin = (toroidalSceneOrigin - imove);
      toroidalSceneOrigin = newTexelsOrigin;
      sceneCascades[scene].lastStatus = SceneCascade::MOVED;
    }

    setSceneVoxelVars();

    TIME_D3D_PROFILE(ssgi_clear_scene_w);
    if (sceneVoxelsColor.getVolTex())
      d3d::set_rwtex(STAGE_CS, 0, sceneVoxelsColor.getVolTex(), 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, sceneVoxelsAlpha.getVolTex(), 0, 0);

    IPoint3 lt = toroidalSceneOrigin - realRes / 2;
    IPoint3 rb = lt + realRes + imove;
    sceneCascades[scene].lastStInvalid = IPoint3(imove.x >= 0 ? lt.x : rb.x, imove.y >= 0 ? lt.y : rb.y, imove.z >= 0 ? lt.z : rb.z);
    sceneCascades[scene].lastRes = res;
    // debug("not repeat %@", imove);
  }
  TIME_D3D_PROFILE(scene_voxelization);
  const IPoint3 stInvalid = sceneCascades[scene].lastStInvalid, res = sceneCascades[scene].lastRes;

  // debug("toroidalSceneOrigin = %@, res = %@, stInvalid = %@", sceneCascades[scene].toroidalOrigin, res, stInvalid);
  // ShaderGlobal::set_color4(scene_voxels_originVarId[si], s.origin.x, s.origin.y, s.origin.z, s.scale);

  const IPoint3 unLt = stInvalid - (sceneCascades[scene].toroidalOrigin - realRes / 2);
  ShaderGlobal::set_color4(scene_voxels_unwrapped_invalid_startVarId, unLt.x, unLt.y, unLt.z, scene);
  ShaderGlobal::set_color4(scene_voxels_invalid_startVarId, stInvalid.x, stInvalid.y, stInvalid.z, scene);
  ShaderGlobal::set_color4(scene_voxels_invalid_widthVarId, res.x, res.y, res.z, res.x * res.y * res.z);
  BBox3 sceneBox;
  sceneBox[0] = mul(point3(stInvalid), voxelSize);
  sceneBox[1] = sceneBox[0] + mul(point3(res), voxelSize);
  {
    Point3 voxelize_box0 = sceneBox[0];
    Point3 voxelize_box_sz = max(Point3(1e-6f, 1e-6f, 1e-6f), sceneBox.width());
    // Point3 voxelize_box1 = Point3(1./sceneBox.width().x, 1./sceneBox.width().y, 1./sceneBox.width().z);
    const int maxRes = max(max(res.x, res.y), max(res.z, 1));
    Point3 voxelize_aspect_ratio = point3(max(res, IPoint3(1, 1, 1))) / maxRes; // use fixed aspect ratio of 1. for oversampling and HW
                                                                                // clipping
    Point3 mult = 2. * div(voxelize_aspect_ratio, voxelize_box_sz);
    Point3 add = -mul(voxelize_box0, mult) - voxelize_aspect_ratio;
    ShaderGlobal::set_color4(voxelize_world_to_rasterize_space_mulVarId, P3D(mult), 0);
    ShaderGlobal::set_color4(voxelize_world_to_rasterize_space_addVarId, P3D(add), 0);
  }

  preclear_cb(sceneBox, voxelSize);
  if (!repeat_last)
  {
    TIME_D3D_PROFILE(scene_clear);
    ssgi_clear_scene_cs->dispatch((res.x * res.y * res.z + 31) / 32, 1, 1); // init with something reasonable
    d3d::resource_barrier({sceneVoxelsAlpha.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
    if (sceneVoxelsColor.getVolTex())
      d3d::resource_barrier({sceneVoxelsColor.getVolTex(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE, 0, 0});
    d3d::resource_barrier({sceneVoxelsAlpha.getVolTex(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE, 0, 0});
  }

  d3d::set_rwtex(STAGE_CS, 0, 0, 0, 0);
  d3d::set_rwtex(STAGE_CS, 1, 0, 0, 0);

  {
    TIME_D3D_PROFILE(voxelize_scene_initial);

    // remove me, has to be done by calling code
    SCOPE_VIEW_PROJ_MATRIX;
    SCOPE_RENDER_TARGET;

    voxelize_cb(sceneBox, voxelSize);
    d3d::resource_barrier({sceneVoxelsAlpha.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
    if (sceneVoxelsColor.getVolTex())
      d3d::resource_barrier({sceneVoxelsColor.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
  }
  setSceneVoxelVars();
  return sceneCascades[scene].lastStatus;
}

bool GI3D::isSceneTeleported(int scene, const Point3 &origin) const
{
  if (gi_force_revoxelization.get())
    return true;
  const IPoint3 realRes(VOXEL_RESOLUTION_X, VOXEL_RESOLUTION_Y, VOXEL_RESOLUTION_Z);
  const IPoint3 newTexelsOrigin = getTexelOrigin(scene, origin);
  const IPoint3 toroidalSceneOrigin = sceneCascades[scene].toroidalOrigin;
  // newTexelsOrigin.y = toroidalSceneOrigin.y;

  const IPoint3 imove = (toroidalSceneOrigin - newTexelsOrigin);
  const IPoint3 amove = abs(imove);
  return max(amove.x, amove.z) >= realRes.x || amove.y >= realRes.y;
}

bool GI3D::isSceneTeleported(const Point3 &origin) const
{
  if (!sceneCascades.size())
    return false;
  return isSceneTeleported(max((int)sceneCascades.size() - 2, (int)0), origin);
}

bool GI3D::updateOriginScene(const Point3 &baseOrigin, const render_scene_fun_cb &preclear_cb, const render_scene_fun_cb &voxelize_cb,
  int max_scenes_to_update, bool repeat_last)
{
  bool sceneMoved = false;
  if (!ssgi_clear_scene_cs || !sceneVoxelsAlpha.getVolTex())
    return false;
  if (gi_force_part_revoxelization.get())
    max_scenes_to_update = sceneCascades.size();
  for (int scene = sceneCascades.size() - 1; scene >= 0; --scene)
  {
    auto ret = updateOriginScene(scene, baseOrigin, preclear_cb, voxelize_cb, repeat_last);
    // TODO: isSceneTeleported only takes into account 1 of the cascades, so in the teleport code path it can return MOVED
    // and in the movement code path TELEPORTED.
    if (ret == SceneCascade::MOVED) // on teleport render all cascades
    {
      sceneMoved = true;
      if (--max_scenes_to_update <= 0)
        return sceneMoved;
    }
  }
  return sceneMoved;
}

int GI3D::sceneXZResolution() const { return VOXEL_RESOLUTION_X; }
int GI3D::sceneYResolution() const { return VOXEL_RESOLUTION_Y; }
int GI3D::getCoordMoveThreshold() const { return MOVE_THRESHOLD; }
float GI3D::sceneDistanceXZ(uint32_t cascade) const { return VOXEL_RESOLUTION_X * sceneVoxelSize.x * sceneCascades[cascade].scale; }
float GI3D::sceneDistanceY(uint32_t cascade) const { return VOXEL_RESOLUTION_Y * sceneVoxelSize.y * sceneCascades[cascade].scale; }
float GI3D::sceneDistanceMoveThreshold(uint32_t cascade) const
{
  return MOVE_THRESHOLD * sceneVoxelSize.x * sceneCascades[cascade].scale;
}

float GI3D::sceneDistanceXZ() const { return sceneDistanceXZ(getSceneCascadesCount() - 1); }
float GI3D::sceneDistanceY() const { return sceneDistanceY(getSceneCascadesCount() - 1); }
float GI3D::sceneDistanceMoveThreshold() const { return sceneDistanceMoveThreshold(getSceneCascadesCount() - 1); }

BBox3 GI3D::getSceneBox(uint32_t cascade) const
{
  cascade = min<int>(cascade, getSceneCascadesCount() - 1);
  const IPoint3 realRes(VOXEL_RESOLUTION_X, VOXEL_RESOLUTION_Y, VOXEL_RESOLUTION_Z);
  IPoint3 lt = sceneCascades[cascade].toroidalOrigin - realRes / 2;
  const Point3 voxelSize = sceneVoxelSize * sceneCascades[cascade].scale;

  BBox3 sceneBox;
  sceneBox[0] = mul(point3(lt), voxelSize);
  sceneBox[1] = sceneBox[0] + mul(point3(realRes), voxelSize);
  return sceneBox;
}
