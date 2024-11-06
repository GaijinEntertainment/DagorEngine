//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_mathBase.h>
#include <vecmath/dag_vecMathDecl.h>
#include <generic/dag_DObject.h>
#include <generic/dag_patchTab.h>
#include <generic/dag_smallTab.h>
#include <osApiWrappers/dag_localConv.h>
#include <util/dag_index16.h>
#include <util/dag_oaHashNameMap.h>

// forward declarations for external classes and structures
class IGenLoad;
class DataBlock;
class IMemAlloc;


namespace AnimV20
{
// Constants
enum
{
  TIME_SubdivExp = 8,
  TIME_TicksPerSec = 60 << TIME_SubdivExp,
  TIME_PosInf = 0xFFFFFFFF,
};

enum
{
  DATATYPE_INVALID = 0,

  DATATYPE_POINT3X,
  DATATYPE_QUAT,
  DATATYPE_REAL,
};

enum
{
  CHTYPE_INVALID = 0,

  CHTYPE_POSITION,
  CHTYPE_SCALE,
  CHTYPE_ORIGIN_LINVEL,
  CHTYPE_ORIGIN_ANGVEL,
  CHTYPE_ROTATION,
};

// Note track labels
struct AnimKeyLabel
{
  PatchablePtr<const char> name;
  int time;
  int _resv;

  void patchData(void *base) { name.patch(base); }
};

struct AnimChan
{};

//
// Generic channels for distinct animation keys
//
struct AnimDataChan
{
  PatchablePtr<AnimChan> nodeAnim;
  unsigned nodeNum;
  unsigned channelType;
  PatchablePtr<real> nodeWt;
  PatchablePtr<PatchablePtr<char>> nodeName;

  AnimDataChan() {}

  void patchData(void *base);

  dag::Index16 getNodeId(const char *node_name)
  {
    for (unsigned id = 0; id < nodeNum; id++)
      if (dd_stricmp(node_name, nodeName[id]) == 0)
        return dag::Index16(id);
    return dag::Index16();
  }

  // has one or more keys
  bool hasKeys(dag::Index16 node_id) const;

  // has more than one key
  bool hasAnimation(dag::Index16 node_id) const;

  unsigned getNumKeys(dag::Index16 node_id) const;

  int keyTimeFirst(dag::Index16 node_id) const;
  int keyTimeLast(dag::Index16 node_id) const;

  int keyTimeFirst() const
  {
    if (nodeNum == 0)
      return INT_MAX;
    int t = keyTimeFirst(dag::Index16(0));
    for (unsigned i = 1; i < nodeNum; i++)
      t = min(t, keyTimeFirst(dag::Index16(i)));
    return t;
  }

  int keyTimeLast() const
  {
    if (nodeNum == 0)
      return INT_MIN;
    int t = keyTimeLast(dag::Index16(0));
    for (unsigned i = 1; i < nodeNum; i++)
      t = max(t, keyTimeLast(dag::Index16(i)));
    return t;
  }
};

struct PrsAnimNodeRef;

//
// Master animation holder (can be shared)
//
class AnimData : public DObject
{
public:
  struct DumpData
  {
    PatchableTab<AnimDataChan> chanPoint3;
    PatchableTab<AnimDataChan> chanQuat;
    PatchableTab<AnimDataChan> chanReal;
    PatchableTab<AnimKeyLabel> noteTrack;

    void patchData(void *base);
    AnimDataChan *getChanPoint3(unsigned channel_type);
    AnimDataChan *getChanQuat(unsigned channel_type);
    AnimDataChan *getChanReal(unsigned channel_type);
  } dumpData;

  struct Anim
  {
    AnimDataChan pos, scl;
    AnimDataChan rot;
    AnimDataChan originLinVel, originAngVel;
    void setup(DumpData &d);
  } anim;
  int resId = -1;

public:
  AnimData(int res_id = -1) : resId(res_id), dump(NULL), extraData(NULL)
  {
    memset(&anim, 0, sizeof(anim));
    memset(&dumpData, 0, sizeof(dumpData));
    animAdditive = false;
  }
  // creates alias of other AnimData and uses only nodes specified
  AnimData(AnimData *src_anim, const NameMap &node_list, IMemAlloc *ma);

  bool load(IGenLoad &cb, IMemAlloc *ma = midmem);

  PrsAnimNodeRef getPrsAnim(const char *node_name);

  int getLabelTime(const char *name, bool fatal_err = true);
  bool isAdditive() const { return animAdditive; }
  AnimData *getSourceAnimData() const { return src; }

protected:
  virtual const char *class_name() const { return "AnimData"; }
  ~AnimData();

  struct AnimDataHeader;
  struct DataTypeHeader;

  Ptr<AnimData> src;

private:
  void *dump;
  void *extraData;
  bool animAdditive;
};

struct PrsAnimNodeRef
{
  const AnimData::Anim *anim = nullptr;
  dag::Index16 posId;
  dag::Index16 rotId;
  dag::Index16 sclId;

  inline bool valid() const { return anim && (posId || rotId || sclId); }
  inline bool allValid() const { return anim && posId && rotId && sclId; }

  inline bool hasAnimation() const
  {
    return anim && (anim->pos.hasAnimation(posId) || anim->rot.hasAnimation(rotId) || anim->scl.hasAnimation(sclId));
  }

  int keyTimeFirst() const;
  int keyTimeLast() const;
  float getDuration() const; // in seconds
};

} // end of namespace AnimV20
