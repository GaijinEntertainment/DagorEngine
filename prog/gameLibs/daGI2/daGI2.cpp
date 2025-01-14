// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_convar.h>
#include <perfMon/dag_statDrv.h>
#include <daSDF/worldSDF.h>
#include <daSDF/objectsSDF.h>
#include <shaders/dag_shaders.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_rwResource.h>
#include <frustumCulling/frustumPlanes.h>
#include <render/viewVecs.h>
#include <render/globTMVars.h>
#include <math/dag_frustum.h>
#include "giFrameInfo.h"
#include "screenSpaceProbes.h"
#include "volumetricGI.h"
#include "daGIVoxelScene.h"
#include "daGIAlbedoScene.h"
#include "radianceGrid.h"
#include "daGIMediaScene.h"

#include "skyVisibility.h"
#include <daGI2/daGI2.h>

CONSOLE_BOOL_VAL("gi", gi_screen_probes, true);
#if RADIANCE_CACHE
#include "radianceCache.h"
CONSOLE_BOOL_VAL("gi", gi_radiance_cache_update_all, true);
CONSOLE_BOOL_VAL("gi", gi_radiance_cache_update_position, true);
CONSOLE_BOOL_VAL("gi", gi_radiance_cache_update_probes, true);
CONSOLE_INT_VAL("gi", gi_debug_radiance_cache_type, 0, 0, 2);
#endif

CONSOLE_BOOL_VAL("gi", gi_world_sdf_update, true);
CONSOLE_BOOL_VAL("gi", gi_world_sdf_update_from_gbuf, true);
CONSOLE_BOOL_VAL("gi", gi_remove_sdf_from_depth, true);
CONSOLE_BOOL_VAL("gi", gi_update_from_gbuf, true);
CONSOLE_BOOL_VAL("gi", gi_update_from_gbuf_stable, true);

CONSOLE_BOOL_VAL("gi", gi_world_sdf_rt, false);
CONSOLE_INT_VAL("gi", gi_debug_volumetric_probes_type, 0, 0, 2);
CONSOLE_INT_VAL("gi", gi_debug_screen_space_probes_type, 0, 0, 8);
CONSOLE_BOOL_VAL("gi", gi_debug_screen_space_probes_tiles_classificator, false);
CONSOLE_INT_VAL("gi", gi_draw_sequence, 0, 0, 1 << 20);
CONSOLE_INT_VAL("gi", gi_debug_sky_visibility, 0, 0, 9);

CONSOLE_BOOL_VAL("gi", gi_radiance_grid_update_position, true);
CONSOLE_INT_VAL("gi", gi_debug_radiance_grid_type, 0, 0, 4);
CONSOLE_INT_VAL("gi", gi_debug_irradiance_grid_type, 0, 0, 6);

#define GLOBAL_VARS_LIST VAR(world_view_pos)

#define GLOBAL_VARS_OPT_LIST VAR(dagi_warp64_irradiance)

#define VAR(a) static ShaderVariableInfo a##VarId(#a);
GLOBAL_VARS_LIST
#undef VAR

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_OPT_LIST
#undef VAR

struct DaGIImpl final : public DaGI
{
  bool sdfClipHasVoxelsLitSceneClip(uint32_t sdf_clip) const
  {
    if (!voxelScene || !worldSdf)
      return false;
    return uint32_t(sdf_clip - voxelScene->firstSdfClip) < uint32_t(voxelScene->clips);
  }
  void afterReset();
  void debugRenderTrans();
  void debugRenderScreenDepth();
  void debugRenderScreen();
  void requestUpdatePosition(const request_sdf_radiance_data_cb &sdf_cb, const cancel_sdf_radiance_data_cb &cancel_sdf_cb,
    const request_albedo_data_cb &albedo_cb, const cancel_albedo_data_cb &cancel_albedo_cb);
  void updatePosition(const rasterize_sdf_radiance_cb &sdf_cb, const rasterize_albedo_cb &albedo_cb);
  void setSettings(const DaGI::Settings &s) { nextSettings = s; }

  void fix(DaGI::Settings &s) const;
  DaGI::Settings getCurrentSettings() const { return currentSettings; }
  DaGI::Settings getNextSettings() const { return nextSettings; }
  const WorldSDF &getWorldSDF() const { return *worldSdf; }
  WorldSDF &getWorldSDF() { return *worldSdf; }
  void beforeRender(uint32_t sw, uint32_t sh, uint32_t maxw, uint32_t maxh, const TMatrix &viewItm, const TMatrix4 &projTm, float zn,
    float zf);
  void beforeFrameLit(float quality);
  void afterFrameRendered(FrameData frame_featues);

  eastl::unique_ptr<WorldSDF> worldSdf;
  DaGIImpl()
  {
    unoderedTypedLoad = !is_only_low_gi_supported();

    debug("waveOps %d, warpSize %d %d", d3d::get_driver_desc().caps.hasWaveOps, d3d::get_driver_desc().minWarpSize,
      d3d::get_driver_desc().maxWarpSize);
    ShaderGlobal::set_int(dagi_warp64_irradianceVarId,
      d3d::get_driver_desc().caps.hasWaveOps && (d3d::get_driver_desc().minWarpSize == 64));
    worldSdf.reset(new_world_sdf());
#if RADIANCE_CACHE
    radianceCache.reset(new RadianceCache);
#endif
    // these below are not needed on low quality
  }
  eastl::unique_ptr<RadianceGrid> radianceGrid;
#if RADIANCE_CACHE
  eastl::unique_ptr<RadianceCache> radianceCache;
#endif
  eastl::unique_ptr<ScreenSpaceProbes> screenProbes;
  eastl::unique_ptr<VolumetricGI> volumetricGI;

  DaGI::Settings currentSettings, nextSettings;
  uint32_t maxSW = 1920, maxSH = 1080;
  struct View : public DaGIFrameInfo
  {
    Frustum frustum;
    Point3 giPos = {0, 0, 0};
    uint32_t sw = 0, sh = 0;
  } view;
  uint32_t frame = 0;
  eastl::unique_ptr<DaGIMediaScene> mediaScene;
  eastl::unique_ptr<DaGIVoxelScene> voxelScene;
  eastl::unique_ptr<DaGIAlbedoScene> albedoScene;
  eastl::unique_ptr<SkyVisibility> skyVisibility;
  UniqueBuf commonTempBuffer;
  bool unoderedTypedLoad = true;
  void workAroundUAVslots()
  {
    // Workaround for RW texture stuck in u slot, conflicting with RT on DX11.
    for (int i = 0; i < Driver3dRenderTarget::MAX_SIMRT; ++i)
      d3d::set_rwtex(STAGE_PS, i, nullptr, 0, 0);
    ShaderElement::invalidate_cached_state_block();
  }
};

static void set_view_vars(const DaGIImpl::View &view)
{
  ShaderGlobal::set_color4(world_view_posVarId, view.world_view_pos.x, view.world_view_pos.y, view.world_view_pos.z, 0);
  set_globtm_to_shader(view.globTm);
  set_frustum_planes(view.frustum);
  set_viewvecs_to_shader(view.viewTm, view.projTm);
}


static const float minTraceDist = 16.f;
static float calc_probe0_size(uint16_t clipW, uint16_t clipH, uint16_t clips, uint16_t sdfW, uint16_t sdfH, uint16_t sdf_clips,
  float sdf_voxel0)
{
  uint32_t w = sdfW & ~7, h = uint32_t(sdfH) & ~7;
  w <<= sdf_clips - 1;
  h <<= sdf_clips - 1;
  float distW = max(minTraceDist, w * sdf_voxel0 - minTraceDist), distH = max(minTraceDist, h * sdf_voxel0 - minTraceDist);
  distW /= clipW << (clips - 1);
  distH /= clipH << (clips - 1);
  return min(distW, distH);
}

static inline float calc_albedo_voxel_size(float albedoVoxel0Size, float sdfVoxelSize, int sdf_clips, int clips)
{
  const float maxVoxelSize = 2.0 * WorldSDF::get_voxel_size(sdfVoxelSize, sdf_clips - 1), minVoxelSize = maxVoxelSize / (1 << clips);
  return clamp(albedoVoxel0Size, minVoxelSize, maxVoxelSize);
}

void DaGIImpl::fix(DaGI::Settings &s) const
{
  uint16_t yRes = s.sdf.texWidth * s.sdf.yResScale;
  worldSdf->fixup_settings(s.sdf.texWidth, yRes, s.sdf.clips);
  s.sdf.yResScale = float(yRes) / s.sdf.texWidth;
  s.sdf.voxel0Size = clamp(s.sdf.voxel0Size, 0.0625f, 4.f);

  s.voxelScene.firstSDFClip = clamp<int>(s.voxelScene.firstSDFClip, 0, s.sdf.clips - 1);
  if (s.voxelScene.clips == 0)
    s.voxelScene.clips = s.sdf.clips;
  s.voxelScene.clips = clamp<int>(s.voxelScene.clips, 1, s.sdf.clips - s.voxelScene.firstSDFClip);
  s.voxelScene.sdfResScale = unoderedTypedLoad ? min(s.voxelScene.sdfResScale, 1.f) : 0.f;

  if (s.radianceGrid.w)
    RadianceGrid::fixup_settings(s.radianceGrid.w, s.radianceGrid.d, s.radianceGrid.clips, s.radianceGrid.additionalIrradianceClips,
      s.radianceGrid.irradianceProbeDetail);

#if RADIANCE_CACHE
  s.radianceCache.probesMax = clamp(s.radianceCache.probesMax, 1.f, 16.f);
  if (s.radianceCache.yResScale <= 0.f)
    s.radianceCache.yResScale = s.sdf.yResScale;
  s.radianceCache.probesMin = clamp(s.radianceCache.probesMin, 256u, 65536u);

  yRes = s.radianceCache.xzRes * s.radianceCache.yResScale;
  RadianceCache::fixup_clipmap_settings(s.radianceCache.xzRes, yRes, s.radianceCache.clips);
  s.radianceCache.yResScale = float(yRes) / s.radianceCache.xzRes;

  if (s.radianceCache.probe0Size <= 0.f)
  {
    s.radianceCache.probe0Size = calc_probe0_size(s.radianceCache.xzRes, s.radianceCache.xzRes * s.radianceCache.yResScale,
      s.radianceCache.clips, s.sdf.texWidth, s.sdf.texWidth * s.sdf.yResScale, s.sdf.clips, s.sdf.voxel0Size);
  }
  s.radianceCache.probe0Size = clamp(s.radianceCache.probe0Size, s.sdf.voxel0Size, 16.f);
#endif

  if (s.volumetricGI.tileSize == 0)
    s.volumetricGI.tileSize = s.screenProbes.tileSize * 4;

  s.volumetricGI.tileSize =
    s.volumetricGI.tileSize ? clamp<uint16_t>(s.volumetricGI.tileSize, max<uint16_t>(8, s.screenProbes.tileSize * 2), 256) : 0;
  s.volumetricGI.spatialFilters = min<uint8_t>(s.volumetricGI.spatialFilters, 3);
  s.volumetricGI.radianceRes = s.volumetricGI.radianceRes
                                 ? clamp<uint8_t>(max<uint8_t>(s.screenProbes.radianceOctRes / 4, s.volumetricGI.radianceRes), 3, 8)
                                 : (s.screenProbes.radianceOctRes * 3) / 8;

  if (s.albedoScene.clips)
  {
    uint8_t w = 1, d = 1;
    DaGIAlbedoScene::fixup_settings(w, d, s.albedoScene.clips);
    s.albedoScene.voxel0Size = calc_albedo_voxel_size(s.albedoScene.voxel0Size, s.sdf.voxel0Size, s.sdf.clips, s.albedoScene.clips);
    const float maxAlbedoVoxelSize = DaGIAlbedoScene::clip_voxel_size(s.albedoScene.voxel0Size, s.albedoScene.clips - 1);
    for (; s.voxelScene.sdfResScale && s.voxelScene.clips >= 1; --s.voxelScene.clips)
    {
      const int sdfClip = s.voxelScene.clips + s.voxelScene.firstSDFClip - 1;
      float litVoxelSize = s.voxelScene.sdfResScale * WorldSDF::get_voxel_size(s.sdf.voxel0Size, sdfClip);

      if (litVoxelSize < maxAlbedoVoxelSize * 0.5)
        break;
    }
  }
}

DaGI *create_dagi() { return new DaGIImpl; }

void DaGIImpl::debugRenderScreenDepth()
{
  set_view_vars(view);
  if (gi_world_sdf_rt)
    worldSdf->debugRender();
  if (albedoScene)
    albedoScene->debugRenderScreen();
}

void DaGIImpl::debugRenderScreen()
{
  set_view_vars(view);
  if (screenProbes && gi_debug_screen_space_probes_tiles_classificator)
    screenProbes->drawDebugTileClassificator();
  if (screenProbes && gi_draw_sequence)
    screenProbes->drawSequence(gi_draw_sequence.get());
}

void DaGIImpl::debugRenderTrans()
{
  set_view_vars(view);
  if (mediaScene)
    mediaScene->debugRender();
  if (voxelScene)
    voxelScene->debugRender();
  if (albedoScene)
    albedoScene->debugRender();

  if (screenProbes && gi_debug_screen_space_probes_type.get() > 0)
    screenProbes->drawDebugRadianceProbes(gi_debug_screen_space_probes_type.get() - 1);
  if (volumetricGI && gi_debug_volumetric_probes_type.get() > 0)
    volumetricGI->drawDebug(gi_debug_volumetric_probes_type.get() - 1);

  if (gi_debug_radiance_grid_type.get() > 0 && radianceGrid)
    radianceGrid->drawDebug(gi_debug_radiance_grid_type.get() - 1);
  if (gi_debug_irradiance_grid_type.get() > 0 && radianceGrid)
    radianceGrid->drawDebugIrradiance(gi_debug_irradiance_grid_type.get() - 1);
#if RADIANCE_CACHE
  if (radianceCache && gi_debug_radiance_cache_type.get() > 0)
    radianceCache->drawDebug(gi_debug_radiance_cache_type.get() - 1);
#endif
  if (skyVisibility && gi_debug_sky_visibility.get() > 0)
    skyVisibility->drawDebug(gi_debug_sky_visibility.get() - 1);
}

void DaGIImpl::requestUpdatePosition(const request_sdf_radiance_data_cb &, const cancel_sdf_radiance_data_cb &,
  const request_albedo_data_cb &, const cancel_albedo_data_cb &)
{}

void DaGIImpl::updatePosition(const rasterize_sdf_radiance_cb &sdf_cb, const rasterize_albedo_cb &albedo_cb)
{
  if (!gi_world_sdf_update)
    return;
  workAroundUAVslots();
  // fixme gi position
  view.giPos = view.world_view_pos; // fixme: gi position is not same as view position!
  set_view_vars(view);

  const Point3 &pos = view.giPos;
  bool renderedSdf = false;
  DA_PROFILE_GPU;
  if (albedoScene)
    albedoScene->update(pos, false,
      [&](const BBox3 &b, float voxel_size, uintptr_t &h) { return (UpdateAlbedoStatus)albedo_cb(b, voxel_size, h); });
  if (voxelScene)
    voxelScene->update();
  if (mediaScene)
    mediaScene->updatePos(pos);

  worldSdf->update(
    pos,
    [&](const bbox3f *, int, UpdateSDFQualityRequest, int, uint32_t &) -> UpdateSDFQualityStatus {
      return UpdateSDFQualityStatus::ALL_OK;
    },
    [&](const bbox3f, int) { return true; },
    [&](int world_clip, const BBox3 &box, float voxelSize, int /*max_instances*/) {
      if (voxelScene && renderedSdf)
        voxelScene->rbNone();
      uintptr_t handle = 0;
      renderedSdf = sdf_cb(world_clip, box, voxelSize, handle) == UpdateGiQualityStatus::RENDERED;
      if (voxelScene && renderedSdf)
        voxelScene->rbFinish();
      return renderedSdf;
    });
  if (voxelScene && renderedSdf)
    voxelScene->rbFinish(); // after copy slice

  if (gi_radiance_grid_update_position && radianceGrid)
  {
    radianceGrid->updatePos(pos);
  }
  if (skyVisibility)
    skyVisibility->updatePos(pos, false);
  /*
  carray<uint8_t, 16> worldClipToObjectMip;
  if (objectsSdf)
  {
    const int maxSDFMip = objectsSdf->getMipsCount()-1;
    const float scaleThreshold = world_sdf_to_object_sdf_mip_scale.get();
    int curMip = 0;
    for (int i = 0, e = worldSdf->getClipsCount(); i < e; ++i)
    {
      while (curMip < maxSDFMip && worldSdf->getVoxelSize(i) > objectsSdf->getMipSize(curMip)*scaleThreshold)
        ++curMip;
      worldClipToObjectMip[i] = curMip;
    }
  }

  dag::Vector<uint8_t, framemem_allocator> mipObjs;
  mipObjs.resize(lruColl.size(), objectsSdf ? objectsSdf->getMipsCount() : 0);
  bool hasUpdatedPrefetch = false;

  //actual update SDF
  worldSdf->update(view.viewItm.getcol(3),
    [&](const bbox3f *boxes, int boxes_cnt, UpdateSDFQualityRequest quality, int world_clip, uint32_t &
  instances_cnt)->UpdateSDFQualityStatus
    {
      if (!objectsSdf || !world_sdf_objects_sdf)
        return UpdateSDFQualityStatus::ALL_OK;
      bbox3f combined = boxes[0];
      for (int i = 1; i < boxes_cnt; ++i)
        v_bbox3_add_box(combined, boxes[i]);
      Tab<uint8_t> obj_mips(framemem_ptr());
      obj_mips.resize(lruColl.size(), objectsSdf->getMipsCount());
      dag::Vector<mat43f, framemem_allocator> inst;
      dag::Vector<uint16_t, framemem_allocator> objs;
      int fast = 0;
      // gather all instances
      lruColl.gatherBox(combined, [&](uint16_t obj_id, mat43f_cref m, bbox3f_cref local_bbox, bbox3f_cref aabb)
      {
        ++fast;
        if (boxes_cnt != 1)
        {
          int i = 0;
          do
          {
            if (v_bbox3_test_box_intersect(boxes[i], aabb)) // not even intersecting
              break;
          } while (++i < boxes_cnt);
          if (i >= boxes_cnt)
            return;
        }
        inst.push_back(m);
        objs.push_back(obj_id);
        // todo : check instance scale may be?
        // 'correct' mip is max(0, obj_mips[obj_id] - log2(sqrt(max (length3_sq(col0), length3_sq(col1), length3_sq(col2)))))
        obj_mips[obj_id] = min(worldClipToObjectMip[world_clip], obj_mips[obj_id]);
      },
      LRUCollision::AlwaysAccept());
      debug("clip %d(%d): combined box %@ %@ of %d, gathered %d / %d", world_clip, worldClipToObjectMip[world_clip],
        *(const Point3*)&combined.bmin, *(const Point3*)&combined.bmax, boxes_cnt, objs.size(), fast);
      UpdateSDFQualityStatus status = objectsSdf->checkObjects(quality, obj_mips.data(), obj_mips.size());
      if (world_sdf_precache && status != UpdateSDFQualityStatus::ALL_OK)
        logerr("wasn't precached");
      instances_cnt = (status == UpdateSDFQualityStatus::ALL_OK) ? objectsSdf->uploadInstances(objs.data(), inst.data(), objs.size()) :
  0; return status;
    },
    [&](const bbox3f box, int world_clip)
    {
      // this is PRE caching (warming up)
      // can be optimized with :
      //  * readback from GPU
      //  * if we use accelerated structures,
      if (!world_sdf_precache || !world_sdf_objects_sdf)
        return true;
      const int curObjectMip = worldClipToObjectMip[world_clip];
      debug("prefetch clip %d(%d): world box %@ %@",
        world_clip, curObjectMip, *(Point3*)(&box.bmin), *(Point3*)(&box.bmax));
      lruColl.gatherBox(box,
        [&](uint16_t oi, mat43f_cref, bbox3f_cref, bbox3f_cref) {mipObjs[oi] = curObjectMip; hasUpdatedPrefetch = true;},
        [&](uint16_t oi) {return mipObjs[oi] <= curObjectMip ? LRUCollision::ObjectClass::Skip : LRUCollision::ObjectClass::Accept;});
      return true;
    },
    [&](int world_clip, const BBox3 &box, float voxelSize, int max_instances)
    {
      debug("render called %d:%@", world_clip, box);
      const bool sdf_collsion = lruColl.voxelizeCollisionElem && world_sdf_from_collision;
      const bool intersectLevel = levelBox&box;
      if (!sdf_collsion && !intersectLevel)
        return false;
      //checking for max_instances is just a hint. We could ALWAYS render if we have something to render
      const bool intersectCollision = sdf_collsion &&
        (!world_sdf_objects_sdf || (worldClipToObjectMip[world_clip] <= 0 && max_instances));
      if (!intersectCollision && !intersectLevel)
        return false;
      BBox3 sceneBox = box;
      const int voxelsAround  = worldSdf->rasterizationVoxelAround();
      sceneBox[0] -= Point3(voxelSize,voxelSize,voxelSize); // we increase by maximum written + 1 where we rounding due to
  rasterization sceneBox[1] += Point3(voxelSize,voxelSize,voxelSize);

      Point3 voxelize_box0 = sceneBox[0];
      Point3 voxelize_box_sz = max(Point3(1e-6f, 1e-6f, 1e-6f), sceneBox.width());
      IPoint3 res = ipoint3(floor(sceneBox.width()/voxelSize + Point3(0.5,0.5,0.5)));
      // Point3 voxelize_box1 = Point3(1./sceneBox.width().x, 1./sceneBox.width().y, 1./sceneBox.width().z);
      const int maxRes = max(max(res.x, res.y), max(res.z, 1));
      Point3 voxelize_aspect_ratio = point3(max(res, IPoint3(1, 1, 1))) / maxRes; // use fixed aspect ratio of 1. for oversampling and
  HW
                                                                                  // clipping
      Point3 mult = 2. * div(voxelize_aspect_ratio, voxelize_box_sz);
      Point3 add = -mul(voxelize_box0, mult) - voxelize_aspect_ratio;
      ShaderGlobal::set_color4(voxelize_world_to_rasterize_space_mulVarId, P3D(mult), 0);
      ShaderGlobal::set_color4(voxelize_world_to_rasterize_space_addVarId, P3D(add), 0);

      if (intersectCollision)
      {
        TIME_D3D_PROFILE(sdf_from_collision)
        //sceneBox[0] -= box.width()*0.25f;
        //sceneBox[1] += box.width()*0.25f;
        bbox3f vbox;
        vbox.bmin = v_ldu(&sceneBox[0].x);
        vbox.bmax = v_ldu(&sceneBox[1].x);
        //vbox.bmin = v_sub(vbox.bmin, v_splats(voxelSize*2));//we rasterize with up 2 voxels dist
        //vbox.bmax = v_add(vbox.bmax, v_splats(voxelSize*2));//we rasterize with up 2 voxels dist
        dag::Vector<uint64_t, framemem_allocator> handles;
        handles.reserve(max_instances);
        lruColl.gatherBox(vbox, handles);
        if (handles.size())
        {
          if (voxelSize <= world_sdf_from_collision_rasterize_below)
          {
            SCOPE_RENDER_TARGET;
            ssgi.setSubsampledTargetAndOverride(maxRes*world_sdf_rasterize_supersample, maxRes*world_sdf_rasterize_supersample);
            bool prims = rasterize_sdf_prims.get() && d3d::get_driver_desc().shaderModel < 6.1_sm;
            lruColl.rasterize(dag::Span<uint64_t>(handles.data(), handles.size()), tex, lruColl.voxelizeCollisionElem, 3, nullptr,
  prims); ssgi.resetOverride(); } else
          {
            lruColl.voxelizeCompute(dag::Span<uint64_t>(handles.data(), handles.size()), tex);
          }
        }
      }
      if (intersectLevel)
      {
        renderVoxelsMediaGeom(sceneBox, Point3(voxelSize,voxelSize,voxelSize)/world_sdf_rasterize_supersample,
  SCENE_MODE_VOXELIZE_SDF);
      }
      return true;
    }
  );
  // pre-warm gathered info
  if (hasUpdatedPrefetch)
    objectsSdf->checkObjects(UpdateSDFQualityRequest::ASYNC_REQUEST_PRECACHE, mipObjs.data(), mipObjs.size());
  */
  workAroundUAVslots();
}

void DaGIImpl::afterReset()
{
  if (radianceGrid)
    radianceGrid->afterReset();
  if (screenProbes)
    screenProbes->afterReset();
  if (skyVisibility)
    skyVisibility->afterReset();
  if (volumetricGI)
    volumetricGI->afterReset();
  if (mediaScene)
    mediaScene->afterReset();
  if (albedoScene)
    albedoScene->afterReset();
  if (voxelScene)
    voxelScene->afterReset();
  if (worldSdf)
    worldSdf->fullReset();

#if RADIANCE_CACHE
  G_ASSERTF(0, "fixme: reset support are not implemented yet");
#endif
}

void DaGIImpl::afterFrameRendered(FrameData frame_featues)
{
  DA_PROFILE_GPU;
  if (gi_update_from_gbuf)
  {
    if (frame_featues & FrameHasDynamic)
    {
      if (gi_remove_sdf_from_depth && ((frame++) & 1))
        worldSdf->removeFromDepth();
      else if (gi_world_sdf_update_from_gbuf)
        worldSdf->markFromGbuffer();
      if (voxelScene && (frame_featues & FrameHasLitScene))
        voxelScene->updateFromGbuf();
      if (albedoScene && (frame_featues & FrameHasAlbedo))
        albedoScene->updateFromGbuf();
      if (mediaScene && (frame_featues & FrameHasAlbedo))
        mediaScene->updateFromGbuf();
    }
    else if (frame_featues & FrameHasDepth)
      worldSdf->removeFromDepth();
  }
  if (albedoScene)
    albedoScene->fixBlocks();
  if (radianceGrid)
    radianceGrid->updateTemporal();
  workAroundUAVslots();
}

void DaGIImpl::beforeRender(uint32_t sw, uint32_t sh, uint32_t maxW, uint32_t maxH, const TMatrix &viewItm, const TMatrix4 &projTm,
  float zn, float zf)
{
  maxSW = maxW;
  maxSH = maxH;
  ((DaGIFrameInfo &)view) = get_frame_info(viewItm, projTm, zn, zf);
  view.sw = sw;
  view.sh = sh;
  view.frustum = Frustum(view.globTm);
  currentSettings = nextSettings;
  fix(currentSettings);
  worldSdf->setValues(currentSettings.sdf.clips, currentSettings.sdf.texWidth,
    currentSettings.sdf.texWidth * currentSettings.sdf.yResScale, currentSettings.sdf.voxel0Size);
  const uint32_t worldSdfBufferSizeRequest = worldSdf->getRequiredTemporalBufferSize(currentSettings.sdf.texWidth,
    currentSettings.sdf.texWidth * currentSettings.sdf.yResScale, currentSettings.sdf.clips);
  worldSdf->setTemporalSpeed(currentSettings.sdf.temporalSpeed);

  int sdfW, sdfH;
  worldSdf->getResolution(sdfW, sdfH);
  if (!mediaScene && currentSettings.mediaScene.clips)
    mediaScene.reset(new DaGIMediaScene);
  else if (mediaScene && !currentSettings.mediaScene.clips)
    mediaScene.reset();

  if (mediaScene)
  {
    uint16_t w = currentSettings.mediaScene.w, d = currentSettings.mediaScene.d, clips = currentSettings.mediaScene.clips;
    d = d ? d : w * sdfH / sdfW;
    float voxel0 = currentSettings.mediaScene.voxel0Size;

    if (voxel0 <= 0.f)
    {
      const float sdfV = worldSdf->getVoxelSize(worldSdf->getClipsCount() - 1);
      voxel0 = max((sdfW * sdfV) / w, (sdfH * sdfV) / d) / (1 << (clips - 1));
    }

    mediaScene->init(w, d, clips, voxel0);
    mediaScene->setTemporalSpeedFromGbuf(currentSettings.mediaScene.temporalSpeed);
  }

  if (voxelScene && currentSettings.voxelScene.sdfResScale <= 0)
    voxelScene.reset();
  else if (!voxelScene && currentSettings.voxelScene.sdfResScale > 0)
  {
    voxelScene = eastl::make_unique<DaGIVoxelScene>();
    worldSdf->fullReset(false);
  }

  if (voxelScene)
  {
    voxelScene->init(sdfW, sdfH, currentSettings.voxelScene.sdfResScale, currentSettings.voxelScene.firstSDFClip,
      currentSettings.voxelScene.clips, currentSettings.voxelScene.anisotropy ? DaGIVoxelScene::Aniso::On : DaGIVoxelScene::Aniso::Off,
      currentSettings.voxelScene.onlyLuma ? DaGIVoxelScene::Radiance::Luma : DaGIVoxelScene::Radiance::Colored);
    voxelScene->setTemporalSpeedFromGbuf(currentSettings.voxelScene.temporalSpeed);
    voxelScene->setTemporalFromGbufStable(gi_update_from_gbuf_stable); // fixme:should be setting
  }
  if (currentSettings.skyVisibility.clipW && !skyVisibility)
    skyVisibility = eastl::make_unique<SkyVisibility>();
  else if (!currentSettings.skyVisibility.clipW && skyVisibility)
    skyVisibility.reset();

  if ((currentSettings.radianceGrid.w != 0) && !radianceGrid)
    radianceGrid.reset(new RadianceGrid);
  else if ((currentSettings.radianceGrid.w == 0) && radianceGrid)
    radianceGrid.reset();

  if (radianceGrid)
  {
    const uint16_t radGridD =
      currentSettings.radianceGrid.d ? currentSettings.radianceGrid.d : (currentSettings.radianceGrid.w * sdfH) / sdfW;

    float radGridProbe0Size = calc_probe0_size(currentSettings.radianceGrid.w, radGridD, currentSettings.radianceGrid.clips, sdfW,
      sdfH, worldSdf->getClipsCount(), worldSdf->getVoxelSize(0));

    radianceGrid->init(currentSettings.radianceGrid.w, radGridD, currentSettings.radianceGrid.clips, radGridProbe0Size,
      currentSettings.radianceGrid.additionalIrradianceClips, currentSettings.radianceGrid.irradianceProbeDetail,
      currentSettings.radianceGrid.framesToUpdateOneClip);
  }

  {
    uint16_t clipW = 0, clipD = 0;

    if (currentSettings.albedoScene.clips)
    {
      if (!albedoScene)
        albedoScene = eastl::make_unique<DaGIAlbedoScene>();
      const float blockSize =
        DaGIAlbedoScene::clip_block_size(currentSettings.albedoScene.voxel0Size, currentSettings.albedoScene.clips - 1);
      const float movingAroundSize = blockSize * (1 + DaGIAlbedoScene::moving_threshold_blocks());
      const float sdfMaxWidth =
        (sdfW + worldSdf->getTexelMoveThresholdXZ() + 1) * worldSdf->getVoxelSize(worldSdf->getClipsCount() - 1);
      const float sdfMaxAlt =
        (sdfH + worldSdf->getTexelMoveThresholdAlt() + 1) * worldSdf->getVoxelSize(worldSdf->getClipsCount() - 1);
      // debug("blockSize %f sdfMaxWidth = %f sdfMaxAlt = %f movingAroundSize = %f", blockSize, sdfMaxWidth, sdfMaxAlt,
      // movingAroundSize);
      clipW = ceilf((sdfMaxWidth + movingAroundSize) / blockSize);
      clipD = ceilf((sdfMaxAlt + movingAroundSize) / blockSize);
      if (currentSettings.albedoScene.yResScale > 0)
        clipD = max<uint16_t>(clipD, clipW * currentSettings.albedoScene.yResScale);
    }
    else
      albedoScene.reset();

    if (albedoScene)
    {
      albedoScene->init(clipW, clipD, currentSettings.albedoScene.clips, currentSettings.albedoScene.voxel0Size,
        currentSettings.albedoScene.allocation);
      albedoScene->setTemporalSpeedFromGbuf(currentSettings.albedoScene.temporalSpeed);
      albedoScene->setTemporalFromGbufStable(gi_update_from_gbuf_stable); // fixme:should be setting
    }
  }
  if (currentSettings.screenProbes.tileSize && !screenProbes)
    screenProbes.reset(new ScreenSpaceProbes);
  else if (!currentSettings.screenProbes.tileSize && screenProbes)
    screenProbes.reset();

  if (screenProbes)
  {
    screenProbes->init(currentSettings.screenProbes.tileSize, view.sw, view.sh, maxSW, maxSH,
      currentSettings.screenProbes.radianceOctRes, currentSettings.screenProbes.angleFiltering,
      currentSettings.screenProbes.temporality);
  }

  // todo: this actually depends on clipmap resolution count and is calculable
#if RADIANCE_CACHE
  if (radianceCache)
  {
    radianceCache->initAtlas(currentSettings.radianceCache.probesMin,
      currentSettings.radianceCache.probesMin * currentSettings.radianceCache.probesMax);

    radianceCache->setTemporalParams(currentSettings.radianceCache.updateInFrames, currentSettings.radianceCache.probeFramesKeep,
      currentSettings.radianceCache.updateOldProbeFreq);
    if (currentSettings.radianceCache.allowDebugAllocation != radianceCache->hasReadBack())
    {
      if (currentSettings.radianceCache.allowDebugAllocation)
        radianceCache->initReadBack();
      else
        radianceCache->closeReadBack();
    }

    radianceCache->initClipmap(currentSettings.radianceCache.xzRes,
      currentSettings.radianceCache.xzRes * currentSettings.radianceCache.yResScale, currentSettings.radianceCache.clips,
      currentSettings.radianceCache.probe0Size);
  }
#endif
  if (currentSettings.skyVisibility.clipW && skyVisibility)
  {
    uint8_t clipD = currentSettings.skyVisibility.clipW * currentSettings.skyVisibility.yResScale;
    if (clipD == 0)
    {
      clipD = (currentSettings.skyVisibility.clipW * sdfH) / sdfW;
    }
    float skyVisProbeSize = calc_probe0_size(currentSettings.skyVisibility.clipW, clipD, currentSettings.skyVisibility.clips, sdfW,
      sdfH, worldSdf->getClipsCount(), worldSdf->getVoxelSize(0));

    skyVisibility->init(currentSettings.skyVisibility.clipW, clipD, currentSettings.skyVisibility.clips, skyVisProbeSize,
      currentSettings.skyVisibility.framesToUpdateTemporally, !bool(radianceGrid));
    skyVisibility->setHasSimulatedBounceCascade(currentSettings.skyVisibility.simulateLightBounceClips);
    float halfSdfSize = 0.5 * worldSdf->getVoxelSize(worldSdf->getClipsCount() - 1) * sdfW;
    const float traceDist = currentSettings.skyVisibility.traceDist > 0 ? currentSettings.skyVisibility.traceDist : halfSdfSize / 3.f;
    skyVisibility->setTracingDistances(traceDist, currentSettings.skyVisibility.upscaleInProbes,
      currentSettings.skyVisibility.airScale);
  }

  const uint32_t skyVisibilityBufferSizeRequest = skyVisibility ? skyVisibility->getRequiredTemporalBufferSize() : 0;
  const uint32_t requestedCommonBufferSize = eastl::max(skyVisibilityBufferSizeRequest, worldSdfBufferSizeRequest);

  if (requestedCommonBufferSize > 0 && (!commonTempBuffer || commonTempBuffer->getNumElements() < requestedCommonBufferSize))
  {
    commonTempBuffer.close();
    commonTempBuffer = dag::create_sbuffer(sizeof(uint32_t), requestedCommonBufferSize,
      SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW | SBCF_BIND_SHADER_RES, 0, "dagi_common_temp_buffer");
  }

  if (skyVisibility)
    skyVisibility->setTemporalBuffer(commonTempBuffer);
  worldSdf->setTemporalBuffer(commonTempBuffer);

  if (!radianceGrid && !skyVisibility)
    currentSettings.volumetricGI.tileSize = 0;

  if (currentSettings.volumetricGI.tileSize && !volumetricGI)
    volumetricGI.reset(new VolumetricGI);
  else if (!currentSettings.volumetricGI.tileSize && volumetricGI)
    volumetricGI.reset();

  if (volumetricGI)
  {
    const float probe0 = radianceGrid ? radianceGrid->get_irradiance_probe_size(0) : 0.0001f;
    const int clipW = radianceGrid ? radianceGrid->get_irradiance_clip_w() : 1024;
    VolumetricGI::ZDistributionParams zParams;
    zParams.zDist = currentSettings.volumetricGI.zDist <= 0
                      ? 0.5f * (radianceGrid ? radianceGrid->horizontalSize() : skyVisibility->horizontalSize())
                      : currentSettings.volumetricGI.zDist;
    zParams.zLogMul = currentSettings.volumetricGI.zLogMul;
    zParams.slices = currentSettings.volumetricGI.slices;
    // todo: provide min dist in settings
    volumetricGI->init(currentSettings.volumetricGI.tileSize, view.sw, view.sh, maxSW, maxSH, currentSettings.volumetricGI.radianceRes,
      currentSettings.volumetricGI.spatialFilters, probe0, clipW, bool(radianceGrid), zParams);
    volumetricGI->setHistoryBlurTexelOfs(currentSettings.volumetricGI.historyBlurTexelOfs);
  }
  G_ASSERT(!screenProbes || radianceGrid);
  G_ASSERT(bool(skyVisibility) != bool(radianceGrid));
  G_ASSERT(skyVisibility || !currentSettings.voxelScene.onlyLuma);
}

void DaGIImpl::beforeFrameLit(float dynamic_quality)
{
  DA_PROFILE_GPU;

  set_view_vars(view);
#if RADIANCE_CACHE
  if (radianceCache)
  {
    if (gi_radiance_cache_update_position)
      radianceCache->updateClipMapPosition(view.viewItm.getcol(3), currentSettings.radianceCache.updateAllClips);
    if (gi_radiance_cache_update_probes)
      radianceCache->start_marking();
  }
#endif

  if (screenProbes && gi_screen_probes.get())
  {
    screenProbes->calcProbesPlacement(view.world_view_pos, view.viewItm, view.projTm, view.znear, view.zfar);
  }

#if RADIANCE_CACHE
  if (radianceCache)
  {
    if (gi_radiance_cache_update_probes)
    {
      radianceCache->mark_new_screen_probes(screenW, screenH);
      // fixme: provide that info externally.
      uint32_t freeAmount = ~0u;
      if (radianceCache->getLatestFreeAmount(freeAmount))
      {
        if (!freeAmount)
          logerr("exhausted radiance cache! increase it", freeAmount);
      }
      static uint32_t frame;
      if (((++frame) & 1023) == 0)
        debug("radiance cache free %d", freeAmount);
    }
    if (gi_radiance_cache_update_probes)
      radianceCache->finish_marking();

    if (gi_radiance_cache_update_probes)
      radianceCache->calcAllProbes();
  }
#endif
  if (screenProbes && gi_screen_probes.get())
  {
    screenProbes->relightProbes(dynamic_quality, dynamic_quality > 0.5);
  }
  if (volumetricGI)
    volumetricGI->calc(view.viewItm, view.projTm, view.znear, view.zfar, dynamic_quality);
  workAroundUAVslots();
}

bool is_only_low_gi_supported()
{
  constexpr uint32_t FORMARS_TO_CHECK[] = {TEXFMT_L8, TEXFMT_A16B16G16R16F, TEXFMT_R11G11B10F, TEXFMT_R16F};
  for (int i = 0; i < countof(FORMARS_TO_CHECK); i++)
  {
    if (!(d3d::get_texformat_usage(FORMARS_TO_CHECK[i]) & d3d::USAGE_UNORDERED_LOAD))
      return true;
  }
  return false;
}