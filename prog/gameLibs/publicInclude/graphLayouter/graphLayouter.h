//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <dag/dag_vector.h>
#include <EASTL/vector_multiset.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/span.h>


using NodeIdx = uint32_t;
using EdgeIdx = uint32_t;
using AdjacencyLists = dag::Vector<dag::Vector<NodeIdx>>;
using SortedAdjacencyLists = dag::Vector<eastl::vector_multiset<NodeIdx>>;
using NodeLayout = dag::Vector<dag::Vector<NodeIdx>>;

struct GraphLayout
{
  /**
   * Adjacency list updated with the fake node connectivity info.
   * Guarantee: for an edge u -> v in this graph, rank(u) + 1 = rank(v).
   */
  SortedAdjacencyLists adjacencyList;
  /**
   * A table of node indices that can be rendered as-is.
   * Has minimized edge-intersections and cluttering.
   * Each node index is contained here exactly once.
   * For `nodeLayout[i][j] = v`, `i` is called node `v`'s rank, while
   * `j` is called it's subrank.
   */
  NodeLayout nodeLayout;

  /**
   * Info useful for reconstructing additional edge data for the new
   * graph.
   * edgeMapping[i][j] = [(i', j1'), (i', j2'), ...]
   * where i -> adjacencyList[i][j] is an edge that came from
   * breaking up and merging the edges of the original graph:
   *   i' -> originalAdjacencyList[i'][j1'],
   *   i' -> originalAdjacencyList[i'][j2'], ...
   *
   * In layout without edges merging all arrays contain exactly one edge.
   */

  using OriginalEdges = eastl::fixed_vector<EdgeIdx, 8>;
  dag::Vector<dag::Vector<eastl::pair<NodeIdx, OriginalEdges>>> edgeMapping;
};

/**
 * Creates a human-readable layout (planarization) of a graph given by
 * it's adjacency lists representation.
 * Also requires a topological sort of the graph for now.
 *
 * This process introduces new fake nodes into the graph in order to
 * "break" edges. Fake nodes are supposed to be used as control points
 * when rendering edges with bezier curves or similar techniques.
 */
class IGraphLayouter
{
public:
  // TODO: make topsort an optional optimization
  virtual GraphLayout layout(const AdjacencyLists &graph, eastl::span<const NodeIdx> topsort, bool merge_fake_nodes) = 0;

  virtual ~IGraphLayouter() = default;
};

/**
 * Creates a default layouter based on graphviz's dot algorithm.
 */
eastl::unique_ptr<IGraphLayouter> make_default_layouter();
