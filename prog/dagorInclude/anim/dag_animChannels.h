//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_mathBase.h>
#include <vecmath/dag_vecMathDecl.h>
#include <generic/dag_DObject.h>
#include <generic/dag_patchTab.h>
#include <generic/dag_smallTab.h>
#include <osApiWrappers/dag_localConv.h>
#include <util/dag_stdint.h>
#include <util/dag_oaHashNameMap.h>
#include <anim/dag_animKeys.h>

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

// Animations for channels as sequence of keys
template <class KEY>
struct AnimChan
{
  PatchablePtr<KEY> key;
  PatchablePtr<uint16_t> keyTime16;
  int keyNum;
  int _resv;

  int keyTimeFirst() const { return unsigned(keyTime16[0]) << TIME_SubdivExp; }
  int keyTimeLast() const { return unsigned(keyTime16[keyNum - 1]) << TIME_SubdivExp; }
  int keyTime(int idx) const { return unsigned(keyTime16[idx]) << TIME_SubdivExp; }

  __forceinline KEY *findKey(int t32, float *out_t) const
  {
    if (keyNum == 1 || t32 <= keyTimeFirst())
    {
      *out_t = 0;
      return &key[0];
    }
    if (t32 >= keyTimeLast())
    {
      *out_t = 0;
      return &key[keyNum - 1];
    }

    int t = t32 >> TIME_SubdivExp, a = 0, b = keyNum - 1;
    while (b - a > 1)
    {
      int c = (a + b) / 2;
      if (keyTime16[c] == t)
      {
        if (keyTime(c) == t32)
        {
          *out_t = 0;
          return &key[c];
        }
        *out_t = float(t32 - (t << TIME_SubdivExp)) / float((keyTime16[c + 1] - t) << TIME_SubdivExp);
        return &key[c];
      }
      else if (keyTime16[c] < t)
        a = c;
      else
        b = c;
    }
    *out_t = float(t32 - keyTime(a)) / float(keyTime(b) - keyTime(a));
    return &key[a];
  }

  __forceinline KEY *findKeyEx(int t32, float *out_t, int &dkeys) const
  {
    if (keyNum == 1 || t32 <= keyTimeFirst())
    {
      dkeys = 0;
      *out_t = 0;
      return &key[0];
    }
    if (t32 >= keyTimeLast())
    {
      dkeys = 0;
      *out_t = 0;
      return &key[keyNum - 1];
    }

    int t = t32 >> TIME_SubdivExp, a = 0, b = keyNum - 1;
    while (b - a > 1)
    {
      int c = (a + b) / 2;
      if (keyTime16[c] == t)
      {
        if (keyTime(c) == t32)
        {
          dkeys = (c < keyNum - 1) ? (keyTime16[c + 1] - keyTime16[c]) : 0;
          *out_t = 0;
          return &key[c];
        }
        dkeys = keyTime16[c + 1] - keyTime16[c];
        *out_t = float(t32 - (t << TIME_SubdivExp)) / float((keyTime16[c + 1] - t) << TIME_SubdivExp);
        return &key[c];
      }
      else if (keyTime16[c] < t)
        a = c;
      else
        b = c;
    }
    dkeys = keyTime16[b] - keyTime16[a];
    *out_t = float(t32 - keyTime(a)) / float(keyTime(b) - keyTime(a));
    return &key[a];
  }
};

typedef AnimChan<AnimKeyPoint3> AnimChanPoint3;
typedef AnimChan<AnimKeyQuat> AnimChanQuat;
typedef AnimChan<AnimKeyReal> AnimChanReal;

// Note track labels
struct AnimKeyLabel
{
  PatchablePtr<const char> name;
  int time;
  int _resv;

  void patchData(void *base) { name.patch(base); }
};

//
// Generic channels for distinct animation keys
//
template <class T>
struct AnimDataChan
{
  PatchablePtr<T> nodeAnim;
  unsigned nodeNum;
  unsigned channelType;
  PatchablePtr<real> nodeWt;
  PatchablePtr<PatchablePtr<char>> nodeName;

  AnimDataChan() {}

  void patchData(void *base)
  {
    nodeAnim.patch(base);
    nodeName.patch(base);
    nodeWt.patch(base);
    for (int i = 0; i < nodeNum; i++)
    {
      nodeName[i].patch(base);
      nodeAnim[i].key.patch(base);
      nodeAnim[i].keyTime16.patch(base);
    }
  }

  int getNodeId(const char *node_name)
  {
    for (int id = 0; id < nodeNum; id++)
      if (dd_stricmp(node_name, nodeName[id]) == 0)
        return id;
    return -1;
  }
};

//
// Master animation holder (can be shared)
//
class AnimData : public DObject
{
public:
  struct DumpData
  {
    PatchableTab<AnimDataChan<AnimChanPoint3>> chanPoint3;
    PatchableTab<AnimDataChan<AnimChanQuat>> chanQuat;
    PatchableTab<AnimDataChan<AnimChanReal>> chanReal;
    PatchableTab<AnimKeyLabel> noteTrack;

    void patchData(void *base);
    AnimDataChan<AnimChanPoint3> *getChanPoint3(unsigned channel_type);
    AnimDataChan<AnimChanQuat> *getChanQuat(unsigned channel_type);
    AnimDataChan<AnimChanReal> *getChanReal(unsigned channel_type);
  } dumpData;

  struct Anim
  {
    AnimDataChan<AnimChanPoint3> pos, scl;
    AnimDataChan<AnimChanQuat> rot;
    AnimChanPoint3 originLinVel, originAngVel;
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
  AnimData(AnimData *src_anim, const NameMap &node_list, IMemAlloc *ma);

  bool load(IGenLoad &cb, IMemAlloc *ma = midmem);

  // creates alias of other AnimData and uses only nodes specified

  const AnimChanPoint3 *getPoint3Anim(unsigned channel_type, const char *node_name);
  const AnimChanQuat *getQuatAnim(unsigned channel_type, const char *node_name);
  const AnimChanReal *getRealAnim(unsigned channel_type, const char *node_name);
  int getLabelTime(const char *name, bool fatal_err = true);
  bool isAdditive() const { return animAdditive; }

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
} // end of namespace AnimV20
