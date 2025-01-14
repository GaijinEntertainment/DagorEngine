// Copyright (C) Gaijin Games KFT.  All rights reserved.

#undef _DEBUG_TAB_
#include <anim/dag_animBlend.h>
#include <anim/dag_animKeyInterp.h>
#include <anim/dag_animBlendCtrl.h>
#include <anim/dag_animIrq.h>
#include <math/dag_quatInterp.h>
#include <math/dag_vecMathCompatibility.h>
#include <debug/dag_debug.h>
#include <debug/dag_fatal.h>
#include <perfMon/dag_perfTimer2.h>
#include <perfMon/dag_statDrv.h>
#include <debug/dag_log.h>
#include <util/dag_string.h>
#include <util/dag_fastNameMapTS.h>
#include <util/dag_lookup.h>
#include <supp/dag_prefetch.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_rwSpinLock.h>
#include <generic/dag_tabUtils.h>
#include <gameRes/dag_gameResources.h>
#include "animInternal.h"
#include <ioSys/dag_dataBlock.h>
#include <regExp/regExp.h>
#include <ctype.h>
#include <EASTL/hash_map.h>
#include <EASTL/bitvector.h>
#include <memory/dag_framemem.h>

// #define DUMP_ANIM_BLENDING 1

using namespace AnimV20;

static constexpr int PROFILE_BLENDING = 0;


struct AnimBlender::NodeSamplers
{
  union Entry
  {
    AnimV20Math::PrsAnimNodeSampler<AnimV20Math::OneShotConfig> sampler;
    dag::Index16 trackId;
    ~Entry() = delete;
  };

  ~NodeSamplers() = delete;

  Entry samplers[MAX_ANIMS_IN_NODE];
  uint32_t totalNum : 8;
  int32_t lastBnl : 24;
};


namespace perfanimgblend
{
PerformanceTimer2 perf_tm;
}
#if MEASURE_PERF
#include <perfMon/dag_perfMonStat.h>
#include <perfMon/dag_perfTimer2.h>
#include <startup/dag_globalSettings.h>

static int perf_cnt = 0, perf_tm_cnt = 0;
static int perf_frameno_start = 0;
using perfanimgblend::perf_tm;
#endif

#if DUMP_ANIM_BLENDING
#include <osApiWrappers/dag_spinlock.h>
#include <osApiWrappers/dag_files.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_zstdIo.h>
#include <gameRes/dag_gameResSystem.h>
static FullFileSaveCB blending_file;
static ZstdSaveCB *blending_zstd = nullptr;
static OSSpinlock blending_lock;
static PtrTab<AnimationGraph> blending_graphs;
static int blending_count = 0;
static int blending_last_frame = -1;

static void finish_blending_dump()
{
  blending_zstd->finish();
  delete blending_zstd;
  blending_zstd = nullptr;

  int graphOfs = blending_file.tell();
  for (auto &p : blending_graphs)
  {
    String name;
    get_game_resource_name(p->resId, name);
    blending_file.writeShortString(name);
  }

  blending_file.seekto(4);
  blending_file.writeInt(blending_graphs.size());
  blending_file.writeInt(graphOfs);
  blending_file.writeInt(blending_count);
  blending_file.close();
  clear_and_shrink(blending_graphs);
}

static void write_blending(IPureAnimStateHolder &st)
{
  OSSpinlockScopedLock guard(blending_lock);

  if (!blending_file.fileHandle)
  {
    blending_file.open("anim_blending.bin", DF_CREATE | DF_WRITE | DF_REALFILE_ONLY);
    blending_file.writeInt(_MAKE4C('ABLD'));
    blending_file.writeInt(0);
    blending_file.writeInt(0);
    blending_file.writeInt(0);
    blending_count = 0;
    G_ASSERT(blending_graphs.empty());
    blending_zstd = new ZstdSaveCB(blending_file, 9);
  }

  int frame = ::dagor_frame_no();
  if (blending_last_frame != frame)
  {
    blending_last_frame = frame;
    blending_zstd->writeIntP<2>(-1);
    blending_count++;
  }

  auto graph = &st.getGraph();
  int graphId;
  if (auto it = eastl::find(blending_graphs.begin(), blending_graphs.end(), graph); it != blending_graphs.end())
    graphId = it - blending_graphs.begin();
  else
  {
    graphId = blending_graphs.size();
    blending_graphs.push_back(graph);
  }

  blending_zstd->writeIntP<2>(graphId);
  st.saveState(*blending_zstd);
  blending_count++;

  if (blending_count % 1000 == 0)
    logerr("blending_count = %d", blending_count);

  if (blending_count >= 100000)
  {
    finish_blending_dump();
    DAG_FATAL("Blending dump saved");
  }
}
#endif

//
// Animation graph implementation
//
AnimationGraph::AnimationGraph() : paramTypes(midmem_ptr()), root(NULL), nodePtr(midmem_ptr()), ignoredAnimationNodes(midmem_ptr())
{
  initState = NULL;
}
AnimationGraph::~AnimationGraph()
{
  del_it(initState);
  for (int i = 0; i < stDest.size(); i++)
    stDest[i].fifo = NULL;
  clear_and_shrink(stDest);
  //==
}
void AnimationGraph::setInitState(const DataBlock &b)
{
  del_it(initState);
  if (b.paramCount() + b.blockCount())
  {
    initState = new DataBlock;
    *initState = b;
  }
}
const DataBlock &AnimationGraph::getEnumList(const char *enum_cls) const
{
  if (initState)
    return *initState->getBlockByNameEx("enum")->getBlockByNameEx(enum_cls);
  return DataBlock::emptyBlock;
}
void AnimationGraph::initChannelsForStates(const DataBlock &stDescBlk, dag::ConstSpan<AnimResManagerV20::NodeMask *> nodemask)
{
  char tmpbuf[SIZE_OF_FATAL_CTX_TMPBUF];
  int nid_chan = stDescBlk.getNameId("chan");
  int nid_state = stDescBlk.getNameId("state");
  int nid_alias = stDescBlk.getNameId("state_alias"), alias_cnt = 0;
  int chan_num = 0, state_num = 0;
  for (int i = 0; i < stDescBlk.blockCount(); i++)
    if (stDescBlk.getBlock(i)->getBlockNameId() == nid_chan)
      chan_num++;
    else if (stDescBlk.getBlock(i)->getBlockNameId() == nid_state)
      state_num++;
    else if (stDescBlk.getBlock(i)->getBlockNameId() == nid_alias)
      alias_cnt++;
    else
      logerr("unsupported block %s%s", stDescBlk.getBlock(i)->getBlockName(), dgs_get_fatal_context(tmpbuf, sizeof(tmpbuf)));

  clear_and_resize(stDest, chan_num);
  mem_set_0(stDest);
  clear_and_resize(stRec, chan_num * state_num);
  mem_set_0(stRec);

  for (int i = 0, cidx = 0; i < stDescBlk.blockCount(); i++)
    if (stDescBlk.getBlock(i)->getBlockNameId() == nid_chan)
    {
      const DataBlock &b = *stDescBlk.getBlock(i);
      const char *name = b.getStr("name", NULL);
      const char *nodemask_nm = b.getStr("nodeMask", name);

      G_ASSERT(name);
      G_ASSERT(nodemask_nm);
      G_ASSERT(b.getStr("fifo3", NULL));

      stDest[cidx].defNodemaskIdx = -2;
      if (!nodemask_nm[0])
        stDest[cidx].defNodemaskIdx = -1;
      for (int j = 0; j < nodemask.size(); j++)
        if (strcmp(nodemask[j]->name, nodemask_nm) == 0)
        {
          stDest[cidx].defNodemaskIdx = j;
          break;
        }
      G_ASSERTF(stDest[cidx].defNodemaskIdx != -2, "nodeMask=%s", nodemask_nm);

      stDest[cidx].condNodemaskTarget = b.getInt("cond_target", -1);
      if (stDest[cidx].condNodemaskTarget >= (signed)stDest.size())
      {
        logerr("bad cond_target=%d for chan #%d (%d total chans)%s", stDest[cidx].condNodemaskTarget, cidx, stDest.size(),
          dgs_get_fatal_context(tmpbuf, sizeof(tmpbuf)));
        stDest[cidx].condNodemaskTarget = -1;
      }
      if (stDest[cidx].condNodemaskTarget >= 0)
      {
        const char *cond_nodeMask = b.getStr("cond_nodeMask", "");
        stDest[cidx].condNodemaskIdx = -2;
        if (!cond_nodeMask[0])
          stDest[cidx].condNodemaskIdx = -1;
        else
          for (int j = 0; j < nodemask.size(); j++)
            if (strcmp(nodemask[j]->name, cond_nodeMask) == 0)
            {
              stDest[cidx].condNodemaskIdx = j;
              break;
            }
        G_ASSERTF(stDest[cidx].condNodemaskIdx != -2, "cond_nodeMask=%s", cond_nodeMask);
      }
      else
        stDest[cidx].condNodemaskIdx = -1;
      stDest[cidx].morphTimeToSetNullOnSingleAnimFinished = b.getReal("autoResetSingleAnimMorphTime", -1);
      cidx++;
    }

  for (int i = 0; i < stDest.size(); i++)
    if (stDest[i].condNodemaskIdx >= 0)
    {
      bool found = false;
      for (int j = 0, idx = 0; j < stDest.size(); j++)
        if (stDest[i].condNodemaskIdx == stDest[j].defNodemaskIdx)
        {
          stDest[i].condNodemaskIdx = idx + 1;
          found = true;
          break;
        }
        else if (stDest[j].defNodemaskIdx >= 0)
          idx++;
      G_ASSERTF(found, "nodemask=%s ref to chan", nodemask[stDest[i].condNodemaskIdx]->name);
      if (!found)
        stDest[i].condNodemaskIdx = 0;
    }
    else
      stDest[i].condNodemaskIdx = 0;
  G_UNUSED(alias_cnt);
}
void AnimationGraph::initStates(const DataBlock &stDescBlk)
{
  FastNameMap chanNm;
  char tmpbuf[SIZE_OF_FATAL_CTX_TMPBUF];
  int nid_chan = stDescBlk.getNameId("chan");
  int nid_state = stDescBlk.getNameId("state"), sidx = 0;
  int nid_alias = stDescBlk.getNameId("state_alias");
  const char *def_node_name = stDescBlk.getStr("defNodeName", "");
  float defMorphTime = stDescBlk.getReal("defMorphTime", 0.15f);
  float defMinTsc = stDescBlk.getReal("minTimeScale", 0.f);
  float defMaxTsc = stDescBlk.getReal("maxTimeScale", FLT_MAX);

  registerBlendNode(nullAnim = new AnimBlendNodeNull, "null");
  for (int i = 0, cidx = 0, nidx = 0; i < stDescBlk.blockCount(); i++)
    if (stDescBlk.getBlock(i)->getBlockNameId() == nid_chan)
    {
      chanNm.addNameId(stDescBlk.getBlock(i)->getStr("name", ""));
      const char *fifo3 = stDescBlk.getBlock(i)->getStr("fifo3", NULL);
      IAnimBlendNode *n = getBlendNodePtr(fifo3);
      stDest[cidx].fifo = (n && n->isSubOf(AnimBlendCtrl_Fifo3CID)) ? static_cast<AnimBlendCtrl_Fifo3 *>(n) : NULL;
      G_ASSERTF(stDest[cidx].fifo.get(), "fifo=%s", fifo3);
      if (stDest[cidx].defNodemaskIdx >= 0)
      {
        stDest[cidx].defNodemaskIdx = nidx + 1;
        nidx++;
      }
      else
        stDest[cidx].defNodemaskIdx = 0;
      cidx++;
    }

  Tab<int> stateStrIndices(framemem_ptr());
  for (int i = 0; i < stDescBlk.blockCount(); i++)
    if (stDescBlk.getBlock(i)->getBlockNameId() == nid_state)
    {
      const DataBlock &b = *stDescBlk.getBlock(i);
      const char *name = b.getStr("name", "");
#if DAGOR_DBGLEVEL > 0
      const char *tag = b.getStr("tag", "");
      int tag_mask = (*tag ? ((tagNames.addNameId(tag) + 1) << STATE_TAG_SHIFT) : 0);
      G_ASSERTF(tagNames.nameCount() < 255, "too many tags, on tag=<%s>", b.getStr("tag", ""));
#else
      int tag_mask = 0;
#endif
      int id = stateNames.addStrId(name, stateNames.strCount() | tag_mask);
      stateStrIndices.push_back(id);
      float defDur = b.getReal("forceDur", -1);
      float defSpd = b.getReal("forceSpd", -1);
      float stMorphTime = b.getReal("morphTime", defMorphTime);
      float stMinTsc = b.getReal("minTimeScale", defMinTsc);
      float stMaxTsc = b.getReal("maxTimeScale", defMaxTsc);
      G_ASSERTF(id * stDest.size() == sidx, "duplicate state name <%s>?", b.getStr("name", ""));

      int def_node_id = StateRec::NODEID_NULL;
      if (strcmp(def_node_name, "*") == 0)
        def_node_id = StateRec::NODEID_LEAVECUR;
      for (int j = 0; j < stDest.size(); j++)
        stRec[sidx + j].nodeId = def_node_id, stRec[sidx + j].morphTime = stMorphTime;

      for (int j = 0; j < b.blockCount(); j++)
      {
        const DataBlock &b2 = *b.getBlock(j);
        int cid = chanNm.getNameId(b2.getBlockName());
        const char *node_nm = b2.getStr("name", def_node_name);
        G_ASSERTF(cid >= 0, "chan=%s", b2.getBlockName());
        if (!node_nm[0])
          stRec[sidx + cid].nodeId = StateRec::NODEID_NULL;
        else if (strcmp(node_nm, "*") == 0)
          stRec[sidx + cid].nodeId = StateRec::NODEID_LEAVECUR;
        else
        {
          stRec[sidx + cid].nodeId = getBlendNodeId(node_nm);
          if (IAnimBlendNode *n = getBlendNodePtr(stRec[sidx + cid].nodeId))
            if (stDest[cid].defNodemaskIdx > 0 && !n->isSubOf(AnimBlendNodeContinuousLeafCID) &&
                !n->isSubOf(AnimBlendNodeStillLeafCID) && !n->isSubOf(AnimBlendNodeParametricLeafCID) &&
                !n->isSubOf(AnimBlendNodeSingleLeafCID) && !n->isSubOf(AnimBlendNodeNullCID) && !n->isSubOf(AnimBlendCtrl_HubCID) &&
                !n->isSubOf(AnimBlendCtrl_1axisCID) && !n->isSubOf(AnimBlendCtrl_RandomSwitcherCID) &&
                !n->isSubOf(AnimBlendCtrl_LinearPolyCID) && !n->isSubOf(AnimBlendCtrl_ParametricSwitcherCID) &&
                !n->isSubOf(AnimBlendCtrl_SetMotionMatchingTagCID))
            {
              G_ASSERTF(0, "type of node=%s  is not supported for animStates", node_nm);
              stRec[sidx + cid].nodeId = StateRec::NODEID_NULL;
            }
        }
        G_ASSERTF(stRec[sidx + cid].nodeId != -1, "node=%s", node_nm);
        stRec[sidx + cid].morphTime = b2.getReal("morphTime", stMorphTime);
        stRec[sidx + cid].forcedStateDur = b2.getReal("forceDur", defDur);
        stRec[sidx + cid].forcedStateSpd = b2.getReal("forceSpd", defSpd);
        stRec[sidx + cid].minTimeScale = b2.getReal("minTimeScale", stMinTsc);
        stRec[sidx + cid].maxTimeScale = b2.getReal("maxTimeScale", stMaxTsc);
      }
      sidx += stDest.size();
    }

  for (int i = 0; i < stDescBlk.blockCount(); i++)
    if (stDescBlk.getBlock(i)->getBlockNameId() == nid_alias)
    {
      const DataBlock &b = *stDescBlk.getBlock(i);
      const char *name = b.getStr("name", "");
      G_ASSERTF_CONTINUE(stateNames.getStrId(name) == -1, "duplicate state name %s in state_alias{}", name);
      const char *sameAs = b.getStr("sameAs", "");
      int alias_id = stateNames.getStrId(sameAs);
      G_ASSERTF_CONTINUE(alias_id != -1, "undefined state name %s referenced in state_alias{}", sameAs);
#if DAGOR_DBGLEVEL > 0
      const char *tag = b.getStr("tag", "");
      int tag_mask = (*tag ? ((tagNames.addNameId(tag) + 1) << STATE_TAG_SHIFT) : 0);
      G_ASSERTF(tagNames.nameCount() < 255, "too many tags, on tag=<%s>", b.getStr("tag", ""));
#else
      int tag_mask = 0;
#endif
      stateNames.setStrId(name, (alias_id & ~STATE_TAG_MASK) | STATE_ALIAS_FLG | tag_mask);
    }
  G_ASSERT(sidx == stRec.size());

  // check anims sufficient for states
  int err_cnt = 0;
  for (int si = 0; si < stRec.size() / stDest.size(); si++)
  {
    dag::ConstSpan<StateRec> s = getState(si);

    for (int i = 0; i < s.size(); i++)
      if (s[i].nodeId == StateRec::NODEID_NULL || s[i].nodeId == StateRec::NODEID_LEAVECUR)
        continue; // skip
      else
      {
        int mask_ofs = stDest[i].defNodemaskIdx;
        IAnimBlendNode *n = getBlendNodePtr(s[i].nodeId + mask_ofs);
        if (!n || (n->isSubOf(AnimBlendNodeNullCID) && n != nullAnim))
        {
          logerr("bad(missing) anim used for state(%s:%s, %d:%d ofs=%d)=%p (%s)%s", stateNames.getStrSlow(stateStrIndices[si]),
            chanNm.getName(i), si, i, mask_ofs, n, getAnimNodeName(s[i].nodeId), dgs_get_fatal_context(tmpbuf, sizeof(tmpbuf)));
          err_cnt++;
        }
        if (stDest[i].condNodemaskTarget >= 0)
        {
          mask_ofs = stDest[i].condNodemaskIdx;
          n = getBlendNodePtr(s[i].nodeId + mask_ofs);
          if (!n || (n->isSubOf(AnimBlendNodeNullCID) && n != nullAnim))
          {
            logerr("bad(missing) anim used for state(%s:%s, %d:%d ofs=%d)=%p (%s) cond=%d%s",
              stateNames.getStrSlow(stateStrIndices[si]), chanNm.getName(i), si, i, mask_ofs, n, getAnimNodeName(s[i].nodeId),
              stDest[i].condNodemaskTarget, dgs_get_fatal_context(tmpbuf, sizeof(tmpbuf)));
            err_cnt++;
          }
        }
      }
  }
  if (err_cnt)
    ANIM_ERR("error count: %d, see logerr for details", err_cnt);
}
void AnimationGraph::enqueueState(IPureAnimStateHolder &st, dag::ConstSpan<StateRec> state, float force_dur, float force_speed)
{
  TIME_PROFILE(AnimationGraph__enqueueState);
  if (state.size() != stDest.size())
    return;
  for (int i = 0; i < stDest.size(); i++)
    if (state[i].nodeId == StateRec::NODEID_NULL)
      stDest[i].fifo->enqueueState(st, nullAnim, state[i].morphTime, 0);
    else if (state[i].nodeId != StateRec::NODEID_LEAVECUR)
    {
      G_ASSERTF(int(stDest[i].condNodemaskTarget) < int(state.size()), "stDest[%d].condNodemaskTarget=%d state.size()=%d", i,
        stDest[i].condNodemaskTarget, state.size());
      int mask_ofs = stDest[i].defNodemaskIdx;
      if (stDest[i].condNodemaskTarget >= 0 &&
          (state[stDest[i].condNodemaskTarget].nodeId == StateRec::NODEID_NULL ||
            (state[stDest[i].condNodemaskTarget].nodeId == StateRec::NODEID_LEAVECUR && stDest[i].fifo->isEnqueued(st, nullAnim))))
        mask_ofs = stDest[i].condNodemaskIdx;
      // debug("enque %d: %d+%d  %s -> %s", i, state[i].nodeId, mask_ofs,
      //   getBlendNodeName(getBlendNodePtr(state[i].nodeId + mask_ofs)), getBlendNodeName(stDest[i].fifo));
      IAnimBlendNode *n = getBlendNodePtr(state[i].nodeId + mask_ofs);
      int tsp = n->getTimeScaleParamId(st);
      if (tsp > 0)
      {
        float dur = force_dur > 0 ? force_dur : state[i].forcedStateDur;
        float spd = force_speed > 0 ? force_speed : state[i].forcedStateSpd;
        float spd0 = n->getAvgSpeed(st);
        if (spd > 0 && spd0 > 0)
          st.setParam(tsp, clamp(spd / spd0, state[i].minTimeScale, state[i].maxTimeScale));
        else
          st.setParam(tsp, dur > 0 ? n->getDuration(st) / state[i].forcedStateDur : 1.0f);
      }
      if (!stDest[i].fifo->isEnqueued(st, n))
        n->resume(st, true);
      stDest[i].fifo->enqueueState(st, n, state[i].morphTime, 0);
    }
}

void AnimationGraph::setStateSpeed(IPureAnimStateHolder &st, dag::ConstSpan<StateRec> state, float force_speed)
{
  TIME_PROFILE(AnimationGraph__setStateSpeed);
  if (state.size() != stDest.size())
    return;
  for (int i = 0; i < stDest.size(); i++)
    if (state[i].nodeId != StateRec::NODEID_LEAVECUR && state[i].nodeId != StateRec::NODEID_NULL)
    {
      int mask_ofs = stDest[i].defNodemaskIdx;
      if (stDest[i].condNodemaskTarget >= 0 &&
          (state[stDest[i].condNodemaskTarget].nodeId == StateRec::NODEID_NULL ||
            (state[stDest[i].condNodemaskTarget].nodeId == StateRec::NODEID_LEAVECUR && stDest[i].fifo->isEnqueued(st, nullAnim))))
        mask_ofs = stDest[i].condNodemaskIdx;
      IAnimBlendNode *n = getBlendNodePtr(state[i].nodeId + mask_ofs);
      int tsp = n->getTimeScaleParamId(st);
      if (tsp > 0)
      {
        float spd = force_speed;
        float spd0 = n->getAvgSpeed(st);
        if (spd0 > 0)
          st.setParam(tsp, clamp(spd / spd0, state[i].minTimeScale, state[i].maxTimeScale));
      }
    }
}

void AnimationGraph::onSingleAnimFinished(IPureAnimStateHolder &st, IAnimBlendNode *n)
{
  if (!stDest.size())
    return;
  for (int i = 0; i < stDest.size(); i++)
    if (stDest[i].morphTimeToSetNullOnSingleAnimFinished >= 0 && stDest[i].fifo->isEnqueued(st, n))
      stDest[i].fifo->enqueueState(st, nullAnim, stDest[i].morphTimeToSetNullOnSingleAnimFinished, 0);
}

int AnimationGraph::addPbcWtParamId(const char *name)
{
  int id = paramNames.getNameId(name);
  if (id < 0)
    return -1;
  int wt_id = find_value_idx(pbcWtParam, id);
  if (wt_id < 0)
  {
    wt_id = pbcWtParam.size();
    pbcWtParam.push_back(id);
  }
  return wt_id;
}
int AnimationGraph::addParamId(const char *name, int type)
{
  int id = paramNames.addNameId(name);
  // DEBUG_CTX("allocated (id=%d) for \"%s\", type=%d", id, name, type);
  if (id == paramTypes.size())
  {
    paramTypes.push_back(type);
    if (type == AnimCommonStateHolder::PT_TimeParam)
      paramTimeInd.push_back(id);
    return id;
  }
  if (paramTypes[id] == type)
    return id;
  ANIM_ERR("adding param <%s> type=%d while such param with type %d already exists!", name, type, paramTypes[id]);
  return -1;
}
int AnimationGraph::addInlinePtrParamId(const char *name, size_t size_bytes, int type)
{
  int id = paramNames.addNameId(name);
  if (id < 0)
    return -1;
  size_t word_sz = sizeof(AnimCommonStateHolder::ParamState);
  size_t num_words = size_bytes ? (size_bytes + word_sz - 1) / word_sz : 1;

  if (id == paramTypes.size())
  {
    int pt_base = append_items(paramTypes, num_words);
    paramTypes[pt_base] = type;
    if (type == AnimCommonStateHolder::PT_Fifo3)
      paramFifo3Ind.push_back(id);
    for (int i = 1; i < num_words; i++)
    {
      paramTypes[pt_base + i] = IPureAnimStateHolder::PT_Reserved;
      G_VERIFYF(paramNames.addNameId(String(0, "?%d", pt_base + i)) == pt_base + i,
        "failed to create unique nameId: resv[%d] for param <%s> size_bytes=%d", i, name, size_bytes);
    }
    // DEBUG_CTX("allocated %d param recs (id=%d) for inline ptr \"%s\", sz=%d, type=%d", num_words, id, name, size_bytes, type);
    return id;
  }

  G_ASSERTF_RETURN(AnimCommonStateHolder::getInlinePtrWords(paramTypes, id) * word_sz >= size_bytes, -1,
    "adding param <%s> type=%d while such param with type %d already exists (max_size=%d need_size=%d)!", name, type, paramTypes[id],
    AnimCommonStateHolder::getInlinePtrWords(paramTypes, id), size_bytes);
  return id;
}

int AnimationGraph::getParamId(const char *name, int type) const
{
  int id = paramNames.getNameId(name);
  if (id < 0)
    return -1;
  if (paramTypes[id] == type)
    return id;
  DEBUG_CTX("param <%s> of type %d is absent; present with type %d", name, type, paramTypes[id]);
  return -1;
}

int AnimationGraph::registerBlendNode(IAnimBlendNode *n, const char *name, const char *nm_suffix)
{
  int id = nodeNames.addNameId(nm_suffix ? String(0, "%s:%s", name, nm_suffix).str() : name);
  if (id >= nodePtr.size())
    nodePtr.resize(id + 1);
  if (nodePtr[id].get())
    nodePtr[id]->setAnimNodeId(-1);
  nodePtr[id] = n;
  n->setAnimNodeId(id);
  return id;
}
void AnimationGraph::unregisterBlendNode(IAnimBlendNode *n)
{
  if (!n)
    return;
  for (int i = nodePtr.size() - 1; i >= 0; i--)
    if (nodePtr[i] == n)
    {
      n->setAnimNodeId(-1);
      nodePtr[i] = NULL;
    }
}

void AnimationGraph::replaceRoot(IAnimBlendNode *new_root)
{
  //==
  root = new_root;
}
void AnimationGraph::resetBlendNodes(IPureAnimStateHolder &st)
{
  const DataBlock *fifo3Init = initState ? initState->getBlockByName("fifo3") : NULL;
  if (fifo3Init && fifo3Init->isEmpty())
    fifo3Init = NULL;
  for (int i = 0; i < nodePtr.size(); i++)
  {
    if (!nodePtr[i])
      continue;
    nodePtr[i]->reset(st);
    if (fifo3Init && nodePtr[i]->isSubOf(AnimBlendCtrl_Fifo3CID))
      if (const char *nm = fifo3Init->getStr(getBlendNodeName(nodePtr[i]), NULL))
        if (IAnimBlendNode *n = getBlendNodePtr(nm))
          static_cast<AnimBlendCtrl_Fifo3 *>(nodePtr[i].get())->enqueueState(st, n, 0, 0);
  }
}
void AnimationGraph::sortPbCtrl(const DataBlock &ord)
{
  // force order
  for (int i = 0, nid = ord.getNameId("name"), dest = 0; i < ord.paramCount(); i++)
    if (ord.getParamNameId(i) == nid && ord.getParamType(i) == ord.TYPE_STRING)
    {
      IAnimBlendNode *n = getBlendNodePtr(ord.getStr(i));
      if (!n)
      {
        logerr("unresolved postBlendCtrlOrder[%d]=%s", i, ord.getStr(i));
        continue;
      }
      if (!n->isSubOf(AnimPostBlendCtrlCID))
      {
        logerr("postBlendCtrlOrder[%d]=%s is not PBC!", i, ord.getStr(i));
        continue;
      }
      int idx = find_value_idx(blender.pbCtrl, static_cast<AnimPostBlendCtrl *>(n));
      if (idx < dest)
      {
        logerr("double mention of postBlendCtrlOrder[%d]=%s ?", i, ord.getStr(i));
        continue;
      }
      if (idx > dest)
      {
        Ptr<AnimPostBlendCtrl> pbn = blender.pbCtrl[idx];
        erase_items(blender.pbCtrl, idx, 1);
        insert_items(blender.pbCtrl, dest, 1, &pbn);
      }
      dest++;
    }

  // update pbcId
  for (int i = 0; i < blender.pbCtrl.size(); i++)
    blender.pbCtrl[i]->pbcId = i;
}

void AnimationGraph::setIgnoredAnimationNodes(const Tab<int> &nodes) { ignoredAnimationNodes = nodes; }
bool AnimationGraph::isNodeWithIgnoredAnimation(int nodeId) { return tabutils::find(ignoredAnimationNodes, nodeId); }

void AnimationGraph::getUsedBlendNodes(eastl::hash_map<IAnimBlendNode *, bool> &usedNodes)
{
  root->collectUsedBlendNodes(*this, usedNodes);
  usedNodes.emplace(root, true);

  if (stDest.size() > 0)
    for (int si = 0; si < stRec.size() / stDest.size(); si++)
    {
      dag::ConstSpan<AnimationGraph::StateRec> s = getState(si);

      for (int i = 0; i < s.size(); i++)
        if (s[i].nodeId == AnimationGraph::StateRec::NODEID_NULL || s[i].nodeId == AnimationGraph::StateRec::NODEID_LEAVECUR)
          continue; // skip
        else
        {
          int mask_ofs = stDest[i].defNodemaskIdx;
          IAnimBlendNode *n = getBlendNodePtr(s[i].nodeId + mask_ofs);
          if (n)
          {
            usedNodes.emplace(n, true);
            n->collectUsedBlendNodes(*this, usedNodes);
          }
          if (stDest[i].condNodemaskTarget >= 0)
          {
            mask_ofs = stDest[i].condNodemaskIdx;
            n = getBlendNodePtr(s[i].nodeId + mask_ofs);
            if (n)
            {
              usedNodes.emplace(n, true);
              n->collectUsedBlendNodes(*this, usedNodes);
            }
          }
        }
    }
}

static void delete_ctx_recursive(AnimBlender::TlsContext *c)
{
  if (c->nextCtx)
    delete_ctx_recursive(c->nextCtx);
  c->clear();
  delete c;
}

AnimBlender::TlsContext AnimBlender::mainCtx;
int AnimBlender::mainCtxRefCount = 0;
static int bytesAllocatedForTlsContexts = 0;

void AnimBlender::releaseContextMemory()
{
  if (bytesAllocatedForTlsContexts)
    debug("%s mainCtxRefCount=%d bytesAllocatedForTlsContexts=%d", __FUNCTION__, mainCtxRefCount, bytesAllocatedForTlsContexts);
  if (mainCtxRefCount > 0)
  {
    logwarn("%s skipped since mainCtxRefCount=%d", __FUNCTION__, mainCtxRefCount);
    return;
  }
  G_ASSERT(is_main_thread());
  if (mainCtx.nextCtx)
    delete_ctx_recursive(mainCtx.nextCtx);
  mainCtx.clear();
  bytesAllocatedForTlsContexts = 0;
}
static struct FinalTlsContextMemReleaser
{
  ~FinalTlsContextMemReleaser()
  {
    if (AnimBlender::mainCtx.nextCtx)
      delete_ctx_recursive(AnimBlender::mainCtx.nextCtx);
    AnimBlender::mainCtx.clear();
  }
} final_tls_ctx_releaser;

//
// Animation blender
//
AnimBlender::AnimBlender() : bnl(midmem), pbCtrl(midmem) { interlocked_increment(mainCtxRefCount); }
AnimBlender::~AnimBlender()
{
  interlocked_decrement(mainCtxRefCount);

  targetNode.reset();
  clear_and_shrink(bnl);
  clear_and_shrink(pbCtrl);
  targetNode.reset();
  clear_and_shrink(bnlChan);
  nodeListValid = false;
}

AnimBlender::TlsContext *AnimBlender::addNewContext(int64_t tid)
{
  TlsContext *ctx = new TlsContext;
  ctx->threadId = tid;

  TlsContext *prevnext = interlocked_acquire_load_ptr(mainCtx.nextCtx), *next;
  do
  {
    next = prevnext;
    interlocked_release_store_ptr(ctx->nextCtx, next);
    prevnext = interlocked_compare_exchange_ptr(mainCtx.nextCtx, ctx, next);
  } while (prevnext != next);

  return ctx;
}

AnimBlender::TlsContext &AnimBlender::selectCtx(intptr_t (*irq)(int, intptr_t, intptr_t, intptr_t, void *), void *irq_arg)
{
  TlsContext &tls = getUnpreparedCtx();
  if (tls.bnlWt.size() != bnl.size() || tls.pbcWt.size() != pbCtrl.size() || tls.readyMark.size() != targetNode.nameCount())
    tls.rebuildNodeList(bnl.size(), pbCtrl.size(), targetNode.nameCount());
  tls.irq = irq;
  tls.irqArg = irq_arg;
  return tls;
}

void AnimBlender::buildNodeList()
{
  int i, j;

  if (nodeListValid)
    return;

  // reset helper structures
  targetNode.reset();
  bnlChan.clear();
  bnlChan.resize(bnl.size());

  // preallocate map storage
  for (i = bnl.size() - 1; i >= 0; i--)
  {
    if (!bnl[i] || !bnl[i]->anim)
      continue;

    AnimData::Anim &anim = bnl[i]->anim->anim;
    bnlChan[i].alloc(anim.pos.nodeNum, anim.scl.nodeNum, anim.rot.nodeNum);
  }

  // create name map and setup node mapping
  for (i = bnl.size() - 1; i >= 0; i--)
  {
    if (!bnl[i] || !bnl[i]->anim)
      continue;

    AnimData::Anim &anim = bnl[i]->anim->anim;
    ChannelMap &cm = bnlChan[i];

    for (j = anim.pos.nodeNum - 1; j >= 0; j--)
      cm.chanNodeId[0][j] = targetNode.addNameId(anim.pos.nodeName[j]);
    for (j = anim.scl.nodeNum - 1; j >= 0; j--)
      cm.chanNodeId[1][j] = targetNode.addNameId(anim.scl.nodeName[j]);
    for (j = anim.rot.nodeNum - 1; j >= 0; j--)
      cm.chanNodeId[2][j] = targetNode.addNameId(anim.rot.nodeName[j]);
  }

  nodeListValid = true;
}

void AnimBlender::TlsContext::rebuildNodeList(int bnl_count, int pbc_count, int target_node_count)
{
  chPos = NULL;
  chScl = NULL;
  chRot = NULL;
  chPrs = NULL;

  int req_sz = +3 * sizeof(NodeWeight) * target_node_count + sizeof(PrsResult) * target_node_count +
               sizeof(NodeSamplers) * target_node_count + 3 * sizeof(WeightedNode) * target_node_count + elem_size(bnlWt) * bnl_count +
               elem_size(bnlCT) * bnl_count + elem_size(pbcWt) * pbc_count + elem_size(readyMark) * target_node_count + 8;
  char *base = (char *)dataPtr;
  if (req_sz <= dataPtrSz)
  {
    if (int ofs = uintptr_t(dataPtr) & 0xF)
      base += 0x10 - ofs;
  }
  else
  {
    if (dataPtr)
      midmem->free(dataPtr);
    interlocked_add(bytesAllocatedForTlsContexts, -dataPtrSz);
    dataPtrSz = req_sz;
    dataPtr = midmem->alloc(dataPtrSz);
    base = (char *)dataPtr;
    interlocked_add(bytesAllocatedForTlsContexts, dataPtrSz);
  }

  const char *data_start = base;
  G_UNREFERENCED(data_start);

  wtPos = (NodeWeight *)base;
  base += sizeof(NodeWeight) * target_node_count;

  wtScl = (NodeWeight *)base;
  base += sizeof(NodeWeight) * target_node_count;

  wtRot = (NodeWeight *)base;
  base += sizeof(NodeWeight) * target_node_count;

  if (target_node_count & 1)
    base += 8;

  chPrs = (PrsResult *)base;
  base += sizeof(PrsResult) * target_node_count;

  chXfm = (NodeSamplers *)base;
  base += sizeof(NodeSamplers) * target_node_count;

  chPos = (WeightedNode *)base;
  base += sizeof(WeightedNode) * target_node_count;

  chScl = (WeightedNode *)base;
  base += sizeof(WeightedNode) * target_node_count;

  chRot = (WeightedNode *)base;
  base += sizeof(WeightedNode) * target_node_count;

  // prepare node-wise buffer for blending
  bnlWt.set((real *)base, bnl_count);
  base += data_size(bnlWt);
  bnlCT.set((int *)base, bnl_count);
  base += data_size(bnlCT);
  pbcWt.set((real *)base, pbc_count);
  base += data_size(pbcWt);
  readyMark.set((uint8_t *)base, target_node_count);
  base += data_size(readyMark);

  G_ASSERTF(base - data_start <= dataPtrSz, "chDataSz=%d used=%d targetNode=%d", dataPtrSz, base - data_start, target_node_count);
}

int AnimBlender::registerBlendNodeLeaf(AnimBlendNodeLeaf *n)
{
  nodeListValid = false;
  for (int i = 0; i < bnl.size(); i++)
    if (!bnl[i])
    {
      bnl[i] = n;
      return i;
    }
  return append_items(bnl, 1, &n);
}
void AnimBlender::unregisterBlendNodeLeaf(int id, AnimBlendNodeLeaf *n)
{
  if ((unsigned)id >= bnl.size())
  {
    if (!bnl.empty()) // On animgraph dtor bnl might be already cleared
      ANIM_ERR("invalid id=%d, n=%p (bnl.count=%d)", id, n, bnl.size());
    return;
  }
  if (bnl[id] && bnl[id] != n)
  {
    ANIM_ERR("invalid id=%d, n=%p (bnl[%d]=%p)", id, n, id, (void *)bnl[id]);
    return;
  }
  bnl[id] = NULL;
}
int AnimBlender::registerPostBlendCtrl(AnimPostBlendCtrl *n)
{
  for (int i = 0; i < pbCtrl.size(); i++)
    if (!pbCtrl[i])
    {
      pbCtrl[i] = n;
      return i;
    }
  int id = append_items(pbCtrl, 1, &n);
  return id;
}
void AnimBlender::unregisterPostBlendCtrl(int id, AnimPostBlendCtrl *n)
{
  if ((unsigned)id >= pbCtrl.size())
  {
    if (!pbCtrl.empty()) // In animgraph dtor pbCtrl might be already cleared
      ANIM_ERR("invalid id=%d, n=%p (pbCtrl.count=%d)", id, n, pbCtrl.size());
    return;
  }
  if (pbCtrl[id] && pbCtrl[id] != n)
  {
    ANIM_ERR("invalid id=%d, n=%p (pbCtrl[%d]=%p)", id, n, id, (void *)pbCtrl[id]);
    return;
  }
  pbCtrl[id] = NULL;
}

void log_active_bnls(const dag::Span<real> &bnlWt, PtrTab<AnimBlendNodeLeaf> &bnl, int bnlNum, const AnimationGraph &graph)
{
  logdbg("Active bnls:");
  for (int bnlIdx = 0; bnlIdx < bnlNum; bnlIdx++)
  {
    if (fabsf(bnlWt[bnlIdx]) <= 1e-6f || !bnl[bnlIdx] || !bnl.data()[bnlIdx]->anim)
      continue;
    logdbg(" - %s (weight: %lf)", graph.getAnimNodeName(bnl[bnlIdx]->getAnimNodeId()), bnlWt[bnlIdx]);
  }
}

static PerformanceTimer2 __pm(false);
static PerformanceTimer2 __pm_quat(false);
static PerformanceTimer2 __pm_p3(false);
static PerformanceTimer2 __pm_prepare(false);
static PerformanceTimer2 __pm_outer(false);
static int __pm_anim_cnt = 0;

bool AnimBlender::blend(TlsContext &tls, IPureAnimStateHolder &st, IAnimBlendNode *root, const CharNodeModif *cmm,
  const AnimationGraph &graph)
{
  G_UNUSED(graph);
  TIME_PROFILE(AnimationGraph__blend);
#if DUMP_ANIM_BLENDING
  write_blending(st);
#endif
#if MEASURE_PERF
  perf_tm.go();
#endif

  int nodenum = targetNode.nameCount(), i, j, bnlNum = bnl.size();

  dag::Span<real> bnlWt = tls.bnlWt;
  dag::Span<int> bnlCT = tls.bnlCT;
  dag::Span<real> pbcWt = tls.pbcWt;
  dag::Span<uint8_t> readyMark = tls.readyMark;
  NodeWeight *wtPos = tls.wtPos, *wtScl = tls.wtScl, *wtRot = tls.wtRot;
  NodeSamplers *chXfm = tls.chXfm;
  WeightedNode *chPos = tls.chPos, *chScl = tls.chScl;
  WeightedNode *chRot = tls.chRot;
  PrsResult *chPrs = tls.chPrs;

  if (PROFILE_BLENDING)
  {
    __pm_anim_cnt++;
    __pm.go();
    __pm_outer.pause();
  }

  // build animation weights
  mem_set_0(bnlWt);
  mem_set_0(pbcWt);

  IAnimBlendNode::BlendCtx bctx(st, tls, true);
  bctx.lastDt = st.getParam(AnimationGraph::PID_GLOBAL_LAST_DT);
  root->buildBlendingList(bctx, 1.0f);

  // added: check for null animation
  bool haveBnl = false;
  int charDepNodeId = cmm ? cmm->chanNodeId() : -1;
  for (i = 0; i < bnlNum; i++)
  {
    if (bnlWt.data()[i] <= 0.f || !bnl.data()[i] || !bnl.data()[i]->anim)
      continue;
    haveBnl = true;
    break;
  }
  if (!haveBnl)
  {
#if MEASURE_PERF
    perf_tm.pause();
#endif
    return false;
  }

  // clear blending lists
  memset(wtPos, 0, nodenum * sizeof(NodeWeight) * 3); // zero wtPos, wtScl, wtRot
  mem_set_0(readyMark);

  if (PROFILE_BLENDING)
    __pm_prepare.go();

  // construct blending lists
  enum
  {
    BMOD_INDEX_MASK = (1 << 5) - 1,
    BMOD_ADDITIVE = 1 << 5,
    BMOD_CHARDEP = 1 << 6,
    BMOD_MOTION_MATCHING_POSE = 1 << 7
  };

  if (bctx.irq(GIRQT_GetMotionMatchingPose, (intptr_t)&tls, 0, 0) == GIRQR_MotionMatchingPoseApplied)
    for (i = 0; i < nodenum; i++)
    {
      if (wtPos[i].totalNum)
        chPos[i].blendMod[0] = BMOD_MOTION_MATCHING_POSE;
      if (wtRot[i].totalNum)
        chRot[i].blendMod[0] = BMOD_MOTION_MATCHING_POSE;
      if (wtScl[i].totalNum)
        chScl[i].blendMod[0] = BMOD_MOTION_MATCHING_POSE;
    }

  for (i = 0; i < nodenum; i++)
  {
    chXfm[i].lastBnl = -1;
    chXfm[i].totalNum = 0;
  }

  auto add_sampler = [](NodeSamplers &smp, int i, int j, int targetChN, const AnimDataChan &chan) {
    G_UNUSED(targetChN);
    if (DAGOR_UNLIKELY(smp.totalNum >= MAX_ANIMS_IN_NODE))
    {
#if DAGOR_DBGLEVEL > 0
      LOGERR_ONCE("xfm: Need to increase max anims for node %d/%s, j=%d, targetChN=%d: %dn", i, chan.nodeName[j].get(), j, targetChN,
        smp.totalNum);
#endif
      return false;
    }
    auto &s = smp.samplers[smp.totalNum];
    s.trackId = chan.nodeTrack[j];
    smp.totalNum++;
    smp.lastBnl = i;
    return true;
  };

  for (i = 0; i < bnlNum; i++)
  {
    if (fabsf(bnlWt[i]) <= 1e-6f || !bnl[i] || !bnl.data()[i]->anim)
      continue;

    real wa_w = bnlWt.data()[i], w;
    bool additive = bnl.data()[i]->additive;

    const AnimData *anim = bnl.data()[i]->anim;
    const AnimDataChan &pos = anim->anim.pos;
    const AnimDataChan &scl = anim->anim.scl;
    const AnimDataChan &rot = anim->anim.rot;

    const ChannelMap &cm = bnlChan[i];
    int wa_pos = bnlCT[i];
    bool charDep = cmm && bnl.data()[i]->foreignAnimation;
    uint8_t bmod = (additive ? BMOD_ADDITIVE : 0) | (charDep ? BMOD_CHARDEP : 0);

    for (j = 0; j < pos.nodeNum; j++)
    {
      int targetChN = cm.chanNodeId[0][j];

      if (targetChN < 0)
        continue;
      w = pos.nodeWt[j] * wa_w;
      NodeWeight &ch_w = wtPos[targetChN];
      WeightedNode &ch = chPos[targetChN];
      if (ch_w.totalNum > 0 && ch.blendMod[0] == BMOD_MOTION_MATCHING_POSE && !additive)
        w *= (1 - ch.blendWt[0]);
      if (fabsf(w) <= 1e-6f)
        continue;


      if (ch_w.totalNum >= MAX_ANIMS_IN_NODE)
      {
#if DAGOR_DBGLEVEL > 0
        static bool logged_ = false;
        if (!logged_)
        {
          logged_ = true;
          log_active_bnls(bnlWt, bnl, bnlNum, graph);
          logerr("pos: Need to increase max anims for node %d/%s, j=%d, targetChN=%d: %d. See log for active bnls dump.", i,
            pos.nodeName[j].get(), j, targetChN, ch_w.totalNum);
        }
#endif
        continue;
      }

      NodeSamplers &smp = chXfm[targetChN];
      // we couldn't have already added animation for this BNL, unless there are nodes with duplicate names
      if (smp.lastBnl != i)
        if (!add_sampler(smp, i, j, targetChN, pos))
          continue;

      ch.blendWt[ch_w.totalNum] = w;
      ch.blendMod[ch_w.totalNum] = bmod | (smp.totalNum - 1);
      if (charDep && charDepNodeId != targetChN)
        ch.blendMod[ch_w.totalNum] &= ~BMOD_CHARDEP;
      ch.readyFlg = (ch_w.totalNum ? ch.readyFlg : 0) | (additive ? RM_POS_A : RM_POS_B);
      ch_w.totalNum++;
      if (!additive)
        ch_w.wTotal += w;
      else if (ch_w.totalNum == 1)
        ch_w.wTotal = 1.0f; //< only additive anims for node, mark wTotal as 'used'
    }

    for (j = 0; j < scl.nodeNum; j++)
    {
      int targetChN = cm.chanNodeId[1][j];

      if (targetChN < 0)
        continue;
      if (charDep && charDepNodeId != targetChN)
        continue;
      w = scl.nodeWt[j] * wa_w;
      NodeWeight &ch_w = wtScl[targetChN];
      WeightedNode &ch = chScl[targetChN];
      if (ch_w.totalNum > 0 && ch.blendMod[0] == BMOD_MOTION_MATCHING_POSE && !additive)
        w *= (1 - ch.blendWt[0]);
      if (fabsf(w) <= 1e-6f)
        continue;

      if (ch_w.totalNum >= MAX_ANIMS_IN_NODE)
      {
#if DAGOR_DBGLEVEL > 0
        static bool logged_ = false;
        if (!logged_)
        {
          logged_ = true;
          log_active_bnls(bnlWt, bnl, bnlNum, graph);
          logerr("scl: Need to increase max anims for node %d/%s, j=%d, targetChN=%d: %d. See log for active bnls dump.", i,
            scl.nodeName[j].get(), j, targetChN, ch_w.totalNum);
        }
#endif
        continue;
      }

      NodeSamplers &smp = chXfm[targetChN];
      if (smp.lastBnl != i)
        if (!add_sampler(smp, i, j, targetChN, scl))
          continue;

      ch.blendWt[ch_w.totalNum] = w;
      ch.blendMod[ch_w.totalNum] = bmod | (smp.totalNum - 1);
      ch.readyFlg = (ch_w.totalNum ? ch.readyFlg : 0) | (additive ? RM_SCL_A : RM_SCL_B);
      ch_w.totalNum++;
      if (!additive)
        ch_w.wTotal += w;
      else if (ch_w.totalNum == 1)
        ch_w.wTotal = 1.0f; //< only additive anims for node, mark wTotal as 'used'
    }

    for (j = 0; j < rot.nodeNum; j++)
    {
      int targetChN = cm.chanNodeId[2][j];

      if (targetChN < 0)
        continue;
      w = rot.nodeWt[j] * wa_w;
      NodeWeight &ch_w = wtRot[targetChN];
      WeightedNode &ch = chRot[targetChN];
      if (ch_w.totalNum > 0 && ch.blendMod[0] == BMOD_MOTION_MATCHING_POSE && !additive)
        w *= (1 - ch.blendWt[0]);
      if (w <= 1e-6f)
        continue;

      if (ch_w.totalNum >= MAX_ANIMS_IN_NODE)
      {
#if DAGOR_DBGLEVEL > 0
        static bool logged_ = false;
        if (!logged_)
        {
          logged_ = true;
          log_active_bnls(bnlWt, bnl, bnlNum, graph);
          logerr("rot: Need to increase max anims for node %d/%s, j=%d, targetChN=%d: %d. See log for active bnls dump.", i,
            rot.nodeName[j].get(), j, targetChN, ch_w.totalNum);
        }
#endif
        continue;
      }

      NodeSamplers &smp = chXfm[targetChN];
      if (smp.lastBnl != i)
        if (!add_sampler(smp, i, j, targetChN, rot))
          continue;

      ch.blendWt[ch_w.totalNum] = w;
      ch.blendMod[ch_w.totalNum] = (bmod & ~BMOD_CHARDEP) | (smp.totalNum - 1);
      ch.readyFlg = (ch_w.totalNum ? ch.readyFlg : 0) | (additive ? RM_ROT_A : RM_ROT_B);
      ch_w.totalNum++;
      if (!additive)
        ch_w.wTotal += w;
      else if (ch_w.totalNum == 1)
        ch_w.wTotal = 1.0f; //< only additive anims for node, mark wTotal as 'used'
    }

    // create added samplers
    for (j = 0; j < nodenum; j++)
    {
      NodeSamplers &smp = chXfm[j];
      if (smp.lastBnl != i)
        continue;
      auto &s = smp.samplers[smp.totalNum - 1];
      new (&s.sampler, _NEW_INPLACE) decltype(s.sampler)(anim, s.trackId, wa_pos);
    }
  }
  if (PROFILE_BLENDING)
  {
    __pm_prepare.pause();
    __pm_p3.go();
  }

  // final passes to blend animation data
  vec4f cmm_scl = cmm ? v_perm_xyzd(cmm->pScale_id, v_zero()) : v_zero();
  vec4f cmm_ofs = cmm ? cmm->pOfs : v_zero();
  for (i = 0; i < nodenum; i++)
  {
    const NodeWeight &ch_w = wtPos[i];
    int bnum = ch_w.totalNum;
    if (!bnum || fabsf(ch_w.wTotal) < 1e-6f)
      continue;

    WeightedNode &ch = chPos[i];
    NodeSamplers &smp = chXfm[i];

    real *bwgt = ch.blendWt;
    uint8_t *bmod = ch.blendMod;
    vec4f w4, v, res = v_zero();
    bnum--;
    readyMark[i] |= ch.readyFlg;

    if (*bmod == BMOD_MOTION_MATCHING_POSE)
    {
      if (!bnum) // result is already in chPrs
        continue;
      v = chPrs[i].pos;
    }
    else
    {
      v = smp.samplers[*bmod & BMOD_INDEX_MASK].sampler.samplePos();
      if (*bmod & BMOD_CHARDEP)
        v = v_madd(v, cmm_scl, cmm_ofs);
    }

    if (!bnum && !(*bmod & BMOD_ADDITIVE) && fabsf(*bwgt) > 0)
      chPrs[i].pos = v;
    else
    {
      w4 = v_splats(*bwgt);
      vec4f inv_total = v_rcp(v_splats(ch_w.wTotal));
      res = v_mul(v, !(*bmod & BMOD_ADDITIVE) ? v_mul(w4, inv_total) : w4);
      bwgt++;
      bmod++;
      for (; bnum; bwgt++, bmod++, bnum--)
      {
        w4 = v_splats(*bwgt);
        v = smp.samplers[*bmod & BMOD_INDEX_MASK].sampler.samplePos();
        if (*bmod & BMOD_CHARDEP)
          v = v_madd(v, cmm_scl, cmm_ofs);

        res = v_madd(v, !(*bmod & BMOD_ADDITIVE) ? v_mul(w4, inv_total) : w4, res);
      }
      chPrs[i].pos = res;
    }
  }

  cmm_scl = cmm ? cmm->sScale : v_zero();
  for (i = 0; i < nodenum; i++)
  {
    const NodeWeight &ch_w = wtScl[i];
    int bnum = ch_w.totalNum;
    if (!bnum || fabsf(ch_w.wTotal) < 1e-6f)
      continue;

    WeightedNode &ch = chScl[i];
    NodeSamplers &smp = chXfm[i];

    real *bwgt = ch.blendWt;
    uint8_t *bmod = ch.blendMod;
    vec4f w4, v, res = v_zero(), additive_scl = V_C_ONE;
    bnum--;
    readyMark[i] |= ch.readyFlg;

    if (*bmod == BMOD_MOTION_MATCHING_POSE)
    {
      if (!bnum) // result is already in chPrs
        continue;
      v = chPrs[i].scl;
    }
    else
    {
      v = smp.samplers[*bmod & BMOD_INDEX_MASK].sampler.sampleScl();
      if (*bmod & BMOD_CHARDEP)
        v = v_mul(v, cmm_scl);
    }

    if (!bnum && !(*bmod & BMOD_ADDITIVE) && fabsf(*bwgt) > 0)
      chPrs[i].scl = v;
    else
    {
      w4 = v_splats(*bwgt);
      vec4f inv_total = v_rcp(v_splats(ch_w.wTotal));
      if (!(*bmod & BMOD_ADDITIVE))
        res = v_mul(v, v_mul(w4, inv_total));
      else
        additive_scl = v_lerp_vec4f(w4, V_C_ONE, v);

      bwgt++;
      bmod++;
      for (; bnum; bwgt++, bmod++, bnum--)
      {
        w4 = v_splats(*bwgt);
        v = smp.samplers[*bmod & BMOD_INDEX_MASK].sampler.sampleScl();
        if (*bmod & BMOD_CHARDEP)
          v = v_mul(v, cmm_scl);

        if (!(*bmod & BMOD_ADDITIVE))
          res = v_madd(v, v_mul(w4, inv_total), res);
        else
          additive_scl = v_mul(additive_scl, v_lerp_vec4f(w4, V_C_ONE, v));
      }
      chPrs[i].scl = (ch.readyFlg & RM_SCL_B) ? v_mul(res, additive_scl) : additive_scl;
    }
  }

  if (PROFILE_BLENDING)
  {
    __pm_p3.pause();
    __pm_quat.go();
  }

  for (i = 0; i < nodenum; i++)
  {
    const NodeWeight &ch_w = wtRot[i];
    int bnum = ch_w.totalNum;
    if (!bnum || fabsf(ch_w.wTotal) < 1e-6f)
      continue;

    WeightedNode &ch = chRot[i];
    NodeSamplers &smp = chXfm[i];

    real *bwgt = ch.blendWt;
    uint8_t *bmod = ch.blendMod;
    vec4f v, additive_rot = V_C_UNIT_0001, res = V_C_UNIT_0001;
    bnum--;
    readyMark[i] |= ch.readyFlg;

    if (*bmod == BMOD_MOTION_MATCHING_POSE)
    {
      if (!bnum) // result is already in chPrs
        continue;
      v = chPrs[i].rot;
    }
    else
      v = smp.samplers[*bmod & BMOD_INDEX_MASK].sampler.sampleRot();

    if (!bnum && !(*bmod & BMOD_ADDITIVE) && fabsf(*bwgt) > 0)
      chPrs[i].rot = v;
    else
    {
      float wsum = 0;
      if (!(*bmod & BMOD_ADDITIVE))
        res = v, wsum = *bwgt;
      else
        additive_rot = v_quat_lerp(v_splats(*bwgt), V_C_UNIT_0001, v);
      bwgt++;
      bmod++;
      for (; bnum; bwgt++, bmod++, bnum--)
      {
        v = smp.samplers[*bmod & BMOD_INDEX_MASK].sampler.sampleRot();
        if (!(*bmod & BMOD_ADDITIVE))
        {
          wsum += *bwgt;
          res = (wsum != *bwgt) ? v_quat_qslerp(safediv(*bwgt, wsum), res, v) : v;
        }
        else
          additive_rot = v_quat_mul_quat(additive_rot, v_quat_lerp(v_splats(*bwgt), V_C_UNIT_0001, v));
      }
      if (!(ch.readyFlg & RM_ROT_A))
        chPrs[i].rot = res;
      else
      {
        vec4f rot_len = v_length4(additive_rot);
        additive_rot = v_sel(V_C_UNIT_0001, v_div(additive_rot, rot_len), v_cmp_gt(rot_len, v_zero()));
        chPrs[i].rot = v_quat_mul_quat(res, additive_rot);
      }
    }
  }

  if (PROFILE_BLENDING)
  {
    __pm_quat.pause();
    __pm.pause();
    if (__pm_anim_cnt >= 200)
    {
      debug("FQ: for %d (CPU %4.1f%%) blends: %5d, (%5d, %2d%% for quat) (%5d, %2d%% for point) (%5d for prepare list)", __pm_anim_cnt,
        __pm.getTotalUsec() * 100.0 / __pm_outer.getTotalUsec(), (int)__pm.getTotalUsec(), (int)__pm_quat.getTotalUsec(),
        int(__pm_quat.getTotalUsec() * 100 / __pm.getTotalUsec()), (int)__pm_p3.getTotalUsec(),
        int(__pm_p3.getTotalUsec() * 100 / __pm.getTotalUsec()), (int)__pm_prepare.getTotalUsec());
      __pm.stop();
      __pm_quat.stop();
      __pm_p3.stop();
      __pm_outer.stop();
      __pm_prepare.stop();
      __pm_anim_cnt = 0;
    }
    __pm_outer.go();
  }

#if MEASURE_PERF
  perf_tm.pause();
  perf_cnt++;
  perf_tm_cnt += nodenum;
#if MEASURE_PERF_FRAMES
  if (::dagor_frame_no() > perf_frameno_start + MEASURE_PERF_FRAMES)
#else
  if (perf_cnt >= MEASURE_PERF)
#endif
  {
    perfmonstat::animchar_blendAnim_cnt = perf_cnt;
    perfmonstat::animchar_blendAnim_tm_cnt = perf_tm_cnt;
    perfmonstat::animchar_blendAnim_time_usec = perf_tm.getTotalUsec();
    perfmonstat::generation_counter++;

    perf_tm.reset();
    perf_tm_cnt = perf_cnt = 0;
    perf_frameno_start = ::dagor_frame_no();
  }
#endif
  return true;
}
void AnimBlender::blendOriginVel(TlsContext &tls, IPureAnimStateHolder &st, IAnimBlendNode *root, bool rebuild_list)
{
#if MEASURE_PERF
  perf_tm.go();
#endif

  dag::Span<real> bnlWt = tls.bnlWt;
  dag::Span<int> bnlCT = tls.bnlCT;
  dag::Span<real> pbcWt = tls.pbcWt;

  int i, bnlNum = bnl.size();
  vec3f originVel = v_zero(), originWVel = v_zero(), originVelWt = v_zero();
  bool has_origin_vel = false;

  if (PROFILE_BLENDING)
    __pm.go();

  if (rebuild_list)
  {
    // build animation weights
    mem_set_0(bnlWt);
    mem_set_0(pbcWt);
    IAnimBlendNode::BlendCtx bctx(st, tls, false);
    root->buildBlendingList(bctx, 1.0f);
  }

  // construct blending lists
  for (i = 0; i < bnlNum; i++)
  {
    if (!*(int *)&bnlWt[i] || !bnl[i] || bnl[i]->dontUseOriginVel || !bnl[i]->anim)
      continue;

    vec3f pts = v_splats(bnlWt[i]);
    vec3f ts = v_splats(bnl[i]->timeRatio);
    int wa_pos = bnlCT[i];

    originVelWt = v_add(originVelWt, pts);

    if (bnl[i]->timeScaleParamId >= 0)
      pts = v_mul(pts, v_splats(*st.getParamScalarPtr(bnl[i]->timeScaleParamId)));

    AnimData::Anim &a = bnl[i]->anim->anim;
    using Sampler = AnimV20Math::Float3AnimNodeSampler<AnimV20Math::OneShotConfig>;
    Sampler linVelSampler(a.originLinVel, wa_pos);
    Sampler angVelSampler(a.originAngVel, wa_pos);

    vec3f vel;
    vec3f addVel = bnl[i]->addOriginVel;

    vel = v_madd(linVelSampler.sample(), ts, addVel);

    originWVel = v_madd(angVelSampler.sample(), v_mul(ts, pts), originWVel);

    originVel = v_madd(vel, pts, originVel);
    has_origin_vel = true;
  }

  if (!has_origin_vel)
  {
    tls.originLinVel = v_zero();
    tls.originAngVel = v_zero();
  }
  else
  {
    originVelWt = v_rcp(originVelWt);
    tls.originLinVel = v_mul(originVel, originVelWt);
    tls.originAngVel = v_mul(originWVel, originVelWt);
  }

  if (PROFILE_BLENDING)
    __pm.pause();
#if MEASURE_PERF
  perf_tm_cnt++;
  perf_tm.pause();
#endif
}
bool AnimBlender::getBlendedNodePRS(TlsContext &tls, int id, const vec4f **p, const quat4f **r, const vec4f **s, const vec4f *orig_prs)
{
  dag::Span<uint8_t> readyMark = tls.readyMark;
  if (uint32_t(id) >= readyMark.size())
    return false;
  uint8_t &mark = readyMark[id];

  if (!mark) //== optimize this!
  {
    if (!orig_prs)
      return false;
    *p = orig_prs + 0;
    *r = orig_prs + 1;
    *s = orig_prs + 2;
    return true;
  }
  PrsResult &prs = tls.chPrs[id];

  if (mark & RM_POS_B)
    *p = &prs.pos;
  else if (!(mark & RM_POS_A))
    *p = orig_prs ? orig_prs + 0 : NULL;
  else if (orig_prs)
  {
    *p = &prs.pos;
    prs.pos = v_add(prs.pos, orig_prs[0]);
    mark |= RM_POS_B;
  }
  else
    *p = NULL;

  if (mark & RM_ROT_B)
    *r = &prs.rot;
  else if (!(mark & RM_ROT_A))
    *r = orig_prs ? orig_prs + 1 : NULL;
  else if (orig_prs)
  {
    *r = &prs.rot;
    prs.rot = v_quat_mul_quat(orig_prs[1], prs.rot);
    mark |= RM_ROT_B;
  }
  else
    *r = NULL;

  if (mark & RM_SCL_B)
    *s = &prs.scl;
  else if (!(mark & RM_SCL_A))
    *s = orig_prs ? orig_prs + 2 : NULL;
  else if (orig_prs)
  {
    *s = &prs.scl;
    prs.scl = v_mul(prs.scl, orig_prs[2]);
    mark |= RM_SCL_B;
  }
  else
    *s = NULL;

  return true;
}

bool AnimBlender::getBlendedNodeTm(TlsContext &tls, int id, mat44f &tm, const vec4f *orig_prs)
{
  const vec4f *p, *r, *s;
  if (!AnimBlender::getBlendedNodePRS(tls, id, &p, &r, &s, orig_prs))
    return false;
  if (!p || !r || !s)
    return false;
  v_mat44_compose(tm, *p, *r, *s);
  return true;
}

void AnimBlender::postBlendInit(IPureAnimStateHolder &st, const GeomNodeTree &tree)
{
  for (int i = 0, e = pbCtrl.size(); i < e; i++)
    if (pbCtrl[i])
      pbCtrl[i]->init(st, tree);
}
void AnimBlender::postBlendProcess(TlsContext &tls, IPureAnimStateHolder &st, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx)
{
  TIME_PROFILE_DEV(postBlendProcess);
  dag::Span<real> pbcWt = tls.pbcWt;
  for (int i = 0, e = pbCtrl.size(); i < e; i++)
    if (pbCtrl[i])
      pbCtrl[i]->process(st, pbcWt[i], tree, ctx);
}

void AnimPostBlendCtrl::buildBlendingList(BlendCtx &ctx, real w) { ctx.pbcWt[pbcId] += w; }

//
// Animation Manager (for sharing animation graphs)
//
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_files.h>
#include <anim/dag_animBlendCtrl.h>
#include <anim/dag_animPostBlendCtrl.h>

namespace AnimResManagerV20
{
using namespace AnimV20;

class AnimNodeMaskApplyFilter
{
  RegExp *nmInclRe = NULL, *nmExclRe = NULL;

public:
  AnimNodeMaskApplyFilter(const DataBlock &b)
  {
    if (const char *re = b.getStr("accept_name_mask_re", NULL))
    {
      nmInclRe = new RegExp();
      if (!nmInclRe->compile(re, "i"))
      {
        del_it(nmInclRe);
        logerr("invalid %s=\"%s\" in block %s", "accept_name_mask_re", re, b.getBlockName());
      }
    }
    if (const char *re = b.getStr("decline_name_mask_re", NULL))
    {
      nmExclRe = new RegExp();
      if (!nmExclRe->compile(re, "i"))
      {
        del_it(nmExclRe);
        logerr("invalid %s=\"%s\" in block %s", "decline_name_mask_re", re, b.getBlockName());
      }
    }
  }
  ~AnimNodeMaskApplyFilter()
  {
    del_it(nmInclRe);
    del_it(nmExclRe);
  }

  bool isNameRejected(const char *nm) { return ((nmInclRe && !nmInclRe->test(nm)) || (nmExclRe && nmExclRe->test(nm))); }
};

static void add_bnl(AnimationGraph &graph, const DataBlock &blk, AnimData *anim, bool ignoredAnimation, bool def_foreign,
  const char *nm_suffix = 0)
{
  fatal_context_push(String(256, "bnl: %s", blk.getStr("name", "n/a")));

  if (dd_stricmp(blk.getBlockName(), "continuous") == 0)
    AnimBlendNodeContinuousLeaf::createNode(graph, blk, anim, ignoredAnimation, def_foreign, nm_suffix);

  else if (dd_stricmp(blk.getBlockName(), "single") == 0)
    AnimBlendNodeSingleLeaf::createNode(graph, blk, anim, ignoredAnimation, def_foreign, nm_suffix);

  else if (dd_stricmp(blk.getBlockName(), "still") == 0)
    AnimBlendNodeStillLeaf::createNode(graph, blk, anim, ignoredAnimation, def_foreign, nm_suffix);

  else if (dd_stricmp(blk.getBlockName(), "parametric") == 0)
    AnimBlendNodeParametricLeaf::createNode(graph, blk, anim, ignoredAnimation, def_foreign, nm_suffix);

  else
    logerr("unknown BlendNodeLeaf class <%s>", blk.getBlockName());

  fatal_context_pop();
}

static void add_bn(AnimationGraph &graph, const DataBlock &blk, const char *nm_suffix)
{
  fatal_context_push(String(256, "ctrl: %s", blk.getStr("name", "n/a")));

  if (dd_stricmp(blk.getBlockName(), "fifo3") == 0)
    AnimBlendCtrl_Fifo3::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "linear") == 0)
    AnimBlendCtrl_1axis::createNode(graph, blk, nm_suffix);

  else if (dd_stricmp(blk.getBlockName(), "linearPoly") == 0)
    AnimBlendCtrl_LinearPoly::createNode(graph, blk, nm_suffix);

  else if (dd_stricmp(blk.getBlockName(), "randomSwitch") == 0)
    AnimBlendCtrl_RandomSwitcher::createNode(graph, blk, nm_suffix);

  else if (dd_stricmp(blk.getBlockName(), "paramSwitch") == 0)
    AnimBlendCtrl_ParametricSwitcher::createNode(graph, blk, nm_suffix);

  else if (dd_stricmp(blk.getBlockName(), "paramSwitchS") == 0)
    AnimBlendCtrl_ParametricSwitcher::createNode(graph, blk, nm_suffix);

  else if (dd_stricmp(blk.getBlockName(), "setMotionMatchingTag") == 0)
    AnimBlendCtrl_SetMotionMatchingTag::createNode(graph, blk, nm_suffix);

  else if (dd_stricmp(blk.getBlockName(), "hub") == 0)
    AnimBlendCtrl_Hub::createNode(graph, blk, nm_suffix);

  else if (dd_stricmp(blk.getBlockName(), "planar") == 0)
    DEBUG_CTX("not implenmented");

  else if (dd_stricmp(blk.getBlockName(), "null") == 0)
    AnimBlendNodeNull::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "stub") == 0)
    AnimBlendNodeNull::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "blender") == 0)
    AnimBlendCtrl_Blender::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "BIS") == 0)
    AnimBlendCtrl_BinaryIndirectSwitch::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "deltaAnglesCalc") == 0)
    DeltaAnglesCtrl::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "alignNode") == 0)
    AnimPostBlendAlignCtrl::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "alignEx") == 0)
    AnimPostBlendAlignExCtrl::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "rotateNode") == 0)
    AnimPostBlendRotateCtrl::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "rotateAroundNode") == 0)
    AnimPostBlendRotateAroundCtrl::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "scaleNode") == 0)
    AnimPostBlendScaleCtrl::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "moveNode") == 0)
    AnimPostBlendMoveCtrl::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "paramsCtrl") == 0)
    ApbParamCtrl::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "defClampCtrl") == 0)
    DefClampParamCtrl::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "condHide") == 0)
    AnimPostBlendCondHideCtrl::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "animateNode") == 0)
    ApbAnimateCtrl::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "legsIK") == 0)
    LegsIKCtrl::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "multiChainFABRIK") == 0)
    MultiChainFABRIKCtrl::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "attachNode") == 0)
    AttachGeomNodeCtrl::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "aim") == 0)
    AnimPostBlendAimCtrl::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "lookat") == 0)
    AnimPostBlendNodeLookatCtrl::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "lookatNode") == 0)
    AnimPostBlendNodeLookatNodeCtrl::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "effFromAttachment") == 0)
    AnimPostBlendEffFromAttachement::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "nodesFromAttachement") == 0)
    AnimPostBlendNodesFromAttachement::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "paramFromNode") == 0)
    AnimPostBlendParamFromNode::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "matFromNode") == 0)
    AnimPostBlendMatVarFromNode::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "compoundRotateShift") == 0)
    AnimPostBlendCompoundRotateShift::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "setParam") == 0)
    AnimPostBlendSetParam::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "twistCtrl") == 0)
    AnimPostBlendTwistCtrl::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "eyeCtrl") == 0)
    AnimPostBlendEyeCtrl::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "effectorFromChildIK") == 0)
    AnimPostBlendNodeEffectorFromChildIK::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "deltaRotateShiftCalc") == 0)
    DeltaRotateShiftCtrl::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "footLockerIK") == 0)
    FootLockerIKCtrl::createNode(graph, blk);

  else if (dd_stricmp(blk.getBlockName(), "animateAndProcNode") == 0)
    createNodeApbAnimateAndPostBlendProc(graph, blk, nm_suffix, add_bn);

  else if (dd_stricmp(blk.getBlockName(), "alias") != 0)
  {
    logerr("unknown BlendNode class <%s>", blk.getBlockName());
    AnimBlendNodeNull::createNode(graph, blk);
  }

  fatal_context_pop();
}

struct ControllerDependencyLoadHelper
{
  Ptr<IAnimBlendNode> node;
  const DataBlock *settings;
  const char *nodeMaskSuffix;
};

static const char *controller_names_with_dependecy[] = {
  "linear", "linearPoly", "randomSwitch", "paramSwitch", "paramSwitchS", "hub", "blender", "BIS"};

static bool controller_has_dependency(const char *block_name)
{
  return lup(block_name, controller_names_with_dependecy, countof(controller_names_with_dependecy)) != -1;
}

static bool find_and_replce_alias_origin(dag::Vector<ControllerDependencyLoadHelper> &controllers, int selected_idx,
  bool allow_suffixless = false)
{
  ControllerDependencyLoadHelper &controller = controllers[selected_idx];
  const char *originName = controller.settings->getStr("origin");
  for (int i = selected_idx + 1; i < controllers.size(); ++i)
  {
    // try find origin with equal name field and with allow_suffixless or bouth null or equal suffix
    if (dd_stricmp(originName, controllers[i].settings->getStr("name")) == 0)
      if (allow_suffixless || (!controller.nodeMaskSuffix && !controllers[i].nodeMaskSuffix) ||
          (controller.nodeMaskSuffix && controllers[i].nodeMaskSuffix &&
            dd_stricmp(controller.nodeMaskSuffix, controllers[i].nodeMaskSuffix) == 0))
      {
        ControllerDependencyLoadHelper tempController = controllers[i];
        controllers.erase(controllers.begin() + i);
        controllers.insert(controllers.begin() + selected_idx, tempController);
        return true;
      }
  }

  return false;
}

static void init_alias_controllers(dag::Vector<ControllerDependencyLoadHelper> &controllers, AnimationGraph &graph)
{
  int aliasChangeOrderingCount = 0;
  for (int i = 0; i < controllers.size(); ++i)
  {
    const char *originName = controllers[i].settings->getStr("origin");
    const char *nodeName = controllers[i].settings->getStr("name");
    IAnimBlendNode *node = graph.getBlendNodePtr(originName, controllers[i].nodeMaskSuffix);
    if (!node)
    {
      if (i + aliasChangeOrderingCount >= controllers.size())
      {
        // Skip node with loop for init other alias nodes
        ANIM_ERR("Find loop in alias nodes, node: %s suffix: %s, with origin: %s", nodeName, controllers[i].nodeMaskSuffix,
          originName);
        aliasChangeOrderingCount = 0;
        continue;
      }
      if (find_and_replce_alias_origin(controllers, i))
      {
        ++aliasChangeOrderingCount;
        --i;
        continue;
      }
    }

    bool allowSuffixless = controllers[i].settings->getBool("allowSuffixless", false);
    if (!node && allowSuffixless)
      node = graph.getBlendNodePtr(originName);

    if (node)
    {
      aliasChangeOrderingCount = 0;
      graph.registerBlendNode(node, nodeName, controllers[i].nodeMaskSuffix);
    }
    else if (allowSuffixless)
    {
      if (find_and_replce_alias_origin(controllers, i, allowSuffixless))
      {
        ++aliasChangeOrderingCount;
        --i;
      }
    }
    else
      ANIM_ERR("Can't find %s node for alias node with name: %s", originName, nodeName);
  }
}

static bool load_generic_graph(AnimationGraph &graph, const DataBlock &blk, dag::ConstSpan<AnimData *> anim_list,
  const Tab<int> &nodesWithIgnoredAnimation)
{
  int nid, nid2, i, j;
  Tab<NodeMask *> nodemask(tmpmem);
  const DataBlock &stDescBlk = *blk.getBlockByNameEx("stateDesc");
  Ptr<IAnimBlendNode> nullAnim = new AnimBlendNodeNull;

  // check for animationless graph
  if (blk.getBool("dont_use_anim", false))
  {
    graph.replaceRoot(NULL);
    return true;
  }

  graph.allocateGlobalTimer();

  // read all node masks (can be used in leaf nodes)
  nid = blk.getNameId("nodeMask");
  nid2 = blk.getNameId("node");
  for (i = 0; i < blk.blockCount(); i++)
    if (blk.getBlock(i)->getBlockNameId() == nid)
    {
      const DataBlock *b = blk.getBlock(i);
      NodeMask *nm = new (tmpmem) NodeMask;

      nm->anim = NULL;
      nm->name = b->getStr("name", "");
      for (j = 0; j < b->paramCount(); j++)
        if (b->getParamType(j) == DataBlock::TYPE_STRING && b->getParamNameId(j) == nid2)
          nm->nm.addNameId(b->getStr(j));

      nodemask.push_back(nm);
    }

  if (stDescBlk.blockCount())
    graph.initChannelsForStates(stDescBlk, nodemask);

  graph.dbgNodeMask.resize(nodemask.size());
  for (i = 0; i < nodemask.size(); i++)
  {
    graph.dbgNodeMask[i].name = nodemask[i]->name;
    graph.dbgNodeMask[i].nm = nodemask[i]->nm;
    graph.dbgNodeMask[i].anim = NULL;
  }

  // append all leaf nodes first
  bool def_foreign = blk.getBool("defaultForeignAnim", false);
  nid = blk.getNameId("AnimBlendNodeLeaf");
  for (int pass = 0; pass < 2; pass++)
    for (i = 0; i < blk.blockCount(); i++)
      if (blk.getBlock(i)->getBlockNameId() == nid)
      {
        const DataBlock *b = blk.getBlock(i);
        Ptr<AnimData> anim;
        bool ignoredAnimation = false;
        {
          int idx = b->getInt("a2d_id", -1);
          if (idx < 0 || idx >= anim_list.size())
          {
            DEBUG_CTX("incorrect anim index: %d", idx);
            continue;
          }
          ignoredAnimation = tabutils::find(nodesWithIgnoredAnimation, idx);

          anim = anim_list[idx];
          if (!anim)
          {
            if (!is_ignoring_unavailable_resources())
              DEBUG_CTX("invalid anim[%d]", idx);
            continue;
          }
          // DEBUG_CTX("anim[%d]: %p ref=%d", idx, anim.get(), anim->getRefCount());
          fatal_context_push(String(256, "a2d id: %d", idx));
        }

        // release cached anims in nodemask
        for (j = 0; j < nodemask.size(); j++)
        {
          if (nodemask[j]->anim)
            nodemask[j]->anim->delRef();
          nodemask[j]->anim = NULL;
        }
        AnimNodeMaskApplyFilter nmFilter(*b);

        // load BlendNodeLeafs
        if (int chan_cnt = graph.getStDest().size())
        {
          for (j = 0; j < b->blockCount(); j++)
          {
            const DataBlock &cb2 = *b->getBlock(j);
            if (pass == 0 && cb2.getBool("additive", false))
              continue;
            if (pass == 1 && !cb2.getBool("additive", false))
              continue;
            if (const char *applyNodeMask = cb2.getStr("apply_node_mask", NULL))
            {
              NodeMask *nm = NULL;
              for (int k = 0; k < nodemask.size(); k++)
                if (strcmp(nodemask[k]->name, applyNodeMask) == 0)
                {
                  nm = nodemask[k];
                  break;
                }

              if (!nm)
              {
                logerr("anim <%s> requires missing nodemask=%s", cb2.getStr("name"), applyNodeMask);
                continue;
              }
              if (!nm->anim)
              {
                nm->anim = new (midmem) AnimData(anim, nm->nm, midmem);
                nm->anim->addRef();
              }
              add_bnl(graph, cb2, nm->anim, ignoredAnimation, def_foreign, NULL);
              if (const char *anim_name = cb2.getStr("name", NULL))
                for (int k = 0; k < chan_cnt; k++)
                  if (graph.getStDest()[k].defNodemaskIdx >= 0)
                    graph.registerBlendNode(nullAnim, anim_name, nodemask[graph.getStDest()[k].defNodemaskIdx]->name);
              chan_cnt = -1; // to skip next for()
            }
            for (int k = -1; k < chan_cnt; k++)
            {
              if (k >= 0 && graph.getStDest()[k].defNodemaskIdx < 0)
                continue;
              NodeMask *nm = k >= 0 ? nodemask[graph.getStDest()[k].defNodemaskIdx] : NULL;
              if (nmFilter.isNameRejected(nm ? nm->name.str() : ""))
              {
                if (const char *anim_name = cb2.getStr("name", NULL))
                  graph.registerBlendNode(nullAnim, anim_name, nm ? nm->name.str() : NULL);
                continue;
              }
              if (nm && !nm->anim)
              {
                nm->anim = new (midmem) AnimData(anim, nm->nm, midmem);
                nm->anim->addRef();
              }
              add_bnl(graph, cb2, nm ? nm->anim : anim.get(), ignoredAnimation, def_foreign, nm ? nm->name.str() : NULL);
            }
          }
        }
        else
          for (j = 0; j < b->blockCount(); j++)
          {
            const DataBlock &cb2 = *b->getBlock(j);
            if (pass == 0 && cb2.getBool("additive", false))
              continue;
            if (pass == 1 && !cb2.getBool("additive", false))
              continue;

            const char *applyNodeMask = cb2.getStr("apply_node_mask", NULL);
            if (!applyNodeMask)
              add_bnl(graph, cb2, anim, ignoredAnimation, def_foreign);
            else
            {
              NodeMask *nm = NULL;
              for (int k = 0; k < nodemask.size(); k++)
                if (strcmp(nodemask[k]->name, applyNodeMask) == 0)
                {
                  nm = nodemask[k];
                  break;
                }
              if (!nm)
              {
                DEBUG_CTX("can't find nodemask: <%s>", applyNodeMask);
                continue;
              }

              if (!nm->anim)
              {
                nm->anim = new (midmem) AnimData(anim, nm->nm, midmem);
                nm->anim->addRef();
              }

              add_bnl(graph, cb2, nm->anim, ignoredAnimation, def_foreign);
            }
          }
        fatal_context_pop();
      }

  // release cached anims in nodemask
  for (j = 0; j < nodemask.size(); j++)
  {
    if (nodemask[j]->anim)
      nodemask[j]->anim->delRef();
    nodemask[j]->anim = NULL;
  }

  // append other controllers
  dag::Vector<ControllerDependencyLoadHelper> controllersWithDependency;
  dag::Vector<ControllerDependencyLoadHelper> aliasControllers;
  nid = blk.getNameId("AnimBlendCtrl");
  for (i = 0; i < blk.blockCount(); i++)
    if (blk.getBlock(i)->getBlockNameId() == nid)
    {
      const DataBlock *b = blk.getBlock(i);
      if (!b->getBool("use", true))
        continue;
      for (j = 0; j < b->blockCount(); j++)
      {
        const DataBlock *settings = b->getBlock(j);
        AnimNodeMaskApplyFilter nmFilter(*settings);
        const char *anim_name = settings->getStr("name", nullptr);
        if (!nmFilter.isNameRejected(""))
        {
          add_bn(graph, *settings, nullptr);
          if (controller_has_dependency(settings->getBlockName()))
            controllersWithDependency.emplace_back(
              ControllerDependencyLoadHelper{graph.getBlendNodePtr(anim_name, nullptr), settings, nullptr});
          else if (dd_stricmp(settings->getBlockName(), "alias") == 0)
            aliasControllers.emplace_back(ControllerDependencyLoadHelper{nullptr, settings, nullptr});
        }
        else if (anim_name)
          graph.registerBlendNode(nullAnim, anim_name);

        bool def_split_chans = strstr("linear linearPoly randomSwitch paramSwitch paramSwitchS alias", settings->getBlockName());
        if (settings->getBool("splitChans", def_split_chans))
          if (int chan_cnt = graph.getStDest().size())
            for (int k = 0; k < chan_cnt; k++)
              if (graph.getStDest()[k].defNodemaskIdx >= 0)
              {
                const char *nm_name = nodemask[graph.getStDest()[k].defNodemaskIdx]->name;
                if (!nmFilter.isNameRejected(nm_name))
                {
                  add_bn(graph, *settings, nm_name);
                  if (controller_has_dependency(settings->getBlockName()))
                    controllersWithDependency.emplace_back(
                      ControllerDependencyLoadHelper{graph.getBlendNodePtr(anim_name, nm_name), settings, nm_name});
                  else if (dd_stricmp(settings->getBlockName(), "alias") == 0)
                    aliasControllers.emplace_back(ControllerDependencyLoadHelper{nullptr, settings, nm_name});
                }
                else if (anim_name)
                  graph.registerBlendNode(nullAnim, anim_name, nm_name);
              }
      }
    }

  init_alias_controllers(aliasControllers, graph);

  for (ControllerDependencyLoadHelper &controller : controllersWithDependency)
  {
    fatal_context_push(String(256, "ctrl: %s", controller.settings->getStr("name", "n/a")));
    controller.node->initChilds(graph, *controller.settings, controller.nodeMaskSuffix);
    fatal_context_pop();
  }

  eastl::bitvector<eastl::allocator, uint32_t, eastl::vector<uint32_t, eastl::allocator>> animNodeState;
  animNodeState.resize(graph.getAnimNodeCount(), false);

  for (ControllerDependencyLoadHelper &controller : controllersWithDependency)
    controller.node->checkHasLoop(graph, animNodeState);

  // clear nodemask
  clear_all_ptr_items_and_shrink(nodemask);

  // set root node
  const char *root = blk.getStr("root", NULL);
  if (!root)
  {
    ANIM_ERR("'root' not defined!");
    return false;
  }

  IAnimBlendNode *node = graph.getBlendNodePtr(root);
  if (!node)
  {
    if (!is_ignoring_unavailable_resources())
      ANIM_ERR("root node '%s' not found!", root);
    return false;
  }
  graph.replaceRoot(node);

  graph.sortPbCtrl(*blk.getBlockByNameEx("initAnimState")->getBlockByNameEx("postBlendCtrlOrder"));

  int null_used = 0;
  for (int i = 0; i < graph.getAnimNodeCount(); i++)
    if (graph.getBlendNodePtr(i) && !graph.getBlendNodePtr(i)->validateNodeNotUsed(graph, nullAnim))
      null_used++;
#if DAGOR_DBGLEVEL > 0
  if (null_used > 0)
    logerr("missing (Null) animNode is used for %d blendNodes, see logerr for details", null_used);
#endif

  if (stDescBlk.blockCount())
    graph.initStates(stDescBlk);

  return true;
}

AnimationGraph *loadUniqueAnimGraph(const DataBlock &blk, dag::ConstSpan<AnimData *> anim_list,
  const Tab<int> &nodesWithIgnoredAnimation)
{
  AnimationGraph *graph = new (midmem) AnimationGraph;
  graph->setInitState(*blk.getBlockByNameEx("initAnimState"));
  if (const DataBlock *bEnum = graph->getInitState() ? graph->getInitState()->getBlockByName("enum") : NULL)
  {
    FastNameMap map;
    for (int i = 0, ie = bEnum->blockCount(); i < ie; i++)
    {
      DataBlock *bEnumCls = const_cast<DataBlock *>(bEnum->getBlock(i));
      for (int j = 0, je = bEnumCls->blockCount(); j < je; j++)
      {
        DataBlock *enumBlock = bEnumCls->getBlock(j);
        const char *enum_nm = enumBlock->getBlockName();
        if (map.getNameId(enum_nm) < 0)
        {
          map.addNameId(enum_nm);
          enumBlock->addInt("_enumValue", AnimV20::addEnumValue(enum_nm));
        }
        else
          logerr("duplicate enum <%s> in block <%s>", enum_nm, bEnumCls->getBlockName());
      }
    }
    // debug("added %d enums", map.nameCount());
  }
  if (!load_generic_graph(*graph, blk, anim_list, nodesWithIgnoredAnimation))
  {
    DEBUG_CTX("can't read anim from stream");
    return NULL;
  }

  return graph;
}
} // namespace AnimResManagerV20

//
// creator functions for supported IAnimBlendNodeTypes
//
static void common_leaf_setup(AnimBlendNodeContinuousLeaf *node, const DataBlock &blk, bool ignoredAnimation, bool def_foreign)
{
  node->enableOriginVel(!blk.getBool("disable_origin_vel", false));

  const char *name = blk.getStr("name", NULL);
  const char *key = blk.getStr("key", name);
  String key_start_def, key_end_def;
  if (key)
  {
    key_start_def.printf(128, "%s_start", key);
    key_end_def.printf(128, "%s_end", key);
  }
  const char *key_start = blk.getStr("key_start", key_start_def);
  const char *key_end = blk.getStr("key_end", key_end_def);

  int tStart = blk.getInt("start", -1);
  int tEnd = blk.getInt("end", -1);
  real time = blk.getReal("time", 1.0f);
  real mdist = blk.getReal("moveDist", 0.0f);
  if (tStart >= 0 && tEnd >= 0)
  {
    node->setRange(tStart, tEnd, time, mdist, name);
  }
  else
  {
    node->setRange(key_start, key_end, time, mdist, name);
    node->setSyncTime(blk.getStr("sync_time", key_start));
  }

  node->buildNamedRanges(blk.getBlockByName("labels"));
  if (blk.paramExists("additive"))
    G_ASSERTF(node->isAdditive() == blk.getBool("additive", false), "%s.additive=%d != anim->additive=%d", name,
      blk.getBool("additive", false), node->isAdditive());
  node->setCharDep(blk.getBool("foreignAnimation", def_foreign));

  if (time > 0)
  {
    real dirh = blk.getReal("addMoveDirH", 0) * PI / 180;
    real dirv = blk.getReal("addMoveDirV", 0) * PI / 180;
    real dist = blk.getReal("addMoveDist", 0);

    double sp = sin(dirh), cp = cos(dirh), st = sin(dirv), ct = cos(dirv);
    node->setAddOriginVel((dist / time) * Point3(cp * ct, st, sp * ct));
  }

  node->setAnimationIgnored(ignoredAnimation);
}
static void common_nontime_leaf_setup(AnimBlendNodeLeaf *node, const DataBlock &blk, bool ignoredAnimation, bool def_foreign)
{
  node->enableOriginVel(!blk.getBool("disable_origin_vel", false));

  real dirh = blk.getReal("addMoveDirH", 0) * PI / 180;
  real dirv = blk.getReal("addMoveDirV", 0) * PI / 180;
  real vel = blk.getReal("addMoveDist", 0);

  if (float_nonzero(vel))
  {
    double sp = sin(dirh), cp = cos(dirh), st = sin(dirv), ct = cos(dirv);
    node->setAddOriginVel(vel * Point3(cp * ct, st, sp * ct));
  }

  node->setCharDep(blk.getBool("foreignAnimation", def_foreign));
  node->setAnimationIgnored(ignoredAnimation);
}

void AnimBlendNodeContinuousLeaf::createNode(AnimationGraph &graph, const DataBlock &blk, AnimData *anim, bool ignoredAnimation,
  bool def_foreign, const char *nm_suffix)
{
  const char *name = blk.getStr("name", NULL);
  if (!name)
  {
    ANIM_ERR("%s <name> param not found for <%s>", __FUNCTION__, blk.getBlockName());
    return;
  }
  bool ownTimer = blk.getBool("own_timer", false);
  bool eoaIrq = blk.getBool("eoa_irq", false);
  const char *timeScaleParam = blk.getStr("timeScaleParam", NULL);

  String timer;
  if (ownTimer)
    timer = String(name) + "::timer";
  else
  {
    const char *timerName = blk.getStr("timer", nullptr);
    if (timerName)
      timer = timerName;
    else
    {
      if (eoaIrq)
        logerr("Continious leaf '%s' must have own_timer:b=yes, as it has eoa_irq:b=yes", name);
      if (timeScaleParam)
        logerr("Continious leaf '%s' must have own_timer:b=yes, as it has timeScaleParam:t=\"%s\"", name, timeScaleParam);
      timer = "::GlobalTime";
    }
  }

  AnimBlendNodeContinuousLeaf *node = new AnimBlendNodeContinuousLeaf(graph, anim, timer);

  common_leaf_setup(node, blk, ignoredAnimation, def_foreign);
  node->setEndOfAnimIrq(eoaIrq);
  node->enableRewind(blk.getBool("rewind", true));
  node->enableStartOffset(blk.getBool("rand_start", false));

  node->setTimeScaleParam(timeScaleParam);
  node->setupIrq(blk, nm_suffix);

  graph.registerBlendNode(node, name, nm_suffix);
}

void AnimBlendNodeSingleLeaf::createNode(AnimationGraph &graph, const DataBlock &blk, AnimData *anim, bool ignoredAnimation,
  bool def_foreign, const char *nm_suffix)
{
  const char *name = blk.getStr("name", NULL);
  if (!name)
  {
    ANIM_ERR("%s <name> param not found for <%s>", __FUNCTION__, blk.getBlockName());
    return;
  }
  String timer;
  if (blk.getBool("own_timer", true))
    timer = String(name) + "::timer";
  else
  {
    // single node MUST own timer/have timer as it pauses global time otherwise
    // in AnimBlendNodeSingleLeaf::buildBlendingList
    const char *timerName = blk.getStr("timer", nullptr);
    if (!timerName)
      logerr("single anim node must either have own_timer:b=yes or have timer:t= set to timer var name");
    timer = timerName ? timerName : "::GlobalTime";
  }

  AnimBlendNodeSingleLeaf *node = new AnimBlendNodeSingleLeaf(graph, anim, timer);

  common_leaf_setup(node, blk, ignoredAnimation, def_foreign);

  node->setTimeScaleParam(blk.getStr("timeScaleParam", NULL));
  node->setupIrq(blk, nm_suffix);

  graph.registerBlendNode(node, name, nm_suffix);
}

void AnimBlendNodeStillLeaf::createNode(AnimationGraph &graph, const DataBlock &blk, AnimData *anim, bool ignoredAnimation,
  bool def_foreign, const char *nm_suffix)
{
  const char *name = blk.getStr("name", NULL);
  if (!name)
  {
    ANIM_ERR("%s <name> param not found for <%s>", __FUNCTION__, blk.getBlockName());
    return;
  }
  const char *key = blk.getStr("key", name);

  int tStart = blk.getInt("start", -1);
  if (tStart < 0)
  {
    tStart = anim->getLabelTime(key, !isdigit(key[0]));
    if (tStart < 0)
      tStart = atoi(key) * TIME_TicksPerSec / 30;
  }

  AnimBlendNodeStillLeaf *node = new AnimBlendNodeStillLeaf(graph, anim, tStart);
  common_nontime_leaf_setup(node, blk, ignoredAnimation, def_foreign);
  graph.registerBlendNode(node, name, nm_suffix);
}

void AnimBlendNodeParametricLeaf::createNode(AnimationGraph &graph, const DataBlock &blk, AnimData *anim, bool ignoredAnimation,
  bool def_foreign, const char *nm_suffix)
{
  const char *name = blk.getStr("name", NULL);
  if (!name)
  {
    ANIM_ERR("%s <name> param not found for <%s>", __FUNCTION__, blk.getBlockName());
    return;
  }
  const char *key = blk.getStr("key", name);
  const char *varname = blk.getStr("varname", NULL);

  if (!varname)
  {
    ANIM_ERR("not found 'varname' param");
    return;
  }

  AnimBlendNodeParametricLeaf *node = new AnimBlendNodeParametricLeaf(graph, anim, varname, blk.getBool("updOnVarChange", false));

  String key_start_def, key_end_def;
  if (key)
  {
    key_start_def.printf(128, "%s_start", key);
    key_end_def.printf(128, "%s_end", key);
  }
  const char *key_start = blk.getStr("key_start", key_start_def);
  const char *key_end = blk.getStr("key_end", key_end_def);

  node->setRange(key_start, key_end, blk.getReal("p_start", 0.0f), blk.getReal("p_end", 1.0f), name);
  node->setLooping(blk.getBool("looping", false));
  common_nontime_leaf_setup(node, blk, ignoredAnimation, def_foreign);
  node->setParamKoef(blk.getReal("mulk", 1.0f), blk.getReal("addk", 0.0f));
  node->setupIrq(blk, nm_suffix);

  graph.registerBlendNode(node, name, nm_suffix);
}

void AnimBlendNodeNull::createNode(AnimationGraph &graph, const DataBlock &blk)
{
  const char *name = blk.getStr("name", NULL);

  if (!name)
  {
    ANIM_ERR("not found 'name' param");
    return;
  }

  // stub null controller we create only if none exists
  if (strcmp(blk.getBlockName(), "stub") == 0)
    if (graph.getBlendNodeId(name) != -1)
      return;

  graph.registerBlendNode(new AnimBlendNodeNull, name);
}

void AnimBlendCtrl_1axis::createNode(AnimationGraph &graph, const DataBlock &blk, const char *nm_suffix)
{
  const char *name = blk.getStr("name", NULL);
  const char *_vname = blk.getStr("varname", NULL);
  String var_name;

  if (!name)
  {
    ANIM_ERR("%s <name> param not found for <%s>", __FUNCTION__, blk.getBlockName());
    return;
  }

  if (_vname)
    var_name = _vname;
  else
    var_name = String("var") + name;

  AnimBlendCtrl_1axis *node = new AnimBlendCtrl_1axis(graph, var_name);
  graph.registerBlendNode(node, name, nm_suffix);
}

void AnimBlendCtrl_1axis::initChilds(AnimationGraph &graph, const DataBlock &blk, const char *nm_suffix)
{
  int child_nid = blk.getNameId("child"), i;
  if (child_nid != -1)
    for (i = 0; i < blk.blockCount(); i++)
      if (blk.getBlock(i)->getBlockNameId() == child_nid)
      {
        const DataBlock *cblk = blk.getBlock(i);
        const char *nm = cblk->getStr("name", NULL);
        if (!nm)
        {
          ANIM_ERR("<name> is not defined in hub child");
          graph.unregisterBlendNode(this);
          return;
        }

        IAnimBlendNode *n = graph.getBlendNodePtr(nm, nm_suffix);
        if (!n)
        {
          if (!is_ignoring_unavailable_resources())
            ANIM_ERR("blend node <%s> (suffix=%s) not found!", nm, nm_suffix);
          graph.unregisterBlendNode(this);
          return;
        }
        addBlendNode(n, cblk->getReal("start", 0), cblk->getReal("end", 1.0f));
      }
}

void AnimBlendCtrl_1axis::checkHasLoop(AnimationGraph &graph,
  eastl::bitvector<eastl::allocator, uint32_t, eastl::vector<uint32_t, eastl::allocator>> &visited_nodes)
{
  int id = getAnimNodeId();
  if (id == -1)
    return;

  visited_nodes[id] = true;
  for (const AnimSlice &child : slice)
  {
    int childId = child.node->getAnimNodeId();
    if (childId == -1)
      continue;

    if (visited_nodes[childId])
    {
      ANIM_ERR("Find loop in controller childs, controller: %s, parent: %s", graph.getAnimNodeName(childId),
        graph.getAnimNodeName(id));
      slice.clear();
      break;
    }
    else
      child.node->checkHasLoop(graph, visited_nodes);
  }
  visited_nodes[id] = false;
}

void AnimBlendCtrl_LinearPoly::createNode(AnimationGraph &graph, const DataBlock &blk, const char *nm_suffix)
{
  const char *name = blk.getStr("name", NULL);
  const char *_vname = blk.getStr("varname", NULL);
  String var_name;

  if (!name)
  {
    ANIM_ERR("%s <name> param not found for <%s>", __FUNCTION__, blk.getBlockName());
    return;
  }

  if (_vname)
    var_name = _vname;
  else
    var_name = String("var") + name;

  AnimBlendCtrl_LinearPoly *node = new AnimBlendCtrl_LinearPoly(graph, var_name, name);
  node->morphTime = blk.getReal("morphTime", 0.0f);
  node->enclosed = blk.getBool("enclosed", false);
  node->paramTau = blk.getReal("paramTau", 0.0f);
  node->paramSpeed = blk.getReal("paramSpeed", 0.0f);
  graph.registerBlendNode(node, name, nm_suffix);
}

void AnimBlendCtrl_LinearPoly::initChilds(AnimationGraph &graph, const DataBlock &blk, const char *nm_suffix)
{
  const char *name = blk.getStr("name");
  int child_nid = blk.getNameId("child"), i;
  if (child_nid != -1)
    for (i = 0; i < blk.blockCount(); i++)
      if (blk.getBlock(i)->getBlockNameId() == child_nid)
      {
        const DataBlock *cblk = blk.getBlock(i);
        const char *nm = cblk->getStr("name", NULL);
        if (!nm)
        {
          ANIM_ERR("<name> is not defined in hub child");
          return;
        }

        IAnimBlendNode *n = graph.getBlendNodePtr(nm, nm_suffix);
        if (!n)
        {
          if (!is_ignoring_unavailable_resources())
            ANIM_ERR("blend node <%s> (suffix=%s) not found!", nm, nm_suffix);
          return;
        }
        addBlendNode(n, cblk->getReal("val", 0), graph, name);
      }
}

void AnimBlendCtrl_LinearPoly::checkHasLoop(AnimationGraph &graph,
  eastl::bitvector<eastl::allocator, uint32_t, eastl::vector<uint32_t, eastl::allocator>> &visited_nodes)
{
  int id = getAnimNodeId();
  if (id == -1)
    return;

  visited_nodes[id] = true;
  for (const AnimPoint &child : poly)
  {
    int childId = child.node->getAnimNodeId();
    if (childId == -1)
      continue;

    if (visited_nodes[childId])
    {
      ANIM_ERR("Find loop in controller childs, controller: %s, parent: %s", graph.getAnimNodeName(childId),
        graph.getAnimNodeName(id));
      poly.clear();
      break;
    }
    else
      child.node->checkHasLoop(graph, visited_nodes);
  }
  visited_nodes[id] = false;
}

void AnimBlendCtrl_Fifo3::createNode(AnimationGraph &graph, const DataBlock &blk)
{
  const char *name = blk.getStr("name", NULL);
  const char *varname = blk.getStr("varname", NULL);

  if (!varname)
  {
    ANIM_ERR("not found 'varname' param");
    return;
  }

  IAnimBlendNode *node = new AnimBlendCtrl_Fifo3(graph, varname);
  if (name)
    graph.registerBlendNode(node, name);
}

void AnimBlendCtrl_RandomSwitcher::createNode(AnimationGraph &graph, const DataBlock &blk, const char *nm_suffix)
{
  const char *name = blk.getStr("name", NULL);
  const char *_vname = blk.getStr("varname", NULL);
  String var_name;

  if (!name)
  {
    ANIM_ERR("%s <name> param not found for <%s>", __FUNCTION__, blk.getBlockName());
    return;
  }

  if (_vname)
    var_name = _vname;
  else
    var_name = String("var") + name;

  AnimBlendCtrl_RandomSwitcher *node = new AnimBlendCtrl_RandomSwitcher(graph, var_name);

  graph.registerBlendNode(node, name, nm_suffix);
}

void AnimBlendCtrl_RandomSwitcher::initChilds(AnimationGraph &graph, const DataBlock &blk, const char *nm_suffix)
{
  // create anim list
  const DataBlock *cblk = blk.getBlockByName("weight");
  const DataBlock *rblk = blk.getBlockByName("maxrepeat");
  int i;

  if (cblk)
  {
    for (i = 0; i < cblk->paramCount(); i++)
    {
      if (cblk->getParamType(i) != DataBlock::TYPE_REAL)
        continue;

      const char *nm = cblk->getName(cblk->getParamNameId(i));
      IAnimBlendNode *n = graph.getBlendNodePtr(nm, nm_suffix);
      if (!n)
      {
        if (!is_ignoring_unavailable_resources())
          LOGERR_CTX("blend node <%s> (suffix=%s) not found!", nm, nm_suffix);
        continue;
      }
      addBlendNode(n, cblk->getReal(i), rblk ? rblk->getInt(nm, 1) : 1);
    }
  }

  recalcWeights();
}

void AnimBlendCtrl_RandomSwitcher::checkHasLoop(AnimationGraph &graph,
  eastl::bitvector<eastl::allocator, uint32_t, eastl::vector<uint32_t, eastl::allocator>> &visited_nodes)
{
  int id = getAnimNodeId();
  if (id == -1)
    return;

  visited_nodes[id] = true;
  for (const RandomAnim &child : list)
  {
    int childId = child.node->getAnimNodeId();
    if (childId == -1)
      continue;

    if (visited_nodes[childId])
    {
      ANIM_ERR("Find loop in controller childs, controller: %s, parent: %s", graph.getAnimNodeName(childId),
        graph.getAnimNodeName(id));
      list.clear();
      break;
    }
    else
      child.node->checkHasLoop(graph, visited_nodes);
  }
  visited_nodes[id] = false;
}

void AnimBlendCtrl_ParametricSwitcher::createNode(AnimationGraph &graph, const DataBlock &blk, const char *nm_suffix)
{
  const char *name = blk.getStr("name", NULL);
  const char *_vname = blk.getStr("varname", NULL);
  String var_name;

  if (!name)
  {
    ANIM_ERR("%s <name> param not found for <%s>", __FUNCTION__, blk.getBlockName());
    return;
  }

  if (_vname)
    var_name = _vname;
  else
    var_name = String("var") + name;

  AnimBlendCtrl_ParametricSwitcher *node = new AnimBlendCtrl_ParametricSwitcher(graph, var_name, blk.getStr("varname_residual", NULL));

  node->morphTime = blk.getReal("morphTime", 0.15f);
  graph.registerBlendNode(node, name, nm_suffix);
}

void AnimBlendCtrl_ParametricSwitcher::initChilds(AnimationGraph &graph, const DataBlock &blk, const char *nm_suffix)
{
  const char *name = blk.getStr("name");
  const char *_vname = blk.getStr("varname", nullptr);
  String var_name;

  if (_vname)
    var_name = _vname;
  else
    var_name = String("var") + name;

  const DataBlock *cblk = blk.getBlockByName("nodes");
  bool has_single = false;

  // create anim list
  if (cblk)
  {
    for (int i = 0; i < cblk->blockCount(); i++)
    {
      const DataBlock *nblk = cblk->getBlock(i);
      const char *nm = nblk->getStr("name", "");
      const bool isOptional = nblk->getBool("optional", false);
      IAnimBlendNode *n = graph.getBlendNodePtr(nm, nm_suffix);
      if (!n)
      {
        if (!isOptional && !is_ignoring_unavailable_resources())
          LOGERR_CTX("blend node <%s> (suffix=%s) not found!", nm, nm_suffix);
        continue;
      }
      real r0 = nblk->getReal("rangeFrom", 0);
      real r1 = nblk->getReal("rangeTo", 0);
      real bv = nblk->getReal("baseVal", 0);
      if (!nblk->paramExists("rangeFrom") && !nblk->paramExists("rangeTo"))
      {
        int eval = AnimV20::getEnumValueByName(nblk->getBlockName());
        if (eval >= 0)
        {
          r0 = eval - 0.1f;
          r1 = eval + 0.1f;
        }
      }
      addBlendNode(n, min(r0, r1), max(r1, r0), bv, graph, name);
      if (n->isSubOf(AnimBlendNodeSingleLeafCID))
        has_single = true;
    }
    if (const char *enum_cls = cblk->getStr("enum_gen", NULL))
    {
      const DataBlock *init_st = graph.getInitState();
      const char *name_template = cblk->getStr("name_template", NULL);
      if (init_st)
        init_st = init_st->getBlockByNameEx("enum")->getBlockByName(enum_cls);
      if (init_st && name_template)
      {
        for (int j = 0, je = init_st->blockCount(); j < je; j++)
        {
          const DataBlock *enumBlock = init_st->getBlock(j);
          const char *enum_nm = enumBlock->getBlockName();
          String nm;

          if (strcmp(name_template, "*") == 0)
          {
            nm = enumBlock->getStr(name, "");
            if (nm.empty()) // try find in parents
            {
              // if recursionCount will become more than then blockCount, so we are in deadlock
              int recursionCount = 0;
              String parentEnumBlockName(enumBlock->getStr("_use", ""));
              while (!parentEnumBlockName.empty() && recursionCount < je)
              {
                recursionCount++;
                if (const DataBlock *parentBlock = init_st->getBlockByName(parentEnumBlockName.c_str()))
                {
                  nm = parentBlock->getStr(name, "");
                  parentEnumBlockName = nm.empty() ? parentBlock->getStr("_use", "") : "";
                }
                else
                  parentEnumBlockName.clear(); // break cycle
              }

              if (recursionCount == je)
              {
                ANIM_ERR("enum <%s> has cycle in _use!", enum_nm);
                continue;
              }

              if (nm.empty())
              {
                ANIM_ERR("blend node <%s> not found in enum <%s>!", name, enum_nm);
                continue;
              }
            }
          }
          else
          {
            nm = name_template;
            nm.replace("$1", enum_nm);
          }

          IAnimBlendNode *n = graph.getBlendNodePtr(nm.c_str(), nm_suffix);
          if (!n)
          {
            if (!is_ignoring_unavailable_resources())
              ANIM_ERR("blend node <%s> (suffix=%s) not found!", nm, nm_suffix);
            continue;
          }
          int eval = AnimV20::getEnumValueByName(enum_nm);
          if (eval < 0)
            ANIM_ERR("missing enum <%s>", enum_nm);
          addBlendNode(n, (eval >= 0) ? eval - 0.1f : 0, (eval >= 0) ? eval + 0.1f : 0, 0, graph, var_name);
          if (n->isSubOf(AnimBlendNodeSingleLeafCID))
            has_single = true;
        }
      }
      else
      {
        if (!init_st)
          ANIM_ERR("enum class <%s> not found in paramSwitch %s", enum_cls, name);
        if (!name_template)
          ANIM_ERR("name_template not set in paramSwitch %s", name);
      }
    }
  }
  if (dd_stricmp(blk.getBlockName(), "paramSwitchS") == 0)
    continuousAnimMode = false;

  if (has_single && continuousAnimMode)
  {
    logerr("paramSwitch %s has single anims, but tuned for continuous mode", name);
    for (int i = 0; i < list.size(); i++)
      logerr("  node[%d]=%s, %s", i, graph.getBlendNodeName(list[i].node),
        list[i].node->isSubOf(AnimBlendNodeSingleLeafCID) ? "single" : "continuous");
  }
}

void AnimBlendCtrl_ParametricSwitcher::checkHasLoop(AnimationGraph &graph,
  eastl::bitvector<eastl::allocator, uint32_t, eastl::vector<uint32_t, eastl::allocator>> &visited_nodes)
{
  int id = getAnimNodeId();
  if (id == -1)
    return;

  visited_nodes[id] = true;
  for (const ItemAnim &child : list)
  {
    int childId = child.node->getAnimNodeId();
    if (childId == -1)
      continue;

    if (visited_nodes[childId])
    {
      ANIM_ERR("Find loop in controller childs, controller: %s, parent: %s", graph.getAnimNodeName(childId),
        graph.getAnimNodeName(id));
      list.clear();
      break;
    }
    else
      child.node->checkHasLoop(graph, visited_nodes);
  }
  visited_nodes[id] = false;
}

void AnimBlendCtrl_Hub::createNode(AnimationGraph &graph, const DataBlock &blk, const char *nm_suffix)
{
  const char *name = blk.getStr("name", NULL);
  if (!name)
  {
    ANIM_ERR("%s <name> param not found for <%s>", __FUNCTION__, blk.getBlockName());
    return;
  }

  AnimBlendCtrl_Hub *node = new AnimBlendCtrl_Hub;
  graph.registerBlendNode(node, name, nm_suffix);
}

void AnimBlendCtrl_Hub::initChilds(AnimationGraph &graph, const DataBlock &blk, const char *nm_suffix)
{
  const char *name = blk.getStr("name");
  String varName = String("var") + name;
  int childNid = blk.getNameId("child");
  if (childNid != -1)
    for (int i = 0; i < blk.blockCount(); i++)
      if (blk.getBlock(i)->getBlockNameId() == childNid)
      {
        const DataBlock *cblk = blk.getBlock(i);
        const char *nm = cblk->getStr("name", NULL);
        if (!nm)
        {
          graph.unregisterBlendNode(this);
          ANIM_ERR("field <name> is not defined in <%s> hub child", name);
          return;
        }

        IAnimBlendNode *n = graph.getBlendNodePtr(nm, nm_suffix);
        if (!n)
        {
          n = graph.getBlendNodePtr(nm);
          if (n && !n->isSubOf(AnimBlendCtrl_HubCID))
            n = NULL;
        }
        if (!n)
        {
          const bool isOptional = cblk->getBool("optional", false);
          if (!isOptional)
          {
            graph.unregisterBlendNode(this);
            if (!is_ignoring_unavailable_resources())
              ANIM_ERR("blend node <%s> (suffix=%s) not found!", nm, nm_suffix);
            return;
          }
          else
            continue;
        }
        addBlendNode(n, cblk->getBool("enabled", true), cblk->getReal("weight", 1.0f));
      }
  if (!blk.getBool("const", false))
    finalizeInit(graph, varName);
}

void AnimBlendCtrl_Hub::checkHasLoop(AnimationGraph &graph,
  eastl::bitvector<eastl::allocator, uint32_t, eastl::vector<uint32_t, eastl::allocator>> &visited_nodes)
{
  int id = getAnimNodeId();
  if (id == -1)
    return;

  visited_nodes[id] = true;
  for (const Ptr<IAnimBlendNode> child : nodes)
  {
    int childId = child->getAnimNodeId();
    if (childId == -1)
      continue;

    if (visited_nodes[childId])
    {
      ANIM_ERR("Find loop in controller childs, controller: %s, parent: %s", graph.getAnimNodeName(childId),
        graph.getAnimNodeName(id));
      nodes.clear();
      break;
    }
    else
      child->checkHasLoop(graph, visited_nodes);
  }
  visited_nodes[id] = false;
}

void AnimBlendCtrl_Blender::createNode(AnimationGraph &graph, const DataBlock &blk)
{
  const char *name = blk.getStr("name", NULL);
  const char *_vname = blk.getStr("varname", NULL);
  String var_name;

  if (!name)
  {
    ANIM_ERR("not found 'name' param");
    return;
  }

  if (_vname)
    var_name = _vname;
  else
    var_name = String("var") + name;

  AnimBlendCtrl_Blender *node = new AnimBlendCtrl_Blender(graph, var_name);
  node->setDuration(blk.getReal("duration", 1.0f));
  node->setBlendTime(blk.getReal("morph", 1.0f));

  graph.registerBlendNode(node, name);
}

void AnimBlendCtrl_Blender::initChilds(AnimationGraph &graph, const DataBlock &blk, const char * /*nm_suffix*/)
{
  const char *anim;
  IAnimBlendNode *n;

  anim = blk.getStr("node1", NULL);
  if (anim)
  {
    n = graph.getBlendNodePtr(anim);
    if (!n)
    {
      ANIM_ERR("blend node <%s> not found!", anim);
      graph.unregisterBlendNode(this);
      return;
    }
    setBlendNode(0, n);
  }
  anim = blk.getStr("node2", NULL);
  if (anim)
  {
    n = graph.getBlendNodePtr(anim);
    if (!n)
    {
      ANIM_ERR("blend node <%s> not found!", anim);
      graph.unregisterBlendNode(this);
      return;
    }
    setBlendNode(1, n);
  }
}

void AnimBlendCtrl_Blender::checkHasLoop(AnimationGraph &graph,
  eastl::bitvector<eastl::allocator, uint32_t, eastl::vector<uint32_t, eastl::allocator>> &visited_nodes)
{
  int id = getAnimNodeId();
  if (id == -1)
    return;

  visited_nodes[id] = true;
  for (const Ptr<IAnimBlendNode> child : node)
  {
    int childId = child->getAnimNodeId();
    if (childId == -1)
      continue;

    if (visited_nodes[childId])
    {
      ANIM_ERR("Find loop in controller childs, controller: %s, parent: %s", graph.getAnimNodeName(childId),
        graph.getAnimNodeName(id));
      node[0] = nullptr;
      node[1] = nullptr;
      break;
    }
    else
      child->checkHasLoop(graph, visited_nodes);
  }
  visited_nodes[id] = false;
}

void AnimBlendCtrl_BinaryIndirectSwitch::createNode(AnimationGraph &graph, const DataBlock &blk)
{
  const char *name = blk.getStr("name", NULL);
  const char *_vname = blk.getStr("varname", NULL);
  String var_name;


  if (!name)
  {
    ANIM_ERR("not found 'name' param");
    return;
  }

  if (_vname)
    var_name = _vname;
  else
    var_name = String("var") + name;

  AnimBlendCtrl_BinaryIndirectSwitch *node =
    new AnimBlendCtrl_BinaryIndirectSwitch(graph, var_name, blk.getInt("maskAnd", 0), blk.getInt("maskEq", 0));

  graph.registerBlendNode(node, name);
}

void AnimBlendCtrl_BinaryIndirectSwitch::initChilds(AnimationGraph &graph, const DataBlock &blk, const char * /*nm_suffix*/)
{
  const char *anim;
  IAnimBlendNode *n1, *n2, *ctrl;

  anim = blk.getStr("node1", NULL);
  if (!anim)
  {
    graph.unregisterBlendNode(this);
    ANIM_ERR("node1 not defined!");
    return;
  }

  n1 = graph.getBlendNodePtr(anim);
  if (!n1)
  {
    graph.unregisterBlendNode(this);
    ANIM_ERR("blend node <%s> not found!", anim);
    return;
  }

  anim = blk.getStr("node2", NULL);
  if (!anim)
  {
    graph.unregisterBlendNode(this);
    ANIM_ERR("node1 not defined!");
    return;
  }
  n2 = graph.getBlendNodePtr(anim);
  if (!n2)
  {
    graph.unregisterBlendNode(this);
    ANIM_ERR("blend node <%s> not found!", anim);
    return;
  }

  anim = blk.getStr("fifo", NULL);
  if (!anim)
  {
    graph.unregisterBlendNode(this);
    ANIM_ERR("node1 not defined!");
    return;
  }
  ctrl = graph.getBlendNodePtr(anim);
  if (!ctrl)
  {
    graph.unregisterBlendNode(this);
    ANIM_ERR("blend node <%s> (fifo) not found!", anim);
    return;
  }
  if (!ctrl->isSubOf(AnimBlendCtrl_Fifo3CID))
  {
    graph.unregisterBlendNode(this);
    ANIM_ERR("blend node <%s> is not fifo3!", anim);
    return;
  }

  setBlendNodes(n1, blk.getReal("morph1", 0.0f), n2, blk.getReal("morph2", 0.0f));
  setFifoCtrl((AnimBlendCtrl_Fifo3 *)ctrl);
}

void AnimBlendCtrl_BinaryIndirectSwitch::checkHasLoop(AnimationGraph &graph,
  eastl::bitvector<eastl::allocator, uint32_t, eastl::vector<uint32_t, eastl::allocator>> &visited_nodes)
{
  int id = getAnimNodeId();
  if (id == -1)
    return;

  visited_nodes[id] = true;
  for (const Ptr<IAnimBlendNode> child : node)
  {
    int childId = child->getAnimNodeId();
    if (childId == -1)
      continue;

    if (visited_nodes[childId])
    {
      ANIM_ERR("Find loop in controller childs, controller: %s, parent: %s", graph.getAnimNodeName(childId),
        graph.getAnimNodeName(id));
      node[0] = nullptr;
      node[1] = nullptr;
      break;
    }
    else
      child->checkHasLoop(graph, visited_nodes);
  }
  visited_nodes[id] = false;
}

void AnimBlendCtrl_SetMotionMatchingTag::createNode(AnimationGraph &graph, const DataBlock &blk, const char *nm_suffix)
{
  const char *name = blk.getStr("name", NULL);
  if (!name)
  {
    ANIM_ERR("not found 'name' param");
    return;
  }
  const char *tag = blk.getStr("mmTag", NULL);
  if (!tag)
  {
    ANIM_ERR("not found 'mmTag' param");
    return;
  }
  AnimBlendCtrl_SetMotionMatchingTag *node = new AnimBlendCtrl_SetMotionMatchingTag;
  int tagsMaskSize = (MAX_TAGS_COUNT + 7) / 8; // one bit for each tag
  node->tagsMaskParamId = graph.addInlinePtrParamId("motion_matching_tags", tagsMaskSize, IPureAnimStateHolder::PT_InlinePtr);
  node->tagName = tag;
  graph.registerBlendNode(node, name, nm_suffix);
}


static FastNameMapTS<false, SpinLockReadWriteLock> enumMap;

int AnimV20::getEnumValueByName(const char *name) { return enumMap.getNameId(name); }
int AnimV20::addEnumValue(const char *name) { return enumMap.addNameId(name); }
const char *AnimV20::getEnumName(const int enum_id)
{
  if ((unsigned)enum_id < enumMap.nameCountRelaxed())
  {
    return enumMap.getName(enum_id);
  }
  return NULL;
}

static FastNameMapTS<false, SpinLockReadWriteLock> irqNames;
int AnimV20::addIrqId(const char *irq_name) { return irqNames.addNameId(irq_name); }
const char *AnimV20::getIrqName(int irq_id)
{
  if ((unsigned)irq_id < AnimV20::GIRQT_FIRST_SERVICE_IRQ)
  {
    return irqNames.getName(irq_id);
  }
  if (irq_id == AnimV20::GIRQT_EndOfSingleAnim)
    return "::eoa.single";
  if (irq_id == AnimV20::GIRQT_EndOfContinuousAnim)
    return "::eoa.continuous";
  return NULL;
}

#if DAGOR_DBGLEVEL > 0
static constexpr const char *debugAnimParamIrqName = "debugAnimParam";
static bool debugAnimParamEnabled = false;
static int debugAnimParamIrqId = -1;

void AnimV20::setDebugAnimParam(bool enable) { debugAnimParamEnabled = enable; }

bool AnimV20::getDebugAnimParam() { return debugAnimParamEnabled; }

int AnimV20::getDebugAnimParamIrqId()
{
  if (debugAnimParamIrqId < 0)
    debugAnimParamIrqId = addIrqId(debugAnimParamIrqName);
  return debugAnimParamIrqId;
}
#else
void AnimV20::setDebugAnimParam(bool) {}
bool AnimV20::getDebugAnimParam() { return false; }
int AnimV20::getDebugAnimParamIrqId() { return -1; }
#endif
