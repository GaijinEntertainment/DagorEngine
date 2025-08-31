//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <rendInst/rendInstDesc.h>
#include <rendInst/layerFlags.h>

#include <EASTL/fixed_function.h>
#include <memory/dag_framemem.h>
#include <vecmath/dag_vecMathDecl.h>
#include <math/dag_TMatrix.h>
#include <math/dag_Point4.h>
#include <generic/dag_tab.h>
#include <util/dag_multicastEvent.h>

class CollisionResource;
class Point3;
class DataBlock;
struct RiGenVisibility;
class Occlusion;
class OcclusionMap;
class IGenLoad;
class RenderableInstanceLodsResource;
class ShaderMaterial;

inline float MIN_SETTINGS_RENDINST_DIST_MUL = 0.5f;
inline float MAX_SETTINGS_RENDINST_DIST_MUL = 3.0f;
inline float MIN_EFFECTIVE_RENDINST_DIST_MUL = 0.1f;
inline float MAX_EFFECTIVE_RENDINST_DIST_MUL = 3.0f;

#define RI_COLLISION_RES_SUFFIX "_collision"

namespace rendinst
{

inline uint32_t mat44_to_ri_type(mat44f_cref node)
{
  return (((uint32_t *)(char *)&node.col2)[3]) & 0xFFFF;
} // this is copy&paste from scene

typedef void *(*ri_register_collision_cb)(const CollisionResource *collRes, const char *debug_name);
typedef void (*ri_unregister_collision_cb)(void *&handle);
typedef eastl::fixed_function<2 * sizeof(void *), void(const char *)> res_walk_cb;

extern void (*do_delayed_ri_extra_destruction)();
extern void (*sweep_rendinst_cb)(const RendInstDesc &);

extern void (*shader_material_validation_cb)(ShaderMaterial *mat, const char *res_name);
extern void (*on_vsm_invalidate)();

void register_land_gameres_factory();

inline constexpr unsigned HUID_LandClassGameRes = 0x03FB59C4u; // LandClass
int get_landclass_data_hash(void *land_cls_res, void *buf, int buf_sz);

void configurateRIGen(const DataBlock &riSetup);

void initRIGen(bool need_render, int cell_pool_sz, float poi_radius, ri_register_collision_cb reg_coll_cb = nullptr,
  ri_unregister_collision_cb unreg_coll_cb = nullptr, int job_manager_id = -1, float sec_layer_poi_radius = 256.0f,
  bool simplified_render = false, bool should_init_gpu_objects = true);
void termRIGen();

void setPoiRadius(float poi_radius);

void clearRIGen();
void clearRIGenDestrData();

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
  Tab<char> *out_reserve_storage_pregen_ent_for_tools = nullptr, const DataBlock *level_blk = nullptr);
void prepareRIGen(bool init_sec_ri_extra_here = false, const DataBlock *level_blk = nullptr);

using FxTypeByNameCallback = int (*)(const char *);
void initRiGenDebris(const DataBlock &ri_blk, FxTypeByNameCallback get_fx_type_by_name, bool init_sec_ri_extra_here = true);
void initRiGenDebrisSecondary(const DataBlock &ri_blk, FxTypeByNameCallback get_fx_type_by_name);
void updateRiDestrFxIds(FxTypeByNameCallback get_fx_type_by_name);
void initRiSoundOccluders(const dag::ConstSpan<eastl::pair<const char *, const char *>> &ri_name_to_occluder_type,
  const dag::ConstSpan<eastl::pair<const char *, float>> &occluders);
float debugGetSoundOcclusion(const char *ri_name, float def_value);

void precomputeRIGenCellsAndPregenerateRIExtra();

const DataBlock *getRIGenExtraConfig();
const DataBlock *registerRIGenExtraConfig(const DataBlock *persist_props);
void addRIGenExtraSubst(const char *ri_res_name);

void walkRIGenResourceNames(res_walk_cb cb);
bool hasRiLayer(int res_idx, LayerFlag layer);

CollisionResource *getRIGenCollInfo(const rendinst::RendInstDesc &desc);

void registerRIGenSweepAreas(dag::ConstSpan<TMatrix> box_itm_list);
int scheduleRIGenPrepare(dag::ConstSpan<Point3> poi);
bool isRIGenPrepareFinished();

using ri_added_from_rigendata_cb = void (*)(riex_handle_t handle);
void registerRiExtraAddedFromGenDataCb(ri_added_from_rigendata_cb cb);
bool unregisterRiExtraAddedFromGenDataCb(ri_added_from_rigendata_cb cb);
void onRiExtraAddedFromGenData(riex_handle_t id);

using ri_gen_cell_loaded_cb = void (*)(int layer_idx, int cell_idx, int x, int z, bool loaded, bool last_job);
void registerRiGenCellLoadedCb(ri_gen_cell_loaded_cb cb);
bool unregisterRiGenCellLoadedCb(ri_gen_cell_loaded_cb cb);
void onRiGenCellLoaded(int layer_idx, int cell_idx, int x, int z, bool loaded, bool last_job);

bool isRiGenCellPrepared(int layer);

void get_ri_color_infos(eastl::fixed_function<sizeof(void *) * 4, void(E3DCOLOR colorFrom, E3DCOLOR colorTo, const char *name)> cb);
extern void regenerateRIGen();

extern void enableSecLayer(bool en);
extern bool isSecLayerEnabled();

extern void applyBurningDecalsToRi(const bbox3f &decal);

extern void copyVisibileImpostorsData(const RiGenVisibility *visibility, bool clear_prev_result);
extern void updateRIGenImpostors(float shadowDistance, const Point3 &sunDir0, const TMatrix &view_itm, const mat44f &proj_tm);
extern void resetRiGenImpostors();
extern void getLodCounter(int lod, const RiGenVisibility *visibility, int &subCellNo, int &cellNo);
extern void set_per_instance_visibility_for_any_tree(bool on); // should be on only in tank

extern float getMaxFarplaneRIGen(bool sec_layer = false);
extern float getMaxAverageFarplaneRIGen(bool sec_layer = false);
extern void startUpdateRIGenGlobalShadows();
extern void startUpdateRIGenClipmapShadows();

extern void prepareOcclusion(Occlusion *occlusion, OcclusionMap *occlusion_map);
struct RIOcclusionData;
extern void destroyOcclusionData(RIOcclusionData *&data);
extern RIOcclusionData *createOcclusionData();
extern void iterateOcclusionData(RIOcclusionData &od, const mat44f *&mat, const IPoint2 *&indices, uint32_t &cnt); // indices.x is
                                                                                                                   // index in mat
                                                                                                                   // array
extern void prepareOcclusionData(mat44f_cref globtm, vec4f vpos, RIOcclusionData &, int max_ri_to_add, bool walls);

void updateRIGen(float dt, bbox3f *movedDebrisBbox = nullptr);

void setTextureMinMipWidth(int textureSize, int impostorSize, float textureSizeMul, float impostorSizeMul);

float getDefaultImpostorsDistAddMul();
float getDefaultDistAddMul();

void updateRiGenVbCell(int layer_idx, int cell_idx);

void preloadTexturesToBuildRendinstShadows();
extern bool rendinstClipmapShadows; // should be set only once before init
extern bool rendinstGlobalShadows;  // should be set only once before init
extern bool rendinstSecondaryLayer; // should be set only once before init

// pregen storage data format for tmInst (12xint32 instead of legacy 12xint16)
extern bool tmInst12x32bit;

void set_billboards_vertical(bool is_vertical);
void setDistMul(float distMul, float distOfs, bool force_impostors_and_mul = false,
  float impostors_far_dist_additional_mul = 1.f); // 0.2353, 0.0824 will remap 0.5 .. 2.2 to 0.2 .. 0.6
void setImpostorsDistAddMul(float impostors_dist_additional_mul);
void setImpostorsFarDistAddMul(float impostors_far_dist_additional_mul);
void setImpostorsMinRange(float impostors_min_range);
void updateSettingsDistMul();
void updateSettingsDistMul(float v);
void updateMinSettingsDistMul(float v);
void updateMinEffectiveRendinstDistMul(float v);
void updateMaxSettingsDistMul(float v);
void updateMaxEffectiveRendinstDistMul(float v);
float getSettingsDistMul();
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

void setPreloadPointForRendInsts(const Point3 &pointAt); // sets additional POI with small radius. this point will expire in 5 seconds

void hideRIGenExtraNotCollidable(const BBox3 &box, bool hide_immortals);

extern float getMaxRendinstHeight();
extern float getMaxRendinstHeight(int variableId);
void closeImpostorShadowTempTex();

void updateHeapVb(); // Optional, when you need to do it outside of the render.
void updateHeapVbNoLock();

using RiApexIterationCallback = eastl::fixed_function<32, void(const char *riResName, const char *apexDestructionOptionsPresetName)>;
void iterate_apex_destructible_rendinst_data(RiApexIterationCallback callback);

bool should_init_ri_damage_model();
void dm_iter_all_ri_layers(eastl::fixed_function<20, void(int, const dag::ConstSpan<const char *> &)> cb);

}; // namespace rendinst
