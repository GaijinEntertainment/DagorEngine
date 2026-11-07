// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "mesh/bvh.h"
#include "kernelExact/predicates.h"
#include "kernelExact/snap.h"
#include <gameLibs/daCSG/kernel_api.h>
#include <dag/dag_vector.h>
#include <EASTL/string.h>

namespace dacsg::exact
{
// A point ref names a registry id, or a candidate index with CAND set until assign_ids resolves it.
constexpr uint32_t CAND = 1u << 31;

// a KeyChains key of three integer cell coordinates
inline uint64_t mix_cell(uint64_t x, uint64_t y, uint64_t z)
{
  return x * 0x9E3779B97F4A7C15ull ^ (y * 0xC2B2AE3D27D4EB4Full + 0x165667B19E3779F9ull) ^ (z * 0x27D4EB2F165667C5ull + 0x9E3779B1ull);
}

// a reserve rounded up to a power of two of bytes within mimalloc's medium range, where a size class commits a 512 KB
// page: the vectors of one octave then share it; a larger object commits what it reserves, so it is sized exactly
template <typename V>
void reserve_octave(V &v, size_t count)
{
  constexpr size_t MEDIUM_MAX = 128 << 10;
  size_t bytes = 64;
  while (bytes < count * sizeof(typename V::value_type))
    bytes *= 2;
  v.reserve(bytes <= MEDIUM_MAX ? bytes / sizeof(typename V::value_type) : count);
}

// ids reached from a 64-bit cell key: a flat table of heads, the chain by id; a slot chains every key that lands on it,
// so a reader checks the key or the point
struct KeyChains
{
  dag::Vector<uint32_t> head, next;
  uint32_t mask = 0;

  void init(uint32_t ids, uint32_t keys)
  {
    uint32_t size = 64;
    while (size < 2 * keys)
      size *= 2;
    head.assign(size, NO_INDEX);
    next.resize(ids); // add sets each before any read
    mask = size - 1;
  }
  uint32_t slot(uint64_t key) const { return uint32_t(key ^ (key >> 32)) & mask; }
  uint32_t first(uint64_t key) const { return head[slot(key)]; }
  void add(uint64_t key, uint32_t id)
  {
    next[id] = head[slot(key)];
    head[slot(key)] = id;
  }
};

struct Tri
{
  uint32_t leaf, face;
  uint32_t v[3]; // registry ids
  uint32_t group;
  int8_t orient; // +1: the leaf face normal is along the group normal
};

struct Group
{
  Plane plane;
  uint32_t firstTri, triCount; // range in Arrangement::groupTris
  int axis;                    // projection drops this axis
  int8_t sign;                 // of plane.n[axis]
};

enum class LineKind : uint8_t
{
  Edge,   // a and b are grid ids
  Planes, // a and b are groups
};

struct Line
{
  LineKind kind;
  uint32_t a, b;
};

// an intersection segment on one triangle, stored once per triangle it cuts
struct Segment
{
  uint32_t p[2];
  uint32_t tri;
  Line line;
};

struct Arrangement
{
  // the snap frame on int64grid, identity on shewchuk
  GridFrame frame;
  dag::Vector<ExactPoint> points; // by id, grid points first
  dag::Vector<Tri> tris;
  dag::Vector<Group> groups;
  dag::Vector<uint32_t> groupTris;
  dag::Vector<Segment> segments;
  dag::Vector<ExactPoint> candidates;
  dag::Vector<uint32_t> leafTriBase;
  dag::ConstSpan<uint32_t> leafOf;               // the caller's index of each leaf in the texts; empty: the same
  dag::Vector<uint8_t> leafConvex;               // a convex leaf cannot cross itself
  dag::Vector<uint32_t> pointLeaf;               // by grid id: the leaf of the vertex, NO_INDEX when leaves share it
  dag::Vector<bbox3f> triBox;                    // outward, of the approx corners; find_pairs frees them
  dag::Vector<Bvh> leafBvh;                      // over each leaf's triangles, leaf-local indices
  Bvh leafTree;                                  // over the leaf boxes
  uint32_t pairsHeld = 0;                        // the most triangle pairs find_pairs held at once
  uint32_t pairCap = ~0u;                        // the leaf pairs find_pairs collects before it gives up
  dag::Vector<uint32_t> triPointStart, triPoint; // ids in the closure of each triangle, corners included
  dag::Vector<uint32_t> triSegStart;             // segments sorted by tri
  ExArena arena;                                 // the expansions of the shewchuk backend
  uint64_t pointsByKind[4] = {};
  bool fp = false; // the shewchuk backend: the vertices as given, in doubles
  double unit = 1; // one grid unit in the arrangement's coordinates; fp: the extent over 2^40

  uint32_t callerLeaf(uint32_t l) const { return leafOf.empty() ? l : leafOf[l]; }
};

// Steps of spec 7.2, run by arrange in this order: build_leaves, find_pairs, assign_ids, index_segments, find_crossings,
// assign_ids again (for the crossings' points), insert_points, finish_tri_points.
bool build_leaves(const BoolTree &tree, const KernelParams &p, Arrangement &a, eastl::string &err);
// workers of daParallel; the candidates and segments come out in one order whatever the count; false past
// pairCap pairs of leaves (identical leaves pair quadratically)
bool find_pairs(Arrangement &a, uint32_t workers = 1);
// ids for the candidates: sorted by key, equal points share one id; refs in segments resolve
void assign_ids(Arrangement &a);
// triSegStart; after the segment refs resolve, before find_crossings
void index_segments(Arrangement &a);
// triPoint of every id; then finish_tri_points
void insert_points(const Arrangement &a, dag::Vector<uint64_t> &tri_point_pairs);
void finish_tri_points(Arrangement &a, dag::Vector<uint64_t> &tri_point_pairs);
void find_crossings(Arrangement &a);

struct Constraint
{
  uint32_t p[2]; // p[0] < p[1]
  Line line;
  double lo[2], hi[2]; // projected box
};
// the edges of the group's triangles and the segments on them, each once, sorted by ids
void group_constraints(const Arrangement &a, uint32_t group, dag::Vector<Constraint> &out);

// true when p is on the plane and inside or on the border of t
bool tri_closure_contains(const Arrangement &a, uint32_t tri, const ExactPoint &p);

struct SubTri
{
  uint32_t v[3]; // oriented along the group normal
  uint32_t firstMember, memberCount;
};

// the edges that lie on a constraint, with its line
struct ConstrainedEdge
{
  uint32_t v[2];
  Line line;
};

struct GroupTriangulation
{
  dag::Vector<SubTri> subTris;
  dag::Vector<uint32_t> members; // triangle indices, ranges in SubTri
  dag::Vector<ConstrainedEdge> edges;
};

// every group's triangulation in one store: group g owns subTris [subTriStart[g], subTriStart[g + 1]) and edges likewise;
// a SubTri's firstMember indexes members
struct Triangulations
{
  dag::Vector<SubTri> subTris;
  dag::Vector<uint32_t> members;
  dag::Vector<ConstrainedEdge> edges;
  dag::Vector<uint32_t> subTriStart, edgeStart;
};
// the groups on the workers of daParallel, chunked by their constraint counts; the first failed group gives err
bool triangulate_groups(const Arrangement &a, uint32_t workers, Triangulations &out, eastl::string &err);

struct Face
{
  uint32_t v[3]; // along the group normal
  uint32_t group;
  uint32_t firstMember, memberCount;
};

struct Cells
{
  dag::Vector<Face> faces;
  dag::Vector<uint32_t> members;     // Arrangement::tris indices
  dag::Vector<uint32_t> faceShell;   // 2 per face: front, back
  dag::Vector<int32_t> shellWinding; // shells x leaves
  dag::Vector<uint32_t> edgeStart;   // sorted wedge lists, one range per edge
  dag::Vector<uint32_t> edgeWedge;   // face * 2 + (1 when the face runs hi to lo)
  dag::Vector<uint32_t> faceEdge;    // 3 per face: the edge of corner k to k + 1
  uint32_t shellCount = 0, componentCount = 0, leafCount = 0;
};

// w[leaf] for the cell on the front side of f: the signed count of the leaf triangles the ray through a point of f meets beyond
// it, +1 where it leaves the leaf and -1 where it enters, so a nested outward shell winds past 1
bool seed_winding(const Arrangement &a, const Face &f, int32_t *w);
bool build_cells(const Arrangement &a, const Triangulations &t, Cells &c, eastl::string &err);

struct OutFace
{
  uint32_t face; // arrangement face
  uint32_t v[3]; // registry ids, oriented to the out cell
  LeafRef ref;
};

// The closed 2-complex of the output becomes a Solid: vertices within the tolerance weld (the folds of float
// instances), faces that lose a corner and skin pairs drop, every edge pairs by the wedge order and the vertices
// duplicate per fan; faces keeps the survivors in order; false with a text when a halfedge finds no pair.
bool output_pass(const Arrangement &a, const Cells &c, double tolerance, dag::Vector<OutFace> &faces, Solid &out, eastl::string &err);
bool extract_solid(const Arrangement &a, const BoolTree &tree, const Cells &c, double tolerance, Solid &out, eastl::string &err);
} // namespace dacsg::exact
