// Copyright (C) Gaijin Games KFT.  All rights reserved.

// A translation of Manifold's boolean3.cpp (Apache-2.0), except ties: P's move decides, Q's only where P's is zero (spec 6.3).

#include "kernelManifold/manifold.h"
#include "mesh/solidUtil.h"

namespace dacsg
{
// p before q in the perturbed order: on a tie P moves by p_move along the axis, and where it does not, Q by q_move
static bool shadows(double p, double q, double p_move, double q_move) { return p == q ? (p_move != 0 ? p_move : -q_move) < 0 : p < q; }

// the point of the segment lo-hi at coordinate x along axis, computed from the nearer end so the rounding stays small
// there, and with that coordinate set to x itself: a later comparison at x then meets the same value; an edge with
// no extent along axis is read at lo
static ShadowPoint interpolate(const ShadowPoint &lo, const ShadowPoint &hi, int axis, double x)
{
  const double den = hi.x[axis] - lo.x[axis], dLo = x - lo.x[axis], dHi = x - hi.x[axis];
  const bool fromLo = fabs(dLo) <= fabs(dHi);
  const double raw = den != 0 ? (fromLo ? dLo : dHi) / den : 0, lim = fromLo ? 1 : -1;
  const double t = raw * lim < 0 ? 0 : raw * lim > 1 ? lim : raw;
  ShadowPoint at = fromLo ? lo : hi;
  at.x += (hi.x - lo.x) * t;
  at.x[axis] = x;
  at.n = DPoint3(0, 0, 0);
  at.id = NO_INDEX;
  return at;
}

// Manifold's Intersect; out: x, y, z on a, z on b
static void intersect_samples(const DPoint3 &aL, const DPoint3 &aR, const DPoint3 &bL, const DPoint3 &bR, double out[4])
{
  const double dyL = bL.y - aL.y, dyR = bR.y - aR.y;
  const bool useL = fabs(dyL) < fabs(dyR);
  const double dx = aR.x - aL.x;
  const double den = dyL - dyR, lambda = den != 0 ? (useL ? dyL : dyR) / den : 0;
  out[0] = lambda * dx + (useL ? aL.x : aR.x);
  const double aDy = aR.y - aL.y, bDy = bR.y - bL.y;
  const bool useA = fabs(aDy) < fabs(bDy);
  out[1] = lambda * (useA ? aDy : bDy) + (useL ? (useA ? aL.y : bL.y) : (useA ? aR.y : bR.y));
  out[2] = lambda * (aR.z - aL.z) + (useL ? aL.z : aR.z);
  out[3] = lambda * (bR.z - bL.z) + (useL ? bL.z : bR.z);
}

Shadow::Shadow(const Solid &p, const Solid &q, int expand_p) : P(p), Q(q), expandP(expand_p)
{
  for (int side = 0; side < 2; ++side)
  {
    const Solid &s = side ? P : Q;
    faceN[side].resize(s.faceCount());
    for (uint32_t f = 0; f < s.faceCount(); ++f)
      faceN[side][f] = unit_or_zero(face_cross(s, f));
  }
}

ShadowPoint Shadow::point(bool is_p, uint32_t v) const
{
  const Solid &s = is_p ? P : Q;
  return ShadowPoint{s.pos[v], s.vertNormal[v], is_p, v};
}

ShadowEdge Shadow::edge(bool is_p, uint32_t he) const
{
  const Solid &s = is_p ? P : Q;
  const uint32_t a = s.halfedge[he].startVert, b = s.halfedge[he].endVert, twin = s.halfedge[he].pairedHalfedge;
  ShadowEdge e;
  e.p[0] = point(is_p, a < b ? a : b);
  e.p[1] = point(is_p, a < b ? b : a);
  e.dir = faceN[is_p][he / 3] + (twin == NO_INDEX ? DPoint3(0, 0, 0) : faceN[is_p][twin / 3]);
  return e;
}

const Provenance *Shadow::provenance(const ShadowPoint &v) const
{
  const Solid &s = v.isP ? P : Q;
  return v.id != NO_INDEX && !s.provenance.empty() && s.provenance[v.id].count() ? &s.provenance[v.id] : nullptr;
}

// a NONE key is no plane: a vertex with a free slot would read it as one
bool Shadow::onPlane(const ShadowPoint &v, bool t_is_p, uint32_t t) const
{
  const Provenance *p = provenance(v);
  const uint64_t key = plane_key(t_is_p ? P : Q, t);
  return key != Provenance::NONE && p && p->has(key);
}

// v lies on the line of e when it shares two planes of one geometry with both ends: two distinct keys of one leaf
// are two distinct planes, two keys of different leaves can be one plane
bool Shadow::onLine(const ShadowPoint &v, const ShadowEdge &e) const
{
  const Provenance *pv = provenance(v), *pl = provenance(e.p[0]), *ph = provenance(e.p[1]);
  if (!pv || !pl || !ph)
    return false;
  for (uint64_t a : pl->plane)
    for (uint64_t b : pl->plane)
      if (a != b && a != Provenance::NONE && b != Provenance::NONE && (a >> 32) == (b >> 32) && ph->has(a) && ph->has(b) &&
          pv->has(a) && pv->has(b))
        return true;
  return false;
}

bool Shadow::inOnePlane(const ShadowEdge &a, const ShadowEdge &b) const
{
  const Provenance *p0 = provenance(a.p[0]), *p1 = provenance(a.p[1]), *q0 = provenance(b.p[0]), *q1 = provenance(b.p[1]);
  if (!p0 || !p1 || !q0 || !q1)
    return false;
  for (uint64_t k : p0->plane)
    if (k != Provenance::NONE && p1->has(k) && q0->has(k) && q1->has(k))
      return true;
  return false;
}

// Manifold's Shadow01: the x order of the vertex and the ends, read with the P point first; then the vertex under the edge point in y
EdgeShadow Shadow::shadow01(const ShadowPoint &v, const ShadowEdge &e) const
{
  const ShadowPoint &lo = e.p[0], &hi = e.p[1];
  EdgeShadow r;
  if (v.isP)
  {
    const double own = expandP * v.n.x;
    r.s = int(shadows(v.x.x, hi.x.x, own, hi.n.x)) - int(shadows(v.x.x, lo.x.x, own, lo.n.x));
  }
  else
    r.s = int(shadows(lo.x.x, v.x.x, expandP * lo.n.x, v.n.x)) - int(shadows(hi.x.x, v.x.x, expandP * hi.n.x, v.n.x));
  if (!r.s)
    return r;
  r.inRange = true;
  r.at = interpolate(lo, hi, 0, v.x.x);
  // a vertex on the edge's line meets the edge at its own y and z: a tie the perturbation decides
  if (onLine(v, e))
  {
    r.at.x.y = v.x.y;
    r.at.x.z = v.x.z;
  }
  const bool under = v.isP ? shadows(v.x.y, r.at.x.y, expandP * v.n.y, e.dir.y) : shadows(r.at.x.y, v.x.y, expandP * e.dir.y, v.n.y);
  if (!under)
    r.s = 0;
  return r;
}

// Manifold's Kernel11: the P edge against the Q edge
Shadow11 Shadow::shadow11(const ShadowEdge &p, const ShadowEdge &q) const
{
  Shadow11 r;
  DPoint3 onP[2], onQ[2];
  int k = 0;
  bool shadowed = false;
  // either the left or the right sample shadows, not both: the crossing lies between them
  const auto sample = [&](const EdgeShadow &s01, const DPoint3 &vert, bool vert_is_p) {
    if (k < 2 && (k == 0 || (s01.s != 0) != shadowed))
    {
      shadowed = s01.s != 0;
      onP[k] = vert_is_p ? vert : s01.at.x;
      onQ[k] = vert_is_p ? s01.at.x : vert;
      ++k;
    }
  };
  for (int i = 0; i < 2; ++i)
  {
    const EdgeShadow s01 = shadow01(p.p[i], q);
    if (!s01.inRange)
      continue;
    r.s += s01.s * (i == 0 ? -1 : 1);
    sample(s01, p.p[i].x, true);
  }
  for (int i = 0; i < 2; ++i)
  {
    const EdgeShadow s10 = shadow01(q.p[i], p);
    if (!s10.inRange)
      continue;
    r.s += s10.s * (i == 0 ? -1 : 1);
    sample(s10, q.p[i].x, false);
  }
  if (!r.s)
    return r;
  if (k < 2)
  {
    onP[1] = onP[0];
    onQ[1] = onQ[0];
  }
  double xyzz[4];
  intersect_samples(onP[0], onP[1], onQ[0], onQ[1], xyzz);
  r.onP = ShadowPoint{DPoint3(xyzz[0], xyzz[1], xyzz[2]), DPoint3(0, 0, 0), true};
  r.onQ = ShadowPoint{DPoint3(xyzz[0], xyzz[1], xyzz[3]), DPoint3(0, 0, 0), false};
  // two edges in one plane meet at one z: a tie the perturbation decides
  if (inOnePlane(p, q))
    r.onQ.x.z = r.onP.x.z;
  r.below = shadows(r.onP.x.z, r.onQ.x.z, expandP * p.dir.z, q.dir.z);
  return r;
}

void Shadow::faceEdges(bool is_p, uint32_t t, ShadowEdge (&out)[3]) const
{
  for (int i = 0; i < 3; ++i)
    out[i] = edge(is_p, 3 * t + i);
}

int Shadow::shadow02(bool v_is_p, uint32_t v, uint32_t t, bool &below, ShadowPoint *on_face) const
{
  ShadowEdge te[3];
  faceEdges(!v_is_p, t, te);
  return shadow02(point(v_is_p, v), t, te, below, on_face);
}

// Manifold's Kernel02; te: the edges of t, faceEdges(!p.isP, t)
int Shadow::shadow02(const ShadowPoint &p, uint32_t t, const ShadowEdge (&te)[3], bool &below, ShadowPoint *on_face) const
{
  const bool v_is_p = p.isP;
  const Solid &ts = v_is_p ? Q : P;
  ShadowPoint at[2];
  int s = 0, k = 0;
  bool shadowed = false;
  for (int i = 0; i < 3; ++i)
  {
    const uint32_t he = 3 * t + i;
    const bool isForward = ts.halfedge[he].startVert < ts.halfedge[he].endVert;
    const EdgeShadow s01 = shadow01(p, te[i]);
    if (!s01.inRange)
      continue;
    s += s01.s * (v_is_p == isForward ? -1 : 1);
    if (k < 2 && (k == 0 || (s01.s != 0) != shadowed))
    {
      shadowed = s01.s != 0;
      at[k++] = s01.at;
    }
  }
  below = false;
  if (!s)
    return 0;
  ShadowPoint z = k == 2 ? interpolate(at[0], at[1], 1, p.x.y) : at[0];
  // a vertex on the face's plane is at the face's z: a tie the perturbation decides
  if (onPlane(p, !v_is_p, t))
    z.x.z = p.x.z;
  const double nz = faceN[!v_is_p][t].z;
  below = v_is_p ? shadows(p.x.z, z.x.z, expandP * p.n.z, nz) : shadows(z.x.z, p.x.z, expandP * nz, p.n.z);
  if (on_face)
    *on_face = z;
  return s;
}

// Manifold's Kernel12
int Shadow::intersect12(bool edge_is_p, uint32_t he, uint32_t t, DPoint3 &pos, double &t_along) const
{
  const Solid &es = edge_is_p ? P : Q, &ts = edge_is_p ? Q : P;
  const ShadowEdge e = edge(edge_is_p, he);
  ShadowEdge te[3];
  faceEdges(!edge_is_p, t, te);
  const uint32_t start = es.halfedge[he].startVert;
  // samples as (x, z, y): the gap to the face is in z, the crossing is read along the edge in x and y
  DPoint3 onEdge[2], onFace[2];
  int k = 0, s = 0;
  bool shadowed = false;
  const auto sample = [&](bool hit, const DPoint3 &xzy_edge, double face_z) {
    if (k < 2 && (k == 0 || hit != shadowed))
    {
      shadowed = hit;
      onEdge[k] = xzy_edge;
      onFace[k] = DPoint3(xzy_edge.x, face_z, xzy_edge.z);
      ++k;
    }
  };
  for (int i = 0; i < 2; ++i)
  {
    bool below = false;
    ShadowPoint z;
    const int s02 = shadow02(e.p[i], t, te, below, &z);
    if (!s02)
      continue;
    if (below)
      s += s02 * ((e.p[i].id == start) == edge_is_p ? 1 : -1);
    sample(below, DPoint3(e.p[i].x.x, e.p[i].x.z, e.p[i].x.y), z.x.z);
  }
  for (int i = 0; i < 3; ++i)
  {
    const uint32_t the = 3 * t + i;
    const bool isForward = ts.halfedge[the].startVert < ts.halfedge[the].endVert;
    const Shadow11 r = edge_is_p ? shadow11(e, te[i]) : shadow11(te[i], e);
    if (!r.s)
      continue;
    if (r.below)
      s -= r.s * (isForward ? 1 : -1);
    const DPoint3 &own = edge_is_p ? r.onP.x : r.onQ.x, &other = edge_is_p ? r.onQ.x : r.onP.x;
    sample(r.below, DPoint3(own.x, own.z, own.y), other.z);
  }
  if (!s)
    return 0;
  if (k < 2)
  {
    onEdge[1] = onEdge[0];
    onFace[1] = onFace[0];
  }
  double xzyy[4];
  intersect_samples(onEdge[0], onEdge[1], onFace[0], onFace[1], xzyy);
  pos = DPoint3(xzyy[0], xzyy[2], xzyy[1]);
  const DPoint3 &a = es.pos[start], d = es.pos[es.halfedge[he].endVert] - a;
  const double dd = d * d, u = dd > 0 ? ((pos - a) * d) / dd : 0;
  t_along = u < 0 ? 0 : u > 1 ? 1 : u;
  // Manifold's x12 counts the other way: it is the inclusion the crossing takes from the edge
  return -s;
}

// a P vertex counts the Q faces above it, a Q vertex the P faces below it with the sign reversed, as Manifold does
int Shadow::winding(bool v_is_p, uint32_t v, dag::ConstSpan<uint32_t> tris) const
{
  int w = 0;
  for (uint32_t t : tris)
  {
    bool below = false;
    const int s = shadow02(v_is_p, v, t, below);
    if (below)
      w += v_is_p ? s : -s;
  }
  return w;
}
} // namespace dacsg
