// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "graph/graphUtil.h"
#include "mesh/solidUtil.h"
#include <gameLibs/daCSG/graph_api.h>
#include <gameLibs/daCSG/leaf_api.h>
#include <math/dag_check_nan.h>
#include <util/dag_hash.h>
#include <EASTL/algorithm.h>
#include <EASTL/iterator.h>
#include <string.h>

namespace dacsg
{
static_assert(sizeof(PrimDesc) == 40 && sizeof(AttrDesc) == 60 && sizeof(PlaneD) == 32 && sizeof(TMatrixD) == 96,
  "payload bytes are hashed and compared, so a payload has no padding");

struct PrimTypeInfo
{
  const char *name;
  uint8_t params;
  bool seg, points, convex, fold;
};

static const PrimTypeInfo prim_info[] = {
  {"box", 3, false, false, true, false},
  {"rbox", 4, true, false, true, false},
  {"sphere", 1, true, false, true, false},
  {"ellipsoid", 3, true, false, true, false},
  {"icosphere", 1, false, false, true, false},
  {"cubesphere", 1, true, false, true, false},
  {"cylinder", 2, true, false, true, false},
  {"cone", 3, true, false, true, false},
  {"ccyl", 3, true, false, true, false},
  {"capsule", 2, true, false, true, false},
  {"torus", 2, true, false, false, false},
  {"convex", 0, false, true, true, false},
  {"extrude", 1, false, true, false, true},
  {"revolve", 0, true, true, false, true},
  {"tube", 1, true, true, false, true},
  {"loft", 2, false, true, false, true},
  {"leaf", 0, false, false, false, true},
  {"mesh", 0, false, false, false, true},
};
static_assert(eastl::size(prim_info) == size_t(PrimType::Mesh) + 1);

static const char *const node_kind_names[] = {"prim", "union", "subtract", "intersect", "concat", "cut", "transform", "attr", "empty"};
static_assert(eastl::size(node_kind_names) == size_t(NodeKind::Empty) + 1);

const char *prim_type_name(PrimType t) { return prim_info[int(t)].name; }
bool is_convex_prim(PrimType t) { return prim_info[int(t)].convex; }
bool prim_can_fold(PrimType t) { return prim_info[int(t)].fold; }
const char *node_kind_name(NodeKind k) { return node_kind_names[int(k)]; }

static uint64_t hash_bytes(const void *p, size_t size, uint64_t h = FNV1Params<64>::offset_basis)
{
  return mem_hash_fnv1<64>((const char *)p, size, h);
}

static uint16_t clamp_seg(int seg) { return seg == 0 ? 0 : uint16_t(eastl::clamp(seg, 3, 256)); }

static PrimDesc prim_desc(PrimType type, int seg, float p0, float p1 = 0, float p2 = 0, float p3 = 0)
{
  PrimDesc d;
  d.type = type;
  d.seg = clamp_seg(seg);
  d.p[0] = p0;
  d.p[1] = p1;
  d.p[2] = p2;
  d.p[3] = p3;
  return d;
}

template <class P>
static PointRange intern_points(dag::Vector<P> &pts, dag::Vector<PointRange> &ranges, HashChain &by_hash, dag::ConstSpan<P> in)
{
  if (in.empty())
    return PointRange();
  // a span into pts (a part of a stored list) would dangle at the insert below
  if (in.data() >= pts.data() && in.data() < pts.data() + pts.size())
  {
    const dag::Vector<P> kept(in.begin(), in.end());
    return intern_points(pts, ranges, by_hash, make_span_const(kept));
  }
  const uint64_t h = hash_bytes(in.data(), in.size() * sizeof(P));
  for (uint32_t i = by_hash.first(h); i != ~0u; i = by_hash.next[i])
    if (ranges[i].count == in.size() && memcmp(pts.data() + ranges[i].first, in.data(), in.size() * sizeof(P)) == 0)
      return ranges[i];
  by_hash.add(h, ranges.size());
  ranges.push_back(PointRange{pts.size(), in.size()});
  pts.insert(pts.end(), in.begin(), in.end());
  return ranges.back();
}

Graph::Graph() { nodes.push_back(Node()); }

template <class T>
OpResult Graph::intern(Node n, dag::Vector<T> *store, const T *payload)
{
  G_ASSERT_RETURN(n.a.id < nodes.size() && n.b.id < nodes.size(), OpResult());
  const uint32_t key[3] = {uint32_t(n.kind), n.a.id, n.b.id};
  uint64_t h = hash_bytes(key, sizeof(key));
  if (store)
    h = hash_bytes(payload, sizeof(T), h);
  for (uint32_t i = nodeByHash.first(h); i != ~0u; i = nodeByHash.next[i])
  {
    const Node &c = nodes[i];
    if (c.kind == n.kind && c.a == n.a && c.b == n.b && (!store || memcmp(&(*store)[c.payload], payload, sizeof(T)) == 0))
      return OpResult{i};
  }
  if (store)
  {
    n.payload = store->size();
    // a copy: the payload may be an element of the store the push grows
    store->push_back(T(*payload));
  }
  const OpResult r{nodes.size()};
  nodes.push_back(n);
  nodeByHash.add(h, r.id);
  return r;
}

OpResult Graph::prim(const PrimDesc &desc)
{
  G_ASSERT_RETURN(desc.type <= PrimType::Mesh && desc.type != PrimType::Leaf, OpResult());
  const PrimTypeInfo &info = prim_info[int(desc.type)];
  PrimDesc d;
  d.type = desc.type;
  d.angleDeg = desc.type == PrimType::Revolve ? desc.angleDeg : 0;
  eastl::copy_n(desc.p, info.params, d.p);
  if (info.seg)
    d.seg = clamp_seg(desc.seg);
  if (info.points)
    d.points = desc.points;
  if (desc.type == PrimType::Torus)
    d.segMinor = clamp_seg(desc.segMinor);
  else if (desc.type == PrimType::IcoSphere)
    d.level = eastl::min<uint16_t>(desc.level, 7);
  else if (desc.type == PrimType::Mesh)
    d.path = desc.path;
  return intern(Node{NodeKind::Prim}, &prims, &d);
}

OpResult Graph::box(const Point3 &size) { return prim(prim_desc(PrimType::Box, 0, size.x, size.y, size.z)); }
OpResult Graph::rbox(const Point3 &size, float radius, int seg)
{
  return prim(prim_desc(PrimType::RBox, seg, size.x, size.y, size.z, radius));
}
OpResult Graph::sphere(float r, int seg) { return prim(prim_desc(PrimType::Sphere, seg, r)); }
OpResult Graph::ellipsoid(const Point3 &radii, int seg)
{
  return prim(prim_desc(PrimType::Ellipsoid, seg, radii.x, radii.y, radii.z));
}
OpResult Graph::icosphere(float r, int level)
{
  PrimDesc d = prim_desc(PrimType::IcoSphere, 0, r);
  d.level = uint16_t(eastl::clamp(level, 0, 7));
  return prim(d);
}
OpResult Graph::cubesphere(float r, int seg) { return prim(prim_desc(PrimType::CubeSphere, seg, r)); }
OpResult Graph::cylinder(float r, float h, int seg) { return prim(prim_desc(PrimType::Cylinder, seg, r, h)); }
OpResult Graph::cone(float r_bottom, float r_top, float h, int seg)
{
  return prim(prim_desc(PrimType::Cone, seg, r_bottom, r_top, h));
}
OpResult Graph::ccyl(float r, float h, float chamfer, int seg) { return prim(prim_desc(PrimType::CCyl, seg, r, h, chamfer)); }
OpResult Graph::capsule(float r, float h, int seg) { return prim(prim_desc(PrimType::Capsule, seg, r, h)); }
OpResult Graph::torus(float r_major, float r_minor, int seg, int seg_minor)
{
  PrimDesc d = prim_desc(PrimType::Torus, seg, r_major, r_minor);
  d.segMinor = clamp_seg(seg_minor);
  return prim(d);
}
OpResult Graph::convex(dag::ConstSpan<Point3> points)
{
  PrimDesc d = prim_desc(PrimType::Convex, 0, 0);
  d.points = addPoints3(points);
  return prim(d);
}
OpResult Graph::extrude(dag::ConstSpan<Point2> poly, float h)
{
  PrimDesc d = prim_desc(PrimType::Extrude, 0, h);
  d.points = addPoints2(poly);
  return prim(d);
}
OpResult Graph::revolve(dag::ConstSpan<Point2> profile, float angle_deg, int seg)
{
  PrimDesc d = prim_desc(PrimType::Revolve, seg, 0);
  d.angleDeg = angle_deg;
  d.points = addPoints2(profile);
  return prim(d);
}
OpResult Graph::tube(dag::ConstSpan<Point3> path, float r, int seg)
{
  PrimDesc d = prim_desc(PrimType::Tube, seg, r);
  d.points = addPoints3(path);
  return prim(d);
}
OpResult Graph::loft(dag::ConstSpan<Point3> sections, uint32_t section_points, bool closed)
{
  PrimDesc d = prim_desc(PrimType::Loft, 0, float(section_points), closed ? 1.f : 0.f);
  d.points = addPoints3(sections);
  return prim(d);
}
OpResult Graph::mesh(const char *path)
{
  PrimDesc d = prim_desc(PrimType::Mesh, 0, 0);
  d.path = addPath(path);
  return prim(d);
}
OpResult Graph::leaf(ILeafSource *source)
{
  G_ASSERT_RETURN(source, OpResult());
  const uint64_t h = source->contentHash();
  PrimDesc d = prim_desc(PrimType::Leaf, 0, 0);
  d.angleDeg = 0;
  d.path = leafSources.size();
  for (uint32_t i = leafByHash.first(h); i != ~0u && d.path == leafSources.size(); i = leafByHash.next[i])
    if (leafSources[i]->equals(*source))
      d.path = i;
  if (d.path == leafSources.size())
  {
    leafSources.push_back(source);
    leafByHash.add(h, d.path);
  }
  return intern(Node{NodeKind::Prim}, &prims, &d);
}

OpResult Graph::op(NodeKind kind, OpResult a, OpResult b)
{
  G_ASSERT_RETURN(is_op_kind(kind), OpResult());
  return intern<uint8_t>(Node{kind, a, b}, nullptr, nullptr);
}

OpResult Graph::cut(OpResult a, const PlaneD &keep_below) { return intern(Node{NodeKind::Cut, a}, &planes, &keep_below); }

OpResult Graph::transform(OpResult a, const TMatrixD &tm) { return intern(Node{NodeKind::Transform, a}, &tms, &tm); }

OpResult Graph::attr(OpResult a, const AttrDesc &d) { return intern(Node{NodeKind::Attr, a}, &attrs, &d); }

void Graph::setName(OpResult r, const char *name, bool generated)
{
  G_ASSERT_RETURN(r.id < nodes.size() && name, );
  eastl::string kept(name); // the name may be an own name in names, which the resize moves
  if (names.size() <= r.id)
  {
    names.resize(nodes.size());
    generatedName.resize(nodes.size(), 0);
  }
  names[r.id] = eastl::move(kept);
  generatedName[r.id] = generated;
}

void Graph::setSource(OpResult r, const char *file, uint32_t line)
{
  G_ASSERT_RETURN(r.id < nodes.size() && file, );
  if (sources.size() <= r.id)
    sources.resize(nodes.size());
  if (sources[r.id].line)
    return;
  uint32_t f = 0;
  while (f < sourceFiles.size() && sourceFiles[f] != file)
    ++f;
  if (f == sourceFiles.size())
    sourceFiles.push_back(eastl::string(file));
  sources[r.id] = Source{f, line};
}

eastl::string Graph::withSource(eastl::string text, OpResult r) const
{
  if (r.id < sources.size() && sources[r.id].line)
    text.append_sprintf(" (%s:%u)", sourceFiles[sources[r.id].file].c_str(), sources[r.id].line);
  return text;
}

bool Graph::source(OpResult r, const char *&file, uint32_t &line) const
{
  if (r.id >= sources.size() || !sources[r.id].line)
    return false;
  file = sourceFiles[sources[r.id].file].c_str();
  line = sources[r.id].line;
  return true;
}

eastl::string Graph::getName(OpResult r) const
{
  if (r.id < names.size() && !names[r.id].empty())
    return generatedName[r.id] ? names[r.id] : withSource(names[r.id], r);
  return autoName(r);
}

const char *Graph::ownName(OpResult r) const
{
  return r.id < names.size() && !names[r.id].empty() && !generatedName[r.id] ? names[r.id].c_str() : nullptr;
}

eastl::string Graph::autoName(OpResult r) const
{
  const Node &n = node(r);
  const char *type = n.kind == NodeKind::Prim ? prim_type_name(prims[n.payload].type) : node_kind_name(n.kind);
  return withSource(eastl::string(eastl::string::CtorSprintf(), "%s#%u", type, r.id), r);
}

PointRange Graph::addPoints3(dag::ConstSpan<Point3> pts) { return intern_points(pts3, pts3Ranges, pts3ByHash, pts); }
PointRange Graph::addPoints2(dag::ConstSpan<Point2> pts) { return intern_points(pts2, pts2Ranges, pts2ByHash, pts); }

uint32_t Graph::addPath(const char *path)
{
  for (uint32_t i = 0; i < paths.size(); ++i)
    if (paths[i] == path)
      return i;
  paths.push_back(eastl::string(path));
  return paths.size() - 1;
}

bool is_op_kind(NodeKind k)
{
  return k == NodeKind::Union || k == NodeKind::Subtract || k == NodeKind::Intersect || k == NodeKind::Concat;
}

uint32_t node_child_count(NodeKind k)
{
  if (is_op_kind(k))
    return 2;
  return k == NodeKind::Cut || k == NodeKind::Transform || k == NodeKind::Attr ? 1 : 0;
}

int exponent_of(double x)
{
  int e = NOT_FINITE;
  if (check_finite(x))
    frexp(x, &e);
  return e;
}

int exponent_of(const DPoint3 *p, int count)
{
  double largest = 0;
  for (int i = 0; i < count; ++i)
    for (int k = 0; k < 3; ++k)
    {
      if (!check_finite(p[i][k]))
        return NOT_FINITE;
      largest = max(largest, fabs(p[i][k]));
    }
  return exponent_of(largest);
}

bool tm_in_range(const TMatrixD &m)
{
  for (int i = 0; i < 12; ++i)
    if (!in_tm_limit(m.col[i / 3][i % 3]))
      return false;
  return true;
}

bool compose_tm(const TMatrixD &outer, const TMatrixD &inner, TMatrixD &r)
{
  // an entry of the product is then under 3 2^600 + 2^300
  if (!tm_in_range(outer) || !tm_in_range(inner))
    return false;
  TMatrixD m;
  for (int i = 0; i < 3; ++i)
    m.col[i] = outer.col[0] * inner.col[i].x + outer.col[1] * inner.col[i].y + outer.col[2] * inner.col[i].z;
  m.col[3] = transform_point(outer, inner.col[3]);
  if (!tm_in_range(m))
    return false;
  r = m;
  return true;
}

bool transform_bbox(const TMatrixD &m, const BBoxD &box, BBoxD &out)
{
  if (box.isempty())
    return true;
  // a corner's coordinate is under the sum of 2^(axis j + box j) over j, and 2^origin
  for (int j = 0; j < 3; ++j)
    if (exponent_of(&m.col[j], 1) + exponent_of(max(fabs(box.lim[0][j]), fabs(box.lim[1][j]))) > FAR_EXPONENT - 3)
      return false;
  if (exponent_of(&m.col[3], 1) > FAR_EXPONENT - 1)
    return false;
  for (int i = 0; i < 8; ++i)
    out += transform_point(m, bbox_corner(box, i));
  return true;
}

bool bounded_product(double a, double b, double &r)
{
  if (exponent_of(a) + exponent_of(b) > FAR_EXPONENT)
    return false;
  r = a * b;
  return true;
}

bool compose_uv_affine(const float outer[2][3], float inner[2][3])
{
  float r[2][3];
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 3; ++j)
    {
      const double w = double(outer[i][0]) * inner[0][j] + double(outer[i][1]) * inner[1][j];
      if (!narrow_float(j == 2 ? w + outer[i][2] : w, r[i][j]))
        return false;
    }
  memcpy(inner, r, sizeof(r));
  return true;
}

bool uv_affine_finite(const float m[2][3])
{
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 3; ++j)
      if (!check_finite(m[i][j]))
        return false;
  return true;
}

bool leaf_bounds(const ILeafSource &source, BBoxD &box, double &reach)
{
  const BBox3 local = source.bbox();
  const DPoint3 lim[2] = {DPoint3(local.lim[0]), DPoint3(local.lim[1])};
  const double r = source.reach();
  if (!check_finite(r) || exponent_of(lim, 2) == NOT_FINITE)
    return false;
  reach = r;
  box = BBoxD();
  if (!local.isempty())
  {
    box += lim[0];
    box += lim[1];
  }
  return true;
}

// the projections are a contiguous run: UvPlanar, UvBox, UvCyl, UvSphere
static_assert(int(AttrKind::UvBox) == int(AttrKind::UvPlanar) + 1 && int(AttrKind::UvSphere) == int(AttrKind::UvPlanar) + 3);

static const char *bad_attr(const AttrDesc &d)
{
  const bool projection = d.kind >= AttrKind::UvPlanar && d.kind <= AttrKind::UvSphere;
  for (float c : d.color)
    if (!check_finite(c))
      return "the color is not finite";
  if (!uv_affine_finite(d.uvAffine))
    return "the uv affine is not finite";
  if (projection && d.kind != AttrKind::UvSphere && (!check_finite(d.scale) || !(d.scale > 0)))
    return "the uv scale must be above zero";
  const double axisLen = length(DPoint3(d.axis));
  if (projection && (!check_finite(axisLen) || !(axisLen > 0)))
    return "the projection axis is zero";
  return nullptr;
}

bool check_node_values(const Graph &g, OpResult r, BBoxD &box, double &reach, eastl::string &err)
{
  const NodeKind kind = g.node(r).kind;
  const ILeafSource *leaf =
    kind == NodeKind::Prim && g.primDesc(r).type == PrimType::Leaf ? g.leafSource(g.primDesc(r).path) : nullptr;
  if (leaf && !leaf_bounds(*leaf, box, reach))
    return (err.sprintf("%s: %s: the bbox or the reach is not finite", g.getName(r).c_str(), leaf->name()), false);
  const PlaneD *plane = kind == NodeKind::Cut ? &g.plane(r) : nullptr;
  const char *why = kind == NodeKind::Attr ? bad_attr(g.attrDesc(r)) : nullptr;
  if (kind == NodeKind::Transform && !tm_in_range(g.tm(r)))
    why = "the transform is not finite or has an entry past 2^300";
  if (plane && !(in_tm_limit(plane->n.x) && in_tm_limit(plane->n.y) && in_tm_limit(plane->n.z) && in_tm_limit(plane->d)))
    why = "the cut plane is not finite or has a value past 2^300";
  if (why)
    err = g.getName(r) + ": " + why;
  return !why;
}
} // namespace dacsg
