// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "graph/graphUtil.h"
#include "mesh/solidUtil.h"
#include <gameLibs/daCSG/execute_api.h>
#include <gameLibs/daCSG/kernel_api.h>
#include <gameLibs/daCSG/solid_api.h>
#include <gameLibs/daParallel/parallel_api.h>
#include <dag/dag_vector.h>
#include <generic/dag_span.h>
#include <perfMon/dag_cpuFreq.h>
#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>

namespace dacsg
{
inline uint64_t usec_since(int64_t ref_ticks) { return uint64_t(get_time_nsec(ref_ticks) / 1000); }
// the four ops keep NodeKind's order, one lower
static_assert(int(NodeKind::Union) == int(BoolTree::Kind::Union) + 1 && int(NodeKind::Concat) == int(BoolTree::Kind::Concat) + 1);
inline BoolTree::Kind bool_tree_kind(NodeKind op) { return BoolTree::Kind(int(op) - 1); }

// 21 bits per axis: aliased cells only add candidates
inline uint64_t cell_key(int64_t x, int64_t y, int64_t z)
{
  const uint64_t mask = (1u << 21) - 1;
  return (uint64_t(x) & mask) | (uint64_t(y) & mask) << 21 | (uint64_t(z) & mask) << 42;
}
// a weld grid takes |coordinate| below it times the tolerance, so floor(p / tolerance) fits int64
constexpr double MAX_WELD_CELLS = 1e15;

struct SourceMesh
{
  Mesh mesh; // channels; a primitive's in the frame of its node, an external leaf's for the world_tm it was given
  Solid solid;
  dag::Vector<uint32_t> faceGroup;         // empty: one smoothing group
  mutable dag::Vector<uint32_t> facePlane; // the plane of each face, computed at the first leaf
  // per corner of solid: one id unless a seam parts them; at the first leaf
  mutable dag::Vector<uint32_t> cornerClass;
  dag::Vector<uint8_t> ownNormal; // per face, or empty: CH_NORMAL is a leaf's and stays
  uint32_t groupCount = 1;
  bool checked = true; // false: a leaf source's mesh under ExecuteParams::validateLeaves = false
};

struct LeafSource
{
  const SourceMesh *src = nullptr;
  bool flipped = false; // leaf corners 1 and 2 are swapped
  uint32_t groupBase = 0;
  TMatrixD tm; // src to the tree
};

struct AttrRecord
{
  AttrDesc desc;
  uint32_t firstLeaf = 0, endLeaf = 0;
  int8_t flip = 1;           // -1: odd Subtract right children above
  TMatrixD toRoot, fromRoot; // the frame of the Attr node
};

struct Lowered
{
  BoolTree tree;
  dag::Vector<OpResult> treeNode;  // per tree node: the graph node it lowers, which messages name
  dag::Vector<LeafSource> sources; // per leaf
  dag::Vector<AttrRecord> attrs;   // outermost last
  uint32_t groupCount = 0;

  // the one way to append a leaf: sources stays parallel to tree.leaves
  void add(Solid &&leaf, LeafSource source) { tree.leaves.push_back(eastl::move(leaf)), sources.push_back(source); }
};

struct LowerContext
{
  const Graph &g;
  KernelParams &kernelParams;
  ExecuteStats &stats;
  float quality = 1;
  double sag = 0; // world units, spec.md 4; graph_bbox sets it
  bool validateLeaves = true;
  MeshLoader meshLoader = nullptr;
  uint32_t workers = 1; // of graph_bbox's prims
  dag::Vector<uint32_t> parents;
  dag::Vector<const SourceMesh *> prim;     // by node id
  dag::Vector<const SourceMesh *> instance; // by node id: a subtree evaluated alone
  dag::Vector<uint8_t> clip;                // by node id, or empty: the top of a convex chain, spec.md 10.7
  dag::Vector<eastl::unique_ptr<SourceMesh>> storage;
  struct LeafMesh
  {
    uint32_t node;
    double scale;
    const SourceMesh *src;
  };
  dag::Vector<LeafMesh> leafMeshes; // leaf tessellations shared by similar instances

  LowerContext(const Graph &graph, KernelParams &kp, ExecuteStats &st) : g(graph), kernelParams(kp), stats(st) {}
  SourceMesh &newSource();
};

// own: subtrees evaluated alone, children first; fills ctx.parents, which graph_bbox reads;
// check_nodes: also the value checks of every node and the checks of every primitive, unless check_graph ran them
bool prepare_graph(LowerContext &ctx, OpResult root, bool check_nodes, dag::Vector<OpResult> &own, eastl::string &err);
// prepare_graph's checks with check_nodes, for OptimizeParams::checkGraph
bool check_graph(const Graph &g, OpResult root, MeshLoader loader, eastl::string &err);
// Conservative: transforms are applied to child boxes;
// resets ctx.prim and ctx.instance, sets ctx.sag, then makes the prims on ctx.workers
bool graph_bbox(LowerContext &ctx, OpResult root, BBoxD &box, eastl::string &err);
// after graph_bbox, which makes the prims
bool lower(LowerContext &ctx, OpResult root, Lowered &out, eastl::string &err);
// per boolean subtree under the top Concats
void cluster_vertices(BoolTree &tree, double tolerance);
// spec.md 10.7: the intersection of convex leaves by plane clipping; leafRef, facePlaneKey and bbox from the leaves,
// which the caller fills, as a Solid may leave them empty; false: no valid result,
// or an empty one the weld tolerance made of leaves no face plane separates, which the kernel decides
bool clip_convex(dag::ConstSpan<Solid> leaves, double tolerance, Solid &out);

// own_normal: per face, the leaf's normal is kept; a corner takes its attributes from the leaf face within tolerance of
// it, its face's source first, then the nearest face of the source's chart, else clamped on the source
bool resolve_attributes(const Lowered &lw, Solid &result, double tolerance, Mesh &out, dag::Vector<uint32_t> &face_group,
  dag::Vector<uint8_t> &own_normal, eastl::string &err);
void compute_corner_classes(const SourceMesh &src);
// one source leaf, and every vertex its two leaf faces share has one corner class
bool same_chart(const Lowered &lw, LeafRef a, LeafRef b);
// u and v with u x v = n
void tangent_frame(const DPoint3 &n, DPoint3 &u, DPoint3 &v);
} // namespace dacsg
