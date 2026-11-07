// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "exec/pipeline.h"
#include "mesh/polygon.h"
#include <gameLibs/daCSG/validate_api.h>
#include <EASTL/sort.h>
#include <math.h>

namespace dacsg
{
namespace
{
// face-plane pairs; a bigger clip goes to the kernel
constexpr uint64_t MAX_CLIP_PAIRS = 20000000;

// Keeps the points of dist <= 0, dist by point; a point within the tolerance of the plane stays where it is
void clip_polygon(dag::Vector<DPoint3> &poly, dag::ConstSpan<double> dist, double tol, dag::Vector<DPoint3> &tmp)
{
  tmp.clear();
  for (uint32_t i = 0, count = poly.size(); i < count; ++i)
  {
    const DPoint3 &a = poly[i], &b = poly[(i + 1) % count];
    const double sa = dist[i], sb = dist[(i + 1) % count];
    if (sa <= tol)
      tmp.push_back(a);
    if ((sa < -tol && sb > tol) || (sa > tol && sb < -tol))
      tmp.push_back(a + (b - a) * (sa / (sa - sb)));
  }
  poly.swap(tmp);
}

// A point within the tolerance of an earlier one takes its id
struct Welder
{
  double tol = 0;
  dag::Vector<DPoint3> pos;
  HashChain cells; // the earlier points of a point's cell

  uint32_t add(const DPoint3 &p)
  {
    const int64_t c[3] = {int64_t(floor(p.x / tol)), int64_t(floor(p.y / tol)), int64_t(floor(p.z / tol))};
    for (int64_t dz = -1; dz <= 1; ++dz)
      for (int64_t dy = -1; dy <= 1; ++dy)
        for (int64_t dx = -1; dx <= 1; ++dx)
          for (uint32_t j = cells.first(cell_key(c[0] + dx, c[1] + dy, c[2] + dz)); j != NO_INDEX; j = cells.next[j])
            if (lengthSq(pos[j] - p) <= tol * tol)
              return j;
    cells.add(cell_key(c[0], c[1], c[2]), pos.size());
    pos.push_back(p);
    return pos.size() - 1;
  }
};

struct Piece
{
  uint32_t leaf, face, first, count; // a convex loop in Clipper::loops
};

uint64_t directed_key(uint32_t a, uint32_t b) { return uint64_t(a) << 32 | b; }

// Each face of each leaf clipped by the other leaves' planes is a piece of one leaf face, so provenance is exact
struct Clipper
{
  dag::ConstSpan<Solid> leaves;
  double tol;
  Welder w;
  dag::Vector<dag::Vector<PlaneD>> planes;
  dag::Vector<Piece> pieces;
  dag::Vector<uint32_t> loops;

  Clipper(dag::ConstSpan<Solid> l, double tolerance) : leaves(l), tol(tolerance) { w.tol = tolerance; }

  bool clipFaces()
  {
    planes.resize(leaves.size());
    for (uint32_t l = 0; l < leaves.size(); ++l)
      for (uint32_t f = 0; f < leaves[l].faceCount(); ++f)
      {
        const DPoint3 n = unit_or_zero(face_cross(leaves[l], f));
        if (!(lengthSq(n) > 0))
          return false;
        planes[l].push_back(PlaneD{n, n * leaves[l].pos[leaves[l].faceVert(f, 0)]});
      }
    dag::Vector<DPoint3> poly, tmp;
    dag::Vector<double> dist;
    for (uint32_t x = 0; x < leaves.size(); ++x)
      for (uint32_t f = 0; f < leaves[x].faceCount(); ++f)
      {
        poly.clear();
        for (uint32_t k = 0; k < 3; ++k)
          poly.push_back(leaves[x].pos[leaves[x].faceVert(f, k)]);
        for (uint32_t y = 0; y < leaves.size() && poly.size() >= 3; ++y)
          for (uint32_t g = 0; y != x && g < planes[y].size() && poly.size() >= 3; ++g)
          {
            const PlaneD &pl = planes[y][g];
            dist.clear();
            for (const DPoint3 &p : poly)
              dist.push_back(pl.n * p - pl.d);
            const double lo = *eastl::min_element(dist.begin(), dist.end()), hi = *eastl::max_element(dist.begin(), dist.end());
            // a face in another leaf's face: the first leaf keeps it, opposite faces bound nothing
            if (lo >= -tol && hi <= tol && (y < x || pl.n * planes[x][f].n < 0))
              poly.clear();
            else if (hi > tol)
              clip_polygon(poly, make_span_const(dist), tol, tmp);
          }
        addPiece(x, f, poly);
      }
    return true;
  }

  void addPiece(uint32_t leaf, uint32_t face, dag::ConstSpan<DPoint3> poly)
  {
    Piece pc{leaf, face, uint32_t(loops.size()), 0};
    for (const DPoint3 &p : poly)
    {
      const uint32_t v = w.add(p);
      if (!pc.count || loops.back() != v)
      {
        loops.push_back(v);
        ++pc.count;
      }
    }
    for (; pc.count > 1 && loops[pc.first] == loops.back(); --pc.count)
      loops.pop_back();
    if (pc.count >= 3)
      pieces.push_back(pc);
    else
      loops.resize(pc.first);
  }

  // A piece edge with no opposite edge has the vertices of the other side's split on it
  void splitTJunctions()
  {
    HashedKeySet<uint64_t, ~uint64_t(0), oa_hashmap_util::MumStepHash<uint64_t>> edges;
    for (const Piece &pc : pieces)
      for (uint32_t i = 0; i < pc.count; ++i)
        edges.insert(directed_key(loops[pc.first + i], loops[pc.first + (i + 1) % pc.count]));
    dag::Vector<uint8_t> isLoose(w.pos.size(), 0);
    dag::Vector<uint32_t> loose;
    for (uint64_t e : edges)
      if (!edges.has_key(e << 32 | e >> 32))
        for (uint32_t v : {uint32_t(e >> 32), uint32_t(e)})
          if (!isLoose[v])
          {
            isLoose[v] = 1;
            loose.push_back(v);
          }
    if (loose.empty())
      return;
    eastl::sort(loose.begin(), loose.end(), [&](uint32_t a, uint32_t b) { return w.pos[a].x < w.pos[b].x; });
    dag::Vector<uint32_t> fixed;
    dag::Vector<eastl::pair<double, uint32_t>> on;
    for (Piece &pc : pieces)
    {
      const uint32_t first = fixed.size();
      for (uint32_t i = 0; i < pc.count; ++i)
      {
        const uint32_t a = loops[pc.first + i], b = loops[pc.first + (i + 1) % pc.count];
        fixed.push_back(a);
        if (edges.has_key(directed_key(b, a)))
          continue;
        const DPoint3 ab = w.pos[b] - w.pos[a];
        const double xMax = max(w.pos[a].x, w.pos[b].x) + tol;
        on.clear();
        for (auto it = eastl::lower_bound(loose.begin(), loose.end(), min(w.pos[a].x, w.pos[b].x) - tol,
               [&](uint32_t v, double x) { return w.pos[v].x < x; });
             it != loose.end() && w.pos[*it].x <= xMax; ++it)
        {
          const uint32_t c = *it;
          const double t = (w.pos[c] - w.pos[a]) * ab / lengthSq(ab);
          if (c != a && c != b && t > 0 && t < 1 && lengthSq(w.pos[a] + ab * t - w.pos[c]) <= tol * tol)
            on.push_back(eastl::make_pair(t, c));
        }
        eastl::sort(on.begin(), on.end());
        for (const auto &o : on)
          fixed.push_back(o.second);
      }
      pc.first = first;
      pc.count = fixed.size() - first;
    }
    loops.swap(fixed);
  }

  bool build(Solid &out)
  {
    out = Solid();
    dag::Vector<uint32_t> vert(w.pos.size(), NO_INDEX), tris;
    dag::Vector<DPoint2> flat;
    eastl::string err;
    auto id = [&](uint32_t welded) {
      if (vert[welded] == NO_INDEX)
      {
        vert[welded] = out.pos.size();
        out.pos.push_back(w.pos[welded]);
      }
      return vert[welded];
    };
    for (const Piece &pc : pieces)
    {
      const Solid &s = leaves[pc.leaf];
      DPoint3 u, v;
      tangent_frame(planes[pc.leaf][pc.face].n, u, v);
      flat.clear();
      for (uint32_t i = 0; i < pc.count; ++i)
        flat.push_back(DPoint2(u * w.pos[loops[pc.first + i]], v * w.pos[loops[pc.first + i]]));
      if (!triangulate_polygon(make_span_const(flat), {}, tris, err, true))
        return false;
      for (uint32_t i = 0; i < tris.size(); i += 3)
      {
        const uint32_t t[3] = {id(loops[pc.first + tris[i]]), id(loops[pc.first + tris[i + 1]]), id(loops[pc.first + tris[i + 2]])};
        for (uint32_t k = 0; k < 3; ++k)
          out.halfedge.push_back(Halfedge{t[k], t[(k + 1) % 3], NO_INDEX});
        out.face.push_back(s.face[pc.face]);
        out.leafRef.push_back(s.leafRef[pc.face]);
        out.facePlaneKey.push_back(s.facePlaneKey[pc.face]);
      }
    }
    out.bbox = points_bbox(out.pos);
    out.provenance.resize(out.pos.size());
    for (uint32_t h = 0; h < out.halfedge.size(); ++h)
      out.provenance[out.halfedge[h].startVert].add(out.facePlaneKey[h / 3]);
    if (!build_halfedges(out, err))
      return false;
    compute_vertex_normals(out);
    out.convex = true;
    return validate(out).valid;
  }

  // a face plane of one leaf with another leaf's whole box on or past it: the intersection has no volume
  bool separated() const
  {
    for (uint32_t y = 0; y < leaves.size(); ++y)
      for (const PlaneD &pl : planes[y])
        for (uint32_t x = 0; x < leaves.size(); ++x)
        {
          bool past = x != y;
          for (int i = 0; i < 8 && past; ++i)
            past = pl.n * bbox_corner(leaves[x].bbox, i) - pl.d >= 0;
          if (past)
            return true;
        }
    return false;
  }
};
} // namespace

bool clip_convex(dag::ConstSpan<Solid> leaves, double tolerance, Solid &out)
{
  uint64_t faces = 0, pairs = 0;
  double maxAbs = 0;
  for (const Solid &leaf : leaves)
  {
    if (!leaf.convex)
      return false;
    pairs += uint64_t(leaf.faceCount()) * faces;
    faces += leaf.faceCount();
    maxAbs = max(maxAbs, leaf.bbox.isempty() ? 0.0 : max_abs(leaf.bbox));
  }
  Clipper c(leaves, tolerance);
  if (leaves.size() < 2 || !(maxAbs < tolerance * MAX_WELD_CELLS) || 2 * pairs > MAX_CLIP_PAIRS || !c.clipFaces())
    return false;
  c.splitTJunctions();
  // Empty by the zero bound only, as a cut: a slice the weld tolerance dropped is the kernel's to decide
  return c.build(out) && (out.faceCount() || c.separated());
}
} // namespace dacsg
