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

template <class T>
static inline void copy_rebased_slice(dag::Span<T> &dest, dag::Span<T> src, ptrdiff_t rebase_ofs)
{
  dest.set((T *)(rebase_ofs + (char *)src.data()), src.size());
}

GeomNodeTree &GeomNodeTree::operator=(const GeomNodeTree &from)
{
  data = from.data;
  if (data.data())
    setWtmOfs(from.wofs);
  lastValidWtmIndex = from.lastValidWtmIndex;
  invalidTmOfs = from.invalidTmOfs;
  lastUnimportantCount = from.lastUnimportantCount;

  ptrdiff_t rebase_ofs = data.data() - from.data.data();
  copy_rebased_slice(wtm, from.wtm, rebase_ofs);
  copy_rebased_slice(tm, from.tm, rebase_ofs);
  copy_rebased_slice(parentId, from.parentId, rebase_ofs);
  copy_rebased_slice(childrenRef, from.childrenRef, rebase_ofs);
  copy_rebased_slice(nameOfs, from.nameOfs, rebase_ofs);
  return *this;
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
#if MEASURE_PERF
  perf_tm.go();
#endif

  Index16 p_idx;
  mat44f p_wtm, c_wtm;
  mat44f *__restrict wtmPtr = wtm.data();
  mat44f *__restrict tmPtr = tm.data();
  const dag::Index16 *__restrict parentIdPtr = parentId.data();

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
#if MEASURE_PERF
  perf_tm.go();
#endif

  Index16 p_idx;
  mat44f p_wtm, c_wtm;

  if (lastValidWtmIndex < 0)
  {
    p_idx = Index16(0);
    p_wtm = tm[0];

    wtm[0] = p_wtm;
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
    p_idx = parentId[lastValidWtmIndex + 1];
    p_wtm = wtm[p_idx.index()];
  }

  int upto_nodeid = upto_ni ? upto_ni.index() : 0;
  if (upto_nodeid >= nodeCount())
    upto_nodeid = nodeCount() - 1;

  if (upto_nodeid > lastValidWtmIndex)
  {
    mat44f *__restrict wtmPtr = wtm.data();
    mat44f *__restrict tmPtr = tm.data();
    int i = lastValidWtmIndex + 1;
    const Index16 *__restrict parentIdPtr = parentId.data() + i + 1;
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

void GeomNodeTree::load(IGenLoad &_cb)
{
  unsigned ofs = _cb.readInt();
  unsigned numNodes = _cb.readInt();
  bool compr = (numNodes & 0x80000000) != 0;
  numNodes &= ~0x80000000;

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

  invalidTmOfs = ofs & 0xFFFFF;
  lastUnimportantCount = ofs >> 20;

  G_ASSERT(numNodes * (elem_size(wtm) + elem_size(tm)) <= invalidTmOfs);

  // read older dump
  SmallTab<char, TmpmemAlloc> old_data;
  clear_and_resize(old_data, invalidTmOfs);
  cb.read(old_data.data(), invalidTmOfs);

  PatchableTab<OldGeomTreeNode> old_nodes;
  int name_pool_sz = 0;
  old_nodes.init(old_data.data(), numNodes);
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
  int pers_wofs_ofs = 0; //<- to be removed later!
  int wtm_ofs = pers_wofs_ofs + sizeof(vec4f);
  int tm_ofs = wtm_ofs + elem_size(wtm) * numNodes;
  int parentId_ofs = tm_ofs + elem_size(tm) * numNodes;
  int childrenRef_ofs = parentId_ofs + elem_size(parentId) * numNodes;
  int nameOfs_ofs = childrenRef_ofs + elem_size(childrenRef) * numNodes;
  invalidTmOfs = nameOfs_ofs + elem_size(nameOfs) * numNodes + name_pool_sz;

  clear_and_resize(data, invalidTmOfs + (numNodes + 7) / 8);
  mem_set_0(data);

#define INIT_SLICE(NM, TYPE) NM.set((TYPE *)&data[NM##_ofs], old_nodes.size());
  INIT_SLICE(wtm, mat44f);
  INIT_SLICE(tm, mat44f);
  INIT_SLICE(parentId, Index16);
  INIT_SLICE(childrenRef, ChildRef);
  INIT_SLICE(nameOfs, uint16_t);
#undef INIT_SLICE

  // copy data from older dump to new layout
  for (int i = 0, nm_cur_ofs = elem_size(nameOfs) * numNodes; i < old_nodes.size(); ++i)
  {
    OldGeomTreeNode &n = old_nodes[i];
    tm[i] = n.tm;
    wtm[i] = n.wtm;
    parentId[i] = (n.parent && n.parent != &n) ? Index16(n.parent.get() - old_nodes.data()) : Index16();

    nameOfs[i] = nm_cur_ofs;
    int nm_len = (int)strlen(n.name);
    memcpy(&data[nameOfs_ofs + nm_cur_ofs], n.name, nm_len + 1);
    nm_cur_ofs += nm_len + 1;

    if (n.child.size())
    {
      int first_child_idx = n.child.data() - old_nodes.data();
      G_ASSERTF_CONTINUE(first_child_idx >= i && first_child_idx < i + (1 << 16) && n.child.size() < (1 << 16),
        "for node[%d]: first_child_idx=%d child.count=%d", i, first_child_idx, n.child.size());
      childrenRef[i].count = n.child.size();
      childrenRef[i].relOfs = first_child_idx - i;
    }
  }

  invalidateWtm();
}


void GeomNodeTree::calcWorldBox(bbox3f &box) const
{
  v_bbox3_init_empty(box);
  for (auto &m : wtm)
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
  for (unsigned i = from_nodeid.index(); i <= lastValidWtmIndex; ++i)
    if (data[invalidTmOfs + (i >> 3)] & (1 << (i & 7)))
    {
      data[invalidTmOfs + (i >> 3)] &= ~(1 << (i & 7));

      if (p_idx != parentId[i])
      {
        p_idx = parentId[i];
        v_mat44_inverse43(p_iwtm, wtm[p_idx.index()]);
      }
      v_mat44_mul43(tm[i], p_iwtm, wtm[i]);
    }
}

void GeomNodeTree::verifyAllData() const
{
#if DAGOR_DBGLEVEL > 0
  for (const mat44f &m : wtm)
    G_ASSERTF(is_valid_tm(m), "Bad wtm[%i] " FMT_TM, int(&m - wtm.data()), VTMD(m));
  for (const mat44f &m : tm)
    G_ASSERTF(is_valid_tm(m), "Bad tm[%i] " FMT_TM, int(&m - tm.data()), VTMD(m));
  G_ASSERTF(is_valid_pos(wofs), "Bad wofs " FMT_P3, V3D(wofs));
#endif
}

void GeomNodeTree::verifyOnlyTmFast() const
{
#if DAGOR_DBGLEVEL > 0
  vec3f res = v_zero();
  for (mat44f m : tm)
    res = v_add(res, m.col3);
  if (!is_valid_pos(v_safediv(res, v_splats(tm.size()))))
  {
    debug("GeomNodeTree nodes tm dump:");
    for (Index16 i(0), ie(nodeCount()); i != ie; ++i)
      debug("\t%s " FMT_TM, getNodeName(i), VTMD(tm[i.index()]));
    logerr("Invalid tm in GeomNodeTree. Nodes tm dumped to log.");
  }
#endif
}
