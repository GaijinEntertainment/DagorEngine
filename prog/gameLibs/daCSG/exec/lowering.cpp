// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "exec/pipeline.h"
#include "graph/optimize.h"
#include "mesh/solidUtil.h"
#include "prims/tessellate.h"
#include <gameLibs/daCSG/leaf_api.h>
#include <gameLibs/daCSG/validate_api.h>
#include <math/dag_check_nan.h>
#include <perfMon/dag_cpuFreq.h>
#include <util/dag_bitwise_cast.h>

namespace dacsg
{
static constexpr double CUT_MARGIN = 0.01;

static const TMatrixD IDENT_TM = {{DPoint3(1, 0, 0), DPoint3(0, 1, 0), DPoint3(0, 0, 1), DPoint3(0, 0, 0)}};

struct BoxFace
{
  int n[3], u[3], v[3];
};

static const BoxFace box_faces[6] = {
  {{1, 0, 0}, {0, 0, -1}, {0, 1, 0}},
  {{-1, 0, 0}, {0, 0, 1}, {0, 1, 0}},
  {{0, 1, 0}, {1, 0, 0}, {0, 0, -1}},
  {{0, -1, 0}, {1, 0, 0}, {0, 0, 1}},
  {{0, 0, 1}, {1, 0, 0}, {0, 1, 0}},
  {{0, 0, -1}, {-1, 0, 0}, {0, 1, 0}},
};

SourceMesh &LowerContext::newSource()
{
  storage.push_back(eastl::make_unique<SourceMesh>());
  return *storage.back();
}

static double determinant(const TMatrixD &m) { return m.col[0] * (m.col[1] % m.col[2]); }

// signed permutation times diagonal: flush stays flush
static bool keeps_contacts(const TMatrixD &m)
{
  for (int i = 0; i < 3; ++i)
  {
    int inColumn = 0, inRow = 0;
    for (int j = 0; j < 3; ++j)
    {
      inColumn += m.col[i][j] != 0;
      inRow += m.col[j][i] != 0;
    }
    if (inColumn != 1 || inRow != 1)
      return false;
  }
  return true;
}

// m is tm_in_range, so det and the cofactors are finite; false: 1 / det or the inverse is past TM_LIMIT
static bool inverse(const Graph &g, OpResult attr, const TMatrixD &m, TMatrixD &r, eastl::string &err)
{
  const double det = determinant(m);
  const DPoint3 cofactor[3] = {m.col[1] % m.col[2], m.col[2] % m.col[0], m.col[0] % m.col[1]};
  const bool inRange = det != 0 && exponent_of(det) >= -1021 && exponent_of(cofactor, 3) + 1 - exponent_of(det) <= 301;
  const double invDet = inRange ? 1 / det : 0;
  const DPoint3 row[3] = {cofactor[0] * invDet, cofactor[1] * invDet, cofactor[2] * invDet};
  for (int i = 0; i < 3; ++i)
    r.col[i] = DPoint3(row[0][i], row[1][i], row[2][i]);
  r.col[3] = -DPoint3(row[0] * m.col[3], row[1] * m.col[3], row[2] * m.col[3]);
  if (inRange && tm_in_range(r))
    return true;
  err.sprintf("%s: the transform above it has no inverse within 2^300", g.getName(attr).c_str());
  return false;
}

// det of a tm_in_range transform: finite
static bool regular(const Graph &g, double det, OpResult node, eastl::string &err)
{
  if (det != 0)
    return true;
  err.sprintf("%s: the transform is singular", g.getName(node).c_str());
  return false;
}

static bool compose(const Graph &g, const TMatrixD &outer, const TMatrixD &inner, OpResult node, TMatrixD &r, eastl::string &err)
{
  if (compose_tm(outer, inner, r))
    return true;
  err.sprintf("%s: the transform composed with the ones above it has an entry past 2^300", g.getName(node).c_str());
  return false;
}

// The Cut's plane in the frame m: unit n, keeps n * p <= d
static bool frame_plane(const Graph &g, OpResult cut, const TMatrixD &m, DPoint3 &n, double &d, eastl::string &err)
{
  const PlaneD &plane = g.plane(cut);
  // m and the plane are within 2^300, so det and the cofactor are finite
  const double det = determinant(m);
  const DPoint3 cofactor = (m.col[1] % m.col[2]) * plane.n.x + (m.col[2] % m.col[0]) * plane.n.y + (m.col[0] % m.col[1]) * plane.n.z;
  // 1 / det in range and n under 2^511, so n's length, n * col3 and d are finite too
  const int detExp = exponent_of(det);
  const bool inRange = det != 0 && detExp >= -1021 && exponent_of(&cofactor, 1) + 1 - detExp <= 511;
  n = inRange ? cofactor * (1 / det) : DPoint3(0, 0, 0);
  d = inRange ? plane.d + n * m.col[3] : 0;
  const double len = length(n);
  // len is at least n's largest |component|, which keeps 1 / len and d / len in range
  const int nExp = exponent_of(&n, 1);
  if (!(len > 0) || nExp < -1021 || exponent_of(d) - nExp > 1021)
    return (err.sprintf("%s: the cut plane is degenerate", g.getName(cut).c_str()), false);
  n = n * (1 / len);
  d /= len;
  return true;
}

static uint32_t mirror_halfedge(uint32_t h) { return h - h % 3 + 2 - h % 3; }

// The section 4 checks of a prim's tessellation, which fill src.solid
static bool check_source(const LowerContext &ctx, OpResult node, SourceMesh &src, eastl::string &err)
{
  if (const char *breach = mesh_contract_breach(src.mesh))
  {
    err.sprintf("%s: %s", ctx.g.getName(node).c_str(), breach);
    return false;
  }
  mesh_to_solid(src.mesh, src.solid);
  const PrimType type = ctx.g.primDesc(node).type;
  const ValidationReport r = validate(src.solid, prim_can_fold(type) ? VALIDATE_SELF_INTERSECTION : 0);
  if (!r.valid)
  {
    err.sprintf("%s: %s", ctx.g.getName(node).c_str(), r.error.c_str());
    return false;
  }
  src.solid.convex = is_convex_prim(type);
  // analytic normals; an imported mesh keeps the smgr rule
  if (type != PrimType::Mesh && src.mesh.findChannel(CH_NORMAL))
    src.ownNormal.assign(src.mesh.faceCount(), 1);
  return true;
}

static void add_source_stats(LowerContext &ctx, OpResult node, uint64_t time_us, const SourceMesh &src)
{
  NodeStats &ns = ctx.stats.nodes.push_back();
  ns.node = node;
  ns.name = ctx.g.getName(node);
  ns.timeUs = time_us;
  ns.trisOut = src.mesh.faceCount();
}

static ILeafSource *leaf_source(const Graph &g, OpResult node)
{
  const PrimDesc &d = g.primDesc(node);
  return d.type == PrimType::Leaf ? g.leafSource(d.path) : nullptr;
}

// exact for float entries: fma splits each triple product
static bool float_singular(const TMatrix &tm)
{
  double e[12] = {};
  int n = 0;
  const auto grow = [&](double q) {
    int kept = 0;
    for (int t = 0; t < n; ++t)
    {
      const double sum = q + e[t], bv = sum - q, low = (q - (sum - bv)) + (e[t] - bv);
      if (low != 0)
        e[kept++] = low;
      q = sum;
    }
    if (q != 0)
      e[kept++] = q;
    n = kept;
  };
  for (int i = 0; i < 3; ++i)
    for (int s = 1; s < 3; ++s)
    {
      const double pair = (s == 1 ? 1.0 : -1.0) * tm.m[1][(i + s) % 3] * tm.m[2][(i + 3 - s) % 3], hi = pair * tm.m[0][i];
      grow(hi);
      grow(fma(pair, tm.m[0][i], -hi));
    }
  return n == 0;
}

static bool leaf_world(const Graph &g, OpResult node, const ILeafSource &source, TMatrixD world, TMatrix &tm, eastl::string &err)
{
  for (int i = 0; i < 4; ++i)
  {
    if (!(max(max(fabs(world.col[i].x), fabs(world.col[i].y)), fabs(world.col[i].z)) <= FLT_MAX))
      return (err.sprintf("%s: %s: the world transform passes the float range", g.getName(node).c_str(), source.name()), false);
    tm.setcol(i, Point3(world.col[i]));
    for (int k = 0; k < 3; ++k)
      tm.m[i][k] = fabsf(tm.m[i][k]) < FLT_MIN ? 0 : tm.m[i][k]; // FTZ and DAZ read it as zero
  }
  if (float_singular(tm))
    return (err.sprintf("%s: %s: the float world transform is singular", g.getName(node).c_str(), source.name()), false);
  return true;
}

// The source works in world space: no transform or flip follows
static bool tessellate_leaf(LowerContext &ctx, OpResult node, ILeafSource &source, const TMatrix &tm, SourceMesh &src,
  eastl::string &err)
{
  const int64_t start = ref_time_ticks();
  eastl::string why;
  if (!source.tessellate(tm, ctx.quality, ctx.kernelParams.tolerance, ctx.sag, src.mesh, why))
  {
    err.sprintf("%s: %s: %s", ctx.g.getName(node).c_str(), source.name(), why.c_str());
    return false;
  }
  Mesh &m = src.mesh;
  for (uint32_t f = 0; f < m.faceCount(); ++f)
  {
    m.face[f].prim = node.id;
    m.face[f].srcFace = f;
  }
  m.addDefaultChannels();
  if (const Channel *normal = m.findChannel(CH_NORMAL))
  {
    if (normal->dim != 3)
    {
      err.sprintf("%s: %s: the normal channel has dim %d, not 3", ctx.g.getName(node).c_str(), source.name(), normal->dim);
      return false;
    }
    src.ownNormal.assign(m.faceCount(), 1);
  }
  if (!ctx.validateLeaves)
  {
    // the contract stays: a value that is not finite would reach the comparisons
    if (const char *breach = mesh_contract_breach(m))
      return (err.sprintf("%s: %s: %s", ctx.g.getName(node).c_str(), source.name(), breach), false);
    mesh_to_solid(m, src.solid);
    src.checked = false;
  }
  else if (!check_source(ctx, node, src, err))
    return false;
  add_source_stats(ctx, node, usec_since(start), src);
  return true;
}

static constexpr double SIMILARITY_EPS = 1e-6; // a float rotation is orthogonal to about 1e-7

// m = s * Q + t, Q orthogonal; m is tm_in_range, so every square below is finite
static bool similarity_scale(const TMatrixD &m, double &s)
{
  const double l = lengthSq(m.col[0]), eps = SIMILARITY_EPS * l;
  if (!(l > 0))
    return false;
  for (int i = 0; i < 3; ++i)
    if (fabs(lengthSq(m.col[i]) - l) > eps || fabs(m.col[i] * m.col[(i + 1) % 3]) > eps)
      return false;
  s = sqrt(l);
  return true;
}

static TMatrixD scale_tm(double s) { return TMatrixD{{DPoint3(s, 0, 0), DPoint3(0, s, 0), DPoint3(0, 0, s), DPoint3(0, 0, 0)}}; }

// addSourceLeaf's frame checks: world is the float frame to tessellate in, s the similarity scale or 0
static bool leaf_frame(const Graph &g, OpResult node, const ILeafSource &source, const TMatrixD &tm, TMatrix &world, double &s,
  eastl::string &err)
{
  s = 0;
  if (!similarity_scale(tm, s))
    return regular(g, determinant(tm), node, err) && leaf_world(g, node, source, tm, world, err);
  return leaf_world(g, node, source, scale_tm(s), world, err);
}

// an op that an Empty operand makes Empty whatever the other operand holds: Intersect with one, Subtract from one;
// the other operand is discarded, neither lowered nor evaluated
static bool empty_by_operand(const Graph &g, const Node &n)
{
  if (!is_op_kind(n.kind))
    return false;
  const bool emptyA = g.node(n.a).kind == NodeKind::Empty, emptyB = g.node(n.b).kind == NodeKind::Empty;
  return n.kind == NodeKind::Intersect ? emptyA || emptyB : n.kind == NodeKind::Subtract && emptyA;
}

// own: the subtrees evaluated alone, children first; parents over the edges that reach the root
static bool lowering_units(const Graph &g, OpResult root, Walk &w, dag::Vector<uint32_t> &parents, dag::Vector<OpResult> &own,
  eastl::string &err)
{
  const uint32_t count = g.nodeCount();
  if (!walk(g, root, w, err))
    return false;
  // the optimizer, when it runs, has pruned the discarded operands already
  parents.assign(count, 0);
  for (uint32_t i = w.order.size(); i-- > 0;)
  {
    const uint32_t node = w.order[i];
    const Node &n = g.node(OpResult{node});
    if ((node != root.id && !parents[node]) || empty_by_operand(g, n))
      continue;
    for (uint32_t k = 0; k < node_child_count(n.kind); ++k)
      ++parents[(k ? n.b : n.a).id];
  }
  dag::Vector<uint8_t> hasBoolean(count, 0), underGeneralTm(count, 0);
  for (uint32_t node : w.order) // children first
  {
    const Node &n = g.node(OpResult{node});
    bool boolean = is_op_kind(n.kind) || n.kind == NodeKind::Cut;
    for (uint32_t i = 0; i < node_child_count(n.kind); ++i)
      boolean = boolean || hasBoolean[(i ? n.b : n.a).id];
    hasBoolean[node] = boolean;
    // only a used transform marks its subtree: a discarded one would send a live shared subtree to its own evaluation
    const bool usedNode = node == root.id || parents[node];
    if (n.kind == NodeKind::Transform && usedNode && hasBoolean[n.a.id] && !keeps_contacts(g.tm(OpResult{node})))
      underGeneralTm[n.a.id] = 1;
  }
  // a node an Empty operand discards has no counted parent: not an own subtree, even under a general transform
  own.clear();
  for (uint32_t node : w.order)
    if (node != root.id && parents[node] && ((parents[node] >= 2 && hasBoolean[node]) || underGeneralTm[node]))
      own.push_back(OpResult{node});
  return true;
}

struct Frame
{
  TMatrixD tm;
  OpResult node;
  uint32_t leafMark = 0, treeMark = 0, attrMark = 0;
  uint8_t parity = 0; // Subtract right children on the path, mod 2
  bool post = false;
};

// The lowering's frame checks and walk, with no tessellation
static bool frames_valid(const Graph &g, OpResult root, dag::ConstSpan<OpResult> own, eastl::string &err)
{
  dag::Vector<uint8_t> instance(g.nodeCount(), 0);
  for (OpResult node : own)
    instance[node.id] = 1;
  dag::Vector<Frame> frames;
  TMatrixD tm;
  TMatrix world;
  DPoint3 n;
  double d, s;
  for (uint32_t i = 0; i <= own.size(); ++i)
  {
    const OpResult unit = i < own.size() ? own[i] : root;
    for (frames.push_back(Frame{IDENT_TM, unit}); !frames.empty();)
    {
      const Frame f = frames.back();
      frames.pop_back();
      const Node &node = g.node(f.node);
      const ILeafSource *leaf = node.kind == NodeKind::Prim ? leaf_source(g, f.node) : nullptr;
      bool ok = true;
      if (f.post)
        ok = node.kind == NodeKind::Attr ? inverse(g, f.node, f.tm, tm, err) : frame_plane(g, f.node, f.tm, n, d, err);
      else if (leaf)
        ok = leaf_frame(g, f.node, *leaf, f.tm, world, s, err);
      else if (node.kind == NodeKind::Prim || (f.node != unit && instance[f.node.id]))
        ok = regular(g, determinant(f.tm), f.node, err);
      else if (node.kind == NodeKind::Transform)
      {
        ok = compose(g, f.tm, g.tm(f.node), f.node, tm, err);
        frames.push_back(Frame{tm, node.a});
      }
      else if (node.kind != NodeKind::Empty && !empty_by_operand(g, node))
      {
        if (node.kind == NodeKind::Attr || node.kind == NodeKind::Cut)
          frames.push_back(Frame{f.tm, f.node, 0, 0, 0, 0, true});
        if (is_op_kind(node.kind))
          frames.push_back(Frame{f.tm, node.b});
        frames.push_back(Frame{f.tm, node.a});
      }
      if (!ok)
        return false;
    }
  }
  return true;
}

// spec.md 10.5: values, frames, then primitives
static bool graph_valid(const Graph &g, OpResult root, const Walk &w, dag::ConstSpan<OpResult> own, MeshLoader loader, bool nodes,
  eastl::string &err)
{
  BBoxD box;
  double reach;
  for (uint32_t node : w.order)
    if (nodes && !check_node_values(g, OpResult{node}, box, reach, err))
      return false;
  if (!frames_valid(g, root, own, err))
    return false;
  for (uint32_t node : w.order)
    if (nodes && g.node(OpResult{node}).kind == NodeKind::Prim && g.primDesc(OpResult{node}).type != PrimType::Leaf &&
        !check_prim(g, OpResult{node}, loader, err))
      return false;
  return true;
}

bool check_graph(const Graph &g, OpResult root, MeshLoader loader, eastl::string &err)
{
  Walk w;
  dag::Vector<uint32_t> parents;
  dag::Vector<OpResult> own;
  return lowering_units(g, root, w, parents, own, err) && graph_valid(g, root, w, own, loader, true, err);
}

bool prepare_graph(LowerContext &ctx, OpResult root, bool check_nodes, dag::Vector<OpResult> &own, eastl::string &err)
{
  Walk w;
  return lowering_units(ctx.g, root, w, ctx.parents, own, err) && graph_valid(ctx.g, root, w, own, ctx.meshLoader, check_nodes, err);
}

static constexpr double SAG_PER_DIAGONAL = 1e-3; // spec.md 4

bool graph_bbox(LowerContext &ctx, OpResult root, BBoxD &box, eastl::string &err)
{
  ctx.prim.assign(ctx.g.nodeCount(), nullptr);
  ctx.instance.assign(ctx.g.nodeCount(), nullptr);
  const auto used = [&](uint32_t id) { return id == root.id || ctx.parents[id]; };
  dag::Vector<BBoxD> boxes(root.id + 1);
  dag::Vector<double> scale(root.id + 1, 0.0), reach(root.id + 1, 0.0);
  dag::Vector<uint32_t> prims, known; // the imports first: only their file gives their bbox
  scale[root.id] = 1;
  for (uint32_t id = root.id; id > 0; --id)
  {
    const OpResult r{id};
    const Node &n = ctx.g.node(r);
    // only a used node's scale reaches its children: a discarded operand's would enlarge a live leaf it shares
    if (!used(id))
      continue;
    // a primitive takes the sag of its most enlarged instance; one past 2^FAR_EXPONENT takes the most segments there too
    double s = scale[id];
    if (n.kind == NodeKind::Transform && !bounded_product(scale[id], largest_scale(ctx.g.tm(r)), s))
      s = ldexp(1.0, FAR_EXPONENT);
    for (uint32_t i = 0; i < node_child_count(n.kind); ++i)
      scale[(i ? n.b : n.a).id] = max(scale[(i ? n.b : n.a).id], s);
    if (n.kind != NodeKind::Prim)
      continue;
    // a second call may answer otherwise
    if (!leaf_source(ctx.g, r))
      (prim_bbox(ctx.g, ctx.g.primDesc(r), boxes[id], reach[id]) ? known : prims).push_back(id);
    else if (!check_node_values(ctx.g, r, boxes[id], reach[id], err))
      return false;
  }
  const uint32_t imports = prims.size();
  prims.insert(prims.end(), known.rbegin(), known.rend());
  dag::Vector<SourceMesh> made(prims.size());
  dag::Vector<eastl::string> madeErr(prims.size());
  dag::Vector<uint64_t> madeUs(prims.size());
  dag::Vector<uint8_t> madeOk(prims.size());
  auto make = [&](uint32_t k, uint32_t, uint32_t) {
    const int64_t start = ref_time_ticks();
    const OpResult node{prims[k]};
    // a scale that rounds to 0, or a sag past 2^FAR_EXPONENT, takes the fewest segments
    const double s = scale[node.id], sag = s > 0 && exponent_of(ctx.sag) - exponent_of(s) <= FAR_EXPONENT ? ctx.sag / s : DBL_MAX;
    madeOk[k] = tessellate(ctx.g, node, sag, made[k].mesh, madeErr[k], ctx.meshLoader) && check_source(ctx, node, made[k], madeErr[k]);
    madeUs[k] = usec_since(start);
  };
  const auto combine = [&]() -> bool {
    for (uint32_t id = 1; id <= root.id; ++id)
    {
      const OpResult r{id};
      const Node &n = ctx.g.node(r);
      if (!used(id) || n.kind == NodeKind::Prim || n.kind == NodeKind::Empty)
        continue;
      BBoxD &b = boxes[id] = BBoxD();
      reach[id] = reach[n.a.id];
      if (n.kind != NodeKind::Transform)
        add_bbox(b, boxes[n.a.id]);
      else if (!transform_bbox(ctx.g.tm(r), boxes[n.a.id], b) ||
               !bounded_product(reach[n.a.id], cbrt(fabs(determinant(ctx.g.tm(r)))), reach[id]))
        return (err = ctx.g.getName(r) + ": the transform takes the box or the reach below it past 2^500", false);
      if (!is_op_kind(n.kind) || n.kind == NodeKind::Subtract)
        continue;
      reach[id] = max(reach[id], reach[n.b.id]);
      // an intersection is bounded by the operands' overlap; disjoint operands keep the first's box, which bounds an empty result
      const BBoxD both{{max(b.lim[0], boxes[n.b.id].lim[0]), min(b.lim[1], boxes[n.b.id].lim[1])}};
      if (n.kind != NodeKind::Intersect)
        add_bbox(b, boxes[n.b.id]);
      else if (both.lim[0].x <= both.lim[1].x && both.lim[0].y <= both.lim[1].y && both.lim[0].z <= both.lim[1].z)
        b = both;
    }
    return true;
  };
  // leaf meshes pass their box by reach
  const auto rootBox = [&] {
    BBoxD b = boxes[root.id];
    b.lim[0] -= DPoint3(reach[root.id], reach[root.id], reach[root.id]);
    b.lim[1] += DPoint3(reach[root.id], reach[root.id], reach[root.id]);
    return b;
  };
  const WorkerLease lease(ctx.kernelParams.budget, min(ctx.workers, uint32_t(prims.size())));
  daparallel::parallel_run(lease.workers(), imports, 1, make);
  for (uint32_t k = 0; k < imports; ++k)
    boxes[prims[k]] = made[k].solid.bbox;
  if (!combine())
    return false;
  const BBoxD all = rootBox();
  ctx.sag = all.isempty() ? 0 : SAG_PER_DIAGONAL * length(all.lim[1] - all.lim[0]) / sqr(double(ctx.quality));
  const bool importsMade = eastl::find(madeOk.begin(), madeOk.begin() + imports, uint8_t(0)) == madeOk.begin() + imports;
  daparallel::parallel_run(lease.workers(), importsMade ? prims.size() - imports : 0, 1,
    [&](uint32_t k, uint32_t, uint32_t) { make(imports + k, 0, 0); });

  for (uint32_t k = 0; k < prims.size(); ++k)
  {
    if (!madeOk[k])
    {
      err = madeErr[k];
      return false;
    }
    SourceMesh &src = ctx.newSource();
    src = eastl::move(made[k]);
    add_source_stats(ctx, OpResult{prims[k]}, madeUs[k], src);
    ctx.prim[prims[k]] = &src;
    boxes[prims[k]] = src.solid.bbox;
  }
  if (!combine())
    return false;
  box = rootBox();
  return true;
}

static void cut_box(const BBoxD &part, const DPoint3 &n, double d, OpResult node, SourceMesh &src)
{
  DPoint3 axes[3];
  tangent_frame(n, axes[0], axes[1]);
  axes[2] = n;
  const double margin = CUT_MARGIN * length(part.lim[1] - part.lim[0]);
  double lo[3] = {DBL_MAX, DBL_MAX, DBL_MAX}, hi[3] = {-DBL_MAX, -DBL_MAX, -DBL_MAX};
  for (int i = 0; i < 8; ++i)
    for (int a = 0; a < 3; ++a)
    {
      const double s = axes[a] * bbox_corner(part, i);
      lo[a] = min(lo[a], s - margin);
      hi[a] = max(hi[a], s + margin);
    }
  hi[2] = d;

  DPoint3 corner[8];
  Mesh &m = src.mesh;
  for (int i = 0; i < 8; ++i)
  {
    corner[i] = axes[0] * (i & 1 ? hi[0] : lo[0]) + axes[1] * (i & 2 ? hi[1] : lo[1]) + axes[2] * (i & 4 ? hi[2] : lo[2]);
    m.pos.push_back(Point3(corner[i]));
  }
  static const int quad_st[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
  static const int quad_tris[2][3] = {{0, 1, 2}, {0, 2, 3}};
  dag::Vector<float> uv;
  for (int f = 0; f < 6; ++f)
  {
    const BoxFace &bf = box_faces[f];
    uint32_t vert[4];
    for (int q = 0; q < 4; ++q)
    {
      vert[q] = 0;
      for (int i = 0; i < 3; ++i)
        if (bf.n[i] + (2 * quad_st[q][0] - 1) * bf.u[i] + (2 * quad_st[q][1] - 1) * bf.v[i] > 0)
          vert[q] |= 1u << i;
    }
    for (const int *t : quad_tris)
    {
      for (int k = 0; k < 3; ++k)
      {
        m.tri.push_back(vert[t[k]]);
        uv.push_back(float(quad_st[t[k]][0]));
        uv.push_back(float(quad_st[t[k]][1]));
      }
      m.face.push_back(FaceInfo{node.id, m.faceCount(), 1u << f, 0, 0});
    }
  }
  m.addChannel(CH_UV, 2).data = uv;
  m.addDefaultChannels();

  mesh_to_solid(m, src.solid);
  for (int i = 0; i < 8; ++i)
    src.solid.pos[i] = corner[i];
  src.solid.bbox = points_bbox(src.solid.pos);
  compute_vertex_normals(src.solid);
  src.solid.convex = true;
}

struct Part
{
  uint32_t node = NO_INDEX; // tree node; NO_INDEX: empty
  BBoxD box;
};

// leaf instances of one source under one transform are one geometry: their vertices and planes tie exactly
struct Geometry
{
  const SourceMesh *src = nullptr;
  TMatrixD tm;
};

class Lowering
{
public:
  Lowering(LowerContext &c, Lowered &o, eastl::string &e) : ctx(c), out(o), err(e) {}
  bool run(OpResult r);

private:
  static Frame child(OpResult node, const TMatrixD &tm, uint8_t parity) { return Frame{tm, node, 0, 0, 0, parity}; }
  bool enter(const Frame &f);
  bool leave(const Frame &f);
  bool addLeaf(const SourceMesh &src, const TMatrixD &tm, OpResult node);
  bool addSourceLeaf(const Frame &f, ILeafSource &leaf);
  uint32_t geometryOf(const SourceMesh &src, const TMatrixD &tm);
  bool addAttr(const Frame &f);
  bool cut(const Frame &f);
  void combine(const Frame &f, NodeKind kind);
  void clipChain(const Frame &f);
  void rollback(const Frame &f);
  uint32_t addNode(BoolTree::Kind kind, uint32_t a, uint32_t b, uint32_t leaf, OpResult node);

  LowerContext &ctx;
  Lowered &out;
  eastl::string &err;
  OpResult root;
  dag::Vector<Frame> frames;
  dag::Vector<Part> parts;
  dag::Vector<Geometry> geometries;
  HashChain geometryChain; // by a hash of source and tm
};

bool Lowering::run(OpResult r)
{
  root = r;
  frames.push_back(Frame{IDENT_TM, r});
  while (!frames.empty())
  {
    const Frame f = frames.back();
    frames.pop_back();
    if (!(f.post ? leave(f) : enter(f)))
      return false;
  }
  const bool rootLast =
    parts.size() == 1 && (parts[0].node == NO_INDEX ? out.tree.nodes.empty() : parts[0].node + 1 == out.tree.nodes.size());
  if (!rootLast)
    err = "lowering: the root is not the last tree node";
  return rootLast;
}

bool Lowering::enter(const Frame &f)
{
  if (f.node != root && ctx.instance[f.node.id])
    return addLeaf(*ctx.instance[f.node.id], f.tm, f.node);
  const Node &n = ctx.g.node(f.node);
  if (n.kind == NodeKind::Empty || empty_by_operand(ctx.g, n))
  {
    parts.push_back(Part());
    return true;
  }
  if (ILeafSource *leaf = n.kind == NodeKind::Prim ? leaf_source(ctx.g, f.node) : nullptr)
    return addSourceLeaf(f, *leaf);
  if (n.kind == NodeKind::Prim)
    return addLeaf(*ctx.prim[f.node.id], f.tm, f.node);
  if (n.kind == NodeKind::Transform)
  {
    TMatrixD tm;
    if (!compose(ctx.g, f.tm, ctx.g.tm(f.node), f.node, tm, err))
      return false;
    frames.push_back(child(n.a, tm, f.parity));
    return true;
  }
  frames.push_back(Frame{f.tm, f.node, out.tree.leaves.size(), out.tree.nodes.size(), out.attrs.size(), f.parity, true});
  if (is_op_kind(n.kind))
    frames.push_back(child(n.b, f.tm, f.parity ^ (n.kind == NodeKind::Subtract ? 1 : 0)));
  frames.push_back(child(n.a, f.tm, f.parity));
  return true;
}

bool Lowering::leave(const Frame &f)
{
  const NodeKind kind = ctx.g.node(f.node).kind;
  if (kind == NodeKind::Attr)
    return addAttr(f);
  if (kind == NodeKind::Cut && !cut(f))
    return false;
  if (kind != NodeKind::Cut)
    combine(f, kind);
  if (f.node.id < ctx.clip.size() && ctx.clip[f.node.id])
    clipChain(f);
  return true;
}

// The chain's leaves stay for the resolve step; the tree holds their intersection as one leaf
void Lowering::clipChain(const Frame &f)
{
  const uint32_t first = f.leafMark, count = out.tree.leaves.size() - first;
  Solid clip;
  if (parts.back().node == NO_INDEX || count < 2 ||
      !clip_convex(make_span_const(out.tree.leaves.data() + first, count), ctx.kernelParams.tolerance, clip))
    return;
  ctx.stats.kernel.add("convex clips", 1);
  if (!clip.faceCount())
  {
    rollback(f);
    parts.back() = Part();
    return;
  }
  out.tree.nodes.resize(f.treeMark);
  out.treeNode.resize(f.treeMark);
  clip.leafIndex = out.tree.leaves.size();
  parts.back().box = clip.bbox;
  parts.back().node = addNode(BoolTree::Kind::Leaf, 0, 0, clip.leafIndex, f.node);
  out.add(eastl::move(clip), out.sources[first]);
}

uint32_t Lowering::addNode(BoolTree::Kind kind, uint32_t a, uint32_t b, uint32_t leaf, OpResult node)
{
  out.tree.nodes.push_back(BoolTree::Node{kind, {a, b}, leaf});
  out.treeNode.push_back(node);
  return out.tree.nodes.size() - 1;
}

void Lowering::rollback(const Frame &f)
{
  out.tree.nodes.resize(f.treeMark);
  out.treeNode.resize(f.treeMark);
  out.tree.leaves.resize(f.leafMark);
  out.sources.resize(f.leafMark);
  out.attrs.resize(f.attrMark);
}

bool Lowering::addLeaf(const SourceMesh &src, const TMatrixD &tm, OpResult node)
{
  const Solid &s = src.solid;
  if (!s.faceCount())
  {
    parts.push_back(Part());
    return true;
  }
  const double det = determinant(tm);
  // frames_valid skips a folded subtree's composed frame
  if (!regular(ctx.g, det, node, err))
    return false;
  const uint32_t index = out.tree.leaves.size();
  Solid leaf;
  leaf.pos.resize(s.pos.size());
  for (uint32_t i = 0; i < s.pos.size(); ++i)
    leaf.pos[i] = transform_point(tm, s.pos[i]);
  leaf.bbox = points_bbox(leaf.pos);
  if (det > 0)
    leaf.halfedge = s.halfedge;
  else
  {
    leaf.halfedge.resize(s.halfedge.size());
    for (uint32_t h = 0; h < s.halfedge.size(); ++h)
    {
      const Halfedge &e = s.halfedge[mirror_halfedge(h)];
      leaf.halfedge[h] = Halfedge{e.endVert, e.startVert, e.pairedHalfedge == NO_INDEX ? NO_INDEX : mirror_halfedge(e.pairedHalfedge)};
    }
  }
  leaf.face = s.face;
  leaf.leafRef.resize(s.faceCount());
  for (uint32_t f = 0; f < s.faceCount(); ++f)
    leaf.leafRef[f] = LeafRef{index, f};
  leaf.leafIndex = index;
  leaf.closed = s.closed;
  leaf.convex = s.convex;
  compute_vertex_normals(leaf);
  const uint32_t geometry = geometryOf(src, tm);
  if (src.facePlane.size() != s.faceCount())
    compute_face_planes(s, src.facePlane);
  if (src.cornerClass.size() != s.halfedge.size())
    compute_corner_classes(src);
  leaf.facePlaneKey.resize(s.faceCount());
  for (uint32_t f = 0; f < s.faceCount(); ++f)
    leaf.facePlaneKey[f] = Provenance::key(geometry, src.facePlane[f]);
  leaf.provenance.assign(leaf.pos.size(), Provenance());
  for (uint32_t h = 0; h < leaf.halfedge.size(); ++h)
    leaf.provenance[leaf.halfedge[h].startVert].add(leaf.facePlaneKey[h / 3]);
  // check_source validated the source, which an identity leaf repeats
  const bool checkLeaf = src.checked && memcmp(&tm, &IDENT_TM, sizeof(TMatrixD)) != 0;
  const ValidationReport r = checkLeaf ? validate(leaf) : ValidationReport();
  if (checkLeaf && !r.valid)
  {
    err.sprintf("%s: %s", ctx.g.getName(node).c_str(), r.error.c_str());
    return false;
  }
  parts.push_back(Part{addNode(BoolTree::Kind::Leaf, 0, 0, index, node), leaf.bbox});
  out.add(eastl::move(leaf), LeafSource{&src, det < 0, out.groupCount, tm});
  out.groupCount += src.groupCount;
  return true;
}

// A similarity shares one tessellation per scale; another map tessellates per instance
bool Lowering::addSourceLeaf(const Frame &f, ILeafSource &leaf)
{
  TMatrix world;
  double s;
  // frames_valid skips a folded subtree's frame
  if (!leaf_frame(ctx.g, f.node, leaf, f.tm, world, s, err))
    return false;
  if (s == 0)
  {
    SourceMesh &own = ctx.newSource();
    return tessellate_leaf(ctx, f.node, leaf, world, own, err) && addLeaf(own, IDENT_TM, f.node);
  }
  // the same float scale: the source sees a float world_tm, so equal float scales give one tessellation, and a
  // channel that depends on the scale differs only when the float scale does
  const SourceMesh *src = nullptr;
  for (const LowerContext::LeafMesh &lm : ctx.leafMeshes)
    if (lm.node == f.node.id && float(lm.scale) == float(s))
    {
      src = lm.src;
      s = lm.scale;
    }
  if (!src)
  {
    SourceMesh &fresh = ctx.newSource();
    if (!tessellate_leaf(ctx, f.node, leaf, world, fresh, err))
      return false;
    ctx.leafMeshes.push_back(LowerContext::LeafMesh{f.node.id, s, &fresh});
    src = &fresh;
  }
  TMatrixD unit;
  return compose(ctx.g, f.tm, scale_tm(1 / s), f.node, unit, err) && addLeaf(*src, unit, f.node);
}

uint32_t Lowering::geometryOf(const SourceMesh &src, const TMatrixD &tm)
{
  uint64_t key = uint64_t(uintptr_t(&src));
  for (int i = 0; i < 4; ++i)
    for (int k = 0; k < 3; ++k)
      key = (key ^ dag::bit_cast<uint64_t>(tm.col[i][k] + 0.0)) * 1099511628211ull; // + 0.0: -0 hashes as 0, as == sees it
  for (uint32_t i = geometryChain.first(key); i != NO_INDEX; i = geometryChain.next[i])
  {
    const Geometry &g = geometries[i];
    if (g.src == &src && g.tm.col[0] == tm.col[0] && g.tm.col[1] == tm.col[1] && g.tm.col[2] == tm.col[2] && g.tm.col[3] == tm.col[3])
      return i;
  }
  geometryChain.add(key, geometries.size());
  geometries.push_back(Geometry{&src, tm});
  return geometries.size() - 1;
}

bool Lowering::addAttr(const Frame &f)
{
  if (parts.back().node == NO_INDEX)
    return true;
  const AttrDesc &d = ctx.g.attrDesc(f.node);
  const uint32_t first = f.leafMark, end = out.tree.leaves.size();
  if (d.kind == AttrKind::UvAffine && out.attrs.size() > f.attrMark && out.attrs.back().desc.onlyUnset == d.onlyUnset)
  {
    AttrRecord &last = out.attrs.back();
    AttrDesc fused = last.desc;
    if (last.desc.kind == AttrKind::UvAffine && last.firstLeaf == first && last.endLeaf == end &&
        compose_uv_affine(d.uvAffine, fused.uvAffine))
    {
      last.desc = fused;
      return true;
    }
  }
  TMatrixD fromRoot;
  if (!inverse(ctx.g, f.node, f.tm, fromRoot, err))
    return false;
  out.attrs.push_back(AttrRecord{d, first, end, int8_t(f.parity ? -1 : 1), f.tm, fromRoot});
  return true;
}

bool Lowering::cut(const Frame &f)
{
  if (parts.back().node == NO_INDEX)
    return true;
  DPoint3 n;
  double d;
  if (!frame_plane(ctx.g, f.node, f.tm, n, d, err))
    return false;

  double inLeaves[2] = {DBL_MAX, -DBL_MAX}, inBox[2] = {DBL_MAX, -DBL_MAX};
  const auto extend = [&](double range[2], const DPoint3 &p) {
    range[0] = min(range[0], n * p - d);
    range[1] = max(range[1], n * p - d);
  };
  for (uint32_t l = f.leafMark; l < out.tree.leaves.size(); ++l)
    for (const DPoint3 &p : out.tree.leaves[l].pos)
      extend(inLeaves, p);
  for (int i = 0; i < 8; ++i)
    extend(inBox, bbox_corner(parts.back().box, i));
  // the result lies within the leaves and within the box: unchanged only when none of it is past the plane, Empty
  // only when all of it is; a tolerance band here would drop or keep a slice thinner than the tolerance
  if (min(inLeaves[1], inBox[1]) <= 0)
    return true;
  if (max(inLeaves[0], inBox[0]) >= 0)
  {
    rollback(f);
    parts.back() = Part();
    return true;
  }

  const Part keep = parts.back();
  parts.pop_back();
  SourceMesh &box = ctx.newSource();
  cut_box(keep.box, n, d, f.node, box);
  if (!addLeaf(box, IDENT_TM, f.node))
    return false;
  const uint32_t boxNode = parts.back().node;
  parts.back().node = addNode(BoolTree::Kind::Intersect, keep.node, boxNode, 0, f.node);
  parts.back().box = keep.box;
  return true;
}

void Lowering::combine(const Frame &f, NodeKind kind)
{
  const Part b = parts.back();
  parts.pop_back();
  const Part a = parts.back();
  parts.pop_back();
  const bool emptyA = a.node == NO_INDEX, emptyB = b.node == NO_INDEX;
  if (kind == NodeKind::Union || kind == NodeKind::Concat)
  {
    if (emptyA || emptyB)
    {
      parts.push_back(emptyA ? b : a);
      return;
    }
  }
  else if (emptyA || (emptyB && kind == NodeKind::Intersect))
  {
    rollback(f);
    parts.push_back(Part());
    return;
  }
  else if (emptyB)
  {
    parts.push_back(a);
    return;
  }

  BoolTree::Kind k = bool_tree_kind(kind);
  if (k == BoolTree::Kind::Union && bboxes_disjoint(a.box, b.box, ctx.kernelParams.tolerance))
    k = BoolTree::Kind::Concat;
  Part r{addNode(k, a.node, b.node, 0, f.node), a.box};
  if (k == BoolTree::Kind::Intersect)
  {
    r.box.lim[0] = max(a.box.lim[0], b.box.lim[0]);
    r.box.lim[1] = min(a.box.lim[1], b.box.lim[1]);
  }
  else if (k != BoolTree::Kind::Subtract)
    add_bbox(r.box, b.box);
  parts.push_back(r);
}

bool lower(LowerContext &ctx, OpResult root, Lowered &out, eastl::string &err)
{
  out = Lowered();
  Lowering lowering(ctx, out, err);
  return lowering.run(root);
}
} // namespace dacsg
