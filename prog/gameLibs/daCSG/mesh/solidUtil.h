// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gameLibs/daCSG/solid_api.h>
#include <dag/dag_vector.h>
#include <generic/dag_span.h>
#include <EASTL/string.h>
#include <EASTL/utility.h>
#include <math.h>

namespace dacsg
{
using TriPair = eastl::pair<uint32_t, uint32_t>;

struct EdgeCounts
{
  uint32_t edges = 0;
  uint32_t unpaired = 0;
  uint32_t overPaired = 0;
  uint32_t overPairedEdges = 0;
  uint32_t misoriented = 0;
  uint32_t badIndex = 0;

  uint32_t defects() const { return unpaired + overPaired + misoriented + badIndex; }
  bool closed() const { return defects() == 0; }
};

// a halfedge or a corner under the key of its edge
struct EdgeKey
{
  uint64_t key;
  uint32_t he;

  bool operator<(const EdgeKey &o) const { return key != o.key ? key < o.key : he < o.he; }
};

// a face by its sorted vertices; odd: the face runs against their order
struct FaceKey
{
  uint32_t v[3], odd, face;

  bool sameVerts(const FaceKey &o) const { return v[0] == o.v[0] && v[1] == o.v[1] && v[2] == o.v[2]; }
  bool operator<(const FaceKey &o) const
  {
    for (int i = 0; i < 3; ++i)
      if (v[i] != o.v[i])
        return v[i] < o.v[i];
    return odd != o.odd ? odd < o.odd : face < o.face;
  }
};

struct FaceShape
{
  double edgeLen[3]; // edge k runs from corner k to corner k + 1
  int longest, shortest;
  double minAngleDeg;
  double height; // of the corner opposite the longest edge
  double foot;   // its projection on the longest edge: 0 at the edge start, 1 at the end
};

// pair[h]: opposite halfedge of h or NO_INDEX
EdgeCounts pair_halfedges(dag::ConstSpan<Halfedge> halfedges, uint32_t vert_count, dag::Vector<uint32_t> &pair);
// tuple i of dim floats starts at data + i * stride; ids[i]: its first-seen index among the bitwise distinct tuples,
// which values holds in that order
void index_tuples(const float *data, uint32_t count, uint32_t dim, uint32_t stride, dag::Vector<uint32_t> &ids,
  dag::Vector<float> &values);
// halfedges[3 * f + k] runs from corner k of m's face f to corner k + 1, paired by pair_halfedges
EdgeCounts pair_mesh_edges(const Mesh &m, dag::Vector<Halfedge> &halfedges);
double signed_volume(const Solid &s);
DPoint3 face_cross(const Solid &s, uint32_t f);
void append_solid(Solid &dst, const Solid &src);
// faces in one plane take one id, the lowest face index among them
void compute_face_planes(const Solid &s, dag::Vector<uint32_t> &plane_of_face);
// before cleanup, spec.md 5.2 step 4
void round_to_float(Solid &s);
bool indices_valid(const Solid &s);
bool auto_tolerance(const BBoxD &box, double &tolerance, eastl::string &err);
DPoint3 bbox_corner(const BBoxD &b, int i);
void add_bbox(BBoxD &dst, const BBoxD &src);
BBoxD points_bbox(dag::ConstSpan<DPoint3> pos);
// the largest |coordinate| of a box that is not empty
double max_abs(const BBoxD &box);
// apart by more than two tolerances on an axis
bool bboxes_disjoint(const BBoxD &a, const BBoxD &b, double tolerance);
FaceShape triangle_shape(const DPoint3 &a, const DPoint3 &b, const DPoint3 &c);
FaceShape face_shape(const Solid &s, uint32_t f);
bool is_sliver(const FaceShape &fs, const CleanupParams &p);
// sorted; a face with a repeated vertex has none
void face_keys(const Solid &s, dag::Vector<FaceKey> &keys);
// faces with the same three vertices and opposite orientation
void skin_pairs(const Solid &s, dag::Vector<TriPair> &pairs);
void closest_barycentric(const DPoint3 &p, const DPoint3 &a, const DPoint3 &b, const DPoint3 &c, double w[3]);
// 3 per corner; a corner averages the faces at its vertex of its group whose smgr bits meet its face's;
// the corners of a keep face keep the values normal holds on entry (nearly equal ones of a fan weld), so the caller fills them first.
// Returns the corner reads of its per-vertex passes, a test's bound on the cost of a fan, not a wall clock.
uint64_t compute_output_normals(const Mesh &m, dag::ConstSpan<uint32_t> face_group, dag::ConstSpan<uint8_t> keep,
  dag::Vector<float> &normal);

inline double point_triangle_dist_sq(const DPoint3 &p, const DPoint3 &a, const DPoint3 &b, const DPoint3 &c)
{
  double w[3];
  closest_barycentric(p, a, b, c, w);
  return lengthSq(a * w[0] + b * w[1] + c * w[2] - p);
}

inline DPoint3 unit_or_zero(const DPoint3 &v)
{
  const double len = length(v);
  return len > 0 ? v * (1.0 / len) : DPoint3(0, 0, 0);
}

// the undirected edge of two ids as one key
inline uint64_t edge_key(uint32_t a, uint32_t b) { return a < b ? uint64_t(a) << 32 | b : uint64_t(b) << 32 | a; }
// eastl::hash truncates uint64 keys on 32-bit
struct KeyHash
{
  size_t operator()(uint64_t k) const { return sizeof(size_t) < 8 ? size_t((k * 0x9E3779B97F4A7C15ull) >> 32) : size_t(k); }
};
inline uint32_t next_he(uint32_t h) { return h - h % 3 + (h + 1) % 3; }
inline uint32_t prev_he(uint32_t h) { return h - h % 3 + (h + 2) % 3; }
inline int dominant_axis(const DPoint3 &n) { return fabs(n.x) >= max(fabs(n.y), fabs(n.z)) ? 0 : fabs(n.y) >= fabs(n.z) ? 1 : 2; }
// k = 1: the u axis of a plane of normal n, k = 2: its v axis, u x v along n
inline int plane_axis(const DPoint3 &n, int k) { return (dominant_axis(n) + (n[dominant_axis(n)] < 0 ? 3 - k : k)) % 3; }

// start[b]..start[b + 1]: the items i < n with key_of(i) == b, in index order
template <typename KeyOf>
void bucket_items(uint32_t n, uint32_t buckets, KeyOf key_of, dag::Vector<uint32_t> &start, dag::Vector<uint32_t> &items)
{
  start.assign(buckets + 1, 0);
  for (uint32_t i = 0; i < n; ++i)
    ++start[key_of(i) + 1];
  for (uint32_t b = 0; b < buckets; ++b)
    start[b + 1] += start[b];
  dag::Vector<uint32_t> fill(start.begin(), start.end() - 1);
  items.resize(n);
  for (uint32_t i = 0; i < n; ++i)
    items[fill[key_of(i)]++] = i;
}
} // namespace dacsg
