// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "kernelManifold/build.h"
#include "mesh/solidUtil.h"
#include "mesh/unionFind.h"
#include <EASTL/sort.h>
#include <math.h>
#include <string.h>

namespace dacsg
{
// A collapse or a flip keeps every live halfedge paired; the skin, flat-shell and pinch steps only mark faces dead,
// so pair_and_split rebuilds the pairing after them. Dead faces are dropped at the end
struct Simplifier
{
  Solid &s;
  double tol;
  dag::Vector<uint8_t> dead;
  dag::Vector<uint32_t> vertHe;
  dag::Vector<uint32_t> ringA, ringB;

  Simplifier(Solid &solid, double tolerance) : s(solid), tol(tolerance) {}

  uint32_t pair(uint32_t h) const { return s.halfedge[h].pairedHalfedge; }
  uint32_t start(uint32_t h) const { return s.halfedge[h].startVert; }
  uint32_t end(uint32_t h) const { return s.halfedge[h].endVert; }
  uint32_t nextOut(uint32_t h) const { return pair(prev_he(h)); }
  uint32_t liveOut(uint32_t h) const
  {
    for (uint32_t i = 0; i < s.faceCount() && dead[h / 3]; ++i)
      h = nextOut(h);
    return h;
  }

  void ring(uint32_t v, dag::Vector<uint32_t> &out) const
  {
    out.clear();
    const uint32_t h0 = vertHe[v];
    for (uint32_t h = h0;;)
    {
      out.push_back(end(h));
      h = nextOut(h);
      if (h == h0 || out.size() > s.faceCount())
        break;
    }
  }

  void init()
  {
    dead.assign(s.faceCount(), 0);
    vertHe.assign(s.pos.size(), NO_INDEX);
    for (uint32_t h = 0; h < s.halfedge.size(); ++h)
      vertHe[start(h)] = h;
  }

  // Both faces of h go; the halfedges across their other edges pair with each other
  void unlinkFace(uint32_t h)
  {
    const uint32_t x = pair(next_he(h)), y = pair(prev_he(h));
    s.halfedge[x].pairedHalfedge = y;
    s.halfedge[y].pairedHalfedge = x;
    dead[h / 3] = 1;
  }

  bool collapse(uint32_t h)
  {
    const uint32_t a = start(h), b = end(h);
    if (a < b)
      return collapse(pair(h));
    const uint32_t c = end(next_he(h)), d = end(next_he(pair(h)));
    if (c == d)
      return false;
    ring(a, ringA);
    ring(b, ringB);
    uint32_t shared = 0;
    for (uint32_t v : ringA)
      for (uint32_t w : ringB)
        if (v == w)
          shared += v == c || v == d ? 1 : 1000;
    if (shared != 2)
      return false;
    const uint32_t hp = pair(h);
    const uint32_t bOut = pair(prev_he(h)), cOut = pair(next_he(h)), dOut = pair(next_he(hp));
    for (uint32_t x = vertHe[a];;)
    {
      s.halfedge[x].startVert = b;
      s.halfedge[prev_he(x)].endVert = b;
      x = nextOut(x);
      if (x == vertHe[a])
        break;
    }
    unlinkFace(h);
    unlinkFace(hp);
    vertHe[b] = liveOut(bOut);
    vertHe[c] = liveOut(cOut);
    vertHe[d] = liveOut(dOut);
    vertHe[a] = NO_INDEX;
    return true;
  }

  // the two faces must be pieces of one leaf face: a flip across a leaf face boundary leaves the face it names
  bool samePiece(uint32_t f, uint32_t g) const
  {
    const FaceInfo &a = s.face[f], &b = s.face[g];
    return s.leafRef[f].leaf == s.leafRef[g].leaf && s.leafRef[f].leafFace == s.leafRef[g].leafFace && a.prim == b.prim &&
           a.srcFace == b.srcFace && a.smgr == b.smgr && a.mat == b.mat && a.attrSet == b.attrSet;
  }

  bool flip(uint32_t h)
  {
    const uint32_t hp = pair(h);
    const uint32_t u = start(h), v = end(h), c = end(next_he(h)), d = end(next_he(hp));
    if (c == d || !samePiece(h / 3, hp / 3))
      return false;
    ring(c, ringA);
    for (uint32_t w : ringA)
      if (w == d)
        return false;
    // both new triangles keep the face's orientation (its neighbour's when it has none): a thin concave pair would turn one over
    const DPoint3 &pu = s.pos[u], &pv = s.pos[v], &pc = s.pos[c], &pd = s.pos[d], own = (pv - pu) % (pc - pu);
    const DPoint3 n = lengthSq(own) > 0 ? own : (pd - pu) % (pv - pu);
    if (!(n * ((pu - pc) % (pd - pc)) > 0) || !(n * ((pv - pd) % (pc - pd)) > 0))
      return false;
    const uint32_t f1 = h - h % 3, f2 = hp - hp % 3;
    const uint32_t cu = pair(prev_he(h)), ud = pair(next_he(hp)), dv = pair(prev_he(hp)), vc = pair(next_he(h));
    s.halfedge[f1] = Halfedge{c, u, cu};
    s.halfedge[f1 + 1] = Halfedge{u, d, ud};
    s.halfedge[f1 + 2] = Halfedge{d, c, f2 + 2};
    s.halfedge[f2] = Halfedge{d, v, dv};
    s.halfedge[f2 + 1] = Halfedge{v, c, vc};
    s.halfedge[f2 + 2] = Halfedge{c, d, f1 + 2};
    s.halfedge[cu].pairedHalfedge = f1;
    s.halfedge[ud].pairedHalfedge = f1 + 1;
    s.halfedge[dv].pairedHalfedge = f2;
    s.halfedge[vc].pairedHalfedge = f2 + 1;
    vertHe[c] = f1;
    vertHe[u] = f1 + 1;
    vertHe[d] = f2;
    vertHe[v] = f2 + 1;
    return true;
  }

  double edgeLength(uint32_t h) const { return length(s.pos[end(h)] - s.pos[start(h)]); }

  bool collapseShortEdges()
  {
    bool any = false;
    for (uint32_t h = 0; h < s.halfedge.size(); ++h)
      if (!dead[h / 3] && h < pair(h) && edgeLength(h) < tol)
        any |= collapse(h);
    return any;
  }

  bool fixFlatFaces()
  {
    bool any = false;
    for (uint32_t f = 0; f < s.faceCount(); ++f)
    {
      if (dead[f])
        continue;
      const DPoint3 cross = face_cross(s, f);
      const double len[3] = {edgeLength(3 * f), edgeLength(3 * f + 1), edgeLength(3 * f + 2)};
      uint32_t longest = 0, shortest = 0;
      for (uint32_t k = 1; k < 3; ++k)
      {
        longest = len[k] > len[longest] ? k : longest;
        shortest = len[k] < len[shortest] ? k : shortest;
      }
      if (!(len[longest] > 0) || length(cross) >= tol * len[longest])
        continue;
      any |= (len[shortest] < tol && collapse(3 * f + shortest)) || flip(3 * f + longest);
    }
    return any;
  }

  static bool less_pos(const DPoint3 &p, const DPoint3 &q) { return p.x != q.x ? p.x < q.x : p.y != q.y ? p.y < q.y : p.z < q.z; }

  // equal positions give equal hashes: -0 counts as 0
  static uint64_t position_hash(const DPoint3 (&p)[3])
  {
    uint64_t h = 0;
    for (const DPoint3 &q : p)
      for (int i = 0; i < 3; ++i)
      {
        uint64_t bits;
        memcpy(&bits, &q[i], sizeof(bits));
        h = (h ^ (bits == 1ull << 63 ? 0 : bits)) * 0x9E3779B97F4A7C15ull;
      }
    return h;
  }

  // A shell of no volume at the tolerance (a flat loop that closed on itself) is nothing
  bool dropFlatShells()
  {
    UnionFind shells;
    shells.init(s.faceCount());
    for (uint32_t h = 0; h < s.halfedge.size(); ++h)
      if (!dead[h / 3] && !dead[pair(h) / 3])
        shells.joinLower(h / 3, pair(h) / 3);
    // the volume relative to a vertex of the shell, so a flat shell gets no rounding of its coordinates
    dag::Vector<double> volume(s.faceCount(), 0), area(s.faceCount(), 0);
    dag::Vector<uint32_t> origin(s.faceCount(), NO_INDEX);
    for (uint32_t f = 0; f < s.faceCount(); ++f)
      if (!dead[f])
      {
        const uint32_t shell = shells.find(f);
        if (origin[shell] == NO_INDEX)
          origin[shell] = s.faceVert(f, 0);
        const DPoint3 &o = s.pos[origin[shell]];
        const DPoint3 a = s.pos[s.faceVert(f, 0)] - o, b = s.pos[s.faceVert(f, 1)] - o, c = s.pos[s.faceVert(f, 2)] - o;
        volume[shell] += (a % b) * c / 6;
        area[shell] += length((b - a) % (c - a)) * 0.5;
      }
    bool any = false;
    for (uint32_t f = 0; f < s.faceCount(); ++f)
      if (!dead[f] && fabs(volume[shells.find(f)]) <= tol * area[shells.find(f)])
        dead[f] = any = true;
    return any;
  }

  // Two faces on one position triple with opposite orientation enclose no volume: both go, and a vertex they
  // share by position only (a fan split around a zero-width strip) is one again; the pairing is rebuilt after
  bool removeSkins()
  {
    struct Key
    {
      DPoint3 p[3];
      uint32_t v[3], f;
      bool even;
    };
    const auto key = [&](uint32_t f) {
      Key k;
      k.f = f;
      k.even = true;
      for (int i = 0; i < 3; ++i)
      {
        k.v[i] = s.faceVert(f, i);
        k.p[i] = s.pos[k.v[i]];
      }
      for (int i = 0; i < 3; ++i)
        for (int j = i + 1; j < 3; ++j)
          if (less_pos(k.p[j], k.p[i]))
          {
            eastl::swap(k.p[i], k.p[j]);
            eastl::swap(k.v[i], k.v[j]);
            k.even = !k.even;
          }
      return k;
    };
    // faces meet by a hash of their positions; only a run of one hash is sorted by the positions
    dag::Vector<eastl::pair<uint64_t, uint32_t>> byHash;
    byHash.reserve(s.faceCount());
    for (uint32_t f = 0; f < s.faceCount(); ++f)
      if (!dead[f])
        byHash.push_back(eastl::make_pair(position_hash(key(f).p), f));
    eastl::sort(byHash.begin(), byHash.end());
    UnionFind verts;
    verts.init(s.pos.size());
    bool any = false, merged = false;
    dag::Vector<Key> keys;
    for (uint32_t r = 0, end = 0; r < byHash.size(); r = end)
    {
      for (end = r + 1; end < byHash.size() && byHash[end].first == byHash[r].first;)
        ++end;
      if (end - r < 2)
        continue;
      keys.clear();
      for (uint32_t i = r; i < end; ++i)
        keys.push_back(key(byHash[i].second));
      eastl::sort(keys.begin(), keys.end(), [](const Key &a, const Key &b) {
        for (int i = 0; i < 3; ++i)
          if (a.p[i] != b.p[i])
            return less_pos(a.p[i], b.p[i]);
        return a.f < b.f;
      });
      for (uint32_t i = 1; i < keys.size(); ++i)
      {
        const Key &a = keys[i - 1], &b = keys[i];
        if (a.p[0] != b.p[0] || a.p[1] != b.p[1] || a.p[2] != b.p[2] || a.even == b.even || dead[a.f] || dead[b.f])
          continue;
        for (int k = 0; k < 3; ++k)
          merged |= verts.joinLower(a.v[k], b.v[k]);
        dead[a.f] = dead[b.f] = 1;
        any = true;
      }
    }
    if (merged)
      mergeVertices(verts);
    return any;
  }

  // Short edges the link condition kept: a contact of two shells; merging them makes the pinch that pair_and_split separates
  bool pinch()
  {
    UnionFind verts;
    verts.init(s.pos.size());
    bool any = false;
    for (uint32_t h = 0; h < s.halfedge.size(); ++h)
      if (!dead[h / 3] && h < pair(h) && edgeLength(h) < tol)
      {
        verts.joinLower(start(h), end(h));
        any = true;
      }
    if (any)
      mergeVertices(verts);
    return any;
  }

  // roots keep shared planes; degenerate faces die
  void mergeVertices(UnionFind &verts)
  {
    for (uint32_t v = 0; v < verts.parent.size() && !s.provenance.empty(); ++v)
    {
      const uint32_t root = verts.find(v);
      if (root != v)
        s.provenance[root].intersect(s.provenance[v]);
    }
    for (Halfedge &e : s.halfedge)
    {
      e.startVert = verts.find(e.startVert);
      e.endVert = verts.find(e.endVert);
    }
    for (uint32_t f = 0; f < s.faceCount(); ++f)
    {
      const uint32_t a = s.faceVert(f, 0), b = s.faceVert(f, 1), c = s.faceVert(f, 2);
      if (a == b || b == c || a == c)
        dead[f] = 1;
    }
  }

  void compact()
  {
    dag::Vector<uint32_t> faceMap(s.faceCount(), NO_INDEX);
    uint32_t live = 0;
    for (uint32_t f = 0; f < s.faceCount(); ++f)
      if (!dead[f])
        faceMap[f] = live++;
    dag::Vector<Halfedge> he;
    dag::Vector<FaceInfo> face;
    dag::Vector<LeafRef> leafRef;
    dag::Vector<uint64_t> facePlaneKey;
    he.reserve(live * 3);
    face.reserve(live);
    leafRef.reserve(live);
    facePlaneKey.reserve(s.facePlaneKey.empty() ? 0 : live);
    for (uint32_t f = 0; f < s.faceCount(); ++f)
    {
      if (dead[f])
        continue;
      for (uint32_t k = 0; k < 3; ++k)
      {
        Halfedge e = s.halfedge[3 * f + k];
        e.pairedHalfedge = faceMap[e.pairedHalfedge / 3] * 3 + e.pairedHalfedge % 3;
        he.push_back(e);
      }
      face.push_back(s.face[f]);
      leafRef.push_back(s.leafRef[f]);
      if (!s.facePlaneKey.empty())
        facePlaneKey.push_back(s.facePlaneKey[f]);
    }
    s.halfedge = eastl::move(he);
    s.face = eastl::move(face);
    s.leafRef = eastl::move(leafRef);
    s.facePlaneKey = eastl::move(facePlaneKey);
    compact_vertices(s);
  }
};

bool simplify_solid(Solid &s, double tolerance, eastl::string &err)
{
  for (int pass = 0; pass < 2; ++pass)
  {
    {
      // the edits below walk one fan per vertex, so the skins and their merged vertices get their own pairing rebuild
      Simplifier pre(s, tolerance);
      pre.dead.assign(s.faceCount(), 0);
      const bool dropped = pre.dropFlatShells();
      if (pre.removeSkins() || dropped)
      {
        pre.compact();
        if (!pair_and_split(s, tolerance, err))
          return false;
      }
    }
    Simplifier sim(s, tolerance);
    sim.init();
    bool edited = false, repair = false;
    for (int round = 0; round < 8; ++round)
    {
      const bool collapse = sim.collapseShortEdges();
      const bool any = sim.fixFlatFaces() || collapse;
      edited |= any;
      if (!any)
        break;
    }
    repair |= edited && sim.dropFlatShells(); // fixFlatFaces collapses too: any edit can flatten a shell
    // no edit since the skins went: no new skin; after the shells, as a removed skin cuts their pairing
    repair |= edited && sim.removeSkins();
    repair |= sim.pinch();
    if (edited || repair)
      sim.compact();
    if (!repair)
      break;
    if (!pair_and_split(s, tolerance, err))
      return false;
  }
  compute_vertex_normals(s); // the edits above do not read normals
  return true;
}
} // namespace dacsg
