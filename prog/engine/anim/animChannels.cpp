// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_globDef.h>
#include <anim/dag_animChannels.h>
#include <anim/dag_animKeyInterp.h>
#include <gameRes/dag_gameResources.h>
#include <ioSys/dag_fileIo.h>
#include <generic/dag_tab.h>
#include <debug/dag_debug.h>
#include "animInternal.h"
using namespace AnimV20;


// common header
struct AnimData::AnimDataHeader
{
  unsigned label;    // MAKE4C('A','N','I','M')
  unsigned ver;      // 0x300
  unsigned hdrSize;  // =sizeof(AnimDataHeader)
  unsigned dumpSize; // total dump size
};


static int count_remapped_nodes(PatchablePtr<char> *nodeName, int nodeNum, const NameMap &node_list)
{
  int remapped = 0;
  for (int j = 0; j < nodeNum; j++)
    if (node_list.getNameId(nodeName[j]) != -1)
      remapped++;
  return remapped;
}

static inline void *regAlloc(int sz, char *&base, int align = 8)
{
  char *p = (char *)(ptrdiff_t(base + align - 1) & ~(align - 1));
  base = p + sz;
  return p;
}

static void cloneAnimDataChan(AnimDataChan &d, const NameMap &nodes, const AnimDataChan &s, char *&allocPtr, Tab<int> &remap_storage)
{
#if DAGOR_DBGLEVEL > 0
  remap_storage.resize(nodes.nameCount());
  mem_set_0(remap_storage);
  for (int i = 0; i < s.nodeNum; i++)
  {
    int id = nodes.getNameId(s.nodeName[i]);
    if (id >= 0)
      remap_storage[id]++;
  }
  for (int id = 0; id < remap_storage.size(); id++)
    if (remap_storage[id] > 1)
      logerr("duplicate node name <%s> in a2d (%d dups)", nodes.getName(id), remap_storage[id]);
#endif
  remap_storage.clear();
  for (int i = 0; i < s.nodeNum; i++)
    if (nodes.getNameId(s.nodeName[i]) != -1)
      remap_storage.push_back(i);

  int remap_count = remap_storage.size();
  if (!remap_count)
  {
    memset(&d, 0, sizeof(d));
    return;
  }

  d.animTracks.setPtr(s.animTracks);
  d.nodeName.setPtr(regAlloc(remap_count * sizeof(d.nodeName[0]), allocPtr, 8));
  d.nodeWt.setPtr(regAlloc(remap_count * sizeof(d.nodeWt[0]), allocPtr, 4));
  d.nodeTrack.setPtr(regAlloc(remap_count * sizeof(d.nodeTrack[0]), allocPtr, 2));
  d.nodeNum = remap_count;
  d.channelType = s.channelType;

  for (int i = 0; i < remap_count; i++)
  {
    d.nodeTrack[i] = s.nodeTrack[remap_storage[i]];
    d.nodeName[i] = s.nodeName[remap_storage[i]];
    d.nodeWt[i] = s.nodeWt[remap_storage[i]];
  }
}

AnimData::AnimData(AnimData *src_anim, const NameMap &node_list, IMemAlloc *ma) : dump(NULL)
{
  memset(&anim, 0, sizeof(anim));
  memset(&dumpData, 0, sizeof(dumpData));

  // set source pointer
  src = src_anim;
  animAdditive = src->animAdditive;

  // compute size of data to be duplicated and altered
  DumpData &d = src->dumpData;
  int dataSize = data_size(d.chanPoint3) + data_size(d.chanQuat) + data_size(d.chanReal);
  int add_per_node = sizeof(PatchablePtr<char>) + sizeof(float) + sizeof(dag::Index16);
  for (int i = 0; i < d.chanPoint3.size(); i++)
  {
    int remapped_nodes = count_remapped_nodes(d.chanPoint3[i].nodeName, d.chanPoint3[i].nodeNum, node_list);
    dataSize += (remapped_nodes * add_per_node + 7) & ~7;
  }
  for (int i = 0; i < d.chanQuat.size(); i++)
  {
    int remapped_nodes = count_remapped_nodes(d.chanQuat[i].nodeName, d.chanQuat[i].nodeNum, node_list);
    dataSize += (remapped_nodes * add_per_node + 7) & ~7;
  }
  for (int i = 0; i < d.chanReal.size(); i++)
  {
    int remapped_nodes = count_remapped_nodes(d.chanReal[i].nodeName, d.chanReal[i].nodeNum, node_list);
    dataSize += (remapped_nodes * add_per_node + 7) & ~7;
  }

  extraData = ma->alloc(dataSize);
  char *allocPtr = (char *)extraData;

  dumpData.chanPoint3.init(regAlloc(data_size(d.chanPoint3), allocPtr), d.chanPoint3.size());
  dumpData.chanQuat.init(regAlloc(data_size(d.chanQuat), allocPtr), d.chanQuat.size());
  dumpData.chanReal.init(regAlloc(data_size(d.chanReal), allocPtr), d.chanReal.size());
  dumpData.noteTrack.init(d.noteTrack.data(), d.noteTrack.size());

  Tab<int> remap(tmpmem);
  for (int i = 0; i < dumpData.chanPoint3.size(); i++)
    cloneAnimDataChan(dumpData.chanPoint3[i], node_list, d.chanPoint3[i], allocPtr, remap);
  for (int i = 0; i < dumpData.chanQuat.size(); i++)
    cloneAnimDataChan(dumpData.chanQuat[i], node_list, d.chanQuat[i], allocPtr, remap);
  for (int i = 0; i < dumpData.chanReal.size(); i++)
    cloneAnimDataChan(dumpData.chanReal[i], node_list, d.chanReal[i], allocPtr, remap);
  clear_and_shrink(remap);

  int unused = ((char *)extraData + dataSize) - allocPtr;
  G_ASSERTF(unused >= 0, "dataSize=%d unused=%d node_list=%d", dataSize, unused, node_list.nameCount());
  if (unused == dataSize)
  {
    ma->free(extraData);
    extraData = NULL;
  }
  else
    G_ASSERTF(unused < 32, "est=%d != %d=used", dataSize, dataSize - unused);

  anim.setup(dumpData);
}

AnimData::~AnimData()
{
  if (dump)
  {
    memfree_anywhere(dump);
    dump = NULL;
  }
  if (extraData)
  {
    memfree_anywhere(extraData);
    extraData = NULL;
  }
  memset(&anim, 0, sizeof(anim));
  memset(&dumpData, 0, sizeof(dumpData));
}

void AnimDataChan::patchData(void *base)
{
  animTracks.patch(base);
  animTracks = acl::make_compressed_tracks(animTracks.get());
  G_ASSERT_EX(animTracks, "invalid ACL compressed tracks");
  G_ASSERTF(animTracks->get_track_type() == acl::track_type8::qvvf || animTracks->get_num_tracks() == nodeNum,
    "expected %d tracks in ACL data, got %d", nodeNum, animTracks->get_num_tracks());

  nodeTrack.patch(base);
  nodeName.patch(base);
  nodeWt.patch(base);
  for (int i = 0; i < nodeNum; i++)
    nodeName[i].patch(base);
}

void AnimData::DumpData::patchData(void *base)
{
  chanPoint3.patch(base);
  chanQuat.patch(base);
  chanReal.patch(base);
  noteTrack.patch(base);
  for (int i = 0; i < chanPoint3.size(); i++)
    chanPoint3[i].patchData(base);
  for (int i = 0; i < chanQuat.size(); i++)
    chanQuat[i].patchData(base);
  for (int i = 0; i < chanReal.size(); i++)
    chanReal[i].patchData(base);
  for (int i = 0; i < noteTrack.size(); i++)
    noteTrack[i].patchData(base);
}

AnimDataChan *AnimData::DumpData::getChanPoint3(unsigned channel_type)
{
  for (int i = 0; i < chanPoint3.size(); i++)
    if (chanPoint3[i].channelType == channel_type)
      return &chanPoint3[i];
  return NULL;
}

AnimDataChan *AnimData::DumpData::getChanQuat(unsigned channel_type)
{
  for (int i = 0; i < chanQuat.size(); i++)
    if (chanQuat[i].channelType == channel_type)
      return &chanQuat[i];
  return NULL;
}

AnimDataChan *AnimData::DumpData::getChanReal(unsigned channel_type)
{
  for (int i = 0; i < chanReal.size(); i++)
    if (chanReal[i].channelType == channel_type)
      return &chanReal[i];
  return NULL;
}


PrsAnimNodeRef AnimData::getPrsAnim(const char *node_name)
{
  PrsAnimNodeRef prs;
  prs.anim = anim.rot.animTracks;
  prs.trackId = anim.rot.getTrackId(node_name);
  return prs;
}

int AnimData::getLabelTime(const char *name, bool fatal_err)
{
  for (int i = 0; i < dumpData.noteTrack.size(); i++)
    if (dd_stricmp(name, dumpData.noteTrack[i].name) == 0)
      return dumpData.noteTrack[i].time;
  if (fatal_err && !is_ignoring_unavailable_resources())
    ANIM_ERR("label <%s> not found", name);
  return -1;
}

bool AnimData::load(IGenLoad &crd, class IMemAlloc *ma)
{
  AnimDataHeader hdr;

  if (src)
  {
    DEBUG_CTX("don't want to load data: it is already alias on other AnimData");
    return false;
  }

  // read header and check integrity
  crd.read(&hdr, sizeof(hdr));
  if (hdr.label != _MAKE4C('ANIM'))
  {
    LOGERR_CTX("unrecognized label %c%c%c%c", _DUMP4C(hdr.label));
    return false;
  }
  else if (hdr.ver != 0x300 && hdr.ver != 0x301)
  {
    LOGERR_CTX("unsupported version 0x%08x", hdr.ver);
    return false;
  }

  if (hdr.hdrSize != sizeof(hdr))
  {
    LOGERR_CTX("incorrect hdrSize=%u for ver 0x%X (should be =%u)", hdr.hdrSize, hdr.ver, (unsigned)sizeof(hdr));
    return false;
  }

  // read pools
  dump = ma->alloc(hdr.dumpSize);
  crd.read(dump, hdr.dumpSize);

  crd.read(&dumpData, sizeof(dumpData));
  dumpData.patchData(dump);
  animAdditive = (hdr.ver == 0x301);

  anim.setup(dumpData);
  if ((anim.pos.animTracks && anim.pos.animTracks != anim.rot.animTracks) ||
      (anim.scl.animTracks && anim.scl.animTracks != anim.rot.animTracks))
  {
    LOGERR_CTX("PRS channels don't share the same animation data: p=%p r=%p s=%p", anim.pos.animTracks, anim.rot.animTracks,
      anim.scl.animTracks);
    return false;
  }

  return true;
}

void AnimData::Anim::setup(DumpData &d)
{
  AnimDataChan *ch;

  ch = d.getChanPoint3(CHTYPE_POSITION);
  if (ch)
    memcpy(&pos, ch, sizeof(pos));

  ch = d.getChanPoint3(CHTYPE_SCALE);
  if (ch)
    memcpy(&scl, ch, sizeof(scl));

  ch = d.getChanQuat(CHTYPE_ROTATION);
  if (ch)
    memcpy(&rot, ch, sizeof(rot));

  ch = d.getChanPoint3(CHTYPE_ORIGIN_LINVEL);
  if (ch)
  {
    G_ASSERT(ch->nodeNum == 1);
    memcpy(&originLinVel, ch, sizeof(originLinVel));
  }

  ch = d.getChanPoint3(CHTYPE_ORIGIN_ANGVEL);
  if (ch)
  {
    G_ASSERT(ch->nodeNum == 1);
    memcpy(&originAngVel, ch, sizeof(originAngVel));
  }
}
