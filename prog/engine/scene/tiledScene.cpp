// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <vecmath/dag_vecMath.h>
#include <scene/dag_tiledScene.h>
#include <startup/dag_globalSettings.h>
#include <math/dag_bounds2.h>
#include <debug/dag_debug.h>
#include <memory/dag_framemem.h>
#include <util/dag_stlqsort.h>
#include <perfMon/dag_cpuFreq.h>
#include <math/dag_vecMathCompatibility.h>

static inline void encodeCommandPoolFlags(mat44f &m, uint32_t pf) { ((uint32_t *)(char *)&m.col2)[3] = pf; }

// default is min_kdtree_nodes_count = 16, kdtree_rebuild_threshold = 1.
void scene::TiledScene::setKdtreeRebuildParams(unsigned min_kdtree_nodes_count_, float kdtree_rebuild_threshold_)
{
  min_kdtree_nodes_count = max(min_kdtree_nodes_count_, (unsigned)2);
  kdtree_rebuild_threshold = max(kdtree_rebuild_threshold_, 0.f);
}

void scene::TiledScene::setKdtreeBuildParams(uint32_t min_to_split_by_geom, uint32_t max_to_split_by_count,
  float min_box_to_split_geom_, float max_box_to_split_count_)
{
  min_to_split_geom = min_to_split_by_geom;
  max_to_split_count = max_to_split_by_count;
  min_box_to_split_geom = min_box_to_split_geom_;
  max_box_to_split_count = max_box_to_split_count_;
}

void scene::TiledScene::reserve(int reservedNodesNumber)
{
  G_ASSERT(isInWritingThread());
  WriteLockRAII lock(*this);
  SimpleScene::reserve(reservedNodesNumber);
}

void scene::TiledScene::initOuter()
{
  if (tileCull.size() <= OUTER_TILE_INDEX)
  {
    tileCull.resize(OUTER_TILE_INDEX + 1);
    tileData.resize(OUTER_TILE_INDEX + 1);
    tileBox.resize(OUTER_TILE_INDEX + 1);
  }
  usedTilesCount = mFirstFreeTile = OUTER_TILE_INDEX + 1;
  v_bbox3_init_empty(tileBox[OUTER_TILE_INDEX]);
  tileBox[OUTER_TILE_INDEX].bmin = v_perm_xyzd(tileBox[OUTER_TILE_INDEX].bmin, v_zero());
  tileCull[OUTER_TILE_INDEX].clear();
  tileData[OUTER_TILE_INDEX].clear();
}

void scene::TiledScene::init(float tile_sz)
{
  term(); // todo: instead of term rearrange all tiles

  G_ASSERT(isInWritingThread());
  setWritingThread(get_current_thread_id());

  WriteLockRAII lock(*this);
  initOuter();
  tileSize = tile_sz;
}

void scene::TiledScene::termTiledStructures()
{
  tileGrid.clear();
  tileCull.clear();
  tileBox.clear();
  tileData.clear();
  kdNodes.clear();
#if !KD_LEAVES_ONLY
  kdNodesDistance.clear();
#endif
  usedKdNodes = 0;
  usedTilesCount = mFirstFreeTile = 0;
  max_dist_scale_sq = 0;
  min_dist_scale_sq = MAX_REAL;
  tileGridInfo.info = v_cast_vec4i(v_zero());
  gridObjMaxExt = 0;
  tilesDX = 0;
  curFirstTileForMaint = curTilesCountForMaint = 0;
}

void scene::TiledScene::term()
{
  G_ASSERT(isInWritingThread());
  WriteLockRAII lock(*this);

  SimpleScene::term();
  termTiledStructures();

  mDeferredCommand.clear();
  pendingNewNodesCount1 = pendingNewNodesCount2 = pendingNewNodesCount3 = 0;
}

void scene::TiledScene::TileCullData::clear()
{
  nodes = 0;
  nodesCount = 0;
#if !KD_LEAVES_ONLY
  kdTreeRightNode = 0;
#endif
}

void scene::TiledScene::TileData::clear()
{
  generation = 0;
  rebuildPenalty = 0;
  objMaxExt = 0;
  kdTreeLeftNode = -1;
  kdTreeNodeTotalCount = 0;
  nodes.clear();
}

uint32_t scene::TiledScene::allocTile(uint32_t tx, uint32_t tz)
{
  G_ASSERT(tx <= 0xFFFF && tz <= 0xFFFF);
  G_ASSERT(mFirstFreeTile >= 1);
  checkSoA();

  if (tileData.size() > usedTilesCount)
    for (int i = mFirstFreeTile; i < tileData.size(); i++)
      if (tileData[i].isDead())
      {
        tileData[i].tileOfsX = tx;
        tileData[i].tileOfsZ = tz;
        mFirstFreeTile = i + 1;
        usedTilesCount++;
        return i;
      }
  G_ASSERT(usedTilesCount == tileData.size());
  tileCull.resize(usedTilesCount + 1);
  tileData.resize(usedTilesCount + 1);
  tileBox.resize(usedTilesCount + 1);
  tileData[usedTilesCount].tileOfsX = tx;
  tileData[usedTilesCount].tileOfsZ = tz;
  usedTilesCount++;
  mFirstFreeTile = usedTilesCount;
  return usedTilesCount - 1;
}

void scene::TiledScene::releaseTile(uint32_t idx)
{
  checkSoA();
  G_ASSERT_RETURN(idx != OUTER_TILE_INDEX, );
  G_ASSERTF(idx < tileData.size(), "idx=%d tiles.size()=%d", idx, tileData.size());
  int32_t gidx = getTileGridIndex(tileData[idx].getTileX(tileGridInfo.x0), tileData[idx].getTileZ(tileGridInfo.z0));
  G_ASSERTF(gidx >= 0 && gidx < tileGrid.size(), "gidx=%d tileGrid.size()=%d", gidx, tileGrid.size());
  tileGrid[gidx] = INVALID_INDEX;
  tileCull[idx].clear();
  tileData[idx].clear();
  v_bbox3_init_empty(tileBox[idx]);
  mFirstFreeTile = min(mFirstFreeTile, idx);
  usedTilesCount--;
  G_ASSERT(mFirstFreeTile >= 1);
  G_ASSERT(usedTilesCount >= 1);
}

void scene::TiledScene::shrink() {}

void scene::TiledScene::rearrange(int include_tx, int include_tz)
{
  int tx0 = getTileX0(), tz0 = getTileZ0(), tx1 = getTileX1(), tz1 = getTileZ1();
  if (tx0 > include_tx)
    tx0 = include_tx;
  else if (tx1 <= include_tx)
    tx1 = include_tx + 1;
  if (tz0 > include_tz)
    tz0 = include_tz;
  else if (tz1 <= include_tz)
    tz1 = include_tz + 1;
  rearrange(tx0, tz0, tx1, tz1);
}

void scene::TiledScene::rearrange(int tx0, int tz0, int tx1, int tz1)
{
  G_ASSERTF_RETURN(isInWritingThread(), , "%s", "not in writing thread");
  G_ASSERTF(tx1 > tx0 && tz1 > tz0, "(%d,%d)-(%d,%d)", tx0, tz0, tx1, tz1);
  G_ASSERTF((tx1 - tx0) < 0xFFFF && (tz1 - tz0) < 0xFFFF, "(%d,%d)-(%d,%d) dim=%dx%d cells", tx0, tz0, tx1, tz1, (tx1 - tx0),
    (tz1 - tz0));

  dag::Vector<uint32_t> new_tileGrid;
  new_tileGrid.resize((tx1 - tx0) * (tz1 - tz0), INVALID_INDEX);
  const int newTilesDX = (tx1 - tx0);
  checkSoA();
  if (tileData.size() > 1)
  {
    // for (auto &tile : tiles)
    for (int i = 1; i < tileData.size(); ++i)
    {
      G_ASSERTF(tileCull.data()[i].isDead() == tileData.data()[i].isDead(), "tile %d %d", tileCull.data()[i].isDead(),
        tileData.data()[i].isDead());
      auto &tile = tileData.data()[i];
      if (tile.isDead())
        continue;
      const int x = tile.getTileX(getTileX0()), z = tile.getTileZ(getTileZ0());
      const int idx = eastl::distance(tileData.data(), &tile);
      if (x < tx0 || z < tz0 || x >= tx1 || z >= tz1)
      {
        releaseTile(idx);
        continue;
      }
      G_ASSERT((z - tz0) <= 0xFFFF && (x - tx0) <= 0xFFFF);
      tile.tileOfsZ = (z - tz0);
      tile.tileOfsX = (x - tx0);
      new_tileGrid[tile.tileOfsZ * newTilesDX + tile.tileOfsX] = idx;
    }
  }

  tileGrid = eastl::move(new_tileGrid);

  tileGridInfo.x0 = tx0, tileGridInfo.z0 = tz0;
  tileGridInfo.x1ofsmax = tx1 - tx0 - 1;
  tileGridInfo.z1ofsmax = tz1 - tz0 - 1;
  G_ASSERT(tileGridInfo.x1ofsmax >= 0 && tileGridInfo.z1ofsmax >= 0);
  tilesDX = newTilesDX;
}

void scene::TiledScene::setTransformNoScaleChange(node_index node, mat44f_cref transform)
{
  std::lock_guard<std::mutex> scopedLock(mDeferredSetTransformMutex);
  if (isInWritingThread() && mDeferredCommand.empty())
  {
    WriteLockRAII lock(*this);
    if (mDeferredCommand.empty())
      return setTransformNoScaleChangeImm(node, transform);
  }

  auto prev = eastl::find_if(mDeferredCommand.begin(), mDeferredCommand.end(), [node](const DeferredCommand &c) {
    return c.node == node && (c.command == DeferredCommand::MOVE || c.command == DeferredCommand::MOVE_FAST);
  });

  if (prev == mDeferredCommand.end())
    mDeferredCommand.addCmd([&](DeferredCommand &cmd) {
      cmd.command = DeferredCommand::MOVE_FAST;
      cmd.transformCommand = transform;
      cmd.node = node;
    });
  else
    prev->transformCommand = transform;
}

void scene::TiledScene::setTransform(node_index node, mat44f_cref transform, bool do_not_wait)
{
  std::lock_guard<std::mutex> scopedLock(mDeferredSetTransformMutex);
  if (isInWritingThread() && mDeferredCommand.empty())
  {
    WriteLockRAII lock(*this, do_not_wait);
    if (lock.locked)
      return setTransformImm(node, transform);
  }

  auto prev = eastl::find_if(mDeferredCommand.begin(), mDeferredCommand.end(), [node](const DeferredCommand &c) {
    return c.node == node && (c.command == DeferredCommand::MOVE || c.command == DeferredCommand::MOVE_FAST);
  });
  if (prev == mDeferredCommand.end())
    mDeferredCommand.addCmd([&](DeferredCommand &cmd) {
      cmd.command = DeferredCommand::MOVE;
      cmd.transformCommand = transform;
      cmd.node = node;
    });
  else
    prev->transformCommand = transform;
}

void scene::TiledScene::setFlags(node_index node, uint16_t flags)
{
  std::lock_guard<std::mutex> scopedLock(mDeferredSetTransformMutex);
  if (isInWritingThread() && mDeferredCommand.empty())
  {
    WriteLockRAII lock(*this);
    return setFlagsImm(node, flags);
  }

  mDeferredCommand.addCmd([&](DeferredCommand &cmd) {
    cmd.command = DeferredCommand::SET_FLAGS;
    get_node_pool_flags_ref(cmd.transformCommand) = 0;
    set_node_flags(cmd.transformCommand, flags);
    cmd.node = node;
  });
}

void scene::TiledScene::unsetFlags(node_index node, uint16_t flags)
{
  std::lock_guard<std::mutex> scopedLock(mDeferredSetTransformMutex);
  if (isInWritingThread() && mDeferredCommand.empty())
  {
    WriteLockRAII lock(*this);
    return unsetFlagsImm(node, flags);
  }

  mDeferredCommand.addCmd([&](DeferredCommand &cmd) {
    cmd.command = DeferredCommand::UNSET_FLAGS;
    get_node_pool_flags_ref(cmd.transformCommand) = 0;
    set_node_flags(cmd.transformCommand, flags);
    cmd.node = node;
  });
}

void scene::TiledScene::unsetFlagsUnordered(dag::ConstSpan<node_index> node_indices, uint16_t flags)
{
  G_ASSERT_RETURN(isInWritingThread(), );
  WriteLockRAII lock(*this);
  for (node_index node_idx : node_indices)
    unsetFlagsImm(node_idx, flags);
}

void scene::TiledScene::setPoolBBox(pool_index pool, bbox3f_cref box)
{
  std::lock_guard<std::mutex> scopedLock(mDeferredSetTransformMutex);
  if (isInWritingThread() && mDeferredCommand.empty())
  {
    WriteLockRAII lock(*this);
    if (mDeferredCommand.empty())
      return SimpleScene::setPoolBBox(pool, box, false);
  }

  mDeferredCommand.addCmd([&](DeferredCommand &cmd) {
    cmd.command = DeferredCommand::SET_POOL_BBOX;
    cmd.transformCommand.col0 = box.bmin;
    cmd.transformCommand.col1 = box.bmax;
    encodeCommandPoolFlags(cmd.transformCommand, pool);
    cmd.node = INVALID_NODE;
  });
}

void scene::TiledScene::setPoolDistanceSqScale(pool_index pool, float dist_scale_sq)
{
  std::lock_guard<std::mutex> scopedLock(mDeferredSetTransformMutex);
  if (isInWritingThread() && mDeferredCommand.empty())
  {
    WriteLockRAII lock(*this);
    if (mDeferredCommand.empty())
      return SimpleScene::setPoolDistanceSqScale(pool, dist_scale_sq);
  }

  mDeferredCommand.addCmd([&](DeferredCommand &cmd) {
    cmd.command = DeferredCommand::SET_POOL_DIST;
    *(float *)(char *)&cmd.transformCommand.col0 = dist_scale_sq;
    encodeCommandPoolFlags(cmd.transformCommand, pool);
    cmd.node = INVALID_NODE;
  });
}

scene::node_index scene::TiledScene::reserveOne()
{
  uint32_t index;
  if (firstAlive > pendingNewNodesCount1)
  {
    pendingNewNodesCount1++;
    index = firstAlive - pendingNewNodesCount1;
  }
  else if (freeIndices.size() > pendingNewNodesCount2)
  {
    pendingNewNodesCount2++;
    index = freeIndices[freeIndices.size() - pendingNewNodesCount2];
  }
  else
  {
    index = uint32_t(nodes.size()) + pendingNewNodesCount3;
    pendingNewNodesCount3++;
  }
  return getNodeFromIndex(index);
}
void scene::TiledScene::applyReserves()
{
  if (pendingNewNodesCount1)
  {
    G_ASSERTF(firstAlive >= pendingNewNodesCount1, "firstAlive=%d pendingNewNodesCount1=%d", firstAlive, pendingNewNodesCount1);
    firstAlive -= pendingNewNodesCount1;
    pendingNewNodesCount1 = 0;
  }
  G_ASSERTF(freeIndices.size() >= pendingNewNodesCount2, "freeIndices.size=%d pendingNewNodesCount2=%d", freeIndices.size(),
    pendingNewNodesCount2);
  for (; pendingNewNodesCount2; pendingNewNodesCount2--)
  {
    uint32_t index = freeIndices.back();
    freeIndices.pop_back();
    G_ASSERT(index > firstAlive && index < nodes.size());
  }
  if (pendingNewNodesCount3)
  {
    nodes.resize(nodes.size() + pendingNewNodesCount3);
#if KEEP_GENERATIONS
    if (generations.size() < nodes.size())
      generations.resize(nodes.size(), 0);
#endif
    pendingNewNodesCount3 = 0;
  }
}

void scene::TiledScene::destroy(node_index node)
{
  std::lock_guard<std::mutex> scopedLock(mDeferredSetTransformMutex);
  if (isInWritingThread() && mDeferredCommand.empty())
  {
    WriteLockRAII lock(*this);
    if (mDeferredCommand.empty())
      return destroyImm(node);
  }

  mDeferredCommand.addCmd([&](DeferredCommand &cmd) {
    cmd.command = DeferredCommand::DESTROY;
    cmd.node = node;
  });
}

void scene::TiledScene::reallocate(node_index node, const mat44f &transform, pool_index pool, uint16_t flags)
{
  std::lock_guard<std::mutex> scopedLock(mDeferredSetTransformMutex);
  if (isInWritingThread() && mDeferredCommand.empty())
  {
    WriteLockRAII lock(*this);
    if (mDeferredCommand.empty())
      return reallocateImm(node, transform, pool, flags);
  }

  mDeferredCommand.addCmd([&](DeferredCommand &cmd) {
    cmd.command = DeferredCommand::REALLOC;
    cmd.node = node;
    cmd.transformCommand = transform;
    encodeCommandPoolFlags(cmd.transformCommand, (flags << 16) | pool);
  });
}

scene::node_index scene::TiledScene::allocate(const mat44f &transform, pool_index pool, uint16_t flags)
{
  std::lock_guard<std::mutex> scopedLock(mDeferredSetTransformMutex);
  if (isInWritingThread() && mDeferredCommand.empty())
  {
    WriteLockRAII lock(*this);
    if (mDeferredCommand.empty())
      return allocateImm(allocateOne(), transform, pool, flags);
  }

  auto node = reserveOne();

  mDeferredCommand.addCmd([&](DeferredCommand &cmd) {
    cmd.command = DeferredCommand::ADD;
    cmd.node = node;
    cmd.transformCommand = transform;
    encodeCommandPoolFlags(cmd.transformCommand, (flags << 16) | pool);
  });

  return node;
}

static inline void updateMaxExt(float *tile_bbox, float &objMaxExt, int tx, int tz, float tileSize)
{
  float x0 = (tx - 0.5f) * tileSize;
  float z0 = (tz - 0.5f) * tileSize;
  objMaxExt = max(0.0f, x0 - tile_bbox[0]);
  inplace_max(objMaxExt, z0 - tile_bbox[2]);
  inplace_max(objMaxExt, tile_bbox[4] - x0 - tileSize);
  inplace_max(objMaxExt, tile_bbox[6] - z0 - tileSize);
}

__forceinline uint32_t scene::TiledScene::selectTileIndex(uint32_t n_index, bool allow_add)
{
  if (get_node_bsphere_rad(nodes[n_index]) * 2 > tileSize) // heuristics to determ if we should fit in outer scene
    return OUTER_TILE_INDEX;

  int tx = 0, tz = 0;
  posToTile(nodes.data()[n_index].col3, tx, tz);
  if (tx < getTileX0() || tz < getTileZ0() || tx >= getTileX1() || tz >= getTileZ1())
  {
    G_ASSERTF(allow_add, "pos=%f %f %f %f-> tile=%d,%d is outside grid (%d,%d)-(%d,%d) (node=%08X)", V4D(nodes.data()[n_index].col3),
      tx, tz, getTileX0(), getTileZ0(), getTileX1(), getTileZ1(), n_index);
    rearrange(tx, tz);
  }
  uint32_t tidx = getTileIndex(tx, tz);
  if (tidx == INVALID_INDEX)
  {
    G_ASSERTF(allow_add, "pos=%f %f %f %f -> tile=%d,%d references invalid tile (node=%08X)", V4D(nodes.data()[n_index].col3), tx, tz,
      n_index);
    tidx = tileGrid[getTileGridIndex(tx, tz)] = allocTile(tx - getTileX0(), tz - getTileZ0());
  }
  return tidx;
}

__forceinline void scene::TiledScene::scheduleTileMaintenance(TileData &tile, uint32_t mflags)
{
  checkSoA();
  if (mflags & tile.MFLG_IVALIDATE_KDTREE)
  {
    bbox3f &tbox = tileBox[eastl::distance(tileData.begin(), &tile)];
    setKdTreeCount(tbox, 0);
    mflags &= ~tile.MFLG_IVALIDATE_KDTREE;
  }
  if (!tile.isDead())
  {
    if (!tile.maintenanceFlags)
    {
      int t_idx = eastl::distance(tileData.data(), &tile);
      if (!curTilesCountForMaint)
        curFirstTileForMaint = t_idx;
      else if (curFirstTileForMaint > t_idx)
        curFirstTileForMaint = t_idx;
      curTilesCountForMaint++;
    }
    tile.maintenanceFlags |= mflags;
  }
  else if (tile.maintenanceFlags)
  {
    tile.maintenanceFlags = 0;
    curTilesCountForMaint--;
  }
  copyTileDataToCull(tile);
}

__forceinline void scene::TiledScene::initTile(TileData &tdata)
{
  bbox3f &tbox = tileBox[eastl::distance(tileData.begin(), &tdata)];
  G_ASSERTF(tdata.nodes.size() == 1, "todo: implement array init");
  G_ASSERT_RETURN(tdata.nodes.size() > 0, );
  uint32_t index = getNodeIndex(tdata.nodes[0]);
  G_ASSERT_RETURN(index != INVALID_INDEX, );
  const auto &m = nodes[index];
  vec4f bsph = get_node_bsphere(m);
  tbox.bmin = v_sub(bsph, v_splat_w(bsph));
  tbox.bmax = v_add(bsph, v_splat_w(bsph));
  // tile.flags = get_node_flags(m);
  setKdTreeCountFlags(tbox, get_node_flags(m) << 16);
  tbox.bmax = v_perm_xyzd(tbox.bmax, m.col0);
  tdata.maintenanceFlags = 0; // it is just inited
  copyTileDataToCull(tdata);
}

__forceinline void scene::TiledScene::copyTileDataToCull(const TileData &tdata)
{
  const int idx = eastl::distance(tileData.cbegin(), &tdata);
  auto &tcull = tileCull[idx];
  const bbox3f &tbox = tileBox[idx];
  tcull.nodes = tdata.nodes.data();
  if (getKdTreeCount(tbox))
    tcull.kdTreeLeftNode = tdata.kdTreeLeftNode;
  else
    tcull.nodesCount = (uint32_t)tdata.nodes.size();
  tcull.maintenanceFlags = tdata.maintenanceFlags;
#if !KD_LEAVES_ONLY
  tcull.kdTreeRightNode = tdata.kdTreeRightNode;
#endif
}

__forceinline void scene::TiledScene::insertNode(TileData &tdata, node_index node)
{
  tdata.nodes.push_back(node);
  if (tdata.nodes.size() == 1)
    initTile(tdata);
  else
  {
    scheduleTileMaintenance(tdata, tdata.MFLG_REBUILD_KDTREE | tdata.MFLG_IVALIDATE_KDTREE);
    // todo: we can find best kdnode to insert node, or create new node and increase penalty
  }
}

static inline void box_from_tree_leaves(bbox3f &tBox, const kdtree::KDNode *i, const kdtree::KDNode *end)
{
  bbox3f newtBox;
  v_bbox3_init_empty(newtBox);
  for (; i != end; ++i)
    v_bbox3_add_box(newtBox, i->getBox());
  tBox.bmin = v_perm_xyzd(newtBox.bmin, tBox.bmin);
  tBox.bmax = newtBox.bmax;
}

__forceinline void scene::TiledScene::removeNode(TileData &tdata, node_index node)
{
  auto it = eastl::find(tdata.nodes.begin(), tdata.nodes.end(), node);
  G_ASSERTF_RETURN(it != tdata.nodes.end(), , "missing node=%08X in tile=%d,%d", node, tdata.getTileX(getTileX0()),
    tdata.getTileZ(getTileZ0()));
  int idx = eastl::distance(tileData.data(), &tdata);
  bbox3f &tBox = tileBox[idx];
  const uint32_t kdTreeNodeCount = getKdTreeCount(tBox);
  bbox3f oldWabb = calcNodeBox(getNode(*it));

  uint32_t flags = (tdata.maintenanceFlags & tdata.MFLG_RECALC_BBOX) ? 0 : tdata.MFLG_RECALC_BBOX;


  if (flags & tdata.MFLG_RECALC_BBOX)
  {
    vec4f border = v_or(v_cmp_ge(tBox.bmin, oldWabb.bmin), v_cmp_ge(oldWabb.bmax, tBox.bmax)); // cmp_eq should be enough, however it
                                                                                               // doesn't matter and safer
    const int32_t isOnTileBorder = v_signmask(border) & 7;
    if (!isOnTileBorder)
      flags &= ~tdata.MFLG_RECALC_BBOX;
  }

  if (kdTreeNodeCount) // if there is kdtree
  {
    if (tdata.nodes.size() < min_kdtree_nodes_count)
      flags |= tdata.MFLG_IVALIDATE_KDTREE;
    else
    {
#if KD_LEAVES_ONLY
      const int tileLocation = eastl::distance(tdata.nodes.begin(), it);
      uint32_t start = 0, count = 0, i = tdata.kdTreeLeftNode, ei = i + kdTreeNodeCount;
      for (; i < ei; ++i, start += count)
      {
        count = v_extract_wi(v_cast_vec4i(kdNodes.data()[i].bmin_start)) & 0xFFFF;
        if (tileLocation < start + count)
          break;
      }
      G_ASSERT(i < ei);
      if (i < ei)
      {
        // int ndi = 0;
        // for (auto nodeIt = &tdata.nodes[start], end = nodeIt+count; nodeIt != end; ++nodeIt, ++ndi)
        //   debug("%d: node %d (%d), ", ndi, *nodeIt, getNodeIndex(*nodeIt));
        if (count == 1) // last node in kdtree
        {
          setKdTreeCount(tBox, kdTreeNodeCount - 1);
          memmove(kdNodes.data() + i, kdNodes.data() + (i + 1), (ei - i - 1) * sizeof(kdNodes[i]));
          if (kdTreeNodeCount == 1)
            flags |= tdata.MFLG_IVALIDATE_KDTREE;
        }
        else
        {
          auto &kdNode = kdNodes[i];
          uint32_t flags_count = v_extract_wi(v_cast_vec4i(kdNode.bmin_start));
          flags_count = (flags_count & 0xFFFF0000) | (count - 1);
          if (!(tdata.maintenanceFlags & tdata.MFLG_REBUILD_KDTREE))
          {
            vec4f border =
              v_or(v_cmp_ge(kdNodes[i].getBox().bmin, oldWabb.bmin), v_cmp_ge(oldWabb.bmax, kdNodes[i].getBox().bmax)); // cmp_eq
                                                                                                                        // should be
                                                                                                                        // enough,
                                                                                                                        // however it
                                                                                                                        // doesn't
                                                                                                                        // matter and
                                                                                                                        // safer
            const int32_t isOnKdNodeBorder = v_signmask(border) & 7;
            if (!isOnKdNodeBorder)
            {
              // fastest case, simply erase node from kdtree
              // assume flags in not changed
            }
            else
            {
              // recalc kdleaf
              bbox3f nKdbox;
              v_bbox3_init_empty(nKdbox);
              uint32_t flags = 0;
              for (auto nodeIt = &tdata.nodes[start], end = nodeIt + count; nodeIt != end; ++nodeIt)
                if (nodeIt != it)
                {
                  const auto &m = getNodeInternal(*nodeIt);
                  flags |= get_node_pool_flags(m);
                  fastGrowNodeBox(nKdbox, m);
                }
              G_ASSERT(v_bbox3_test_box_inside(kdNode.getBox(), nKdbox));
              flags_count = (flags_count & 0xFFFF) | (flags & 0xFFFF0000);
              kdNode.bmin_start = v_perm_xyzd(nKdbox.bmin, kdNode.bmin_start);
              kdNode.bmax_count = nKdbox.bmax;
            }
          }
          kdNode.bmin_start = v_perm_xyzd(kdNode.bmin_start, v_cast_vec4f(v_splatsi(flags_count)));
        }
        if (flags & tdata.MFLG_RECALC_BBOX)
        {
          box_from_tree_leaves(tBox, kdNodes.data() + tdata.kdTreeLeftNode, kdNodes.data() + tdata.kdTreeLeftNode + kdTreeNodeCount);
          flags &= ~tdata.MFLG_RECALC_BBOX;
        }
      }
      else
#endif
        flags |= tdata.MFLG_REBUILD_KDTREE | tdata.MFLG_IVALIDATE_KDTREE;
    }
  }
  if (kdTreeNodeCount && !(flags & tdata.MFLG_IVALIDATE_KDTREE))
    tdata.nodes.erase(it);
  else
    tdata.nodes.erase_unsorted(it);
  if (tdata.nodes.size() == 1)
    initTile(tdata);
  else if (!tdata.nodes.size())
  {
    if (idx == OUTER_TILE_INDEX)
    {
      tileCull[idx].clear();
      tileData[idx].clear();
      v_bbox3_init_empty(tileBox[idx]);
    }
    else
      return releaseTile(idx);
  }
  else
    scheduleTileMaintenance(tdata, flags);

  // scheduleTileMaintenance(tdata, tdata.MFLG_RECALC_BBOX|tdata.MFLG_REBUILD_KDTREE|tdata.MFLG_IVALIDATE_KDTREE);//simple, but slow
}

__forceinline void scene::TiledScene::updateTileExtents(TileData &tdata, mat44f_cref node, bbox3f &wabb)
{
  const int tidx = eastl::distance(tileData.begin(), &tdata);
  bbox3f &tbox = tileBox[tidx];

  // todo. we can not call calcNodeBox, and update max ext if node sphere is totally inside current tilebox
  // this will optimize movement, but some of calling code now requires wabb to be node bbox.
  wabb = calcNodeBox(node);
  wabb.bmax = v_perm_xyzd(wabb.bmax, node.col0);
  if (tdata.nodes.size() > 1)
  {
    tbox.bmin = v_perm_xyzd(v_min(tbox.bmin, wabb.bmin), tbox.bmin);
    tbox.bmax = v_max(tbox.bmax, wabb.bmax);
  }
  else
  {
    tbox.bmin = v_perm_xyzd(wabb.bmin, v_zero());
    tbox.bmax = wabb.bmax;
  }
  updateTileMaxExt(tidx);
  orTileFlags(tbox, get_node_flags(node));
}

__forceinline void scene::TiledScene::fastGrowNodeBox(bbox3f &tBox, mat44f_cref node)
{
  bbox3f wabb;
  vec4f sphere = get_node_bsphere(node);
  wabb.bmin = v_sub(sphere, v_splat_w(sphere));
  wabb.bmax = v_add(sphere, v_splat_w(sphere));
  if (!v_bbox3_test_box_inside(tBox, wabb))
  {
    const pool_index pool = get_node_pool(node);
    if (pool < poolBox.size())
      v_bbox3_init(wabb, node, getPoolBboxInternal(pool));
    wabb.bmax = v_perm_xyzd(wabb.bmax, node.col0);
    v_bbox3_add_box(tBox, wabb);
  }
  else
  {
    // bounding sphere is already inside tile box
    tBox.bmax = v_perm_xyzd(tBox.bmax, v_max(node.col0, tBox.bmax));
  }
}


void scene::TiledScene::updateTile(int t_idx, uint32_t index, const mat44f *old_node)
{
  mat44f_cref node = nodes[index];
  G_ASSERT(t_idx != INVALID_INDEX);
  TileData &tdata = getTileByIndex(t_idx);
  const bbox3f &tbox = tileBox[t_idx];
  uint32_t isOnTileBorder = 0;
  // if we will allow changing flags, should also change that
  bbox3f oldWabb, wabb, oldTbox;
  if (old_node && tdata.nodes.size() > 1)
  {
    oldWabb = calcNodeBox(*old_node);
    oldTbox = tbox;
    vec4f border = v_or(v_cmp_ge(tbox.bmin, oldWabb.bmin), v_cmp_ge(oldWabb.bmax, tbox.bmax)); // cmp_eq should be enough, however it
                                                                                               // doesn't matter and safer
    isOnTileBorder = v_signmask(border) & 7;
  }
  updateTileExtents(tdata, node, wabb);

  if (tdata.nodes.size() > 1)
  {
    uint32_t flags = 0;
    // check if we are on tile border AND not moving out (otherwise doesn't make sense to recalc box, it is already correct)
    if (isOnTileBorder) // || newRad < oldRad (too expensive, we'd better compare newRad with smallest radius)!
    {
      vec4f border = v_and(v_cmp_gt(wabb.bmin, oldTbox.bmin), v_cmp_gt(oldTbox.bmax, wabb.bmax));
      if (v_signmask(border) & isOnTileBorder)
        flags |= tdata.MFLG_RECALC_BBOX;
    }

    const uint32_t kdTreeNodeCount = getKdTreeCount(tbox);
    // change existing kdtree, if tile kdtree is valid. first go down to leaf containing index, then update it's bbox and go up to root
    if ((!old_node && tdata.nodes.size() >= min_kdtree_nodes_count) || kdTreeNodeCount)
    {
      flags |= tdata.MFLG_REBUILD_KDTREE | tdata.MFLG_IVALIDATE_KDTREE;
#if KD_LEAVES_ONLY
      if (old_node && kdTreeNodeCount)
      {
        // todo: refactor to spearate function, for use in removal as well
        auto it = eastl::find(tdata.nodes.begin(), tdata.nodes.end(), getNodeFromIndex(index));
        G_ASSERT(it != tdata.nodes.end());
        if (it != tdata.nodes.end())
        {
          const int tileLocation = eastl::distance(tdata.nodes.begin(), it);
          uint32_t start = 0, count = 0, i = tdata.kdTreeLeftNode, ei = i + kdTreeNodeCount;
          for (; i < ei; ++i, start += count)
          {
            count = v_extract_wi(v_cast_vec4i(kdNodes.data()[i].bmin_start)) & 0xFFFF;
            if (tileLocation < start + count)
              break;
          }
          G_ASSERT(i < ei);
          if (i < ei)
          {
            auto &kdNode = kdNodes[i];
            bbox3f kdNodeBox = kdNode.getBox();
            vec4f oldborder = v_or(v_andnot(v_cmp_eq(oldWabb.bmin, wabb.bmin), v_cmp_ge(kdNodeBox.bmin, oldWabb.bmin)),
              v_andnot(v_cmp_eq(oldWabb.bmax, wabb.bmax), v_cmp_ge(oldWabb.bmax, kdNodeBox.bmax)));
            vec4f crossedBorder = v_or(v_cmp_gt(kdNodeBox.bmin, wabb.bmin), v_cmp_gt(wabb.bmax, kdNodeBox.bmax));
            oldborder = v_or(oldborder, crossedBorder);
            uint32_t oldBorderMask = v_signmask(oldborder) & 7;
            if (oldBorderMask) // (was on border && moved from that border) || crossed the border.
            {
              uint32_t newBorderMask = v_signmask(crossedBorder);
              if (~newBorderMask & oldBorderMask) // is inside && (was on border && moved from that border), 'crossed the border'
                                                  // canceled by 'is inside'.
              {
                // we moved inside leaf
                bbox3f nKdbox;
                v_bbox3_init_empty(nKdbox);
                for (auto nodeIt = &tdata.nodes[start], end = nodeIt + count; nodeIt != end; ++nodeIt)
                  fastGrowNodeBox(nKdbox, getNodeInternal(*nodeIt));
                kdNode.bmin_start = v_perm_xyzd(nKdbox.bmin, kdNode.bmin_start);
                kdNode.bmax_count = nKdbox.bmax;
                flags &= ~(tdata.MFLG_IVALIDATE_KDTREE | tdata.MFLG_REBUILD_KDTREE); // it is already valid tree!
                if (flags & tdata.MFLG_RECALC_BBOX)
                {
                  box_from_tree_leaves(tileBox[t_idx], kdNodes.data() + tdata.kdTreeLeftNode,
                    kdNodes.data() + tdata.kdTreeLeftNode + kdTreeNodeCount);
                  flags &= ~tdata.MFLG_RECALC_BBOX;
                }
              }
              else
              {
                // we move outside
                vec4f difference = v_max(v_sub(kdNodeBox.bmin, wabb.bmin), v_sub(wabb.bmax, kdNodeBox.bmax));
                difference = v_max(difference, v_zero());
                difference = v_div(difference, v_sub(kdNodeBox.bmax, kdNodeBox.bmin));
                difference = v_max(difference, v_rot_1(difference));
                difference = v_max(difference, v_rot_1(difference));
                kdNode.bmin_start = v_perm_xyzd(v_min(wabb.bmin, kdNodeBox.bmin), kdNode.bmin_start);
                kdNode.bmax_count = v_max(wabb.bmax, kdNode.bmax_count);
                flags &= ~tdata.MFLG_IVALIDATE_KDTREE; // it is already valid tree!

                // calculate penalty
                tdata.rebuildPenalty += v_extract_x(difference);
                if (tdata.rebuildPenalty < kdtree_rebuild_threshold)
                  flags &= ~tdata.MFLG_REBUILD_KDTREE;
              }
            }
            else // it is geometrically correct kdtree already
            {
              flags &= ~(tdata.MFLG_IVALIDATE_KDTREE | tdata.MFLG_RECALC_BBOX);
              // if we hadn't scale radius AND moving inside we can just say nothing changed
              const float oldDist = v_extract_w(old_node->col0), newDist = v_extract_w(node.col0);
              if (oldDist == newDist) // completely unchanged leaf
                flags &= ~tdata.MFLG_REBUILD_KDTREE;
              else // we have to change kdtree leaf
              {
                if (oldDist > newDist)
                {
                  // make maximum of current node dist and current kdnode
                  kdNode.bmax_count = v_max(kdNode.bmax_count, wabb.bmax);
                  flags &= ~tdata.MFLG_REBUILD_KDTREE;
                }
                else
                {
                  // just recalc best distance in leaf
                  vec4f dist = v_zero();
                  for (int nodeIndex = start; nodeIndex < start + count; ++nodeIndex)
                    dist = v_max(dist, getNodeInternal(tdata.nodes[nodeIndex]).col0);
                  kdNode.bmax_count = v_perm_xyzd(kdNode.bmax_count, dist);
                  flags &= ~tdata.MFLG_REBUILD_KDTREE;
                }
              }
            }
          }
        }
      }
#endif
    }
    scheduleTileMaintenance(tdata, flags);
  }
  else
    copyTileDataToCull(tdata);
  tdata.nextGeneration();
}

void scene::TiledScene::allocateNodeTile(node_index node, uint16_t pool, uint16_t /*flags*/)
{
  if (pool < poolBox.size())
  {
    max_dist_scale_sq = max(max_dist_scale_sq, v_extract_w(poolBox[pool].bmin));
    min_dist_scale_sq = min(min_dist_scale_sq, v_extract_w(poolBox[pool].bmin));
  }

  uint32_t index = getNodeIndex(node);
  G_ASSERT(index < nodes.size());

  int t_idx = selectTileIndex(index, true);
  insertNode(getTileByIndex(t_idx), node);
  updateTile(t_idx, index, NULL);
}

scene::node_index scene::TiledScene::allocateImm(node_index node, const mat44f &transform, uint16_t pool, uint16_t flags)
{
  G_ASSERT_RETURN(isInWritingThread(), INVALID_NODE);
  SimpleScene::reallocate(node, transform, pool, flags);
  if (tileSize > 0)
    allocateNodeTile(node, pool, flags);
  return node;
}

void scene::TiledScene::reallocateImm(node_index node, const mat44f &transform, uint16_t pool, uint16_t flags)
{
  if (tileSize <= 0)
  {
    SimpleScene::reallocate(node, transform, pool, flags);
    return;
  }

  G_ASSERT_RETURN(isInWritingThread(), );
  uint32_t index = getNodeIndex(node);
  if (isFreeIndex(index))
    return;

  int t_idx = selectTileIndex(index, false);
  // fixme: this is too slow if node is still within same tile
  removeNode(getTileByIndex(t_idx), node);
  allocateImm(node, transform, pool, flags);
}

void scene::TiledScene::setTransformNoScaleChangeImm(node_index node, mat44f_cref transform)
{
  if (tileSize <= 0)
  {
    SimpleScene::setTransformNoScaleChange(node, transform);
    return;
  }

  G_ASSERT_RETURN(isInWritingThread(), );
  uint32_t index = getNodeIndex(node);
  if (isFreeIndex(index))
    return;


  int t_idx = selectTileIndex(index, false);
  mat44f oldTransform = nodes.data()[index];
  vec3f move = v_sub(transform.col3, oldTransform.col3);
  SimpleScene::setTransformNoScaleChange(node, transform);
  if (t_idx != OUTER_TILE_INDEX && v_test_vec_x_gt_0(v_length3_sq(move)))
  {
    int nt_idx = selectTileIndex(index, true);
    if (nt_idx != t_idx)
    {
      removeNode(getTileByIndex(t_idx), node);
      insertNode(getTileByIndex(nt_idx), node);
      t_idx = nt_idx;
    }
  }

  updateTile(t_idx, index, &oldTransform);
}

void scene::TiledScene::setTransformImm(node_index node, mat44f_cref transform)
{
  if (tileSize <= 0)
  {
    SimpleScene::setTransform(node, transform);
    return;
  }

  G_ASSERT_RETURN(isInWritingThread(), );
  uint32_t index = getNodeIndex(node);
  if (isFreeIndex(index))
    return;

  int t_idx = selectTileIndex(index, false);
  mat44f oldTransform = nodes.data()[index];
  vec3f move = v_sub(transform.col3, oldTransform.col3);
  SimpleScene::setTransform(node, transform);

  if (t_idx != OUTER_TILE_INDEX && v_test_vec_x_gt_0(v_length3_sq(move)))
  {
    int nt_idx = selectTileIndex(index, true);
    if (nt_idx != t_idx)
    {
      removeNode(getTileByIndex(t_idx), node);
      insertNode(getTileByIndex(nt_idx), node);
      t_idx = nt_idx;
    }
  }

  updateTile(t_idx, index, &oldTransform);
}

void scene::TiledScene::setFlagsImm(node_index node, uint16_t flags)
{
  uint32_t nodeIdx = getNodeIndexInternal(node);
  if (isFreeIndex(nodeIdx))
    return;

  SimpleScene::setFlags(node, flags);
  uint32_t tileIdx = selectTileIndex(nodeIdx, false);
  scheduleTileMaintenance(getTileByIndex(tileIdx), TileData::MFLG_RECALC_FLAGS);
}

void scene::TiledScene::unsetFlagsImm(node_index node, uint16_t flags)
{
  uint32_t nodeIdx = getNodeIndexInternal(node);
  if (isFreeIndex(nodeIdx))
    return;

  SimpleScene::unsetFlags(node, flags);
  uint32_t tileIdx = selectTileIndex(nodeIdx, false);
  scheduleTileMaintenance(getTileByIndex(tileIdx), TileData::MFLG_RECALC_FLAGS);
}

void scene::TiledScene::destroyImm(node_index node)
{
  if (tileSize <= 0)
  {
    SimpleScene::destroy(node);
    return;
  }
  G_ASSERT_RETURN(isInWritingThread(), );
  uint32_t index = getNodeIndex(node);
  G_ASSERT_RETURN(index < nodes.size(), );
  G_ASSERT_RETURN(!isFreeIndex(index), );

  int t_idx = selectTileIndex(index, false);
  G_ASSERTF_RETURN(t_idx >= 0 && t_idx < tileData.size() && t_idx < tileBox.size() && t_idx < tileCull.size(),
    SimpleScene::destroy(node), "node=%08X index=%08X t_idx=%d tileData.size()=%d tileBox.size()=%d tileCull.size()=%d  pos=%@ rad=%@",
    node, index, t_idx, tileData.size(), tileBox.size(), tileCull.size(), as_point3(&nodes[index].col3),
    get_node_bsphere_rad(nodes[index]));
  removeNode(getTileByIndex(t_idx), node);
  SimpleScene::destroy(node);
}

void scene::TiledScene::rearrange(float tile_sz)
{
  G_ASSERT(isInWritingThread());
  WriteLockRAII lock(*this);
  termTiledStructures();
  tileSize = tile_sz;
  if (tileSize <= 0)
    return;
  initOuter();
  for (auto node : ((SimpleScene &)*this))
  {
    uint32_t index = getNodeIndex(node);
    if (index >= nodes.size())
      continue;
    allocateNodeTile(node, getIndexPool(index), getIndexFlags(index));
  }
}

void scene::TiledScene::shrinkKdTree(float dynamicAmount)
{
  if (usedKdNodes == kdNodes.size())
    return;
  dag::Vector<kdtree::KDNode> kdNodes2;
  kdNodes2.reserve(usedKdNodes);

#if !KD_LEAVES_ONLY
  dag::Vector<float> kdNodesDistance2;
  kdNodesDistance2.reserve(usedKdNodes);
#endif

  dynamicAmount = clamp(dynamicAmount, 0.f, 1.f);
  checkSoA();
  for (int i = 0; i < tileData.size(); ++i)
  {
    auto &tdata = tileData[i];
    auto &tcull = tileCull[i];
    auto &tbox = tileBox[i];
    G_ASSERT(tdata.isDead() == tcull.isDead());
    if (tdata.isDead() || !getKdTreeCount(tbox))
      continue;
    const auto kdTreeNodeCount = getKdTreeCount(tbox);
    if (!kdTreeNodeCount)
      continue;
    const uint32_t newRoot = (uint32_t)kdNodes2.size();
    const int newTotalCount = kdTreeNodeCount + dynamicAmount * (tdata.kdTreeNodeTotalCount - kdTreeNodeCount);
    kdNodes2.resize(kdNodes2.size() + newTotalCount);
    memcpy(kdNodes2.data() + newRoot, kdNodes.begin() + tdata.kdTreeLeftNode, kdTreeNodeCount * sizeof(kdtree::KDNode));
#if DAGOR_DBGLEVEL > 0
    memset(kdNodes2.data() + newRoot + kdTreeNodeCount, 0xFF /*invalidate*/,
      (newTotalCount - kdTreeNodeCount) * sizeof(kdtree::KDNode));
#endif

#if !KD_LEAVES_ONLY
    kdNodesDistance2.resize(kdNodes2.size());
    memcpy(kdNodesDistance2.data() + newRoot, kdNodesDistance.begin() + tdata.kdTreeLeftNode, kdTreeNodeCount * sizeof(float));
#if DAGOR_DBGLEVEL > 0
    memset(kdNodesDistance2.data() + newRoot + kdTreeNodeCount, 0xFF /*invalidate*/,
      (newTotalCount - kdTreeNodeCount) * sizeof(float));
#endif
    tdata.kdTreeRightNode += newRoot - tdata.kdTreeLeftNode;
    G_ASSERT(tdata.kdTreeRightNode > newRoot);
#endif
    tdata.kdTreeLeftNode = newRoot;
    tdata.kdTreeNodeTotalCount = newTotalCount;
    copyTileDataToCull(tdata);
  }
  eastl::swap(kdNodes, kdNodes2);

#if !KD_LEAVES_ONLY
  eastl::swap(kdNodesDistance, kdNodesDistance2);
#endif
}

template <class Nodes, class Indices, class T>
static void rearrange_by_indices(Nodes &nodes, const Indices &indices, T *scratch)
{
  G_ASSERT(indices.size() == nodes.size());
  for (int i = 0; i < indices.size(); ++i)
    scratch[i] = nodes.data()[indices.data()[i]];
  memcpy(nodes.data(), scratch, nodes.size() * sizeof(T));
}

bool scene::TiledScene::buildKdTree(TileData &tdata)
{
  FRAMEMEM_REGION;
  tdata.rebuildPenalty = 0;
  // todo: only for semi-static grids (not very-very dynamic). For already (almost)sorted arrays cost is very small.
  bbox3f &tbox = tileBox[eastl::distance(tileData.begin(), &tdata)];

  const auto oldKdTreeNodeCount = getKdTreeCount(tbox);
  usedKdNodes -= oldKdTreeNodeCount;

  if (tdata.nodes.size() < min_kdtree_nodes_count)
  {
    setKdTreeCount(tbox, 0);
    return false;
  }

  // we sort it here, because for dynamic scenes min_kdtree_nodes_count can be very big
  // sort nodes for cache locality, when then will be checking visibility.
  stlsort::sort(tdata.nodes.begin(), tdata.nodes.end(),
    [&](node_index a, node_index b) -> auto { return getNodeIndexInternal(a) < getNodeIndexInternal(b); });


  dag::Vector<bbox3f, framemem_allocator> boxes;
  dag::Vector<uint32_t, framemem_allocator> indices;
  dag::Vector<float, framemem_allocator> centers;
  indices.resize(tdata.nodes.size());
  boxes.resize(tdata.nodes.size());
  centers.resize(tdata.nodes.size() * 3);

  int i = 0;
  bbox3f totalBox;
  v_bbox3_init_empty(totalBox);
  uint16_t totalFlags = 0;
  for (auto it = tdata.nodes.begin(); it != tdata.nodes.end(); it++, i++)
  {
    const auto &node = getNode(*it);
    bbox3f box = calcNodeBox(node);
    vec3f center = v_bbox3_center(box);
    totalFlags |= get_node_flags(node);
    box.bmax = v_perm_xyzd(box.bmax, node.col0);
    v_bbox3_add_box(totalBox, box);

    centers[i] = v_extract_x(center);
    centers[i + indices.size()] = v_extract_y(center);
    centers[i + indices.size() * 2] = v_extract_z(center);
    boxes[i] = box;
    indices[i] = i;
  }
  tbox.bmin = v_perm_xyzd(totalBox.bmin, tbox.bmin);
  tbox.bmax = totalBox.bmax;
  setTileFlags(tbox, totalFlags);

  const uint32_t prevTotalCount = (uint32_t)kdNodes.size();
  kdtree::KDNode node;
  // after that build it can happen that some of the leaves are completely within other leaves
  //  and it will happen about 50% of times :(
  //  moreover around 2% of leaves combined will still be less than max_to_split_count
  //  can be optimized

  const int32_t leftNode =
    make_nodes(&node, boxes.begin(), &centers[0], &centers[indices.size()], &centers[indices.size() * 2], 0, (int)boxes.size() - 1,
      indices.begin(), kdNodes, min_to_split_geom, max_to_split_count, min_box_to_split_geom, max_box_to_split_count);

  if (leftNode < 0 || kdNodes.size() <= 1 + prevTotalCount) // we could not built Kd-tree, or it is useless tree
  {
    // todo: if we could not build kd-tree, we'd better decrease tileSize (rearrange(tileSize*0.5))
    // however, this is very unlikely to happen, and calling from maintenance cycle is not possible
    kdNodes.resize(prevTotalCount);
    if (leftNode < 0)
      logerr("can not build kdtree");
    setKdTreeCount(tbox, 0);
    return true;
  }

#if KD_LEAVES_ONLY
  int totalLeaves = 0;
  kdtree::leaves_only(kdNodes.begin() + leftNode, node, 0, totalLeaves);
  // debug("%d leaves left out of all %d", totalLeaves, kdNodes.size()-prevTotalCount);
  kdNodes.resize(prevTotalCount + totalLeaves);
#endif

  const int builtCount = (int)(kdNodes.size()) - leftNode;

  // debug("build kdtree nodes %d builtCount = %d", tile.nodes.size(), builtCount);
  // rearrange tile.nodes according to indices
  rearrange_by_indices(tdata.nodes, indices, (scene::node_index *)centers.data());

  G_ASSERT(leftNode == prevTotalCount);
  G_ASSERT(tdata.kdTreeLeftNode < 0 || tdata.kdTreeNodeTotalCount > 0);


  if (builtCount <= tdata.kdTreeNodeTotalCount && tdata.kdTreeLeftNode + builtCount <= prevTotalCount)
  {
    // reuse same space, as we have enough
    memmove(&kdNodes[tdata.kdTreeLeftNode], &kdNodes[leftNode], builtCount * sizeof(kdtree::KDNode));
    kdNodes.resize(prevTotalCount);
    tdata.kdTreeNodeTotalCount = builtCount;
  }
  else
  {
    tdata.kdTreeNodeTotalCount = builtCount;
    tdata.kdTreeLeftNode = leftNode;
  }
  setKdTreeCount(tbox, builtCount);

#if !KD_LEAVES_ONLY
  tdata.kdTreeRightNode = node.getRightToLeft(tdata.kdTreeLeftNode);
  kdNodesDistance.resize(kdNodes.size());
#endif
  const auto newKdTreeNodeCount = getKdTreeCount(tbox);

  // fill distance
  uint32_t cStart = 0;
  for (int i = tdata.kdTreeLeftNode; i < tdata.kdTreeLeftNode + newKdTreeNodeCount && i < kdNodes.size(); ++i)
  {
    auto &kdNode = kdNodes[i];

    // sort nodes for cache locality, when then will be checking visibility.
    stlsort::sort(tdata.nodes.begin() + kdNode.getStart(), tdata.nodes.begin() + kdNode.getStart() + kdNode.getCount(),
      [&](node_index a, node_index b) -> auto { return getNodeIndexInternal(a) < getNodeIndexInternal(b); });
    // fixme: if !KD_LEAVES_ONLY, this is very unoptimal, we'd better calc recursive
    vec4f bmax_count = boxes[indices[kdNode.getStart()]].bmax;
    uint16_t flags = 0;
    for (int nodeIndex = kdNode.getStart(), nodeEnd = nodeIndex + kdNode.getCount(); nodeIndex < nodeEnd; ++nodeIndex)
    {
      flags |= get_node_flags(getNodeInternal(tdata.nodes.data()[nodeIndex]));
      bmax_count = v_max(bmax_count, boxes[indices[nodeIndex]].bmax);
    }
#if KD_LEAVES_ONLY
    G_ASSERT(cStart == kdNode.getStart());
    G_ASSERTF(kdNode.getCount() <= 65535, "todo:split leaf");
    cStart += kdNode.getCount();
    uint32_t flags_count = kdNode.getCount() | (flags << 16);
    kdNode.bmin_start = v_perm_xyzd(kdNode.bmin_start, v_cast_vec4f(v_splatsi(flags_count)));
    kdNode.bmax_count = bmax_count;
#else
    G_UNUSED(flags);
    G_UNUSED(cStart);
    kdNodesDistance[i] = v_extract_w(bmax_count);
#endif
  }

  usedKdNodes += newKdTreeNodeCount;
  if (usedKdNodes < kdNodes.size() * 2 / 3) // 66% load factor
    shrinkKdTree(0.5f);                     // fixme: use some other heurestiscs
  return true;
}
template <typename Callable>
bool scene::TiledScene::doMaintenancePass(int64_t reft, unsigned max_time_to_spend_usec, const Callable &c)
{
  for (int i = curFirstTileForMaint; i < (int)tileData.size() && curTilesCountForMaint; i++)
  {
    auto &tdata = tileData[i];
    auto &tcull = tileCull[i];
    G_ASSERT(tcull.isDead() == tdata.isDead());
    if (tdata.isDead())
      continue;
    if (!tdata.maintenanceFlags)
      continue;

    c(tdata);
    if (!tdata.maintenanceFlags)
    {
      curTilesCountForMaint--;
      if (curFirstTileForMaint == i)
        curFirstTileForMaint = i + 1;
    }
    if (get_time_usec(reft) > max_time_to_spend_usec)
      return false;
  }
  return true;
}

void scene::TiledScene::updateTileMaxExt(int tidx)
{
  if (tidx == OUTER_TILE_INDEX)
    return;
  bbox3f &tbox = tileBox[tidx];
  auto &tdata = tileData[tidx];
  updateMaxExt((float *)(char *)(&tbox.bmin), tdata.objMaxExt, tdata.getTileX(getTileX0()), tdata.getTileZ(getTileZ0()), tileSize);
  // todo: we currently never shrink gridObjMaxExt!!!
  inplace_max(gridObjMaxExt, tdata.getObjMaxExt());
}

bool scene::TiledScene::doMaintenance(int64_t reft, unsigned max_time_to_spend_usec)
{
  G_ASSERT_RETURN(isInWritingThread(), true);
  G_ASSERT_AND_DO(max_time_to_spend_usec < 100000000, max_time_to_spend_usec = 100000000);
  WriteLockRAII lock(*this);

  checkSoA();

  if (usedTilesCount < tileData.size() && mFirstFreeTile < usedTilesCount)
  {
    for (int i = mFirstFreeTile, lastUsedTile = tileData.size() - 1; i < lastUsedTile; i++)
      if (tileData[i].isDead())
      {
        for (; lastUsedTile > i; lastUsedTile--)
          if (!tileData[lastUsedTile].isDead())
            break;
        if (lastUsedTile > i)
        {
          eastl::swap(tileCull[i], tileCull[lastUsedTile]);
          eastl::swap(tileData[i], tileData[lastUsedTile]);
          eastl::swap(tileBox[i], tileBox[lastUsedTile]);
          tileGrid[getTileGridIndex(tileData[i].getTileX(getTileX0()), tileData[i].getTileZ(getTileZ0()))] = i;
          lastUsedTile--;
          mFirstFreeTile = i + 1;
        }
        else
        {
          G_ASSERT(i == usedTilesCount);
          break;
        }
      }
    mFirstFreeTile = usedTilesCount;
    tileData.resize(usedTilesCount);
    tileCull.resize(usedTilesCount);
    tileBox.resize(usedTilesCount);
  }

  // pass 1: update KD-tree
  if (!doMaintenancePass(reft, max_time_to_spend_usec, [&](TileData &tdata) {
        if (tdata.maintenanceFlags & tdata.MFLG_REBUILD_KDTREE)
        {
          if (buildKdTree(tdata)) // if true, bbox is also updated, so we don't need to rebuild it
          {
            updateTileMaxExt(eastl::distance(tileData.begin(), &tdata));
            tdata.maintenanceFlags &= ~tdata.MFLG_RECALC_BBOX;
          }
          tdata.maintenanceFlags &= ~tdata.MFLG_REBUILD_KDTREE;
          copyTileDataToCull(tdata);
        }
      }))
    return false;

  // pass 2: update boxes
  if (!doMaintenancePass(reft, max_time_to_spend_usec, [&](TileData &tdata) {
        if ((tdata.maintenanceFlags & tdata.MFLG_RECALC_BBOX))
        {
          bbox3f tBox;
          v_bbox3_init_empty(tBox);
          uint32_t flags = 0;
          for (auto ni : tdata.nodes)
          {
            const auto &node = getNodeInternal(ni);
            fastGrowNodeBox(tBox, node);
            flags |= get_node_pool_flags(node);
          }
          const int tidx = eastl::distance(tileData.begin(), &tdata);
          bbox3f &tbox = tileBox[tidx];
          tbox.bmin = v_perm_xyzd(tBox.bmin, tbox.bmin);
          tbox.bmax = tBox.bmax;
          setTileFlags(tbox, flags >> 16);
          updateTileMaxExt(tidx);
          tdata.maintenanceFlags &= ~tdata.MFLG_RECALC_BBOX;
          copyTileDataToCull(tdata);
        }
      }))
    return false;

  // pass 3: update flags
  if (!doMaintenancePass(reft, max_time_to_spend_usec, [&](TileData &tdata) {
        if (tdata.maintenanceFlags & tdata.MFLG_RECALC_FLAGS)
        {
          TIME_PROFILE(tiled_scene_recalc_tile_flags);
          const int tidx = eastl::distance(tileData.begin(), &tdata);
          bbox3f &tbox = tileBox[tidx];

          uint32_t tileFlags = 0;
#if KD_LEAVES_ONLY
          uint16_t kdNodeCount = getKdTreeCount(tbox);
          uint32_t nodeStart = 0;
          for (int i = tdata.kdTreeLeftNode; i < tdata.kdTreeLeftNode + kdNodeCount && i < kdNodes.size(); ++i)
          {
            auto &kdNode = kdNodes[i];
            uint16_t flags = 0;
            uint16_t nodeCount = v_extract_wi(v_cast_vec4i(kdNode.bmin_start)) & 0xFFFF;
            for (int nodeIndex = nodeStart, nodeEnd = nodeIndex + nodeCount; nodeIndex < nodeEnd; ++nodeIndex)
              tileFlags |= flags |= get_node_flags(getNodeInternal(tdata.nodes[nodeIndex]));
            nodeStart += nodeCount;
            uint32_t flags_count = nodeCount | (flags << 16);
            kdNode.bmin_start = v_perm_xyzd(kdNode.bmin_start, v_cast_vec4f(v_splatsi(flags_count)));
          }
          if (kdNodeCount)
          {
            G_ASSERT(nodeStart == tdata.nodes.size());
          }
          else
#endif
          {
            for (auto ni : tdata.nodes)
              tileFlags |= get_node_flags(getNodeInternal(ni));
          }

          setTileFlags(tbox, tileFlags);

          tdata.maintenanceFlags &= ~tdata.MFLG_RECALC_FLAGS;
          copyTileDataToCull(tdata);
        }
      }))
    return false;

  G_ASSERT(!tileData.size() || tileData[OUTER_TILE_INDEX].isDead() == tileCull[OUTER_TILE_INDEX].isDead());
  G_ASSERT(
    !tileData.size() || tileData[OUTER_TILE_INDEX].isDead() == isEmptyTile(tileBox[OUTER_TILE_INDEX], tileCull[OUTER_TILE_INDEX]));

#if DAGOR_DBGLEVEL > 1
  for (auto &td : tileData)
  {
    for (auto ni : td.nodes)
    {
      G_ASSERT(isAliveNode(ni));
    }
  }
#endif
  // todo: from time to time call shrinkKdTree(), if nothing is changing for a while
  return true;
}

#include <math/dag_vecMathCompatibility.h>
#include <math/dag_Point2.h>
void scene::TiledScene::dumpSceneState(bool dump_node_too) const
{
  int obj_cnt = 0, obj_cnt_max = 0;
  for (int tz = getTileZ0(); tz < getTileZ1(); tz++)
    for (int tx = getTileX0(); tx < getTileX1(); tx++)
      if (getTileIndex(tx, tz) != INVALID_INDEX)
      {
        int tile_obj = (int)tileData[getTileIndex(tx, tz)].nodes.size();
        obj_cnt += tile_obj;
        if (obj_cnt_max < tile_obj)
          obj_cnt_max = tile_obj;
      }

  debug("TiledScene(%p) tileSize=%.0f tile=(%d,%d)-(%d,%d) tiles=%d (%d used) "
        "objs=%d(%d) +%d pending, avg/tile=%.1f max/tile=%d maxExt=%.2f",
    this, tileSize, getTileX0(), getTileZ0(), getTileX1(), getTileZ1(), tileGrid.size(), usedTilesCount, getNodesAliveCount(), obj_cnt,
    pendingNewNodesCount1 + pendingNewNodesCount2 + pendingNewNodesCount3, usedTilesCount ? float(obj_cnt) / usedTilesCount : 0,
    obj_cnt_max, gridObjMaxExt);
  for (int tz = getTileZ0(); tz < getTileZ1(); tz++)
    for (int tx = getTileX0(); tx < getTileX1(); tx++)
    {
      if (getTileIndex(tx, tz) == INVALID_INDEX)
      {
        debug("  tile(%+d,%+d)=empty", tx, tz);
        continue;
      }
      const TileData *t = &tileData[getTileIndex(tx, tz)];
      bbox3f tbox = tileBox[getTileIndex(tx, tz)];

      debug("  tileGrid(%+d,%+d)=tile[%2d]=%p objs=%3d maxExt=%.1f maxVisF=%g kdTcnt=%d tileBox=(%@)-(%@) loc=%@", tx, tz,
        getTileIndex(tx, tz), t, t->nodes.size(), t->getObjMaxExt(), as_point4(&tbox.bmax).w, ((int *)&tbox.bmin)[3] & 0xFFFF,
        as_point3(&tbox.bmin), as_point3(&tbox.bmax),
        BBox2(Point2(getX0() + (tx - getTileX0()) * tileSize, getZ0() + (tz - getTileZ0()) * tileSize),
          Point2(getX0() + (tx + 1 - getTileX0()) * tileSize, getZ0() + (tz + 1 - getTileZ0()) * tileSize)));
      if (dump_node_too)
        for (int j = 0; j < t->nodes.size(); j++)
        {
          float bsph[4];
          v_stu(bsph, get_node_bsphere(getNode(t->nodes[j])));
          debug("    %08X  bsph=(%.2f,%.2f,%.2f) r=%.2f", t->nodes[j], bsph[0], bsph[1], bsph[2], bsph[3]);
        }
    }
  if (tileData.size())
  {
    if (tileData[0].nodes.size())
      debug("  outer tile=%p objs=%d tileBox=(%@)-(%@)", &tileData[0], tileData[0].nodes.size(), as_point3(&tileBox[0].bmin),
        as_point3(&tileBox[0].bmax));
    else
      debug("  outer tile=%p objs=%d", &tileData[0], tileData[0].nodes.size());
  }
}
