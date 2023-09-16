#include <util/dag_globDef.h>
#include <anim/dag_animChannels.h>
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
  unsigned ver;      // 0x220
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

template <class ANIM_CHAN>
static void cloneAnimDataChan(AnimDataChan<ANIM_CHAN> &d, const NameMap &nodes, const AnimDataChan<ANIM_CHAN> &s, char *&allocPtr,
  Tab<int> &remap_storage)
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

  d.nodeAnim.setPtr(regAlloc(remap_count * sizeof(d.nodeAnim[0]), allocPtr, 8));
  d.nodeName.setPtr(regAlloc(remap_count * sizeof(d.nodeName[0]), allocPtr, 8));
  d.nodeWt.setPtr(regAlloc(remap_count * sizeof(d.nodeWt[0]), allocPtr, 4));
  d.nodeNum = remap_count;
  d.channelType = s.channelType;

  for (int i = 0; i < remap_count; i++)
  {
    d.nodeAnim[i] = s.nodeAnim[remap_storage[i]];
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
  int add_per_node = sizeof(PatchablePtr<char>) + sizeof(float);
  for (int i = 0; i < d.chanPoint3.size(); i++)
  {
    int remapped_nodes = count_remapped_nodes(d.chanPoint3[i].nodeName, d.chanPoint3[i].nodeNum, node_list);
    dataSize += (remapped_nodes * (add_per_node + sizeof(d.chanPoint3[i].nodeAnim[0])) + 7) & ~7;
  }
  for (int i = 0; i < d.chanQuat.size(); i++)
  {
    int remapped_nodes = count_remapped_nodes(d.chanQuat[i].nodeName, d.chanQuat[i].nodeNum, node_list);
    dataSize += (remapped_nodes * (add_per_node + sizeof(d.chanQuat[i].nodeAnim[0])) + 7) & ~7;
  }
  for (int i = 0; i < d.chanReal.size(); i++)
  {
    int remapped_nodes = count_remapped_nodes(d.chanReal[i].nodeName, d.chanReal[i].nodeNum, node_list);
    dataSize += (remapped_nodes * (add_per_node + sizeof(d.chanReal[i].nodeAnim[0])) + 7) & ~7;
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

AnimDataChan<AnimChanPoint3> *AnimData::DumpData::getChanPoint3(unsigned channel_type)
{
  for (int i = 0; i < chanPoint3.size(); i++)
    if (chanPoint3[i].channelType == channel_type)
      return &chanPoint3[i];
  return NULL;
}

AnimDataChan<AnimChanQuat> *AnimData::DumpData::getChanQuat(unsigned channel_type)
{
  for (int i = 0; i < chanQuat.size(); i++)
    if (chanQuat[i].channelType == channel_type)
      return &chanQuat[i];
  return NULL;
}

AnimDataChan<AnimChanReal> *AnimData::DumpData::getChanReal(unsigned channel_type)
{
  for (int i = 0; i < chanReal.size(); i++)
    if (chanReal[i].channelType == channel_type)
      return &chanReal[i];
  return NULL;
}


const AnimChanPoint3 *AnimData::getPoint3Anim(unsigned channel_type, const char *node_name)
{
  AnimDataChan<AnimChanPoint3> *animDataChan = dumpData.getChanPoint3(channel_type);
  if (animDataChan)
  {
    int animId = animDataChan->getNodeId(node_name);
    if (animId != -1)
      return &animDataChan->nodeAnim[animId];
  }
  return NULL;
}

const AnimChanQuat *AnimData::getQuatAnim(unsigned channel_type, const char *node_name)
{
  AnimDataChan<AnimChanQuat> *animDataChan = dumpData.getChanQuat(channel_type);
  if (animDataChan)
  {
    int animId = animDataChan->getNodeId(node_name);
    if (animId != -1)
      return &animDataChan->nodeAnim[animId];
  }
  return NULL;
}

const AnimChanReal *AnimData::getRealAnim(unsigned channel_type, const char *node_name)
{
  AnimDataChan<AnimChanReal> *animDataChan = dumpData.getChanReal(channel_type);
  if (animDataChan)
  {
    int animId = animDataChan->getNodeId(node_name);
    if (animId != -1)
      return &animDataChan->nodeAnim[animId];
  }
  return NULL;
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
    debug_ctx("don't want to load data: it is already alias on other AnimData");
    return false;
  }

  // read header and check integrity
  crd.read(&hdr, sizeof(hdr));
  if (hdr.label != _MAKE4C('ANIM'))
  {
    logerr_ctx("unrecognized label %c%c%c%c", _DUMP4C(hdr.label));
    return false;
  }
  else if (hdr.ver != 0x220 && hdr.ver != 0x221)
  {
    logerr_ctx("unsupported version 0x%08x", hdr.ver);
    return false;
  }

  if (hdr.hdrSize != sizeof(hdr))
  {
    logerr_ctx("incorrect hdrSize=%u for ver 0x%X (should be =%u)", hdr.hdrSize, hdr.ver, (unsigned)sizeof(hdr));
    return false;
  }

  // read pools
  dump = ma->alloc(hdr.dumpSize);
  crd.read(dump, hdr.dumpSize);

  crd.read(&dumpData, sizeof(dumpData));
  dumpData.patchData(dump);
  animAdditive = (hdr.ver == 0x221);

  anim.setup(dumpData);
  return true;
}

void AnimData::Anim::setup(DumpData &d)
{
  AnimDataChan<AnimChanPoint3> *p3;
  AnimDataChan<AnimChanQuat> *quat;

  p3 = d.getChanPoint3(CHTYPE_POSITION);
  if (p3)
    memcpy(&pos, p3, sizeof(pos));

  p3 = d.getChanPoint3(CHTYPE_SCALE);
  if (p3)
    memcpy(&scl, p3, sizeof(scl));

  quat = d.getChanQuat(CHTYPE_ROTATION);
  if (quat)
    memcpy(&rot, quat, sizeof(rot));


  p3 = d.getChanPoint3(CHTYPE_ORIGIN_LINVEL);
  if (p3)
  {
    G_ASSERT(p3->nodeNum == 1);
    memcpy(&originLinVel, &p3->nodeAnim[0], sizeof(originLinVel));
  }

  p3 = d.getChanPoint3(CHTYPE_ORIGIN_ANGVEL);
  if (p3)
  {
    G_ASSERT(p3->nodeNum == 1);
    memcpy(&originAngVel, &p3->nodeAnim[0], sizeof(originAngVel));
  }
}
