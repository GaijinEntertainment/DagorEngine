// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gameLibs/daCSG/mesh_api.h>
#include <dag/dag_vector.h>
#include <math/dag_Point3.h>
#include <util/dag_stdint.h>
#include <EASTL/string.h>
#include <float.h>

namespace dacsg
{
constexpr uint32_t NO_INDEX = ~0u;

struct CleanupParams
{
  double sliverAngleDeg = 0.5;
  double maxAspect = 1000;
  double flipDihedralDeg = 1;
  double maxMoveFactor = 100; // surface move bound in automatic tolerances, at least 2 tolerances (spec 8); a vertex may move further
  bool wideCrossingCheck = false; // an edit's crossing test takes every face near its new faces, not only near the moved vertex
};

struct BBoxD
{
  DPoint3 lim[2] = {DPoint3(DBL_MAX, DBL_MAX, DBL_MAX), DPoint3(-DBL_MAX, -DBL_MAX, -DBL_MAX)};

  bool isempty() const { return lim[0].x > lim[1].x; }
  BBoxD &operator+=(const DPoint3 &p)
  {
    for (int i = 0; i < 3; ++i)
    {
      lim[0][i] = p[i] < lim[0][i] ? p[i] : lim[0][i];
      lim[1][i] = p[i] > lim[1][i] ? p[i] : lim[1][i];
    }
    return *this;
  }
};

struct Halfedge
{
  uint32_t startVert, endVert, pairedHalfedge;
};

// leaf indexes BoolTree::leaves, NO_INDEX for a face of no tree (mesh_to_solid); leafFace is a face of that leaf
struct LeafRef
{
  uint32_t leaf, leafFace;
};

// The leaf planes a vertex lies on, keyed geometry << 32 | plane: a leaf vertex takes the planes of its faces, a
// crossing the two of its edge and the one it crossed. NONE fills the rest; all NONE, or no array in the Solid at
// all, is a vertex that lies on nothing known.
struct Provenance
{
  static constexpr uint64_t NONE = ~uint64_t(0);
  static constexpr int MAX_PLANES = 3;
  uint64_t plane[MAX_PLANES] = {NONE, NONE, NONE};

  static uint64_t key(uint32_t geometry, uint32_t plane) { return (uint64_t(geometry) << 32) | plane; }
  bool has(uint64_t k) const { return plane[0] == k || plane[1] == k || plane[2] == k; }
  int count() const { return int(plane[0] != NONE) + int(plane[1] != NONE) + int(plane[2] != NONE); }
  // false when k is new and no slot is free
  bool add(uint64_t k)
  {
    if (has(k))
      return true;
    for (uint64_t &p : plane)
      if (p == NONE)
      {
        p = k;
        return true;
      }
    return false;
  }
  // keeps the planes in both
  void intersect(const Provenance &o)
  {
    for (uint64_t &p : plane)
      p = o.has(p) ? p : NONE;
  }
};

struct Solid
{
  dag::Vector<DPoint3> pos;
  dag::Vector<DPoint3> vertNormal; // angle-weighted mean of face normals
  dag::Vector<Halfedge> halfedge;
  dag::Vector<FaceInfo> face;
  dag::Vector<LeafRef> leafRef;
  dag::Vector<Provenance> provenance; // per vertex, or empty
  dag::Vector<uint64_t> facePlaneKey; // per face, the key of its leaf plane, or empty
  BBoxD bbox;
  uint32_t leafIndex = NO_INDEX; // in BoolTree::leaves; NO_INDEX: not one leaf, as a join or a boolean result
  bool closed = false;
  bool convex = false;

  uint32_t vertCount() const { return pos.size(); }
  uint32_t faceCount() const { return face.size(); }
  uint32_t faceVert(uint32_t f, uint32_t corner) const { return halfedge[3 * f + corner].startVert; }
};

// Fills pairedHalfedge and sets closed.
bool build_halfedges(Solid &s, eastl::string &err);
void compute_vertex_normals(Solid &s);
// Builds halfedges and normals; check closed after. Positions no face uses are dropped, the others keep their order.
// A used position that is not finite leaves bbox empty and vertNormal zero: a caller checks mesh_contract_breach first.
void mesh_to_solid(const Mesh &m, Solid &s);
// No channels: the resolve step adds them.
void solid_to_mesh(const Solid &s, Mesh &m);
} // namespace dacsg
