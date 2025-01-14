//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <scene/dag_scene.h>
#include <scene/dag_kdtree.h>
#include <scene/dag_kdtreeCull.h>
#include <EASTL/fixed_vector.h>
#include <dag/dag_vector.h>
#include <osApiWrappers/dag_rwLock.h>
#include <osApiWrappers/dag_atomic.h>
#include <memory/dag_framemem.h>
#include <atomic>
#include <mutex>

namespace scene
{
class TiledScene;
struct TiledSceneCullContext;
} // namespace scene

#define KD_LEAVES_ONLY 1

class scene::TiledScene : protected SimpleScene
{
public:
  void reserve(int reservedNodesNumber);
  void init(float tileSize);
  void rearrange(float tileSize); // keep current nodes, just rearrange tiled structure
  void term();
  void shrink();

  using SimpleScene::getNode;
  using SimpleScene::getNodeBSphere;
  using SimpleScene::getNodeFlags;
  using SimpleScene::getNodeFromIndex;
  using SimpleScene::getNodeIndex;
  using SimpleScene::getNodePool;
  using SimpleScene::getNodesAliveCount;
  using SimpleScene::getNodesCount;
  using SimpleScene::getPoolBbox;
  using SimpleScene::getPoolDistanceSqScale;
  using SimpleScene::getPoolsCount;
  using SimpleScene::getPoolSphereRad;
  using SimpleScene::getPoolSphereVerticalCenter;
  using SimpleScene::isAliveNode;

  using SimpleScene::checkConsistency;
  using SimpleScene::isInWritingThread;
  using SimpleScene::setWritingThread;

  using SimpleScene::begin;
  using SimpleScene::end;

  using SimpleScene::nodesInRange;

  const SimpleScene &getBaseSimpleScene() const { return *this; }

  struct NoAction
  {
    void operator()(uint32_t ucmd, TiledScene &, node_index, const void *) const { G_VERIFYF(ucmd & 0, "ucmd=%d", ucmd); }
  };
  template <typename T = void, typename F = NoAction>
  void flushDeferredTransformUpdates(F user_cmd_processor = NoAction());

  bool doMaintenance(int64_t reft, unsigned max_time_to_spend_usec);

  float getTileSize() const { return tileSize; }

  void setTransformNoScaleChange(node_index node, mat44f_cref transform); // not updating bounding sphere, i.e. assume transform is and
                                                                          // was without scale, or same uniform scale
  void setTransform(node_index node, mat44f_cref transform, bool do_not_wait = false); // updating bounding sphere from pool,
                                                                                       // potentially slower, and requries pool to be
                                                                                       // valid
  void setFlags(node_index node, uint16_t flags);
  void unsetFlags(node_index node, uint16_t flags);
  void setPoolBBox(pool_index pool, bbox3f_cref box); // will create pool if missing. Requires 'pool' to be less than 65535
  void setPoolDistanceSqScale(pool_index pool, float dist_scale_sq); // will create pool if missing. Requires 'pool' to be less than
                                                                     // 65535
  void destroy(node_index node);
  void reallocate(node_index node, const mat44f &transform, pool_index pool, uint16_t flags); // replace instead of delete, to avoid
                                                                                              // messing with freeNodes
  node_index allocate(const mat44f &transform, pool_index pool, uint16_t flags);

  struct DoNothing
  {
    bool operator()(uint32_t, TiledScene &, node_index, const void *) const { return false; }
  };
  template <typename POD, typename ImmediatePerform = DoNothing> // ImmediatePerform accepting const char* as data payload
  void setNodeUserData(node_index node, unsigned user_cmd, const POD &data, ImmediatePerform user_cmd_processsor = DoNothing());

  void dumpSceneState(bool dump_node_too = true) const;

  template <bool use_flags, bool use_pools, bool use_occlusion, typename VisibleNodesF>
  __forceinline // VisibleNodesF is [](scene::node_index, mat44f_cref, vec4f dist)
    void
    frustumCull(mat44f_cref globtm, vec4f pos_distscale, uint32_t test_flags, uint32_t equal_flags, const Occlusion *occlusion,
      VisibleNodesF visible_nodes) const;

  bool lockForRead() const DAG_TS_TRY_ACQUIRE_SHARED(true, mRWLock)
  {
    if (!isInWritingThread())
    {
      mRWLock.lockRead();
      addReader(1);
      return true;
    }
    return false;
  }

  void unlockAfterRead() const DAG_TS_RELEASE_SHARED(mRWLock)
  {
    addReader(-1);
    mRWLock.unlockRead();
  }

  template <bool use_flags, bool use_pools, bool use_occlusion>
  __forceinline void frustumCullTilesPass(mat44f_cref globtm, vec4f pos_distscale, uint32_t test_flags, uint32_t equal_flags,
    const Occlusion *occlusion, TiledSceneCullContext &out_context) const;

  template <bool use_flags, bool use_pools, bool use_occlusion, typename VisibleNodesF>
  __forceinline // VisibleNodesF is [](scene::node_index, mat44f_cref, vec4f dist)
    void
    frustumCullOneTile(const scene::TiledSceneCullContext &ctx, mat44f_cref globtm, vec4f pos_distscale, const Occlusion *occlusion,
      int tile_idx, VisibleNodesF visible_nodes) const;

  // VisibleNodesFunctor(scene::node_index, mat44f_cref)
  template <bool use_flags, bool use_pools, typename VisibleNodesFunctor>
  void nodesInRange(bbox3f_cref box, uint32_t test_flags, uint32_t equal_flags, VisibleNodesFunctor visible_nodes, int start_index,
    int end_index) const;

  template <bool use_flags, bool use_pools, typename VisibleNodesFunctor> // VisibleNodesFunctor(scene::node_index, mat44f_cref)
  void boxCull(bbox3f_cref box, uint32_t test_flags, uint32_t equal_flags, VisibleNodesFunctor visible_nodes) const;

  // default is min_nodes_count = 16, kdtree_rebuild_threshold = 1.
  void setKdtreeRebuildParams(unsigned min_nodes_count, float kdtree_rebuild_threshold);
  // defualt is min_to_split_by_geom = 8, max_to_split_by_count = 32;
  // min_box_to_split_geom=32.f, max_box_to_split_count = 8.f
  void setKdtreeBuildParams(uint32_t min_to_split_by_geom, uint32_t max_to_split_by_count, float min_box_to_split_geom,
    float max_box_to_split_count);

  int getTileCountInBox(bbox3f_cref box) const;

protected:
  node_index reserveOne();
  void allocateNodeTile(node_index node, uint16_t pool, uint16_t flags);
  node_index allocateImm(node_index node, const mat44f &transform, uint16_t pool, uint16_t flags);
  void reallocateImm(node_index node, const mat44f &transform, uint16_t pool, uint16_t flags);

  void setTransformNoScaleChangeImm(node_index node, mat44f_cref transform);
  void setTransformImm(node_index node, mat44f_cref transform);
  void setFlagsImm(node_index node, uint16_t flags);
  void unsetFlagsImm(node_index node, uint16_t flags);
  void destroyImm(node_index node);
  void applyReserves();
  void initOuter();
  void termTiledStructures();

  uint32_t allocTile(uint32_t tx, uint32_t tz);
  void releaseTile(uint32_t idx);
  void rearrange(int include_tx, int include_tz);
  void rearrange(int tx0, int tz0, int tx1, int tz1);
  inline void posToTile(vec3f pos, int &out_tx, int &out_tz)
  {
    out_tx = (int)floorf((v_extract_x(pos) + 0.5f * tileSize) / tileSize);
    out_tz = (int)floorf((v_extract_z(pos) + 0.5f * tileSize) / tileSize);
  }
  inline int32_t getTileX0() const { return tileGridInfo.x0; }
  inline int32_t getTileZ0() const { return tileGridInfo.z0; }
  inline int32_t getTileX1() const { return tileGridInfo.x0 + tilesDX; }
  inline int32_t getTileZ1() const { return tileGridInfo.z0 + tileGridInfo.z1ofsmax + 1; }
  inline int32_t getTileGridIndex(int tx, int tz) const { return (tz - tileGridInfo.z0) * tilesDX + (tx - tileGridInfo.x0); }
  inline uint32_t getTileIndex(int tx, int tz) const
  {
    int32_t g_idx = getTileGridIndex(tx, tz);
    return (g_idx >= 0 && g_idx < tileGrid.size()) ? tileGrid[g_idx] : INVALID_INDEX;
  }
  inline uint32_t getTileIndexOffseted(uint32_t tx, uint32_t tz) const
  {
    uint32_t g_idx = tx + tz * tilesDX;
    G_FAST_ASSERT(g_idx < tileGrid.size());
    return tileGrid.data()[g_idx];
  }
  inline float getX0() const { return (getTileX0() - 0.5f) * tileSize; }
  inline float getZ0() const { return (getTileZ0() - 0.5f) * tileSize; }

  struct TileCullData
  {
    const node_index *nodes = 0;
    union
    {
      int32_t kdTreeLeftNode;
      int32_t nodesCount = 0;
    }; // may be even better to split it from nodes. If kdtree is there, we don't touch nodes until kdtree vis
#if !KD_LEAVES_ONLY
    int32_t kdTreeRightNode = 0; // can be even uint16_t if offset, but that doesn't saves us any memory
#endif

    __forceinline bool isDead() const { return nodes == 0; }
    void clear();
  };

  struct TileData // not needed during culling, 16 bytes
  {
    dag::Vector<node_index> nodes; // todo: for very static scenes it makes sense to keep all nodes in one array

    float objMaxExt = 0;
    float rebuildPenalty = 0; // if rebuildPenalty is bigger than threshold we have to rebuild kdtree
    uint32_t generation = 0;
    int32_t kdTreeLeftNode = -1;
#if !KD_LEAVES_ONLY
    int32_t kdTreeRightNode = 0; // can be even uint16_t if offset, but that doesn't saves us anu memory
#endif

    uint16_t kdTreeNodeTotalCount = 0;
    uint16_t tileOfsX = 0, tileOfsZ = 0;
    uint16_t maintenanceFlags = 0;
    float getObjMaxExt() const { return objMaxExt; }

    __forceinline int getTileX(int tx0) const { return tx0 + tileOfsX; }
    __forceinline int getTileZ(int tz0) const { return tz0 + tileOfsZ; }
    __forceinline bool isDead() const
    {
      G_ASSERT(generation == 0 || nodes.size() != 0);
      return !generation;
    }
    __forceinline void nextGeneration()
    {
      generation++;
      generation = max((uint32_t)1, generation);
    }

    void clear();

    enum
    {
      MFLG_RECALC_BBOX = 1 << 0,
      MFLG_RECALC_FLAGS = 1 << 1,
      MFLG_REBUILD_KDTREE = 1 << 2,
      MFLG_IVALIDATE_KDTREE = 1 << 3,
      MFLG_ALL = 0xF
    };
  };
  __forceinline void copyTileDataToCull(const TileData &tile); // copy data from TileMainenanceData to TileData

  static constexpr int OUTER_TILE_INDEX = 0U;
  static inline void setKdTreeCountFlags(bbox3f &c, uint32_t v) { c.bmin = v_perm_xyzd(c.bmin, v_cast_vec4f(v_splatsi(v))); }
  static inline uint32_t getKdTreeCountFlags(bbox3f_cref c) { return v_extract_wi(v_cast_vec4i(c.bmin)); }
  static inline uint16_t getKdTreeCount(bbox3f_cref c) { return getKdTreeCountFlags(c) & 0xFFFF; }
  static inline void setKdTreeCount(bbox3f &c, uint16_t count)
  {
    return setKdTreeCountFlags(c, (getKdTreeCountFlags(c) & ~0xFFFF) | count);
  }
  static inline void setTileFlags(bbox3f &c, uint16_t flags)
  {
    return setKdTreeCountFlags(c, (getKdTreeCountFlags(c) & ~0xFFFF0000) | (flags << 16));
  }
  static inline void orTileFlags(bbox3f &c, uint16_t flags) { return setKdTreeCountFlags(c, getKdTreeCountFlags(c) | (flags << 16)); }

  dag::Vector<uint32_t> tileGrid; // tileGrid is tilesX*tilesZ size array, adressing TileData
  // tileBox[].bmin.w = uint32_t kdTreeNodesCount|(union of flags << 16)
  // tileBox[].bmax.w = (maxSphereRad * pool.distScale)^2
  // SoA
  dag::Vector<TileData> tileData;
  dag::Vector<bbox3f> tileBox;        // separate boxes from tiles, as boxes are used to early exit, and are only 32bytes
  dag::Vector<TileCullData> tileCull; // separate tiles from tileData, as tileData is not used in culling
  // tileData[0], tileCull[0] and tileBox[0] serve as outer tile (for too big objects, or objects outside the boundings if boundings
  // limit exhausted)

  uint32_t usedTilesCount = 0, mFirstFreeTile = 0; // to avoid immediate deleting of empty tiles

  dag::Vector<kdtree::KDNode> kdNodes;
#if !KD_LEAVES_ONLY
  dag::Vector<float> kdNodesDistance; // todo: use 16 bit as halves and flags
#endif
  uint32_t usedKdNodes = 0; // for defragmentation

  bool buildKdTree(TileData &tile); // return builded
  void shrinkKdTree(float dynamic_amount);
  void checkSoA() const { G_ASSERT(tileData.size() == tileCull.size() && tileBox.size() == tileCull.size()); }
  // alignas(8) struct
  //{
  float tileSize = 0;
  float gridObjMaxExt = 0;
  float max_dist_scale_sq = 0;
  float min_dist_scale_sq = MAX_REAL;
  //} data;//to keep it in near each other
  // keep it aligned
  union
  {
    vec4i info = ZERO<vec4i>(); // same
    struct
    {
      int32_t x0, z0, x1ofsmax, z1ofsmax;
    };
  } tileGridInfo;
  int32_t tilesDX = 0; // fixme: ugly

  int curFirstTileForMaint = 0, curTilesCountForMaint = 0;
  uint32_t pendingNewNodesCount1 = 0, pendingNewNodesCount2 = 0, pendingNewNodesCount3 = 0;

  struct DeferredCommand
  {
    mat44f transformCommand;
    node_index node;
    enum Cmd
    {
      DESTROY,
      ADD,
      MOVE_FAST,
      MOVE,
      SET_FLAGS,
      UNSET_FLAGS,
      REALLOC,
      SET_POOL_BBOX,
      SET_POOL_DIST,
      SET_USER_DATA = 0x40000000
    } command;
  };

  // Helper class to ensure that counter is written atomically and after command data write
  // (for lockless read in `flushDeferredTransformUpdates`)
  class DeferredCommandVector : public dag::Vector<DeferredCommand>
  {
  public:
    using dag::Vector<DeferredCommand>::Vector;

    DeferredCommand &push_back(...) = delete;    // Use `addCmd` instead
    void *push_back_uninitialized(...) = delete; // Use `addCmd` instead

    template <typename F>
    void addCmd(F cb)
    {
      if (EASTL_UNLIKELY(size() == capacity()))
        DoGrow(GetNewCapacity(size()));
      cb(data()[mCount]);
      interlocked_increment(*(volatile int *)&mCount);
    }
    bool empty() const { return interlocked_acquire_load(*(volatile int *)&mCount) == 0; }
  };
  DeferredCommandVector mDeferredCommand;
  std::mutex mDeferredSetTransformMutex;

  __forceinline uint32_t selectTileIndex(uint32_t n_index, bool allow_add);
  __forceinline void insertNode(TileData &tile, node_index node);
  __forceinline void removeNode(TileData &tile, node_index node);
  __forceinline void updateTileExtents(TileData &tile, mat44f_cref node, bbox3f &wabb);
  __forceinline void updateTileMaxExt(int tidx);
  __forceinline void scheduleTileMaintenance(TileData &tile, uint32_t mflags);
  __forceinline void initTile(TileData &tile);
  __forceinline void updateTile(int t_idx, uint32_t index, const mat44f *old_node);
  __forceinline void fastGrowNodeBox(bbox3f &box, mat44f_cref node);
  template <typename Callable>
  bool doMaintenancePass(int64_t reft, unsigned max_time_to_spend_usec, const Callable &c);

  __forceinline TileData &getTileByIndex(uint32_t t_idx) { return tileData[t_idx]; }
  __forceinline const TileData &getTileByIndex(uint32_t t_idx) const { return tileData[t_idx]; }

  template <bool use_dist, bool use_flags, bool use_pools, bool use_occlusion, typename VisibleNodesF>
  void internalFrustumCull(mat44f_cref globtm, vec3f plane03X, vec3f plane03Y, vec3f plane03Z, const vec3f &plane03W,
    const vec3f &plane47X, const vec3f &plane47Y, const vec3f &plane47Z, const vec3f &plane47W, const vec4f &pos_distscale,
    uint32_t test_flags, uint32_t equal_flags, const Occlusion *occlusion, int regions[4], VisibleNodesF visible_nodes) const;

  template <bool use_dist, bool use_flags, bool use_pools, bool use_occlusion, typename VisibleNodesF>
  __forceinline void internalFrustumCull(bbox3f_cref bbox, const TileCullData &tile, vec3f plane03X, vec3f plane03Y, vec3f plane03Z,
    const vec3f &plane03W, const vec3f &plane47X, const vec3f &plane47Y, const vec3f &plane47Z, const vec3f &plane47W,
    mat44f_cref globtm, const vec4f &pos_distscale, uint32_t test_flags, uint32_t equal_flags, const Occlusion *occlusion,
    VisibleNodesF visible_nodes) const;

  template <bool use_flags, bool use_pools, typename VisibleNodesFunctor> // VisibleNodesFunctor(scene::node_index, mat44f_cref)
  __forceinline void internalBoxCull(bbox3f_cref bbox, const TileCullData &tile, bbox3f_cref cull_box, uint32_t test_flags,
    uint32_t equal_flags, VisibleNodesFunctor visible_nodes) const;
  __forceinline void getBoxRegion(int *__restrict regions, vec4f bmin, vec4f bmax) const;

  static __forceinline bool isEmptyTileMemory(bbox3f_cref bbox) // we assume it is in memory, not real box, so vector ops are slower
  {
    return (((int *)(char *)&bbox)[7] & 0x80000000);
  }
  static __forceinline bool isEmptyTile(bbox3f_cref bbox, const TileCullData &)
  {
#if !_TARGET_SIMD_SSE
    return v_test_vec_x_lt_0(v_splat_w(bbox.bmax));
#else
    return _mm_movemask_ps(bbox.bmax) & 8;
#endif
  }

#if DAGOR_DBGLEVEL > 0
  volatile mutable int readersCount = 0;
  void addReader(int v) const { interlocked_add(readersCount, v); }
  bool debugIsReadingAllowed() const { return interlocked_acquire_load(readersCount) || isInWritingThread(); }
#else
  void addReader(int) const {}
  constexpr bool debugIsReadingAllowed() const { return true; }
#endif

private:
  mutable ReadWriteLock mRWLock;

protected:
  struct WriteLockRAII
  {
    TiledScene &s;
    bool needUnlock;
    bool locked;
    WriteLockRAII(TiledScene &scene, bool do_not_wait = false) : s(scene)
    {
      if (s.writingThread != -1)
      {
        G_ASSERT(s.isInWritingThread());
        if (do_not_wait)
          needUnlock = locked = s.mRWLock.tryLockWrite();
        else
        {
          s.mRWLock.lockWrite();
          needUnlock = locked = true;
        }
      }
      else
      {
        needUnlock = false;
        locked = true;
      }
    }
    ~WriteLockRAII()
    {
      if (needUnlock)
        s.mRWLock.unlockWrite();
    }
  };
  struct ReadLockRAII
  {
    const TiledScene &s;
    bool needUnlock = false;
    ReadLockRAII(const TiledScene &scene) : s(scene), needUnlock(s.lockForRead()) {}
    ~ReadLockRAII()
    {
      if (needUnlock)
        s.unlockAfterRead();
    }
  };

  unsigned min_kdtree_nodes_count = 16;
  float kdtree_rebuild_threshold = 1.0f;
  uint32_t min_to_split_geom = 8, max_to_split_count = 32;
  float min_box_to_split_geom = 32.f, max_box_to_split_count = 8.f;
};

// implementations

template <bool use_dist, bool use_flags, bool use_pools, bool use_occlusion, typename VisibleNodesF>
__forceinline void scene::TiledScene::internalFrustumCull(bbox3f_cref bbox, const TileCullData &tile, vec3f plane03X, vec3f plane03Y,
  vec3f plane03Z, const vec3f &plane03W, const vec3f &plane47X, const vec3f &plane47Y, const vec3f &plane47Z, const vec3f &plane47W,
  mat44f_cref globtm, const vec4f &pos_distscale, uint32_t test_flags, uint32_t equal_flags, const Occlusion *occlusion,
  VisibleNodesF visible_nodes) const
{
  const uint32_t flags_and_kdtreenodes_count = getKdTreeCountFlags(bbox);
  if (use_flags && ((flags_and_kdtreenodes_count & equal_flags) != equal_flags))
    return;
  G_UNUSED(pos_distscale);
  G_FAST_ASSERT(!tile.isDead());
  G_UNUSED(equal_flags);
  G_UNUSED(test_flags); // because we can have no use_pools
  vec4f center = v_bbox3_center(bbox), extent_half = v_sub(bbox.bmax, center);
  // check tile visibility by box
  const int tileVis = v_box_frustum_intersect_extent2(center, extent_half, plane03X, plane03Y, plane03Z, plane03W, plane47X, plane47Y,
    plane47Z, plane47W);
  if (!tileVis)
    return;
  // debug("tileVis = %d" , tileVis);

  // check tile visibility by max sphere radius
  if constexpr (use_dist)
  {
    vec4f sqDistScaled = v_mul_x(v_distance_sq_to_bbox_x(bbox.bmin, bbox.bmax, pos_distscale), v_splat_w(pos_distscale));
    if (v_test_vec_x_lt(v_splat_w(bbox.bmax), sqDistScaled))
      return;
    //
    // vec4f sqMaxDistScaled = v_mul_x(v_max_dist_sq_to_bbox_x(bbox.bmin, bbox.bmax, pos_distscale), v_splat_w(pos_distscale));
    // if (v_test_vec_x_lt(sqDistScaled, v_splat_w(bbox.bmin)))//we need smallest scaled radius than, not bigger
    //  check_dist = false;//none of hbv need to check distance
  }

  if constexpr (use_occlusion)
    if (!occlusion->isVisibleBox(bbox.bmin, bbox.bmax))
      return;
  // float invMinOcclusionSphereRad2 = MAX_REAL;
  // if (use_occlusion)
  //   invMinOcclusionSphereRad2 = (256/0.5f)*(256/0.5f)*v_extract_w(pos_distscale);
  const bool intersectFrustum = (tileVis == Frustum::INTERSECT);

  eastl::fixed_vector<kdtree::VisibleLeaf, 256, true, framemem_allocator> visible; // 2kb size
  const uint16_t kdTreeNodeCount = flags_and_kdtreenodes_count & 0xFFFF;
  if (kdTreeNodeCount)
  {
#if KD_LEAVES_ONLY
    const int32_t kdTreeLeftNode = tile.kdTreeLeftNode;
    G_FAST_ASSERT(kdTreeLeftNode >= 0);
    G_FAST_ASSERT(kdTreeLeftNode + kdTreeNodeCount <= kdNodes.size());
    uint32_t start = 0, count = 0;
    for (int i = kdTreeLeftNode, ei = kdTreeLeftNode + kdTreeNodeCount; i < ei; ++i, start += count)
    {
      const auto kdNode = kdNodes.data()[i];
      const uint32_t flags_nodes_count = v_extract_wi(v_cast_vec4i(kdNode.bmin_start));
      count = flags_nodes_count & 0xFFFF;
#if DAGOR_DBGLEVEL > 1
      G_ASSERTF(start + count <= tileData[&tile - tileCull.data()].nodes.size(),
        "%p, tile=%d start+count = %d nodes = %d kdTreeLeftNode = %d kdTreeNodeCount = %d", this, &tile - tileCull.data(),
        start + count, tileData[&tile - tileCull.data()].nodes.size(), kdTreeLeftNode, kdTreeNodeCount);
#endif

      if constexpr (use_flags)
        if (((flags_nodes_count & equal_flags) != equal_flags)) //-V1051
          continue;
      if constexpr (use_dist)
      {
        vec4f sqDistScaled =
          v_mul_x(v_distance_sq_to_bbox_x(kdNode.getBox().bmin, kdNode.getBox().bmax, pos_distscale), v_splat_w(pos_distscale));
        if (v_test_vec_x_lt(v_splat_w(kdNode.getBox().bmax), sqDistScaled))
          continue;
      }

      int vis = Frustum::INSIDE;
      if (tileVis == Frustum::INTERSECT)
      {
        vec3f center = v_bbox3_center(kdNode.getBox());
        vis = v_box_frustum_intersect_extent2(center, v_sub(kdNode.getBox().bmax, center), plane03X, plane03Y, plane03Z, plane03W,
          plane47X, plane47Y, plane47Z, plane47W);
        if (!vis)
          continue;
      }
      if constexpr (use_occlusion)
        if (!occlusion->isVisibleBox(kdNode.getBox().bmin, kdNode.getBox().bmax))
          continue;
      // const int start = kdNode.getStart(), count = kdNode.getCount();
      const uint32_t fully_inside = (vis == Frustum::INSIDE ? kdtree::fully_inside_node_flag : 0);
      if (visible.size()) // optimize reallocation by collapsing node
      {
        auto &last = visible.back();
        const int lastCount = last.count & (~kdtree::fully_inside_node_flag);
        if ((last.count & kdtree::fully_inside_node_flag) == fully_inside && last.start + lastCount == start)
        {
          last.count = (lastCount + count) | fully_inside;
          continue;
        }
      }
      visible.emplace_back(start, fully_inside | count);
    }
#else
    const int32_t kdTreeLeftNode = tile.kdTreeLeftNode;
    G_FAST_ASSERT(kdTreeLeftNode >= 0);
    const int MAX_KDTREE_DEPTH = 128;                             // todo: do not allow building nodes with greater depth
    kdtree::FastStack<uint32_t, MAX_KDTREE_DEPTH * 2> workingSet; ////1kb size
    workingSet.push(kdTreeLeftNode | (intersectFrustum ? 0 : kdtree::fully_inside_node_flag));
    const int32_t kdTreeRightNode = tile.kdTreeRightNode; // kdTreeLeftNode + tile.kdTreeRightNodeOffset;
    workingSet.push(kdTreeRightNode | (intersectFrustum ? 0 : kdtree::fully_inside_node_flag));
    G_FAST_ASSERT(kdTreeLeftNode + kdTreeNodeCount <= kdNodes.size());
    if constexpr (use_dist)
    {
      G_FAST_ASSERT(kdNodesDistance.size() == kdNodes.size());
      if (use_occlusion)
        kdtree::kd_frustum_visibility<false>(workingSet, plane03X, plane03Y, plane03Z, plane03W, plane47X, plane47Y, plane47Z,
          plane47W, kdNodes.data(),
          // actually AlwaysVisible is almost same speed, due to better cache utilization. consider remove kdnode distance altogether
          kdtree::DistFastCheck(kdNodesDistance.data(), pos_distscale), // kdtree::AlwaysVisible(),
          kdtree::CheckNotOccluded(occlusion), visible);
      else
        kdtree::kd_frustum_visibility<false>(workingSet, plane03X, plane03Y, plane03Z, plane03W, plane47X, plane47Y, plane47Z,
          plane47W, kdNodes.data(),
          // actually AlwaysVisible is almost same speed, due to better cache utilization. consider remove kdnode distance altogether
          kdtree::DistFastCheck(kdNodesDistance.data(), pos_distscale), // kdtree::AlwaysVisible(),
          kdtree::AlwaysVisible(), visible);
    }
    else
    {
      G_ASSERTF(!use_occlusion, "use_dist == off and use_occlusion == on is not implemented (trivial to do)");
      kdtree::kd_frustum_visibility<true>(workingSet, plane03X, plane03Y, plane03Z, plane03W, plane47X, plane47Y, plane47Z, plane47W,
        kdNodes.data(),
        kdtree::AlwaysVisible(), // todo: replace with dist and flags check!
        kdtree::AlwaysVisible(), visible);
    }
#endif
  }
  else
  {
#if DAGOR_DBGLEVEL > 1
    G_ASSERTF(tile.nodesCount == tileData[&tile - tileCull.data()].nodes.size(), "%d != %d", tile.nodesCount,
      tileData[&tile - tileCull.data()].nodes.size());
#endif
    visible.push_back(kdtree::VisibleLeaf(0, (intersectFrustum ? 0 : kdtree::fully_inside_node_flag) | tile.nodesCount));
  }

  for (auto vl : visible)
  {
    const bool intersectFrustum = !(vl.count & kdtree::fully_inside_node_flag);
    const int end = (vl.count & ~kdtree::fully_inside_node_flag) + vl.start;
    bool checkNoIntersection;
    if constexpr (use_dist)
      checkNoIntersection = false;
    else
      checkNoIntersection = !intersectFrustum;
    if (checkNoIntersection)
    {
      for (int i = vl.start; i < end; ++i)
      {
        const node_index ni = tile.nodes[i];
#if DAGOR_DBGLEVEL > 1
        const mat44f &m = getNode(ni);
#else
        const uint32_t index = getNodeIndexInternal(ni);
        const mat44f &m = nodes.data()[index];
#endif

        uint32_t poolFlags = get_node_pool_flags(m);
        if constexpr (use_flags)
          if (((test_flags & poolFlags) != equal_flags))
            continue;
        poolFlags &= 0xFFFF;
        if constexpr (use_occlusion)
        {
          if constexpr (use_pools)
          {
            G_FAST_ASSERT(poolFlags < poolBox.size());
            // narrow check
            mat44f clipTm;
            v_mat44_mul43(clipTm, globtm, m);
            bbox3f pool = poolBox.data()[poolFlags];
            if constexpr (use_occlusion)
            {
              if (!occlusion->isVisibleBox(pool.bmin, pool.bmax, clipTm))
                continue;
            }
          }
          else
          {
            vec4f sphere = get_node_bsphere(m);
            if (!occlusion->isVisibleSphere(sphere, v_splat_w(sphere)))
              continue;
          }
        }
        visible_nodes(ni, m, v_zero());
      }
    }
    else
      for (int i = vl.start; i < end; ++i)
      {
        const node_index ni = tile.nodes[i];
#if DAGOR_DBGLEVEL > 1
        const mat44f &m = getNode(ni);
        G_FAST_ASSERT(!isInvalidIndex(getNodeIndex(ni)));
#else
        const uint32_t index = getNodeIndexInternal(ni);
        const mat44f &m = nodes.data()[index];
#endif

        uint32_t poolFlags = get_node_pool_flags(m);
        if constexpr (use_flags)
          if ((test_flags & poolFlags) != equal_flags)
            continue;
        vec4f sphere = get_node_bsphere(m); // broad phase
        vec4f sphereRad = v_splat_w(sphere);

        int sphereVis = 1;
        if (intersectFrustum) // may be move this if outside loop?
        {
          bool checkSphere;
          if constexpr (use_occlusion)
            checkSphere = true;
          else
            checkSphere = !use_pools;
          if (checkSphere)
            sphereVis =
              v_is_visible_sphere(sphere, sphereRad, plane03X, plane03Y, plane03Z, plane03W, plane47X, plane47Y, plane47Z, plane47W);
          else
            sphereVis =
              v_sphere_intersect(sphere, sphereRad, plane03X, plane03Y, plane03Z, plane03W, plane47X, plane47Y, plane47Z, plane47W);
          if (!sphereVis)
            continue;
        }

        vec3f distToSphereSqScaled;
        if constexpr (use_dist)
        {
          distToSphereSqScaled = v_mul_x(v_length3_sq_x(v_sub(pos_distscale, sphere)), v_splat_w(pos_distscale)); // more correct is
                                                                                                                  // using z-distance,
                                                                                                                  // but it will be
                                                                                                                  // view dependent. We
                                                                                                                  // avoid that by
                                                                                                                  // using eucledian
                                                                                                                  // distance.
          if (v_test_vec_x_lt(v_splat_w(m.col0), distToSphereSqScaled))
            continue;
        }

        if constexpr (use_pools) // can be if poolFlags < poolBox.size(), but less optimal. assume it never happen
        {
          // narrow check
          if (use_occlusion || sphereVis == 2) //(use_dist && v_test_vec_x_lt(v_splats(min_dist_scale_sq), invClipSpaceRadius)))
          {
            poolFlags &= 0xFFFF;
            G_FAST_ASSERT(poolFlags < poolBox.size());
            // precise distance check
            bbox3f pool = poolBox.data()[poolFlags];

            if constexpr (use_occlusion)
            {
              // if sphereVis==1 and use_occlusion, first check sphere screen size (based on radius and distance).
              // It can be, that size is less than occlusion pixel (which is fixed 1./256), and so there is no reason to transform
              // matrix and check anything In practice, however, we try to disappear objects earlier than that if (use_dist &&
              // sphereVis == 1 && v_test_vec_x_lt(v_splats(invMinOcclusionSphereRad2), invClipSpaceRadiusSqScaled))
              //{
              //   if (!occlusion->isVisibleSphere(sphere, sphereRad))
              //     continue;
              // } else
              {
                mat44f clipTm;
                v_mat44_mul43(clipTm, globtm, m);
                if (!occlusion->isVisibleBox(pool.bmin, pool.bmax, clipTm))
                  continue;
              }
            }
            else if (sphereVis == 2)
            {
              mat44f clipTm;
              v_mat44_mul43(clipTm, globtm, m);
              if (!v_is_visible_b_fast_8planes(pool.bmin, pool.bmax, clipTm))
                continue;
            }
          }
        }
        else
        {
          if constexpr (use_occlusion)
            if (!occlusion->isVisibleSphere(sphere, v_splat_w(sphere)))
              continue;
        }
        if constexpr (use_dist)
        {
          visible_nodes(ni, m, distToSphereSqScaled);
        }
        else
        {
          visible_nodes(ni, m, v_zero());
        }
      }
  }
}

template <bool use_dist, bool use_flags, bool use_pools, bool use_occlusion, typename VisibleNodesF>
void scene::TiledScene::internalFrustumCull(mat44f_cref globtm, vec3f plane03X, vec3f plane03Y, vec3f plane03Z, const vec3f &plane03W,
  const vec3f &plane47X, const vec3f &plane47Y, const vec3f &plane47Z, const vec3f &plane47W, const vec4f &pos_distscale,
  uint32_t test_flags, uint32_t equal_flags, const Occlusion *occlusion, int regions[4], VisibleNodesF visible_nodes) const
{
  const int tilesInGrid = (regions[2] - regions[0] + 1) * (regions[3] - regions[1] + 1);
  checkSoA();
  if ((int)tileCull.size() <= tilesInGrid)
  {
    // if there are too much tiles in selected area - iterate over tiles, to avoid indirection
    // it actually happen
    for (int i = OUTER_TILE_INDEX; i < tileCull.size(); ++i)
    {
      if (isEmptyTileMemory(tileBox.data()[i]))
        continue;
      internalFrustumCull<use_dist, use_flags, use_pools, use_occlusion>(tileBox.data()[i], tileCull.data()[i], plane03X, plane03Y,
        plane03Z, plane03W, plane47X, plane47Y, plane47Z, plane47W, globtm, pos_distscale, test_flags, equal_flags, occlusion,
        visible_nodes);
    }
  }
  else
  {
    for (int tz = regions[1]; tz <= regions[3]; ++tz) // otherwise use grid
      for (int tx = regions[0]; tx <= regions[2]; ++tx)
      {
        const uint32_t tileIndex = getTileIndexOffseted(tx, tz);
        if (tileIndex == INVALID_INDEX)
          continue;
        G_FAST_ASSERT(tileIndex < tileCull.size());
        G_FAST_ASSERT(!isEmptyTileMemory(tileBox.data()[tileIndex]));
        internalFrustumCull<use_dist, use_flags, use_pools, use_occlusion>(tileBox.data()[tileIndex], tileCull.data()[tileIndex],
          plane03X, plane03Y, plane03Z, plane03W, plane47X, plane47Y, plane47Z, plane47W, globtm, pos_distscale, test_flags,
          equal_flags, occlusion, visible_nodes);
      }
    if (!isEmptyTileMemory(tileBox.data()[OUTER_TILE_INDEX]))
      internalFrustumCull<use_dist, use_flags, use_pools, use_occlusion>(tileBox.data()[OUTER_TILE_INDEX],
        tileCull.data()[OUTER_TILE_INDEX], plane03X, plane03Y, plane03Z, plane03W, plane47X, plane47Y, plane47Z, plane47W, globtm,
        pos_distscale, test_flags, equal_flags, occlusion, visible_nodes);
  }
  // check outer tile
}

__forceinline void scene::TiledScene::getBoxRegion(int *__restrict regions, vec4f bmin, vec4f bmax) const
{
  vec4f worldBboxXZ = v_perm_xzac(bmin, bmax);
  vec4f extents = v_make_vec4f(-gridObjMaxExt, -gridObjMaxExt, +gridObjMaxExt, +gridObjMaxExt); // fixme: todo: maintain 4-way ext
  extents = v_add(extents, v_splats(0.5f * tileSize));
  worldBboxXZ = v_add(worldBboxXZ, extents);
  vec4f regionV = v_div(worldBboxXZ, v_splats(tileSize));
  vec4i regionI = sse4_cvt_floori(regionV);
  vec4i info = tileGridInfo.info;
  regionI = v_subi(regionI, v_cast_vec4i(v_perm_xyxy(v_cast_vec4f(info)))); // todo: optimize
  regionI = v_mini(v_maxi(regionI, v_cast_vec4i(v_zero())), v_cast_vec4i(v_perm_zwzw(v_cast_vec4f(info))));
  v_sti(regions, regionI);
}

template <bool use_flags, bool use_pools, bool use_occlusion, typename VisibleNodesF>
void scene::TiledScene::frustumCull(mat44f_cref globtm, vec4f pos_distscale, uint32_t test_flags, uint32_t equal_flags,
  const Occlusion *occlusion, VisibleNodesF visible_nodes) const
{
  if constexpr (use_occlusion)
  {
    G_ASSERT_RETURN(occlusion, );
  }
  if (!getNodesAliveCount())
    return;

  ReadLockRAII lock(*this);
  if (tileCull.size() <= 1) // if evrything is in outer tile, or no tiles at all
  {
    SimpleScene::frustumCull<use_flags, use_pools, use_occlusion, VisibleNodesF>(globtm, pos_distscale, test_flags, equal_flags,
      occlusion, visible_nodes);
    return;
  }

  test_flags <<= 16;
  equal_flags <<= 16;
  vec3f plane03X, plane03Y, plane03Z, plane03W;
  vec3f plane47X, plane47Y, plane47Z, plane47W;
  v_construct_camplanes(globtm, plane03X, plane03Y, plane03Z, plane03W, plane47X, plane47Y);
  bbox3f frustumBox;
  v_frustum_box_unsafe(frustumBox, plane03X, plane03Y, plane03Z, plane03W, plane47X, plane47Y);
  plane03X = v_norm3(plane03X);
  plane03Y = v_norm3(plane03Y);
  plane03Z = v_norm3(plane03Z);
  plane03W = v_norm3(plane03W);
  plane47X = v_norm3(plane47X);
  plane47Y = v_norm3(plane47Y);
  v_mat44_transpose(plane03X, plane03Y, plane03Z, plane03W);

  plane47Z = plane47X, plane47W = plane47Y; // we can use some useful planes instead of replicating
  v_mat44_transpose(plane47X, plane47Y, plane47Z, plane47W);
  alignas(16) int regions[4];
  getBoxRegion(regions, frustumBox.bmin, frustumBox.bmax);

  if (v_extract_w(pos_distscale) > 0.f)
    internalFrustumCull<true, use_flags, use_pools, use_occlusion>(globtm, plane03X, plane03Y, plane03Z, plane03W, plane47X, plane47Y,
      plane47Z, plane47W, pos_distscale, test_flags, equal_flags, occlusion, regions, visible_nodes);
  else
  {
    internalFrustumCull<false, use_flags, use_pools, use_occlusion>(globtm, plane03X, plane03Y, plane03Z, plane03W, plane47X, plane47Y,
      plane47Z, plane47W, pos_distscale, test_flags, equal_flags, occlusion, regions, visible_nodes);
  }
}


struct scene::TiledSceneCullContext
{
  vec3f plane03X, plane03Y, plane03Z, plane03W;
  vec3f plane47X, plane47Y, plane47Z, plane47W;
  uint32_t test_flags;
  uint32_t equal_flags;
  std::atomic<uint32_t> tilesPassDone{0u}; // flag, set to 1 when code finishes pushing to tiles
  int *tilesPtr = nullptr;
  uint32_t &totalTilesCount;
  std::atomic<uint32_t> tilesCount{0u};
  uint32_t tilesMax = 0;
  bool use_dist = false;
  bool needToUnlock = false;
  uint16_t wakeUpOnNTiles = USHRT_MAX;
  void *wake_up_ctx = nullptr;
  void (*wake_up_cb)(void *) = [](void *) {};
  mutable std::atomic<uint32_t> nextIdxToProcess{0u}; // index in tiles to be processed

  TiledSceneCullContext(uint32_t &ttc) : totalTilesCount(ttc) {} //-V730
  ~TiledSceneCullContext()
  {
    G_ASSERT(!needToUnlock);
    framemem_ptr()->free(tilesPtr);
  }

  void done()
  {
    G_ASSERTF(tilesCount <= tilesMax, "max=%d count=%d", tilesMax, tilesCount.load());
    tilesPassDone.store(1);
  }
};

template <bool use_flags, bool use_pools, bool use_occlusion>
void scene::TiledScene::frustumCullTilesPass(mat44f_cref globtm, vec4f pos_distscale, uint32_t test_flags, uint32_t equal_flags,
  const Occlusion *occlusion, scene::TiledSceneCullContext &octx) const
{
  if (use_occlusion)
  {
    G_ASSERT_RETURN(occlusion, );
  }
  if (!getNodesAliveCount())
    return octx.done();

  G_ASSERT(debugIsReadingAllowed());

  octx.test_flags = test_flags << 16;
  octx.equal_flags = equal_flags << 16;
  const auto wakeOn = octx.wakeUpOnNTiles;
  uint32_t &totalTilesCount = octx.totalTilesCount;
  auto addTile = [&](int ti) {
    octx.tilesPtr[octx.tilesCount.load(std::memory_order_relaxed)] = ti;
    octx.tilesCount++;
    if (DAGOR_UNLIKELY(++totalTilesCount == wakeOn))
      octx.wake_up_cb(octx.wake_up_ctx);
  };
  if (tileCull.size() <= 1)
  {
    octx.tilesPtr = (int *)framemem_ptr()->alloc(sizeof(int));
    octx.tilesMax = 1;
    addTile(OUTER_TILE_INDEX - 1);
    octx.done();
    return;
  }

  octx.tilesCount = 0;
  octx.tilesPtr = (int *)framemem_ptr()->alloc(sizeof(int) * usedTilesCount);
  octx.tilesMax = usedTilesCount;

  v_construct_camplanes(globtm, octx.plane03X, octx.plane03Y, octx.plane03Z, octx.plane03W, octx.plane47X, octx.plane47Y);
  bbox3f frustumBox;
  v_frustum_box_unsafe(frustumBox, octx.plane03X, octx.plane03Y, octx.plane03Z, octx.plane03W, octx.plane47X, octx.plane47Y);
  octx.plane03X = v_norm3(octx.plane03X);
  octx.plane03Y = v_norm3(octx.plane03Y);
  octx.plane03Z = v_norm3(octx.plane03Z);
  octx.plane03W = v_norm3(octx.plane03W);
  octx.plane47X = v_norm3(octx.plane47X);
  octx.plane47Y = v_norm3(octx.plane47Y);
  v_mat44_transpose(octx.plane03X, octx.plane03Y, octx.plane03Z, octx.plane03W);

  octx.plane47Z = octx.plane47X, octx.plane47W = octx.plane47Y; // we can use some useful planes instead of replicating
  v_mat44_transpose(octx.plane47X, octx.plane47Y, octx.plane47Z, octx.plane47W);

  alignas(16) int regions[4];
  getBoxRegion(regions, frustumBox.bmin, frustumBox.bmax);

  octx.use_dist = v_extract_w(pos_distscale) > 0.f;

  const int tilesInGrid = (regions[2] - regions[0] + 1) * (regions[3] - regions[1] + 1);
  checkSoA();
  if ((int)tileCull.size() <= tilesInGrid)
  {
    // if there are too much tiles in selected area - iterate over tiles, to avoid indirection
    // it actually happen
    for (int i = OUTER_TILE_INDEX; i < tileCull.size(); ++i)
    {
      if (isEmptyTileMemory(tileBox.data()[i]))
        continue;
      addTile(i);
    }
  }
  else
  {
    for (int tz = regions[1]; tz <= regions[3]; ++tz) // otherwise use grid
      for (int tx = regions[0]; tx <= regions[2]; ++tx)
      {
        const uint32_t tileIndex = getTileIndexOffseted(tx, tz);
        if (tileIndex == INVALID_INDEX)
          continue;
        G_FAST_ASSERT(tileIndex < tileCull.size());
        G_FAST_ASSERT(!isEmptyTileMemory(tileBox.data()[tileIndex]));
        addTile(tileIndex);
      }
    if (!isEmptyTileMemory(tileBox.data()[OUTER_TILE_INDEX]))
      addTile(OUTER_TILE_INDEX);
  }
  octx.done();
}

template <bool use_flags, bool use_pools, bool use_occlusion, typename VisibleNodesF>
void scene::TiledScene::frustumCullOneTile(const scene::TiledSceneCullContext &ctx, mat44f_cref globtm, vec4f pos_distscale,
  const Occlusion *occlusion, int tile_idx, VisibleNodesF visible_nodes) const
{
  if (tile_idx == OUTER_TILE_INDEX - 1)
    return SimpleScene::frustumCull<use_flags, use_pools, use_occlusion, VisibleNodesF>(globtm, pos_distscale, ctx.test_flags,
      ctx.equal_flags, occlusion, visible_nodes);

  if (ctx.use_dist)
    internalFrustumCull<true, use_flags, use_pools, use_occlusion>(tileBox.data()[tile_idx], tileCull.data()[tile_idx], ctx.plane03X,
      ctx.plane03Y, ctx.plane03Z, ctx.plane03W, ctx.plane47X, ctx.plane47Y, ctx.plane47Z, ctx.plane47W, globtm, pos_distscale,
      ctx.test_flags, ctx.equal_flags, occlusion, visible_nodes);
  else
    internalFrustumCull<false, use_flags, use_pools, use_occlusion>(tileBox.data()[tile_idx], tileCull.data()[tile_idx], ctx.plane03X,
      ctx.plane03Y, ctx.plane03Z, ctx.plane03W, ctx.plane47X, ctx.plane47Y, ctx.plane47Z, ctx.plane47W, globtm, pos_distscale,
      ctx.test_flags, ctx.equal_flags, occlusion, visible_nodes);
}


template <bool use_flags, bool use_pools, typename VisibleNodesFunctor>
__forceinline void scene::TiledScene::internalBoxCull(bbox3f_cref bbox, const TileCullData &tile, bbox3f_cref cull_box,
  uint32_t test_flags, uint32_t equal_flags, VisibleNodesFunctor visible_nodes) const
{
  const uint32_t flags_and_kdtreenodes_count = getKdTreeCountFlags(bbox);
  if (use_flags && ((flags_and_kdtreenodes_count & equal_flags) != equal_flags))
    return;
  G_FAST_ASSERT(!tile.isDead());
  G_UNUSED(equal_flags);
  G_UNUSED(test_flags); // because we can have no use_pools
  const bbox3f cullBox = cull_box;
  if (!v_bbox3_test_box_intersect(bbox, cullBox))
    return;

#if !KD_LEAVES_ONLY
#error unsupport full kd trees
#endif
  eastl::fixed_vector<kdtree::VisibleLeaf, 256, true> visible; // 2kb size
  const uint16_t kdTreeNodeCount = flags_and_kdtreenodes_count & 0xFFFF;
  const bool tileFullyInside = v_bbox3_test_box_inside(cullBox, bbox);
  if (kdTreeNodeCount)
  {
    const int32_t kdTreeLeftNode = tile.kdTreeLeftNode;
    G_FAST_ASSERT(kdTreeLeftNode >= 0);
    G_FAST_ASSERT(kdTreeLeftNode + kdTreeNodeCount <= kdNodes.size());
    uint32_t start = 0, count = 0;
    if (!tileFullyInside)
    {
      for (int i = kdTreeLeftNode, ei = kdTreeLeftNode + kdTreeNodeCount; i < ei; ++i, start += count)
      {
        const auto kdNode = kdNodes.data()[i];
        const uint32_t flags_nodes_count = v_extract_wi(v_cast_vec4i(kdNode.bmin_start));
        count = flags_nodes_count & 0xFFFF;
#if DAGOR_DBGLEVEL > 1
        G_ASSERTF(start + count <= tileData[&tile - tileCull.data()].nodes.size(),
          "%p, tile=%d start+count = %d nodes = %d kdTreeLeftNode = %d kdTreeNodeCount = %d", this, &tile - tileCull.data(),
          start + count, tileData[&tile - tileCull.data()].nodes.size(), kdTreeLeftNode, kdTreeNodeCount);
#endif

        if (use_flags && ((flags_nodes_count & equal_flags) != equal_flags)) //-V1051
          continue;

        bbox3f kdBox = kdNode.getBox();
        if (!v_bbox3_test_box_intersect(kdBox, cullBox))
          continue;
        // const int start = kdNode.getStart(), count = kdNode.getCount();
        const uint32_t fully_inside = (v_bbox3_test_box_inside(cullBox, kdBox) ? kdtree::fully_inside_node_flag : 0);
        if (visible.size()) // optimize reallocation by collapsing node
        {
          auto &last = visible.back();
          const int lastCount = last.count & (~kdtree::fully_inside_node_flag);
          if ((last.count & kdtree::fully_inside_node_flag) == fully_inside && last.start + lastCount == start)
          {
            last.count = (lastCount + count) | fully_inside;
            continue;
          }
        }
        visible.emplace_back(start, fully_inside | count);
      }
    }
    else
    {
      for (int i = kdTreeLeftNode, ei = kdTreeLeftNode + kdTreeNodeCount; i < ei; ++i, start += count)
      {
        const auto kdNode = kdNodes.data()[i];
        const uint32_t flags_nodes_count = v_extract_wi(v_cast_vec4i(kdNode.bmin_start));
        count = flags_nodes_count & 0xFFFF;
#if DAGOR_DBGLEVEL > 1
        G_ASSERTF(start + count <= tileData[&tile - tileCull.data()].nodes.size(),
          "%p, tile=%d start+count = %d nodes = %d kdTreeLeftNode = %d kdTreeNodeCount = %d", this, &tile - tileCull.data(),
          start + count, tileData[&tile - tileCull.data()].nodes.size(), kdTreeLeftNode, kdTreeNodeCount);
#endif

        if (use_flags && ((flags_nodes_count & equal_flags) != equal_flags)) //-V1051
          continue;

        if (visible.size()) // optimize reallocation by collapsing node
        {
          auto &last = visible.back();
          const int lastCount = last.count & (~kdtree::fully_inside_node_flag);
          if (last.start + lastCount == start)
          {
            last.count = (lastCount + count) | kdtree::fully_inside_node_flag;
            continue;
          }
        }
        visible.emplace_back(start, kdtree::fully_inside_node_flag | count);
      }
    }
  }
  else
  {
#if DAGOR_DBGLEVEL > 1
    G_ASSERTF(tile.nodesCount == tileData[&tile - tileCull.data()].nodes.size(), "%d != %d", tile.nodesCount,
      tileData[&tile - tileCull.data()].nodes.size());
#endif
    // visible.push_back(kdtree::VisibleLeaf(0, (tileFullyInside ? kdtree::fully_inside_node_flag : 0)| tile.nodesCount));
    visible.push_back(kdtree::VisibleLeaf(0,
      (tileFullyInside ? kdtree::fully_inside_node_flag : 0) | uint32_t(tileData[&tile - tileCull.data()].nodes.size())));
  }

  for (auto vl : visible)
  {
    const bool fullyInside = (vl.count & kdtree::fully_inside_node_flag);
    const int end = (vl.count & ~kdtree::fully_inside_node_flag) + vl.start;
    if (fullyInside)
    {
      for (int i = vl.start; i < end; ++i)
      {
        const node_index ni = tile.nodes[i];
#if DAGOR_DBGLEVEL > 1
        const mat44f &m = getNode(ni);
#else
        const uint32_t index = getNodeIndexInternal(ni);
        const mat44f &m = nodes.data()[index];
#endif

        uint32_t poolFlags = get_node_pool_flags(m);
        G_UNUSED(poolFlags);
        if (use_flags && ((test_flags & poolFlags) != equal_flags))
          continue;
        visible_nodes(ni, m);
      }
    }
    else
    {
      for (int i = vl.start; i < end; ++i)
      {
        const node_index ni = tile.nodes[i];
#if DAGOR_DBGLEVEL > 1
        const mat44f &m = getNode(ni);
        G_FAST_ASSERT(!isInvalidIndex(getNodeIndex(ni)));
#else
        const uint32_t index = getNodeIndexInternal(ni);
        const mat44f &m = nodes.data()[index];
#endif

        uint32_t poolFlags = get_node_pool_flags(m);
        if (use_flags && ((test_flags & poolFlags) != equal_flags))
          continue;
        vec4f sphere = get_node_bsphere(m); // broad phase
        vec4f sphereRad = v_splat_w(sphere);
        bbox3f sphereBox;
        sphereBox.bmin = v_sub(sphere, sphereRad);
        sphereBox.bmax = v_add(sphere, sphereRad);
        if (!v_bbox3_test_box_intersect(cullBox, sphereBox))
          continue;

        if (!v_bbox3_test_box_inside(cullBox, sphereBox)) // if !fullyInside
        {
          if (!v_bbox3_test_sph_intersect(cullBox, sphere, v_splat_w(v_mul(sphere, sphere))))
            continue;
          if (use_pools)
          {
            poolFlags &= 0xFFFF;
            if (poolFlags < poolBox.size())
            {
              bbox3f transformed;
              bbox3f pool = poolBox.data()[poolFlags];
              v_bbox3_init(transformed, m, pool);
              if (!v_bbox3_test_box_intersect(transformed, cullBox))
                continue;
            }
          }
        }
        visible_nodes(ni, m);
      }
    }
  }
}

inline int scene::TiledScene::getTileCountInBox(bbox3f_cref box) const
{
  if (!getNodesAliveCount())
    return 0;

  ReadLockRAII lock(*this);

  alignas(16) int regions[4];
  getBoxRegion(regions, box.bmin, box.bmax);

  const int tilesInGrid = (regions[2] - regions[0] + 1) * (regions[3] - regions[1] + 1);
  return eastl::min((int)tileCull.size(), tilesInGrid);
}

// VisibleNodesFunctor(scene::node_index, mat44f_cref)
template <bool use_flags, bool use_pools, typename VisibleNodesFunctor>
void scene::TiledScene::boxCull(bbox3f_cref box, uint32_t test_flags, uint32_t equal_flags, VisibleNodesFunctor visible_nodes) const
{
  nodesInRange<use_flags, use_pools>(box, test_flags, equal_flags, visible_nodes, 0, tileCull.size());
}

// VisibleNodesFunctor(scene::node_index, mat44f_cref)
template <bool use_flags, bool use_pools, typename VisibleNodesFunctor>
void scene::TiledScene::nodesInRange(bbox3f_cref box, uint32_t test_flags, uint32_t equal_flags, VisibleNodesFunctor visible_nodes,
  int start_index, int end_index) const
{
  if (!getNodesAliveCount())
    return;

  ReadLockRAII lock(*this);
  if (tileCull.size() <= 1) // if everything is in outer tile, or no tiles at all
  {
    SimpleScene::boxCull<use_flags, use_pools, VisibleNodesFunctor>(box, test_flags, equal_flags, visible_nodes);
    return;
  }

  test_flags <<= 16;
  equal_flags <<= 16;

  alignas(16) int regions[4];
  getBoxRegion(regions, box.bmin, box.bmax);

  const int tilesInGrid = (regions[2] - regions[0] + 1) * (regions[3] - regions[1] + 1);
  checkSoA();
  if ((int)tileCull.size() <= tilesInGrid)
  {
    // if there are too much tiles in selected area - iterate over tiles, to avoid indirection
    // it actually happen
    for (int i = eastl::max<int>(OUTER_TILE_INDEX, start_index), ie = eastl::min<int>(tileCull.size(), end_index); i < ie; ++i)
    {
      if (isEmptyTileMemory(tileBox.data()[i]))
        continue;
      internalBoxCull<use_flags, use_pools>(tileBox.data()[i], tileCull.data()[i], box, test_flags, equal_flags, visible_nodes);
    }
  }
  else
  {
    int i = 0;
    for (int tz = regions[1]; tz <= regions[3]; ++tz) // otherwise use grid
    {
      if (i >= end_index)
        break;

      for (int tx = regions[0]; tx <= regions[2]; ++tx, ++i)
      {
        if (i < start_index)
          continue;
        if (i >= end_index)
          break;

        const uint32_t tileIndex = getTileIndexOffseted(tx, tz);
        if (tileIndex == INVALID_INDEX)
          continue;
        G_FAST_ASSERT(tileIndex < tileCull.size());
        G_FAST_ASSERT(!isEmptyTileMemory(tileBox.data()[tileIndex]));

        internalBoxCull<use_flags, use_pools>(tileBox.data()[tileIndex], tileCull.data()[tileIndex], box, test_flags, equal_flags,
          visible_nodes);
      }
    }
    if (!isEmptyTileMemory(tileBox.data()[OUTER_TILE_INDEX]))
      internalBoxCull<use_flags, use_pools>(tileBox.data()[OUTER_TILE_INDEX], tileCull.data()[OUTER_TILE_INDEX], box, test_flags,
        equal_flags, visible_nodes);
  }
}

template <typename POD, typename ImmediatePerform>
void scene::TiledScene::setNodeUserData(node_index node, unsigned user_cmd, const POD &data, ImmediatePerform user_cmd_processsor)
{
  G_STATIC_ASSERT(sizeof(POD) <= sizeof(mat44f));
  G_ASSERTF_AND_DO(user_cmd < DeferredCommand::SET_USER_DATA, user_cmd = DeferredCommand::SET_USER_DATA - 1, "user_cmd=0x%X",
    user_cmd);
  std::lock_guard<std::mutex> scopedLock(mDeferredSetTransformMutex);
  if (isInWritingThread() && mDeferredCommand.empty())
  {
    WriteLockRAII lock(*this);
    if (user_cmd_processsor(user_cmd, *this, node, (const char *)&data))
      return;
  }

  mDeferredCommand.addCmd([&](DeferredCommand &cmd) {
    cmd.command = DeferredCommand::Cmd(cmd.SET_USER_DATA + user_cmd);
    cmd.node = node;
    memcpy(&cmd.transformCommand, &data, sizeof(data));
  });
}

template <typename T, typename F>
void scene::TiledScene::flushDeferredTransformUpdates(F user_cmd_processor)
{
  G_ASSERT_RETURN(isInWritingThread(), );
  if (mDeferredCommand.empty()) // Note: atomic read (intentionally without mutex taken)
    return;

  std::lock_guard<std::mutex> scopedLock(mDeferredSetTransformMutex);
  WriteLockRAII lock(*this);
  if (usedTilesCount == 0) // drop commands added after tile data was terminated
  {
    mDeferredCommand.clear();
    return;
  }
  applyReserves();
  for (const DeferredCommand &c : mDeferredCommand)
  {
    switch (c.command)
    {
      case DeferredCommand::ADD:
        allocateImm(c.node, c.transformCommand, get_node_pool(c.transformCommand), get_node_flags(c.transformCommand));
        break;
      case DeferredCommand::DESTROY: destroyImm(c.node); break;
      case DeferredCommand::MOVE_FAST: setTransformNoScaleChangeImm(c.node, c.transformCommand); break;
      case DeferredCommand::MOVE: setTransformImm(c.node, c.transformCommand); break;
      case DeferredCommand::SET_FLAGS: setFlagsImm(c.node, get_node_flags(c.transformCommand)); break;
      case DeferredCommand::UNSET_FLAGS: unsetFlagsImm(c.node, get_node_flags(c.transformCommand)); break;
      case DeferredCommand::REALLOC:
        reallocateImm(c.node, c.transformCommand, get_node_pool(c.transformCommand), get_node_flags(c.transformCommand));
        break;
      case DeferredCommand::SET_POOL_BBOX:
        SimpleScene::setPoolBBox(get_node_pool(c.transformCommand), *(bbox3f *)&c.transformCommand, false);
        break;
      case DeferredCommand::SET_POOL_DIST:
        SimpleScene::setPoolDistanceSqScale(get_node_pool(c.transformCommand), *(float *)&c.transformCommand.col0);
        break;

      default:
        if (c.command >= DeferredCommand::SET_USER_DATA)
        {
          user_cmd_processor(c.command - c.SET_USER_DATA, *this, c.node, (const T *)(char *)&c.transformCommand);
          break;
        }
        G_ASSERTF(0, "mDeferredCommand[%d].command=%d", eastl::distance(mDeferredCommand.cbegin(), &c), c.command);
    }
  }

  // this allows to save up to 80MB for mobiles
  if (mDeferredCommand.capacity() > 256)
  {
    decltype(mDeferredCommand) newCommands;
    newCommands.reserve(256);
    newCommands.swap(mDeferredCommand);
  }
  mDeferredCommand.clear();
  G_ASSERT(tileData[OUTER_TILE_INDEX].isDead() == tileCull[OUTER_TILE_INDEX].isDead());
  G_ASSERT(tileData[OUTER_TILE_INDEX].isDead() == isEmptyTile(tileBox[OUTER_TILE_INDEX], tileCull[OUTER_TILE_INDEX]));
}
