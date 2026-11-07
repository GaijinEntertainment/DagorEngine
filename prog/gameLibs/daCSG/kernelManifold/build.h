// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "kernelManifold/manifold.h"
#include "mesh/solidUtil.h"
#include <math/dag_Point2.h>
#include <EASTL/string.h>

namespace dacsg
{
struct Edge
{
  uint32_t v0, v1; // v0 < v1
  uint32_t he;     // the halfedge v0 -> v1
};

// An intersection vertex on an edge of one solid, inside a face of the other; or, with face NO_INDEX and s 0, the edge
// split at the other side's crossing vertex that lies on it (split_edges_at_crossings)
struct Crossing
{
  uint32_t edge, face;
  double t;      // along v0 -> v1
  int s;         // winding change along v0 -> v1
  uint32_t vert; // output vertex id
};

struct Side
{
  const Solid &s;
  bool isP;
  int want;              // winding in the other solid of a retained face
  bool flip;             // Q under Subtract
  uint32_t vertBase = 0; // the offset of the operand's vertices in Assembly::pos
  dag::Vector<Edge> edges;
  dag::Vector<uint32_t> edgeOf; // per halfedge
  dag::Vector<int> winding;     // per vertex
  dag::Vector<Crossing> crossings;
  dag::Vector<uint32_t> crossStart; // per edge, size edges + 1
  dag::Vector<uint8_t> boundsWant;  // per crossing: want is among the windings along its edge around it, after order_coincident

  Side(const Solid &solid, bool is_p, int want_w, bool flip_q) : s(solid), isP(is_p), want(want_w), flip(flip_q) {}
  dag::ConstSpan<Crossing> edgeCrossings(uint32_t e) const
  {
    return dag::ConstSpan<Crossing>(crossings.data() + crossStart[e], crossStart[e + 1] - crossStart[e]);
  }
};

// A vertex where face p of P meets face q of Q
struct FacePairVert
{
  uint32_t p, q, vert;
  bool onP;    // the vertex is on an edge of P (else of Q)
  uint32_t he; // the halfedge of the face that holds the edge
  uint32_t crossing;
};

struct NewEdge
{
  uint32_t face; // of the side it belongs to
  uint32_t from, to;
};

// Output triangles with their face data
struct Emitted
{
  dag::Vector<uint32_t> tri;
  dag::Vector<FaceInfo> face;
  dag::Vector<LeafRef> leafRef;
  dag::Vector<uint64_t> facePlaneKey;

  // faces [from, to) of src
  void append(const Emitted &src, uint32_t from, uint32_t to);
  void reserve(uint32_t faces)
  {
    tri.reserve(3 * faces);
    face.reserve(faces);
    leafRef.reserve(faces);
    facePlaneKey.reserve(faces);
  }
};

struct Seg
{
  uint32_t from, to;
};

// segments out of a vertex less those into it
struct VertexFlow
{
  uint32_t vert;
  int outMinusIn;
};

// Buffers that assemble_face reuses from face to face, one set per worker
struct FaceScratch
{
  dag::Vector<Seg> segs;
  dag::Vector<uint8_t> used, queued;
  dag::Vector<uint32_t> split, splitStart, loopVerts, loopStart, outerOf, ids, tris, twin, work;
  dag::Vector<DPoint2> pts;     // per item of loopVerts
  dag::Vector<DPoint2> flatIds; // per item of ids
  dag::Vector<double> area;
  dag::Vector<dag::ConstSpan<DPoint2>> holes;
  dag::Vector<EdgeKey> cornerKeys;
  dag::Vector<VertexFlow> flow;
  dag::Vector<uint32_t> visitMark, visitAt, loopSlots; // by a vertex's slot, the first seg from it; loopSlots per item of loopVerts
  uint32_t loopMark = 0;                               // the mark of the loop being chained, across faces
};

struct Assembly : Emitted
{
  double tolerance = 0;
  dag::Vector<DPoint3> pos;
  dag::Vector<Provenance> provenance; // per vertex of pos
  dag::Vector<uint32_t> remap;        // per vertex of pos: the one vertex of its coincident group, empty for none

  uint32_t id(uint32_t v) const { return remap.empty() ? v : remap[v]; }
};

// Direction of the new edge at the crossing within the face that holds he: +1 it starts there, -1 it ends
int new_edge_direction(const Side &side, uint32_t he, uint32_t crossing);
// Retained pieces of the face boundary and its new edges, chained, triangulated and appended to out
bool assemble_face(const Side &side, uint32_t f, dag::ConstSpan<NewEdge> new_edges, const Assembly &in, Emitted &out,
  FaceScratch &scratch, eastl::string &err);
// Pairs halfedges, splits touching shells, drops unused vertices; closed on success, no vertex normals
bool finish_solid(Assembly &a, Solid &out, eastl::string &err);
// Pairs by vertex id again after an edit that merged vertices; flat faces within the tolerance of an edge pair first
bool pair_and_split(Solid &s, double tolerance, eastl::string &err);
// Drops unused vertices, the rest in the order the halfedges first use them; s carries no vertex normals yet
void compact_vertices(Solid &s);
// Ends with the vertex normals of the result
bool simplify_solid(Solid &s, double tolerance, eastl::string &err);
} // namespace dacsg
