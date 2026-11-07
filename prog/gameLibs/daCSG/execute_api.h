// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gameLibs/daCSG/graph_api.h>
#include <gameLibs/daCSG/kernel_api.h>
#include <gameLibs/daCSG/mesh_api.h>
#include <gameLibs/daCSG/validate_api.h>
#include <dag/dag_vector.h>
#include <util/dag_stdint.h>
#include <EASTL/string.h>

namespace dacsg
{
struct ExecuteParams
{
  const char *kernel = "manifold";
  float quality = 1.f; // multiplies Graph quality
  // validate: the root result, crossing faces included
  bool optimize = true, cleanup = true, validate = true;
  bool clusterVertices = true;     // leaf vertices within tolerance merge
  bool kernelFallback = true;      // the other library kernel takes a refused tree or an invalid root, with a warning
  bool validateLeaves = true;      // false: an ILeafSource mesh meets the contract check only, its client validated it
  MeshLoader meshLoader = nullptr; // for this call; null: a Graph::mesh import fails
  CleanupParams cleanupParams;
  KernelParams kernelParams; // tolerance 0: auto from the root bbox; execute sets the budget
};

struct CleanupStats
{
  uint32_t weldRemoved = 0;       // edits
  uint32_t coincidentRemoved = 0; // face pairs
  uint32_t degenerateRemoved = 0; // edits
  uint32_t sliverRemoved = 0;     // edits
  uint32_t mergeRemoved = 0;      // faces; a merge can also only reshape
  uint32_t unfixed = 0;
  uint32_t rolledBack = 0;      // steps undone for volume or genus, a bit each: weld, coincident, degenerate, sliver, merge, overlap
  uint32_t overlapRemoved = 0;  // faces of coplanar overlaps of opposite orientation, replaced by their symmetric difference
  uint32_t genusRolledBack = 0; // the bits of rolledBack whose step changed the genus of the shells
  AngleHistogram minAngleBefore, minAngleAfter;
};

// Rewrites of the optimizer passes, spec.md 10; node counts include the empty node
struct OptimizeStats
{
  uint32_t nodesBefore = 0, nodesAfter = 0;
  uint32_t deadNodes = 0;
  uint32_t transformsComposed = 0, transformsRemoved = 0;
  uint32_t uvAffinesMerged = 0;
  uint32_t subtreesMerged = 0;
  uint32_t emptiesFolded = 0, subtractsPruned = 0, intersectsPruned = 0, unionsToConcat = 0;
  uint32_t unionsFlattened = 0, partsConcatenated = 0, subtractChains = 0;
  uint32_t convexChains = 0;
};

struct NodeStats
{
  OpResult node;
  eastl::string name;
  uint64_t timeUs = 0;
  uint32_t trisIn = 0, trisOut = 0;
  CleanupStats cleanup;
};

struct ExecuteStats
{
  dag::Vector<NodeStats> nodes;
  uint64_t optimizeUs = 0, lowerUs = 0, kernelUs = 0, cleanupUs = 0, validateUs = 0, resolveUs = 0, totalUs = 0;
  uint32_t leaves = 0, trisIn = 0, trisOut = 0;
  uint32_t largestRegion = 0; // leaf triangles of the biggest boolean region that one kernel call takes
  double tolerance = 0;
  CleanupStats cleanup;
  KernelStats kernel;
  OptimizeStats optimizer;
  dag::Vector<eastl::string> warnings;
};

// On failure out is empty, except after a failed validation: then it holds the root mesh, with normals only when its
// attributes resolve, and err starts with the validation text.
bool execute(Graph &g, OpResult root, const ExecuteParams &p, Mesh &out, ExecuteStats *stats = nullptr, eastl::string *err = nullptr);
} // namespace dacsg
