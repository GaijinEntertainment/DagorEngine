//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_smallTab.h>
#include <generic/dag_patchTab.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_vecMathCompatibility.h>
#include <math/dag_TMatrix.h>
#include <math/dag_Point4.h>
#include <util/dag_index16.h>
#include <util/dag_baseDef.h> // for DestroyDeleter
#include <EASTL/unique_ptr.h>


class IGenSave;
class IGenLoad;
class BBox3;


class GeomNodeTree
{
public:
  typedef dag::Index16 Index16;

  GeomNodeTree() = default;
  GeomNodeTree(const GeomNodeTree &from) { *this = from; }
  ~GeomNodeTree();

  GeomNodeTree &operator=(const GeomNodeTree &from);

  // make()/load() return one block holding the instance and its data, released by destroy()
  static GeomNodeTree *make(const GeomNodeTree &from);
  static GeomNodeTree *load(IGenLoad &cb);
  void destroy();

  // instances live only in co-allocated make()/load() blocks freed by destroy(); no raw heap new/delete
  static void *operator new(size_t) = delete;
  static void *operator new[](size_t) = delete;
  static void operator delete(void *) = delete;
  static void operator delete[](void *) = delete;

  bool empty() const { return !numNodes; }
  unsigned nodeCount() const { return numNodes; }
  unsigned importantNodeCount() const { return nodeCount() - lastUnimportantCount; }
  bool isIndexValid(Index16 ni) const { return ni.index() < nodeCount(); }

  inline void markNodeTmInvalid(Index16 ni) { data[invalidTmOfs + (ni.index() >> 3)] |= 1 << (ni.index() & 7); }

  Index16 getParentNodeIdx(Index16 ni) const { return ni ? parentId()[ni.index()] : Index16(); }

  uint32_t getChildCount(Index16 ni) const { return ni ? childrenRef()[ni.index()].count : 0; }

  Index16 getChildNodeIdx(Index16 ni, uint32_t child) const
  {
    return ni ? Index16(ni.index() + childrenRef()[ni.index()].relOfs + child) : Index16();
  }

  const char *getNodeName(Index16 ni) const { return ni ? nameOfs()[ni.index()] + (const char *)nameOfs().data() : nullptr; }

  Index16 findNodeIndex(const char *name) const;
  Index16 findINodeIndex(const char *name) const;

  // Monotonic pose stamp: every wtm update entry and named world-state mutator (wtm/wofs
  // setters) bumps it, so consumers caching pose-derived data (collision instances, TLAS
  // feeders) can cheaply detect a change. Never 0. Local-tm writers do not bump (calcWtm
  // does); direct wtm writers through the non-const accessors must call markPoseChanged().
  uint32_t getPoseGeneration() const { return poseGeneration; }
  void markPoseChanged()
  {
    if (DAGOR_UNLIKELY(++poseGeneration == 0))
      poseGeneration = 1;
  }

  void invalidateWtm() { lastValidWtmIndex = -1; }
  void invalidateWtm(Index16 ni)
  {
    if (!ni)
      lastValidWtmIndex = -1;
    else if (ni.index() < lastValidWtmIndex)
      lastValidWtmIndex = ni.index();
  }

  void calcWtm();
  void partialCalcWtm(Index16 upto_nodeid);

  void validateTm(Index16 from_nodeid = Index16(0));
  void verifyAllData() const;
  void verifyOnlyTmFast() const;

  void calcWorldBox(bbox3f &box) const;
  void calcWorldBox(BBox3 &box) const;

  void recalcTm(Index16 ni, bool allow_recalc_root = true)
  {
    if (auto p_idx = getParentNodeIdx(ni))
    {
      mat44f p_iwtm;
      v_mat44_inverse43(p_iwtm, wtm()[p_idx.index()]);
      v_mat44_mul43(tm()[ni.index()], p_iwtm, wtm()[ni.index()]);
    }
    else if (allow_recalc_root)
      tm()[ni.index()] = wtm()[ni.index()];
  }

  void calcWtmForBranch(Index16 start_idx)
  {
    markPoseChanged();
    calcWtmForBranchRec(start_idx);
  }
  void calcWtmForBranchRec(Index16 start_idx)
  {
    const mat44f &n_wtm = wtm()[start_idx.index()];
    auto cidx = getChildNodeIdx(start_idx, 0);
    auto cnum = getChildCount(start_idx);
    mat44f *wtmPtr = wtm().data(), *tmPtr = tm().data();
    for (unsigned c = cidx.index(), ce = c + cnum; c < ce; c++)
      v_mat44_mul43(wtmPtr[c], n_wtm, tmPtr[c]);
    for (Index16 i = cidx, ie(i.index() + cnum); i != ie; ++i)
      calcWtmForBranchRec(i);
  }

  int getLastValidWtmIndex() const { return lastValidWtmIndex; }
  bool isWtmValid(bool all) const { return lastValidWtmIndex + 1 >= int(all ? nodeCount() : importantNodeCount()); }

  vec3f translateToZero()
  {
    mat44f *n_wtm = wtm().data();
    if (!n_wtm)
      return v_zero();

    markPoseChanged();
    mat44f *n_tm = tm().data();
    vec3f w_trans = n_tm[0].col3;
    n_tm[0].col3 = n_wtm->col3 = v_zero();
    mat44f *n_last = n_wtm + lastValidWtmIndex;
    for (n_wtm++; n_wtm <= n_last; n_wtm++)
      n_wtm->col3 = v_sub(n_wtm->col3, w_trans);
    return w_trans;
  }
  void translate(vec3f add)
  {
    mat44f *n_wtm = wtm().data();
    if (!n_wtm)
      return;

    markPoseChanged();
    mat44f *n_tm = tm().data();
    n_tm[0].col3 = n_wtm->col3 = v_add(n_tm[0].col3, add);
    mat44f *n_last = n_wtm + lastValidWtmIndex;
    for (n_wtm++; n_wtm <= n_last; n_wtm++)
      n_wtm->col3 = v_add(n_wtm->col3, add);
  }

  const mat44f &getNodeTm(Index16 ni) const { return tm()[ni.index()]; }
  mat44f &getNodeTm(Index16 ni) { return tm()[ni.index()]; }
  void getNodeTmScalar(Index16 ni, TMatrix &out_tm) const { mat44f_to_TMatrix(getNodeTm(ni), out_tm); }
  void setNodeTmScalar(Index16 ni, const TMatrix &m)
  {
    mat44f &n_tm = getNodeTm(ni);
    TMatrix_to_mat44f(m, n_tm);
    G_ASSERTF(is_valid_pos(n_tm.col3), "setNodeTmScalar: invalid col3 " FMT_P3, V3D(n_tm.col3));
  }
  Point3 getNodeLposScalar(Index16 ni) const { return as_point3(&tm()[ni.index()].col3); }

  const mat44f &getNodeWtmRel(Index16 ni) const { return wtm()[ni.index()]; }
  mat44f &getNodeWtmRel(Index16 ni) { return wtm()[ni.index()]; }
  void getNodeWtmRelScalar(Index16 ni, TMatrix &out_wtm) const { mat44f_to_TMatrix(getNodeWtmRel(ni), out_wtm); }

  void getNodeWtm(Index16 ni, mat44f &out_wtm) const
  {
    const mat44f &rel_wtm = getNodeWtmRel(ni);
    out_wtm.col0 = rel_wtm.col0;
    out_wtm.col1 = rel_wtm.col1;
    out_wtm.col2 = rel_wtm.col2;
    out_wtm.col3 = v_add(rel_wtm.col3, wofs);
  }
  void getNodeWtmScalar(Index16 ni, TMatrix &out_wtm) const
  {
    mat44f n_wtm;
    getNodeWtm(ni, n_wtm);
    v_mat_43cu_from_mat44(out_wtm.array, n_wtm);
  }

  void setNodeWtm(Index16 ni, const mat44f &m)
  {
    markPoseChanged();
    mat44f &n_wtm = getNodeWtmRel(ni);
    n_wtm.col0 = m.col0;
    n_wtm.col1 = m.col1;
    n_wtm.col2 = m.col2;
    n_wtm.col3 = v_sub(m.col3, wofs);
    G_ASSERTF(is_valid_pos(n_wtm.col3), "setNodeWtm: invalid col3 " FMT_P3, V3D(n_wtm.col3));
  }
  void setNodeWtmScalar(Index16 ni, const TMatrix &m)
  {
    mat44f n_tm;
    v_mat44_make_from_43cu(n_tm, m.array);
    setNodeWtm(ni, n_tm);
  }
  void setNodeWtmRelScalar(Index16 ni, const TMatrix &m)
  {
    markPoseChanged();
    mat44f &n_wtm = getNodeWtmRel(ni);
    TMatrix_to_mat44f(m, n_wtm);
    G_ASSERTF(is_valid_pos(n_wtm.col3), "setNodeWtmRelScalar: invalid col3 " FMT_P3, V3D(n_wtm.col3));
  }

  vec3f nodeWtmMulVec3p(Index16 ni, vec3f p) const { return v_add(v_mat44_mul_vec3p(getNodeWtmRel(ni), p), wofs); }

  vec3f getNodeWposRel(Index16 ni) const { return wtm()[ni.index()].col3; }
  vec3f getNodeWpos(Index16 ni) const { return v_add(wtm()[ni.index()].col3, wofs); }
  Point3 getNodeWposRelScalar(Index16 ni) const { return as_point3(&wtm()[ni.index()].col3); }
  Point3 getNodeWposScalar(Index16 ni) const
  {
    Point3_vec4 ret;
    v_st(&ret.x, v_add(wtm()[ni.index()].col3, wofs));
    return ret;
  }

  void setNodeWpos(Index16 ni, vec3f wpos)
  {
    markPoseChanged();
    wtm()[ni.index()].col3 = v_sub(wpos, wofs);
    G_ASSERTF(is_valid_pos(wtm()[ni.index()].col3), "setNodeWpos: invalid col3 " FMT_P3, V3D(wtm()[ni.index()].col3));
  }

  void setWtmOfs(vec3f _wofs)
  {
    markPoseChanged(); // wofs shifts every node's effective world position
    wofs = _wofs;
    G_ASSERTF(is_valid_pos(_wofs), "setWtmOfs: invalid wofs " FMT_P3, V3D(_wofs));
  }
  vec3f getWtmOfs() const { return wofs; }
  const vec3f *getWtmOfsPtr() const { return &wofs; } //< tree address is stable (see make()), so this may be cached

  const mat44f &getRootTm() const { return tm()[0]; }
  mat44f &getRootTm() { return tm()[0]; }
  const mat44f &getRootWtmRel() const { return wtm()[0]; }
  mat44f &getRootWtmRel() { return wtm()[0]; }

  void setRootTmScalar(const TMatrix &m) { setNodeTmScalar(Index16(0), m); }
  void getRootTmScalar(TMatrix &out_tm) const { getNodeTmScalar(Index16(0), out_tm); }
  Point3 getRootLposScalar() const { return as_point3(&tm()[0].col3); }

  void setRootWtmRelScalar(const TMatrix &m) { setNodeWtmRelScalar(Index16(0), m); }
  void getRootWtmRelScalar(TMatrix &out_wtm) const { getNodeWtmRelScalar(Index16(0), out_wtm); }
  void getRootWtmScalar(TMatrix &out_wtm) const { getNodeWtmScalar(Index16(0), out_wtm); }
  Point3 getRootWposRelScalar() const { return as_point3(&wtm()[0].col3); }
  Point3 getRootWposScalar() const { return getNodeWposScalar(Index16(0)); }

  void setRootTm(const mat44f &m, bool setup_wofs = false)
  {
    if (empty())
      return;
    mat44f *n_tm = tm().data();
    n_tm[0] = m;
    if (!setup_wofs)
      setWtmOfs(v_zero());
    else
    {
      setWtmOfs(calc_optimal_wofs(m.col3));
      n_tm[0].col3 = v_sub(m.col3, wofs);
    }
    wtm()[0] = n_tm[0];
    G_ASSERTF(is_valid_tm(m), "setRootTm: invalid tm " FMT_TM, VTMD(m));
  }
  void setRootTm(const mat44f &rel_m, vec3f w_ofs)
  {
    if (empty())
      return;
    setWtmOfs(w_ofs);
    wtm()[0] = tm()[0] = rel_m;
    G_ASSERTF(is_valid_tm(rel_m), "setRootTm: invalid tm " FMT_TM, VTMD(rel_m));
  }

  void changeRootPos(vec3f p, bool setup_wofs = false)
  {
    if (empty())
      return;
    setWtmOfs(setup_wofs ? calc_optimal_wofs(p) : v_zero());
    mat44f &n_tm = tm()[0];
    n_tm.col3 = wtm()[0].col3 = v_sub(p, wofs);
    G_ASSERTF(is_valid_pos(n_tm.col3), "changeRootPos: invalid pos " FMT_P3, V3D(n_tm.col3));
  }

  static vec3f calc_optimal_wofs(vec3f pos)
  {
    // use wofs = (m.col3 round 8) to provide optimal accuracy
    return v_and(v_mul(v_round(v_mul(pos, v_splats(1.f / 8.f))), v_splats(8.f)), v_cast_vec4f(V_CI_MASK1110));
  }

  static Point3 calc_optimal_wofs(const Point3 &pos)
  {
    vec3f pos_v = v_ldu(&pos.x);
    vec3f res_v = GeomNodeTree::calc_optimal_wofs(pos_v);

    Point3_vec4 res;
    v_st(&res.x, res_v);
    return Point3::xyz(res);
  }

  static void mat44f_to_TMatrix(const mat44f &m, TMatrix &out_m) { v_mat_43cu_from_mat44(out_m.array, m); }
  static void TMatrix_to_mat44f(const TMatrix &m, mat44f &out_m) { v_mat44_make_from_43cu(out_m, m.array); }
  static bool is_valid_pos(vec3f pos, float valid_world_rad = 1e6f)
  {
    return !v_test_xyz_nan(pos) && v_check_xyz_all_true(v_cmp_lt(v_abs(pos), v_splats(valid_world_rad)));
  }
  static bool is_valid_tm(mat44f m) { return is_valid_pos(v_add(v_add(m.col0, m.col1), v_add(m.col2, m.col3)), 4 * 1e6f); }

protected:
  struct ChildRef
  {
    uint16_t count, relOfs;
  };

  dag::Span<mat44f> wtm() { return {(mat44f *)data.data(), numNodes}; }
  dag::ConstSpan<mat44f> wtm() const { return {(const mat44f *)data.data(), numNodes}; }
  dag::Span<mat44f> tm() { return {(mat44f *)(data.data() + tmByteOfs()), numNodes}; }
  dag::ConstSpan<mat44f> tm() const { return {(const mat44f *)(data.data() + tmByteOfs()), numNodes}; }
  dag::Span<Index16> parentId() { return {(Index16 *)(data.data() + parentIdByteOfs()), numNodes}; }
  dag::ConstSpan<Index16> parentId() const { return {(const Index16 *)(data.data() + parentIdByteOfs()), numNodes}; }
  dag::Span<ChildRef> childrenRef() { return {(ChildRef *)(data.data() + childrenRefByteOfs()), numNodes}; }
  dag::ConstSpan<ChildRef> childrenRef() const { return {(const ChildRef *)(data.data() + childrenRefByteOfs()), numNodes}; }
  dag::Span<uint16_t> nameOfs() { return {(uint16_t *)(data.data() + nameOfsByteOfs()), numNodes}; }
  dag::ConstSpan<uint16_t> nameOfs() const { return {(const uint16_t *)(data.data() + nameOfsByteOfs()), numNodes}; }

  size_t tmByteOfs() const { return sizeof(mat44f) * numNodes; }
  size_t parentIdByteOfs() const { return 2 * sizeof(mat44f) * numNodes; }
  size_t childrenRefByteOfs() const { return parentIdByteOfs() + sizeof(Index16) * numNodes; }
  size_t nameOfsByteOfs() const { return childrenRefByteOfs() + sizeof(ChildRef) * numNodes; }

  vec4f wofs = {};          //< world offset for matrices (accuracy of wtm and wtm+wofs positions)
  uint16_t numNodes = 0;    //< node indices are Index16, so count fits uint16 (kept unpacked, no mask)
  bool dataCoalloc = false; //< data is the tail of this very block, see make()
  int lastValidWtmIndex = -1;
  unsigned invalidTmOfs : 20 = 0, lastUnimportantCount : 12 = 0;
  uint32_t poseGeneration = 1; // see getPoseGeneration; never 0

private:
  void copyDataFrom(const GeomNodeTree &from); //< data must be already sized for `from`
  static GeomNodeTree *allocWithData(intptr_t data_sz);

  dag::Span<char> data;

  friend class GeomNodeTreeBuilder;
};

static_assert(sizeof(GeomNodeTree) % 16 == 0); // make() puts data right after us and it starts with mat44f

using GeomNodeTreeUniquePtr = eastl::unique_ptr<GeomNodeTree, DestroyDeleter<GeomNodeTree>>;
