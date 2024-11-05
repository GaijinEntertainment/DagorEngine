// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_globDef.h>
#include "old_animChannels.h"
#include <ioSys/dag_fileIo.h>
#include <generic/dag_tab.h>
#include <debug/dag_debug.h>
using namespace AnimV20;


// common header
struct AnimData::AnimDataHeader
{
  unsigned label;    // MAKE4C('A','N','I','M')
  unsigned ver;      // 0x200
  unsigned hdrSize;  // =sizeof(AnimDataHeader)
  unsigned reserved; // 0x00000000
};

// header for a2d version less 150
struct AnimData::AnimDataHeaderLessV150
{
  int totalKeyPosNum, totalKeySclNum, totalKeyRotNum;
  int totalPosNodeNum, totalSclNodeNum, totalRotNodeNum;
  unsigned namePoolSize, totalAnimLabelNum;
  unsigned originLinVelKeyNum, originAngVelKeyNum;
};

struct AnimData::TmpNodeBuf
{
  int nameIdx;
  int keyNum;
  real w;
};


// header for a2d version 200
struct AnimData::AnimDataHeaderV200
{
  unsigned namePoolSize;      // size of name pool, in bytes
  unsigned timeNum;           // number of entries in time pool
  unsigned keyPoolSize;       // size of key pool, in bytes
  unsigned totalAnimLabelNum; // number of keys in note track (time labels)
  unsigned dataTypeNum;       // number of data type records
};

struct AnimData::DataTypeHeader
{
  unsigned dataType;         // id of data type, analogue for channelType: POINT3, QUAT, REAL
  unsigned offsetInKeyPool;  // offset in bytes from the beginning of key pool
  unsigned offsetInTimePool; // offset in elements from the beginning of time pool
  unsigned channelNum;       // number of channels of this data type (see below)
};


AnimData::AnimData() : pool(NULL)
{
  memset(&originLinVel, 0, sizeof(originLinVel));
  memset(&originAngVel, 0, sizeof(originAngVel));
}


AnimData::~AnimData()
{
  for (int i = animDataChannelsPoint3.size(); --i >= 0;)
    animDataChannelsPoint3[i].clear();

  for (int i = animDataChannelsQuat.size(); --i >= 0;)
    animDataChannelsQuat[i].clear();

  for (int i = animDataChannelsReal.size(); --i >= 0;)
    animDataChannelsReal[i].clear();

  clearOldData();

  noteTrack.clear();

  if (pool)
  {
    delete[] pool;
    pool = NULL;
  }
}


template <class ANIM_CHAN, class ANIM_KEY>
void AnimData::setAnimDataChanParams(const TmpNodeBuf *node_buf, const AnimDataChan<ANIM_CHAN> *animDataChan, ANIM_KEY *channel_pool,
  char *name_pool, int *time_pool, int &pool_n, int &time_pool_n)
{
  if (!animDataChan)
    return;

  for (int i = 0; i < animDataChan->nodeNum; i++)
  {
    // reading pos-channel nodes
    animDataChan->nodeName[i] = name_pool + node_buf[i].nameIdx;
    //    debug( "node %d.pos: %s", i, animDataChan->nodeName[i] );

    animDataChan->nodeAnim[i].key = channel_pool + pool_n;
    animDataChan->nodeAnim[i].keyTime = time_pool + time_pool_n;
    animDataChan->nodeAnim[i].keyNum = node_buf[i].keyNum;
    animDataChan->nodeWt[i] = node_buf[i].w;
    pool_n += node_buf[i].keyNum;
    time_pool_n += node_buf[i].keyNum;
    //    debug( "pos[%d]: %s: keyNum=%d w=%.3f  key=%p time=%p",
    //      i, animDataChan->nodeName[i], animDataChan->nodeAnim[i].keyNum,
    //      animDataChan->nodeWt[i], animDataChan->nodeAnim[i].key,
    //      animDataChan->nodeAnim[i].keyTime );
  }
}


template <class ANIM_CHANNEL, class ANIM_KEY>
void AnimData::readAnimDataChanParams(SmallTab<AnimDataChan<ANIM_CHANNEL>, MidmemAlloc> &channels, const unsigned channel_num,
  const void *key_pool, const char *name_pool, const int *time_pool, IGenLoad &cb, class IMemAlloc *ma)
{
  clear_and_resize(channels, channel_num);
  Tab<unsigned> keyNums(tmpmem);
  Tab<int> nodeNameIds(tmpmem);
  Tab<float> nodeWeights(tmpmem);
  unsigned channelType;
  unsigned nodeNum;
  unsigned timePoolOffset = 0;
  for (int channelId = 0; channelId < channel_num; ++channelId)
  {
    cb.read(&channelType, sizeof(channelType));
    cb.read(&nodeNum, sizeof(nodeNum));
    keyNums.resize(nodeNum);
    nodeNameIds.resize(nodeNum);
    nodeWeights.resize(nodeNum);
    cb.read(&keyNums[0], sizeof(unsigned) * nodeNum);
    cb.read(&nodeNameIds[0], sizeof(int) * nodeNum);
    cb.read(&nodeWeights[0], sizeof(float) * nodeNum);

    AnimDataChan<ANIM_CHANNEL> &channel = channels[channelId];
    channel.channelType = channelType;
    channel.alloc(nodeNum, ma);

    for (int nodeId = 0; nodeId < nodeNum; ++nodeId)
    {
      channel.nodeName[nodeId] = (char *)name_pool + nodeNameIds[nodeId];
      const unsigned keyNum = keyNums[nodeId];
      channel.nodeAnim[nodeId].keyNum = keyNum;
      channel.nodeAnim[nodeId].key = ((ANIM_KEY *)key_pool) + timePoolOffset;
      channel.nodeAnim[nodeId].keyTime = (int *)time_pool + timePoolOffset;
      channel.nodeWt[nodeId] = nodeWeights[nodeId];
      timePoolOffset += keyNum;
    }
  }
}

AnimDataChan<AnimChanPoint3> *AnimData::getPoint3AnimDataChan(const unsigned channel_type)
{
  for (int i = animDataChannelsPoint3.size(); --i >= 0;)
    if (channel_type == animDataChannelsPoint3[i].channelType)
      return &animDataChannelsPoint3[i];
  return NULL;
}


AnimDataChan<AnimChanQuat> *AnimData::getQuatAnimDataChan(const unsigned channel_type)
{
  for (int i = animDataChannelsQuat.size(); --i >= 0;)
    if (channel_type == animDataChannelsQuat[i].channelType)
      return &animDataChannelsQuat[i];
  return NULL;
}


AnimDataChan<AnimChanReal> *AnimData::getRealAnimDataChan(const unsigned channel_type)
{
  for (int i = animDataChannelsReal.size(); --i >= 0;)
    if (channel_type == animDataChannelsReal[i].channelType)
      return &animDataChannelsReal[i];
  return NULL;
}

AnimChanPoint3 *AnimData::getPoint3Anim(const unsigned channel_type, const char *node_name)
{
  int animId = -1;
  AnimDataChan<AnimChanPoint3> *animDataChan = getPoint3AnimDataChan(channel_type);
  if (animDataChan)
    if (-1 != (animId = animDataChan->getAnimId(node_name)))
      return &animDataChan->nodeAnim[animId];
  return NULL;
}

struct ChannelTypeParams
{
  unsigned channelType;
  int keyCount;
  int nodeCount;

  ChannelTypeParams() : channelType(0), keyCount(0), nodeCount(0) {}
  ChannelTypeParams(unsigned channel_type, int key_count, int node_count) :
    channelType(channel_type), keyCount(key_count), nodeCount(node_count)
  {}
};


bool AnimData::load(IGenLoad &cb, class IMemAlloc *ma)
{
  AnimDataHeader hdr;

  // read header and check integrity
  cb.read(&hdr, sizeof(hdr));
  if (hdr.label != MAKE4C('A', 'N', 'I', 'M'))
  {
    DEBUG_CTX("unrecognized label %c%c%c%c", hdr.label >> 24, hdr.label >> 16, hdr.label >> 8, hdr.label);
    return false;
  }
  if (hdr.ver <= 0x150)
    return loadLessV150(cb, ma, hdr);
  else if (hdr.ver == 0x200)
    return loadV200(cb, ma, hdr);

  DEBUG_CTX("unsupported version 0x%p", hdr.ver);
  return false;
}


bool AnimData::loadV200(IGenLoad &cb, class IMemAlloc *ma, AnimDataHeader &hdr)
{
  AnimDataHeaderV200 hdrV200;
  cb.read(&hdrV200, sizeof(hdrV200));

  const unsigned hdrSize = sizeof(hdr) + sizeof(hdrV200);
  if (hdr.hdrSize != hdrSize)
  {
    DEBUG_CTX("incorrect hdrSize=%d for ver 0x100 (should be =%d)", hdr.hdrSize, sizeof(hdr));
    return false;
  }

  // read pools
  const unsigned poolSize = hdrV200.namePoolSize + (sizeof(unsigned) * hdrV200.timeNum) + hdrV200.keyPoolSize;
  pool = new (ma) char[poolSize];
  cb.read(pool, poolSize);

  noteTrack.alloc(hdrV200.totalAnimLabelNum);
  cb.read(noteTrack.pool, sizeof(AnimKeyLabel) * hdrV200.totalAnimLabelNum);

  // remap pointers
  const char *namePool = pool;
  const int *timePool = (int *)(namePool + hdrV200.namePoolSize);
  const char *keyPool = (char *)(timePool + hdrV200.timeNum);
  // unsigned timePoolOffset = 0;


  // read data of animation
  DataTypeHeader dataTypeHeader;
  for (int dataTypeId = hdrV200.dataTypeNum; --dataTypeId >= 0;)
  {
    cb.read(&dataTypeHeader, sizeof(dataTypeHeader));
    const unsigned dataType = dataTypeHeader.dataType;
    const unsigned channelNum = dataTypeHeader.channelNum;
    const void *currentKeyPool = keyPool + dataTypeHeader.offsetInKeyPool;
    const int *curreneTimePool = timePool + dataTypeHeader.offsetInTimePool;

    if (dataType == DATATYPE_POINT3)
      readAnimDataChanParams<AnimChanPoint3, AnimKeyPoint3>(animDataChannelsPoint3, channelNum, currentKeyPool, namePool,
        curreneTimePool, cb, ma);
    else if (dataType == DATATYPE_QUAT)
      readAnimDataChanParams<AnimChanQuat, AnimKeyQuat>(animDataChannelsQuat, channelNum, currentKeyPool, namePool, curreneTimePool,
        cb, ma);
    else if (dataType == DATATYPE_REAL)
      readAnimDataChanParams<AnimChanReal, AnimKeyReal>(animDataChannelsReal, channelNum, currentKeyPool, namePool, curreneTimePool,
        cb, ma);
  }


  // resolve note track names
  for (int i = 0; i < noteTrack.num; ++i)
  {
    noteTrack.pool[i].name = namePool + (int)noteTrack.pool[i].name;
    // debug ( "noteTrack: %d: %s: t=%d", i, noteTrack.pool[i].name, noteTrack.pool[i].time );
  }
  // remove trailing white space
  for (int i = 0; i < noteTrack.num; ++i)
  {
    char *poolName = (char *)noteTrack.pool[i].name;
    int cnum = strlen(poolName);
    while (cnum > 0 && strchr(" \r\n\t", poolName[cnum - 1]))
    {
      --cnum;
      poolName[cnum] = '\0';
    }
  }


  setOldData(ma);
  return true;
}

bool AnimData::loadLessV150(IGenLoad &cb, class IMemAlloc *ma, AnimDataHeader &hdr)
{
  AnimDataHeaderLessV150 hdrLessV150;
  cb.read(&hdrLessV150, sizeof(hdrLessV150));

  const unsigned hdrSize = sizeof(hdr) + sizeof(hdrLessV150);
  if (hdr.ver == 0x100 && (hdr.hdrSize) == hdrSize - 8)
  {
    DEBUG_CTX("loading anim rev 1.00   vvv");
    // for backward compatibility with rev 1.00
    hdr.ver = 0x150;
    hdr.hdrSize = hdrSize;
    hdrLessV150.originLinVelKeyNum = 0;
    hdrLessV150.originAngVelKeyNum = 0;
    cb.seekrel(-8);
  }

  if (hdr.ver != 0x150)
  {
    DEBUG_CTX("unsupported version 0x%p", hdr.ver);
    return false;
  }
  if (hdr.hdrSize != hdrSize)
  {
    DEBUG_CTX("incorrect hdrSize=%d for ver 0x100 (should be =%d)", hdr.hdrSize, sizeof(hdr));
    return false;
  }

  // calculate channel counts, key counts and save using channel types
  Tab<ChannelTypeParams> usingPoint3ChannelTypes(tmpmem);

  if (hdrLessV150.totalKeyPosNum > 0)
    usingPoint3ChannelTypes.push_back(ChannelTypeParams(CHTYPE_POSITION, hdrLessV150.totalKeyPosNum, hdrLessV150.totalPosNodeNum));
  if (hdrLessV150.totalKeySclNum > 0)
    usingPoint3ChannelTypes.push_back(ChannelTypeParams(CHTYPE_SCALE, hdrLessV150.totalKeySclNum, hdrLessV150.totalSclNodeNum));
  if (hdrLessV150.originLinVelKeyNum > 0)
    usingPoint3ChannelTypes.push_back(ChannelTypeParams(CHTYPE_ORIGIN_LIN_VEL, 0, 1));
  if (hdrLessV150.originAngVelKeyNum > 0)
    usingPoint3ChannelTypes.push_back(ChannelTypeParams(CHTYPE_ORIGIN_ANG_VEL, 0, 1));

  Tab<ChannelTypeParams> usingQuatChannelTypes(tmpmem);
  if (hdrLessV150.totalKeyRotNum > 0)
    usingQuatChannelTypes.push_back(ChannelTypeParams(CHTYPE_ROTATION, hdrLessV150.totalKeyRotNum, hdrLessV150.totalRotNodeNum));

  Tab<ChannelTypeParams> usingRealChannelTypes(tmpmem);

  // allocate memory
  int point3KeyCount = 0;
  clear_and_resize(animDataChannelsPoint3, usingPoint3ChannelTypes.size());
  for (int i = animDataChannelsPoint3.size(); --i >= 0;)
  {
    animDataChannelsPoint3[i].channelType = usingPoint3ChannelTypes[i].channelType;
    animDataChannelsPoint3[i].alloc(usingPoint3ChannelTypes[i].nodeCount, ma);
    point3KeyCount += usingPoint3ChannelTypes[i].keyCount;
    if (CHTYPE_ORIGIN_LIN_VEL == animDataChannelsPoint3[i].channelType)
      animDataChannelsPoint3[i].nodeName[0] = str_dup(origin_lin_vel_node_name, ma);
    else if (CHTYPE_ORIGIN_ANG_VEL == animDataChannelsPoint3[i].channelType)
      animDataChannelsPoint3[i].nodeName[0] = str_dup(origin_ang_vel_node_name, ma);
  }

  int quatKeyCount = 0;
  clear_and_resize(animDataChannelsQuat, usingQuatChannelTypes.size());
  for (int i = animDataChannelsQuat.size(); --i >= 0;)
  {
    animDataChannelsQuat[i].channelType = usingQuatChannelTypes[i].channelType;
    animDataChannelsQuat[i].alloc(usingQuatChannelTypes[i].nodeCount, ma);
    quatKeyCount += usingQuatChannelTypes[i].keyCount;
  }

  int realKeyCount = 0;
  clear_and_resize(animDataChannelsReal, usingRealChannelTypes.size());
  for (int i = animDataChannelsReal.size(); --i >= 0;)
  {
    animDataChannelsReal[i].channelType = usingRealChannelTypes[i].channelType;
    animDataChannelsReal[i].alloc(usingRealChannelTypes[i].nodeCount, ma);
    realKeyCount += usingRealChannelTypes[i].keyCount;
  }

  noteTrack.alloc(hdrLessV150.totalAnimLabelNum);

  const int timeKeyCount = hdrLessV150.totalKeyPosNum + hdrLessV150.totalKeySclNum + hdrLessV150.totalKeyRotNum;

  const int namePoolSize = hdrLessV150.namePoolSize;
  const int timePoolSize = sizeof(int) * timeKeyCount;
  const int point3PoolSize = sizeof(AnimKeyPoint3) * point3KeyCount;
  const int quatPoolSize = sizeof(AnimKeyQuat) * quatKeyCount;
  const int realPoolSize = sizeof(real) * realKeyCount;

  // read pools
  const int poolSize = namePoolSize + timePoolSize + point3PoolSize + quatPoolSize + realPoolSize;
  pool = new (ma) char[poolSize];
  cb.read(pool, poolSize);
  cb.read(noteTrack.pool, sizeof(AnimKeyLabel) * hdrLessV150.totalAnimLabelNum);

  // remap pointers
  char *namePool = pool;
  int *timePool = (int *)(namePool + namePoolSize);
  AnimKeyPoint3 *posPool = (AnimKeyPoint3 *)(timePool + timeKeyCount);
  AnimKeyPoint3 *scalePool = posPool + hdrLessV150.totalKeyPosNum;
  AnimKeyQuat *rotatePool = (AnimKeyQuat *)(scalePool + hdrLessV150.totalKeySclNum);
  // real *realPool = (real *)( rotatePool + hdrLessV150.totalKeyRotNum );

  // read subobjects
  int posPoolN = 0;
  int keyTimePoolN = 0;

  //   first pos keys are origin's velocities
  AnimChanPoint3 *originVel = NULL;
  originVel = getPoint3Anim(CHTYPE_ORIGIN_LIN_VEL, origin_lin_vel_node_name);
  if (originVel)
  {
    originVel->keyNum = hdrLessV150.originLinVelKeyNum;
    originVel->key = posPool + posPoolN;
    originVel->keyTime = timePool + keyTimePoolN;
    posPoolN += originVel->keyNum;
    keyTimePoolN += originVel->keyNum;
  }
  originVel = getPoint3Anim(CHTYPE_ORIGIN_ANG_VEL, origin_ang_vel_node_name);
  if (originVel)
  {
    originVel->keyNum = hdrLessV150.originAngVelKeyNum;
    originVel->key = posPool + posPoolN;
    originVel->keyTime = timePool + keyTimePoolN;
    posPoolN += originVel->keyNum;
    keyTimePoolN += originVel->keyNum;
  }

  //   setup temp buf
  int max_tmp_size = hdrLessV150.totalPosNodeNum;
  if (max_tmp_size < hdrLessV150.totalSclNodeNum)
    max_tmp_size = hdrLessV150.totalSclNodeNum;
  if (max_tmp_size < hdrLessV150.totalRotNodeNum)
    max_tmp_size = hdrLessV150.totalRotNodeNum;

  TmpNodeBuf *tmpBuf = new (tmpmem) TmpNodeBuf[max_tmp_size];

  cb.read(tmpBuf, sizeof(TmpNodeBuf) * hdrLessV150.totalPosNodeNum);
  AnimDataChan<AnimChanPoint3> *point3AnimDataChan = getPoint3AnimDataChan(CHTYPE_POSITION);
  setAnimDataChanParams<AnimChanPoint3, AnimKeyPoint3>(tmpBuf, point3AnimDataChan, posPool, namePool, timePool, posPoolN,
    keyTimePoolN);

  cb.read(tmpBuf, sizeof(TmpNodeBuf) * hdrLessV150.totalSclNodeNum);
  int sclPoolN = 0;
  point3AnimDataChan = getPoint3AnimDataChan(CHTYPE_SCALE);
  setAnimDataChanParams<AnimChanPoint3, AnimKeyPoint3>(tmpBuf, point3AnimDataChan, scalePool, namePool, timePool, sclPoolN,
    keyTimePoolN);

  cb.read(tmpBuf, sizeof(TmpNodeBuf) * hdrLessV150.totalRotNodeNum);
  int rotPoolN = 0;
  AnimDataChan<AnimChanQuat> *quatAnimDataChan = getQuatAnimDataChan(CHTYPE_ROTATION);
  setAnimDataChanParams<AnimChanQuat, AnimKeyQuat>(tmpBuf, quatAnimDataChan, rotatePool, namePool, timePool, rotPoolN, keyTimePoolN);

  // resolve note track names
  for (int i = 0; i < noteTrack.num; i++)
  {
    noteTrack.pool[i].name = namePool + (int)noteTrack.pool[i].name;
    // debug ( "noteTrack: %d: %s: t=%d", i, noteTrack.pool[i].name, noteTrack.pool[i].time );
  }
  // remove trailing white space
  for (int i = 0; i < noteTrack.num; i++)
  {
    char *pool_name = (char *)noteTrack.pool[i].name;
    int cnum = strlen(pool_name);
    while (cnum > 0 && strchr(" \r\n\t", pool_name[cnum - 1]))
    {
      --cnum;
      pool_name[cnum] = '\0';
    }
  }

  delete[] tmpBuf;

  // check integrity
  if (posPoolN != hdrLessV150.totalKeyPosNum || sclPoolN != hdrLessV150.totalKeySclNum || rotPoolN != hdrLessV150.totalKeyRotNum)
  {
    DEBUG_CTX("incorrect counters: posPoolN=%d != %d=hdr.totalKeyPosNum || sclPoolN=%d != %d=hdr.totalKeySclNum || rotPoolN=%d != "
              "%d=hdr.totalKeyRotNum",
      posPoolN, hdrLessV150.totalKeyPosNum, sclPoolN, hdrLessV150.totalKeySclNum, rotPoolN, hdrLessV150.totalKeyRotNum);
    return false;
  }

  setOldData(ma);
  return true;
}


void AnimData::setOldData(IMemAlloc *ma)
{
  AnimDataChan<AnimChanPoint3> *point3AnimDataChan = getPoint3AnimDataChan(CHTYPE_POSITION);
  if (point3AnimDataChan)
  {
    pos.alloc(point3AnimDataChan->nodeNum, ma);
    for (int i = pos.nodeNum; --i >= 0;)
    {
      pos.nodeAnim[i] = point3AnimDataChan->nodeAnim[i];
      pos.nodeName[i] = point3AnimDataChan->nodeName[i];
      pos.nodeWt[i] = point3AnimDataChan->nodeWt[i];
    }
    pos.channelType = point3AnimDataChan->channelType;
  }

  point3AnimDataChan = getPoint3AnimDataChan(CHTYPE_SCALE);
  if (point3AnimDataChan)
  {
    scl.alloc(point3AnimDataChan->nodeNum, ma);
    for (int i = scl.nodeNum; --i >= 0;)
    {
      scl.nodeAnim[i] = point3AnimDataChan->nodeAnim[i];
      scl.nodeName[i] = point3AnimDataChan->nodeName[i];
      scl.nodeWt[i] = point3AnimDataChan->nodeWt[i];
    }
    scl.channelType = point3AnimDataChan->channelType;
  }

  AnimDataChan<AnimChanQuat> *quatAnimDataChan = getQuatAnimDataChan(CHTYPE_ROTATION);
  if (quatAnimDataChan)
  {
    rot.alloc(quatAnimDataChan->nodeNum, ma);
    for (int i = rot.nodeNum; --i >= 0;)
    {
      rot.nodeAnim[i] = quatAnimDataChan->nodeAnim[i];
      rot.nodeName[i] = quatAnimDataChan->nodeName[i];
      rot.nodeWt[i] = quatAnimDataChan->nodeWt[i];
    }
    rot.channelType = quatAnimDataChan->channelType;
  }

  AnimChanPoint3 *originVel = NULL;
  originVel = getPoint3Anim(CHTYPE_ORIGIN_LIN_VEL, origin_lin_vel_node_name);
  if (originVel)
  {
    originLinVel.keyNum = originVel->keyNum;
    originLinVel.key = originVel->key;
    originLinVel.keyTime = originVel->keyTime;
  }
  else
    memset(&originLinVel, 0, sizeof(originLinVel));

  originVel = getPoint3Anim(CHTYPE_ORIGIN_ANG_VEL, origin_ang_vel_node_name);
  if (originVel)
  {
    originAngVel.keyNum = originVel->keyNum;
    originAngVel.key = originVel->key;
    originAngVel.keyTime = originVel->keyTime;
  }
  else
    memset(&originAngVel, 0, sizeof(originAngVel));
}


void AnimData::clearOldData()
{
  pos.clear();
  scl.clear();
  rot.clear();
  memset(&originLinVel, 0, sizeof(originLinVel));
  memset(&originAngVel, 0, sizeof(originAngVel));
}
