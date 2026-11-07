// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "graph/graphUtil.h"
#include "graph/optimize.h"
#include "mesh/solidUtil.h"
#include <gameLibs/daCSG/leaf_api.h>
#include <math/dag_check_nan.h>
#include <EASTL/algorithm.h>
#include <EASTL/sort.h>
#include <math.h>

namespace dacsg
{
// covers the float rounding of tessellated positions
static constexpr double PRIM_BOX_MARGIN = 1e-6;
static constexpr uint32_t MORTON_MAX = (1u << 21) - 1;

bool walk(const Graph &g, OpResult root, Walk &w, eastl::string &err)
{
  const uint32_t count = g.nodeCount();
  w.order.clear();
  w.parents.assign(count, 0);
  w.parent.assign(count, NO_INDEX);
  dag::Vector<uint8_t> seen(count, 0);
  struct Item
  {
    uint32_t node, next;
  };
  dag::Vector<Item> stack;
  stack.push_back(Item{root.id, 0});
  seen[root.id] = 1;
  while (!stack.empty())
  {
    const Item it = stack.back();
    const Node &n = g.node(OpResult{it.node});
    if (it.next < node_child_count(n.kind))
    {
      const uint32_t c = (it.next ? n.b : n.a).id;
      ++stack.back().next;
      ++w.parents[c];
      w.parent[c] = it.node;
      if (!seen[c])
      {
        if (stack.size() >= MAX_GRAPH_DEPTH)
        {
          err.sprintf("the graph is deeper than %u nodes", MAX_GRAPH_DEPTH);
          return false;
        }
        seen[c] = 1;
        stack.push_back(Item{c, 0});
      }
      continue;
    }
    w.order.push_back(it.node);
    stack.pop_back();
  }
  return true;
}

// analytic primitives are centered at the origin, spec.md 4; a size the primitive refuses keeps the box unknown, so no
// prune hides the error (a relation between sizes stays the primitive's own check)
static bool half_extent(const PrimDesc &d, DPoint3 &half)
{
  const double a = d.p[0], b = d.p[1], c = d.p[2];
  switch (d.type)
  {
    case PrimType::Box:
    case PrimType::RBox: half = DPoint3(a, b, c) * 0.5; return a > 0 && b > 0 && c > 0;
    case PrimType::Sphere:
    case PrimType::IcoSphere:
    case PrimType::CubeSphere: half = DPoint3(a, a, a); return a > 0;
    case PrimType::Ellipsoid: half = DPoint3(a, b, c); return a > 0 && b > 0 && c > 0;
    case PrimType::Cylinder:
    case PrimType::CCyl: half = DPoint3(a, 0.5 * b, a); return a > 0 && b > 0;
    case PrimType::Capsule: half = DPoint3(a, max(0.5 * b, a), a); return a > 0 && b > 0;
    case PrimType::Cone: half = DPoint3(max(a, b), 0.5 * c, max(a, b)); return a >= 0 && b >= 0 && max(a, b) > 0 && c > 0;
    case PrimType::Torus: half = DPoint3(a + b, b, a + b); return a > 0 && b > 0;
    default: return false;
  }
}

bool prim_bbox(const Graph &g, const PrimDesc &d, BBoxD &box, double &reach)
{
  box = BBoxD();
  reach = 0;
  // a parameter that is not finite is the prim's own error: an unknown box keeps it from a prune
  if (!eastl::all_of(eastl::begin(d.p), eastl::end(d.p), [](float x) { return check_finite(x); }) || !check_finite(d.angleDeg))
    return false;
  const double p0 = fabs(d.p[0]);
  DPoint3 half;
  if (half_extent(d, half))
  {
    box += half;
    box += -half;
  }
  else if (d.type == PrimType::Convex || d.type == PrimType::Tube || d.type == PrimType::Loft)
  {
    const DPoint3 r = d.type == PrimType::Tube ? DPoint3(p0, p0, p0) : DPoint3(0, 0, 0);
    // a coordinate that is not a number drops out of the box's comparisons, so it is checked before
    for (const Point3 &q : g.points3(d.points))
    {
      if (!check_finite(q.x) || !check_finite(q.y) || !check_finite(q.z))
        return false;
      box += DPoint3(q) + r;
      box += DPoint3(q) - r;
    }
  }
  else if (d.type == PrimType::Extrude)
    for (const Point2 &q : g.points2(d.points))
    {
      if (!check_finite(q.x) || !check_finite(q.y))
        return false;
      box += DPoint3(q.x, 0.5 * p0, q.y);
      box += DPoint3(q.x, -0.5 * p0, q.y);
    }
  else if (d.type == PrimType::Revolve)
    for (const Point2 &q : g.points2(d.points))
    {
      if (!check_finite(q.x) || !check_finite(q.y))
        return false;
      const double r = fabs(q.x);
      box += DPoint3(r, q.y, r);
      box += DPoint3(-r, q.y, -r);
    }
  else if (d.type == PrimType::Leaf)
  {
    // a leaf's mesh may pass its bbox by reach: a flat skin is its thickness thick
    if (!leaf_bounds(*g.leafSource(d.path), box, reach))
      return false;
    box.lim[0] -= DPoint3(reach, reach, reach);
    box.lim[1] += DPoint3(reach, reach, reach);
  }
  // a zero extent (a zero size) or none (no points) is a prim the parameter check must reject, so its box stays unknown
  // and no prune hides the error
  if (box.lim[0].x >= box.lim[1].x || box.lim[0].y >= box.lim[1].y || box.lim[0].z >= box.lim[1].z)
    return false;
  const double maxAbs = max_abs(box);
  const double grow = PRIM_BOX_MARGIN * maxAbs;
  const DPoint3 margin(grow, grow, grow);
  box.lim[0] -= margin;
  box.lim[1] += margin;
  return true;
}

struct NodeBox
{
  BBoxD box;
  double reach = 0; // of the leaves below under the transforms between, spec 10: a transform adds it to the box
  bool known = true;
};

double largest_scale(const TMatrixD &m)
{
  const auto row = [&](int i) { return DPoint3(m.col[0][i], m.col[1][i], m.col[2][i]); };
  double columnGram = 0, rowGram = 0;
  for (int i = 0; i < 3; ++i)
  {
    double columnSum = 0, rowSum = 0;
    for (int j = 0; j < 3; ++j)
    {
      columnSum += fabs(m.col[i] * m.col[j]);
      rowSum += fabs(row(i) * row(j));
    }
    columnGram = max(columnGram, columnSum);
    rowGram = max(rowGram, rowSum);
  }
  return sqrt(min(columnGram, rowGram));
}

// Conservative, combined as lowering combines the leaf boxes
static void node_boxes(const Graph &g, const Walk &w, dag::Vector<NodeBox> &boxes)
{
  boxes.assign(g.nodeCount(), NodeBox());
  for (uint32_t id : w.order)
  {
    const OpResult r{id};
    const Node &n = g.node(r);
    const NodeBox &a = boxes[n.a.id], &b = boxes[n.b.id];
    NodeBox &out = boxes[id];
    switch (n.kind)
    {
      case NodeKind::Prim: out.known = prim_bbox(g, g.primDesc(r), out.box, out.reach); break;
      case NodeKind::Transform:
      {
        // a transform that is not tm_in_range, or a box or reach it would take far, leaves the node unknown
        const TMatrixD &m = g.tm(r);
        out.known =
          a.known && tm_in_range(m) && transform_bbox(m, a.box, out.box) && bounded_product(a.reach, largest_scale(m), out.reach);
        if (!out.known)
          break;
        // the child's box holds its reach: along an axis the mapped box falls short of the scaled reach by what the
        // matrix row gives less than the largest scale
        for (int i = 0; i < 3; ++i)
        {
          const double more = max(0.0, out.reach - a.reach * length(DPoint3(m.col[0][i], m.col[1][i], m.col[2][i])));
          out.box.lim[0][i] -= more;
          out.box.lim[1][i] += more;
        }
        break;
      }
      case NodeKind::Union:
      case NodeKind::Concat:
        out.known = a.known && b.known;
        out.reach = max(a.reach, b.reach);
        add_bbox(out.box, a.box);
        add_bbox(out.box, b.box);
        break;
      case NodeKind::Intersect:
        if (!a.known || !b.known)
        {
          out = a.known ? a : b;
          break;
        }
        out.reach = max(a.reach, b.reach);
        out.box.lim[0] = max(a.box.lim[0], b.box.lim[0]);
        out.box.lim[1] = min(a.box.lim[1], b.box.lim[1]);
        // disjoint operands keep the first's box, which bounds an empty result, as the lowering does
        if (out.box.lim[0].x > out.box.lim[1].x || out.box.lim[0].y > out.box.lim[1].y || out.box.lim[0].z > out.box.lim[1].z)
          out.box = a.box;
        break;
      case NodeKind::Empty: break;
      default: out = a;
    }
  }
}

// One pass: rewrites the reachable nodes of src into a new graph, children first
class Pass
{
public:
  using Rewrite = OpResult (*)(Pass &p, OpResult n);

  const Graph &src;
  Graph dst;
  Walk w;
  OptimizeStats &stats;
  dag::Vector<uint32_t> map;    // by src node; NO_INDEX: taken into a parent's rewrite
  dag::Vector<uint32_t> origin; // by dst node: the src node whose rewrite made it
  dag::Vector<NodeBox> boxes;   // by src node, for a pass with a tolerance
  double tolerance = -1;        // below zero: no rule that compares boxes applies
  dag::Vector<uint8_t> mark;    // by dst node, all zero between uses
  dag::Vector<uint8_t> named;   // by src node: has an own name in the user's graph

  Pass(const Graph &g, OptimizeStats &s) : src(g), stats(s) {}
  bool run(OpResult root, Rewrite rewrite, bool with_boxes, eastl::string &err);
  OpResult to(OpResult n) const { return OpResult{map[n.id]}; }
  OpResult copy(OpResult n);
  OpResult interned(OpResult r)
  {
    if (r.id && r.id < created)
      ++stats.subtreesMerged;
    return r;
  }

private:
  uint32_t created = 0; // dst nodes before the current rewrite
};

bool Pass::run(OpResult root, Rewrite rewrite, bool with_boxes, eastl::string &err)
{
  if (!walk(src, root, w, err))
    return false;
  if (with_boxes)
    node_boxes(src, w, boxes);
  dst.setQuality(src.getQuality());
  map.assign(src.nodeCount(), NO_INDEX);
  map[0] = 0;
  origin.assign(1, 0);
  for (uint32_t id : w.order)
  {
    created = dst.nodeCount();
    const OpResult r = rewrite(*this, OpResult{id});
    if (r.id == NO_INDEX)
      continue;
    // a named node whose rewrite interns into an unnamed one lends it the name: the part label survives later folds
    if (r.id && r.id < created && named[id] && !named[origin[r.id]])
      origin[r.id] = id;
    map[id] = r.id;
    origin.resize(dst.nodeCount(), id);
  }
  return true;
}

OpResult Pass::copy(OpResult n)
{
  const Node s = src.node(n);
  switch (s.kind)
  {
    case NodeKind::Prim:
    {
      PrimDesc d = src.primDesc(n);
      if (d.type == PrimType::Leaf)
        return interned(dst.leaf(src.leafSource(d.path)));
      if (d.type == PrimType::Convex || d.type == PrimType::Tube || d.type == PrimType::Loft)
        d.points = dst.addPoints3(src.points3(d.points));
      else if (d.type == PrimType::Extrude || d.type == PrimType::Revolve)
        d.points = dst.addPoints2(src.points2(d.points));
      else if (d.type == PrimType::Mesh)
        d.path = dst.addPath(src.path(d.path));
      return interned(dst.prim(d));
    }
    case NodeKind::Cut: return interned(dst.cut(to(s.a), src.plane(n)));
    case NodeKind::Transform: return interned(dst.transform(to(s.a), src.tm(n)));
    case NodeKind::Attr: return interned(dst.attr(to(s.a), src.attrDesc(n)));
    case NodeKind::Empty: return OpResult();
    default: return interned(dst.op(s.kind, to(s.a), to(s.b)));
  }
}

static OpResult copy_node(Pass &p, OpResult n) { return p.copy(n); }

static bool is_identity(const TMatrixD &m)
{
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 3; ++j)
      if (m.col[i][j] != (i == j ? 1 : 0))
        return false;
  return true;
}

static OpResult fold_transforms(Pass &p, OpResult n)
{
  if (p.src.node(n).kind != NodeKind::Transform)
    return p.copy(n);
  OpResult a = p.to(p.src.node(n).a);
  TMatrixD tm = p.src.tm(n);
  // a named transform is a part label: it stays a node; so do two that compose past TM_LIMIT or are not finite, for the
  // input check or the lowering to name
  if (p.dst.node(a).kind == NodeKind::Transform && !p.named[p.origin[a.id]] && compose_tm(tm, p.dst.tm(a), tm))
  {
    a = p.dst.node(a).a;
    ++p.stats.transformsComposed;
  }
  if (is_identity(tm) && !p.named[n.id])
  {
    ++p.stats.transformsRemoved;
    return a;
  }
  return p.interned(p.dst.transform(a, tm));
}

static bool is_uv_affine(const Graph &g, OpResult n)
{
  return g.node(n).kind == NodeKind::Attr && g.attrDesc(n).kind == AttrKind::UvAffine && uv_affine_finite(g.attrDesc(n).uvAffine);
}

// Lowering composes consecutive affines in this order too, so the uv stays bit for bit
static OpResult merge_uv_affines(Pass &p, OpResult n)
{
  if (!is_uv_affine(p.src, n))
    return p.copy(n);
  const OpResult a = p.to(p.src.node(n).a);
  // a named inner Attr is a part label: it stays a node
  if (!is_uv_affine(p.dst, a) || p.named[p.origin[a.id]] || p.dst.attrDesc(a).onlyUnset != p.src.attrDesc(n).onlyUnset)
    return p.copy(n);
  AttrDesc d = p.dst.attrDesc(a);
  if (!compose_uv_affine(p.src.attrDesc(n).uvAffine, d.uvAffine))
    return p.copy(n);
  ++p.stats.uvAffinesMerged;
  return p.interned(p.dst.attr(p.dst.node(a).a, d));
}

static OpResult prune_by_bbox(Pass &p, OpResult n)
{
  const Node s = p.src.node(n);
  const uint32_t children = node_child_count(s.kind);
  const OpResult a = p.to(s.a), b = p.to(s.b);
  const bool emptyA = children > 0 && !a.id, emptyB = children > 1 && !b.id;
  if (emptyA || emptyB)
  {
    ++p.stats.emptiesFolded;
    if (s.kind == NodeKind::Union || s.kind == NodeKind::Concat)
      return emptyA ? b : a;
    return s.kind == NodeKind::Subtract && !emptyA ? a : OpResult();
  }
  if (children < 2 || s.kind == NodeKind::Concat || p.tolerance < 0)
    return p.copy(n);
  const NodeBox &boxA = p.boxes[s.a.id], &boxB = p.boxes[s.b.id];
  if (!boxA.known || !boxB.known || !bboxes_disjoint(boxA.box, boxB.box, p.tolerance))
    return p.copy(n);
  if (s.kind == NodeKind::Subtract)
  {
    ++p.stats.subtractsPruned;
    return a;
  }
  if (s.kind == NodeKind::Intersect)
  {
    ++p.stats.intersectsPruned;
    return OpResult();
  }
  ++p.stats.unionsToConcat;
  return p.interned(p.dst.op(NodeKind::Concat, a, b));
}

static uint64_t spread_bits(uint32_t v)
{
  uint64_t x = v & MORTON_MAX;
  x = (x | x << 32) & 0x1f00000000ffffull;
  x = (x | x << 16) & 0x1f0000ff0000ffull;
  x = (x | x << 8) & 0x100f00f00f00f00full;
  x = (x | x << 4) & 0x10c30c30c30c30c3ull;
  x = (x | x << 2) & 0x1249249249249249ull;
  return x;
}

struct Part
{
  OpResult node;  // in dst
  uint32_t order; // the first source position in the part
  uint64_t code;  // of the bbox center
};

// The part that comes first in the source stays the first operand, spec.md 5.4
static OpResult pair_up(Pass &p, NodeKind kind, dag::Vector<Part> &parts)
{
  if (parts.empty())
    return OpResult();
  while (parts.size() > 1)
  {
    uint32_t out = 0;
    for (uint32_t i = 0; i < parts.size(); i += 2)
    {
      if (i + 1 == parts.size())
      {
        parts[out++] = parts[i];
        continue;
      }
      const bool swap = parts[i + 1].order < parts[i].order;
      const Part &a = parts[swap ? i + 1 : i], &b = parts[swap ? i : i + 1];
      const Part joined{p.interned(p.dst.op(kind, a.node, b.node)), a.order, 0};
      parts[out++] = joined;
    }
    parts.resize(out);
  }
  return parts[0].node;
}

// Parts disjoint from all others concat at the end; the rest pair up in Morton order, so near parts merge first
static OpResult flat_union(Pass &p, dag::ConstSpan<uint32_t> members)
{
  dag::Vector<Part> parts;
  dag::Vector<const NodeBox *> box;
  p.mark.resize(p.dst.nodeCount(), 0);
  for (uint32_t i = 0; i < members.size(); ++i)
  {
    const OpResult r = p.to(OpResult{members[i]});
    if (!r.id || p.mark[r.id])
      continue;
    p.mark[r.id] = 1;
    parts.push_back(Part{r, i, ~0ull});
    box.push_back(&p.boxes[members[i]]);
  }
  for (const Part &part : parts)
    p.mark[part.node.id] = 0;

  const uint32_t count = parts.size();
  bool known = p.tolerance >= 0;
  BBoxD all;
  for (const NodeBox *b : box)
  {
    known = known && b->known;
    add_bbox(all, b->box);
  }
  dag::Vector<uint8_t> touches(count, known ? 0 : 1);
  if (known && !all.isempty())
  {
    const DPoint3 size = all.lim[1] - all.lim[0];
    const int axis = size.x >= size.y && size.x >= size.z ? 0 : size.y >= size.z ? 1 : 2;
    dag::Vector<uint32_t> byMin(count);
    for (uint32_t i = 0; i < count; ++i)
      byMin[i] = i;
    eastl::sort(byMin.begin(), byMin.end(), [&](uint32_t a, uint32_t b) {
      const double la = box[a]->box.lim[0][axis], lb = box[b]->box.lim[0][axis];
      return la < lb || (la == lb && a < b);
    });
    const double tol = p.tolerance;
    uint32_t marked = 0;
    for (uint32_t i = 0; i < count && marked < count; ++i)
    {
      const BBoxD &bi = box[byMin[i]]->box;
      for (uint32_t j = i + 1; j < count && !(bi.lim[1][axis] + tol < box[byMin[j]]->box.lim[0][axis] - tol); ++j)
        if (!bboxes_disjoint(bi, box[byMin[j]]->box, tol))
        {
          marked += 2 - touches[byMin[i]] - touches[byMin[j]];
          touches[byMin[i]] = touches[byMin[j]] = 1;
        }
    }
  }

  BBoxD centers;
  for (uint32_t i = 0; i < count; ++i)
    if (touches[i] && box[i]->known && !box[i]->box.isempty())
      centers += (box[i]->box.lim[0] + box[i]->box.lim[1]) * 0.5;
  const DPoint3 extent = centers.isempty() ? DPoint3(0, 0, 0) : centers.lim[1] - centers.lim[0];
  const double side = max(max(extent.x, extent.y), extent.z), scale = side > 0 ? MORTON_MAX / side : 0;
  dag::Vector<Part> joined, alone;
  for (uint32_t i = 0; i < count; ++i)
  {
    if (!touches[i])
    {
      alone.push_back(parts[i]);
      continue;
    }
    if (box[i]->known && !box[i]->box.isempty())
    {
      const DPoint3 c = (box[i]->box.lim[0] + box[i]->box.lim[1]) * 0.5 - centers.lim[0];
      uint32_t q[3];
      for (int k = 0; k < 3; ++k)
      {
        const double v = c[k] * scale;
        q[k] = v > 0 ? uint32_t(min(v + 0.5, double(MORTON_MAX))) : 0;
      }
      parts[i].code = spread_bits(q[0]) << 2 | spread_bits(q[1]) << 1 | spread_bits(q[2]);
    }
    joined.push_back(parts[i]);
  }
  eastl::sort(joined.begin(), joined.end(),
    [](const Part &a, const Part &b) { return a.code < b.code || (a.code == b.code && a.order < b.order); });
  if (count > 1)
    p.stats.partsConcatenated += alone.size();
  const OpResult u = pair_up(p, NodeKind::Union, joined), c = pair_up(p, NodeKind::Concat, alone);
  return u.id && c.id ? p.interned(p.dst.op(NodeKind::Concat, u, c)) : u.id ? u : c;
}

static bool union_like(const Pass &p, OpResult n)
{
  const Node &s = p.src.node(n);
  if (s.kind != NodeKind::Concat || p.tolerance < 0)
    return s.kind == NodeKind::Union;
  const NodeBox &a = p.boxes[s.a.id], &b = p.boxes[s.b.id];
  return a.known && b.known && bboxes_disjoint(a.box, b.box, p.tolerance);
}

// Only a node with one parent goes into its parent's rewrite, so a shared subtree stays shared, spec.md 3.3
static OpResult flatten_unions(Pass &p, OpResult n)
{
  const Node s = p.src.node(n);
  const bool single = p.w.parents[n.id] == 1;
  const OpResult parent{single ? p.w.parent[n.id] : 0};
  dag::Vector<uint32_t> members;
  // a named node is a part label: it stays a node, neither taken into its parent nor dissolved into its members
  if (union_like(p, n))
  {
    if (single && union_like(p, parent) && !p.named[n.id])
      return OpResult{NO_INDEX};
    dag::Vector<uint32_t> stack;
    stack.push_back(s.b.id);
    stack.push_back(s.a.id);
    while (!stack.empty())
    {
      const OpResult m{stack.back()};
      stack.pop_back();
      if (p.w.parents[m.id] == 1 && union_like(p, m) && !p.named[m.id])
      {
        stack.push_back(p.src.node(m).b.id);
        stack.push_back(p.src.node(m).a.id);
      }
      else
        members.push_back(m.id);
    }
    if (members.size() < 3)
      return p.copy(n);
    ++p.stats.unionsFlattened;
    return flat_union(p, members);
  }
  if (s.kind != NodeKind::Subtract)
    return p.copy(n);
  if (single && p.src.node(parent).kind == NodeKind::Subtract && p.src.node(parent).a == n && !p.named[n.id])
    return OpResult{NO_INDEX};
  OpResult base = n;
  do
  {
    members.push_back(p.src.node(base).b.id);
    base = p.src.node(base).a;
  } while (p.src.node(base).kind == NodeKind::Subtract && p.w.parents[base.id] == 1 && !p.named[base.id]);
  if (members.size() < 2)
    return p.copy(n);
  eastl::reverse(members.begin(), members.end());
  ++p.stats.subtractChains;
  const OpResult cutter = flat_union(p, members);
  return cutter.id ? p.interned(p.dst.op(NodeKind::Subtract, p.to(base), cutter)) : p.to(base);
}

// Subtrees of Intersect, Cut, Transform and Attr over convex primitives: the top Intersect or Cut of each, spec.md 10.7
static void mark_convex_chains(const Graph &g, const Walk &w, dag::Vector<uint32_t> &tops)
{
  dag::Vector<uint8_t> convex(g.nodeCount(), 0), inside(g.nodeCount(), 0);
  for (uint32_t id : w.order)
  {
    const Node &n = g.node(OpResult{id});
    if (n.kind == NodeKind::Prim)
      convex[id] = is_convex_prim(g.primDesc(OpResult{id}).type);
    else if (n.kind == NodeKind::Intersect || n.kind == NodeKind::Cut || n.kind == NodeKind::Transform || n.kind == NodeKind::Attr)
      convex[id] = convex[n.a.id] && (n.kind != NodeKind::Intersect || convex[n.b.id]);
  }
  for (uint32_t i = w.order.size(); i-- > 0;)
  {
    const uint32_t id = w.order[i];
    const Node &n = g.node(OpResult{id});
    const bool clips = n.kind == NodeKind::Intersect || n.kind == NodeKind::Cut;
    if (convex[id] && clips && (!inside[id] || w.parents[id] > 1))
      tops.push_back(id);
    if (convex[id] && (clips || inside[id]))
      inside[n.a.id] = inside[n.b.id] = 1;
  }
}

// With a checker, the lowering's checks before a prune can drop a node, on the user's graph: a fold or a merge would hide an operand.
// Then the tolerance, below zero when the root bbox is unknown;
// false when the root does not fit the float range, which a prune to Empty would hide
static bool root_tolerance(const Graph &user, OpResult root, const Optimized &cur, const OptimizeParams &p, double &tolerance,
  eastl::string &err)
{
  Walk w;
  tolerance = -1;
  if (p.checkGraph && !p.checkGraph(user, root, p.meshLoader, err))
    return false;
  if (!walk(cur.graph, cur.root, w, err))
    return false;
  dag::Vector<NodeBox> boxes;
  node_boxes(cur.graph, w, boxes);
  return !boxes[cur.root.id].known || auto_tolerance(boxes[cur.root.id].box, tolerance, err);
}

static bool step(const Graph &user, Optimized &cur, OptimizeStats &stats, Pass::Rewrite rewrite, const double *tolerance,
  eastl::string &err)
{
  const bool first = cur.origin.empty();
  Pass pass(first ? user : cur.graph, stats);
  pass.named.resize(pass.src.nodeCount(), 0);
  for (uint32_t i = 1; i < pass.src.nodeCount(); ++i)
    pass.named[i] = user.ownName(OpResult{first ? i : cur.origin[i]}) != nullptr;
  if (tolerance)
    pass.tolerance = *tolerance;
  if (!pass.run(cur.root, rewrite, tolerance != nullptr, err))
    return false;
  cur.root = pass.to(cur.root);
  if (!first)
    for (uint32_t &o : pass.origin)
      o = cur.origin[o];
  cur.graph = eastl::move(pass.dst);
  cur.origin = eastl::move(pass.origin);
  return true;
}

bool optimize(const Graph &g, OpResult root, const OptimizeParams &p, Optimized &out, OptimizeStats &stats, eastl::string &err)
{
  out = Optimized();
  out.root = root;
  stats.nodesBefore = g.nodeCount();
  if (!step(g, out, stats, copy_node, nullptr, err))
    return false;
  stats.deadNodes = g.nodeCount() - out.graph.nodeCount();
  if (p.transforms && !step(g, out, stats, fold_transforms, nullptr, err))
    return false;
  if (p.uvAffines && !step(g, out, stats, merge_uv_affines, nullptr, err))
    return false;
  if (p.bboxPrune && !p.checkGraph)
    return (err = "the prune needs OptimizeParams::checkGraph", false);
  // the checks before the root bbox: a transform that is not finite fails with its node's name, not as a float-range error
  double autoTolerance = -1;
  if (!root_tolerance(g, root, out, p, autoTolerance, err))
    return false;
  const double tolerance = p.tolerance > 0 ? p.tolerance : autoTolerance;
  out.tolerance = tolerance;
  if (p.bboxPrune && !step(g, out, stats, prune_by_bbox, &tolerance, err))
    return false;
  // either pass leaves only the reachable nodes
  if (!step(g, out, stats, p.flatten ? flatten_unions : copy_node, p.flatten ? &tolerance : nullptr, err))
    return false;
  // a .name() label stays an own name; a generated label is a display name only
  for (uint32_t i = 1; i < out.graph.nodeCount(); ++i)
  {
    const OpResult from{out.origin[i]};
    const char *own = g.ownName(from), *file = nullptr;
    uint32_t line = 0;
    out.graph.setName(OpResult{i}, own ? own : g.getName(from).c_str(), own == nullptr);
    if (own && g.source(from, file, line))
      out.graph.setSource(OpResult{i}, file, line);
  }
  stats.nodesAfter = out.graph.nodeCount();
  Walk w;
  if (p.convexChains && walk(out.graph, out.root, w, err))
    mark_convex_chains(out.graph, w, out.convexChains);
  stats.convexChains = out.convexChains.size();
  return true;
}
} // namespace dacsg
