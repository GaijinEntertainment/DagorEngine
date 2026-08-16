// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if !_TARGET_PC
#undef _DEBUG_TAB_
#endif

#include <math/dag_geomTree.h>
#include <math/dag_bounds3.h>
#include <math/dag_mathUtils.h>
#include <vecmath/dag_vecMath.h>
#include <ioSys/dag_genIo.h>
#include <osApiWrappers/dag_localConv.h>
#include <debug/dag_debug.h>
#include <generic/dag_span.h>
#include <ioSys/dag_oodleIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_chainedMemIo.h>
#include <ioSys/dag_btagCompr.h>

#if MEASURE_PERF
#include <perfMon/dag_perfMonStat.h>
#include <perfMon/dag_perfTimer2.h>
#include <startup/dag_globalSettings.h>

static PerformanceTimer2 perf_tm;
static int perf_cnt = 0, perf_tm_cnt = 0;
static int perf_frameno_start = 0;
#endif

GeomNodeTree::~GeomNodeTree()
{
  if (!dataCoalloc && data.data())
    memfree(data.data(), midmem);
}


void GeomNodeTree::copyDataFrom(const GeomNodeTree &from)
{
  const intptr_t dataSz = from.data.size();
  G_ASSERT(data.size() == dataSz);
  if (dataSz)
    memcpy(data.data(), from.data.data(), dataSz);

  numNodes = from.numNodes;
  lastValidWtmIndex = from.lastValidWtmIndex;
  invalidTmOfs = from.invalidTmOfs;
  lastUnimportantCount = from.lastUnimportantCount;
  setWtmOfs(from.wofs); // node arrays derive from data+numNodes, so no pointer fixup is needed
  markPoseChanged();    // content replacement: bump the TARGET's own generation (never inherit the source's)
}

GeomNodeTree &GeomNodeTree::operator=(const GeomNodeTree &from)
{
  const intptr_t dataSz = from.data.size();
  G_ASSERT(!dataCoalloc || data.size() == dataSz); // co-allocated data can't be resized, see make()
  if (!dataCoalloc && data.size() != dataSz)
  {
    if (data.data())
      memfree(data.data(), midmem);
    data.set(dataSz ? (char *)memalloc(dataSz, midmem) : nullptr, dataSz);
  }
  copyDataFrom(from);
  return *this;
}

/* static */
GeomNodeTree *GeomNodeTree::allocWithData(intptr_t data_sz)
{
  char *blk = (char *)memalloc(sizeof(GeomNodeTree) + data_sz, midmem);
  GeomNodeTree *tree = ::new (blk) GeomNodeTree();

  tree->data.set(data_sz ? blk + sizeof(GeomNodeTree) : nullptr, data_sz);
  tree->dataCoalloc = true;
  return tree;
}

/* static */
GeomNodeTree *GeomNodeTree::make(const GeomNodeTree &from)
{
  GeomNodeTree *tree = allocWithData(from.data.size());
  tree->copyDataFrom(from);
  return tree;
}

void GeomNodeTree::destroy()
{
  G_ASSERT(dataCoalloc);
  this->~GeomNodeTree();
  memfree(this, midmem);
}

dag::Index16 GeomNodeTree::findNodeIndex(const char *name) const
{
  for (Index16 i(0), ie(nodeCount()); i != ie; ++i)
    if (strcmp(getNodeName(i), name) == 0) //-V575
      return i;
  return {};
}
dag::Index16 GeomNodeTree::findINodeIndex(const char *name) const
{
  for (Index16 i(0), ie(nodeCount()); i != ie; ++i)
    if (dd_stricmp(getNodeName(i), name) == 0)
      return i;
  return {};
}

void GeomNodeTree::calcWtm()
{
  const int nodesCount = nodeCount();
  int important_nodes = nodesCount - lastUnimportantCount;
  if (lastValidWtmIndex >= important_nodes - 1 || !important_nodes)
    return;
  markPoseChanged();
#if MEASURE_PERF
  perf_tm.go();
#endif

  Index16 p_idx;
  mat44f p_wtm, c_wtm;
  mat44f *__restrict wtmPtr = wtm().data();
  mat44f *__restrict tmPtr = tm().data();
  const dag::Index16 *__restrict parentIdPtr = parentId().data();

  if (lastValidWtmIndex < 0)
  {
    p_idx = Index16(0);
    p_wtm = tmPtr[0];

    wtmPtr[0] = p_wtm;
    lastValidWtmIndex = 0;
    if (nodesCount == 1)
    {
#if MEASURE_PERF
      perf_tm.pause();
      perf_tm_cnt += 1;
#endif
      return;
    }
  }
  else
  {
    p_idx = parentIdPtr[lastValidWtmIndex + 1];
#ifdef _DEBUG_TAB_
    G_FAST_ASSERT(p_idx.index() < nodesCount);
#endif
    p_wtm = wtmPtr[p_idx.index()];
  }

  int i = lastValidWtmIndex + 1, ie = min(important_nodes, nodesCount - 1);
  parentIdPtr += i + 1;
  for (; i < ie; ++i, ++parentIdPtr)
  {
    v_mat44_mul43(c_wtm, p_wtm, tmPtr[i]);
    wtmPtr[i] = c_wtm;

    const Index16 parentI = *parentIdPtr;
    if (parentI.index() == i)
    {
      p_idx = Index16(i);
      p_wtm = c_wtm;
    }
    else if (parentI != p_idx)
    {
      p_idx = parentI;
#ifdef _DEBUG_TAB_
      G_FAST_ASSERT(p_idx.index() < nodesCount);
#endif
      p_wtm = wtmPtr[p_idx.index()];
    }
  }
  if (i == nodesCount - 1)
    v_mat44_mul43(wtmPtr[i], p_wtm, tmPtr[i]);
#if MEASURE_PERF
  perf_tm_cnt += important_nodes - lastValidWtmIndex - 1;
#endif
  lastValidWtmIndex = important_nodes - 1;

#if MEASURE_PERF
  perf_tm.pause();
  perf_cnt++;
#if MEASURE_PERF_FRAMES
  if (::dagor_frame_no() > perf_frameno_start + MEASURE_PERF_FRAMES)
#else
  if (perf_cnt >= MEASURE_PERF)
#endif
  {
    perfmonstat::geomtree_calcWtm_cnt = perf_cnt;
    perfmonstat::geomtree_calcWtm_tm_cnt = perf_tm_cnt;
    perfmonstat::geomtree_calc_time_usec = perf_tm.getTotalUsec();
    perfmonstat::generation_counter++;

    perf_tm.reset();
    perf_cnt = perf_tm_cnt = 0;
    perf_frameno_start = ::dagor_frame_no();
  }
#endif
}
void GeomNodeTree::partialCalcWtm(dag::Index16 upto_ni)
{
  if (lastValidWtmIndex + 1 >= (int)nodeCount() || !nodeCount())
    return;
  markPoseChanged();
#if MEASURE_PERF
  perf_tm.go();
#endif

  Index16 p_idx;
  mat44f p_wtm, c_wtm;

  if (lastValidWtmIndex < 0)
  {
    p_idx = Index16(0);
    p_wtm = tm()[0];

    wtm()[0] = p_wtm;
    lastValidWtmIndex = 0;
    if (nodeCount() == 1)
    {
#if MEASURE_PERF
      perf_tm.pause();
      perf_tm_cnt += 1;
#endif
      return;
    }
  }
  else
  {
    p_idx = parentId()[lastValidWtmIndex + 1];
    p_wtm = wtm()[p_idx.index()];
  }

  int upto_nodeid = upto_ni ? upto_ni.index() : 0;
  if (upto_nodeid >= nodeCount())
    upto_nodeid = nodeCount() - 1;

  if (upto_nodeid > lastValidWtmIndex)
  {
    mat44f *__restrict wtmPtr = wtm().data();
    mat44f *__restrict tmPtr = tm().data();
    int i = lastValidWtmIndex + 1;
    const Index16 *__restrict parentIdPtr = parentId().data() + i + 1;
    for (; i < upto_nodeid; ++i, ++parentIdPtr)
    {
      v_mat44_mul43(c_wtm, p_wtm, tmPtr[i]);
      wtmPtr[i] = c_wtm;

      const Index16 parentI = *parentIdPtr;
      if (parentI.index() == i)
      {
        p_idx = Index16(i);
        p_wtm = c_wtm;
      }
      else if (parentI != p_idx)
      {
        p_idx = parentI;
        p_wtm = wtmPtr[p_idx.index()];
      }
    }
    v_mat44_mul43(wtmPtr[upto_nodeid], p_wtm, tmPtr[upto_nodeid]);
#if MEASURE_PERF
    perf_tm_cnt += upto_nodeid - lastValidWtmIndex;
#endif
    lastValidWtmIndex = upto_nodeid;
  }

#if MEASURE_PERF
  perf_cnt++;
#if MEASURE_PERF_FRAMES
  if (::dagor_frame_no() > perf_frameno_start + MEASURE_PERF_FRAMES)
#else
  if (perf_cnt >= MEASURE_PERF)
#endif
  {
    perfmonstat::geomtree_calcWtm_cnt = perf_cnt;
    perfmonstat::geomtree_calcWtm_tm_cnt = perf_tm_cnt;
    perfmonstat::geomtree_calc_time_usec = perf_tm.getTotalUsec();
    perfmonstat::generation_counter++;

    perf_tm.reset();
    perf_cnt = perf_tm_cnt = 0;
    perf_frameno_start = ::dagor_frame_no();
  }
#endif
}

namespace
{
struct OldGeomTreeNode
{
  mat44f tm, wtm;
  PatchableTab<OldGeomTreeNode> child;
  PatchablePtr<OldGeomTreeNode> parent;
  PatchablePtr<const char> name;
};
} // namespace

/* static */
GeomNodeTree *GeomNodeTree::load(IGenLoad &_cb)
{
  unsigned ofs = _cb.readInt();
  unsigned nodeCnt = _cb.readInt();
  bool compr = (nodeCnt & 0x80000000) != 0;
  nodeCnt &= ~0x80000000;

  MemoryChainedData *unpacked_data = nullptr;
  unsigned blockFlags = 0;
  const int blockSize = compr ? _cb.beginBlock(&blockFlags) : 0;

  if (blockFlags == btag_compr::ZSTD)
  {
    MemorySaveCB cwrUnpack(clamp((blockSize * 4 + 0xFFF) & ~0xFFF, 2 << 10, 64 << 10));
    zstd_decompress_data(cwrUnpack, _cb, blockSize);
    unpacked_data = cwrUnpack.takeMem();
  }
  else if (blockFlags == btag_compr::OODLE)
  {
    int decompSize = _cb.readInt();
    MemorySaveCB cwrUnpack(eastl::min(decompSize, 64 << 10));
    oodle_decompress_data(cwrUnpack, _cb, blockSize - sizeof(int), decompSize);
    unpacked_data = cwrUnpack.takeMem();
  }
  if (compr)
    _cb.endBlock();

  MemoryLoadCB crd(unpacked_data, true);
  IGenLoad &cb = compr ? static_cast<IGenLoad &>(crd) : _cb;

  const unsigned old_data_sz = ofs & 0xFFFFF;

  G_ASSERT(nodeCnt * 2 * sizeof(mat44f) <= old_data_sz);

  // read older dump
  SmallTab<char, TmpmemAlloc> old_data;
  clear_and_resize(old_data, old_data_sz);
  cb.read(old_data.data(), old_data_sz);

  PatchableTab<OldGeomTreeNode> old_nodes;
  int name_pool_sz = 0;
  old_nodes.init(old_data.data(), nodeCnt);
  for (int i = 0; i < old_nodes.size(); ++i)
  {
    OldGeomTreeNode &n = old_nodes[i];

    // patch pointers
    n.child.patch(old_data.data());
    n.parent.patch(old_data.data());
    n.name.patch(old_data.data());
    name_pool_sz += (int)strlen(n.name) + 1;
  }

  // build new layout
  int wtm_ofs = 0;
  int tm_ofs = wtm_ofs + int(sizeof(mat44f)) * nodeCnt;
  int parentId_ofs = tm_ofs + int(sizeof(mat44f)) * nodeCnt;
  int childrenRef_ofs = parentId_ofs + int(sizeof(Index16)) * nodeCnt;
  int nameOfs_ofs = childrenRef_ofs + int(sizeof(ChildRef)) * nodeCnt;
  int invalid_tm_ofs = nameOfs_ofs + int(sizeof(uint16_t)) * nodeCnt + name_pool_sz;

  const intptr_t dataSz = invalid_tm_ofs + (nodeCnt + 7) / 8;
  GeomNodeTree *tree = allocWithData(dataSz);
  dag::Span<char> &data = tree->data;
  if (dataSz)
    memset(data.data(), 0, dataSz); // leaf childrenRef and the invalid-tm bits are left zeroed
  tree->numNodes = nodeCnt;
  tree->invalidTmOfs = invalid_tm_ofs;
  tree->lastUnimportantCount = ofs >> 20;

  auto wtmA = tree->wtm();
  auto tmA = tree->tm();
  auto parentIdA = tree->parentId();
  auto childrenRefA = tree->childrenRef();
  auto nameOfsA = tree->nameOfs();

  // copy data from older dump to new layout
  for (int i = 0, nm_cur_ofs = int(sizeof(uint16_t)) * nodeCnt; i < old_nodes.size(); ++i)
  {
    OldGeomTreeNode &n = old_nodes[i];
    tmA[i] = n.tm;
    wtmA[i] = n.wtm;
    parentIdA[i] = (n.parent && n.parent != &n) ? Index16(n.parent.get() - old_nodes.data()) : Index16();

    nameOfsA[i] = nm_cur_ofs;
    int nm_len = (int)strlen(n.name);
    memcpy(&data[nameOfs_ofs + nm_cur_ofs], n.name, nm_len + 1);
    nm_cur_ofs += nm_len + 1;

    if (n.child.size())
    {
      int first_child_idx = n.child.data() - old_nodes.data();
      G_ASSERTF_CONTINUE(first_child_idx >= i && first_child_idx < i + (1 << 16) && n.child.size() < (1 << 16),
        "for node[%d]: first_child_idx=%d child.count=%d", i, first_child_idx, n.child.size());
      childrenRefA[i].count = n.child.size();
      childrenRefA[i].relOfs = first_child_idx - i;
    }
  }

  tree->invalidateWtm();
  tree->markPoseChanged(); // content established: same bump rule as copy-assign
  return tree;
}


void GeomNodeTree::calcWorldBox(bbox3f &box) const
{
  v_bbox3_init_empty(box);
  for (auto &m : wtm())
    v_bbox3_add_pt(box, m.col3);
}

void GeomNodeTree::calcWorldBox(BBox3 &box) const
{
  bbox3f vbox;
  calcWorldBox(vbox);
  v_stu_bbox3(box, vbox);
}

void GeomNodeTree::validateTm(dag::Index16 from_nodeid)
{
  Index16 p_idx;
  mat44f p_iwtm;

  v_mat44_ident(p_iwtm);
  const Index16 *parentIdPtr = parentId().data();
  mat44f *wtmPtr = wtm().data(), *tmPtr = tm().data();
  for (unsigned i = from_nodeid.index(); i <= lastValidWtmIndex; ++i)
    if (data[invalidTmOfs + (i >> 3)] & (1 << (i & 7)))
    {
      data[invalidTmOfs + (i >> 3)] &= ~(1 << (i & 7));

      if (p_idx != parentIdPtr[i])
      {
        p_idx = parentIdPtr[i];
        v_mat44_inverse43(p_iwtm, wtmPtr[p_idx.index()]);
      }
      v_mat44_mul43(tmPtr[i], p_iwtm, wtmPtr[i]);
    }
}

void GeomNodeTree::verifyAllData() const
{
#if DAGOR_DBGLEVEL > 0
  for (auto wtmA = wtm(); const mat44f &m : wtmA)
    G_ASSERTF(is_valid_tm(m), "Bad wtm[%i] " FMT_TM, int(&m - wtmA.data()), VTMD(m));
  for (auto tmA = tm(); const mat44f &m : tmA)
    G_ASSERTF(is_valid_tm(m), "Bad tm[%i] " FMT_TM, int(&m - tmA.data()), VTMD(m));
  G_ASSERTF(is_valid_pos(wofs), "Bad wofs " FMT_P3, V3D(wofs));
#endif
}

void GeomNodeTree::verifyOnlyTmFast() const
{
#if DAGOR_DBGLEVEL > 0
  vec3f res = v_zero();
  for (auto tmA = tm(); mat44f m : tmA)
    res = v_add(res, m.col3);
  if (!is_valid_pos(v_safediv(res, v_splats(numNodes))))
  {
    debug("GeomNodeTree nodes tm dump:");
    auto tmA = tm();
    for (Index16 i(0), ie(nodeCount()); i != ie; ++i)
      debug("\t%s " FMT_TM, getNodeName(i), VTMD(tmA[i.index()]));
    logerr("Invalid tm in GeomNodeTree. Nodes tm dumped to log.");
  }
#endif
}
