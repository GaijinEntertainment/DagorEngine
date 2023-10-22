// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef _GAIJIN_ANIM_ITRANSLATOR_H
#define _GAIJIN_ANIM_ITRANSLATOR_H
#pragma once

#include <anim/dag_animDecl.h>

namespace AnimV20
{

//
// interface to call internal AnimV20 methods from external modules
//
class ITranslator
{
public:
  virtual int addParamId(AnimationGraph *anim, const char *name, int type) = 0;
  virtual int addInlinePtrParamId(AnimationGraph *anim, const char *name, int size_bytes,
    int type = IPureAnimStateHolder::PT_InlinePtr) = 0;
  virtual int getNamedRangeId(AnimationGraph *anim, const char *name) = 0;
  virtual IAnimBlendNode *getBlendNodePtr(AnimationGraph *anim, const char *node_name) = 0;
  virtual void enqueueState(AnimBlendCtrl_Fifo3 *ctrl, IPureAnimStateHolder &st, IAnimBlendNode *node, float overlap_time,
    float max_lag) = 0;
  virtual bool checkFifo3(IAnimBlendNode *node) = 0;
  virtual bool isAliasOf(IAnimBlendNode *n_this, IPureAnimStateHolder &st, IAnimBlendNode *n) = 0;
  virtual void resetQueue(AnimBlendCtrl_Fifo3 *fifo, IPureAnimStateHolder &st, bool leave_cur_state) = 0;
  virtual float getDuration(IAnimBlendNode *n, IPureAnimStateHolder &st) = 0;
  virtual void resume(IAnimBlendNode *n, IPureAnimStateHolder &st, bool rewind) = 0;
  virtual float tell(IAnimBlendNode *n, IPureAnimStateHolder &st) = 0;
};

} // end of namespace AnimV20

#endif
