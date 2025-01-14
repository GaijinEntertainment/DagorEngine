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

namespace acl
{
class compressed_tracks;
}


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

typedef acl::compressed_tracks AnimChan;

//
// Generic channels for distinct animation keys
//
struct AnimDataChan
{
  PatchablePtr<AnimChan> animTracks;
  PatchablePtr<dag::Index16> nodeTrack;
  unsigned nodeNum;
  unsigned channelType;
  PatchablePtr<real> nodeWt;
  PatchablePtr<PatchablePtr<char>> nodeName;

  AnimDataChan() {}

  void patchData(void *base);

  dag::Index16 getNodeId(const char *node_name) const
  {
    for (unsigned id = 0; id < nodeNum; id++)
      if (dd_stricmp(node_name, nodeName[id]) == 0)
        return dag::Index16(id);
    return dag::Index16();
  }

  dag::Index16 getTrackId(dag::Index16 node_id) const
  {
    if (node_id.index() >= nodeNum)
      return dag::Index16();
    return nodeTrack[node_id.index()];
  }

  dag::Index16 getTrackId(const char *node_name) const { return getTrackId(getNodeId(node_name)); }

  // has one or more keys
  bool hasKeys() const;

  // has more than one key
  bool hasAnimation() const;

  unsigned getNumKeys() const;

  int keyTimeFirst() const { return 0; }
  int keyTimeLast() const;
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
  const AnimChan *anim = nullptr;
  dag::Index16 trackId;

  inline bool valid() const { return anim && trackId; }

  inline bool hasAnimation() const;

  int keyTimeFirst() const { return 0; }
  int keyTimeLast() const;
  float getDuration() const; // in seconds
};

} // end of namespace AnimV20
