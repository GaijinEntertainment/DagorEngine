// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "riGen/riExtraPool.h"
#include <scene/dag_tiledScene.h>
#include <util/dag_delayedAction.h>
#include <math/dag_mathUtils.h>


namespace rendinst
{

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
    HAS_PER_INSTANCE_RENDER_DATA = 1 << 15,
  };
  using SimpleScene::calcNodeBox;
  using SimpleScene::calcNodeBoxFromSphere;
  using SimpleScene::getNode;
  using SimpleScene::getNodeFlags;
  using SimpleScene::getNodePool;
  using SimpleScene::getPoolSphereRad;
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
  using TiledScene::getNodesAliveCount;
  using TiledScene::getNodesCount;
  using TiledScene::getPoolBbox;
  using TiledScene::getTileCountInBox;
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

  using TiledScene::nodesInRange;

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
    SET_FLAGS_IN_BOX,
    SET_PER_INSTANCE_OFFSET,
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

    mDeferredCommand.addCmd([&](DeferredCommand &cmd) {
      cmd.command = DeferredCommand::Cmd(cmd.SET_USER_DATA + user_cmd);
      cmd.node = node;
      memcpy(&cmd.transformCommand, dw_data, dw_cnt * 4);
      ((int *)(char *)&cmd.transformCommand.col3)[3] = dw_cnt;
    });
  }

  template <typename ImmediatePerform = DoNothing>
  void setNodeUserDataEx(scene::node_index node, unsigned user_cmd, int dw_cnt, const int32_t *dw_data,
    ImmediatePerform user_cmd_processsor)
  {
    G_ASSERT(dw_cnt * 4 + 4 <= sizeof(mat44f));
    G_ASSERTF_AND_DO(user_cmd < DeferredCommand::SET_USER_DATA, user_cmd = DeferredCommand::SET_USER_DATA - 1, "user_cmd=0x%X",
      user_cmd);
    std::lock_guard<std::mutex> scopedLock(mDeferredSetTransformMutex);
    if (isInWritingThread() && mDeferredCommand.empty())
    {
      WriteLockRAII lock(*this);
      if (mDeferredCommand.empty())
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
      return nullptr;
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

  uint32_t getPerInstanceRenderDataOffset(scene::node_index ni) const
  {
    eastl::vector_map<scene::node_index, uint32_t>::const_iterator it = perInstanceRenderDataOffset.find(ni);
    if (it != perInstanceRenderDataOffset.end())
      return it->second;
    return 0;
  }

  void setPerInstanceRenderDataOffsetImm(scene::node_index ni, uint32_t offset)
  {
    G_ASSERT_RETURN(isInWritingThread(), );
    if (offset)
    {
      setFlagsImm(ni, HAS_PER_INSTANCE_RENDER_DATA);
      perInstanceRenderDataOffset[ni] = offset;
    }
    else
    {
      if (getNodeFlags(ni) & HAS_PER_INSTANCE_RENDER_DATA)
      {
        unsetFlagsImm(ni, HAS_PER_INSTANCE_RENDER_DATA);
        perInstanceRenderDataOffset.erase(ni);
      }
    }
  }

  void clearPerInstanceRenderDataOffsets()
  {
    G_ASSERT_RETURN(isInWritingThread(), );
    for (auto &it : perInstanceRenderDataOffset)
      unsetFlagsImm(it.first, HAS_PER_INSTANCE_RENDER_DATA);
    perInstanceRenderDataOffset.clear();
  }

  void onDirFromSunChanged(const Point3 &nd);

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
  eastl::vector_map<scene::node_index, uint32_t> perInstanceRenderDataOffset;
  int userDataWordCount = 0;
  Point3 dirFromSunOnPrevDistInvalidation = {0, 0, 0};
};


struct TiledScenePoolInfo
{
  float distSqLOD[RiExtraPool::MAX_LODS];
  uint32_t lodLimits, poolIdx;
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

} // namespace rendinst
