//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
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


class CollisionResource;
class Point3;
class DataBlock;
struct RiGenVisibility;
class Occlusion;
class OcclusionMap;
class IGenLoad;

inline constexpr float MIN_SETTINGS_RENDINST_DIST_MUL = 0.5f;
inline constexpr float MAX_SETTINGS_RENDINST_DIST_MUL = 3.0f;
inline constexpr float MIN_EFFECTIVE_RENDINST_DIST_MUL = 0.1f;
inline constexpr float MAX_EFFECTIVE_RENDINST_DIST_MUL = 3.0f;

// use additional data as hashVal only when in Tools (De3X, AV2)
#define RIGEN_PERINST_ADD_DATA_FOR_TOOLS (_TARGET_PC && !_TARGET_STATIC_LIB)
#define RI_COLLISION_RES_SUFFIX          "_collision"

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

extern void (*shadow_invalidate_cb)(const BBox3 &box);
extern BBox3 (*get_shadows_bbox_cb)();

void register_land_gameres_factory();

inline constexpr unsigned HUID_LandClassGameRes = 0x03FB59C4u; // LandClass
const DataBlock &get_detail_data(void *land_cls_res);
int get_landclass_data_hash(void *land_cls_res, void *buf, int buf_sz);

void configurateRIGen(const DataBlock &riSetup);

void initRIGen(bool need_render, int cell_pool_sz, float poi_radius, ri_register_collision_cb reg_coll_cb = nullptr,
  ri_unregister_collision_cb unreg_coll_cb = nullptr, int job_manager_id = -1, float sec_layer_poi_radius = 256.0f,
  bool simplified_render = false, bool should_init_gpu_objects = true);
void termRIGen();

void setPoiRadius(float poi_radius);

void clearRIGen();

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

void precomputeRIGenCellsAndPregenerateRIExtra();

const DataBlock *registerRIGenExtraConfig(const DataBlock *persist_props);
void addRIGenExtraSubst(const char *ri_res_name);

const bbox3f &getMovedDebrisBbox();
void walkRIGenResourceNames(res_walk_cb cb);
bool hasRiLayer(int res_idx, LayerFlag layer);

CollisionResource *getRIGenCollInfo(const rendinst::RendInstDesc &desc);

void registerRIGenSweepAreas(dag::ConstSpan<TMatrix> box_itm_list);
int scheduleRIGenPrepare(dag::ConstSpan<Point3> poi);
bool isRIGenPrepareFinished();

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

void updateRIGen(uint32_t curFrame, float dt);

void setTextureMinMipWidth(int textureSize, int impostorSize, float textureSizeMul, float impostorSizeMul);

void setDirFromSun(const Point3 &d); // used for shadows
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
void updateSettingsDistMul();
void updateSettingsDistMul(float v);
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

void updateHeapVb(); // Optional, when you need to do it outside of the render.
void updateHeapVbNoLock();

using RiApexIterationCallback = eastl::fixed_function<32, void(const char *riResName, const char *apexDestructionOptionsPresetName)>;
void iterate_apex_destructible_rendinst_data(RiApexIterationCallback callback);

bool should_init_ri_damage_model();
void dm_iter_all_ri_layers(eastl::fixed_function<16, void(int, const dag::ConstSpan<const char *> &)> cb);

}; // namespace rendinst
