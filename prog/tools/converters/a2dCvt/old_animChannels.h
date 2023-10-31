/*
 * Dagor Engine 3
 * Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
 *
 * (for conditions of use see prog/license.txt)
 */

#ifndef _DAGOR3_PUBLIC_ANIM_DAG_ANIMCHANNELS_H_
#define _DAGOR3_PUBLIC_ANIM_DAG_ANIMCHANNELS_H_
#pragma once


#include <math/dag_Quat.h>
#include <generic/dag_DObject.h>
#include <generic/dag_smallTab.h>
#include <osApiWrappers/dag_localConv.h>

// forward declarations for external classes and structures
class IGenLoad;
class DataBlock;
class IMemAlloc;


// Constants
namespace AnimV20Math
{
enum
{
  TIME_TicksPerSec = 4800,
  TIME_NegInf = 0x80000000,
  TIME_PosInf = 0x7FFFFFFF,
};
};


namespace AnimV20
{

enum
{
  DATATYPE_POINT3 = 0x577084E5u,
  DATATYPE_QUAT = 0x39056464u,
  DATATYPE_REAL = 0xEE0BD66Au,
};

enum
{
  CHTYPE_POSITION = 0xE66CF0CCu,
  CHTYPE_SCALE = 0x48D43D77u,
  CHTYPE_ORIGIN_LIN_VEL = 0x31DE5F48u,
  CHTYPE_ORIGIN_ANG_VEL = 0x09FD7093u,
  CHTYPE_ROTATION = 0x8D490AE4u,
};

const char origin_lin_vel_node_name[] = "origin_lin_vel_node_name";
const char origin_ang_vel_node_name[] = "origin_ang_vel_node_name";

// Key data
struct AnimKeyReal
{
  real p, k1, k2, k3;
};

struct AnimKeyPoint3
{
  Point3 p, k1, k2, k3;
};

struct AnimKeyQuat
{
  real pw, bw;
  real sinpw, sinbw;
  Quat p, b0, b1;
};

// Animations for channels as sequence of keys
struct AnimChanReal
{
  AnimKeyReal *key;
  int *keyTime;
  int keyNum;
};

struct AnimChanPoint3
{
  AnimKeyPoint3 *key;
  int *keyTime;
  int keyNum;
};

struct AnimChanQuat
{
  AnimKeyQuat *key;
  int *keyTime;
  int keyNum;
};

//
// Generic channels for distinct animation keys
//
template <class T>
struct AnimDataChan
{
  T *nodeAnim;
  int nodeNum;
  char **nodeName;
  real *nodeWt;
  unsigned channelType;

  AnimDataChan() : nodeAnim(NULL), nodeNum(0), nodeName(NULL), channelType(0) {}

  void alloc(int node_num, IMemAlloc *ma = midmem)
  {
    nodeAnim = new (ma) T[node_num];
    nodeName = new (ma) char *[node_num];
    nodeWt = new (ma) real[node_num];
    nodeNum = node_num;
  }

  void clear()
  {
    if (nodeAnim)
    {
      memset(nodeAnim, 0, sizeof(T) * nodeNum);
      delete nodeAnim;
      nodeAnim = NULL;
    }
    if (nodeName)
    {
      memset(nodeName, 0, sizeof(char *) * nodeNum);
      delete nodeName;
      nodeName = NULL;
    }
    if (nodeWt)
    {
      memset(nodeWt, 0, sizeof(real) * nodeNum);
      delete nodeWt;
      nodeWt = NULL;
    }
    nodeNum = 0;
  }

  int getAnimId(const char *node_name)
  {
    int nodeId;
    for (nodeId = nodeNum; --nodeId >= 0;)
      if (!dd_stricmp(node_name, nodeName[nodeId]))
        break;
    return nodeId;
  }
};


// old data
typedef AnimDataChan<AnimChanPoint3> AnimDataPos;
typedef AnimDataChan<AnimChanPoint3> AnimDataScale;
typedef AnimDataChan<AnimChanQuat> AnimDataRotate;


// Note track labels
struct AnimKeyLabel
{
  const char *name;
  int time;
};
struct AnimNoteTrack
{
  AnimKeyLabel *pool;
  int num;

  AnimNoteTrack() : pool(NULL), num(0) {}

  void alloc(int sz, IMemAlloc *ma = midmem)
  {
    pool = new (ma) AnimKeyLabel[sz];
    num = sz;
  }
  void clear()
  {
    if (pool)
    {
      memset(pool, 0, sizeof(AnimKeyLabel) * num);
      delete pool;
      pool = NULL;
      num = 0;
    }
  }
  int getTime(const char *name, bool fatal_err = true);
};


//
// Master animation holder (can be shared)
//
class AnimData : public DObject
{
public:
  SmallTab<AnimDataChan<AnimChanPoint3>, MidmemAlloc> animDataChannelsPoint3;
  SmallTab<AnimDataChan<AnimChanQuat>, MidmemAlloc> animDataChannelsQuat;
  SmallTab<AnimDataChan<AnimChanReal>, MidmemAlloc> animDataChannelsReal;

  AnimNoteTrack noteTrack;

  // old data
  AnimDataPos pos;
  AnimDataScale scl;
  AnimDataRotate rot;
  AnimChanPoint3 originLinVel;
  AnimChanPoint3 originAngVel;

private:
  char *pool;

public:
  AnimData();

  bool load(IGenLoad &cb, IMemAlloc *ma = midmem);

  AnimChanPoint3 *getPoint3Anim(const unsigned channel_type, const char *node_name);

protected:
  virtual const char *class_name() const { return "AnimData"; }
  ~AnimData();
  AnimDataChan<AnimChanPoint3> *getPoint3AnimDataChan(const unsigned channel_type);
  AnimDataChan<AnimChanQuat> *getQuatAnimDataChan(const unsigned channel_type);
  AnimDataChan<AnimChanReal> *getRealAnimDataChan(const unsigned channel_type);
  struct AnimDataHeader;

  struct AnimDataHeaderLessV150;
  struct TmpNodeBuf;
  bool loadLessV150(IGenLoad &cb, class IMemAlloc *ma, AnimDataHeader &hdr);
  template <class ANIM_CHAN, class ANIM_KEY>
  void setAnimDataChanParams(const TmpNodeBuf *node_buf, const AnimDataChan<ANIM_CHAN> *animDataChan, ANIM_KEY *channel_pool,
    char *name_pool, int *time_pool, int &pool_n, int &time_pool_n);

  struct AnimDataHeaderV200;
  struct DataTypeHeader;
  bool loadV200(IGenLoad &cb, class IMemAlloc *ma, AnimDataHeader &hdr);
  template <class ANIM_CHANNEL, class ANIM_KEY>
  void readAnimDataChanParams(SmallTab<AnimDataChan<ANIM_CHANNEL>, MidmemAlloc> &channels, const unsigned channel_num,
    const void *key_pool, const char *name_pool, const int *time_pool, IGenLoad &cb, class IMemAlloc *ma);

  // old data
  void setOldData(IMemAlloc *ma);
  void clearOldData();
};
} // end of namespace AnimV20

#endif
