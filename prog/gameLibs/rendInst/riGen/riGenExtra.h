#pragma once

#include <rendInst/rendInstGen.h>

#include <generic/dag_tab.h>
#include <vecmath/dag_vecMathDecl.h>
#include <osApiWrappers/dag_rwLock.h>
#include <osApiWrappers/dag_spinlock.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_simpleString.h>
#include <util/dag_delayedAction.h>
#include <util/dag_simpleString.h>
#include <scene/dag_physMat.h>
#include <scene/dag_tiledScene.h>
#include <math/dag_mathUtils.h>
#include <dag/dag_vector.h>
#include <3d/dag_texStreamingContext.h>
#include <3d/dag_resPtr.h>
#include <util/dag_multicastEvent.h>


struct MaterialRayStrat
{
  PhysMat::MatID rayMatId;
  bool processPosRI = false;

  MaterialRayStrat(PhysMat::MatID ray_mat, bool process_pos_ri = false) : rayMatId(ray_mat), processPosRI(process_pos_ri) {}

  bool shouldIgnoreRendinst(bool is_pos, bool /* is_immortal */, PhysMat::MatID mat_id) const
  {
    if (is_pos && !processPosRI)
      return true;

    if (rayMatId == PHYSMAT_INVALID)
      return false;

    return !PhysMat::isMaterialsCollide(rayMatId, mat_id);
  }

  inline bool isCollisionRequired() const { return true; }
};


class RenderableInstanceLodsResource;
class CollisionResource;
class RingDynamicSB;

namespace rendinstgenrender
{
template <int NUM_LODS>
class GeomInstancingData;
}

namespace rendinst
{
inline constexpr int RIEXTRA_VECS_COUNT = 4;
struct RiExtraPool
{
  static constexpr int MAX_LODS = 4;
  RenderableInstanceLodsResource *res;
  CollisionResource *collRes;
  void *collHandle;

  vec4f bsphXYZR;
  union
  {
    float distSqLOD[MAX_LODS];
    unsigned distSqLOD_i[MAX_LODS];
    vec4f distSqLODV;
  };
  int riPoolRef;
  uint32_t layers = 0; // LAYER_OPAQUE, LAYER_TRANSPARENT, LAYER_DECALS
  unsigned riPoolRefLayer : 4;
  unsigned useShadow : 1, posInst : 1, destroyedColl : 1, immortal : 1, hasColoredShaders : 1, isTree : 1, isTessellated : 1;
  unsigned hasOccluder : 1, largeOccluder : 1, isWalls : 1, useVsm : 1, usedInLandmaskHeight : 1;
  unsigned wasSavedToElems : 1, patchesHeightmap : 1, hasDynamicDisplacement : 1, usingClipmap : 1;
  unsigned killsNearEffects : 1;
  int gridIdx : 5;
  uint8_t hideMask;
  struct ElemMask
  {
    uint32_t atest, cullN;
  } elemMask[MAX_LODS];
  struct ElemUniqueData
  {
    int cellId;
    int offset;
  };
  struct HPandLDT
  {
    float healthPoints, lastDmgTime; // fixme: replace with short?

    void init(float max_hp)
    {
      lastDmgTime = -0.5f;
      healthPoints = max_hp;
    }
    float getHp(float t, float rrate, float max_hp)
    {
      if (lastDmgTime < 0 || t < lastDmgTime)
        return healthPoints;
      float hp = healthPoints + (t - lastDmgTime) * rrate;
      if (hp < max_hp)
        return hp;
      lastDmgTime = -0.5f;
      return healthPoints = max_hp;
    }
    void makeInvincible() { lastDmgTime = -2; }
    void makeNonRegenerating() { lastDmgTime = -1; }
    bool isInvincible() const { return lastDmgTime < -1.5f; }
    void applyDamageNoClamp(float dmg, float t)
    {
      healthPoints -= dmg;
      if (lastDmgTime > -0.75f)
        lastDmgTime = t;
    }
  };

  Tab<mat43f> riTm;
  Tab<vec4f> riXYZR; // R<0 mean 'not in grid'
  E3DCOLOR poolColors[2];
  Tab<uint16_t> uuIdx;
  Tab<HPandLDT> riHP;
  Tab<ElemUniqueData> riUniqueData; // This way we'll identify them after generation, so we can keep them synced
  bbox3f lbb;
  bbox3f collBb;
  bbox3f fullWabb;

  eastl::vector<uint32_t> tsNodeIdx; //(2:20) TiledScene node_index for each instanse (highest 2 bits select TileScene index)
  int tsIndex = -1;

  eastl::vector<int> variableIdsToUpdateMaxHeight;
  float initialHP;
  float regenHpRate;
  int destroyedRiIdx;
  int parentForDestroyedRiIdx;
  bool scaleDebris;
  int destrDepth;
  class DynamicPhysObjectData *destroyedPhysRes;
  bool isDestroyedPhysResExist;
  int destrFxType;
  int destrCompositeFxId = -1;
  float destrFxScale;
  int dmgFxType = -1;
  float dmgFxScale = 1.f;
  float damageThreshold;
  uint8_t destrStopsBullets : 1;

  float sphereRadius;
  float sphCenterY;
  float radiusFadeDrown;
  float radiusFade;
  unsigned lodLimits;
  float ttl;
  bool isRendinstClipmap;
  bool isPaintFxOnHit;
  bool isDynamicRendinst;

  struct RiLandclassCachedData
  {
    int index;
    Point4 mapping;
  };
  Tab<RiLandclassCachedData> riLandclassCachedData; // 0 size if not RI landclass (dirty hack to emulate unique ptr)

  float hardness;
  float rendinstHeight; // some buildings made by putting one ri at other, so the actual height of ri not equal bbox height

  int clonedFromIdx = -1;

  char qlPrevBestLod;
  SimpleString dmgFxTemplate;
  SimpleString destrFxTemplate;

  float scaleForPrepasses = 1.0f;

  RiExtraPool() :
    res(NULL),
    collRes(NULL),
    collHandle(NULL),
    useShadow(false),
    riPoolRef(-1),
    posInst(0),
    hasColoredShaders(0),
    hideMask(0),
    largeOccluder(false),
    hasOccluder(0),
    initialHP(-1),
    regenHpRate(0),
    destroyedRiIdx(-1),
    parentForDestroyedRiIdx(-1),
    destrDepth(0),
    destroyedPhysRes(NULL),
    isDestroyedPhysResExist(false),
    destroyedColl(false),
    destrFxType(-1),
    destrFxScale(0),
    destrStopsBullets(true),
    damageThreshold(0),
    lodLimits(defLodLimits),
    sphereRadius(1.f),
    sphCenterY(0.f),
    scaleDebris(false),
    ttl(-1.f),
    usingClipmap(0),
    patchesHeightmap(0),
    isDynamicRendinst(false),
    hardness(1),
    rendinstHeight(0.0f),
    radiusFadeDrown(0.f),
    radiusFade(0.f),
    wasSavedToElems(1), // Note: 1 to ignore this instance on rebuild until setWasNotSavedToElems() called
    qlPrevBestLod(0),
    hasDynamicDisplacement(0)
  {
    memset(distSqLOD, 0, sizeof(distSqLOD));
    memset(elemMask, 0, sizeof(elemMask));
    bsphXYZR = v_make_vec4f(0, 0, 0, 1);
  }
  RiExtraPool(const RiExtraPool &) = delete;
  ~RiExtraPool();

  bool isEmpty() const { return riTm.size() == uuIdx.size(); }
  int getEntitiesCount() const { return riTm.size() - uuIdx.size(); }
  void setWasNotSavedToElems();

  bool isPosInst() const { return posInst; }
  bool hasImpostor() const { return posInst; }
  bool isValid(uint32_t idx) const
  {
    if (idx >= riTm.size())
      return false;
    uint64_t *p = (uint64_t *)&riTm.data()[idx];
    return p[0] || p[1];
  }
  bool isInGrid(int idx) const
  {
    if (idx < 0 || idx >= riXYZR.size())
      return false;
    uint32_t r_code = ((uint32_t *)&riXYZR[idx])[3];
    return r_code && (r_code & 0x80000000) == 0;
  }
  float getObjRad(int idx) const { return ((unsigned)idx < (unsigned)riXYZR.size()) ? v_extract_w(riXYZR[idx]) : 0.f; }

  float getHp(int idx)
  {
    if (idx < 0 || idx >= riHP.size())
      return 0.f;
    return riHP[idx].getHp(*session_time, regenHpRate, initialHP);
  }
  void setHp(int idx, float hp)
  {
    if (idx < 0 || idx >= riHP.size())
      return;
    riHP[idx].init((hp <= 0.0f) ? initialHP : hp);
    if (hp < -1.5f) // -2
      riHP[idx].makeInvincible();
    else if (hp != 0.0f) // -1 or > 0
      riHP[idx].makeNonRegenerating();
  }
  scene::node_index getNodeIdx(int idx) const
  {
    if (idx < 0 || idx >= tsNodeIdx.size())
      return scene::INVALID_NODE;
    return tsNodeIdx[idx] & 0x3fffffff;
  }
  float bsphRad() const { return v_extract_w(bsphXYZR); }

  void validateLodLimits();

  static unsigned defLodLimits;
  static const float *session_time;
};

struct RenderElem
{
  ShaderElement *shader;
  short vstride; // can be 8 bit
  uint8_t vbIdx, drawOrder;
  int si, numf, baseVertex;
};
extern dag::Vector<RenderElem> allElems;
extern dag::Vector<uint32_t> allElemsIndex;
extern int pendingRebuildCnt;
extern bool relemsForGpuObjectsHasRebuilded;
void rebuildAllElemsInternal();
inline void rebuildAllElems()
{
  if (interlocked_acquire_load(pendingRebuildCnt))
    rebuildAllElemsInternal();
}

extern CallbackToken meshRElemsUpdatedCbToken;
void on_ri_mesh_relems_updated(const RenderableInstanceLodsResource *r);
void on_ri_mesh_relems_updated_pool(uint32_t poolId);
void updateShaderElems(uint32_t poolI);
class RiExtraPoolsVec : public dag::Vector<RiExtraPool> // To consider: move this methods to dag::Vector?
{
public:
  using dag::Vector<RiExtraPool>::Vector;
  uint32_t size_interlocked() const;
  uint32_t interlocked_increment_size();
};
extern RiExtraPoolsVec riExtra;
extern FastNameMap riExtraMap;

extern SmartReadWriteFifoLock ccExtra;
extern int maxExtraRiCount;

// 0th is main, 1th is for async opaque render
inline constexpr size_t RIEX_RENDERING_CONTEXTS = 2;

extern int maxRenderedRiEx[RIEX_RENDERING_CONTEXTS];
extern int perDrawInstanceDataBufferType;
extern int instancePositionsBufferType;
extern float riExtraLodDistSqMul;
extern float riExtraCullDistSqMul;
extern Tab<uint16_t> riExPoolIdxPerStage[RIEX_STAGE_COUNT];

extern float half_minimal_grid_cell_size;
inline float short_ray_length() { return half_minimal_grid_cell_size; }


void initRIGenExtra(bool need_render = true, const DataBlock *level_blk = nullptr);
void allocateRIGenExtra(struct VbExtraCtx &vbctx);
void termRIGenExtra();
void update_per_draw_gathered_data(uint32_t id);

void renderRIGenExtra(const RiGenVisibility &v, RenderPass render_pass, OptimizeDepthPass optimization_depth_pass,
  OptimizeDepthPrepass optimization_depth_prepass, IgnoreOptimizationLimits ignore_optimization_instances_limits, uint32_t layer,
  uint32_t instance_count_mul, TexStreamingContext texCtx, AtestStage atest_stage = AtestStage::All,
  const RiExtraRenderer *riex_renderer = nullptr);

bool isRIGenExtraObstacle(const char *nm);
bool isRIGenExtraUsedInDestr(const char *nm);
bool rayHitRIGenExtraCollidable(const Point3 &p0, const Point3 &norm_dir, float len, rendinst::RendInstDesc &ri_desc,
  const MaterialRayStrat &strategy, float min_r);
void reapplyLodRanges();
void addRiExtraRefs(DataBlock *b, const DataBlock *riConf, const char *name);

class RendinstTiledScene : protected scene::TiledScene
{
public:
  enum
  {
    REFLECTION = 1 << 0,
    HAS_OCCLUDER = 1 << 1,
    LARGE_OCCLUDER = 1 << 2,
    HAVE_SHADOWS = 1 << 3,
    VISIBLE_0 = 1 << 4,
    VISUAL_COLLISION = 1 << 5,
    IS_WALLS = 1 << 6,
    VISIBLE_2 = 1 << 7,
    IS_RENDINST_CLIPMAP = 1 << 8,
    VISIBLE_IN_SHADOWS = 1 << 9,
    CHECKED_IN_SHADOWS = 1 << 10,
    VISIBLE_IN_VSM = 1 << 11,
    DRAFT_DEPTH = 1 << 12,
    VISIBLE_IN_LANDMASK = 1 << 13,
    NEEDS_CHECK_IN_SHADOW = 1 << 14,
  };
  using SimpleScene::calcNodeBox;
  using SimpleScene::getNode;
  using SimpleScene::getNodeFlags;
  using SimpleScene::getNodePool;
  using SimpleScene::isAliveNode;
  using TiledScene::allocate;
  using TiledScene::boxCull;
  using TiledScene::destroy;
  using TiledScene::doMaintenance;
  using TiledScene::dumpSceneState;
  using TiledScene::flushDeferredTransformUpdates;
  using TiledScene::frustumCull;
  using TiledScene::frustumCullOneTile;
  using TiledScene::frustumCullTilesPass;
  using TiledScene::getNodeIndexInternal; // for faster visibility in dev
  using TiledScene::getNodesCount;
  using TiledScene::getTileSize;
  using TiledScene::lockForRead;
  using TiledScene::setFlags;
  using TiledScene::setKdtreeBuildParams;
  using TiledScene::setKdtreeRebuildParams;
  using TiledScene::setNodeUserData;
  using TiledScene::setPoolBBox;
  using TiledScene::setPoolDistanceSqScale;
  using TiledScene::setTransform;
  using TiledScene::term;
  using TiledScene::unlockAfterRead;
  using TiledScene::unsetFlags;

  using SimpleScene::begin;
  using SimpleScene::end;

  enum
  {
    ADDED = 0,
    MOVED,
    POOL_ADDED,
    BOX_OCCLUDER,
    QUAD_OCCLUDER,
    SET_UDATA,
    COMPARE_AND_SWAP_UDATA,
    INVALIDATE_SHADOW_DIST,
    SET_FLAGS_IN_BOX
  };
  enum
  {
    LIGHTDIST_TOOBIG = 255,
    LIGHTDIST_DYNAMIC = 254,
    LIGHTDIST_INVALID = 253
  };
  void init(float tile_sz, int nodes_reserve)
  {
    TiledScene::init(tile_sz);
    reserve(nodes_reserve);
  }

  uint8_t getDistance(scene::node_index id) const
  {
    uint32_t index = TiledScene::getNodeIndexInternal(id); // for perf in dev mode use internal index
    if (index < distance.size())
      return distance.data()[index];
    return LIGHTDIST_INVALID;
  }

  void setDistance(scene::node_index ni, uint8_t dist) const
  {
    const uint32_t index = TiledScene::getNodeIndex(ni);
    if (index >= distance.size())
      distance.resize(index + 1, LIGHTDIST_INVALID);
    distance[index] = dist;
  }

  uint8_t getDistanceMT(scene::node_index id) const
  {
    uint32_t index = TiledScene::getNodeIndexInternal(id); // for perf in dev mode use internal index
    G_ASSERT_RETURN(index < distance.size(), LIGHTDIST_INVALID);
    return interlocked_relaxed_load(distance.data()[index]);
  }

  void setDistanceMT(scene::node_index ni, uint8_t dist) const
  {
    const uint32_t index = TiledScene::getNodeIndex(ni);
    G_ASSERT_RETURN(index < distance.size(), );
    interlocked_relaxed_store(distance.data()[index], dist);
  }

  void debugDumpScene(const char *file_name);
  void shrinkDistance() { distance.resize(getNodesCount(), LIGHTDIST_INVALID); }

  void setNodeUserDataDeferred(scene::node_index node, unsigned user_cmd, int dw_cnt, const int32_t *dw_data)
  {
    G_ASSERT(dw_cnt * 4 + 4 <= sizeof(mat44f));
    G_ASSERTF_AND_DO(user_cmd < DeferredCommand::SET_USER_DATA, user_cmd = DeferredCommand::SET_USER_DATA - 1, "user_cmd=0x%X",
      user_cmd);
    size_t idx = mDeferredCommand.size();
    mDeferredCommand.resize(idx + 1);

    DeferredCommand &cmd = mDeferredCommand[idx];
    cmd.command = DeferredCommand::Cmd(cmd.SET_USER_DATA + user_cmd);
    cmd.node = node;
    memcpy(&cmd.transformCommand, dw_data, dw_cnt * 4);
    ((int *)&cmd.transformCommand.col3)[3] = dw_cnt;
  }

  template <typename ImmediatePerform = DoNothing>
  void setNodeUserDataEx(scene::node_index node, unsigned user_cmd, int dw_cnt, const int32_t *dw_data,
    ImmediatePerform user_cmd_processsor)
  {
    G_ASSERT(dw_cnt * 4 + 4 <= sizeof(mat44f));
    G_ASSERTF_AND_DO(user_cmd < DeferredCommand::SET_USER_DATA, user_cmd = DeferredCommand::SET_USER_DATA - 1, "user_cmd=0x%X",
      user_cmd);
    std::lock_guard<std::mutex> scopedLock(mDeferredSetTransformMutex);
    if (isInWritingThread() && !mDeferredCommand.size())
    {
      WriteLockRAII lock(*this);
      if (!mDeferredCommand.size())
      {
        int32_t data[16];
        memcpy(data, dw_data, dw_cnt * 4);
        data[15] = dw_cnt;
        if (user_cmd_processsor(user_cmd, *this, node, (const char *)&data))
          return;
      }
    }

    setNodeUserDataDeferred(node, user_cmd, dw_cnt, dw_data);
  }

  int getUserDataWordCount() const { return userDataWordCount; }
  const int32_t *getUserData(scene::node_index ni) const
  {
    if (!userDataWordCount)
      return NULL;
    const uint32_t index = getNodeIndexInternal(ni) * userDataWordCount;
    if (index < nodeUserData.size())
      return &nodeUserData[index];
    return ZERO_PTR<int32_t>();
  }
  void setUserData(scene::node_index ni, const int *data, int word_cnt)
  {
    if (word_cnt > userDataWordCount)
      rearrangeUserData(word_cnt);

    const uint32_t index = TiledScene::getNodeIndex(ni) * userDataWordCount;
    if (index >= nodeUserData.size())
      nodeUserData.resize(index + userDataWordCount, 0);

    if (word_cnt)
      memcpy(&nodeUserData[index], data, word_cnt * 4);
    // if (word_cnt < userDataWordCount)
    //   memset(&nodeUserData[index+word_cnt], 0, (userDataWordCount-word_cnt)*4);
    // debug("getUserData(%d)=%08X", ni, *getUserData(ni));
  }
  void casUserData(scene::node_index ni, const int *old_data, const int *new_data, int word_cnt)
  {
    if (word_cnt > userDataWordCount)
      rearrangeUserData(word_cnt);

    const uint32_t index = TiledScene::getNodeIndex(ni) * userDataWordCount;
    if (index >= nodeUserData.size())
      nodeUserData.resize(index + userDataWordCount, 0);

    if (word_cnt)
      if (memcmp(&nodeUserData[index], old_data, word_cnt * 4) == 0) // compare
        memcpy(&nodeUserData[index], new_data, word_cnt * 4);        // swap
  }
  void rearrangeUserData(int new_word_cnt)
  {
    if (!nodeUserData.size())
    {
      userDataWordCount = new_word_cnt;
      return;
    }

    eastl::vector<int32_t> new_data;
    int cnt = (int)nodeUserData.size() / userDataWordCount;
    new_data.resize(cnt * new_word_cnt, 0);
    for (int i = 0; i < cnt; i++)
      memcpy(&new_data[i * new_word_cnt], &nodeUserData[i * userDataWordCount], userDataWordCount * 4);

    eastl::swap(nodeUserData, new_data);
    userDataWordCount = new_word_cnt;
  }
  void setDirFromSun(const Point3 &nd);

protected:
  void invalidateShadowDist()
  {
    for (auto &di : distance)
      if (di < LIGHTDIST_INVALID)
        di = LIGHTDIST_INVALID;
  }
  friend struct SetSceneUserData;
  template <int N>
  friend class TiledScenesGroup;
  mutable eastl::vector<uint8_t> distance;
  eastl::vector<int32_t> nodeUserData;
  int userDataWordCount = 0;
  Point3 dirFromSunForShadows = {0, 0, 0};
};

struct TiledScenePoolInfo
{
  float distSqLOD[RiExtraPool::MAX_LODS];
  uint32_t lodLimits;
  uint16_t boxOccluder, quadOccluder;
  bool isDynamic;
};

template <int N>
class TiledScenesGroup
{
  RendinstTiledScene mScn[N];
  int mCount = 0;
  eastl::vector<TiledScenePoolInfo> pools;
  eastl::vector<bbox3f> boxOccluders;
  eastl::vector<mat44f> quadOccluders;

  void term()
  {
    pools.clear();
    boxOccluders.clear();
    quadOccluders.clear();
  }

public:
  bbox3f newlyCreatedNodesBox = ZERO<bbox3f>();

public:
  static constexpr int MAX_COUNT = N;
  eastl::vector<TiledScenePoolInfo> &getPools() { return pools; }
  eastl::vector<mat44f> &getQuadOccluders() { return quadOccluders; }
  eastl::vector<bbox3f> &getBoxOccluders() { return boxOccluders; }

  const eastl::vector<TiledScenePoolInfo> &getPools() const { return pools; }
  const eastl::vector<mat44f> &getQuadOccluders() const { return quadOccluders; }
  const eastl::vector<bbox3f> &getBoxOccluders() const { return boxOccluders; }

  ~TiledScenesGroup() { reinit(0); }

  void setWritingThread(int64_t tid)
  {
    for (auto &scn : scenes())
      scn.setWritingThread(tid);
  }

  int size() const { return mCount; }
  int at(const RendinstTiledScene &p) const
  {
    G_ASSERT(&p >= mScn && &p - mScn < mCount);
    return &p - mScn;
  }
  dag::Span<RendinstTiledScene> scenes() { return dag::Span<RendinstTiledScene>(mScn, mCount); }
  dag::Span<RendinstTiledScene> scenes(int fr, int cnt) { return dag::Span<RendinstTiledScene>(mScn + fr, cnt); }
  dag::ConstSpan<RendinstTiledScene> cscenes(int fr, int cnt) const { return make_span_const(mScn + fr, cnt); }
  void reinit(int sz)
  {
    G_ASSERT_RETURN(sz <= N, );
    for (auto &s : scenes())
    {
      if (s.isInWritingThread())
      {
        debug("scene(%d) had %d nodes %d capacity", eastl::distance(mScn, &s), s.getNodesCount(), s.getNodesCapacity());
        s.term();
      }
      else
        execute_delayed_action_on_main_thread(make_delayed_action([&s] { s.term(); }));
    }
    execute_delayed_action_on_main_thread(make_delayed_action([&] { term(); }));
    mCount = sz;
    v_bbox3_init_empty(newlyCreatedNodesBox);
  }
  BBox3 getNewlyCreatedInstBoxAndReset()
  {
    BBox3 box;
    v_stu_bbox3(box, newlyCreatedNodesBox);
    v_bbox3_init_empty(newlyCreatedNodesBox);
    return box;
  }

  inline const RendinstTiledScene &operator[](size_t i) const { return mScn[i]; }
  inline RendinstTiledScene &operator[](size_t i) { return mScn[i]; }
};
enum
{
  DYNAMIC_SCENE = 0,
  STATIC_SCENES_START = 1
}; // first tiled scene is for dynamic objects
typedef TiledScenesGroup<4> RITiledScenes;
extern RITiledScenes riExTiledScenes;
extern float riExTiledSceneMaxDist[RITiledScenes::MAX_COUNT];

struct PerInstanceParameters
{
  uint32_t materialOffset;
  uint32_t matricesOffset;
};

void init_tiled_scenes(const DataBlock *level_blk);
void term_tiled_scenes();
void add_ri_pool_to_tiled_scenes(RiExtraPool &pool, int pool_id, const char *name, float max_lod_dist);
scene::node_index alloc_instance_for_tiled_scene(RiExtraPool &pool, int pool_idx, int inst_idx, mat44f &tm44, vec4f &out_sphere);
void move_instance_in_tiled_scene(RiExtraPool &pool, int pool_idx, scene::node_index &nodeId, mat44f &tm44, bool do_not_wait);
void move_instance_to_original_scene(rendinst::RiExtraPool &pool, int pool_idx, scene::node_index &nodeId);
void set_user_data(rendinst::riex_render_info_t, uint32_t start, const uint32_t *add_data, uint32_t count);
void set_cas_user_data(rendinst::riex_render_info_t, uint32_t start, const uint32_t *was_data, const uint32_t *new_data,
  uint32_t count);
const mat44f &get_tiled_scene_node(rendinst::riex_render_info_t);
void remove_instance_from_tiled_scene(scene::node_index nodeId);
void init_instance_user_data_for_tiled_scene(rendinst::RiExtraPool &pool, scene::node_index ni, vec4f sphere, int add_data_dwords,
  const int32_t *add_data);
void set_instance_user_data(scene::node_index ni, int add_data_dwords, const int32_t *add_data);
const uint32_t *get_user_data(rendinst::riex_handle_t);
} // namespace rendinst

DAG_DECLARE_RELOCATABLE(rendinst::RiExtraPool);
