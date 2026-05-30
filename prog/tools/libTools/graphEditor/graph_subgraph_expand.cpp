// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <graphEditor/graph_subgraph_expand.h>

#include <debug/dag_debug.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_string.h>

#include <EASTL/algorithm.h>
#include <EASTL/hash_map.h>
#include <EASTL/hash_set.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>

#include <cstring>

namespace
{

// Per-compile cache of loaded children + a recursion stack. Owned by the top-level entry point
// and threaded through `expand_recursive` so two instances of the same child file share a single
// parse and so cycle detection observes the whole call chain.
struct ExpandContext
{
  const DataBlock &baseNodesBlk;
  eastl::string rootDir; // ends with '/'
  eastl::hash_map<eastl::string, GraphData> cache;
  eastl::vector<eastl::string> stack;
};

bool starts_with(const char *s, const char *prefix)
{
  while (*prefix)
  {
    if (*s++ != *prefix++)
    {
      return false;
    }
  }
  return true;
}

// Looks up `<root>/<stem>.subgraph.blk` and returns its absolute path, or empty on miss.
// Files outside `root_dir` are not searched.
eastl::string resolve_child_path(const eastl::string &root_dir, const char *stem)
{
  eastl::string path(root_dir);
  if (!path.empty() && path.back() != '/' && path.back() != '\\')
  {
    path.push_back('/');
  }
  path.append(stem);
  path.append(".subgraph.blk");
  if (dd_file_exist(path.c_str()))
  {
    return path;
  }
  return {};
}

bool expand_recursive(GraphData &gd, ExpandContext &ctx);

// Splices one subgraph instance node (identified by its stable Node::id, not a vector index
// which can shift across prior splices in the same loop). Logs and bails on any
// per-instance failure; the caller continues with the next instance.
void splice_one(GraphData &gd, int instance_id, ExpandContext &ctx)
{
  // Re-find the instance by id -- prior splices in the enclosing loop have appended
  // child nodes, so indices captured pre-loop are stale.
  int instanceIdx = -1;
  for (int i = 0; i < static_cast<int>(gd.nodes.size()); ++i)
  {
    if (gd.nodes[i].id == instance_id)
    {
      instanceIdx = i;
      break;
    }
  }
  if (instanceIdx < 0)
  {
    return;
  }

  // Snapshot the instance node so subsequent gd.nodes/gd.edges mutations don't invalidate
  // any references we hold across the splice.
  const GraphData::Node instance = gd.nodes[instanceIdx];
  const eastl::string descName = instance.descName;

  eastl::string childPath = resolve_child_path(ctx.rootDir, descName.c_str());
  if (childPath.empty())
  {
    debug("graphEditor: subgraph instance id=%d references missing file '%s.subgraph.blk' (under '%s')", instance_id, descName.c_str(),
      ctx.rootDir.c_str());
    return;
  }

  // Cycle guard: bail before touching the cache so a recursive child isn't even parsed.
  for (const eastl::string &p : ctx.stack)
  {
    if (p == childPath)
    {
      eastl::string chain;
      for (const eastl::string &q : ctx.stack)
      {
        if (!chain.empty())
        {
          chain.append(" -> ");
        }
        chain.append(q.c_str());
      }
      chain.append(" -> ");
      chain.append(childPath.c_str());
      debug("graphEditor: subgraph recursion detected: %s", chain.c_str());
      return;
    }
  }

  // Cache lookup. Cache holds the unexpanded child -- each instance gets a fresh deep copy
  // before being spliced/recursed so id remap and edge surgery don't bleed across siblings.
  auto cacheIt = ctx.cache.find(childPath);
  if (cacheIt == ctx.cache.end())
  {
    GraphData child;
    if (!load_graph_data_blk(child, childPath.c_str(), "", &ctx.baseNodesBlk))
    {
      debug("graphEditor: subgraph '%s' failed to load", childPath.c_str());
      return;
    }

    // Schema check at load time -- a file that fails validation MUST NOT be inlined.
    // Without this guard, a file with zero boundary nodes would have an empty boundary
    // map, the surgery passes would no-op, and `working`'s entire interior would be
    // pasted into the parent on splice -- which is the catastrophic "I dropped a level
    // graph into my graph and now my graph contains the whole level" failure mode.
    // Failing the load on schema errors keeps the cache out of a poisoned state.
    eastl::vector<SubgraphSchemaError> schemaErrors;
    if (!validate_subgraph_schema(child, schemaErrors))
    {
      for (const SubgraphSchemaError &err : schemaErrors)
      {
        debug("graphEditor: subgraph '%s' schema error: %s", childPath.c_str(), format_subgraph_schema_error(err).c_str());
      }
      return;
    }

    cacheIt = ctx.cache.emplace(childPath, eastl::move(child)).first;
  }

  // Working copy: cache holds the original unexpanded child; we splice into the parent
  // from a freshly-cloned working copy so two instances of the same child don't fight
  // over id remap or boundary deletion state.
  GraphData working = cacheIt->second;

  // Recurse FIRST so the spliced child is already flat -- otherwise nested subgraph
  // instances inside the child would survive into the parent post-splice and break
  // single-pass invariants downstream.
  ctx.stack.push_back(childPath);
  expand_recursive(working, ctx);
  ctx.stack.pop_back();

  // ----- Boundary index -----
  // For each `subgraph in: TYPE` we capture its single out-pin (the side that drives interior
  // edges); for each `subgraph out` we capture its single in-pin (the side fed by an interior
  // producer). Boundaries are keyed by the user-authored `name` property -- duplicate names
  // within one role are rejected to avoid ambiguous splice.
  struct InBoundary
  {
    int nodeId = -1;
    int pinIdx = -1; // out-role pin index on the boundary node
  };
  struct OutBoundary
  {
    int nodeId = -1;
    int pinIdx = -1; // in-role pin index on the boundary node
  };
  eastl::hash_map<eastl::string, InBoundary> inBoundaries;
  eastl::hash_map<eastl::string, OutBoundary> outBoundaries;
  bool boundaryError = false;

  for (const GraphData::Node &n : working.nodes)
  {
    const char *dn = n.descName.c_str();
    const bool isIn = starts_with(dn, "subgraph in:");
    const bool isOut = strcmp(dn, "subgraph out") == 0;
    if (!isIn && !isOut)
    {
      continue;
    }
    // Use the shared helper -- it falls back to deriving the name from the boundary's
    // first connected interior pin when the `name` property is empty. The synthesis side
    // does the same, so the instance's pin name and the boundary's interface name always
    // resolve to the same string -- which is what the splice surgery below matches against.
    eastl::string ifaceName = effective_subgraph_boundary_name(working, n.id);
    if (ifaceName.empty())
    {
      debug("graphEditor: subgraph '%s' has a %s boundary with empty name and no edges to derive from", childPath.c_str(),
        isIn ? "input" : "output");
      boundaryError = true;
      continue;
    }
    if (isIn)
    {
      if (inBoundaries.find(ifaceName) != inBoundaries.end())
      {
        debug("graphEditor: subgraph '%s' has duplicate input boundary named '%s'", childPath.c_str(), ifaceName.c_str());
        boundaryError = true;
        continue;
      }
      // The subgraph-in template has exactly one out-role pin (the interior-driving side).
      int pinIdx = -1;
      for (int i = 0; i < static_cast<int>(n.pins.size()); ++i)
      {
        if (n.pins[i].role == PinRole::Out)
        {
          pinIdx = i;
          break;
        }
      }
      if (pinIdx < 0)
      {
        debug("graphEditor: subgraph '%s' input boundary '%s' has no out-role pin", childPath.c_str(), ifaceName.c_str());
        boundaryError = true;
        continue;
      }
      inBoundaries[ifaceName] = InBoundary{n.id, pinIdx};
    }
    else
    {
      if (outBoundaries.find(ifaceName) != outBoundaries.end())
      {
        debug("graphEditor: subgraph '%s' has duplicate output boundary named '%s'", childPath.c_str(), ifaceName.c_str());
        boundaryError = true;
        continue;
      }
      int pinIdx = -1;
      for (int i = 0; i < static_cast<int>(n.pins.size()); ++i)
      {
        if (n.pins[i].role == PinRole::In)
        {
          pinIdx = i;
          break;
        }
      }
      if (pinIdx < 0)
      {
        debug("graphEditor: subgraph '%s' output boundary '%s' has no in-role pin", childPath.c_str(), ifaceName.c_str());
        boundaryError = true;
        continue;
      }
      outBoundaries[ifaceName] = OutBoundary{n.id, pinIdx};
    }
  }
  if (boundaryError)
  {
    return;
  }

  // ----- ID remap -----
  // Compute a single offset that exceeds every id currently in `gd` (nodes AND edges).
  // Deterministic per splice -- two compiles of the same graph produce identical ids,
  // which is required for stable `%graph_hash%` substitution downstream.
  int maxId = 0;
  for (const GraphData::Node &n : gd.nodes)
  {
    if (n.id > maxId)
    {
      maxId = n.id;
    }
  }
  for (const GraphData::Edge &e : gd.edges)
  {
    if (e.id > maxId)
    {
      maxId = e.id;
    }
  }
  const int idOffset = maxId + 1;
  for (GraphData::Node &n : working.nodes)
  {
    n.id += idOffset;
  }
  for (GraphData::Edge &e : working.edges)
  {
    e.id += idOffset;
    e.elemA += idOffset;
    e.elemB += idOffset;
  }
  for (auto &kv : inBoundaries)
  {
    kv.second.nodeId += idOffset;
  }
  for (auto &kv : outBoundaries)
  {
    kv.second.nodeId += idOffset;
  }

  // ----- Classify parent edges touching the instance -----
  // Walk gd.edges once, collecting edge indices that connect to the instance on one end.
  // `incomingByName[ifaceName]` is the (single) parent edge driving the named input pin;
  // duplicates fail multi-source. `outgoingByName[ifaceName]` collects all parent consumers
  // of the named output pin (fan-out is legal there).
  eastl::hash_map<eastl::string, int> incomingByName;
  eastl::hash_map<eastl::string, eastl::vector<int>> outgoingByName;
  bool wiringError = false;
  for (int i = 0; i < static_cast<int>(gd.edges.size()); ++i)
  {
    const GraphData::Edge &e = gd.edges[i];
    int instPinIdx = -1;
    if (e.elemA == instance_id)
    {
      instPinIdx = e.pinA;
    }
    else if (e.elemB == instance_id)
    {
      instPinIdx = e.pinB;
    }
    if (instPinIdx < 0 || instPinIdx >= static_cast<int>(instance.pins.size()))
    {
      continue;
    }
    const GraphData::Pin &p = instance.pins[instPinIdx];
    const eastl::string &name = p.name;
    if (p.role == PinRole::In)
    {
      auto it = incomingByName.find(name);
      if (it != incomingByName.end())
      {
        debug("graphEditor: subgraph instance id=%d input pin '%s' has multiple sources -- rejected", instance_id, name.c_str());
        wiringError = true;
        break;
      }
      incomingByName[name] = i;
    }
    else
    {
      outgoingByName[name].push_back(i);
    }
  }
  if (wiringError)
  {
    return;
  }

  // ----- Surgery: input boundaries -----
  // For each input boundary, look up the parent edge driving the matching named pin. The
  // boundary's interior-side edges (those originating at boundary.nodeId / boundary.pinIdx)
  // get their (elemA, pinA) rewritten to the parent source so the original consumer in the
  // child reads the parent source directly. If the parent has no source for this pin, drop
  // the interior edges and the consumer falls back to def_val via substitute() at compile.
  for (const auto &kv : inBoundaries)
  {
    const eastl::string &name = kv.first;
    const InBoundary &b = kv.second;

    auto incIt = incomingByName.find(name);
    if (incIt == incomingByName.end())
    {
      // No parent source -- drop interior edges from this boundary; cleanup below removes
      // boundary-touching edges anyway, so nothing to do here.
      continue;
    }
    const GraphData::Edge &parentEdge = gd.edges[incIt->second];
    int parentSrcElem = -1;
    int parentSrcPin = -1;
    if (parentEdge.elemA == instance_id)
    {
      parentSrcElem = parentEdge.elemB;
      parentSrcPin = parentEdge.pinB;
    }
    else
    {
      parentSrcElem = parentEdge.elemA;
      parentSrcPin = parentEdge.pinA;
    }

    for (GraphData::Edge &ie : working.edges)
    {
      if (ie.elemA == b.nodeId && ie.pinA == b.pinIdx)
      {
        ie.elemA = parentSrcElem;
        ie.pinA = parentSrcPin;
      }
      if (ie.elemB == b.nodeId && ie.pinB == b.pinIdx)
      {
        ie.elemB = parentSrcElem;
        ie.pinB = parentSrcPin;
      }
    }
  }

  // ----- Surgery: output boundaries -----
  // Find the interior edge that drives each output boundary's in-pin (its upstream producer).
  // For every parent consumer of the matching named pin, rewrite the parent edge endpoint
  // to point at the producer. If the boundary has no upstream producer, drop the parent
  // consumer edges entirely -- nothing to wire.
  for (const auto &kv : outBoundaries)
  {
    const eastl::string &name = kv.first;
    const OutBoundary &b = kv.second;

    int producerElem = -1;
    int producerPin = -1;
    for (const GraphData::Edge &ie : working.edges)
    {
      if (ie.elemA == b.nodeId && ie.pinA == b.pinIdx)
      {
        producerElem = ie.elemB;
        producerPin = ie.pinB;
        break;
      }
      if (ie.elemB == b.nodeId && ie.pinB == b.pinIdx)
      {
        producerElem = ie.elemA;
        producerPin = ie.pinA;
        break;
      }
    }

    auto outIt = outgoingByName.find(name);
    if (outIt == outgoingByName.end())
    {
      continue;
    }
    if (producerElem < 0)
    {
      // No upstream -- leave the parent edges referring to the instance; the cleanup step
      // below removes them. Compile then sees the downstream consumer pins as unconnected
      // and falls back to def_val.
      continue;
    }
    for (int edgeIdx : outIt->second)
    {
      GraphData::Edge &pe = gd.edges[edgeIdx];
      if (pe.elemA == instance_id)
      {
        pe.elemA = producerElem;
        pe.pinA = producerPin;
      }
      else if (pe.elemB == instance_id)
      {
        pe.elemB = producerElem;
        pe.pinB = producerPin;
      }
    }
  }

  // ----- Cleanup: remove boundary nodes + edges that still touch boundary nodes or the
  // instance itself. The instance-touching parent edges are the unwired ones (no boundary
  // matched their pin name, or the boundary had no producer/source); they get dropped so
  // the downstream consumer pin falls back to def_val via substitute() at compile time.
  eastl::hash_set<int> boundaryNodeIds;
  for (const auto &kv : inBoundaries)
  {
    boundaryNodeIds.insert(kv.second.nodeId);
  }
  for (const auto &kv : outBoundaries)
  {
    boundaryNodeIds.insert(kv.second.nodeId);
  }

  working.edges.erase(
    eastl::remove_if(working.edges.begin(), working.edges.end(),
      [&](const GraphData::Edge &e) { return boundaryNodeIds.count(e.elemA) > 0 || boundaryNodeIds.count(e.elemB) > 0; }),
    working.edges.end());

  working.nodes.erase(eastl::remove_if(working.nodes.begin(), working.nodes.end(),
                        [&](const GraphData::Node &n) { return boundaryNodeIds.count(n.id) > 0; }),
    working.nodes.end());

  // ----- Splice the working child into the parent -----
  gd.nodes.reserve(gd.nodes.size() + working.nodes.size());
  for (GraphData::Node &n : working.nodes)
  {
    gd.nodes.push_back(eastl::move(n));
  }
  gd.edges.reserve(gd.edges.size() + working.edges.size());
  for (const GraphData::Edge &e : working.edges)
  {
    gd.edges.push_back(e);
  }

  // Drop any parent edge still pointing at the now-removed instance (these are edges whose
  // pin name didn't match a boundary, or whose boundary was unwired -- treated as disconnected
  // consumers / floating sources by the compile step).
  gd.edges.erase(eastl::remove_if(gd.edges.begin(), gd.edges.end(),
                   [&](const GraphData::Edge &e) { return e.elemA == instance_id || e.elemB == instance_id; }),
    gd.edges.end());

  gd.nodes.erase(eastl::remove_if(gd.nodes.begin(), gd.nodes.end(), [&](const GraphData::Node &n) { return n.id == instance_id; }),
    gd.nodes.end());
}

bool expand_recursive(GraphData &gd, ExpandContext &ctx)
{
  // Snapshot ids first: each splice mutates gd.nodes (append + erase), and capturing indices
  // pre-loop would alias to wrong nodes after the first splice. Ids are stable across splices.
  eastl::vector<int> instanceIds;
  for (const GraphData::Node &n : gd.nodes)
  {
    if (n.plugin == "subgraph")
    {
      instanceIds.push_back(n.id);
    }
  }
  for (int id : instanceIds)
  {
    splice_one(gd, id, ctx);
  }
  return true;
}

} // namespace

bool expand_subgraphs(GraphData &gd, const DataBlock &base_nodes_blk, const char *graphs_root_dir)
{
  ExpandContext ctx{base_nodes_blk, eastl::string(graphs_root_dir ? graphs_root_dir : ""), {}, {}};
  return expand_recursive(gd, ctx);
}
