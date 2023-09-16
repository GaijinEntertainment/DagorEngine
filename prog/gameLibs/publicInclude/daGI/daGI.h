//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_resPtr.h>
#include <util/dag_baseDef.h>
#include <generic/dag_carray.h>
#include <generic/dag_staticTab.h>
#include <EASTL/unique_ptr.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <shaders/dag_computeShaders.h>
// #include <render/toroidalHelper.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IPoint3.h>
#include <math/dag_Point3.h>
#include <math/dag_bounds3.h>
#include <math/dag_hlsl_floatx.h>
#include "dagi_volmap_cb.hlsli"
#include <daGI25D/scene.h>
#include <daGI25D/irradiance.h>
#include <textureUtil/textureUtil.h>
#include <vecmath/dag_vecMath.h>

#define GI3D_VERBOSE_DEBUG 0

class Sbuffer;
class ComputeShaderElement;
class BBox3;
struct RaytraceTopAccelerationStructure;

namespace scene
{
class TiledScene;
};
namespace dagi
{

class GIWindows;
class GIWalls;

typedef eastl::function<void(const BBox3 &box, const Point3 &voxel_size)> render_scene_fun_cb;
class GI3D
{
public:
  enum QualitySettings
  {
    ONLY_AO = 0,
    COLORED = 1,
    RAYTRACING = 2
  };
  void setQuality(QualitySettings set);
  QualitySettings getQuality() const { return common.quality; }
  enum BouncingMode
  {
    ONE_BOUNCE,
    TILL_CONVERGED,
    CONTINUOUS
  };
  void setBouncingMode(BouncingMode mode);
  BouncingMode getBouncingMode() const { return bouncingMode; }
  void setQuality(QualitySettings qs, float max_height_above_floor, float xz_25d_distance, float xz_3d_distance, float cascade0_scale,
    int xz_3d_res, int y_3d_res);
  // to be removed
  void init(int xz, int y, float scene25d_xz_size = 2.0f, float scene25d_scene_y_size = 2.0f);
  void updateVolmapResolution(float xz_voxel_size, float y_voxel_size, float xz_voxel_size2, float y_voxel_size2);
  //--
  void afterReset();
  void render(const TMatrix &view_tm, const TMatrix4 &proj_tm);
  void markVoxelsFromRT(float speed = 0.5);
  float getSceneDist3D() const;  // 3d quality dist of lit area provided in that dist
  float getLightDist3D() const;  // 3d quality dist of lit area provided in that dist
  float getLightMaxDist() const; // 2.5d quality dist of lit area provided in that dist
  void updateOrigin(const Point3 &center, const dagi25d::voxelize_scene_fun_cb &voxelize_25d_cb,
    const render_scene_fun_cb &preclear_cb = {}, const render_scene_fun_cb &voxelize_cb = {},
    int max_scenes_to_update = 1); // will schedule dispatch
  void updateVoxelSceneBox(const BBox3 &sceneBox);
  void updateVoxelMinSize(float voxel_resolution);
  void updateVoxelSceneBox(const BBox3 &sceneBox, float voxel_resolution) // to be removed
  {
    updateVoxelSceneBox(sceneBox);
    updateVoxelMinSize(voxel_resolution);
  }
  void setMaxVoxelsPerBin(float intersected, float non_intersected, float initial); // between 0 and 1
  void setVars();
  void setSceneVoxelVars();
  void invalidateEnvi(bool force);
  void invalidateDeferred();
  void invalidate(const BBox3 &model_bbox, const TMatrix &tm, const BBox3 &approx);
  void initDebug();
  enum DebugVolmapType
  {
    VISIBLE = 1,
    INTERSECTED,
    NON_INTERSECTED,
    SELECTED_INTERSECTED,
    SELECTED_NON_INTERSECTED,
    SELECTED_INIT
  }; //
  enum DebugAllVolmapType
  {
    AMBIENT = 0,
    INTERSECTION = 1,
    AGE = 2,
    OCTAHEDRAL = 4
  };
  enum DebugSceneType
  {
    VIS_RAYCAST,
    EXACT_VOXELS,
    LIT_VOXELS
  }; //, TEMPORAL_AGE
  // Sbuffer *getVolmapCB() const {return common.volmapCB.getBuf();}
  const ManagedBuf &getSceneVoxels25d() const { return scene25D.getSceneVoxels(); };
  IPoint2 getSceneVoxels25dResolution() const { return IPoint2(scene25D.getXZResolution(), scene25D.getYResolution()); };
  const ManagedTex &getSceneVoxelsAlpha() const { return sceneVoxelsAlpha; };
  const ManagedTex &getSceneVoxels() const { return sceneVoxelsColor; };

  void initWalls(eastl::unique_ptr<scene::TiledScene> &&walls);
  void initWindows(eastl::unique_ptr<scene::TiledScene> &&windows);

  void drawDebugAllProbes(int cascade, DebugAllVolmapType debug_all_volmap = AMBIENT);
  void drawDebug(int cascade, DebugVolmapType debug_volmap);
  void debugSceneVoxelsRayCast(DebugSceneType debug_scene, int cascade = 0);
  void debugScene25DVoxelsRayCast(dagi25d::SceneDebugType debug_scene);
  void debugVolmap25D(dagi25d::IrradianceDebugType type);
  int getSceneCascadesCount() const { return sceneCascades.size(); }
  IPoint3 getSceneRes(unsigned cascade_id) const;
  GI3D();
  ~GI3D();

  eastl::unique_ptr<scene::TiledScene> &getWallsScene() const;
  eastl::unique_ptr<scene::TiledScene> &getWindowsScene() const;
  void setSceneTop(RaytraceTopAccelerationStructure *scene_top) { common.rt.sceneTop = scene_top; }

  typedef bbox3f (*get_restricted_update_bbox_cb_type)(const bbox3f &intersect_bbox, const Frustum &instesect_frustum,
    bool only_trees);

  void setRestrictedUpdateBBoxCB(get_restricted_update_bbox_cb_type cb) { get_restricted_update_bbox_cb = cb; }

  // will (usually) return true and set rt + override with multi-sampled target
  // it can be NULL if only DeviceDriverCapabilities::hasUAVOnlyForcedSampleCount is supported
  // it can be backbuffer, if backbuffer is not multisampled and DeviceDriverCapabilities::hasForcedSamplerCount is supported
  // it can be special msaa target, if DeviceDriverCapabilities::hasForcedSamplerCount is not suppoorted
  // it can be special non msaa target/backbuf with super sampling, if no any msaa is not supported at all.
  // the last one is worst option, performance wise and memory wise
  bool setSubsampledTargetAndOverride(int w, int h); // sets RT, view, and override
  void resetOverride();
  float getCascade0Dist() const { return cascade0Dist; }

  bool hasPhysObjInCascade() { return lastHasPhysObjInCascade; }
  bbox3f getRestrictedUpdateGiBox() { return lastRestrictedUpdateGiBox; }

private:
  BouncingMode bouncingMode;
  GI3D &operator=(GI3D &) = delete;
  GI3D &operator=(GI3D &&) = delete;
  GI3D(GI3D &) = delete;
  GI3D(GI3D &&) = delete;

  void initSceneVoxels();
  void closeSceneVoxels();
  void initSceneVoxelsCommon();
  void calcEnviCube();
  void toroidalUpdateVolmap(const Point3 &origin);

  struct VolmapCommonData
  {
    QualitySettings quality = COLORED;
    eastl::unique_ptr<ComputeShaderElement> cull_ambient_voxels_cs, cull_ambient_voxels_cs_warp_64, light_ss_ambient_voxels_cs,
      random_point_occlusion_ambient_voxels_cs, temporal_ambient_voxels_cs;
    eastl::unique_ptr<ComputeShaderElement> light_ss_combine_ambient_voxels_cs, light_initial_ambient_voxels_cs,
      calc_initial_ambient_walls_dist_cs;
    eastl::unique_ptr<ComputeShaderElement> light_initialize_ambient_voxels_cs, light_initialize_clear_indirect_cs,
      light_initialize_culling_cs, light_initialize_culling_check_age_cs, light_partial_initialize_ambient_voxels_cs;

    eastl::unique_ptr<ComputeShaderElement> create_point_occlusion_dispatch_cs;
    eastl::unique_ptr<ComputeShaderElement> ssgi_change_probability_cs;
    eastl::unique_ptr<ComputeShaderElement> ssgi_calc_envi_ambient_cube_cs, ssgi_average_ambient_cube_cs;
    eastl::unique_ptr<ComputeShaderElement> ssgi_clear_volmap_select_cs;

    eastl::unique_ptr<ComputeShaderElement> ssgi_clear_volmap_cs, ssgi_copy_from_volmap_cs, ssgi_copy_to_volmap_cs;
    eastl::unique_ptr<ComputeShaderElement> move_y_from_cs, move_y_to_cs;

    UniqueBufHolder frustumVisibleAmbientVoxels, frustumVisiblePointVoxels;
    BufPtr selectedAmbientVoxels, selectedAmbientVoxelsPlanes, traceRayResults;
    BufPtr frustumVisibleAmbientVoxelsCount, visibleAmbientVoxelsIndirect;
    UniqueBufHolder poissonBuf;
    UniqueBufHolder enviCube, enviCubes; // for two stage reduction
    eastl::shared_ptr<texture_util::ShaderHelper> shaderHelperUtils;
    UniqueBufHolder volmapCB;
    UniqueTexHolder cube;
    UniqueTexHolder ssgiTemporalWeight;

    void init(int xz_dimensions, int y_dimensions, int maxVoxels, bool scalar_ao);
    void initCommon();
    void calcEnviCube();
    void initEnviCube();
    int maxVoxels = 0;
    int warpSize = 32;
    bool enviCubeValid = false;
    void invalidateTextureContent();
    void afterReset();
    struct RT
    {
      eastl::unique_ptr<ComputeShaderElement> octahedral_distances_cs;
      eastl::unique_ptr<ComputeShaderElement> move_y_octahderal_distances_cs, move_y_dead_probes_cs;
      UniqueTexHolder octahedralDistances, deadProbes;
      RaytraceTopAccelerationStructure *sceneTop = nullptr;
      void createTextures(QualitySettings quality, int xzDim, int yDim);
      void initShaders(QualitySettings quality);
      void createOctahedralDistances(int xz_dim, int y_dim);
    } rt;
  } common;

  BufPtr debugVolmapDrawIndirect;
  DynamicShaderHelper drawDebugVolmap, drawDebugAllVolmap, debug_rasterize_voxels;
  eastl::unique_ptr<ComputeShaderElement> create_debug_render_cs;
  mat44f debugLastGTM;
  // ToroidalHelper toroidalVolmapXZ;
  struct VolmapCascade
  {
    void init(const char *name, const Point2 &sz, int xz, int y, int y_from, int y_total, int cascade_id);
    void setVars() const;
    bool setResolution(const Point2 &sz);
    bool toroidalUpdateVolmap(const Point3 &origin, VolmapCommonData &data, int last_cascade_id, const BBox3 &sceneBox);
    bool updateOrigin(const Point3 &center, VolmapCommonData &data, int last_cascade_id, const BBox3 &sceneBox);
    void render(VolmapCommonData &data, mat44f_cref globtm);

    const VolmapCB &getParams() const { return params; }
    bool isResetted() const { return resetted; }
    BBox3 getBox() const;

  private:
    void cull(VolmapCommonData &common, mat44f_cref globtm);
    bool setMaxVoxelsPerBin(uint32_t intersected, uint32_t non_intersected, uint32_t initial);
    friend class GI3D;
    VolmapCB params = VolmapCB();
    String name;
    IPoint3 toroidalOrigin = {0, -1000, 0};
    int toroidalOfsY = 0;
    BufPtr voxelsWeightHistogram, voxelChoiceProbability[2];
    int currentProbability = 0;
    int getDimXZ() const { return params.xz_dim; } // it is important to have it int, so it can be used in modulo
    int getDimY() const { return params.y_dim; }   // it is important to have it int, so it can be used in modulo
    float getVoxelSizeXZ() const { return params.voxelSize.x; }
    float getVoxelSizeY() const { return params.voxelSize.y; }
    Point3 getResolution() const { return Point3(getVoxelSizeXZ(), getVoxelSizeY(), getVoxelSizeXZ()); }
    void fillBoxParams();
    void copyFromNext(VolmapCommonData &data);
    bool moveY(VolmapCommonData &common, int move_y);
    bool resetted = true;
    int cascadeId = -1;
  };
  int xz_dimensions = 0, y_dimensions = 0;
  int sceneXZResolution() const;
  int sceneYResolution() const;
  int getCoordMoveThreshold() const;
  float scene25dXZSize = 2.0, scene25dYSize = 2.0;
  float sceneDistanceXZ(uint32_t cascade) const;
  float sceneDistanceY(uint32_t cascade) const;
  float sceneDistanceMoveThreshold(uint32_t cascade) const;
  float sceneDistanceXZ() const;
  float sceneDistanceY() const;
  float sceneDistanceMoveThreshold() const;
  BBox3 getSceneBox(uint32_t cascade) const;

  get_restricted_update_bbox_cb_type get_restricted_update_bbox_cb = 0;

  static constexpr int CASCADES_3D = 2;
  carray<VolmapCascade, CASCADES_3D> volmap;
  carray<Point2, CASCADES_3D> cascadeResolution;
  void updateVolmapCB();

  // 2d scene
  dagi25d::Scene scene25D;
  dagi25d::Irradiance irradiance25D;
  // scene
  void invalidateGiCascades();
  void afterResetScene();
  UniqueTexHolder sceneVoxelsColor, sceneVoxelsAlpha; // to consider using only one (4 bytes)
  eastl::unique_ptr<ComputeShaderElement> ssgi_mark_scene_voxels_from_gbuf_cs, ssgi_temporal_scene_voxels_cs, ssgi_clear_scene_cs;
  eastl::unique_ptr<ComputeShaderElement> ssgi_temporal_scene_voxels_create_cs, ssgi_invalidate_voxels_cs;
  BufPtr selectedGbufVoxels, selectedGbufVoxelsCount;
  bool invalidateCount = true;
  Point3 sceneVoxelSize = {1, 1, 1};
  struct SceneCascade
  {
    Point3 origin = {0, 0, 0};
    IPoint3 toroidalOrigin = {0, -10000, 10000};
    float scale = 1;
    IPoint3 lastRes = {0, 0, 0}, lastStInvalid = {0, 0, 0};
    enum STATUS
    {
      NOT_MOVED,
      MOVED,
      TELEPORTED
    } lastStatus = NOT_MOVED;
    void invalidate()
    {
      toroidalOrigin += IPoint3{10000, -10000, 10000};
      lastStatus = NOT_MOVED;
    }
  };
  StaticTab<SceneCascade, 4> sceneCascades;
  void invalidateVoxelScene();
  BBox3 sceneVoxelBox;

  TexPtr sampledTarget;
  shaders::UniqueOverrideStateId voxelizeOverride;
  bool ensureSampledTarget(int w, int h, uint32_t fmt);
  bool isSceneTeleported(const Point3 &center) const;
  bool isSceneTeleported(int scene, const Point3 &center) const;
  IPoint3 getTexelOrigin(int scene, const Point3 &baseOrigin) const;

  SceneCascade::STATUS updateOriginScene(int scene, const Point3 &center, const render_scene_fun_cb &preclear_cb,
    const render_scene_fun_cb &voxelize_cb, bool repeat_last);
  bool updateOriginScene(const Point3 &center, const render_scene_fun_cb &preclear_cb, const render_scene_fun_cb &voxelize_cb,
    int max_scenes, bool repeat_last);


  void updateWallsPos(const Point3 &pos);
  void invalidateWalls();

  void updateWindowsPos(const Point3 &pos);
  void invalidateWindows();
  struct GIWindowsDestroyer
  {
    void operator()(GIWindows *ptr);
  };
  eastl::unique_ptr<GIWindows, GIWindowsDestroyer> cWindows;

  struct GIWallsDestroyer
  {
    void operator()(GIWalls *ptr);
  };
  eastl::unique_ptr<GIWalls, GIWallsDestroyer> cWalls;

  void fillRestrictedUpdateBox(TMatrix4 globtm);
  bool lastHasPhysObjInCascade = false;
  bbox3f lastRestrictedUpdateGiBox;

  void DebugInlineRt(const TMatrix &view_tm, const TMatrix4 &proj_tm);
  eastl::unique_ptr<ComputeShaderElement> debug_inline_rt_cs;
  UniqueBufHolder debugInlineRtConstants;
  UniqueTexHolder debugInlineRtTarget;

  Tab<bbox3f> invalidBoxes;
  UniqueBufHolder invalidBoxesSB;
  static constexpr int MAX_BOXES_INV_PER_FRAME = 4;
  float cascade0Dist = -1.f;
};

} // namespace dagi
