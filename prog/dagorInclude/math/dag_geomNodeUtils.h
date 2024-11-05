//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_geomTree.h>
#include <debug/dag_assert.h>

static inline void compute_tm_from_wtm(GeomNodeTree &g, dag::Index16 node0)
{
  if (auto p_idx = g.getParentNodeIdx(node0))
  {
    mat44f p_iwtm;
    v_mat44_inverse43(p_iwtm, g.getNodeWtmRel(p_idx));
    v_mat44_mul43(g.getNodeTm(node0), p_iwtm, g.getNodeWtmRel(node0));
  }
  else
    g.getNodeTm(node0) = g.getNodeWtmRel(node0);
}

static inline void align_node_along(GeomNodeTree &g, dag::Index16 tnode, dag::Index16 node0, dag::Index16 node1)
{
  G_ASSERTF(g.isIndexValid(tnode) && g.isIndexValid(node0) && g.isIndexValid(node1), "tnode=%d node=%d node1=%d nodeCount=%d",
    (int)tnode, (int)node0, (int)node1, g.nodeCount());
  mat44f &n_wtm = g.getNodeWtmRel(tnode);
  mat44f &n_tm = g.getNodeTm(tnode);
  mat44f &n0_wtm = g.getNodeWtmRel(node0);
  mat44f &n1_wtm = g.getNodeWtmRel(node1);

  auto pnode = g.getParentNodeIdx(tnode);
  mat44f &pn_wtm = g.getNodeWtmRel(pnode);
  v_mat44_mul43(n_wtm, pn_wtm, n_tm);

  quat4f q = v_quat_from_arc(v_sub(n_wtm.col3, pn_wtm.col3), v_sub(n1_wtm.col3, n0_wtm.col3));
  mat44f pm;
  pm.col0 = v_quat_mul_vec3(q, pn_wtm.col0);
  pm.col1 = v_quat_mul_vec3(q, pn_wtm.col1);
  pm.col2 = v_quat_mul_vec3(q, pn_wtm.col2);
  pm.col3 = pn_wtm.col3;
  if (pnode == node0)
  {
    v_mat44_mul43(n_wtm, pm, n_tm);
    q = v_quat_from_arc(n_wtm.col0, n1_wtm.col0);
    pm.col0 = v_quat_mul_vec3(q, n_wtm.col0);
    pm.col1 = v_quat_mul_vec3(q, n_wtm.col1);
    pm.col2 = v_quat_mul_vec3(q, n_wtm.col2);
    n_wtm.col0 = pm.col0;
    n_wtm.col1 = pm.col1;
    n_wtm.col2 = pm.col2;
    compute_tm_from_wtm(g, tnode);
  }
  else
  {
    pn_wtm = pm;
    compute_tm_from_wtm(g, pnode);
    v_mat44_mul43(n_wtm, pn_wtm, n_tm);
  }
}

template <class T>
void gather_children_nodes_indices(const GeomNodeTree &tree, dag::Index16 node, T &out)
{
  out.insert(node);
  for (unsigned i = 0, ie = tree.getChildCount(node); i < ie; ++i)
    gather_children_nodes_indices(tree, tree.getChildNodeIdx(node, i), out);
}

inline const mat44f *get_node_tm_ptr(const GeomNodeTree &t, dag::Index16 n) { return n ? &t.getNodeTm(n) : nullptr; }
inline mat44f *get_node_tm_ptr(GeomNodeTree &t, dag::Index16 n) { return n ? &t.getNodeTm(n) : nullptr; }
inline const mat44f *get_node_wtm_rel_ptr(const GeomNodeTree &t, dag::Index16 n) { return n ? &t.getNodeWtmRel(n) : nullptr; }
inline mat44f *get_node_wtm_rel_ptr(GeomNodeTree &t, dag::Index16 n) { return n ? &t.getNodeWtmRel(n) : nullptr; }

static inline mat44f *resolve_node_wtm(GeomNodeTree &tree, const char *nodeName, mat44f const **opt_parent_wtm = nullptr,
  mat44f **opt_tm = nullptr)
{
  auto node_idx = tree.findINodeIndex(nodeName);
  if (opt_parent_wtm)
  {
    auto pnode_idx = node_idx ? tree.getParentNodeIdx(node_idx) : dag::Index16();
    *opt_parent_wtm = get_node_wtm_rel_ptr(tree, pnode_idx);
  }
  if (opt_tm)
    *opt_tm = get_node_tm_ptr(tree, node_idx);
  return get_node_wtm_rel_ptr(tree, node_idx);
}

static inline const mat44f *resolve_node_wtm(const GeomNodeTree &tree, const char *nodeName, mat44f const **opt_parent_wtm = nullptr,
  mat44f const **opt_tm = nullptr)
{
  auto node_idx = tree.findINodeIndex(nodeName);
  if (opt_parent_wtm)
  {
    auto pnode_idx = node_idx ? tree.getParentNodeIdx(node_idx) : dag::Index16();
    *opt_parent_wtm = get_node_wtm_rel_ptr(tree, pnode_idx);
  }
  if (opt_tm)
    *opt_tm = get_node_tm_ptr(tree, node_idx);
  return get_node_wtm_rel_ptr(tree, node_idx);
}
