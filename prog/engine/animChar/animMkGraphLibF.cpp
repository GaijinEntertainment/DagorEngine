// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "animPdlMgr.h"
#include "animGraphFactory.h"
#include "animITranslator.h"

#include <anim/dag_animBlendCtrl.h>
#include <generic/dag_initOnDemand.h>
#include <perfMon/dag_cpuFreq.h>
#include <debug/dag_debug.h>


using namespace AnimV20;

class AnimFuncTranslatorCpp : public ITranslator
{
public:
  virtual int addParamId(AnimationGraph *anim, const char *name, int type) { return anim->addParamId(name, type); }
  virtual int addInlinePtrParamId(AnimationGraph *anim, const char *name, int size_bytes,
    int type = IPureAnimStateHolder::PT_InlinePtr)
  {
    return anim->addInlinePtrParamId(name, size_bytes, type);
  }
  virtual int getNamedRangeId(AnimationGraph *anim, const char *name) { return anim->getNamedRangeId(name); }
  virtual IAnimBlendNode *getBlendNodePtr(AnimationGraph *anim, const char *node_name) { return anim->getBlendNodePtr(node_name); }
  virtual void enqueueState(AnimBlendCtrl_Fifo3 *ctrl, IPureAnimStateHolder &st, IAnimBlendNode *node, float overlap_time,
    float max_lag)
  {
    // DEBUG_CTX("%.2f: enqueueState ( %p, %p )", get_time_msec()/1000.0, ctrl, node);
    ctrl->enqueueState(st, node, overlap_time, max_lag);
  }
  virtual bool checkFifo3(IAnimBlendNode *node)
  {
    if (!node)
      return false;
    return node->isSubOf(AnimBlendCtrl_Fifo3CID);
  }
  virtual bool isAliasOf(IAnimBlendNode *n_this, IPureAnimStateHolder &st, IAnimBlendNode *n) { return n_this->isAliasOf(st, n); }
  virtual void resetQueue(AnimBlendCtrl_Fifo3 *fifo, IPureAnimStateHolder &st, bool leave_cur_state)
  {
    fifo->resetQueue(st, leave_cur_state);
  }
  virtual float getDuration(IAnimBlendNode *n, IPureAnimStateHolder &st) { return n->getDuration(st); }
  virtual void resume(IAnimBlendNode *n, IPureAnimStateHolder &st, bool rewind) { n->resume(st, rewind); }
  virtual float tell(IAnimBlendNode *n, IPureAnimStateHolder &st) { return n->tell(st); }
};

static InitOnDemand<AnimFuncTranslatorCpp> anim_func_translator;
AnimV20::ITranslator *anim_iface_trans;

void AnimCharV20::registerGraphCppFactory()
{
  makeGraphFromRes = &::make_graph_by_res_name;
  anim_func_translator.demandInit();
  anim_iface_trans = anim_func_translator;
}
