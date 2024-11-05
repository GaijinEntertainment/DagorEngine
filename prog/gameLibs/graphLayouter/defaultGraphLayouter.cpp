// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "defaultGraphLayouter.h"
#include <EASTL/sort.h>


void DefaultGraphLayouter::rankNodes(const AdjacencyLists &graph, eastl::span<const NodeIdx> topsort)
{
  // Edges point backwards in the topsort order: a <- b <- c <- ... <- z

  nodeRanks.clear();
  nodeRanks.resize(graph.size(), 0);
  // Move stuff to the left such that i <- j => rank[i] > rank[j]
  // rank[j] = max{rank[i] | edge i->j exists} + 1
  for (auto it = topsort.rbegin(); it != topsort.rend(); ++it)
    for (NodeIdx next : graph[*it])
      nodeRanks[next] = eastl::max(nodeRanks[next], nodeRanks[*it] + 1);
  // Now move stuff even more to the left to eliminate "long legs"
  for (auto it = topsort.begin(); it != topsort.end(); ++it)
  {
    if (graph[*it].empty())
      continue;

    auto minAmongNexts = *eastl::min_element(graph[*it].begin(), graph[*it].end(),
      [&nodeRanks = nodeRanks](NodeIdx a, NodeIdx b) { return nodeRanks[a] < nodeRanks[b]; });

    nodeRanks[*it] = nodeRanks[minAmongNexts] - 1;
  }
}

void DefaultGraphLayouter::generateInitialLayout(eastl::span<const NodeIdx> topsort)
{
  nodeTable.clear();
  nodeTable.resize(1 + (!nodeRanks.empty() ? *eastl::max_element(nodeRanks.begin(), nodeRanks.end()) : 0));

  for (NodeIdx node = 0; node < nodeRanks.size(); ++node)
  {
    auto &column = nodeTable[nodeRanks[node]];
    column.push_back(node);
  }


  nodeSubranks.clear();
  nodeSubranks.resize(nodeRanks.size());

  // Initial subrank sort for stability
  for (auto &column : nodeTable)
  {
    eastl::sort(column.begin(), column.end(), [&topsort](NodeIdx first, NodeIdx second) {
      // Due to culling shceduler, topsort might not contain all nodes.
      // Consider culled nodes to be greater than all other nodes.
      if (first >= topsort.size())
        return false;
      if (second >= topsort.size())
        return true;
      return topsort[first] < topsort[second];
    });

    for (rank_t i = 0; i < column.size(); ++i)
    {
      nodeSubranks[column[i]] = i;
    }
  }
}

void DefaultGraphLayouter::breakMultirankEdges(const AdjacencyLists &graph, bool merge_fake_nodes)
{
  updatedGraph.clear();
  updatedGraph.resize(graph.size());

  edgeMapping.clear();
  edgeMapping.resize(graph.size());

  NodeIdx firstFreeNodeIdx = graph.size();

  for (NodeIdx from = 0; from < graph.size(); ++from)
  {
    for (EdgeIdx edgeIdx = 0; edgeIdx < graph[from].size(); ++edgeIdx)
    {
      NodeIdx to = graph[from][edgeIdx];
      NodeIdx last = from;
      for (NodeIdx i = nodeRanks[from] + 1; i < nodeRanks[to]; ++i)
      {
        if (merge_fake_nodes && !updatedGraph[last].empty() && updatedGraph[last].back() >= graph.size())
        {
          // fake node for current rank already created, reuse it.
          edgeMapping[last].back().second.push_back(edgeIdx);
          last = updatedGraph[last].back();
          continue;
        }
        NodeIdx fakeNode = firstFreeNodeIdx++;

        updatedGraph.emplace_back();
        updatedGraph[last].emplace(fakeNode);

        edgeMapping.emplace_back();
        // `fakeNode` currently is largest nodeIdx, so we can easily push edge to the end and it
        // doesn't break order.
        GraphLayout::OriginalEdges edges = {edgeIdx};
        edgeMapping[last].emplace_back(from, edges);

        nodeSubranks.emplace_back(nodeTable[i].size());
        nodeTable[i].emplace_back(fakeNode);
        nodeRanks.emplace_back(i);

        last = fakeNode;
      }
      if (merge_fake_nodes) // actually this is merging of multiedges
      {
        auto it = updatedGraph[last].find(to);
        if (it != updatedGraph[last].end())
        {
          edgeMapping[last][it - updatedGraph[last].begin()].second.push_back(edgeIdx);
          continue;
        }
      }
      auto emplacedPos = updatedGraph[last].emplace(to);
      // `edgeMapping` is a simple vector without autosort. But we keep order by insert to the same
      // position as in sorted `updatedGraph`.
      auto insertHint = edgeMapping[last].begin() + (emplacedPos - updatedGraph[last].begin());
      GraphLayout::OriginalEdges edges = {edgeIdx};
      edgeMapping[last].emplace(insertHint, from, edges);
    }
  }

  inverseUpdatedGraph.clear();
  inverseUpdatedGraph.resize(updatedGraph.size());
  for (NodeIdx from = 0; from < updatedGraph.size(); ++from)
  {
    for (NodeIdx to : updatedGraph[from])
    {
      inverseUpdatedGraph[to].emplace(from);
    }
  }
}

size_t DefaultGraphLayouter::calculateCrossings(rank_t rank)
{
  // Pick two distinct nodes from the current column
  // and count outgoing edges that cross

  // TODO: This can probably be optimized asymptotically using
  // dynamic programming, but graphs are small for now so this is ok.

  size_t result = 0;

  auto &column = nodeTable[rank];
  for (rank_t j = 0; j < column.size(); ++j)
    for (rank_t k = j + 1; k < column.size(); ++k)
    {
      if (rank < nodeTable.size() - 1)
        for (NodeIdx jTo : updatedGraph[column[j]])
          for (NodeIdx kTo : updatedGraph[column[k]])
            if (nodeSubranks[jTo] > nodeSubranks[kTo])
              ++result;

      if (rank > 0)
        for (NodeIdx jTo : inverseUpdatedGraph[column[j]])
          for (NodeIdx kTo : inverseUpdatedGraph[column[k]])
            if (nodeSubranks[jTo] > nodeSubranks[kTo])
              ++result;
    }

  return result;
}

size_t DefaultGraphLayouter::calculateTotalCrossings()
{
  size_t result = 0;
  for (rank_t rank = 0; rank < nodeTable.size(); ++rank)
    result += calculateCrossings(rank);
  return result;
}

void DefaultGraphLayouter::transposeNodes(rank_t rank, rank_t firstSubrank, rank_t secondSubrank)
{
  NodeIdx &firstIdx = nodeTable[rank][firstSubrank];
  NodeIdx &secondIdx = nodeTable[rank][secondSubrank];

  eastl::swap(firstIdx, secondIdx);
  eastl::swap(nodeSubranks[firstIdx], nodeSubranks[secondIdx]);
}

dag::Vector<eastl::pair<float, NodeIdx>> DefaultGraphLayouter::calculateWavgHeuristic(NodeIdx rank, bool forward)
{
  auto &column = nodeTable[rank];
  auto &adjColumn = nodeTable[forward ? rank + 1 : rank - 1];

  dag::Vector<eastl::pair<float, NodeIdx>> result(column.size());

  for (rank_t j = 0; j < column.size(); ++j)
  {
    result[j].second = column[j];
    auto &adjacents = (forward ? updatedGraph : inverseUpdatedGraph)[column[j]];

    rank_t adj_sum = 0;
    for (rank_t k = 0; k < adjColumn.size(); ++k)
    {
      if (adjacents.find(adjColumn[k]) != adjacents.end())
        adj_sum += k;
    }

    result[j].first = static_cast<float>(adj_sum) / (0.01f + static_cast<float>(adjacents.size()));
  }

  return result;
}

void DefaultGraphLayouter::optimizeHeuristicSort(bool ascend)
{
  for (size_t i = 1; i < nodeTable.size(); ++i)
  {
    rank_t rank = ascend ? i : nodeTable.size() - i - 1;
    auto &column = nodeTable[rank];
    auto subrankAvgAndIdx = calculateWavgHeuristic(rank, !ascend);

    eastl::sort(subrankAvgAndIdx.begin(), subrankAvgAndIdx.end(),
      [](const auto &pair1, const auto &pair2) { return pair1.first < pair2.first; });
    for (rank_t j = 0; j < column.size(); ++j)
    {
      column[j] = subrankAvgAndIdx[j].second;
      nodeSubranks[subrankAvgAndIdx[j].second] = j;
    }
  }
}

void DefaultGraphLayouter::optimizeTranspose()
{
  bool improved = true;
  while (improved)
  {
    improved = false;
    for (rank_t i = 0; i < nodeTable.size(); ++i)
    {
      size_t current = calculateCrossings(i);
      for (rank_t j = 0; j + 1 < nodeTable[i].size(); ++j)
      {
        // try to transpose j and j+1 and see if it helps
        transposeNodes(i, j, j + 1);
        size_t previous = current;
        current = calculateCrossings(i);
        if (previous > current)
        {
          improved = true;
        }
        else
        {
          // it didn't help, rollback
          transposeNodes(i, j, j + 1);
          current = previous;
        }
      }
    }
  }
}

void DefaultGraphLayouter::optimizeLayout(size_t iterations)
{
  auto bestNodeTable = nodeTable;
  size_t bestTotalCrossings = calculateTotalCrossings();

  for (int iteration = 0; iteration < iterations; ++iteration)
  {
    optimizeHeuristicSort(iteration % 2 == 0);
    optimizeTranspose();

    size_t newTotalCrossings = calculateTotalCrossings();
    if (newTotalCrossings < bestTotalCrossings)
    {
      bestNodeTable = nodeTable;
      bestTotalCrossings = newTotalCrossings;
    }
  }

  nodeTable = eastl::move(bestNodeTable);
}

GraphLayout DefaultGraphLayouter::layout(const AdjacencyLists &graph, eastl::span<const NodeIdx> topsort, bool merge_fake_nodes)
{
  rankNodes(graph, topsort);
  generateInitialLayout(topsort);
  breakMultirankEdges(graph, merge_fake_nodes);
  // Recommended constant from the paper is 24,
  // but it already looks good with 12 so I reduced it for performance.
  // If visualized graphs look bad, increase this number.
  // (see header comments for the paper name)
  optimizeLayout(12);
  return GraphLayout{eastl::move(updatedGraph), eastl::move(nodeTable), eastl::move(edgeMapping)};
}

eastl::unique_ptr<IGraphLayouter> make_default_layouter() { return eastl::make_unique<DefaultGraphLayouter>(); }
