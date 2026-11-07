// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "mesh/solidUtil.h"
#include <math/dag_check_nan.h>
#include <util/dag_hash.h>
#include <util/dag_hashedKeyMap.h>
#include <EASTL/sort.h>
#include <float.h>
#include <string.h>

namespace dacsg
{
EdgeCounts pair_halfedges(dag::ConstSpan<Halfedge> halfedges, uint32_t vert_count, dag::Vector<uint32_t> &pair)
{
  EdgeCounts c;
  const uint32_t heCount = halfedges.size();
  pair.assign(heCount, NO_INDEX);
  // full sort: slow case on mirrored halves
  dag::Vector<uint32_t> start(vert_count + 1, 0);
  for (const Halfedge &e : halfedges)
    if (e.startVert < vert_count && e.endVert < vert_count && e.startVert != e.endVert)
      ++start[min(e.startVert, e.endVert) + 1];
  for (uint32_t v = 0; v < vert_count; ++v)
    start[v + 1] += start[v];
  dag::Vector<EdgeKey> keys;
  keys.resize_noinit(start[vert_count]);
  for (uint32_t h = 0; h < heCount; ++h)
  {
    const Halfedge &e = halfedges[h];
    if (e.startVert >= vert_count || e.endVert >= vert_count)
      ++c.badIndex;
    else if (e.startVert == e.endVert)
      ++c.unpaired;
    else
      keys[start[min(e.startVert, e.endVert)]++] = EdgeKey{edge_key(e.startVert, e.endVert), h};
  }
  for (uint32_t v = 0, first = 0; v < vert_count; first = start[v++])
    eastl::sort(keys.begin() + first, keys.begin() + start[v]);
  for (uint32_t i = 0; i < keys.size();)
  {
    uint32_t j = i + 1;
    while (j < keys.size() && keys[j].key == keys[i].key)
      ++j;
    ++c.edges;
    const uint32_t a = keys[i].he;
    if (j - i == 1)
      ++c.unpaired;
    else if (j - i > 2)
    {
      c.overPaired += j - i;
      ++c.overPairedEdges;
    }
    else if (halfedges[a].startVert == halfedges[keys[i + 1].he].startVert)
      ++c.misoriented;
    else
    {
      pair[a] = keys[i + 1].he;
      pair[keys[i + 1].he] = a;
    }
    i = j;
  }
  return c;
}

bool build_halfedges(Solid &s, eastl::string &err)
{
  dag::Vector<uint32_t> pair;
  const EdgeCounts c = pair_halfedges(s.halfedge, s.pos.size(), pair);
  for (uint32_t h = 0; h < s.halfedge.size(); ++h)
    s.halfedge[h].pairedHalfedge = pair[h];
  s.closed = c.closed();
  if (!s.closed)
    err.sprintf("not closed: %u unpaired halfedges, %u over-paired halfedges, %u misoriented edges, %u bad vertex indices", c.unpaired,
      c.overPaired, c.misoriented, c.badIndex);
  return s.closed;
}

void index_tuples(const float *data, uint32_t count, uint32_t dim, uint32_t stride, dag::Vector<uint32_t> &ids,
  dag::Vector<float> &values)
{
  // keyed by h >> 1: never the empty key
  HashedKeyMap<uint64_t, uint32_t, ~uint64_t(0), oa_hashmap_util::MumStepHash<uint64_t>> newest;
  dag::Vector<uint32_t> before;
  ids.resize(count);
  values.clear();
  for (uint32_t i = 0; i < count; ++i)
  {
    const float *v = data + i * stride;
    const uint64_t h = mem_hash_fnv1<64>((const char *)v, dim * sizeof(float), FNV1Params<64>::offset_basis, fnv1a_step<64>);
    const auto slot = newest.emplace_if_missing(h >> 1);
    uint32_t t = slot.second ? NO_INDEX : *slot.first;
    while (t != NO_INDEX && memcmp(&values[t * dim], v, dim * sizeof(float)) != 0)
      t = before[t];
    if (t == NO_INDEX)
    {
      before.push_back(slot.second ? NO_INDEX : *slot.first);
      t = *slot.first = before.size() - 1;
      values.insert(values.end(), v, v + dim);
    }
    ids[i] = t;
  }
}

EdgeCounts pair_mesh_edges(const Mesh &m, dag::Vector<Halfedge> &halfedges)
{
  const uint32_t faceCount = eastl::min<uint32_t>(m.face.size(), m.tri.size() / 3);
  halfedges.resize(faceCount * 3);
  for (uint32_t h = 0; h < halfedges.size(); ++h)
    halfedges[h] = Halfedge{m.tri[h], m.tri[next_he(h)], NO_INDEX};
  dag::Vector<uint32_t> pair;
  const EdgeCounts c = pair_halfedges(halfedges, m.pos.size(), pair);
  for (uint32_t h = 0; h < halfedges.size(); ++h)
    halfedges[h].pairedHalfedge = pair[h];
  return c;
}

void compute_vertex_normals(Solid &s)
{
  const uint32_t vertCount = s.pos.size();
  s.vertNormal.assign(vertCount, DPoint3(0, 0, 0));
  for (uint32_t f = 0; f < s.faceCount(); ++f)
  {
    const uint32_t v[3] = {s.faceVert(f, 0), s.faceVert(f, 1), s.faceVert(f, 2)};
    if (v[0] >= vertCount || v[1] >= vertCount || v[2] >= vertCount)
      continue;
    const DPoint3 n = unit_or_zero(face_cross(s, f));
    for (int k = 0; k < 3; ++k)
    {
      const DPoint3 e1 = s.pos[v[(k + 1) % 3]] - s.pos[v[k]];
      const DPoint3 e2 = s.pos[v[(k + 2) % 3]] - s.pos[v[k]];
      s.vertNormal[v[k]] += n * atan2(length(e1 % e2), e1 * e2);
    }
  }
  for (DPoint3 &n : s.vertNormal)
    n = unit_or_zero(n);
}

void mesh_to_solid(const Mesh &m, Solid &s)
{
  s = Solid();
  const uint32_t faceCount = eastl::min<uint32_t>(m.face.size(), m.tri.size() / 3);
  // a position no face uses would widen the box and the tolerances taken from it
  dag::Vector<uint32_t> newId(m.pos.size(), NO_INDEX);
  for (uint32_t c = 0; c < 3 * faceCount; ++c)
    if (m.tri[c] < newId.size())
      newId[m.tri[c]] = 0;
  bool finite = true;
  for (uint32_t v = 0; v < m.pos.size(); ++v)
    if (newId[v] != NO_INDEX)
    {
      newId[v] = s.pos.size();
      s.pos.push_back(DPoint3(m.pos[v]));
      finite = finite && check_finite(m.pos[v].x) && check_finite(m.pos[v].y) && check_finite(m.pos[v].z);
    }
  // a NaN has no order or normal, and the validation counts such positions
  s.bbox = finite ? points_bbox(s.pos) : BBoxD();
  s.face.assign(m.face.begin(), m.face.begin() + faceCount);
  s.leafRef.resize(faceCount);
  for (uint32_t f = 0; f < faceCount; ++f)
    s.leafRef[f] = LeafRef{NO_INDEX, f};
  s.closed = pair_mesh_edges(m, s.halfedge).closed();
  for (Halfedge &e : s.halfedge)
  {
    e.startVert = e.startVert < newId.size() ? newId[e.startVert] : e.startVert;
    e.endVert = e.endVert < newId.size() ? newId[e.endVert] : e.endVert;
  }
  if (finite)
    compute_vertex_normals(s);
  else
    s.vertNormal.assign(s.pos.size(), DPoint3(0, 0, 0));
}

void solid_to_mesh(const Solid &s, Mesh &m)
{
  m = Mesh();
  m.pos.resize(s.pos.size());
  for (uint32_t i = 0; i < s.pos.size(); ++i)
    m.pos[i] = Point3(s.pos[i]);
  m.tri.resize(s.halfedge.size());
  for (uint32_t h = 0; h < s.halfedge.size(); ++h)
    m.tri[h] = s.halfedge[h].startVert;
  m.face = s.face;
}

DPoint3 face_cross(const Solid &s, uint32_t f)
{
  const DPoint3 &p0 = s.pos[s.faceVert(f, 0)];
  return (s.pos[s.faceVert(f, 1)] - p0) % (s.pos[s.faceVert(f, 2)] - p0);
}

double signed_volume(const Solid &s)
{
  const uint32_t vertCount = s.pos.size();
  if (!s.faceCount() || s.faceVert(0, 0) >= vertCount)
    return 0;
  const DPoint3 origin = s.pos[s.faceVert(0, 0)];
  double sum = 0;
  for (uint32_t f = 0; f < s.faceCount(); ++f)
  {
    const uint32_t v0 = s.faceVert(f, 0), v1 = s.faceVert(f, 1), v2 = s.faceVert(f, 2);
    if (v0 < vertCount && v1 < vertCount && v2 < vertCount)
      sum += (s.pos[v0] - origin) * ((s.pos[v1] - origin) % (s.pos[v2] - origin));
  }
  return sum / 6;
}

void append_solid(Solid &dst, const Solid &src)
{
  const uint32_t vertBase = dst.pos.size();
  const uint32_t heBase = dst.halfedge.size();
  dst.pos.insert(dst.pos.end(), src.pos.begin(), src.pos.end());
  dst.vertNormal.insert(dst.vertNormal.end(), src.vertNormal.begin(), src.vertNormal.end());
  dst.face.insert(dst.face.end(), src.face.begin(), src.face.end());
  dst.leafRef.insert(dst.leafRef.end(), src.leafRef.begin(), src.leafRef.end());
  if (!dst.provenance.empty() || !src.provenance.empty())
  {
    dst.provenance.resize(vertBase);
    if (src.provenance.empty())
      dst.provenance.resize(vertBase + src.pos.size());
    else
      dst.provenance.insert(dst.provenance.end(), src.provenance.begin(), src.provenance.end());
  }
  if (!dst.facePlaneKey.empty() || !src.facePlaneKey.empty())
  {
    dst.facePlaneKey.resize(heBase / 3, Provenance::NONE);
    if (src.facePlaneKey.empty())
      dst.facePlaneKey.resize(dst.face.size(), Provenance::NONE);
    else
      dst.facePlaneKey.insert(dst.facePlaneKey.end(), src.facePlaneKey.begin(), src.facePlaneKey.end());
  }
  for (const Halfedge &e : src.halfedge)
    dst.halfedge.push_back(
      Halfedge{e.startVert + vertBase, e.endVert + vertBase, e.pairedHalfedge == NO_INDEX ? NO_INDEX : e.pairedHalfedge + heBase});
  add_bbox(dst.bbox, src.bbox);
  dst.closed = (heBase == 0 || dst.closed) && src.closed;
  dst.convex = false;
  dst.leafIndex = NO_INDEX;
}

// the plane is the unit normal and the offset from the box center, both on a grid of 2^-24 and 2^-30 of the solid's size:
// the triangles of a tessellated polygon land on one cell, a sliver may land elsewhere and stand alone; a flat face stands alone
void compute_face_planes(const Solid &s, dag::Vector<uint32_t> &plane_of_face)
{
  struct Key
  {
    int64_t n[3], d;
    uint32_t face;
    bool operator<(const Key &o) const
    {
      for (int i = 0; i < 3; ++i)
        if (n[i] != o.n[i])
          return n[i] < o.n[i];
      return d != o.d ? d < o.d : face < o.face;
    }
    bool operator==(const Key &o) const { return n[0] == o.n[0] && n[1] == o.n[1] && n[2] == o.n[2] && d == o.d; }
  };
  const uint32_t count = s.faceCount();
  double size = 0;
  for (int i = 0; i < 3; ++i)
    size = max(size, s.bbox.isempty() ? 1.0 : s.bbox.lim[1][i] - s.bbox.lim[0][i]);
  const double nScale = ldexp(1.0, 24), dScale = ldexp(1.0, 30) / (size > 0 ? size : 1.0);
  const DPoint3 center = (s.bbox.lim[0] + s.bbox.lim[1]) * 0.5; // an offset from it fits int64 anywhere
  dag::Vector<Key> keys(count);
  for (uint32_t f = 0; f < count; ++f)
  {
    const DPoint3 n = unit_or_zero(face_cross(s, f));
    const double d = n * (s.pos[s.faceVert(f, 0)] - center);
    keys[f] = Key{{llround(n.x * nScale), llround(n.y * nScale), llround(n.z * nScale)}, llround(d * dScale), f};
  }
  eastl::sort(keys.begin(), keys.end());
  plane_of_face.resize(count);
  for (uint32_t i = 0; i < count; ++i)
  {
    const bool flat = keys[i].n[0] == 0 && keys[i].n[1] == 0 && keys[i].n[2] == 0;
    plane_of_face[keys[i].face] = i > 0 && !flat && keys[i] == keys[i - 1] ? plane_of_face[keys[i - 1].face] : keys[i].face;
  }
}

void round_to_float(Solid &s)
{
  for (DPoint3 &p : s.pos)
    p = DPoint3(Point3(p));
  s.bbox = points_bbox(s.pos);
}

bool indices_valid(const Solid &s)
{
  if (s.halfedge.size() != 3 * s.face.size())
    return false;
  for (const Halfedge &e : s.halfedge)
    if (e.startVert >= s.pos.size() || e.endVert >= s.pos.size())
      return false;
  return true;
}

const char *mesh_contract_breach(const Mesh &m)
{
  if (m.tri.size() != m.face.size() * 3)
    return "the triangle indices are not three per face";
  for (uint32_t v : m.tri)
    if (v >= m.pos.size())
      return "a triangle index is out of range";
  for (const Point3 &p : m.pos)
    if (!check_finite(p.x) || !check_finite(p.y) || !check_finite(p.z))
      return "a position is not finite";
  for (const Channel &c : m.channels)
  {
    const uint8_t dim = channel_dim(c.id) ? channel_dim(c.id) : c.dim;
    if (c.id > CH_UV10 && c.id < CH_USER)
      return "a channel has an id outside the contract";
    if (!c.dim || c.dim != dim)
      return "a channel has the wrong dim";
    if (c.data.size() != m.face.size() * 3 * c.dim)
      return "a channel is not one tuple per corner";
    if (m.findChannel(c.id) != &c)
      return "a channel id appears twice";
    for (float x : c.data)
      if (!check_finite(x))
        return "a channel holds a value that is not finite";
  }
  for (uint32_t ofs : m.userNameOfs)
  {
    if (ofs == Mesh::NO_NAME)
      continue;
    const uint32_t page = m.userNames.get_page_id(ofs), at = m.userNames.get_page_ofs(ofs);
    const StringTableAllocator::StringPage &p = page < m.userNames.pages.size() ? m.userNames.pages[page] : m.userNames.head;
    if (page > m.userNames.pages.size() || at >= p.used || !memchr(p.data.get() + at, 0, p.used - at))
      return "a user channel name is outside the name table";
  }
  return nullptr;
}

bool auto_tolerance(const BBoxD &box, double &tolerance, eastl::string &err)
{
  tolerance = 0;
  if (box.isempty())
    return true;
  // a box holds no NaN: += drops one, and the transformed boxes stay finite, spec.md 11
  const double maxAbs = max_abs(box);
  if (maxAbs > FLT_MAX)
  {
    err.sprintf("the model does not fit the float range: |coordinate| up to %g", maxAbs);
    return false;
  }
  const float f = float(maxAbs);
  const double ulp = f < FLT_MAX ? double(nextafterf(f, FLT_MAX)) - double(f) : double(f) - double(nextafterf(f, 0));
  tolerance = max(1e-7 * length(box.lim[1] - box.lim[0]), 8 * ulp);
  return true;
}

DPoint3 bbox_corner(const BBoxD &b, int i) { return DPoint3(b.lim[i & 1].x, b.lim[(i >> 1) & 1].y, b.lim[(i >> 2) & 1].z); }

void add_bbox(BBoxD &dst, const BBoxD &src)
{
  if (!src.isempty())
  {
    dst += src.lim[0];
    dst += src.lim[1];
  }
}

BBoxD points_bbox(dag::ConstSpan<DPoint3> pos)
{
  BBoxD box;
  for (const DPoint3 &p : pos)
    box += p;
  return box;
}

double max_abs(const BBoxD &box)
{
  double m = 0;
  for (int i = 0; i < 3; ++i)
    m = max(m, max(fabs(box.lim[0][i]), fabs(box.lim[1][i])));
  return m;
}

bool bboxes_disjoint(const BBoxD &a, const BBoxD &b, double tolerance)
{
  for (int i = 0; i < 3; ++i)
    if (a.lim[1][i] + tolerance < b.lim[0][i] - tolerance || b.lim[1][i] + tolerance < a.lim[0][i] - tolerance)
      return true;
  return false;
}

FaceShape triangle_shape(const DPoint3 &a, const DPoint3 &b, const DPoint3 &c)
{
  FaceShape r;
  const DPoint3 p[3] = {a, b, c};
  DPoint3 e[3];
  r.longest = r.shortest = 0;
  for (int k = 0; k < 3; ++k)
  {
    e[k] = p[(k + 1) % 3] - p[k];
    r.edgeLen[k] = length(e[k]);
    r.longest = r.edgeLen[k] > r.edgeLen[r.longest] ? k : r.longest;
    r.shortest = r.edgeLen[k] < r.edgeLen[r.shortest] ? k : r.shortest;
  }
  const double crossLen = length(e[0] % e[1]);
  const int apex = (r.shortest + 2) % 3;
  r.minAngleDeg = atan2(crossLen, e[apex] * (p[(apex + 2) % 3] - p[apex])) * (180 / 3.141592653589793);
  const double len = r.edgeLen[r.longest];
  r.height = len > 0 ? crossLen / len : 0;
  r.foot = len > 0 ? (p[(r.longest + 2) % 3] - p[r.longest]) * e[r.longest] / (len * len) : 0;
  return r;
}

FaceShape face_shape(const Solid &s, uint32_t f)
{
  return triangle_shape(s.pos[s.faceVert(f, 0)], s.pos[s.faceVert(f, 1)], s.pos[s.faceVert(f, 2)]);
}

bool is_sliver(const FaceShape &fs, const CleanupParams &p)
{
  return fs.minAngleDeg < p.sliverAngleDeg || fs.height * p.maxAspect < fs.edgeLen[fs.longest];
}

void face_keys(const Solid &s, dag::Vector<FaceKey> &keys)
{
  dag::Vector<FaceKey> all;
  all.reserve(s.faceCount());
  for (uint32_t f = 0; f < s.faceCount(); ++f)
  {
    uint32_t v[3] = {s.faceVert(f, 0), s.faceVert(f, 1), s.faceVert(f, 2)};
    if (v[0] == v[1] || v[1] == v[2] || v[2] == v[0])
      continue;
    const int lo = v[0] < v[1] ? (v[0] < v[2] ? 0 : 2) : (v[1] < v[2] ? 1 : 2);
    const uint32_t b = v[(lo + 1) % 3], c = v[(lo + 2) % 3];
    all.push_back(FaceKey{{v[lo], b < c ? b : c, b < c ? c : b}, b > c, f});
  }
  // full sort: slow case on mirrored halves
  dag::Vector<uint32_t> start, items;
  bucket_items(all.size(), s.pos.size(), [&](uint32_t i) { return all[i].v[0]; }, start, items);
  keys.resize(all.size());
  for (uint32_t i = 0; i < items.size(); ++i)
    keys[i] = all[items[i]];
  for (uint32_t v = 0; v < s.pos.size(); ++v)
    eastl::sort(keys.begin() + start[v], keys.begin() + start[v + 1]);
}

void skin_pairs(const Solid &s, dag::Vector<TriPair> &pairs)
{
  pairs.clear();
  dag::Vector<FaceKey> keys;
  face_keys(s, keys);
  for (uint32_t i = 0; i < keys.size();)
  {
    uint32_t odd = i, j = i;
    while (j < keys.size() && keys[j].sameVerts(keys[i]))
      odd += !keys[j++].odd;
    for (uint32_t k = 0; i + k < odd && odd + k < j; ++k)
      pairs.push_back(TriPair(keys[i + k].face, keys[odd + k].face));
    i = j;
  }
}

void closest_barycentric(const DPoint3 &p, const DPoint3 &a, const DPoint3 &b, const DPoint3 &c, double w[3])
{
  w[0] = 1;
  w[1] = w[2] = 0;
  const DPoint3 ab = b - a, ac = c - a, ap = p - a;
  const double d1 = ab * ap, d2 = ac * ap;
  if (d1 <= 0 && d2 <= 0)
    return;
  const DPoint3 bp = p - b;
  const double d3 = ab * bp, d4 = ac * bp;
  if (d3 >= 0 && d4 <= d3)
  {
    w[0] = 0;
    w[1] = 1;
    return;
  }
  const double vc = d1 * d4 - d3 * d2;
  if (vc <= 0 && d1 >= 0 && d3 <= 0 && d1 - d3 > 0)
  {
    w[1] = d1 / (d1 - d3);
    w[0] = 1 - w[1];
    return;
  }
  const DPoint3 cp = p - c;
  const double d5 = ab * cp, d6 = ac * cp;
  if (d6 >= 0 && d5 <= d6)
  {
    w[0] = 0;
    w[2] = 1;
    return;
  }
  const double vb = d5 * d2 - d1 * d6;
  if (vb <= 0 && d2 >= 0 && d6 <= 0 && d2 - d6 > 0)
  {
    w[2] = d2 / (d2 - d6);
    w[0] = 1 - w[2];
    return;
  }
  const double va = d3 * d6 - d5 * d4;
  if (va <= 0 && d4 - d3 >= 0 && d5 - d6 >= 0 && (d4 - d3) + (d5 - d6) > 0)
  {
    w[0] = 0;
    w[2] = (d4 - d3) / ((d4 - d3) + (d5 - d6));
    w[1] = 1 - w[2];
    return;
  }
  const double sum = va + vb + vc;
  if (!(sum > 0))
    return;
  w[1] = vb / sum;
  w[2] = vc / sum;
  w[0] = 1 - w[1] - w[2];
}
} // namespace dacsg
