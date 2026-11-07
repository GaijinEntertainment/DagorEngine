// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gameLibs/daCSG/kernel_api.h>
#include <gameLibs/daCSG/solid_api.h>
#include <dag/dag_vector.h>
#include <generic/dag_span.h>
#include <EASTL/string.h>

namespace dacsg
{
// A point of P or Q with its vertex normal; id: its vertex, or NO_INDEX for a point between vertices
struct ShadowPoint //-V730 made per predicate, set before any read
{
  DPoint3 x, n;
  bool isP;
  uint32_t id = NO_INDEX;
};

// An edge with its ends in vertex id order and the sum of its two face normals
struct ShadowEdge
{
  ShadowPoint p[2];
  DPoint3 dir;
};

// a vertex against an edge
struct EdgeShadow //-V730 at is read only when inRange
{
  int s = 0;            // the winding change of the edge around the vertex, 0 when the vertex is not under it
  bool inRange = false; // the edge spans the vertex x, so at holds
  ShadowPoint at;       // the edge point at the vertex x
};

struct Shadow11 //-V730 onP, onQ are read only when s != 0
{
  int s = 0;            // the winding change of the Q edge around a point moving along the P edge in xy
  bool below = false;   // the P edge is below the Q edge at the crossing
  ShadowPoint onP, onQ; // the crossing on each edge, when s is not 0
};

// Smith-Dodgson shadows of (P, Q) in the order x, y, z, with the three kernels of Manifold's boolean.
// A tie p == q reads P's move along the axis, and Q's outward move only where P's is zero (spec 6.3).
// A vertex moves along its vertex normal by expandP, an edge along the sum of its two face normals, a face along its normal.
// A kernel with a Q vertex or edge against P compares with P first; a vertex on a plane or line by provenance is a tie there.
class Shadow
{
public:
  Shadow(const Solid &p, const Solid &q, int expand_p);

  // s: the winding of t around v in xy; below: the pair counts for the winding: a P vertex under the Q face, a P face
  // under the Q vertex (that sum is negated); on_face: the face point over or under v. Public for the kernel tests
  int shadow02(bool v_is_p, uint32_t v, uint32_t t, bool &below, ShadowPoint *on_face = nullptr) const;
  // halfedge he of one solid against face t of the other: the winding change along it, pos and t_along on it.
  // pos interpolates the shadow samples, so it agrees with the decision even when the edge grazes the face
  int intersect12(bool edge_is_p, uint32_t he, uint32_t t, DPoint3 &pos, double &t_along) const;
  // vertex v of one solid against the other: its winding number; tris must hold every face of the other solid over or under
  // v in xy, as a box query of the vertical column through v gives, and any other face adds nothing
  int winding(bool v_is_p, uint32_t v, dag::ConstSpan<uint32_t> tris) const;

private:
  ShadowPoint point(bool is_p, uint32_t v) const;
  ShadowEdge edge(bool is_p, uint32_t he) const;
  // vertex v of one solid against edge e of the other
  EdgeShadow shadow01(const ShadowPoint &v, const ShadowEdge &e) const;
  // the P edge p against the Q edge q
  Shadow11 shadow11(const ShadowEdge &p, const ShadowEdge &q) const;
  // the same as the public shadow02 with the vertex and the edges of t built once by the caller
  int shadow02(const ShadowPoint &p, uint32_t t, const ShadowEdge (&te)[3], bool &below, ShadowPoint *on_face) const;
  void faceEdges(bool is_p, uint32_t t, ShadowEdge (&out)[3]) const;

  // the planes a vertex is known to lie on; null for a point between vertices, a vertex that knows none, or an operand
  // without provenance (one not from the lowering): then only equal coordinates are ties, as in Manifold
  const Provenance *provenance(const ShadowPoint &v) const;
  bool onPlane(const ShadowPoint &v, bool t_is_p, uint32_t t) const;
  bool onLine(const ShadowPoint &v, const ShadowEdge &e) const;
  bool inOnePlane(const ShadowEdge &a, const ShadowEdge &b) const;

  const Solid &P;
  const Solid &Q;
  int expandP;
  dag::Vector<DPoint3> faceN[2]; // unit face normals of Q and P
};

inline uint64_t plane_key(const Solid &s, uint32_t f) { return s.facePlaneKey.empty() ? Provenance::NONE : s.facePlaneKey[f]; }

// op: Union, Subtract or Intersect; an operand with faces must be closed, an empty one counts as closed and gives the
// result without a cut; out is closed on success
bool manifold_boolean(BoolTree::Kind op, const Solid &a, const Solid &b, const KernelParams &params, Solid &out, KernelStats &stats,
  eastl::string &err);
IKernel *manifold_kernel_instance();
} // namespace dacsg
