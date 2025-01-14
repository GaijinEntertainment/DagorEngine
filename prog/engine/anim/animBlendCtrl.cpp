// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <anim/dag_animBlend.h>
#include <anim/dag_animBlendCtrl.h>
#include "animFifo.h"
#include <anim/dag_animIrq.h>
#include <animChar/dag_animCharacter2.h>
#include <generic/dag_sort.h>
#include <ioSys/dag_dataBlock.h>
#include <math/random/dag_random.h>
#include <math/dag_Point4.h>
#include <math/dag_mathUtils.h>
#include <debug/dag_debug.h>
#include <debug/dag_fatal.h>
#include <math/random/dag_random.h>
#include <stdlib.h>
#include <ctype.h>
#include <EASTL/hash_map.h>
#include <EASTL/fixed_vector.h>

using namespace AnimV20;

static bool validate_node_not_used(AnimationGraph &g, IAnimBlendNode *main, IAnimBlendNode *n, int index, IAnimBlendNode *test_n)
{
  if (n == test_n)
  {
    G_UNREFERENCED(main);
    G_UNREFERENCED(index);
    G_UNREFERENCED(g);
    logerr("animnode \"%s\" uses missing (Null) anim (index %d)", g.getBlendNodeName(main), index);
    return false;
  }
  return true;
}

template <typename NodeList, typename Callable>
static void process_nodes_with_bitmap(NodeList &list, IPureAnimStateHolder &st, dag::ConstSpan<int> rewind_bitmap_params_ids,
  Callable c)
{
  uint32_t bitmap = 0;
  int paramIdToClear = -1;
  for (uint32_t i = 0, sz = list.size(), bit = 1; i < sz; ++i, bit = (bit << 1) | (bit >> 31))
  {
    if (i % 32 == 0)
    {
      if (paramIdToClear >= 0)
        st.setParamInt(paramIdToClear, 0);
      int bitmapId = rewind_bitmap_params_ids[i >> 5];
      bitmap = st.getParamInt(bitmapId);
      paramIdToClear = bitmapId;
    }
    c(list[i].node, bitmap & bit);
  }
  if (paramIdToClear >= 0)
    st.setParamInt(paramIdToClear, 0);
}

static void set_rewind_bit(uint32_t bit_idx, IPureAnimStateHolder &st, dag::ConstSpan<int> rewind_bitmap_params_ids)
{
  int bitmapId = rewind_bitmap_params_ids[bit_idx >> 5]; // >> 5 == / 32
  uint32_t bitmap = st.getParamInt(bitmapId);
  st.setParamInt(bitmapId, bitmap | (1 << (bit_idx % 32)));
}

static void add_rewind_bitmap(Tab<int> &rewind_bitmap, AnimationGraph &graph, const char *param_name)
{
  const size_t bitmapNum = rewind_bitmap.size();
  const int bitmapParamId =
    graph.addParamId(String(128, ":%s_rewlist_bitmap_%d", param_name, bitmapNum), IPureAnimStateHolder::PT_ScalarParamInt);
  rewind_bitmap.push_back(bitmapParamId);
}

// Irq position struct to be used in BNLs
void IrqPos::setupIrqs(Tab<IrqPos> &irqs, AnimData *anim, int t0, int dt, const DataBlock &blk, const char *anim_suffix)
{
  irqs.reserve(blk.blockCount());
  bool def_irq_en = blk.getBool("defIrqEnabled", true);
  if (anim_suffix)
    def_irq_en = blk.getBool(String(0, "%s_IrqEnabled", anim_suffix), def_irq_en);
  for (int i = 0, nid = blk.getNameId("irq"); i < blk.blockCount(); i++)
  {
    const DataBlock &irqBlk = *blk.getBlock(i);
    if (irqBlk.getBlockNameId() != nid)
      continue;

    if (!irqBlk.getBool(anim_suffix ? String(0, "%s_enabled", anim_suffix).str() : "enabled", def_irq_en))
      continue;

    int ts = 0;
    if (irqBlk.paramExists("relPos"))
    {
      float relPos = irqBlk.getReal("relPos", 0.0f);
      ts = t0 + int(dt * relPos);
      if (ts < t0 || ts > t0 + dt)
      {
        logerr("%s: bad irq <%s> pos relPos=%.2f (%d ticks)  is out of range (%d..%d) ticks", blk.getStr("name"),
          irqBlk.getStr("name"), relPos, ts, t0, t0 + dt);
        continue;
      }
    }
    else if (irqBlk.paramExists("keyFloat"))
    {
      ts = t0 + irqBlk.getReal("keyFloat") * TIME_TicksPerSec;
      if (ts < t0 || ts > t0 + dt)
      {
        logerr("%s: bad irq <%s> %d ticks  is out of range (%d..%d) ticks", blk.getStr("name"), irqBlk.getStr("name"), ts, t0,
          t0 + dt);
        continue;
      }
    }
    else
    {
      const char *key = irqBlk.getStr("key");
      ts = anim->getLabelTime(key, !isdigit(key[0]));
      if (ts < 0)
        ts = atoi(key) * TIME_TicksPerSec / 30;

      if (ts < t0 || ts > t0 + dt)
      {
        logerr("%s: bad irq <%s> pos <%s>=%d ticks  is out of range (%d..%d) ticks", blk.getStr("name"), irqBlk.getStr("name"), key,
          ts, t0, t0 + dt);
        continue;
      }
    }

    IrqPos &r = irqs.push_back();
    r.irqId = AnimV20::addIrqId(irqBlk.getStr("name"));
    r.irqTime = ts;
    // debug("%p.irq[%d]=%d, %d", this, irqs.size()-1, r.irqId, r.irqTime);
  }
  irqs.shrink_to_fit();
  sort(irqs, &IrqPos::cmp_irq_time_id);
}

void IrqPos::debug_set_irq_pos(const char *irq_name, float rel_pos, Tab<IrqPos> &irqs, int t0, int dt)
{
  const int irqId = AnimV20::addIrqId(irq_name);
  auto it = eastl::find_if(irqs.begin(), irqs.end(), [irqId](const IrqPos &irqPos) { return irqPos.irqId == irqId; });
  if (it == irqs.end())
    return;
  const int irqTime = t0 + int(dt * rel_pos);
  if (irqTime < t0 || irqTime > t0 + dt)
  {
    logerr("bad irq <%s> pos rel_pos=%.2f (%d ticks) is out of range (%d..%d) ticks", irq_name, rel_pos, irqTime, t0, t0 + dt);
    return;
  }
  it->irqTime = irqTime;
  sort(irqs, &IrqPos::cmp_irq_time_id);
}

// Basic blend node leaf implementation
AnimBlendNodeLeaf::AnimBlendNodeLeaf(AnimationGraph &g, AnimData *a) :
  anim(a),
  graph(g),
  timeRatio(0),
  ignoredAnimation(false),
  namedRanges(midmem),
  dontUseOriginVel(false),
  additive(a ? a->isAdditive() : false),
  foreignAnimation(false),
  timeScaleParamId(-1)
{
  bnlId = graph.registerBlendNodeLeaf(this);
  memset(&addOriginVel, 0, sizeof(addOriginVel));
}


AnimBlendNodeLeaf::~AnimBlendNodeLeaf()
{
  AnimBlendNodeLeaf::destroy();
  if (bnlId != -1)
  {
    graph.unregisterBlendNodeLeaf(bnlId, this);
    bnlId = -1;
  }
}


void AnimBlendNodeLeaf::setTimeScaleParam(const char *name)
{
  if (!name)
  {
    timeScaleParamId = -1;
    return;
  }

  timeScaleParamId = graph.addParamId(name, IPureAnimStateHolder::PT_ScalarParam);
}


void AnimBlendNodeLeaf::destroy()
{
  if (anim)
  {
    anim = NULL;
  }
}
void AnimBlendNodeLeaf::enableOriginVel(bool en) { dontUseOriginVel = !en; }
void AnimBlendNodeLeaf::setAdditive(bool add) { additive = add; }
void AnimBlendNodeLeaf::setAddOriginVel(const Point3 &add_ov) { memcpy(&addOriginVel, &add_ov, sizeof(add_ov)); }

int AnimBlendNodeLeaf::_cmp_rangenames(const void *p1, const void *p2)
{
  NamedRange *nr1 = (NamedRange *)p1, *nr2 = (NamedRange *)p2;
  return nr1->rangeId - nr2->rangeId;
}
void AnimBlendNodeLeaf::buildNamedRanges(const DataBlock *blk)
{
  int i;

  clear_and_shrink(namedRanges);

  if (!blk)
    return;

  int rnum = 0;
  for (i = 0; i < blk->blockCount(); i++)
  {
    const DataBlock *cb = blk->getBlock(i);
    if (cb)
    {
      const char *label_name = cb->getBlockName();
      if (label_name)
      {
        graph.addNamedRange(label_name);
        rnum++;
      }
    }
  }

  namedRanges.resize(rnum);
  rnum = 0;
  for (i = 0; i < blk->blockCount(); i++)
  {
    const DataBlock *cb = blk->getBlock(i);
    const char *label_name = cb->getBlockName();
    const char *lStart = cb->getStr("start", NULL);
    const char *lEnd = cb->getStr("end", NULL);
    IPoint2 r;
    real rpos;

    if (label_name && lStart && lEnd)
    {
      int id = graph.getNamedRangeId(label_name);
      if (id != -1)
      {
        namedRanges[rnum].rangeId = id;

        r = IPoint2(anim->getLabelTime(lStart, true), anim->getLabelTime(lEnd, true));
        rpos = cb->getReal("relative_start", 0.0f);
        namedRanges[rnum].range[0] = int(r[1] * rpos + r[0] * (1 - rpos));
        rpos = cb->getReal("relative_end", 1.0f);
        namedRanges[rnum].range[1] = int(r[1] * rpos + r[0] * (1 - rpos));
        // DEBUG_CTX("%s: (%d,%d) -> (%d,%d)", label_name, r[0], r[1], namedRanges[rnum].range[0], namedRanges[rnum].range[1]);
        rnum++;
      }
    }
  }

  qsort(&namedRanges[0], namedRanges.size(), elem_size(namedRanges), _cmp_rangenames);
  // DEBUG_CTX("ranges.count=%d", namedRanges.size());
}


//
// Continuous leaf blend node implemenations
//
AnimBlendNodeContinuousLeaf::AnimBlendNodeContinuousLeaf(AnimationGraph &graph, AnimData *a, const char *time_param_name) :
  AnimBlendNodeLeaf(graph, a)
{
  paramId = graph.addParamId(time_param_name, IPureAnimStateHolder::PT_TimeParam);
  t0 = 0;
  dt = 1;
  rate = 0;
  syncTime = 0;
  duration = 0;
  avgSpeed = 0;
  eoaIrq = false;
  canRewind = false;
  startOffsetEnabled = false;
}
void AnimBlendNodeContinuousLeaf::buildBlendingList(BlendCtx &bctx, real w)
{
  if (paramId == -1 || bnlId == -1)
    return;
  BUILD_BLENDING_LIST_PROLOGUE_BNL(bctx, st, wt);

  real p = st.getParam(paramId);
  if (eoaIrq && bctx.lastDt)
    if (!st.getParamFlags(paramId, IPureAnimStateHolder::PF_Paused) && p > duration - 1.5f / fabs(rate))
    {

      bctx.irq(GIRQT_EndOfContinuousAnim, (intptr_t)this, 0, 0);
      for (int i = irqs.size() - 1; i >= 0; i--)
        if (irqs[i].irqTime >= t0 + dt - 1)
          bctx.irq(irqs[i].irqId, (intptr_t)this, t0 + dt - 1, 0);
        else
          break;
      p -= duration;
      st.setParam(paramId, p);
    }

  int curTime = calcAnimTimePos(p);
  if (irqs.size() && bctx.lastDt && w > 0 && !wt[bnlId])
  {
    int prevTime = calcAnimTimePos(p - bctx.lastDt * st.getParamEffTimeScale(paramId));
    for (int i = 0; i < irqs.size(); i++)
      if (irqs[i].irqTime > curTime)
        break;
      else if (prevTime < irqs[i].irqTime || (prevTime == 0 && irqs[i].irqTime == 0))
        bctx.irq(irqs[i].irqId, (intptr_t)this, curTime, 0);
  }

#if DAGOR_DBGLEVEL > 0
  if (getDebugAnimParam())
    bctx.irq(getDebugAnimParamIrqId(), (intptr_t)this, curTime - t0, dt);
#endif

  wt[bnlId] += w;
  bctx.cKeyPos[bnlId] = curTime;

  // DEBUG_CTX("%p.[%d]=%.5f   p*rate=%.0f  curTime=%7d", this, paramId, st.getParam(paramId), p*rate, curTime);
}
void AnimBlendNodeContinuousLeaf::seek(IPureAnimStateHolder &st, real rel_pos)
{
  if (paramId == -1 || bnlId == -1)
    return;
  real pos = rel_pos * dt;
  st.setParam(paramId, pos / rate);
}
real AnimBlendNodeContinuousLeaf::tell(IPureAnimStateHolder &st)
{
  if (paramId == -1 || bnlId == -1)
    return 0;
  int t = int(st.getParam(paramId) * rate) % dt;
  return real(t) / real(dt);
}
void AnimBlendNodeContinuousLeaf::reset(IPureAnimStateHolder &st)
{
  if (timeScaleParamId > 0)
  {
    st.setTimeScaleParamId(paramId, timeScaleParamId);
    st.setParam(timeScaleParamId, 1.0f); // timescale defaults to 1.0
  }
  else
    st.setTimeScaleParamId(paramId, 0);
}

bool AnimBlendNodeContinuousLeaf::isInRange(IPureAnimStateHolder &st, int rangeId)
{
  int i, ct;
  if (paramId == -1 || bnlId == -1 || rangeId < 0)
    return false;

  for (i = 0; i < namedRanges.size(); i++)
    if (namedRanges[i].rangeId == rangeId)
    {
      ct = calcAnimTimePos(st.getParam(paramId));
      for (; i < namedRanges.size(); i++)
        if (namedRanges[i].rangeId != rangeId)
          return false;
        else if (ct >= namedRanges[i].range[0] && ct <= namedRanges[i].range[1])
          return true;
      return false;
    }

  return false;
}
void AnimBlendNodeContinuousLeaf::setAnim(AnimData *a)
{
  anim = a;
  additive = a ? a->isAdditive() : false;
}
void AnimBlendNodeContinuousLeaf::setRange(int tStart, int tEnd, real anim_time, float move_dist, const char *name)
{
  G_UNREFERENCED(name);
  if (anim_time < 0.0001f)
    DAG_FATAL("incorrect anim time=%.5f", anim_time);

  duration = anim_time;
  avgSpeed = (duration > 0) ? move_dist / duration : 0.0f;
  t0 = tStart;
  dt = tEnd - tStart;
  if (dt > 0)
    dt++;
  else
    dt--;
  if (dt < 0)
    logerr("incorrect animation <%s> direction (reverse)", name);
  rate = dt / anim_time;

  timeRatio = rate;
}
void AnimBlendNodeContinuousLeaf::setRange(const char *keyStart, const char *keyEnd, real anim_time, float move_dist, const char *name)
{
  if (!anim)
  {
    DEBUG_CTX("setRange ( %s, %s, %.3f ) while anim=NULL", keyStart, keyEnd, anim_time);
    return;
  }
  int ts = anim->getLabelTime(keyStart, !isdigit(keyStart[0]));
  int te = anim->getLabelTime(keyEnd, !isdigit(keyEnd[0]));
  if (ts < 0)
    ts = atoi(keyStart) * TIME_TicksPerSec / 30;
  if (te < 0)
    te = atoi(keyEnd) * TIME_TicksPerSec / 30;

  setRange(ts, te, anim_time, move_dist, name);
}
void AnimBlendNodeContinuousLeaf::setSyncTime(const char *syncKey)
{
  if (!anim)
  {
    DEBUG_CTX("setSyncTime ( %s ) while anim=NULL", syncKey);
    return;
  }
  int ts = anim->getLabelTime(syncKey, !isdigit(syncKey[0]));
  if (ts < 0)
    ts = atoi(syncKey) * TIME_TicksPerSec / 30;
  if (fabs(rate) < 0.00001f)
    syncTime = 0;
  else
    syncTime = (ts - t0) / rate;
}
void AnimBlendNodeContinuousLeaf::setSyncTime(real stime) { syncTime = stime; }
void AnimBlendNodeContinuousLeaf::setEndOfAnimIrq(bool on) { eoaIrq = on; }
void AnimBlendNodeContinuousLeaf::enableRewind(bool en) { canRewind = en; }
void AnimBlendNodeContinuousLeaf::enableStartOffset(bool en) { startOffsetEnabled = en; }
bool AnimBlendNodeContinuousLeaf::isStartOffsetEnabled() { return startOffsetEnabled; }
void AnimBlendNodeContinuousLeaf::pause(IPureAnimStateHolder &st)
{
  if (paramId != AnimationGraph::PID_GLOBAL_TIME)
    st.setParamFlags(paramId, IPureAnimStateHolder::PF_Paused, IPureAnimStateHolder::PF_Paused);
}
void AnimBlendNodeContinuousLeaf::resume(IPureAnimStateHolder &st, bool rewind)
{
  if (paramId != AnimationGraph::PID_GLOBAL_TIME)
    st.setParamFlags(paramId, 0, IPureAnimStateHolder::PF_Paused);
  if (rewind && canRewind)
    AnimBlendNodeContinuousLeaf::seekToSyncTime(st, 0.f);
}
void AnimBlendNodeContinuousLeaf::seekToSyncTime(IPureAnimStateHolder &st, real offset)
{
  if (paramId != AnimationGraph::PID_GLOBAL_TIME)
    st.setParam(paramId, syncTime + offset);
}

//
// Single animation
//
AnimBlendNodeSingleLeaf::AnimBlendNodeSingleLeaf(AnimationGraph &graph, AnimData *a, const char *time_param_name) :
  AnimBlendNodeContinuousLeaf(graph, a, time_param_name)
{
  canRewind = true;
}
void AnimBlendNodeSingleLeaf::buildBlendingList(BlendCtx &bctx, real w)
{
  IPureAnimStateHolder &st = *bctx.st;
  // DEBUG_CTX("%p.time=%.3f duration=%.3f", this, st.getParam(paramId), duration);
  real p = st.getParam(paramId);

  if (bctx.lastDt && p > duration - 1.5f / fabs(rate))
  {
    int resp = GIRQR_NoResponse;
    if (!st.getParamFlags(paramId, IPureAnimStateHolder::PF_Paused))
    {
      st.setParamFlags(paramId, IPureAnimStateHolder::PF_Paused, IPureAnimStateHolder::PF_Paused);
      st.setParam(paramId, duration - 1.5f / fabs(rate));
      bctx.st->getGraph().onSingleAnimFinished(*bctx.st, this);
      resp = bctx.irq(GIRQT_EndOfSingleAnim, (intptr_t)this, 0, 0);
      for (int i = irqs.size() - 1; i >= 0; i--)
        if (irqs[i].irqTime >= t0 + dt - 1)
          bctx.irq(irqs[i].irqId, (intptr_t)this, t0 + dt - 1, 0);
        else
          break;
    }

    if (resp != GIRQR_NoResponse)
    {
      if (resp == GIRQR_RewindSingleAnim)
      {
        st.setParam(paramId, p - duration);
        st.setParamFlags(paramId, 0, IPureAnimStateHolder::PF_Paused);
      }
      else if (resp == GIRQR_StopSingleAnim)
      {
        st.setParam(paramId, duration - 1.5f / fabs(rate));
        st.setParamFlags(paramId, IPureAnimStateHolder::PF_Paused, IPureAnimStateHolder::PF_Paused);
      }
      else if (resp == GIRQR_ResumeSingleAnim)
      {
        st.setParamFlags(paramId, 0, IPureAnimStateHolder::PF_Paused);
      }
    }
  }
  AnimBlendNodeContinuousLeaf::buildBlendingList(bctx, w);
}
void AnimBlendNodeSingleLeaf::seekToSyncTime(IPureAnimStateHolder &st, real offset)
{
  AnimBlendNodeContinuousLeaf::seekToSyncTime(st, offset);
  st.setParamFlags(paramId, 0, IPureAnimStateHolder::PF_Paused);
}
void AnimBlendNodeSingleLeaf::resume(IPureAnimStateHolder &st, bool rewind)
{
  st.setParamFlags(paramId, 0, IPureAnimStateHolder::PF_Paused);
  if (rewind)
    AnimBlendNodeContinuousLeaf::seekToSyncTime(st, 0);
}


//
// Parametric leaf blend node implemenations
//
AnimBlendNodeParametricLeaf::AnimBlendNodeParametricLeaf(AnimationGraph &graph, AnimData *a, const char *param_name, bool upd_pc) :
  AnimBlendNodeLeaf(graph, a), looping(false)
{
  paramId = graph.addParamId(param_name, IPureAnimStateHolder::PT_ScalarParam);
  paramLastId = -1;
  paramMulK = 1.0f;
  paramAddK = 0.0f;
  p0 = 0;
  dp = 1.0f;
  t0 = 0;
  dt = 1;
  updateOnParamChanged = upd_pc;
}

real AnimBlendNodeParametricLeaf::tell(IPureAnimStateHolder &st)
{
  if (paramId == -1 || bnlId == -1)
    return 0;

  real param = safediv(st.getParam(paramId) * paramMulK + paramAddK - p0, dp);
  if (looping)
    param -= floorf(param);
  else
    param = clamp(param, 0.0f, 1.0f);
  int curTime = int(t0 + (param)*dt);
  return real(curTime) / real(dt);
}
void AnimBlendNodeParametricLeaf::reset(IPureAnimStateHolder &st)
{
  if (paramLastId < 0)
    return;

  real param = safediv(st.getParam(paramId) * paramMulK + paramAddK - p0, dp);
  if (looping)
    param -= floorf(param);
  else
    param = clamp(param, 0.0f, 1.0f);

  if (looping)
    st.setParamInt(paramLastId, int(t0 + (param)*dt));
  else
    st.setParamInt(paramLastId, t0);
}

void AnimBlendNodeParametricLeaf::buildBlendingList(BlendCtx &bctx, real w)
{
  if (paramId == -1 || bnlId == -1)
    return;
  BUILD_BLENDING_LIST_PROLOGUE_BNL(bctx, st, wt);
  if (updateOnParamChanged && w >= 1 && bctx.animBlendPass)
  {
    if (!st.getParamFlags(paramId, st.PF_Changed))
      return;
    st.setParamFlags(paramId, 0, st.PF_Changed);
  }

  real param = safediv(st.getParam(paramId) * paramMulK + paramAddK - p0, dp);

  if (looping)
    param -= floorf(param);
  else
  {
    if (param < 0)
      param = 0;
    else if (param > 1.0f)
      param = 1.0f;
  }

  const int curTime = int(t0 + param * dt);

#if DAGOR_DBGLEVEL > 0
  if (getDebugAnimParam())
    bctx.irq(getDebugAnimParamIrqId(), (intptr_t)this, curTime - t0, dt);
#endif

  if (paramLastId >= 0)
  {
    const int last_ct = st.getParamInt(paramLastId);
    if (last_ct >= 0 && last_ct != curTime)
    {
      const int ct0 = curTime > last_ct ? last_ct : curTime;
      const int ct1 = curTime > last_ct ? curTime : last_ct;
      if ((ct1 - ct0) * 2 < dt) // ignore param rewind
        for (auto &irq : irqs)
        {
          if (irq.irqTime > ct1)
            break;
          else if (ct0 < irq.irqTime && irq.irqTime != last_ct)
            bctx.irq(irq.irqId, (intptr_t)this, curTime, curTime - last_ct);
        }
    }
    st.setParamInt(paramLastId, curTime);
  }
  wt[bnlId] += w;
  bctx.cKeyPos[bnlId] = curTime;
}


bool AnimBlendNodeParametricLeaf::isInRange(IPureAnimStateHolder &st, int rangeId)
{
  int i, ct;
  if (paramId == -1 || bnlId == -1 || rangeId < 0)
    return false;

  for (i = 0; i < namedRanges.size(); i++)
    if (namedRanges[i].rangeId == rangeId)
    {
      real param = st.getParam(paramId) - p0;
      if (param < 0)
        param = 0;
      else if (param > dp)
        param = dp;

      ct = t0 + int(safediv(param, dp) * dt);

      for (; i < namedRanges.size(); i++)
        if (namedRanges[i].rangeId != rangeId)
          return false;
        else if (ct >= namedRanges[i].range[0] && ct <= namedRanges[i].range[1])
          return true;

      return false;
    }

  return false;
}
void AnimBlendNodeParametricLeaf::setAnim(AnimData *a)
{
  anim = a;
  additive = a ? a->isAdditive() : false;
}
void AnimBlendNodeParametricLeaf::setRange(int tStart, int tEnd, real _p0, real _p1, const char *name)
{
  G_UNREFERENCED(name);
  t0 = tStart;
  dt = tEnd - tStart;
  if (dt > 0)
    dt++;
  else
    dt--;
  if (dt < 0)
    logerr("incorrect animation <%s> direction (reverse)", name);
  p0 = _p0;
  dp = _p1 - _p0;
}
void AnimBlendNodeParametricLeaf::setRange(const char *keyStart, const char *keyEnd, real _p0, real _p1, const char *name)
{
  if (!anim)
  {
    DEBUG_CTX("setRange ( %s, %s, %.3f, %.3f ) while anim=NULL", keyStart, keyEnd, _p0, _p1);
    return;
  }
  int ts = anim->getLabelTime(keyStart, !isdigit(keyStart[0]));
  int te = anim->getLabelTime(keyEnd, !isdigit(keyEnd[0]));
  if (ts < 0)
    ts = atoi(keyStart) * TIME_TicksPerSec / 30;
  if (te < 0)
    te = atoi(keyEnd) * TIME_TicksPerSec / 30;

  setRange(ts, te, _p0, _p1, name);
}

void AnimBlendNodeParametricLeaf::setParamKoef(real mulk, real addk)
{
  paramMulK = mulk;
  paramAddK = addk;
}

void AnimBlendNodeParametricLeaf::setLooping(bool loop) { looping = loop; }


//
// Still leaf blend node implemenations
//
void AnimBlendNodeStillLeaf::buildBlendingList(BlendCtx &bctx, real w)
{
  if (bnlId == -1)
    return;
  bctx.wt[bnlId] += w;
  bctx.cKeyPos[bnlId] = ctPos;
}
void AnimBlendNodeStillLeaf::setAnim(AnimData *a)
{
  anim = a;
  additive = a ? a->isAdditive() : false;
}


//
// 1-axis blend node
//
AnimBlendCtrl_1axis::AnimBlendCtrl_1axis(AnimationGraph &graph, const char *param_name) : slice(midmem)
{
  paramId = graph.addParamId(param_name, IPureAnimStateHolder::PT_ScalarParam);
}
void AnimBlendCtrl_1axis::destroy() {}
void AnimBlendCtrl_1axis::reset(IPureAnimStateHolder & /*st*/) {}
bool AnimBlendCtrl_1axis::validateNodeNotUsed(AnimationGraph &g, IAnimBlendNode *test_n)
{
  bool valid = true;
  for (int i = 0; i < slice.size(); i++)
    if (!validate_node_not_used(g, this, slice[i].node, i, test_n))
      valid = false;
  return valid;
}
void AnimBlendCtrl_1axis::collectUsedBlendNodes(AnimationGraph &g, eastl::hash_map<IAnimBlendNode *, bool> &node_map)
{
  for (int i = 0; i < slice.size(); i++)
  {
    IAnimBlendNode *n = slice[i].node;
    if (node_map.find(n) == node_map.end())
    {
      node_map.emplace(n, true);
      n->collectUsedBlendNodes(g, node_map);
    }
  }
}
void AnimBlendCtrl_1axis::buildBlendingList(BlendCtx &bctx, real w)
{
  if (paramId == -1)
    return;
  BUILD_BLENDING_LIST_PROLOGUE(bctx, st);

  Tab<AnimTmpWeightedNode> active(tmpmem);
  AnimTmpWeightedNode wn;
  real param = st.getParam(paramId);
  int i, j;
  real cStart = 0, cEnd = 0;

  active.reserve(slice.size());
  for (i = 0; i < slice.size(); i++)
    if (slice[i].start <= param && param <= slice[i].end)
    {
      if (active.size() == 0)
      {
        // add first animation
        wn.n = slice[i].node;
        wn.w = w;
        append_items(active, 1, &wn);
        cStart = slice[i].start;
        cEnd = slice[i].end;
      }
      else
      {
        real t0 = slice[i].start, t1 = slice[i].end;
        real a0 = 0, a1 = 0, a;

        if (t0 < cStart)
        {
          t0 = cStart;
          cStart = slice[i].start;
          a0 = 0;
          a1 = 1;
        }
        if (t1 > cEnd)
        {
          t1 = cEnd;
          cEnd = slice[i].end;
          a0 = 1;
          a1 = 0;
        }

        if (t1 - t0 < 0.00001f)
          a = 0.5;
        else
          a = a0 + (param - t0) / (t1 - t0) * (a1 - a0);

        if (a < 0.001f || a >= 0.999f)
          continue;

        for (j = 0; j < active.size(); j++)
          active[j].w *= a;

        wn.n = slice[i].node;
        wn.w = w * (1 - a);
        append_items(active, 1, &wn);
      }
    }

  for (i = 0; i < active.size(); i++)
    active[i].n->buildBlendingList(bctx, active[i].w);
}
void AnimBlendCtrl_1axis::addBlendNode(IAnimBlendNode *n, real start, real end)
{
  int l = append_items(slice, 1);
  if (l < 0)
    DAG_FATAL("no mem");

  slice[l].node = n;
  slice[l].start = start;
  slice[l].end = end;
}


//
// Simplest linear blend node
//
AnimBlendCtrl_LinearPoly::AnimBlendCtrl_LinearPoly(AnimationGraph &graph, const char *param_name, const char *ctrl_name) :
  poly(midmem), morphTime(0), enclosed(false), paramTau(0.0f), paramSpeed(0.0f)
{
  paramId = graph.addParamId(param_name, IPureAnimStateHolder::PT_ScalarParam);
  t0Pid = graph.addParamId(String(128, ":lp_t0_%s", ctrl_name), IPureAnimStateHolder::PT_ScalarParam);
  curPosPid = graph.addParamId(String(128, ":lp_pos_%s", ctrl_name), IPureAnimStateHolder::PT_ScalarParam);
}
void AnimBlendCtrl_LinearPoly::destroy() {}
int AnimBlendCtrl_LinearPoly::anim_point_p0_cmp(const AnimBlendCtrl_LinearPoly::AnimPoint *p1,
  const AnimBlendCtrl_LinearPoly::AnimPoint *p2)
{
  if (p1->p0 > p2->p0)
    return 1;
  if (p1->p0 < p2->p0)
    return -1;
  return 0;
}
void AnimBlendCtrl_LinearPoly::reset(IPureAnimStateHolder & /*st*/) { sort(poly, &anim_point_p0_cmp); }
bool AnimBlendCtrl_LinearPoly::validateNodeNotUsed(AnimationGraph &g, IAnimBlendNode *test_n)
{
  bool valid = true;
  for (int i = 0; i < poly.size(); i++)
    if (!validate_node_not_used(g, this, poly[i].node, i, test_n))
      valid = false;
  return valid;
}
void AnimBlendCtrl_LinearPoly::collectUsedBlendNodes(AnimationGraph &g, eastl::hash_map<IAnimBlendNode *, bool> &node_map)
{
  for (int i = 0; i < poly.size(); i++)
  {
    IAnimBlendNode *n = poly[i].node;
    if (node_map.find(n) == node_map.end())
    {
      node_map.emplace(n, true);
      n->collectUsedBlendNodes(g, node_map);
    }
  }
}
bool AnimBlendCtrl_LinearPoly::isAliasOf(IPureAnimStateHolder & /*st*/, IAnimBlendNode *n)
{
  if (n == this)
    return true;
  for (int i = 0; i < poly.size(); i++)
    if (n == poly[i].node)
      return true;
  return false;
}
void AnimBlendCtrl_LinearPoly::pause(IPureAnimStateHolder &st)
{
  for (int i = 0; i < poly.size(); i++)
    poly[i].node->pause(st);
}
void AnimBlendCtrl_LinearPoly::resume(IPureAnimStateHolder &st, bool rewind)
{
  process_nodes_with_bitmap(poly, st, rewindBitmapParamsIds,
    [&st, rewind](IAnimBlendNode *node, bool should_rewind) { node->resume(st, should_rewind && rewind); });
}
real AnimBlendCtrl_LinearPoly::getDuration(IPureAnimStateHolder &st) { return poly.size() ? poly[0].node->getDuration(st) : 1.0f; }
void AnimBlendCtrl_LinearPoly::AnimPoint::applyWt(BlendCtx &bctx, real wt, real node_w)
{
  node->buildBlendingList(bctx, wt * node_w);
  bctx.st->setParam(wtPid, wt);
}
void AnimBlendCtrl_LinearPoly::buildBlendingList(BlendCtx &bctx, real w)
{
  if (paramId == -1 || !poly.size())
    return;
  BUILD_BLENDING_LIST_PROLOGUE(bctx, st);

  real ctime = st.getParam(AnimationGraph::PID_GLOBAL_TIME);
  real dt = ctime - st.getParam(t0Pid);
  st.setParam(t0Pid, ctime);

  real param = st.getParam(paramId);
  const real tauThreshold = 0.01f;
  const real speedThreshold = 0.01f;
  if (enclosed && (paramTau > tauThreshold || paramSpeed > speedThreshold))
  {
    real minParam = poly[0].p0;
    real maxParam = poly.back().p0;
    real curPos = st.getParam(curPosPid);
    real diff = param - curPos;
    real range = maxParam - minParam;
    if (rabs(diff) > 0.5f * range)
      param = -sign(diff) * range;
    curPos = approach(curPos, param, dt, paramTau);
    curPos = move_to(curPos, param, dt, paramSpeed);
    if (curPos > maxParam)
      curPos -= range;
    else if (curPos < minParam)
      curPos += range;
    st.setParam(curPosPid, curPos);
    param = curPos;
  }
  int wishPt = 0;
  real alpha = 0.0f;
  if (param <= poly[0].p0)
    wishPt = 0;
  if (param >= poly.back().p0)
    wishPt = poly.size() - 1;

  for (int i = 0; i < (int)poly.size() - 1; ++i)
    if (param >= poly[i].p0 && param <= poly[i + 1].p0)
    {
      real dp = poly[i + 1].p0 - poly[i].p0;
      wishPt = i;
      alpha = safediv(param - poly[i].p0, dp);
      break;
    }

  // for some cases we want to go beyond the given limits (e.g. when standing up the person goes a bit above the standing up pose and
  // comes back) to do this we specifically don't clamp alpha here
  if (poly.size() == 2 && poly[0].p0 < poly[1].p0 && morphTime <= 0)
  {
    // apply unconstrained cross-blend
    wishPt = 0;
    alpha = safediv(param - poly[0].p0, poly[1].p0 - poly[0].p0);
  }

  const real alphaThreshold = 0.0001f; // if alpha is lower assume only one point is active
  const real morphThreshold = 0.001f;
  if (morphTime > morphThreshold)
  {
    const real morphSpeed = 1.0f / morphTime;
    real totalNewWeight = 0.0f;

    struct NodeWeight
    {
      int idx;
      real w;
    };

    eastl::fixed_vector<NodeWeight, 3> nonActiveMorphedNodes;

    for (int i = 0; i < poly.size(); ++i)
    {
      real wishWeight = 0.0f;
      if (i == wishPt)
      {
        if (fabsf(alpha) > alphaThreshold)
          wishWeight = 1.0f - alpha;
        else
          wishWeight = 1.0f;
      }
      else if (fabsf(alpha) > alphaThreshold && i == int(ceilf(wishPt + alpha)))
        wishWeight = alpha;

      const real curWeight = st.getParam(poly[i].wtPid);
      real newWeight = move_to(curWeight, wishWeight, dt, morphSpeed);
      if (wishWeight == 0.0f && newWeight > 0.0f)
      {
        if (nonActiveMorphedNodes.size() < nonActiveMorphedNodes.capacity())
        {
          if (nonActiveMorphedNodes.size() == 0 || newWeight < nonActiveMorphedNodes.back().w)
            nonActiveMorphedNodes.push_back({i, newWeight});
          else
          {
            int insertIndex = 0;
            for (; insertIndex < nonActiveMorphedNodes.size() && nonActiveMorphedNodes[insertIndex].w > newWeight; ++insertIndex)
              ;
            nonActiveMorphedNodes.insert(nonActiveMorphedNodes.begin() + insertIndex, {i, newWeight});
          }
        }
        else
        {
          if (newWeight < nonActiveMorphedNodes.back().w)
            newWeight = 0.0f;
          else
          {
            NodeWeight &node = nonActiveMorphedNodes.back();
            totalNewWeight -= node.w;
            st.setParam(poly[node.idx].wtPid, 0.0f);
            nonActiveMorphedNodes.pop_back();

            int insertIndex = 0;
            for (; insertIndex < nonActiveMorphedNodes.size() && nonActiveMorphedNodes[insertIndex].w > newWeight; ++insertIndex)
              ;
            nonActiveMorphedNodes.insert(nonActiveMorphedNodes.begin() + insertIndex, {i, newWeight});
          }
        }
      }

      totalNewWeight += newWeight;
      st.setParam(poly[i].wtPid, newWeight);
    }

    // Separate loop because we need totalNewWeight to normalize weights
    for (int i = 0; i < poly.size(); ++i)
    {
      real newWeight = totalNewWeight > 0.0f ? st.getParam(poly[i].wtPid) / totalNewWeight : st.getParam(poly[i].wtPid);
      if (newWeight > 0.0f)
      {
        poly[i].applyWt(bctx, newWeight, w);
        set_rewind_bit(i, st, rewindBitmapParamsIds);
      }
    }
    return;
  }

  set_rewind_bit(wishPt, st, rewindBitmapParamsIds);
  if (fabsf(alpha) > alphaThreshold)
  {
    poly[wishPt].applyWt(bctx, 1.0f - alpha, w);
    poly[wishPt + 1].applyWt(bctx, alpha, w);
    set_rewind_bit(wishPt + 1, st, rewindBitmapParamsIds);
  }
  else
    poly[wishPt].applyWt(bctx, 1.0f, w);
}
void AnimBlendCtrl_LinearPoly::addBlendNode(IAnimBlendNode *n, real p0, AnimationGraph &graph, const char *ctrl_name)
{
  int l = append_items(poly, 1);
  if (l < 0)
    DAG_FATAL("no mem");

  poly[l].node = n;
  poly[l].p0 = p0;
  poly[l].wtPid = graph.addParamId(String(128, ":lp_wt_%s%d", ctrl_name, l), IPureAnimStateHolder::PT_ScalarParam);

  if (l % 32 == 0) // We just exceeded our 32 bit limit in the last bitmap
    add_rewind_bitmap(rewindBitmapParamsIds, graph, ctrl_name);
}


//
// FIFO-3 blend controller (fast and simple; max queue length - 3 items)
//
AnimBlendCtrl_Fifo3::AnimBlendCtrl_Fifo3(AnimationGraph &graph, const char *fifo_param_name)
{
  fifoParamId = graph.addInlinePtrParamId(fifo_param_name, sizeof(AnimFifo3Queue), IPureAnimStateHolder::PT_Fifo3);
}
void AnimBlendCtrl_Fifo3::destroy() {}

void AnimBlendCtrl_Fifo3::buildBlendingList(BlendCtx &bctx, real w)
{
  BUILD_BLENDING_LIST_PROLOGUE(bctx, st);
  AnimFifo3Queue *fifo = (AnimFifo3Queue *)st.getInlinePtr(fifoParamId);
  fifo->buildBlendingList(bctx, st.getParam(AnimationGraph::PID_GLOBAL_TIME), w);
}
void AnimBlendCtrl_Fifo3::reset(IPureAnimStateHolder &st)
{
  if (!st.getParamIdValid(fifoParamId))
    return;
  AnimFifo3Queue *fifo = (AnimFifo3Queue *)st.getInlinePtr(fifoParamId);
  fifo->reset(false);
}

void AnimBlendCtrl_Fifo3::enqueueState(IPureAnimStateHolder &st, IAnimBlendNode *n, real overlap_time, real max_lag)
{
  AnimFifo3Queue *fifo = (AnimFifo3Queue *)st.getInlinePtr(fifoParamId);
  fifo->enqueueItem(st.getParam(AnimationGraph::PID_GLOBAL_TIME), st, n, overlap_time, max_lag);
}
void AnimBlendCtrl_Fifo3::resetQueue(IPureAnimStateHolder &st, bool leave_cur_state)
{
  AnimFifo3Queue *fifo = (AnimFifo3Queue *)st.getInlinePtr(fifoParamId);
  fifo->reset(leave_cur_state);
}
bool AnimBlendCtrl_Fifo3::isEnqueued(IPureAnimStateHolder &st, IAnimBlendNode *n)
{
  AnimFifo3Queue *fifo = (AnimFifo3Queue *)st.getInlinePtr(fifoParamId);
  return fifo->isEnqueued(n);
}

//
// Random switcher
//
AnimBlendCtrl_RandomSwitcher::AnimBlendCtrl_RandomSwitcher(AnimationGraph &graph, const char *param_name) : list(midmem)
{
  paramId = graph.addParamId(param_name, IPureAnimStateHolder::PT_ScalarParamInt);
  repParamId = graph.addParamId(String(128, ":repcnt_%s", param_name), IPureAnimStateHolder::PT_ScalarParamInt);
}

void AnimBlendCtrl_RandomSwitcher::destroy() {}

void AnimBlendCtrl_RandomSwitcher::reset(IPureAnimStateHolder &st) { st.setParamInt(paramId, -1); }
bool AnimBlendCtrl_RandomSwitcher::validateNodeNotUsed(AnimationGraph &g, IAnimBlendNode *test_n)
{
  bool valid = true;
  for (int i = 0; i < list.size(); i++)
    if (!validate_node_not_used(g, this, list[i].node, i, test_n))
      valid = false;
  return valid;
}

void AnimBlendCtrl_RandomSwitcher::collectUsedBlendNodes(AnimationGraph &g, eastl::hash_map<IAnimBlendNode *, bool> &node_map)
{
  for (int i = 0; i < list.size(); i++)
  {
    IAnimBlendNode *n = list[i].node;
    if (node_map.find(n) == node_map.end())
    {
      node_map.emplace(n, true);
      n->collectUsedBlendNodes(g, node_map);
    }
  }
}

IAnimBlendNode *AnimBlendCtrl_RandomSwitcher::getCurAnim(IPureAnimStateHolder &st)
{
  int id = st.getParamInt(paramId);
  if (id < 0 || id >= list.size())
  {
    st.setParamInt(paramId, -1);
    return NULL;
  }
  return list[id].node;
}

void AnimBlendCtrl_RandomSwitcher::buildBlendingList(BlendCtx &bctx, real w)
{
  BUILD_BLENDING_LIST_PROLOGUE(bctx, st);
  IAnimBlendNode *node = getCurAnim(st);
  if (node)
    node->buildBlendingList(bctx, w);
}
bool AnimBlendCtrl_RandomSwitcher::isAliasOf(IPureAnimStateHolder &st, IAnimBlendNode *n)
{
  if (this == n)
    return true;

  IAnimBlendNode *node = getCurAnim(st);
  return node && node == n;
}
real AnimBlendCtrl_RandomSwitcher::getDuration(IPureAnimStateHolder &st)
{
  IAnimBlendNode *node = getCurAnim(st);
  if (node)
    return node->getDuration(st);
  return 1.0f;
}
void AnimBlendCtrl_RandomSwitcher::seekToSyncTime(IPureAnimStateHolder &st, real offset)
{
  IAnimBlendNode *node = getCurAnim(st);
  if (node)
    node->seekToSyncTime(st, offset);
}
void AnimBlendCtrl_RandomSwitcher::pause(IPureAnimStateHolder &st)
{
  IAnimBlendNode *node = getCurAnim(st);
  if (node)
    node->pause(st);
}
void AnimBlendCtrl_RandomSwitcher::resume(IPureAnimStateHolder &st, bool rewind)
{
  if (rewind)
    setRandomAnim(st);

  IAnimBlendNode *node = getCurAnim(st);
  if (node)
    node->resume(st, rewind);
}
bool AnimBlendCtrl_RandomSwitcher::isInRange(IPureAnimStateHolder &st, int rangeId)
{
  IAnimBlendNode *node = getCurAnim(st);
  if (node)
    return node->isInRange(st, rangeId);
  return false;
}

void AnimBlendCtrl_RandomSwitcher::addBlendNode(IAnimBlendNode *n, real rnd_w, int max_rep)
{
  int l = append_items(list, 1);
  list[l].node = n;
  list[l].rndWt = rnd_w;
  list[l].maxRepeat = max_rep;
}
void AnimBlendCtrl_RandomSwitcher::recalcWeights(bool reverse)
{
  int i;
  double sum = 0, tmp;

  if (!reverse)
  {
    // normalize weights and make probability func
    for (i = 0; i < list.size(); i++)
      sum += list[i].rndWt;

    if (sum > 0)
    {
      for (i = 0; i < list.size(); i++)
        list[i].rndWt /= sum;

      // sum weights
      sum = 0;
      for (i = 0; i < list.size(); i++)
      {
        sum += list[i].rndWt;
        list[i].rndWt = sum;
      }
      list.back().rndWt = 1;
    }
  }
  else
  {
    // probability func -> explicit weights
    for (i = 0; i < list.size(); i++)
    {
      tmp = list[i].rndWt;
      list[i].rndWt -= sum;
      sum = tmp;
    }
  }
}

void AnimBlendCtrl_RandomSwitcher::setRandomAnim(IPureAnimStateHolder &st)
{
  if (list.size() < 1)
  {
    st.setParamInt(paramId, -1);
    return;
  }
  if (list.size() == 1)
  {
    st.setParamInt(paramId, 0);
    return;
  }

  int lastId = st.getParamInt(paramId);
  int lastIdRepeat = st.getParamInt(repParamId);

  for (;;)
  {
    real p = gfrnd();
    int id = -1, i;

    for (i = 0; i < list.size(); i++)
    {
      if (p < list[i].rndWt)
      {
        id = i;
        break;
      }
    }
    if (id == -1)
      id = list.size() - 1;

    if (lastIdRepeat >= list[id].maxRepeat && lastId == id)
      continue;

    if (lastId == id)
      lastIdRepeat++;
    else
      lastIdRepeat = 0;

    lastId = id;
    break;
  }

  st.setParamInt(paramId, lastId);
  st.setParamInt(repParamId, lastIdRepeat);
}
bool AnimBlendCtrl_RandomSwitcher::setAnim(IPureAnimStateHolder &st, IAnimBlendNode *n)
{
  if (list.size() < 1)
  {
    st.setParamInt(paramId, -1);
    return false;
  }

  for (int i = 0; i < list.size(); i++)
    if (list[i].node == n)
    {
      if (st.getParamInt(paramId) == i)
        st.setParamInt(repParamId, st.getParamInt(repParamId));
      else
      {
        st.setParamInt(paramId, i);
        st.setParamInt(repParamId, 0);
      }
      return true;
    }
  return false;
}

//
// Hub controller
//
void AnimBlendCtrl_Hub::finalizeInit(AnimationGraph &graph, const char *param_name)
{
  if (param_name && defNodeWt.size())
    paramId = graph.addInlinePtrParamId(param_name, sizeof(float) * defNodeWt.size());
}

inline const float *AnimBlendCtrl_Hub::getProps(IPureAnimStateHolder &st)
{
  return paramId < 0 ? defNodeWt.data() : (float *)st.getInlinePtr(paramId);
}

void AnimBlendCtrl_Hub::buildBlendingList(BlendCtx &bctx, real w)
{
  if (nodes.size() < 1)
    return;
  BUILD_BLENDING_LIST_PROLOGUE(bctx, st);
  const float *np = getProps(st);

  for (int i = 0; i < nodes.size(); i++)
    if (reinterpret_cast<const int *>(np)[i] != 0)
      nodes[i]->buildBlendingList(bctx, w * np[i]);
}
void AnimBlendCtrl_Hub::reset(IPureAnimStateHolder &st)
{
  if (paramId == -1 || !st.getParamIdValid(paramId))
    return;

  mem_copy_to(defNodeWt, st.getInlinePtr(paramId));
}
bool AnimBlendCtrl_Hub::validateNodeNotUsed(AnimationGraph &g, IAnimBlendNode *test_n)
{
  bool valid = true;
  for (int i = 0; i < nodes.size(); i++)
    if (!validate_node_not_used(g, this, nodes[i], i, test_n))
      valid = false;
  return valid;
}
void AnimBlendCtrl_Hub::collectUsedBlendNodes(AnimationGraph &g, eastl::hash_map<IAnimBlendNode *, bool> &node_map)
{
  for (int i = 0; i < nodes.size(); i++)
  {
    IAnimBlendNode *n = nodes[i];
    if (node_map.find(n) == node_map.end())
    {
      node_map.emplace(n, true);
      n->collectUsedBlendNodes(g, node_map);
    }
  }
}
bool AnimBlendCtrl_Hub::isAliasOf(IPureAnimStateHolder &st, IAnimBlendNode *n)
{
  if (n == this)
    return true;
  for (int i = 0; i < nodes.size(); i++)
    if (n == nodes[i] || nodes[i]->isAliasOf(st, n))
      return true;
  return false;
}
void AnimBlendCtrl_Hub::seekToSyncTime(IPureAnimStateHolder &st, real offset)
{
  const float *np = getProps(st);

  for (int i = 0; i < nodes.size(); i++)
    if (reinterpret_cast<const int *>(np)[i] != 0)
      nodes[i]->seekToSyncTime(st, offset);
}
void AnimBlendCtrl_Hub::pause(IPureAnimStateHolder &st)
{
  const float *np = getProps(st);

  for (int i = 0; i < nodes.size(); i++)
    if (reinterpret_cast<const int *>(np)[i] != 0)
      nodes[i]->pause(st);
}
void AnimBlendCtrl_Hub::resume(IPureAnimStateHolder &st, bool rewind)
{
  const float *np = getProps(st);

  for (int i = 0; i < nodes.size(); i++)
    if (reinterpret_cast<const int *>(np)[i] != 0)
      nodes[i]->resume(st, rewind);
}
bool AnimBlendCtrl_Hub::isInRange(IPureAnimStateHolder &st, int rangeId)
{
  const float *np = getProps(st);

  for (int i = 0; i < nodes.size(); i++)
    if (reinterpret_cast<const int *>(np)[i] != 0)
      if (nodes[i]->isInRange(st, rangeId))
        return true;
  return false;
}

void AnimBlendCtrl_Hub::addBlendNode(IAnimBlendNode *n, bool active, real wt)
{
  G_ASSERTF_RETURN(paramId < 0, , "%s may be called only before finalizeInit()", __FUNCTION__);
  for (int i = 0; i < nodes.size(); i++)
    if (nodes[i] == n)
      return;

  nodes.push_back(n);
  defNodeWt.push_back(active ? wt : 0);
}
void AnimBlendCtrl_Hub::setBlendNodeWt(IPureAnimStateHolder &st, IAnimBlendNode *n, real wt)
{
  if (paramId == -1)
    return;

  for (int i = 0; i < nodes.size(); i++)
    if (n == nodes[i])
    {
      ((float *)st.getInlinePtr(paramId))[i] = wt;
      return;
    }
}

int AnimBlendCtrl_Hub::getNodeIndex(IAnimBlendNode *n)
{
  for (int i = 0; i < nodes.size(); i++)
    if (n == nodes[i])
      return i;
  return -1;
}

//
// Simple blender - blends 2 nodes for blendTime and then lasts winth node2 until duration time runs out
//
AnimBlendCtrl_Blender::AnimBlendCtrl_Blender(AnimationGraph &graph, const char *param_name)
{
  paramId = graph.addParamId(param_name, IPureAnimStateHolder::PT_TimeParam);
  duration = 1.0f;
  blendTime = 1.0f;
}

void AnimBlendCtrl_Blender::destroy() {}

void AnimBlendCtrl_Blender::buildBlendingList(BlendCtx &bctx, real w)
{
  BUILD_BLENDING_LIST_PROLOGUE(bctx, st);
  real t = st.getParam(paramId);

  if (t > duration)
  {
    if (t < duration + 8)
    {
      bctx.st->getGraph().onSingleAnimFinished(*bctx.st, this);
      bctx.irq(GIRQT_EndOfSingleAnim, (intptr_t)this, 0, 0);
    }
    st.setParam(paramId, duration + 12);
  }
  if (t > duration || t < 0)
    return;

  if (t < blendTime)
  {
    real k = t / blendTime;
    if (node[0])
      node[0]->buildBlendingList(bctx, w * (1 - k));
    if (node[1])
      node[1]->buildBlendingList(bctx, w * k);
    return;
  }
  if (node[1])
    node[1]->buildBlendingList(bctx, w);
}
void AnimBlendCtrl_Blender::reset(IPureAnimStateHolder & /*st*/) {}
bool AnimBlendCtrl_Blender::validateNodeNotUsed(AnimationGraph &g, IAnimBlendNode *test_n)
{
  bool valid = true;
  for (int i = 0; i < 2; i++)
    if (!validate_node_not_used(g, this, node[i], i, test_n))
      valid = false;
  return valid;
}
void AnimBlendCtrl_Blender::collectUsedBlendNodes(AnimationGraph &g, eastl::hash_map<IAnimBlendNode *, bool> &node_map)
{
  for (int i = 0; i < 2; i++)
  {
    IAnimBlendNode *n = node[i];
    if (node_map.find(n) == node_map.end())
    {
      node_map.emplace(n, true);
      n->collectUsedBlendNodes(g, node_map);
    }
  }
}
bool AnimBlendCtrl_Blender::isAliasOf(IPureAnimStateHolder & /*st*/, IAnimBlendNode *n)
{
  if (n && (n == this || n == node[0] || n == node[1]))
    return true;
  return false;
}
real AnimBlendCtrl_Blender::getDuration(IPureAnimStateHolder & /*st*/) { return duration; }
real AnimBlendCtrl_Blender::tell(IPureAnimStateHolder &st)
{
  real t = st.getParam(paramId);
  if (t > duration)
    return 1;
  if (t < 0)
    return 0;
  return t / duration;
}

void AnimBlendCtrl_Blender::seekToSyncTime(IPureAnimStateHolder &st, real /*offset*/) { st.setParam(paramId, 0); }
void AnimBlendCtrl_Blender::pause(IPureAnimStateHolder &st)
{
  if (node[0])
    node[0]->pause(st);
  if (node[1])
    node[0]->pause(st);
}
void AnimBlendCtrl_Blender::resume(IPureAnimStateHolder &st, bool rewind)
{
  if (rewind)
    seekToSyncTime(st, 0);

  if (node[0])
    node[0]->resume(st, rewind);
  if (node[1])
    node[0]->resume(st, rewind);
}
bool AnimBlendCtrl_Blender::isInRange(IPureAnimStateHolder &st, int rangeId)
{
  if (node[0] && node[0]->isInRange(st, rangeId))
    return true;
  if (node[1] && node[1]->isInRange(st, rangeId))
    return true;
  return false;
}

void AnimBlendCtrl_Blender::setBlendNode(int n, IAnimBlendNode *_node)
{
  if (n < 0 || n >= 2)
    return;
  node[n] = _node;
}
void AnimBlendCtrl_Blender::setDuration(real dur) { duration = dur; }
void AnimBlendCtrl_Blender::setBlendTime(real dur) { blendTime = dur; }

//
// Binary indirect switch: checks condition and sets one of anims to fifo ctrl
//
AnimBlendCtrl_BinaryIndirectSwitch::AnimBlendCtrl_BinaryIndirectSwitch(AnimationGraph &graph, const char *param_name, int mask_and,
  int mask_eq) :
  maskAnd(mask_and), maskEq(mask_eq), morphTime{0.f, 0.f}
{
  paramId = graph.addParamId(param_name, IPureAnimStateHolder::PT_ScalarParamInt);
}
void AnimBlendCtrl_BinaryIndirectSwitch::destroy() {}
void AnimBlendCtrl_BinaryIndirectSwitch::buildBlendingList(BlendCtx &bctx, real w)
{
  G_ASSERT(fifo);
  BUILD_BLENDING_LIST_PROLOGUE(bctx, st);

  int v = st.getParamInt(paramId);
  if ((v & maskAnd) == maskEq)
    fifo->enqueueState(st, node[0], morphTime[0], morphTime[0] * 2);
  else
    fifo->enqueueState(st, node[1], morphTime[1], morphTime[1] * 2);
}
void AnimBlendCtrl_BinaryIndirectSwitch::reset(IPureAnimStateHolder &st)
{
  BlendCtx bctx(st);
  AnimBlendCtrl_BinaryIndirectSwitch::buildBlendingList(bctx, 0);
}
bool AnimBlendCtrl_BinaryIndirectSwitch::validateNodeNotUsed(AnimationGraph &g, IAnimBlendNode *test_n)
{
  bool valid = true;
  for (int i = 0; i < 2; i++)
    if (!validate_node_not_used(g, this, node[i], i, test_n))
      valid = false;
  return valid;
}

void AnimBlendCtrl_BinaryIndirectSwitch::collectUsedBlendNodes(AnimationGraph &g, eastl::hash_map<IAnimBlendNode *, bool> &node_map)
{
  for (int i = 0; i < 2; i++)
  {
    IAnimBlendNode *n = node[i];
    if (node_map.find(n) == node_map.end())
    {
      node_map.emplace(n, true);
      n->collectUsedBlendNodes(g, node_map);
    }
  }
}
void AnimBlendCtrl_BinaryIndirectSwitch::setBlendNodes(IAnimBlendNode *n1, real n1_mtime, IAnimBlendNode *n2, real n2_mtime)
{
  node[0] = n1;
  node[1] = n2;
  morphTime[0] = n1_mtime;
  morphTime[1] = n2_mtime;

  G_ASSERT(node[0]);
  G_ASSERT(node[1]);
}
void AnimBlendCtrl_BinaryIndirectSwitch::setFifoCtrl(AnimBlendCtrl_Fifo3 *n)
{
  fifo = n;
  G_ASSERT(fifo);
}


//
// Parametric switcher blend controller
//

AnimBlendCtrl_ParametricSwitcher::AnimBlendCtrl_ParametricSwitcher(AnimationGraph &graph, const char *param_name, const char *res_pn) :
  list(midmem), rewindBitmapParamsIds(midmem)
{
  paramId = graph.addParamId(param_name, IPureAnimStateHolder::PT_ScalarParam);
  residualParamId = (res_pn && *res_pn) ? graph.addParamId(res_pn, IPureAnimStateHolder::PT_ScalarParam) : -1;
  fifoParamId = graph.addInlinePtrParamId(String(128, "fifo_%p", this), sizeof(AnimFifo3Queue), IPureAnimStateHolder::PT_Fifo3);
  lastAnimParamId = graph.addParamId(String(128, ":lastAnim_%p", this), IPureAnimStateHolder::PT_ScalarParamInt);
  continuousAnimMode = true;
}

void AnimBlendCtrl_ParametricSwitcher::destroy() {}

void AnimBlendCtrl_ParametricSwitcher::buildBlendingList(BlendCtx &bctx, real w)
{
  BUILD_BLENDING_LIST_PROLOGUE(bctx, st);
  AnimFifo3Queue *fifo = (AnimFifo3Queue *)st.getInlinePtr(fifoParamId);

  float p = st.getParam(paramId);
  int newAnim = getAnimForRange(p);
  if (newAnim != -1 && newAnim + 1 != st.getParamInt(lastAnimParamId))
  {
    set_rewind_bit(newAnim, st, rewindBitmapParamsIds);
    list[newAnim].node->resume(st, !continuousAnimMode);
    fifo->enqueueItem(st.getParam(AnimationGraph::PID_GLOBAL_TIME), st, list[newAnim].node, morphTime, morphTime * 2);
    st.setParamInt(lastAnimParamId, newAnim + 1);
  }
  else if (newAnim == -1 && !continuousAnimMode)
    st.setParamInt(lastAnimParamId, 0);

  if (residualParamId >= 0)
  {
    float res = 0, ctime = st.getParam(AnimationGraph::PID_GLOBAL_TIME);
    for (int i = 0; i < list.size(); i++)
      if (float nw = fifo->getEnqueuedWeight(list[i].node, ctime))
        res = (p - list[i].baseVal) * nw;
    st.setParam(residualParamId, res * w);
  }

  fifo->buildBlendingList(bctx, st.getParam(AnimationGraph::PID_GLOBAL_TIME), w);
}

void AnimBlendCtrl_ParametricSwitcher::reset(IPureAnimStateHolder &st)
{
  AnimFifo3Queue *fifo = (AnimFifo3Queue *)st.getInlinePtr(fifoParamId);
  fifo->reset(false);
  st.setParamInt(lastAnimParamId, 0);
}

void AnimBlendCtrl_ParametricSwitcher::resume(IPureAnimStateHolder &st, bool rewind)
{
  AnimFifo3Queue *fifo = (AnimFifo3Queue *)st.getInlinePtr(fifoParamId);
  if (rewind)
  {
    fifo->reset(false);
    process_nodes_with_bitmap(list, st, rewindBitmapParamsIds, [&st](IAnimBlendNode *node, bool should_rewind) {
      if (should_rewind)
        node->resume(st, true);
    });
    st.setParamInt(lastAnimParamId, 0);
  }
}
bool AnimBlendCtrl_ParametricSwitcher::validateNodeNotUsed(AnimationGraph &g, IAnimBlendNode *test_n)
{
  bool valid = true;
  for (int i = 0; i < list.size(); i++)
    if (!validate_node_not_used(g, this, list[i].node, i, test_n))
      valid = false;
  return valid;
}
void AnimBlendCtrl_ParametricSwitcher::collectUsedBlendNodes(AnimationGraph &g, eastl::hash_map<IAnimBlendNode *, bool> &node_map)
{
  for (int i = 0; i < list.size(); i++)
  {
    IAnimBlendNode *n = list[i].node;
    if (node_map.find(n) == node_map.end())
    {
      node_map.emplace(n, true);
      n->collectUsedBlendNodes(g, node_map);
    }
  }
}
bool AnimBlendCtrl_ParametricSwitcher::isAliasOf(IPureAnimStateHolder &st, IAnimBlendNode *n)
{
  AnimFifo3Queue *fifo = (AnimFifo3Queue *)st.getInlinePtr(fifoParamId);
  switch (fifo->state)
  {
    case AnimFifo3Queue::ST_0: return false;
    case AnimFifo3Queue::ST_1: return n == fifo->node[0] || fifo->node[0]->isAliasOf(st, n);
    case AnimFifo3Queue::ST_1_2: return n == fifo->node[1] || fifo->node[1]->isAliasOf(st, n);
  }
  return false;
}

// creation-time routines
void AnimBlendCtrl_ParametricSwitcher::addBlendNode(IAnimBlendNode *n, real range0, real range1, real bval, AnimationGraph &graph,
  const char *ctrl_name)
{
  int l = append_items(list, 1);
  list[l].node = n;
  list[l].range[0] = range0;
  list[l].range[1] = range1;
  list[l].baseVal = bval;

  if (l % 32 == 0) // We just exceeded our 32 bit limit in the last bitmap
    add_rewind_bitmap(rewindBitmapParamsIds, graph, ctrl_name);
}

int AnimBlendCtrl_ParametricSwitcher::getAnimForRange(real range)
{
  for (int i = 0; i < list.size(); i++)
  {
    if ((range >= list[i].range[0]) && (range < list[i].range[1]))
      return i;
  }
  return -1;
}

void AnimBlendCtrl_SetMotionMatchingTag::buildBlendingList(BlendCtx &bctx, real w)
{
  if (w < 0.0001f || tagIdx < 0)
    return;
  uint32_t *activeNodesMask = static_cast<uint32_t *>(bctx.st->getInlinePtr(tagsMaskParamId));
  activeNodesMask[tagIdx >> 5] |= 1 << (tagIdx & 31);
}
