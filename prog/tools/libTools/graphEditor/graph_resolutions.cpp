// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <graphEditor/graph_resolutions.h>
#include <graphEditor/graph_dead_paths.h>

#include <graphEditor/graph_data.h>

#include <ioSys/dag_dataBlock.h>
#include <debug/dag_debug.h>

#include <EASTL/algorithm.h>
#include <EASTL/hash_map.h>

#include <cstring>

namespace
{

constexpr int MAX_TEXTURE_DIM = 8192;

int clamp_dim(int v) { return eastl::max(eastl::min(v, MAX_TEXTURE_DIM), 1); }

eastl::string texture_property(const GraphData::Node &n, const DataBlock *desc, const char *name, const char *fallback)
{
  eastl::string v = texture_property_value(n, desc, name);
  return v.empty() ? eastl::string(fallback) : v;
}

bool descriptor_has_main_pin(const DataBlock *desc)
{
  if (!desc)
  {
    return false;
  }
  for (uint32_t i = 0; i < desc->blockCount(); ++i)
  {
    const DataBlock *b = desc->getBlock(i);
    if (strcmp(b->getBlockName(), "pin") == 0 && b->getBool("main", false))
    {
      return true;
    }
  }
  return false;
}

enum class ResolveResult
{
  Resolved,
  AlreadyResolved, // settled by an earlier iteration or by a seed
  Waiting,         // the node feeding its main input has not settled yet
  NeverResolves,   // produces no texture, so no later iteration will change that
};

// The connected input pin a node inherits its texture from, or -1.
int find_main_input(int node_index, const GraphData &g, const PinConnectivity &conn, const DataBlock *desc, bool desc_has_main_pin)
{
  const GraphData::Node &n = g.nodes[node_index];
  int mainInputIndex = -1;
  for (int i = 0; i < static_cast<int>(n.pins.size()); ++i)
  {
    const GraphData::Pin &p = n.pins[i];
    const bool pinConnected = conn.connected[conn.pinBase[node_index] + i] != 0;
    if (p.role == PinRole::In)
    {
      const DataBlock *const descPin = desc_has_main_pin ? find_pin_desc(desc, p.name.c_str(), p.role) : nullptr;
      const bool isMain = descPin && descPin->getBool("main", false);
      const bool isTexFirstType = !p.types.empty() && is_texture_type_for_inherit(p.types.front());
      if (isMain && pinConnected)
      {
        mainInputIndex = i;
      }
      if (mainInputIndex == -1 && pinConnected && isTexFirstType)
      {
        mainInputIndex = i;
      }
    }
  }
  return mainInputIndex;
}

// Only ResolveResult::Resolved counts as progress for the driver. With `main_source_lost` a node
// whose main input never gets a source resolves from its own properties, as if unconnected.
ResolveResult resolve_node(int node_index, const GraphData &g, const PinConnectivity &conn, const DataBlock *desc,
  bool desc_has_main_pin, bool main_source_lost, const eastl::vector<int> &pin_source, GraphResolutions &out)
{
  NodeResolution &res = out.nodes[node_index];
  if (res.resolved)
  {
    return ResolveResult::AlreadyResolved;
  }

  const GraphData::Node &n = g.nodes[node_index];

  if (node_out_texture_dimension(n) == 0)
  {
    return ResolveResult::NeverResolves;
  }

  const int mainInputIndex = find_main_input(node_index, g, conn, desc, desc_has_main_pin);
  const int source = mainInputIndex >= 0 ? pin_source[conn.pinBase[node_index] + mainInputIndex] : -1;
  if (mainInputIndex >= 0 && source < 0 && !main_source_lost)
  {
    return ResolveResult::Waiting; // connected, but the upstream has not resolved yet
  }

  const eastl::string w = texture_property(n, desc, "texture width", "parent size");
  const eastl::string h = texture_property(n, desc, "texture height", "width");
  const eastl::string d = texture_property(n, desc, "texture depth", "width");
  const eastl::string t = texture_property(n, desc, "texture type", "parent type");
  const eastl::string a = texture_property(n, desc, "texture wrap", "parent wrap");

  int width = g.graphTextureWidth;
  int height = g.graphTextureHeight;
  int depth = g.graphTextureDepth;
  eastl::string type = g.graphTextureType;
  eastl::string wrap = g.graphTextureWrap;

  int parsed = 0;
  if (source < 0)
  {
    if (parse_prefixed_int(w.c_str(), '=', parsed))
    {
      width = parsed;
    }
    if (parse_prefixed_int(h.c_str(), '=', parsed))
    {
      height = parsed;
    }
    if (parse_prefixed_int(d.c_str(), '=', parsed))
    {
      depth = parsed;
    }
    if (h == "width")
    {
      height = width;
    }
    if (d == "width")
    {
      depth = width;
    }
    if (t != "parent type")
    {
      type = t;
    }
    if (a != "parent wrap")
    {
      wrap = a;
    }
  }
  else
  {
    const NodeResolution &up = out.nodes[source];
    width = up.width;
    height = up.height;
    depth = up.depth;
    type = up.type;
    wrap = up.wrap;
    if (parse_prefixed_int(w.c_str(), 'x', parsed))
    {
      width *= parsed;
    }
    if (parse_prefixed_int(h.c_str(), 'x', parsed))
    {
      height *= parsed;
    }
    if (parse_prefixed_int(d.c_str(), 'x', parsed))
    {
      depth *= parsed;
    }
    if (parse_prefixed_int(w.c_str(), '/', parsed) && parsed != 0)
    {
      width /= parsed;
    }
    if (parse_prefixed_int(h.c_str(), '/', parsed) && parsed != 0)
    {
      height /= parsed;
    }
    if (parse_prefixed_int(d.c_str(), '/', parsed) && parsed != 0)
    {
      depth /= parsed;
    }
    if (parse_prefixed_int(w.c_str(), '=', parsed))
    {
      width = parsed;
    }
    if (parse_prefixed_int(h.c_str(), '=', parsed))
    {
      height = parsed;
    }
    if (parse_prefixed_int(d.c_str(), '=', parsed))
    {
      depth = parsed;
    }
    if (w == "graph size")
    {
      width = g.graphTextureWidth;
    }
    if (h == "graph size")
    {
      height = g.graphTextureHeight;
    }
    if (d == "graph size")
    {
      depth = g.graphTextureDepth;
    }
    if (h == "width")
    {
      height = width;
    }
    if (d == "width")
    {
      depth = width;
    }
    if (t != "parent type")
    {
      type = t;
    }
    if (t == "graph type")
    {
      type = g.graphTextureType;
    }
    if (a != "parent wrap")
    {
      wrap = a;
    }
    if (a == "graph wrap")
    {
      wrap = g.graphTextureWrap;
    }
    width = clamp_dim(width);
    height = clamp_dim(height);
    depth = clamp_dim(depth);
  }

  res.resolved = true;
  res.width = width;
  res.height = height;
  res.depth = depth;
  res.type = eastl::move(type);
  res.wrap = eastl::move(wrap);
  return ResolveResult::Resolved;
}

// first push wins, so a pin fed by more than one edge takes whichever upstream resolved
// first. The iteration order is part of the result and must not be reordered.
bool push_along_edges(const GraphData &g, const eastl::vector<LiveEdge> &live_edges, const PinConnectivity &conn,
  const GraphResolutions &res, eastl::vector<int> &pin_source)
{
  bool changed = false;
  for (const LiveEdge &e : live_edges)
  {
    auto push = [&](int from_node, int to_node, int to_pin) {
      if (!res.nodes[from_node].resolved)
      {
        return;
      }
      int &slot = pin_source[conn.pinBase[to_node] + to_pin];
      if (slot >= 0)
      {
        return;
      }
      slot = from_node;
      changed = true;
    };

    const GraphData::Pin &pa = g.nodes[e.nodeA].pins[e.pinA];
    const GraphData::Pin &pb = g.nodes[e.nodeB].pins[e.pinB];
    if (pa.role == PinRole::Out && pb.role == PinRole::In)
    {
      push(e.nodeA, e.nodeB, e.pinB);
    }
    if (pb.role == PinRole::Out && pa.role == PinRole::In)
    {
      push(e.nodeB, e.nodeA, e.pinA);
    }
  }
  return changed;
}

} // namespace

void compute_graph_resolutions(const GraphData &g, const eastl::vector<const DataBlock *> &node_descriptors,
  const eastl::hash_map<int, int> &id_to_index, const DeadPaths &dead, const PinConnectivity &conn,
  const eastl::hash_map<int, NodeResolution> *seeds, GraphResolutions &out)
{
  const int nodeCount = static_cast<int>(g.nodes.size());
  out.nodes.clear();
  out.nodes.resize(nodeCount);
  if (nodeCount == 0)
  {
    return;
  }

  if (seeds)
  {
    for (int i = 0; i < nodeCount; ++i)
    {
      // instances only. Node ids are recycled, so a seed whose instance was deleted
      // would otherwise land on whatever node took the id next.
      if (!is_subgraph_instance(g.nodes[i]))
      {
        continue;
      }
      const auto it = seeds->find(g.nodes[i].id);
      if (it != seeds->end() && it->second.resolved)
      {
        out.nodes[i] = it->second;
      }
    }
  }

  eastl::hash_map<const DataBlock *, bool> mainPinByDesc;
  eastl::vector<uint8_t> nodeHasMainPin(nodeCount, 0);
  for (int i = 0; i < nodeCount; ++i)
  {
    const DataBlock *const desc = i < static_cast<int>(node_descriptors.size()) ? node_descriptors[i] : nullptr;
    auto it = mainPinByDesc.find(desc);
    if (it == mainPinByDesc.end())
    {
      it = mainPinByDesc.insert(eastl::make_pair(desc, descriptor_has_main_pin(desc))).first;
    }
    nodeHasMainPin[i] = it->second ? 1 : 0;
  }

  eastl::vector<LiveEdge> liveEdges;
  liveEdges.reserve(g.edges.size());
  for_each_live_edge(g, dead, id_to_index, [&](const LiveEdge &e) { liveEdges.push_back(e); });

  eastl::vector<uint8_t> neverResolves(nodeCount, 0);
  eastl::vector<uint8_t> mainSourceLost(nodeCount, 0);
  auto descOf = [&](int i) { return i < static_cast<int>(node_descriptors.size()) ? node_descriptors[i] : nullptr; };

  // Which resolved node feeds each input pin; -1 until one does.
  eastl::vector<int> pinSource(conn.pinBase[nodeCount], -1);

  for (int round = 0; round <= nodeCount; ++round)
  {
    int iteration = 0;
    for (bool changed = true; changed;)
    {
      changed = false;
      for (int i = 0; i < nodeCount; ++i)
      {
        if (neverResolves[i])
        {
          continue;
        }
        const ResolveResult r = resolve_node(i, g, conn, descOf(i), nodeHasMainPin[i] != 0, mainSourceLost[i] != 0, pinSource, out);
        if (r == ResolveResult::Resolved)
        {
          changed = true;
        }
        else if (r == ResolveResult::NeverResolves)
        {
          neverResolves[i] = 1;
        }
      }
      if (push_along_edges(g, liveEdges, conn, out, pinSource))
      {
        changed = true;
      }
      ++iteration;
      if (iteration > MAX_GRAPH_CHAIN_PASSES)
      {
        debug("compute_graph_resolutions: propagation loop hit %d-iteration guard", MAX_GRAPH_CHAIN_PASSES);
        return;
      }
    }

    // Stalled. A node still waiting has a main input that no resolvable node feeds (it produces no
    // texture), or it waits on a loop. Release the first kind; else break the loop at one node.
    auto waiting = [&](int i) { return !out.nodes[i].resolved && !neverResolves[i]; };
    eastl::vector<int> mainPin(nodeCount, -1);
    int firstWaiting = -1;
    for (int i = nodeCount - 1; i >= 0; --i)
    {
      if (waiting(i))
      {
        mainPin[i] = find_main_input(i, g, conn, descOf(i), nodeHasMainPin[i] != 0);
        firstWaiting = i;
      }
    }
    if (firstWaiting < 0)
    {
      return;
    }
    eastl::vector<int> waitsOn(nodeCount, -1);
    auto feed = [&](int to, int to_pin, int from, int from_pin) {
      if (waitsOn[to] < 0 && to_pin == mainPin[to] && g.nodes[from].pins[from_pin].role == PinRole::Out && !neverResolves[from])
      {
        waitsOn[to] = from;
      }
    };
    for (const LiveEdge &e : liveEdges)
    {
      feed(e.nodeA, e.pinA, e.nodeB, e.pinB);
      feed(e.nodeB, e.pinB, e.nodeA, e.pinA);
    }
    bool released = false;
    for (int i = 0; i < nodeCount; ++i)
    {
      if (waiting(i) && waitsOn[i] < 0)
      {
        mainSourceLost[i] = 1;
        released = true;
      }
    }
    if (!released)
    {
      // Every waiting node now waits on another waiting node, so nodeCount steps end on a loop.
      int cur = firstWaiting;
      for (int step = 0; step < nodeCount; ++step)
        cur = waitsOn[cur];
      // the first loop node, whatever the annotations
      int lowest = cur;
      for (int i = waitsOn[cur]; i != cur; i = waitsOn[i])
        lowest = eastl::min(lowest, i);
      mainSourceLost[lowest] = 1;
    }
  }
}
