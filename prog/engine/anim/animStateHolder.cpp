// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if !_TARGET_PC_WIN
#undef _DEBUG_TAB_
#endif

#include <anim/dag_animStateHolder.h>
#include <util/dag_globDef.h>
#include <ioSys/dag_genIo.h>
#include <ioSys/dag_dataBlock.h>
#include <anim/dag_animBlend.h>
#include "animFifo.h"
#include <anim/dag_animIrq.h>

using namespace AnimV20;


//
// Common animation state holder implementation
//
AnimV20::AnimCommonStateHolder::AnimCommonStateHolder(AnimV20::AnimationGraph &g) :
  graph(g), paramNames(g.paramNames), paramTypes(g.paramTypes), valTimeInd(g.paramTimeInd), valFifo3Ind(g.paramFifo3Ind)
{
  init();
}

AnimV20::AnimCommonStateHolder::AnimCommonStateHolder(const AnimCommonStateHolder &st) :
  graph(st.graph),
  paramNames(st.paramNames),
  paramTypes(st.paramTypes),
  val(st.val),
  valTimeInd(st.valTimeInd),
  valFifo3Ind(st.valFifo3Ind)
{
  G_ASSERTF_AND_DO(val.size() == paramNames.nameCount(), init(), "val=%d != paramNames=%d", val.size(), paramNames.nameCount());
  for (int i = 0; i < paramTypes.size(); ++i)
    if (paramTypes[i] == PT_InlinePtrCTZ)
    {
      int w = getInlinePtrWords(paramTypes, i);
      mem_set_0(make_span(val).subspan(i, w));
      i += w - 1;
    }
}

static int getIntParam(const DataBlock &b, const char *nm, int def)
{
  int i = b.findParam(nm);
  if (i < 0)
    return def;
  if (b.getParamType(i) == b.TYPE_STRING)
    return AnimV20::getEnumValueByName(b.getStr(i));
  if (b.getParamType(i) == b.TYPE_INT)
    return b.getInt(i);
  return def;
}
static float getFloatParam(const DataBlock &b, const char *nm, float def)
{
  int i = b.findParam(nm);
  if (i < 0)
    return def;
  if (b.getParamType(i) == b.TYPE_STRING)
    return (float)AnimV20::getEnumValueByName(b.getStr(i));
  if (b.getParamType(i) == b.TYPE_REAL)
    return b.getReal(i);
  return def;
}

void AnimCommonStateHolder::init()
{
  clear_and_resize(val, paramNames.nameCount());
  mem_set_0(val);

  if (const DataBlock *init = graph.getInitState())
    iterate_names(paramNames, [&](int id, const char *name) {
      switch (paramTypes[id])
      {
        case PT_ScalarParam:
        case PT_TimeParam: val[id].scalar = getFloatParam(*init, name, val[id].scalar); break;
        case PT_ScalarParamInt: val[id].scalarInt = getIntParam(*init, name, val[id].scalarInt); break;
      }
    });
}
void AnimCommonStateHolder::term() { clear_and_shrink(val); }

void AnimCommonStateHolder::reset()
{
  term();
  init();
}

void AnimCommonStateHolder::setParamFlags(int id, int flags, int mask)
{
  G_ASSERTF_RETURN(id >= 0 && id < val.size(), , "id=%d val.size()=%d", id, val.size());
  G_ASSERTF(isBasicType(paramTypes[id]), "paramTypes[%d]=%d", id, paramTypes[id]);
  val[id].flags &= ~mask;
  val[id].flags |= flags & mask;
}
int AnimCommonStateHolder::getParamFlags(int id, int mask) const
{
  G_ASSERTF_RETURN(id >= 0 && id < val.size(), 0, "id=%d val.size()=%d", id, val.size());
  G_ASSERTF(isBasicType(paramTypes[id]), "paramTypes[%d]=%d", id, paramTypes[id]);
  return val[id].flags & mask;
}


real *AnimCommonStateHolder::getParamScalarPtr(int id)
{
  G_ASSERTF_RETURN(id >= 0 && id < val.size(), nullptr, "id=%d val.size()=%d", id, val.size());
  G_ASSERT(paramTypes[id] == PT_ScalarParam || paramTypes[id] == PT_TimeParam);
  return &val[id].scalar;
}


int AnimCommonStateHolder::getTimeScaleParamId(int id) const
{
  G_ASSERTF_RETURN(id >= 0 && id < val.size(), -1, "id=%d val.size()=%d", id, val.size());
  G_ASSERT(paramTypes[id] == PT_TimeParam);
  return val[id].timeScaleId;
}


void AnimCommonStateHolder::setTimeScaleParamId(int id, int tspid)
{
  G_ASSERTF_RETURN(id >= 0 && id < val.size(), , "id=%d val.size()=%d", id, val.size());
  G_ASSERT(paramTypes[id] == PT_TimeParam);
  val[id].timeScaleId = tspid;
}


int AnimCommonStateHolder::getParamInt(int id) const
{
  G_ASSERTF_RETURN(id >= 0 && id < val.size(), 0, "id=%d val.size()=%d", id, val.size());
  G_ASSERT(paramTypes[id] == PT_ScalarParamInt);
  return val[id].scalarInt;
}
void AnimCommonStateHolder::setParamInt(int id, int value)
{
  G_ASSERTF_RETURN(id >= 0 && id < val.size(), , "id=%d val.size()=%d", id, val.size());
  G_ASSERT(paramTypes[id] == PT_ScalarParamInt);
  if (val[id].scalarInt != value)
    val[id].flags |= PF_Changed;
  val[id].scalarInt = value;
}

float AnimCommonStateHolder::getParamEffTimeScale(int id) const
{
  G_ASSERTF_RETURN(id >= 0 && id < val.size(), 1.0f, "id=%d val.size()=%d", id, val.size());
  G_ASSERT(paramTypes[id] == PT_TimeParam);
  int tspid = val[id].timeScaleId;
  if (tspid > 0)
  {
    float s = val[tspid].scalar;
    while ((tspid = val[tspid].timeScaleId) > 0)
      s *= val[tspid].scalar;
    return s;
  }
  return 1.0f;
}
void AnimCommonStateHolder::advance(real dt)
{
  graph.postBlendCtrlAdvance(*this, dt);
  ParamState *__restrict val_ptr = val.data();
  const float global_time = val_ptr[AnimationGraph::PID_GLOBAL_TIME].scalar;
  val_ptr[AnimationGraph::PID_GLOBAL_LAST_DT].scalar = dt;

  for (int idx : valTimeInd)
  {
    ParamState &cur_val = val_ptr[idx];
    if (GCC_LIKELY((cur_val.flags & PF_Paused) == 0))
    {
      int tspid = cur_val.timeScaleId;
      if (tspid > 0)
      {
        float s = val_ptr[tspid].scalar;
        while ((tspid = val_ptr[tspid].timeScaleId) > 0)
          s *= val_ptr[tspid].scalar;
        cur_val.scalar += s * dt;
      }
      else if (paramTypes[idx] == PT_TimeParam)
        cur_val.scalar += dt;
    }
  }

  for (int idx : valFifo3Ind)
  {
    ParamState &cur_val = val_ptr[idx];
    ((AnimFifo3Queue *)&cur_val.scalarInt)->update(*this, global_time, dt);
  }
}


//
// Store/restore state of anim state holder via snapshot
//
void AnimCommonStateHolder::saveState(IGenSave &cb) const
{
  for (int i = 0; i < val.size(); ++i)
  {
    const ParamState &s = val[i];

    switch (paramTypes[i])
    {
      case PT_ScalarParam:
      case PT_ScalarParamInt:
      case PT_TimeParam:
        cb.writeIntP<2>(s.flags);
        cb.writeIntP<2>(s.timeScaleId);
        cb.write(&s.scalar, sizeof(s.scalar));
        break;

      case PT_Fifo3: ((AnimFifo3Queue *)&val[i].scalarInt)->save(cb, graph); break;

      default: break;
    }
  }
}


void AnimCommonStateHolder::loadState(IGenLoad &cb)
{
  for (int i = 0; i < val.size(); ++i)
  {
    ParamState &s = val[i];

    switch (paramTypes[i])
    {
      case PT_ScalarParam:
      case PT_ScalarParamInt:
      case PT_TimeParam:
        s.flags = cb.readIntP<2>();
        s.timeScaleId = cb.readIntP<2>();
        cb.read(&s.scalar, sizeof(s.scalar));
        break;

      case PT_Fifo3: ((AnimFifo3Queue *)&val[i].scalarInt)->load(cb, graph); break;

      default: break;
    }
  }
}
