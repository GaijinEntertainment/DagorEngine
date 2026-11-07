// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "kernelExact/arrangement.h"
#include <gameLibs/daParallel/parallel_api.h>
#include <EASTL/sort.h>
#include <float.h>

namespace dacsg::exact
{
struct CdtTri
{
  uint32_t v[3];
  uint32_t adj[3]; // across edge (v[k], v[k+1]): neighbor * 3 + its matching edge, or NO_INDEX
};

// chain[first .. first + count) on the left of a -> b
struct ChainRange
{
  uint32_t a, b, first, count;
};

struct CdtWorkspace
{
  dag::Vector<uint32_t> ids; // local vertex to registry id, ascending
  dag::Vector<uint32_t> order, chain, fan;
  dag::Vector<CdtTri> tris;
  dag::Vector<uint32_t> vertTri, hullNext, hullPrev, hullTri;
  dag::Vector<uint32_t> channel, left, right, inChannel;
  dag::Vector<EdgeKey> outer, fresh; // a directed edge u << 32 | v, the slot it is at
  dag::Vector<eastl::pair<double, uint32_t>> ears;
  dag::Vector<ChainRange> ranges;
  dag::Vector<uint64_t> pairs;
  dag::Vector<uint32_t> memberStart; // runs of pairs by id
  dag::Vector<Constraint> constraints;
  fp::Memo memo; // one group triangulates on one worker, so its repeated orient2d triples hit here
};

namespace
{
struct Cdt
{
  const Arrangement &a;
  const Group &g;
  CdtWorkspace &w;
  eastl::string &err;

  int orient(uint32_t i, uint32_t j, uint32_t k) const
  {
    return orient2d(a.points[w.ids[i]], a.points[w.ids[j]], a.points[w.ids[k]], g.axis, uint32_t(&g - a.groups.data()), &w.memo);
  }
  bool before(uint32_t i, uint32_t j) const
  {
    const int u = (g.axis + 1) % 3, v = (g.axis + 2) % 3;
    const int c = compare_coord(a.points[w.ids[i]], a.points[w.ids[j]], u);
    return c != 0 ? c < 0 : compare_coord(a.points[w.ids[i]], a.points[w.ids[j]], v) < 0;
  }
  uint32_t local(uint32_t id) const { return uint32_t(eastl::lower_bound(w.ids.begin(), w.ids.end(), id) - w.ids.begin()); }

  void collect_points();
  uint32_t add_tri(uint32_t x, uint32_t y, uint32_t z)
  {
    const uint32_t t = w.tris.size();
    w.tris.push_back(CdtTri{{x, y, z}, {NO_INDEX, NO_INDEX, NO_INDEX}});
    for (int k = 0; k < 3; ++k)
      w.vertTri[w.tris[t].v[k]] = t * 3 + k;
    return t;
  }
  void link(uint32_t ta, uint32_t tb)
  {
    w.tris[ta / 3].adj[ta % 3] = tb;
    w.tris[tb / 3].adj[tb % 3] = ta;
  }
  bool start_triangulation();
  bool add_point(uint32_t p, uint32_t q);
  void collect_fan(uint32_t v);
  bool insert_constraint(uint32_t from, uint32_t to, const Line &line, GroupTriangulation &out);
  bool walk(uint32_t from, uint32_t to, uint32_t t, int k, uint32_t &reached);
  bool retriangulate(uint32_t from, uint32_t to);
  double height(uint32_t x, uint32_t y, uint32_t z) const;
  bool fill_side(uint32_t base_a, uint32_t base_b, dag::ConstSpan<uint32_t> chain, uint32_t &next_slot);
  void set_tri(uint32_t slot, uint32_t x, uint32_t y, uint32_t z)
  {
    w.tris[slot] = CdtTri{{x, y, z}, {NO_INDEX, NO_INDEX, NO_INDEX}};
    for (int k = 0; k < 3; ++k)
      w.vertTri[w.tris[slot].v[k]] = slot * 3 + k;
  }
  void emit(GroupTriangulation &out);
};

void Cdt::collect_points()
{
  w.ids.clear();
  w.pairs.clear();
  for (uint32_t i = g.firstTri; i < g.firstTri + g.triCount; ++i)
  {
    const uint32_t t = a.groupTris[i];
    for (uint32_t j = a.triPointStart[t]; j < a.triPointStart[t + 1]; ++j)
      w.pairs.push_back(uint64_t(a.triPoint[j]) << 32 | (i - g.firstTri));
  }
  eastl::sort(w.pairs.begin(), w.pairs.end());
  w.memberStart.clear();
  for (uint32_t i = 0; i < w.pairs.size(); ++i)
    if (i == 0 || w.pairs[i] >> 32 != w.pairs[i - 1] >> 32)
    {
      w.ids.push_back(uint32_t(w.pairs[i] >> 32));
      w.memberStart.push_back(i);
    }
  w.memberStart.push_back(w.pairs.size());
  w.order.resize(w.ids.size());
  for (uint32_t i = 0; i < w.order.size(); ++i)
    w.order[i] = i;
  eastl::sort(w.order.begin(), w.order.end(), [&](uint32_t x, uint32_t y) { return before(x, y); });
}

// the sorted points up to the first one off the line of the first two make a fan of triangles
bool Cdt::start_triangulation()
{
  const uint32_t n = w.order.size();
  w.tris.clear();
  w.vertTri.assign(n, NO_INDEX);
  w.hullNext.assign(n, NO_INDEX);
  w.hullPrev.assign(n, NO_INDEX);
  w.hullTri.assign(n, NO_INDEX);
  w.chain.clear();
  uint32_t i = 0;
  for (; i < n && (i < 2 || orient(w.order[0], w.order[1], w.order[i]) == 0); ++i)
    w.chain.push_back(w.order[i]);
  if (i == n)
  {
    err = "plane group points are collinear";
    return false;
  }
  const uint32_t p = w.order[i], m = w.chain.size();
  const bool left = orient(w.chain[0], w.chain[1], p) > 0;
  for (uint32_t j = 0; j + 1 < m; ++j)
  {
    const uint32_t c0 = w.chain[j], c1 = w.chain[j + 1];
    const uint32_t t = left ? add_tri(c0, c1, p) : add_tri(c1, c0, p);
    if (j > 0)
      link(t * 3 + (left ? 2 : 1), (t - 1) * 3 + (left ? 1 : 2));
    const uint32_t hullStart = left ? c0 : c1;
    w.hullNext[hullStart] = left ? c1 : c0;
    w.hullPrev[left ? c1 : c0] = hullStart;
    w.hullTri[hullStart] = t * 3;
  }
  const uint32_t first = left ? w.chain[0] : w.chain[m - 1], last = left ? w.chain[m - 1] : w.chain[0];
  const uint32_t firstTri = 0, lastTri = m - 2;
  w.hullNext[last] = p;
  w.hullPrev[p] = last;
  w.hullNext[p] = first;
  w.hullPrev[first] = p;
  w.hullTri[last] = (left ? lastTri : firstTri) * 3 + 1;
  w.hullTri[p] = (left ? firstTri : lastTri) * 3 + 2;
  uint32_t q = p;
  for (++i; i < n; ++i)
  {
    if (!add_point(w.order[i], q))
      return false;
    q = w.order[i];
  }
  return true;
}

// q is the last point: at least one of its hull edges sees p
bool Cdt::add_point(uint32_t p, uint32_t q)
{
  uint32_t s = q, e = q;
  while (orient(w.hullPrev[s], s, p) < 0)
    s = w.hullPrev[s];
  while (orient(e, w.hullNext[e], p) < 0)
    e = w.hullNext[e];
  if (s == e)
  {
    err = "a point sees no hull edge";
    return false;
  }
  uint32_t prevTri = NO_INDEX, firstTri = NO_INDEX;
  for (uint32_t x = s; x != e; x = w.hullNext[x])
  {
    const uint32_t y = w.hullNext[x], old = w.hullTri[x];
    const uint32_t t = add_tri(x, p, y);
    link(t * 3 + 2, old);
    if (prevTri != NO_INDEX)
      link(t * 3, prevTri * 3 + 1);
    else
      firstTri = t;
    prevTri = t;
  }
  w.hullNext[s] = p;
  w.hullPrev[p] = s;
  w.hullNext[p] = e;
  w.hullPrev[e] = p;
  w.hullTri[s] = firstTri * 3;
  w.hullTri[p] = prevTri * 3 + 1;
  return true;
}

void Cdt::collect_fan(uint32_t v)
{
  w.fan.clear();
  const uint32_t start = w.vertTri[v];
  for (uint32_t c = start;;)
  {
    w.fan.push_back(c);
    const uint32_t n = w.tris[c / 3].adj[c % 3];
    if (n == NO_INDEX)
      break;
    c = n - n % 3 + (n % 3 + 1) % 3;
    if (c == start)
      return;
  }
  for (uint32_t c = start;;)
  {
    const uint32_t n = w.tris[c / 3].adj[(c % 3 + 2) % 3];
    if (n == NO_INDEX)
      return;
    c = n;
    w.fan.push_back(c);
  }
}

bool Cdt::insert_constraint(uint32_t from, uint32_t to, const Line &line, GroupTriangulation &out)
{
  while (from != to)
  {
    collect_fan(from);
    uint32_t reached = NO_INDEX;
    for (uint32_t c : w.fan)
    {
      const CdtTri &t = w.tris[c / 3];
      const int k = c % 3;
      const uint32_t v1 = t.v[(k + 1) % 3], v2 = t.v[(k + 2) % 3];
      if (v1 == to || v2 == to)
      {
        reached = to;
        break;
      }
      const int o1 = orient(from, v1, to), o2 = orient(from, v2, to);
      if (o1 == 0 && o2 < 0)
        reached = v1;
      else if (o2 == 0 && o1 > 0)
        reached = v2;
      else if (o1 > 0 && o2 < 0)
      {
        if (!walk(from, to, c / 3, k, reached))
          return false;
      }
      else
        continue;
      break;
    }
    if (reached == NO_INDEX)
    {
      err = "a constraint leaves the fan of its vertex";
      return false;
    }
    out.edges.push_back(ConstrainedEdge{{w.ids[from], w.ids[reached]}, line});
    from = reached;
  }
  return true;
}

// from t.v[k] across the opposite edge until a vertex on the segment: the crossed triangles and the vertices on each side
bool Cdt::walk(uint32_t from, uint32_t to, uint32_t t, int k, uint32_t &reached)
{
  w.channel.clear();
  w.left.clear();
  w.right.clear();
  w.channel.push_back(t);
  w.left.push_back(w.tris[t].v[(k + 2) % 3]);
  w.right.push_back(w.tris[t].v[(k + 1) % 3]);
  uint32_t cur = t;
  int ce = (k + 1) % 3;
  for (;;)
  {
    const uint32_t nb = w.tris[cur].adj[ce];
    if (nb == NO_INDEX)
    {
      err = "a constraint leaves the triangulation";
      return false;
    }
    const uint32_t n = nb / 3, x = w.tris[n].v[(nb % 3 + 2) % 3];
    w.channel.push_back(n);
    const int s = orient(from, to, x);
    if (s == 0)
    {
      reached = x;
      return retriangulate(from, x);
    }
    (s > 0 ? w.left : w.right).push_back(x);
    ce = (nb % 3 + (s > 0 ? 1 : 2)) % 3;
    cur = n;
  }
}

static uint64_t directed(uint32_t u, uint32_t v) { return uint64_t(u) << 32 | v; }

static uint32_t find_slot(const dag::Vector<EdgeKey> &edges, uint64_t key)
{
  auto it = eastl::lower_bound(edges.begin(), edges.end(), EdgeKey{key, 0});
  return it != edges.end() && it->key == key ? it->he : NO_INDEX;
}

// Edge flips deadlock at a fan over collinear vertices (every quad is straight), so the channel is cut out instead:
// the polygon on each side of the segment is ear clipped into the slots the crossed triangles held.
bool Cdt::retriangulate(uint32_t from, uint32_t to)
{
  w.outer.clear();
  w.inChannel.resize(w.tris.size());
  for (uint32_t t : w.channel)
    w.inChannel[t] = 1;
  for (uint32_t t : w.channel)
    for (int e = 0; e < 3; ++e)
    {
      const uint32_t nb = w.tris[t].adj[e];
      if (nb == NO_INDEX || !w.inChannel[nb / 3])
        w.outer.push_back(EdgeKey{directed(w.tris[t].v[e], w.tris[t].v[(e + 1) % 3]), nb});
    }
  for (uint32_t t : w.channel)
    w.inChannel[t] = 0;
  eastl::sort(w.outer.begin(), w.outer.end());
  uint32_t next = 0;
  if (!fill_side(from, to, make_span_const(w.left), next))
    return false;
  eastl::reverse(w.right.begin(), w.right.end());
  if (!fill_side(to, from, make_span_const(w.right), next) || next != w.channel.size())
  {
    err = err.empty() ? "the channel polygons do not fill the channel" : err;
    return false;
  }
  w.fresh.clear();
  for (uint32_t t : w.channel)
    for (int e = 0; e < 3; ++e)
      w.fresh.push_back(EdgeKey{directed(w.tris[t].v[e], w.tris[t].v[(e + 1) % 3]), t * 3 + e});
  eastl::sort(w.fresh.begin(), w.fresh.end());
  for (const EdgeKey &f : w.fresh)
  {
    const uint32_t u = uint32_t(f.key >> 32), v = uint32_t(f.key);
    uint32_t other = find_slot(w.fresh, directed(v, u));
    if (other == NO_INDEX)
      other = find_slot(w.outer, f.key);
    w.tris[f.he / 3].adj[f.he % 3] = other;
    if (other != NO_INDEX)
      w.tris[other / 3].adj[other % 3] = f.he;
  }
  return true;
}

// the double area of (x, y, z): ranks the chain by height above the base, never a decision
double Cdt::height(uint32_t x, uint32_t y, uint32_t z) const
{
  const int u = (g.axis + 1) % 3, v = (g.axis + 2) % 3;
  const DPoint3 &px = a.points[w.ids[x]].approx, &py = a.points[w.ids[y]].approx, &pz = a.points[w.ids[z]].approx;
  return (py[u] - px[u]) * (pz[v] - px[v]) - (py[v] - px[v]) * (pz[u] - px[u]);
}

// Anglada's recursion from the lowest chain vertex: every other chain vertex is higher, so its triangle with the base
// is empty and no chain edge crosses it; chain runs from a to b on the left of a -> b
bool Cdt::fill_side(uint32_t base_a, uint32_t base_b, dag::ConstSpan<uint32_t> chain, uint32_t &next_slot)
{
  w.ranges.clear();
  w.ranges.push_back(ChainRange{base_a, base_b, 0, uint32_t(chain.size())});
  while (!w.ranges.empty())
  {
    const ChainRange r = w.ranges.back();
    w.ranges.pop_back();
    if (!r.count)
      continue;
    w.ears.clear();
    for (uint32_t i = r.first; i < r.first + r.count; ++i)
      w.ears.push_back(eastl::pair<double, uint32_t>(height(r.a, r.b, chain[i]), i));
    // the lowest goes first; the rest is sorted only when it fails the exact test
    eastl::swap(w.ears[0], *eastl::min_element(w.ears.begin(), w.ears.end()));
    const int u = (g.axis + 1) % 3, v = (g.axis + 2) % 3;
    uint32_t pick = NO_INDEX;
    for (uint32_t n = 0; n < w.ears.size() && pick == NO_INDEX; ++n)
    {
      if (n == 1)
        eastl::sort(w.ears.begin() + 1, w.ears.end());
      const uint32_t i = w.ears[n].second, c = chain[i];
      if (orient(r.a, r.b, c) <= 0)
        continue;
      // a point outside the triangle's box by more than its error is outside the exact triangle
      double lo[2] = {DBL_MAX, DBL_MAX}, hi[2] = {-DBL_MAX, -DBL_MAX};
      for (uint32_t corner : {r.a, r.b, c})
      {
        const ExactPoint &p = a.points[w.ids[corner]];
        lo[0] = min(lo[0], p.approx[u] - p.err);
        lo[1] = min(lo[1], p.approx[v] - p.err);
        hi[0] = max(hi[0], p.approx[u] + p.err);
        hi[1] = max(hi[1], p.approx[v] + p.err);
      }
      bool empty = true;
      for (uint32_t j = r.first; j < r.first + r.count && empty; ++j)
      {
        const ExactPoint &q = a.points[w.ids[chain[j]]];
        if (j == i || q.approx[u] + q.err < lo[0] || q.approx[u] - q.err > hi[0] || q.approx[v] + q.err < lo[1] ||
            q.approx[v] - q.err > hi[1])
          continue;
        empty = orient(r.a, r.b, chain[j]) < 0 || orient(r.b, c, chain[j]) < 0 || orient(c, r.a, chain[j]) < 0;
      }
      if (empty)
        pick = i;
    }
    if (pick == NO_INDEX || next_slot >= w.channel.size())
    {
      err = "no empty triangle on a channel base";
      return false;
    }
    set_tri(w.channel[next_slot++], r.a, r.b, chain[pick]);
    w.ranges.push_back(ChainRange{r.a, chain[pick], r.first, pick - r.first});
    w.ranges.push_back(ChainRange{chain[pick], r.b, pick + 1, r.first + r.count - pick - 1});
  }
  return true;
}

void Cdt::emit(GroupTriangulation &out)
{
  for (const CdtTri &t : w.tris)
  {
    SubTri s;
    s.firstMember = out.members.size();
    uint32_t i = w.memberStart[t.v[0]], j = w.memberStart[t.v[1]], k = w.memberStart[t.v[2]];
    const uint32_t ie = w.memberStart[t.v[0] + 1], je = w.memberStart[t.v[1] + 1], ke = w.memberStart[t.v[2] + 1];
    while (i < ie && j < je && k < ke)
    {
      const uint32_t mi = uint32_t(w.pairs[i]), mj = uint32_t(w.pairs[j]), mk = uint32_t(w.pairs[k]);
      const uint32_t lo = mi < mj ? (mi < mk ? mi : mk) : (mj < mk ? mj : mk);
      if (mi == lo && mj == lo && mk == lo)
        out.members.push_back(a.groupTris[g.firstTri + lo]);
      i += mi == lo;
      j += mj == lo;
      k += mk == lo;
    }
    s.memberCount = out.members.size() - s.firstMember;
    if (!s.memberCount)
      continue;
    for (int c = 0; c < 3; ++c)
      s.v[c] = w.ids[t.v[c]];
    if (g.sign < 0)
      eastl::swap(s.v[1], s.v[2]);
    out.subTris.push_back(s);
  }
}
} // namespace

// pure: reads a, writes out; the workspace is per thread
static bool triangulate_group(const Arrangement &a, uint32_t group, CdtWorkspace &w, GroupTriangulation &out, eastl::string &err)
{
  out.subTris.clear();
  out.members.clear();
  out.edges.clear();
  Cdt cdt{a, a.groups[group], w, err};
  cdt.collect_points();
  if (!cdt.start_triangulation())
    return false;
  group_constraints(a, group, w.constraints);
  for (const Constraint &c : w.constraints)
  {
    const uint32_t from = cdt.local(c.p[0]), to = cdt.local(c.p[1]);
    if (from >= w.ids.size() || to >= w.ids.size() || w.ids[from] != c.p[0] || w.ids[to] != c.p[1])
    {
      err.sprintf("constraint point %u is not in its plane group", from >= w.ids.size() || w.ids[from] != c.p[0] ? c.p[0] : c.p[1]);
      return false;
    }
    if (!cdt.insert_constraint(from, to, c.line, out))
      return false;
  }
  cdt.emit(out);
  return true;
}

// the constraints a group inserts: its triangle edges and its segments
static uint32_t group_weight(const Arrangement &a, const Group &g)
{
  uint32_t w = 3 * g.triCount;
  for (uint32_t i = g.firstTri; i < g.firstTri + g.triCount; ++i)
    w += a.triSegStart[a.groupTris[i] + 1] - a.triSegStart[a.groupTris[i]];
  return w;
}

bool triangulate_groups(const Arrangement &a, uint32_t workers, Triangulations &out, eastl::string &err)
{
  out = Triangulations();
  // a chunk holds about 1/32 of a worker's share of the constraints, so few heavy groups still spread
  dag::Vector<uint32_t> weight(a.groups.size()), chunkStart;
  uint64_t total = 0;
  for (uint32_t g = 0; g < a.groups.size(); ++g)
    total += weight[g] = group_weight(a, a.groups[g]);
  // a worker pays off per 4096 constraints
  workers = eastl::min(workers, uint32_t(1 + total / 4096));
  const uint64_t per = eastl::max<uint64_t>(total / (32 * workers), 1);
  uint64_t acc = 0;
  chunkStart.push_back(0);
  for (uint32_t g = 0; g < a.groups.size(); ++g)
    if ((acc += weight[g]) >= per && g + 1 < a.groups.size())
    {
      chunkStart.push_back(g + 1);
      acc = 0;
    }
  chunkStart.push_back(a.groups.size());

  struct Done
  {
    uint32_t from[3]; // one group's first subTri, member and edge in the worker's vectors
  };
  struct alignas(64) Worker
  {
    CdtWorkspace ws;
    GroupTriangulation group;
    dag::Vector<SubTri> subTris;
    dag::Vector<uint32_t> members;
    dag::Vector<ConstrainedEdge> edges;
    dag::Vector<Done> done;
    uint32_t failed = NO_INDEX;
    eastl::string err;
    uint64_t fallbacks = 0;
    bool overflow = false;
  };
  workers = eastl::min(workers, uint32_t(chunkStart.size() - 1));
  dag::Vector<Worker> work(workers);
  dag::Vector<daparallel::Chunk> chunks;
  daparallel::parallel_chunks(
    workers, chunkStart.size() - 1, 1, chunks,
    [&](uint32_t begin, uint32_t end, uint32_t worker) {
      Worker &w = work[worker];
      // the first failed group in group order gives the error, as on one thread
      for (uint32_t g = chunkStart[begin]; g < chunkStart[end] && g < w.failed; ++g)
      {
        if (!triangulate_group(a, g, w.ws, w.group, w.err))
        {
          w.failed = g;
          break;
        }
        Done &d = w.done.push_back();
        d.from[0] = w.subTris.size();
        d.from[1] = w.members.size();
        d.from[2] = w.edges.size();
        w.subTris.insert(w.subTris.end(), w.group.subTris.begin(), w.group.subTris.end());
        w.members.insert(w.members.end(), w.group.members.begin(), w.group.members.end());
        w.edges.insert(w.edges.end(), w.group.edges.begin(), w.group.edges.end());
      }
      w.fallbacks += take_exact_fallbacks();
      w.overflow |= take_ex_overflow();
    },
    [&](uint32_t worker) { return uint32_t(work[worker].done.size()); });
  uint32_t failed = NO_INDEX;
  for (const Worker &w : work)
  {
    add_exact_fallbacks(w.fallbacks);
    ex_overflow |= w.overflow;
    if (w.failed < failed)
    {
      failed = w.failed;
      err = w.err;
    }
  }
  if (failed != NO_INDEX)
    return false;
  out.subTriStart.push_back(0);
  out.edgeStart.push_back(0);
  for (const daparallel::Chunk &c : chunks)
    for (uint32_t i = c.from; i < c.to; ++i)
    {
      const Worker &w = work[c.worker];
      const uint32_t end[3] = {uint32_t(w.subTris.size()), uint32_t(w.members.size()), uint32_t(w.edges.size())};
      const uint32_t *from = w.done[i].from, *to = i + 1 < w.done.size() ? w.done[i + 1].from : end;
      const uint32_t memberBase = out.members.size();
      for (uint32_t s = from[0]; s < to[0]; ++s)
      {
        SubTri t = w.subTris[s];
        t.firstMember += memberBase;
        out.subTris.push_back(t);
      }
      out.members.insert(out.members.end(), w.members.begin() + from[1], w.members.begin() + to[1]);
      out.edges.insert(out.edges.end(), w.edges.begin() + from[2], w.edges.begin() + to[2]);
      out.subTriStart.push_back(out.subTris.size());
      out.edgeStart.push_back(out.edges.size());
    }
  return true;
}
} // namespace dacsg::exact
