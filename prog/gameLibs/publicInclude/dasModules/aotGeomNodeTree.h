//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>
#include <math/dag_geomTree.h>
#include <math/dag_geomNodeUtils.h>
#include <dasModules/aotDagorMath.h>

MAKE_TYPE_FACTORY(GeomNodeTree, GeomNodeTree);

namespace bind_dascript
{
inline void check_geomtree_out_of_bounds(const GeomNodeTree &geom_node_tree, int node_id, das::Context *context,
  das::LineInfoArg *line_info)
{
  const uint32_t nodeCount = geom_node_tree.nodeCount();
  if (EASTL_UNLIKELY(uint32_t(node_id) >= nodeCount))
    context->throw_error_at(line_info, "Node index out of range, %d of %d", node_id, nodeCount);
}

inline const mat44f &geomtree_get_node_tm_c(const GeomNodeTree &geom_node_tree, int node_idx)
{
  return geom_node_tree.getNodeTm(dag::Index16(node_idx));
}
inline mat44f &geomtree_get_node_tm(GeomNodeTree &geom_node_tree, int node_idx)
{
  return geom_node_tree.getNodeTm(dag::Index16(node_idx));
}

inline void geomtree_recalc_tm(GeomNodeTree &geom_node_tree, int node_idx, bool recalc_root)
{
  return geom_node_tree.recalcTm(dag::Index16(node_idx), recalc_root);
}

inline const mat44f &geomtree_get_node_wtm_rel_c(const GeomNodeTree &geom_node_tree, int node_idx)
{
  return geom_node_tree.getNodeWtmRel(dag::Index16(node_idx));
}
inline mat44f &geomtree_get_node_wtm_rel(GeomNodeTree &geom_node_tree, int node_idx)
{
  return geom_node_tree.getNodeWtmRel(dag::Index16(node_idx));
}

inline void geomtree_get_node_tm_scalar(const GeomNodeTree &geom_node_tree, int node_idx, das::float3x4 &out_tm)
{
  geom_node_tree.getNodeTmScalar(dag::Index16(node_idx), reinterpret_cast<TMatrix &>(out_tm));
}

inline void geomtree_set_node_wtm_rel_scalar(GeomNodeTree &geom_node_tree, int node_idx, const das::float3x4 &in_wtm)
{
  geom_node_tree.setNodeWtmRelScalar(dag::Index16(node_idx), reinterpret_cast<const TMatrix &>(in_wtm));
}

inline void geomtree_set_node_tm_scalar(GeomNodeTree &geom_node_tree, int node_idx, const das::float3x4 &in_tm)
{
  geom_node_tree.setNodeTmScalar(dag::Index16(node_idx), reinterpret_cast<const TMatrix &>(in_tm));
}

inline int geomtree_find_node_index(const GeomNodeTree &geom_node_tree, const char *node_name)
{
  return (int)geom_node_tree.findNodeIndex(node_name ? node_name : "");
}

inline das::float3 geomtree_get_node_wpos(const GeomNodeTree &geom_node_tree, int node_id, das::Context *context,
  das::LineInfoArg *line_info)
{
  check_geomtree_out_of_bounds(geom_node_tree, node_id, context, line_info);
  return geom_node_tree.getNodeWpos(dag::Index16(node_id));
}

inline void geomtree_get_node_wtm(const GeomNodeTree &geom_node_tree, int node_id, das::float3x4 &out_wtm, das::Context *context,
  das::LineInfoArg *line_info)
{
  check_geomtree_out_of_bounds(geom_node_tree, node_id, context, line_info);
  geom_node_tree.getNodeWtmScalar(dag::Index16(node_id), reinterpret_cast<TMatrix &>(out_wtm));
}

inline void geomtree_get_node_rel_wtm(const GeomNodeTree &geom_node_tree, int node_id, das::float3x4 &out_wtm, das::Context *context,
  das::LineInfoArg *line_info)
{
  check_geomtree_out_of_bounds(geom_node_tree, node_id, context, line_info);
  geom_node_tree.getNodeWtmRelScalar(dag::Index16(node_id), reinterpret_cast<TMatrix &>(out_wtm));
}

inline void geomtree_set_node_wtm(GeomNodeTree &geom_node_tree, int node_id, const das::float3x4 &in_wtm, das::Context *context,
  das::LineInfoArg *line_info)
{
  check_geomtree_out_of_bounds(geom_node_tree, node_id, context, line_info);
  geom_node_tree.setNodeWtmScalar(dag::Index16(node_id), reinterpret_cast<const TMatrix &>(in_wtm));
}

inline das::float3 geomtree_get_wtm_ofs(const GeomNodeTree &geom_node_tree) { return geom_node_tree.getWtmOfs(); }

inline das::float3 geomtree_get_node_wpos_rel(const GeomNodeTree &geom_node_tree, int node_id, das::Context *context,
  das::LineInfoArg *line_info)
{
  check_geomtree_out_of_bounds(geom_node_tree, node_id, context, line_info);
  return geom_node_tree.getNodeWposRel(dag::Index16(node_id));
}

inline das::float3 geomtree_calc_optimal_wofs(das::float3 pos) { return GeomNodeTree::calc_optimal_wofs(pos); }

inline void geomtree_change_root_pos(GeomNodeTree &geom_node_tree, das::float3 pos, bool setup_wofs)
{
  geom_node_tree.changeRootPos(pos, setup_wofs);
}

inline void geomtree_invalidateWtm(GeomNodeTree &t, int i) { t.invalidateWtm(dag::Index16(i)); }
inline void geomtree_invalidateWtm0(GeomNodeTree &t) { t.invalidateWtm(); }
inline void geomtree_markNodeTmInvalid(GeomNodeTree &t, int i) { t.markNodeTmInvalid(dag::Index16(i)); }
inline void geomtree_validateTm(GeomNodeTree &t, int i) { t.validateTm(dag::Index16(i)); }
inline const char *geomtree_getNodeName(const GeomNodeTree &t, int i) { return t.getNodeName(dag::Index16(i)); }
inline int geomtree_getParentNodeIdx(const GeomNodeTree &t, int i) { return (int)t.getParentNodeIdx(dag::Index16(i)); }
inline uint32_t geomtree_getChildCount(const GeomNodeTree &t, int i) { return t.getChildCount(dag::Index16(i)); }
inline int geomtree_getChildNodeIdx(const GeomNodeTree &t, int i, unsigned o) { return (int)t.getChildNodeIdx(dag::Index16(i), o); }
inline const mat44f *geomtree_get_node_wtm_rel_ptr(const GeomNodeTree &t, int n) { return get_node_wtm_rel_ptr(t, dag::Index16(n)); }
inline void geomtree_calcWtmForBranch(GeomNodeTree &t, int i) { return t.calcWtmForBranch(dag::Index16(i)); }

} // namespace bind_dascript
