//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <debug/dag_assert.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_index16.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_tab.h>
#include <math/dag_check_nan.h>

class IGenSave;
class IGenLoad;

namespace AnimV20
{
class AnimationGraph;
//
// Simple animation state holder implementation
//
class AnimCommonStateHolder
{
public:
  enum
  {              // parameter types
    PT_Reserved, // holds non-acessible data; these data may be accessible through inlinePtr

    PT_ScalarParam,    // 32 bit float
    PT_ScalarParamInt, // 32 bit integer
    PT_TimeParam,      // 32 bit float: time, advanced automatically by advance(dt)

    PT_InlinePtr,    // start of sequence of 32 bit words (PT_InlinePtr, PT_Reserved, PT_Reserved, ...)
    PT_InlinePtrCTZ, // inline ptr that requires clear-to-zero after cloning
    PT_Fifo3,        // inline ptr holding AnimFifo3Queue struct
    PT_Effector,     // inline ptr holding 128bit EffectorVar struct (Point3 effector pos + effector flags)

    PF_Paused = 0x00000001,  // for time: disables advance; effectivly pauses time
    PF_Changed = 0x00000002, // for scalar: set in setParam/setParamInt when new value differs from old one
  };
  struct EffectorVar
  {
    enum
    {
      T_looseEnd,
      T_useEffector,
      T_useGeomNode
    };
    static constexpr int F_localCoordEff = 0x1;
    float x = 0, y = 0, z = 0;
    unsigned char type = T_useGeomNode;
    unsigned char flags = 0;
    dag::Index16 nodeId;

    void set(float _x, float _y, float _z, bool world)
    {
      x = _x;
      y = _y;
      z = _z;
      type = T_useEffector;
      if (world)
        flags &= ~F_localCoordEff;
      else
        flags |= F_localCoordEff;
    }
    void setLooseEnd() { type = T_looseEnd; }
    bool isLocalEff() const { return flags & F_localCoordEff; }
  };

public:
  AnimCommonStateHolder(AnimationGraph &graph);
  AnimCommonStateHolder(const AnimCommonStateHolder &st);
  ~AnimCommonStateHolder() { term(); }

  void reset();

  bool getParamIdValid(int id) const { return id >= 0 && id < val.size(); }

  void setParamFlags(int id, int flags, int mask);
  int getParamFlags(int id, int mask) const;

  float getParam(int id) const;
  void setParam(int id, float val);

  int getTimeScaleParamId(int id) const;
  void setTimeScaleParamId(int id, int time_scale_param_id);

  int getParamInt(int id) const;
  void setParamInt(int id, int val);

  void *getInlinePtr(int id)
  {
    G_ASSERT(id >= 0 && id < val.size());
    G_ASSERTF(isInlinePtrType(paramTypes[id]), "paramTypes[%d]=%d", id, paramTypes[id]);
    return &val[id].scalarInt;
  }
  const void *getInlinePtr(int id) const { return const_cast<AnimCommonStateHolder *>(this)->getInlinePtr(id); }
  unsigned getInlinePtrMaxSz(int id) { return getInlinePtrWords(paramTypes, id) * elem_size(val); }

  float *getParamScalarPtr(int id);
  const float *getParamScalarPtr(int id) const { return const_cast<AnimCommonStateHolder *>(this)->getParamScalarPtr(id); }
  float getParamEffTimeScale(int id) const;

  const EffectorVar &getParamEffector(int id) const { return *(const EffectorVar *)getInlinePtr(id); }
  EffectorVar &paramEffector(int id) { return *(EffectorVar *)getInlinePtr(id); }

  void advance(float dt);

  int getSize() const { return data_size(val); }

  // store/restore routines
  void saveState(IGenSave &cb) const;
  void loadState(IGenLoad &cb);

  AnimationGraph &getGraph() { return graph; }

  static bool isInlinePtrType(int t) { return t == PT_InlinePtr || t == PT_InlinePtrCTZ || t == PT_Fifo3 || t == PT_Effector; }
  static bool isBasicType(int t) { return t == PT_ScalarParam || t == PT_ScalarParamInt || t == PT_TimeParam; }
  static unsigned getInlinePtrWords(dag::ConstSpan<uint8_t> param_types, int id)
  {
    G_ASSERT_RETURN(id >= 0 && id < param_types.size(), 0);
    G_ASSERTF_RETURN(isInlinePtrType(param_types[id]), 0, "paramTypes[%d]=%d", id, param_types[id]);
    int numw = 1;
    while (id + numw < param_types.size() && param_types[id + numw] == PT_Reserved)
      numw++;
    return numw;
  }

private:
  struct ParamState
  {
    union
    {
      float scalar;
      int scalarInt;
    };
    int flags : 16;
    int timeScaleId : 16;
  };

  AnimationGraph &graph;
  const FastNameMap &paramNames;
  SmallTab<ParamState, MidmemAlloc> val;
  const Tab<int16_t> &valTimeInd;
  const Tab<int16_t> &valFifo3Ind;
  const Tab<uint8_t> &paramTypes;

  void init();
  void term();

  friend class AnimationGraph;
};


inline float AnimCommonStateHolder::getParam(int id) const
{
  G_ASSERT((unsigned)id < val.size());
  G_ASSERT(paramTypes[id] == PT_ScalarParam || paramTypes[id] == PT_TimeParam);
  return val[id].scalar;
}

inline void AnimCommonStateHolder::setParam(int id, float value)
{
  G_ASSERTF(id < val.size() && check_finite(value), "%d/%d %g", id, val.size(), value);
  G_ASSERTF(paramTypes[id] == PT_ScalarParam || paramTypes[id] == PT_TimeParam, "Unexpected (%d) non float type on param <%s>(%d)",
    paramTypes[id], paramNames.getName(id), id);
  if (val[id].scalar != value)
    val[id].flags |= PF_Changed;
  val[id].scalar = value;
}

} // end of namespace AnimV20
