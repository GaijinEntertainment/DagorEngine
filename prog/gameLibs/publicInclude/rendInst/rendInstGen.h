//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <rendInst/rendInstExHandle.h>
#include <rendInst/rendInstDesc.h>
#include <rendInst/rendInstHasRIClipmap.h>
#include <rendInst/layerFlags.h>
#include <rendInst/renderPass.h>
#include <rendInst/rendInstCollision.h>
#include <rendInst/constants.h>

#include <shaders/dag_rendInstRes.h>
#include <generic/dag_span.h>
#include <generic/dag_carray.h>
#include <generic/dag_tab.h>
#include <generic/dag_relocatableFixedVector.h>
#include <memory/dag_framemem.h>
#include <util/dag_simpleString.h>
#include <scene/dag_physMatIdDecl.h>
#include <render/gpuVisibilityTest.h>
#include <EASTL/fixed_function.h>
#include <EASTL/string_view.h>
#include <EASTL/hash_map.h>
#include <3d/dag_texStreamingContext.h>
#include <util/dag_bitFlagsMask.h>


class Bitarray;
class CollisionResource;
typedef eastl::fixed_function<64, bool(int)> CollisionNodeFilter;
class RenderableInstanceLodsResource;
struct Capsule;
class TMatrix;
class Point4;
class Point3;
class BBox3;
class BSphere3;
class DataBlock;
struct RiGenVisibility;
struct Frustum;
class DynamicPhysObjectData;
struct Trace;
struct TraceMeshFaces;
class Occlusion;
class OcclusionMap;
class AcesEffect;
struct RendInstGenData;

#define MIN_SETTINGS_RENDINST_DIST_MUL  0.5f
#define MAX_SETTINGS_RENDINST_DIST_MUL  3.0f
#define MIN_EFFECTIVE_RENDINST_DIST_MUL 0.1f
#define MAX_EFFECTIVE_RENDINST_DIST_MUL 3.0f

// use additional data as hashVal only when in Tools (De3X, AV2)
#define RIGEN_PERINST_ADD_DATA_FOR_TOOLS (_TARGET_PC && !_TARGET_STATIC_LIB)
#define RI_COLLISION_RES_SUFFIX          "_collision"

namespace rendinst
{

struct OpaqueGlobalDynVarsPolicy;
template <typename, class>
class RiExtraRendererT;
using RiExtraRenderer = RiExtraRendererT<EASTLAllocatorType, OpaqueGlobalDynVarsPolicy>;

using VisibilityExternalFilter = eastl::fixed_function<sizeof(vec4f), bool(vec4f bbmin, vec4f bbmax)>;


struct DestroyedCellData;
inline uint32_t mat44_to_ri_type(mat44f_cref node)
{
  return (((uint32_t *)(char *)&node.col2)[3]) & 0xFFFF;
} // this is copy&paste from scene

typedef void (*ri_damage_effect_cb)(int type, const TMatrix &emitter_tm, const TMatrix &fx_tm, int pool_idx, bool is_player,
  AcesEffect **locked_fx, const char *effect_template);
typedef void *(*ri_register_collision_cb)(const CollisionResource *collRes, const char *debug_name);
typedef void (*ri_unregister_collision_cb)(void *&handle);
typedef void (*invalidate_handle_cb)(riex_handle_t invalidate_handle);
typedef void (*ri_destruction_cb)(riex_handle_t handle, bool is_dynamic, int32_t user_data);
typedef eastl::fixed_function<2 * sizeof(void *), void(const char *)> res_walk_cb;

extern void (*shadow_invalidate_cb)(const BBox3 &box);
extern BBox3 (*get_shadows_bbox_cb)();

struct RendInstBufferData
{
  mat44f tm;
  carray<int16_t, 12> data;
};

struct DestroyedRi
{
  vec3f axis, normal;
  unsigned frameAdded;
  float timeToDamage;
  int fxType;
  riex_handle_t riHandle;
  bool shouldUpdate;
  float accumulatedPower;
  float rotationPower;

  RendInstBufferData savedData;

  DestroyedRi() : riHandle(RIEX_HANDLE_NULL) {}
  ~DestroyedRi();
};

struct TraceRayRendInstData
{
  rendinst::RendInstDesc riDesc;
  Point3 norm;
  int matId;
  unsigned collisionNodeId;

  union
  {
    int sortKey;
    float tIn;
  };
  float tOut;

  bool operator<(const TraceRayRendInstData &rhs) const { return sortKey < rhs.sortKey; }
};

struct ColorInfo
{
  const char *name;
  E3DCOLOR colorFrom;
  E3DCOLOR colorTo;
};

void riex_lock_write(const char *locker_nm = NULL);
void riex_unlock_write();

void riex_lock_read();
void riex_unlock_read();

struct ScopedRIExtraWriteLock
{
  ScopedRIExtraWriteLock(const char *n = "riEx::wr") { riex_lock_write(n); }
  ~ScopedRIExtraWriteLock() { riex_unlock_write(); }
};

struct ScopedRIExtraReadLock
{
  ScopedRIExtraReadLock() { riex_lock_read(); }
  ~ScopedRIExtraReadLock() { riex_unlock_read(); }
};

void register_land_gameres_factory();

inline constexpr unsigned HUID_LandClassGameRes = 0x03FB59C4u; // LandClass
const DataBlock &get_detail_data(void *land_cls_res);
int get_landclass_data_hash(void *land_cls_res, void *buf, int buf_sz);

void prepareRiExtraRefs(const DataBlock &riConf);

void configurateRIGen(const DataBlock &riSetup);

void initRIGen(bool need_render, int cell_pool_sz, float poi_radius, ri_register_collision_cb reg_coll_cb = NULL,
  ri_unregister_collision_cb unreg_coll_cb = NULL, int job_manager_id = -1, float sec_layer_poi_radius = 256.0f,
  bool simplified_render = false, bool should_init_gpu_objects = true);
void termRIGen();

void init_impostors();

void setPoiRadius(float poi_radius);

void setRIGenRenderMode(int mode);
int getRIGenRenderMode();

void clearRIGen();

int getRIGenMaterialId(const RendInstDesc &desc);
void setRiGenResMatId(uint32_t res_idx, int matId);
void setRiGenResHp(uint32_t res_idx, float hp);
void setRiGenResDestructionImpulse(uint32_t res_idx, float impulse);

void setLoadingState(bool is_loading);
bool isRiExtraLoaded();
struct RIGenLoadingAutoLock
{
  RIGenLoadingAutoLock() { setLoadingState(true); }
  ~RIGenLoadingAutoLock() { setLoadingState(false); }
};

bool loadRIGen(IGenLoad &crd, void (*add_resource_cb)(const char *resname), bool init_sec_ri_extra_here = false,
  Tab<char> *out_reserve_storage_pregen_ent_for_tools = NULL, const DataBlock *level_blk = nullptr);
void prepareRIGen(bool init_sec_ri_extra_here = false, const DataBlock *level_blk = nullptr);

using FxTypeByNameCallback = int (*)(const char *);
void initRiGenDebris(const DataBlock &ri_blk, FxTypeByNameCallback get_fx_type_by_name, bool init_sec_ri_extra_here = true);
void initRiGenDebrisSecondary(const DataBlock &ri_blk, FxTypeByNameCallback get_fx_type_by_name);
void updateRiDestrFxIds(FxTypeByNameCallback get_fx_type_by_name);

void precomputeRIGenCellsAndPregenerateRIExtra();

void setRiExtraTiledSceneWritingThread(int64_t tid);

const DataBlock *registerRIGenExtraConfig(const DataBlock *persist_props);
void addRIGenExtraSubst(const char *ri_res_name);

int addRIGenExtraResIdx(const char *ri_res_name, int ri_pool_ref, int ri_pool_ref_layer, AddRIFlags ri_flags);
int getRIGenExtraResIdx(const char *ri_res_name);
int cloneRIGenExtraResIdx(const char *source_res_name, const char *new_res_name);
void setRIGenExtraResDynamic(const char *ri_res_name);
bool isRIGenExtraDynamic(int res_idx);
void addRiGenExtraDebris(uint32_t res_idx, int layer_idx);
void reloadRIExtraResources(const char *ri_res_name);
RenderableInstanceLodsResource *getRIGenExtraRes(int res_idx);
CollisionResource *getRIGenExtraCollRes(int res_idx);
const bbox3f *getRIGenExtraCollBb(int res_idx);
dag::Vector<BBox3> getRIGenExtraInstancesWorldBboxesByGrid(int res_idx, Point2 grid_origin, Point2 grid_size, IPoint2 grid_cells);
bbox3f getRIGenExtraOverallInstancesWorldBbox(int res_idx);
const bbox3f &getMovedDebrisBbox();
void walkRIGenResourceNames(res_walk_cb cb);
const char *getRIGenExtraName(uint32_t res_idx);
void iterateRIExtraMap(eastl::fixed_function<sizeof(void *) * 3, void(int, const char *)> cb);
bool hasRiLayer(int res_idx, uint32_t layer);
int getRIGenExtraPreferrableLod(int res_idx, float squared_distance);
bool isRIExtraGenPosInst(uint32_t res_idx);
bool updateRiExtraReqLod(uint32_t res_idx, unsigned lod);

void applyTiledScenesUpdateForRIGenExtra(int max_quota_usec, int max_maintenance_quota_usec);

riex_handle_t addRIGenExtra43(int res_idx, bool movable, mat43f_cref tm, bool collision, int orig_cell, int orig_offset,
  int add_data_dwords = 0, const int32_t *add_data = NULL);
riex_handle_t addRIGenExtra44(int res_idx, bool movable, mat44f_cref tm, bool collision, int orig_cell, int orig_offset,
  int add_data_dwords = 0, const int32_t *add_data = NULL);
const mat43f &getRIGenExtra43(riex_handle_t id);
void getRIGenExtra44(riex_handle_t id, mat44f &out_tm);

void getRIExtraCollInfo(riex_handle_t id, CollisionResource **out_collision, BSphere3 *out_bsphere);
const CollisionResource *getDestroyedRIExtraCollInfo(riex_handle_t handle);
CollisionResource *getRIGenCollInfo(const rendinst::RendInstDesc &desc);

float getInitialHP(riex_handle_t id);
float getHP(riex_handle_t id);
bool isPaintFxOnHit(riex_handle_t id);
bool isKillsNearEffects(riex_handle_t id);
void moveToOriginalScene(riex_handle_t id);
// Returns true if lock succeed
bool moveRIGenExtra43(riex_handle_t id, mat43f_cref tm, bool moved, bool do_not_wait);
bool moveRIGenExtra44(riex_handle_t id, mat44f_cref tm, bool moved, bool do_not_wait);
void setRIGenExtraImmortal(uint32_t pool, bool value);
bool isRIGenExtraImmortal(uint32_t pool);
bool isRIGenExtraWalls(uint32_t pool);
float getRIGenExtraBsphRad(uint32_t pool);
Point3 getRIGenExtraBsphPos(uint32_t pool);
Point4 getRIGenExtraBSphereByTM(uint32_t pool, const TMatrix &tm);
int getRIGenExtraParentForDestroyedRiIdx(uint32_t pool);
bool isRIGenExtraDestroyedPhysResExist(uint32_t pool);
int getRIGenExtraDestroyedRiIdx(uint32_t pool);
vec4f getRIGenExtraBSphere(riex_handle_t id);
void setRiGenExtraHp(riex_handle_t id, float hp); // special values: 0 (default HP with regen), -1 (default HP no regen), -2
                                                  // (invincible)

typedef uint32_t riex_render_info_t;
void setRIGenExtraData(riex_render_info_t id, uint32_t start, const uint32_t *udata, uint32_t cnt);
void setCASRIGenExtraData(riex_render_info_t id, uint32_t start, const uint32_t *wasdata, const uint32_t *newdata,
  uint32_t cnt); // this would set new data only if it was equal to wasdata
void setRIGenExtraNodeShadowsVisibility(riex_render_info_t id, bool visible);
const mat44f &getRIGenExtraTiledSceneNode(rendinst::riex_render_info_t);

bool isRiGenExtraValid(riex_handle_t id);
bool delRIGenExtra(riex_handle_t id); // return true if ri exta was actually deleted and false otherwise
void delRIGenExtraFromCell(riex_handle_t id, int cell_Id, int offset);
void delRIGenExtraFromCell(const RendInstDesc &desc);
void registerRIGenExtraInvalidateHandleCb(invalidate_handle_cb cb);
bool unregisterRIGenExtraInvalidateHandleCb(invalidate_handle_cb cb);
bool removeRIGenExtraFromGrid(riex_handle_t id);

void registerRiExtraDestructionCb(ri_destruction_cb cb);
bool unregisterRiExtraDestructionCb(ri_destruction_cb cb);
void onRiExtraDestruction(riex_handle_t id, bool is_dynamic, int32_t user_data = -1);
bbox3f riex_get_lbb(int res_idx);
float ries_get_bsph_rad(int res_idx);
dag::ConstSpan<mat43f> riex_get_instance_matrices(int res_idx);
bool riex_is_instance_valid(const mat43f &ri_tm);

void setGameClockForRIGenExtraDamage(const float *session_time);
bool applyDamageRIGenExtra(riex_handle_t id, float dmg_pts, float *absorbed_dmg_impulse = NULL, bool ignore_damage_threshold = false);
bool applyDamageRIGenExtra(const rendinst::RendInstDesc &desc, float dmg_pts, float *absorbed_dmg_impulse = NULL,
  bool ignore_damage_threshold = false);
bool damageRIGenExtra(riex_handle_t &id, float dmg_pts, mat44f *out_destr_tm = nullptr, float *absorbed_dmg_impulse = nullptr,
  int *out_destroyed_riex_offset = nullptr, DestrOptionFlags destroy_flags = DestrOptionFlag::AddDestroyedRi);
void play_riextra_dmg_fx(rendinst::riex_handle_t id, const Point3 &pos, ri_damage_effect_cb effect_cb);


typedef dag::RelocatableFixedVector<riex_handle_t, 64, true, framemem_allocator> riex_collidable_t;

void gatherRIGenExtraCollidable(riex_collidable_t &out_handles, const BBox3 &box, bool read_lock);
void gatherRIGenExtraCollidable(riex_collidable_t &out_handles, const Point3 &p0, const Point3 &dir, float len, bool read_lock);
void gatherRIGenExtraCollidableMin(riex_collidable_t &out_handles, bbox3f_cref box, float min_bsph_rad);

// if res_idx == nullptr, would get all, otherwise only those with res_idx in sorted_res_idx array.
void gatherRIGenExtraRenderable(Tab<riex_render_info_t> &out_handles, const int *sorted_res_idx, int count, bbox3f_cref box, bool fast,
  SceneSelection s); // if fast, it will be checked using spheres only
void gatherRIGenExtraRenderable(Tab<mat44f> &out_handles, bbox3f_cref box, bool fast, SceneSelection s); // if fast, it will be checked
                                                                                                         // using spheres only
void gatherRIGenExtraRenderable(Tab<mat44f> &out_handles, const BBox3 &box, bool fast, SceneSelection s); // if fast, it will be
                                                                                                          // checked using spheres only
void gatherRIGenExtraToTestForShadows(eastl::vector<GpuVisibilityTestManager::TestObjectData> &out_bboxes, mat44f_cref globtm_cull,
  float static_shadow_texel_size, uint32_t usr_data);
void gatherRIGenExtraShadowInvisibleBboxes(eastl::vector<bbox3f> &out_bboxes, bbox3f_cref gather_bbox);
void invalidateRIGenExtraShadowsVisibility();
void invalidateRIGenExtraShadowsVisibilityBox(bbox3f_cref box);
struct pos_and_render_info_t
{
  Point3 pos;
  riex_render_info_t info;
};
typedef dag::RelocatableFixedVector<pos_and_render_info_t, 128, true /*overflow*/> riex_pos_and_ri_tab_t;
void gatherRIGenExtraRenderableNotCollidable(riex_pos_and_ri_tab_t &out, bbox3f_cref box, bool fast, SceneSelection s);
bool gatherRIGenExtraBboxes(const RiGenVisibility *main_visibility, mat44f_cref volume_box,
  eastl::function<void(mat44f_cref, const BBox3 &, const char *)> callback);
void collectPixelsHistogramRIGenExtra(const RiGenVisibility *main_visibility, vec4f camera_fov, float histogram_scale,
  eastl::vector<eastl::vector_map<eastl::string_view, int>> &histogram);
void validateFarLodsWithHeavyShaders(const RiGenVisibility *main_visibility, vec4f camera_fov, float histogram_scale);
uint32_t getRiGenExtraResCount();
HasRIClipmap hasRIClipmapPools();
int getRiGenExtraInstances(Tab<riex_handle_t> &out_handles, uint32_t res_idx);
int getRiGenExtraInstances(Tab<riex_handle_t> &out_handles, uint32_t res_idx, const bbox3f &box);

void updateRiExtraBBoxScalesForPrepasses(const DataBlock &blk);

void registerRIGenSweepAreas(dag::ConstSpan<TMatrix> box_itm_list);
int scheduleRIGenPrepare(dag::ConstSpan<Point3> poi);
bool isRIGenPrepareFinished();

void get_ri_color_infos(eastl::fixed_function<sizeof(void *) * 4, void(E3DCOLOR colorFrom, E3DCOLOR colorTo, const char *name)> cb);
bool update_rigen_color(const char *name, E3DCOLOR from, E3DCOLOR to);
extern void regenerateRIGen();

extern void enableSecLayer(bool en);
extern bool enableSecLayerRender(bool en);
extern bool enablePrimaryLayerRender(bool en);
extern bool enableRiExtraRender(bool en);
extern bool isSecLayerRenderEnabled();

extern void applyBurningDecalsToRi(const bbox3f &decal);

// render with current matrix, autocomputes visibility
extern void renderRIGen(RenderPass render_pass, const Point3 &view_pos, const TMatrix &view_itm, uint32_t layer_flags = LAYER_OPAQUE,
  bool for_vsm = false, TexStreamingContext texCtx = TexStreamingContext(0));
extern void renderRIGen(RenderPass render_pass, const RiGenVisibility *visibility, const TMatrix &view_itm, uint32_t layer_flags,
  OptimizeDepthPass depth_optimized = OptimizeDepthPass::No, uint32_t instance_count_mul = 1, AtestStage atest_stage = AtestStage::All,
  const RiExtraRenderer *riex_renderer = nullptr, TexStreamingContext texCtx = TexStreamingContext(0));
extern void renderGpuObjectsFromVisibility(RenderPass render_pass, const RiGenVisibility *visibility, uint32_t layer_flags);
extern void renderGpuObjectsLayer(RenderPass render_pass, const RiGenVisibility *visibility, uint32_t layer_flags, uint32_t layer);
extern void renderRIGenOptimizationDepth(RenderPass render_pass, const RiGenVisibility *visibility,
  IgnoreOptimizationLimits ignore_optimization_instances_limits = IgnoreOptimizationLimits::No, SkipTrees skip_trees = SkipTrees::No,
  RenderGpuObjects gpu_objects = RenderGpuObjects::No, uint32_t instance_count_mul = 1,
  TexStreamingContext texCtx = TexStreamingContext(0));
extern void renderRITreeDepth(const RiGenVisibility *visibility);
extern void ensureElemsRebuiltRIGenExtra(bool gpu_instancing);
extern void renderRIGenExtraFromBuffer(Sbuffer *buffer, dag::ConstSpan<IPoint2> offsets_and_count, dag::ConstSpan<uint16_t> ri_indices,
  dag::ConstSpan<uint32_t> lod_offsets, RenderPass render_pass, OptimizeDepthPass optimization_depth_pass,
  OptimizeDepthPrepass optimization_depth_prepass, IgnoreOptimizationLimits ignore_optimization_instances_limits, uint32_t layer,
  ShaderElement *shader_override = nullptr, uint32_t instance_multiply = 1, bool gpu_instancing = false,
  Sbuffer *indirect_buffer = nullptr, Sbuffer *ofs_buffer = nullptr);
extern void setRIGenExtraDiffuseTexture(uint16_t ri_idx, int tex_var_id);

extern bool hasRIExtraOnLayers(const RiGenVisibility *visibility, uint32_t layer_flags);

extern void beforeDraw(RenderPass render_pass, const RiGenVisibility *visibility, const Frustum &frustum, const Occlusion *occlusion,
  const char *mission_name = "", const char *map_name = "", bool gpu_instancing = false);
extern void update(const Point3 &origin);
extern void copyVisibileImpostorsData(const RiGenVisibility *visibility, bool clear_prev_result);
extern void updateRIGenImpostors(float shadowDistance, const Point3 &sunDir0, const TMatrix &view_itm);
extern void resetRiGenImpostors();
extern void getLodCounter(int lod, const RiGenVisibility *visibility, int &subCellNo, int &cellNo);
extern void set_per_instance_visibility_for_any_tree(bool on); // should be on only in tank

extern float getMaxFarplaneRIGen(bool sec_layer = false);
extern float getMaxAverageFarplaneRIGen(bool sec_layer = false);
extern bool renderRIGenClipmapShadowsToTextures(const Point3 &sunDir0, bool for_sli, bool force_update = true);
extern bool renderRIGenGlobalShadowsToTextures(const Point3 &sunDir0, bool force_update = true, bool use_compression = true);
extern void startUpdateRIGenGlobalShadows();
extern void startUpdateRIGenClipmapShadows();


// renderRIGenToShadowDepth renders with shadow_globtm, autocomputes visbility
// the same as prepareVisiblity(visibility), renderRIGen(RENDER_PASS_TO_SHADOW, visibility, LAYER_OPAQUE);
extern void renderRIGenToShadowDepth(const Point3 &vpos);
extern bool notRenderedClipmapShadowsBBox(BBox2 &box, int cascadeNo);
extern bool notRenderedStaticShadowsBBox(BBox3 &box);
extern void setClipmapShadowsRendered(int cascadeNo);
extern void renderRIGenShadowsToClipmap(const BBox2 &region, int renderNewForCascadeNo); //-1 - render all, not only new
extern RiGenVisibility *createRIGenVisibility(IMemAlloc *mem);
extern void setRIGenVisibilityDistMul(RiGenVisibility *visibility, float dist_mul);
extern void destroyRIGenVisibility(RiGenVisibility *visibility);
extern void setRIGenVisibilityMinLod(RiGenVisibility *visibility, int ri_lod, int ri_extra_lod);
extern void setRIGenVisibilityAtestSkip(RiGenVisibility *visibility, bool skip_atest, bool skip_noatest);

extern void setRIGenVisibilityRendering(RiGenVisibility *visibility, VisibilityRenderingFlags r);
extern void enableGpuObjectsForVisibility(RiGenVisibility *visibility);
extern void disableGpuObjectsForVisibility(RiGenVisibility *visibility);
extern void clearGpuObjectsFromVisibility(RiGenVisibility *visibility);

extern void renderDebug();
extern void prepareOcclusion(Occlusion *occlusion, OcclusionMap *occlusion_map);
struct RIOcclusionData;
extern void destroyOcclusionData(RIOcclusionData *&data);
extern RIOcclusionData *createOcclusionData();
extern void iterateOcclusionData(RIOcclusionData &od, const mat44f *&mat, const IPoint2 *&indices, uint32_t &cnt); // indices.x is
                                                                                                                   // index in mat
                                                                                                                   // array
extern void prepareOcclusionData(mat44f_cref globtm, vec4f vpos, RIOcclusionData &, int max_ri_to_add, bool walls);

// prepares visibility for specified frustum/position.
// if forShadow is true, only opaque part will be created, and only partially transparent cells are added to opaque
// returns false if nothing is visible
//  if for_visual_collision is true, only rendinst without collision will be rendered
extern bool prepareRIGenVisibility(const Frustum &frustum, const Point3 &viewPos, RiGenVisibility *, bool forShadow,
  Occlusion *occlusion, bool for_visual_collision = false, const VisibilityExternalFilter &external_filter = {});

extern void sortRIGenVisibility(RiGenVisibility *visibility, const Point3 &viewPos, const Point3 &viewDir, float vertivalFov,
  float horizontalFov, float areaThreshold);

template <bool use_external_filter = false>
bool prepareExtraVisibilityInternal(mat44f_cref gtm, const Point3 &viewPos, RiGenVisibility &v, bool forShadow, Occlusion *occlusion,
  RiExtraCullIntention cullIntention = RiExtraCullIntention::MAIN, bool for_visual_collision = false,
  bool filter_rendinst_clipmap = false, bool for_vsm = false, const VisibilityExternalFilter &external_filter = {});

extern bool prepareRIGenExtraVisibility(mat44f_cref gtm, const Point3 &viewPos, RiGenVisibility &v, bool forShadow,
  Occlusion *occlusion, RiExtraCullIntention cullIntention = RiExtraCullIntention::MAIN, bool for_visual_collision = false,
  bool filter_rendinst_clipmap = false, bool for_vsm = false, const VisibilityExternalFilter &external_filter = {});
extern bool prepareRIGenExtraVisibilityBox(bbox3f_cref box_cull, int forced_lod, float min_size, float min_dist,
  RiGenVisibility &vbase, bbox3f *result_box = nullptr);

void prepareToStartAsyncRIGenExtraOpaqueRender(RiGenVisibility &vis, int frame_stblk, TexStreamingContext texCtx, bool enable = true);
void startPreparedAsyncRIGenExtraOpaqueRender(RiGenVisibility &vis, bool wake = true);
void waitAsyncRIGenExtraOpaqueRenderVbFill(const RiGenVisibility *vis);
RiExtraRenderer *waitAsyncRIGenExtraOpaqueRender(const RiGenVisibility *vis = nullptr);

void updateRIGen(uint32_t curFrame, float dt);

void setTextureMinMipWidth(int textureSize, int impostorSize, float textureSizeMul, float impostorSizeMul);

bool traceRayRIGenNormalized(dag::Span<Trace> traces, TraceFlags trace_flags, int ray_mat_id = -1,
  rendinst::RendInstDesc *ri_desc = nullptr, const TraceMeshFaces *ri_cache = nullptr,
  riex_handle_t skip_riex_handle = rendinst::RIEX_HANDLE_NULL);
bool traceTransparencyRayRIGenNormalizedWithDist(const Point3 &pos, const Point3 &dir, float &t, float transparency_threshold,
  PhysMat::MatID ray_mat = PHYSMAT_INVALID, rendinst::RendInstDesc *ri_desc = NULL, int *out_mat_id = NULL,
  float *out_transparency = NULL, bool check_canopy = true);
bool traceTransparencyRayRIGenNormalized(const Point3 &pos, const Point3 &dir, float mint, float transparency_threshold,
  PhysMat::MatID ray_mat = PHYSMAT_INVALID, rendinst::RendInstDesc *ri_desc = NULL, int *out_mat_id = NULL,
  float *out_transparency = NULL, bool check_canopy = true);
void initTraceTransparencyParameters(float tree_trunk_opacity, float tree_canopy_opacity);

bool testObjToRIGenIntersection(CollisionResource *obj_res, const CollisionNodeFilter &filter, const TMatrix &obj_tm,
  const Point3 &obj_vel, Point3 *intersected_obj_pos, bool *tree_sphere_intersected, Point3 *collisionPoint);

void testObjToRIGenIntersection(const BBox3 &obj_box, const TMatrix &obj_tm, RendInstCollisionCB &callback, GatherRiTypeFlags ri_types,
  const TraceMeshFaces *ri_cache = nullptr, PhysMat::MatID ray_mat = PHYSMAT_INVALID, bool unlock_in_cb = false);
CheckBoxRIResultFlags checkBoxToRIGenIntersection(const BBox3 &box);

struct ForeachCB
{
  virtual void executeForTm(RendInstGenData * /* rgl */, const rendinst::RendInstDesc & /* ri_desc */, const TMatrix & /* tm */) {}
  virtual void executeForPos(RendInstGenData * /* rgl */, const rendinst::RendInstDesc & /* ri_desc */, const TMatrix & /* tm */) {}
};
void foreachRIGenInBox(const BBox3 &box, GatherRiTypeFlags ri_types, ForeachCB &cb);

bool destroyRIGenWithBullets(const Point3 &from, const Point3 &dir, float &dist, Point3 &norm, bool killBuildings, unsigned frameNo,
  ri_damage_effect_cb effect_cb);
bool destroyRIGenInSegment(const Point3 &p0, const Point3 &p1, bool trees, bool buildings, Point4 &contactPt, bool doKill,
  unsigned frameNo, ri_damage_effect_cb effect_cb);
void doRIGenDamage(const BSphere3 &sphere, unsigned frameNo, ri_damage_effect_cb effect_cb, const Point3 &axis = Point3(0.f, 0.f, 0.f),
  bool create_debris = true);
void doRIGenDamage(const BBox3 &box, unsigned frameNo, ri_damage_effect_cb effect_cb, const Point3 &axis = Point3(0.f, 0.f, 0.f),
  bool create_debris = true);
void doRIGenDamage(const BBox3 &box, unsigned frameNo, ri_damage_effect_cb effect_cb, const TMatrix &check_itm,
  const Point3 &axis = Point3(0.f, 0.f, 0.f), bool create_debris = true);
void doRIGenDamage(const Point3 &pos, unsigned frameNo, ri_damage_effect_cb effect_cb, const Point3 &axis = Point3(0.f, 0.f, 0.f),
  float dmg_pts = 1000, bool create_debris = true);
DestroyedRi *doRIGenExternalControl(const RendInstDesc &desc, bool rem_rendinst = true);
bool returnRIGenExternalControl(const RendInstDesc &desc, DestroyedRi *ri);
bool removeRIGenExternalControl(const RendInstDesc &desc, DestroyedRi *ri);
bool restoreRiGen(const RendInstDesc &desc, const RendInstBufferData &buffer);
riex_handle_t restoreRiGenDestr(const RendInstDesc &desc, const RendInstBufferData &buffer);
void setDirFromSun(const Point3 &d); // used for shadows
float getDefaultImpostorsDistAddMul();
float getDefaultDistAddMul();

bool isRIGenDestr(const RendInstDesc &desc);
void *getCollisionResourceHandle(const RendInstDesc &desc);
const CollisionResource *getRiGenCollisionResource(const RendInstDesc &desc);
bool isRIGenPosInst(const RendInstDesc &desc);
bool isDestroyedRIExtraFromNextRes(const RendInstDesc &desc);
int getRIExtraNextResIdx(int pool_id);
TMatrix getRIGenMatrix(const RendInstDesc &desc);
TMatrix getRIGenMatrixNoLock(const RendInstDesc &desc); // Assumes that everything required is already locked
TMatrix getRIGenMatrixDestr(const RendInstDesc &desc);
const char *getRIGenResName(const RendInstDesc &desc);
const char *getRIGenDestrName(const RendInstDesc &desc);
RendInstCollisionCB::CollisionInfo getRiGenDestrInfo(const RendInstDesc &desc);
int getRIGenStrideRaw(int layer_idx, int pool_id);
int getRIGenStride(int layer_idx, int cell_id, int pool_id);
BBox3 getRIGenBBox(const RendInstDesc &desc);
BBox3 getRIGenFullBBox(const RendInstDesc &desc);
DynamicPhysObjectData *doRIGenDestr(const RendInstDesc &desc, RendInstBufferData &out_buffer, RendInstDesc &out_desc, float dmg_pts,
  ri_damage_effect_cb effect_cb = NULL, riex_handle_t *out_gen_riex = NULL, bool restorable = false, int32_t user_data = -1,
  const Point3 *coll_point = NULL, bool *ri_removed = NULL,
  DestrOptionFlags destroy_flags = DestrOptionFlag::AddDestroyedRi | DestrOptionFlag::ForceDestroy);
bool isRiGenDescValid(const rendinst::RendInstDesc &desc);
float getTtl(const RendInstDesc &desc);

// This one ignores subcells and doesn't return buffer (as we use it when we don't need to restore it)
// As well it doesn't updateVb, as it's used in batches, so you'll updateVb only once, when you need it
DynamicPhysObjectData *doRIGenDestrEx(const RendInstDesc &desc, float dmg_pts, ri_damage_effect_cb effect_cb = NULL,
  int32_t user_data = -1);
DynamicPhysObjectData *doRIExGenDestrEx(rendinst::riex_handle_t riex_handle, ri_damage_effect_cb effect_cb = NULL);
void updateRiGenVbCell(int layer_idx, int cell_idx);

dag::Span<DestroyedCellData> getDestrCellData(int layer_idx);

// compatibility
void renderRendinstShadowsToTextures(const Point3 &sunDir0);
void preloadTexturesToBuildRendinstShadows();
extern bool rendinstClipmapShadows; // should be set only once before init
extern bool rendinstGlobalShadows;  // should be set only once before init
extern bool rendinstSecondaryLayer; // should be set only once before init

// pregen storage data format for tmInst (12xint32 instead of legacy 12xint16)
extern bool tmInst12x32bit;

typedef void (*damage_effect_cb)(int type, const TMatrix &emitter_tm, const TMatrix &fx_tm, int pool_idx, bool is_player,
  AcesEffect **locked_fx, const char *effect_template);
using calc_expl_damage_cb =
  eastl::fixed_function<3 * sizeof(void *), float(const rendinst::RendInstCollisionCB::CollisionInfo &coll_info, float distance)>;

void set_billboards_vertical(bool is_vertical);
void disableShadowImpostorForAllLod();
void setImpostorDiffuseSizeMul(int value);
bool enable_impostors_compression(bool enabled);
void setDistMul(float distMul, float distOfs, bool force_impostors_and_mul = false,
  float impostors_far_dist_additional_mul = 1.f); // 0.2353, 0.0824 will remap 0.5 .. 2.2 to 0.2 .. 0.6
void setImpostorsDistAddMul(float impostors_dist_additional_mul);
void setImpostorsFarDistAddMul(float impostors_far_dist_additional_mul);
void updateSettingsDistMul();
void updateSettingsDistMul(float v);
void updateMinCullSettingsDistMul(float v);
void updateRIExtraMulScale();
void setLodsShiftDistMult();
void setLodsShiftDistMult(float mul);
void resetLodsShiftDistMult();
void setAdditionalRiExtraLodDistMul(float mul);
void setZoomDistMul(float mul, bool scale_lod1_dist, float impostors_far_dist_additional_mul = 1.f); // multiplication for different
                                                                                                     // aspect ratio
void overrideLodRanges(const DataBlock &ri_lod_ranges);
void onSettingsChanged(const Point3 &sun_dir_0);
inline bool traceRayRendInstsNormalized(dag::Span<Trace> traces, bool = false, bool trace_meshes = false,
  rendinst::RendInstDesc *ri_desc = NULL, bool trace_trees = false, int ray_mat_id = -1, const TraceMeshFaces *ri_cache = nullptr)
{
  TraceFlags traceFlags = TraceFlag::Destructible;
  if (trace_meshes)
    traceFlags |= TraceFlag::Meshes;
  if (trace_trees)
    traceFlags |= TraceFlag::Trees;
  return traceRayRIGenNormalized(traces, traceFlags, ray_mat_id, ri_desc, ri_cache);
}

bool traceDownMultiRay(dag::Span<Trace> traces, bbox3f_cref rayBox, dag::Span<rendinst::RendInstDesc> ri_desc,
  const TraceMeshFaces *ri_cache = NULL, int ray_mat_id = -1, TraceFlags trace_flags = TraceFlag::Destructible,
  Bitarray *filter_pools = nullptr); // all rays should be down
bool checkCachedRiData(const TraceMeshFaces *ri_cache);
bool initializeCachedRiData(TraceMeshFaces *ri_cache);
void clipCapsuleRI(const ::Capsule &c, Point3 &lpt, Point3 &wpt, real &md, const Point3 &movedirNormalized,
  const TraceMeshFaces *ri_cache);

bool traceRayRendInstsNormalized(const Point3 &from, const Point3 &dir, float &tout, Point3 &norm, bool extend_bbox = false,
  bool trace_meshes = false, rendinst::RendInstDesc *ri_desc = NULL, bool trace_trees = false, int ray_mat_id = -1,
  int *out_mat_id = nullptr);
bool traceRayRendInstsListNormalized(const Point3 &from, const Point3 &dir, float dist, Tab<rendinst::TraceRayRendInstData> &ri_data,
  bool trace_meshes = false);
using RendInstsIntersectionsList = dag::Vector<TraceRayRendInstData, framemem_allocator>;
bool traceRayRendInstsListAllNormalized(const Point3 &from, const Point3 &dir, float dist, RendInstsIntersectionsList &ri_data,
  bool trace_meshes = false);

void computeRiIntersectedVolumes(RendInstsIntersectionsList &intersected, const Point3 &dir, VolumeSectionsMerge merge_mode);

bool rayhitRendInstNormalized(const Point3 &from, const Point3 &dir, float t, int ray_mat_id, const rendinst::RendInstDesc &ri_desc);
bool rayhitRendInstsNormalized(const Point3 &from, const Point3 &dir, float t, float min_size, int ray_mat_id,
  rendinst::RendInstDesc *ri_desc);
inline bool rayhitRendInstsNormalized(const Point3 &from, const Point3 &dir, float t, int ray_mat_id = -1,
  rendinst::RendInstDesc *ri_desc = NULL)
{
  return rayhitRendInstsNormalized(from, dir, t, 0.0f, ray_mat_id, ri_desc);
}

inline bool destroyRIInSegment(const Point3 &p0, const Point3 &p1, bool trees, bool buildings, Point4 &contactPt, bool doKill,
  uint32_t frameNo, damage_effect_cb effect_cb)
{
  return destroyRIGenInSegment(p0, p1, trees, buildings, contactPt, doKill, frameNo, effect_cb);
}
inline bool destroyRIWithBullets(const Point3 &from, const Point3 &dir, float &dist, Point3 &norm, bool killBuildings,
  uint32_t frameNo, damage_effect_cb effect_cb)
{
  return destroyRIGenWithBullets(from, dir, dist, norm, killBuildings, frameNo, effect_cb);
}

inline bool testObjectToRendinstIntersection(CollisionResource *object_res, const CollisionNodeFilter &filter,
  const TMatrix &object_tm, const Point3 &velocity, Point3 *intersected_object_pos, String *intersected_object_name,
  bool *tree_sphere_intersected, Point3 *collisionPoint, bool *intersectedWithWood,
  bool, // intersectWithWood
  bool) // intersectWithOther
{
  (void)intersected_object_name;
  if (tree_sphere_intersected)
    *tree_sphere_intersected = false;

  if (testObjToRIGenIntersection(object_res, filter, object_tm, velocity, intersected_object_pos, tree_sphere_intersected,
        collisionPoint))
  {
    if (intersectedWithWood)
      *intersectedWithWood = true;
    return true;
  }
  return false;
}
void setPreloadPointForRendInsts(const Point3 &pointAt); // sets additional POI with small radius. this point will expire in 5 seconds

inline void doRendinstDebris(const BBox3 &, bool, uint32_t, damage_effect_cb)
{
  // fixme: not implemented!
}

inline void doRendinstDebris(const BSphere3 &, bool, uint32_t, damage_effect_cb)
{
  // fixme: not implemented!
}

void hideRIGenExtraNotCollidable(const BBox3 &box, bool hide_immortals);

struct RendinstLandclassData
{
  int index;
  TMatrix matrixInv;
  Point4 mapping;
  bbox3f bbox;
  float radius;
  float sortOrder;
};
using TempRiLandclassVec = dag::Vector<rendinst::RendinstLandclassData, framemem_allocator>;
extern void getRendinstLandclassData(TempRiLandclassVec &ri_landclasses);

extern float getMaxRendinstHeight();
extern float getMaxRendinstHeight(int variableId);
void closeImpostorShadowTempTex();

bool isRiGenVisibilityLodsLoaded(const RiGenVisibility *visibility);

void set_riextra_instance_seed(rendinst::riex_handle_t, int32_t data);
void reinitOnShadersReload();
extern void updateHeapVb(); // Optional, when you need to do it outside of the render.

using RiApexIterationCallback = eastl::fixed_function<32, void(const char *riResName, const char *apexDestructionOptionsPresetName)>;
void iterate_apex_destructible_rendinst_data(RiApexIterationCallback callback);

bool should_init_ri_damage_model();
dag::ConstSpan<const char *> get_ri_res_names_for_damage_model();

const char *get_rendinst_res_name_from_col_info(const RendInstCollisionCB::CollisionInfo &col_info);
int get_debris_fx_type_id(RendInstGenData *rgl, const rendinst::RendInstDesc &ri_desc);

struct RiDestrData
{
  float collisionHeightScale;
  bool bushBehaviour;

  int fxType;
  float fxScale;
  const char *fxTemplate;
};

RiDestrData gather_ri_destr_data(const RendInstDesc &ri_desc);
bool ri_gen_has_collision(const RendInstDesc &ri_desc);

uint32_t get_riextra_instance_seed(rendinst::riex_handle_t);
float get_riextra_ttl(rendinst::riex_handle_t);
bool get_riextra_immortality(rendinst::riex_handle_t);

RendInstDesc get_restorable_desc(const RendInstDesc &ri_desc);
int find_restorable_data_index(const RendInstDesc &desc);

}; // namespace rendinst

namespace rendinstgenrender
{
inline constexpr int MAX_LOD_COUNT_WITH_ALPHA = rendinst::MAX_LOD_COUNT + 1;
enum
{
  INSTANCING_TEXREG = 12,
  GPU_INSTANCING_OFSBUFFER_TEXREG = 11,
  TREECROWN_TEXREG = 16
};
extern bool avoidStaticShadowRecalc;
extern bool useConditionalRendering;
extern bool debugLods;
extern void setApexInstancing();
extern bool tmInstColored;
extern bool impostorPreshadowNeedUpdate;
extern float riExtraMinSizeForReflection;
extern float riExtraMinSizeForDraftDepth;

void useRiDepthPrepass(bool use);
void useRiCellsDepthPrepass(bool use);
void useImpostorDepthPrepass(bool use);
}; // namespace rendinstgenrender
