// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gameLibs/daCSG/execute_api.h>
#include <gameLibs/daCSG/graph_api.h>
#include <gameLibs/daCSG/solid_api.h>
#include <dag/dag_vector.h>
#include <EASTL/string.h>

namespace dacsg
{
// Dead nodes go and equal subtrees merge in every pass
struct OptimizeParams
{
  double tolerance = 0; // 0: from the root bbox of the primitive parameters, as spec.md 5.1
  bool transforms = true, uvAffines = true, bboxPrune = false, flatten = true, convexChains = true;
  // the lowering's checks and those of every reachable primitive but a Leaf, whose source the lowering checks, run when it is
  // set, before the prune can drop one; the prune needs it, the executor sets it, bboxPrune and the meshLoader it passes
  bool (*checkGraph)(const Graph &g, OpResult root, MeshLoader loader, eastl::string &err) = nullptr;
  MeshLoader meshLoader = nullptr;
};

struct Optimized
{
  Graph graph; // every node has the name of its source node
  OpResult root;
  dag::Vector<uint32_t> origin;       // by node: the source node it comes from, or a named one of any kind folded into it
  dag::Vector<uint32_t> convexChains; // the top Intersect or Cut of each, which the lowering clips
  double tolerance = 0;               // the prune's: the given one, or the root's automatic one before the prune; below 0: unknown
};

bool optimize(const Graph &g, OpResult root, const OptimizeParams &p, Optimized &out, OptimizeStats &stats, eastl::string &err);
// Holds the tessellation at any seg, in the frame of the primitive; reach: a leaf's, else 0; false: unknown (a mesh import)
bool prim_bbox(const Graph &g, const PrimDesc &d, BBoxD &box, double &reach);
// An upper bound of the largest singular value, exact for a rotation times a diagonal on either side; m is tm_in_range
double largest_scale(const TMatrixD &m);
} // namespace dacsg
