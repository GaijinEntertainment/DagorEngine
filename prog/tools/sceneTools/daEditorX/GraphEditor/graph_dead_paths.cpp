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

// An edge marks BOTH its endpoints connected, but only when one of them is an In pin, so an
// out-to-out or any-to-any edge marks nothing. The compile reads these bits rather than deriving
// them again.
void mark_connected_pins(const GraphData &g, const DeadPaths &dead, const eastl::hash_map<int, int> &id_to_index,
  PinConnectivity &conn)
{
  for_each_live_edge(g, dead, id_to_index, [&](const LiveEdge &e) {
    if (g.nodes[e.nodeA].pins[e.pinA].role == PinRole::In || g.nodes[e.nodeB].pins[e.pinB].role == PinRole::In)
    {
      conn.connected[conn.pinBase[e.nodeA] + e.pinA] = 1;
      conn.connected[conn.pinBase[e.nodeB] + e.pinB] = 1;
    }
  });
}

} // namespace

void build_pin_connectivity(const GraphData &g, const DeadPaths &dead, const eastl::hash_map<int, int> &id_to_index,
  PinConnectivity &conn)
{
  const int nodeCount = static_cast<int>(g.nodes.size());
  conn.pinBase.assign(nodeCount + 1, 0);
  for (int i = 0; i < nodeCount; ++i)
  {
    conn.pinBase[i + 1] = conn.pinBase[i] + static_cast<int>(g.nodes[i].pins.size());
  }
  conn.connected.assign(conn.pinBase[nodeCount], 0);
  mark_connected_pins(g, dead, id_to_index, conn);
}

void compute_dead_paths(const GraphData &g, const eastl::hash_map<int, int> &idToIdx, DeadPaths &out)
{
  const int nodeCount = static_cast<int>(g.nodes.size());
  const int edgeCount = static_cast<int>(g.edges.size());

  out.deadNode.assign(nodeCount, 0);
  out.deadEdge.assign(edgeCount, 0);
  if (nodeCount == 0 || edgeCount == 0)
  {
    return;
  }

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
  LiveEdge le;
  for (int e = 0; e < edgeCount; ++e)
  {
    if (!resolve_edge_endpoints(g, idToIdx, g.edges[e], le))
    {
      continue;
    }
    const GraphData::Node &na = g.nodes[le.nodeA];
    const GraphData::Node &nb = g.nodes[le.nodeB];

    EdgeFlow &f = flows[e];
    if (na.pins[le.pinA].role == PinRole::In)
    {
      f.add(le.nodeA, le.pinA, le.nodeB);
    }
    if (nb.pins[le.pinB].role == PinRole::In)
    {
      f.add(le.nodeB, le.pinB, le.nodeA);
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
