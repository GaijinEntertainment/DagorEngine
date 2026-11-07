// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gameLibs/daCSG/graph_api.h>
#include <gameLibs/daCSG/solid_api.h>
#include <dag/dag_vector.h>
#include <math/dag_check_nan.h>
#include <util/dag_stdint.h>
#include <EASTL/string.h>
#include <float.h>
#include <math.h>

namespace dacsg
{
constexpr uint32_t MAX_GRAPH_DEPTH = 100000;
// spec.md 11: every entry of a transform, and of the composed ones; a box, a reach or a scale that transforms take past
// 2^FAR_EXPONENT is unknown or an error, so none of them overflows
constexpr double TM_LIMIT = 0x1p300;
constexpr int FAR_EXPONENT = 500;
constexpr int NOT_FINITE = 4096;

struct Walk
{
  dag::Vector<uint32_t> order;   // reachable nodes, children first
  dag::Vector<uint32_t> parents; // by node: edges from reachable nodes
  dag::Vector<uint32_t> parent;  // by node: the last of them
};

bool walk(const Graph &g, OpResult root, Walk &w, eastl::string &err);
bool is_op_kind(NodeKind k);
uint32_t node_child_count(NodeKind k);
bool is_convex_prim(PrimType t);
// its mesh can fold through itself: an import, or a primitive that follows a curve
bool prim_can_fold(PrimType t);
// 2^(e - 1) <= |x| < 2^e for x, or for the largest |component| of the points; 0 for zero, and NOT_FINITE for a value that
// is not finite, so a sum with any other exponent stays past the double range
int exponent_of(double x);
int exponent_of(const DPoint3 *p, int count);
inline bool in_tm_limit(double x) { return check_finite(x) && fabs(x) <= TM_LIMIT; }
// finite, with every entry within TM_LIMIT
bool tm_in_range(const TMatrixD &m);
// r = outer after inner, which r may alias; false, r unchanged: an operand or the product is not tm_in_range
bool compose_tm(const TMatrixD &outer, const TMatrixD &inner, TMatrixD &r);
// out grows by the corners of box under m; false, out unchanged: a corner could pass 2^FAR_EXPONENT
bool transform_bbox(const TMatrixD &m, const BBoxD &box, BBoxD &out);
// r = a * b; false, r unchanged: the product could pass 2^FAR_EXPONENT
bool bounded_product(double a, double b, double &r);
// false, r unchanged: x is past FLT_MAX
inline bool narrow_float(double x, float &r)
{
  if (!check_finite(x) || fabs(x) > FLT_MAX)
    return false;
  r = float(x);
  return true;
}
// inner becomes outer after inner, both finite; false, inner unchanged: an entry of the product would pass the float range
bool compose_uv_affine(const float outer[2][3], float inner[2][3]);
bool uv_affine_finite(const float m[2][3]);
// the source's local bbox and reach; false, both unchanged: either is not finite
bool leaf_bounds(const ILeafSource &source, BBoxD &box, double &reach);
// the value checks of a transform, a cut plane, an attribute and a leaf's bounds, spec.md 11: false with err naming the node;
// a leaf's bounds, read once, go to box and reach as leaf_bounds gives them
bool check_node_values(const Graph &g, OpResult r, BBoxD &box, double &reach, eastl::string &err);

inline DPoint3 transform_point(const TMatrixD &m, const DPoint3 &p)
{
  return m.col[0] * p.x + m.col[1] * p.y + m.col[2] * p.z + m.col[3];
}
} // namespace dacsg
