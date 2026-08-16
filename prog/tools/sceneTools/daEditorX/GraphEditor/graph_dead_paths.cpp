// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "graph_dead_paths.h"

#include <graphEditor/graph_data.h>

#include <EASTL/hash_map.h>

namespace
{

// Which input pin(s) an edge feeds, and where the data comes from. Mirrors build_connectivity
// (graph_compile.cpp): an endpoint participates only when its pin role is In. A well-formed
// edge has exactly one such end; the two-slot form absorbs the malformed in->in case without
// needing a separate path.
struct EdgeFlow
{
  int dstNode[2] = {-1, -1};
  int dstPin[2] = {-1, -1};
  int srcNode[2] = {-1, -1};
  int count = 0;

  void add(int dst_node, int dst_pin, int src_node)
  {
    dstNode[count] = dst_node;
    dstPin[count] = dst_pin;
    srcNode[count] = src_node;
    ++count;
  }
};

} // namespace

void compute_dead_paths(const GraphData &g, DeadPaths &out)
{
  const int nodeCount = static_cast<int>(g.nodes.size());
  const int edgeCount = static_cast<int>(g.edges.size());

  out.deadNode.assign(nodeCount, 0);
  out.deadEdge.assign(edgeCount, 0);
  if (nodeCount == 0 || edgeCount == 0)
  {
    return;
  }

  eastl::hash_map<int, int> idToIdx;
  idToIdx.reserve(nodeCount);
  for (int i = 0; i < nodeCount; ++i)
  {
    idToIdx[g.nodes[i].id] = i;
  }
  auto findNode = [&idToIdx](int id) {
    auto it = idToIdx.find(id);
    return it == idToIdx.end() ? -1 : it->second;
  };

  // (node, pin) counters kept in one flat array indexed by pinBase[node] + pin, so a recompute
  // costs no per-node allocation.
  eastl::vector<int> pinBase(nodeCount + 1, 0);
  for (int i = 0; i < nodeCount; ++i)
  {
    pinBase[i + 1] = pinBase[i] + static_cast<int>(g.nodes[i].pins.size());
  }
  eastl::vector<int> liveCount(pinBase[nodeCount], 0);
  eastl::vector<int> liveFedPins(nodeCount, 0);

  eastl::vector<EdgeFlow> flows(edgeCount);
  for (int e = 0; e < edgeCount; ++e)
  {
    const GraphData::Edge &edge = g.edges[e];
    const int a = findNode(edge.elemA);
    const int b = findNode(edge.elemB);
    if (a < 0 || b < 0)
    {
      continue;
    }
    const GraphData::Node &na = g.nodes[a];
    const GraphData::Node &nb = g.nodes[b];
    if (edge.pinA < 0 || edge.pinA >= static_cast<int>(na.pins.size()))
    {
      continue;
    }
    if (edge.pinB < 0 || edge.pinB >= static_cast<int>(nb.pins.size()))
    {
      continue;
    }

    EdgeFlow &f = flows[e];
    if (na.pins[edge.pinA].role == PinRole::In)
    {
      f.add(a, edge.pinA, b);
    }
    if (nb.pins[edge.pinB].role == PinRole::In)
    {
      f.add(b, edge.pinB, a);
    }

    // Muted edges are counted live here and killed by the seed loop below, so both cases go
    // through one code path.
    for (int k = 0; k < f.count; ++k)
    {
      if (liveCount[pinBase[f.dstNode[k]] + f.dstPin[k]]++ == 0)
      {
        ++liveFedPins[f.dstNode[k]];
      }
    }
  }

  // Outgoing adjacency (producing node -> edge), CSR.
  eastl::vector<int> outStart(nodeCount + 1, 0);
  for (int e = 0; e < edgeCount; ++e)
  {
    const EdgeFlow &f = flows[e];
    for (int k = 0; k < f.count; ++k)
    {
      ++outStart[f.srcNode[k] + 1];
    }
  }
  for (int i = 0; i < nodeCount; ++i)
  {
    outStart[i + 1] += outStart[i];
  }
  eastl::vector<int> outEdges(outStart[nodeCount], 0);
  {
    eastl::vector<int> cursor(outStart.begin(), outStart.begin() + nodeCount);
    for (int e = 0; e < edgeCount; ++e)
    {
      const EdgeFlow &f = flows[e];
      for (int k = 0; k < f.count; ++k)
      {
        outEdges[cursor[f.srcNode[k]]++] = e;
      }
    }
  }

  // Least fixed point seeded from the authored mutes. Deadness only ever spreads, so this
  // terminates in one pass over each edge and an allowLoop cycle cannot kill itself.
  eastl::vector<int> work;
  for (int e = 0; e < edgeCount; ++e)
  {
    if (g.edges[e].muted)
    {
      out.deadEdge[e] = 1;
      work.push_back(e);
    }
  }

  while (!work.empty())
  {
    const int e = work.back();
    work.pop_back();

    const EdgeFlow &f = flows[e];
    for (int k = 0; k < f.count; ++k)
    {
      const int n = f.dstNode[k];
      if (--liveCount[pinBase[n] + f.dstPin[k]] != 0)
      {
        continue;
      }
      if (--liveFedPins[n] != 0)
      {
        continue;
      }
      // Reaching here means a fed input pin of n just lost its last live edge and no other fed
      // pin has one either. A node with no incoming edge never gets decremented at all, which
      // is what keeps source nodes alive.
      if (out.deadNode[n])
      {
        continue;
      }
      out.deadNode[n] = 1;
      for (int i = outStart[n]; i < outStart[n + 1]; ++i)
      {
        const int o = outEdges[i];
        if (!out.deadEdge[o])
        {
          out.deadEdge[o] = 1;
          work.push_back(o);
        }
      }
    }
  }
}
