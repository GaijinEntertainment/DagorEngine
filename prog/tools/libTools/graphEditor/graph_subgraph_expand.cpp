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
#include <math.h>

namespace
{

// What one splice did, for the editor's explode, which applies it as an edit.
struct SpliceReport
{
  eastl::vector<int> addedNodeIds;
  eastl::vector<int> transplantedEdgeIds; // parent edges into the instance whose source now feeds the body
};

// Per-compile cache of loaded children + a recursion stack. Owned by the top-level entry point
// and threaded through `expand_recursive` so two instances of the same child file share a single
// parse and so cycle detection observes the whole call chain.
struct ExpandContext
{
  const DataBlock &baseNodesBlk;
  eastl::string rootDir; // ends with '/'
  eastl::hash_map<eastl::string, GraphData> cache;
  eastl::vector<eastl::string> stack;
  // Top-level graph only; nulled while a child expands, whose ids are not in this id space yet.
  eastl::hash_map<uint64_t, uint64_t> *instanceOutputSource = nullptr;
  // constraint: never goes backwards. A splice that leaves nothing behind lets the graph max drop,
  // and a reused instance id breaks instanceOutputSource.
  int idFloor = 0;
  bool recurse = true; // the editor's explode keeps nested instances as they are
  // Top-level splice only, nulled while a child expands, like instanceOutputSource.
  SpliceReport *report = nullptr;
  eastl::vector<eastl::string> stackStems;
  SubgraphExpandError *error = nullptr;
  bool failed = false;
};

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

void expand_recursive(GraphData &gd, ExpandContext &ctx);

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

  auto bail = [&](const String &msg) {
    debug("graphEditor: %s", msg.str());
    ctx.failed = true;
    if (ctx.error && ctx.error->message.empty())
    {
      ctx.error->message = msg.str();
      ctx.error->chain = ctx.stackStems;
      ctx.error->chain.push_back(descName);
    }
  };

  eastl::string childPath = resolve_child_path(ctx.rootDir, descName.c_str());
  if (childPath.empty())
  {
    bail(String(0, "subgraph instance id=%d references missing file '%s.subgraph.blk' (under '%s')", instance_id, descName.c_str(),
      ctx.rootDir.c_str()));
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
      bail(String(0, "subgraph recursion detected: %s", chain.c_str()));
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
      bail(String(0, "subgraph '%s' failed to load", childPath.c_str()));
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
        bail(String(0, "subgraph '%s' schema error: %s", childPath.c_str(), format_subgraph_schema_error(err).c_str()));
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
  if (ctx.recurse)
  {
    ctx.stack.push_back(childPath);
    ctx.stackStems.push_back(descName);
    eastl::hash_map<uint64_t, uint64_t> *const parentOutputSource = ctx.instanceOutputSource;
    SpliceReport *const parentReport = ctx.report;
    ctx.instanceOutputSource = nullptr;
    ctx.report = nullptr;
    expand_recursive(working, ctx);
    ctx.instanceOutputSource = parentOutputSource;
    ctx.report = parentReport;
    ctx.stackStems.pop_back();
    ctx.stack.pop_back();
  }

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
    const bool isIn = is_input_boundary(n);
    const bool isOut = is_output_boundary(n);
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
      bail(String(0, "subgraph '%s' has a %s boundary with empty name and no edges to derive from", childPath.c_str(),
        isIn ? "input" : "output"));
      boundaryError = true;
      continue;
    }
    if (isIn)
    {
      if (inBoundaries.find(ifaceName) != inBoundaries.end())
      {
        bail(String(0, "subgraph '%s' has duplicate input boundary named '%s'", childPath.c_str(), ifaceName.c_str()));
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
        bail(String(0, "subgraph '%s' input boundary '%s' has no out-role pin", childPath.c_str(), ifaceName.c_str()));
        boundaryError = true;
        continue;
      }
      inBoundaries[ifaceName] = InBoundary{n.id, pinIdx};
    }
    else
    {
      if (outBoundaries.find(ifaceName) != outBoundaries.end())
      {
        bail(String(0, "subgraph '%s' has duplicate output boundary named '%s'", childPath.c_str(), ifaceName.c_str()));
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
        bail(String(0, "subgraph '%s' output boundary '%s' has no in-role pin", childPath.c_str(), ifaceName.c_str()));
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
  // constraint: deterministic, because `%graph_hash%` substitution downstream depends on the ids.
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
  const int idOffset = eastl::max(maxId, ctx.idFloor) + 1;
  for (GraphData::Node &n : working.nodes)
  {
    n.id += idOffset;
    ctx.idFloor = eastl::max(ctx.idFloor, n.id);
  }
  for (GraphData::Edge &e : working.edges)
  {
    e.id += idOffset;
    ctx.idFloor = eastl::max(ctx.idFloor, e.id);
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
      if (it == incomingByName.end())
      {
        incomingByName[name] = i;
        continue;
      }
      if (!gd.edges[it->second].muted && !e.muted)
      {
        bail(String(0, "subgraph instance id=%d input pin '%s' has multiple sources -- rejected", instance_id, name.c_str()));
        wiringError = true;
        break;
      }
      // A muted duplicate carries no data, so it is not a genuine ambiguity. Keep whichever
      // edge is live so the boundary transplants the real source.
      if (!e.muted)
      {
        it->second = i;
      }
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

    bool transplanted = false;
    for (GraphData::Edge &ie : working.edges)
    {
      if (ie.elemA == b.nodeId && ie.pinA == b.pinIdx)
      {
        ie.elemA = parentSrcElem;
        ie.pinA = parentSrcPin;
        ie.muted = ie.muted || parentEdge.muted;
        transplanted = true;
      }
      if (ie.elemB == b.nodeId && ie.pinB == b.pinIdx)
      {
        ie.elemB = parentSrcElem;
        ie.pinB = parentSrcPin;
        ie.muted = ie.muted || parentEdge.muted;
        transplanted = true;
      }
    }
    if (transplanted && ctx.report)
    {
      ctx.report->transplantedEdgeIds.push_back(parentEdge.id);
    }
  }

  // ----- Surgery: output boundaries -----
  // Find the interior edge that drives each output boundary's in-pin (its upstream producer).
  // For every parent consumer of the matching named pin, rewrite the parent edge endpoint
  // to point at the producer. If the boundary has no upstream producer, drop the parent
  // consumer edges entirely -- nothing to wire.
  // A pass-through child with an unwired input leaves its in-boundary as the producer, and cleanup
  // below deletes it, so that counts as no producer.
  eastl::hash_set<int> boundaryNodeIds;
  for (const auto &kv : inBoundaries)
  {
    boundaryNodeIds.insert(kv.second.nodeId);
  }
  for (const auto &kv : outBoundaries)
  {
    boundaryNodeIds.insert(kv.second.nodeId);
  }

  for (const auto &kv : outBoundaries)
  {
    const eastl::string &name = kv.first;
    const OutBoundary &b = kv.second;

    int producerElem = -1;
    int producerPin = -1;
    bool producerMuted = false;
    for (const GraphData::Edge &ie : working.edges)
    {
      const bool matchA = (ie.elemA == b.nodeId && ie.pinA == b.pinIdx);
      const bool matchB = (ie.elemB == b.nodeId && ie.pinB == b.pinIdx);
      if (!matchA && !matchB)
      {
        continue;
      }
      producerElem = matchA ? ie.elemB : ie.elemA;
      producerPin = matchA ? ie.pinB : ie.pinA;
      producerMuted = ie.muted;
      if (!producerMuted)
      {
        break; // a live producer wins outright; a muted one is only a fallback
      }
    }

    // The pin index comes off the instance itself: an output nobody consumes has no parent edge to
    // read it from. A muted producer feeds nothing once the parent edge is rewired.
    for (int pi = 0; pi < static_cast<int>(instance.pins.size()); ++pi)
    {
      if (instance.pins[pi].role != PinRole::Out || instance.pins[pi].name != name)
      {
        continue;
      }
      if (ctx.instanceOutputSource && producerElem >= 0 && !producerMuted && boundaryNodeIds.count(producerElem) == 0)
      {
        (*ctx.instanceOutputSource)[make_pin_id(instance_id, pi)] = make_pin_id(producerElem, producerPin);
      }
      break;
    }

    auto outIt = outgoingByName.find(name);
    if (outIt == outgoingByName.end())
    {
      continue;
    }
    if (producerElem < 0 || boundaryNodeIds.count(producerElem) > 0)
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
      pe.muted = pe.muted || producerMuted;
    }
  }

  // ----- Cleanup: remove boundary nodes + edges that still touch boundary nodes or the
  // instance itself. The instance-touching parent edges are the unwired ones (no boundary
  // matched their pin name, or the boundary had no producer/source); they get dropped so
  // the downstream consumer pin falls back to def_val via substitute() at compile time.
  // `subgraph separator` is a pinless section-label boundary (pure UI, exposed by the JS
  // editor as a non-connectable divider pin) -- never splice it into the parent.
  for (const GraphData::Node &n : working.nodes)
  {
    if (n.descName == "subgraph separator")
    {
      boundaryNodeIds.insert(n.id);
    }
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
    if (ctx.report)
    {
      ctx.report->addedNodeIds.push_back(n.id);
    }
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

void expand_recursive(GraphData &gd, ExpandContext &ctx)
{
  // Snapshot ids first: each splice mutates gd.nodes (append + erase), and capturing indices
  // pre-loop would alias to wrong nodes after the first splice. Ids are stable across splices.
  eastl::vector<int> instanceIds;
  for (const GraphData::Node &n : gd.nodes)
  {
    if (is_subgraph_instance(n))
    {
      instanceIds.push_back(n.id);
    }
  }
  for (int id : instanceIds)
  {
    splice_one(gd, id, ctx);
  }
}

} // namespace

bool expand_subgraphs(GraphData &gd, const DataBlock &base_nodes_blk, const char *graphs_root_dir,
  eastl::hash_map<uint64_t, uint64_t> *out_instance_output_source, SubgraphExpandError *out_error)
{
  ExpandContext ctx{base_nodes_blk, eastl::string(graphs_root_dir ? graphs_root_dir : ""), {}, {}, out_instance_output_source};
  if (out_error)
  {
    *out_error = SubgraphExpandError();
  }
  ctx.error = out_error;
  expand_recursive(gd, ctx);
  return !ctx.failed;
}

bool explode_subgraph_instance(const GraphData &gd, int instance_id, const DataBlock &base_nodes_blk, const char *graphs_root_dir,
  float snap, ExplodeResult &out)
{
  out = ExplodeResult();
  const GraphData::Node *const instance = find_node_by_id(gd, instance_id);
  if (!instance || !is_subgraph_instance(*instance))
  {
    out.error = "not a subgraph instance";
    return false;
  }

  // Nodes and edges only: all the splice reads, and the compiled BLKs belong to the texgen worker.
  GraphData working;
  working.nodes = gd.nodes;
  working.edges = gd.edges;
  SpliceReport report;
  SubgraphExpandError failure;
  ExpandContext ctx{base_nodes_blk, eastl::string(graphs_root_dir ? graphs_root_dir : "")};
  ctx.recurse = false;
  ctx.report = &report;
  ctx.error = &failure;
  splice_one(working, instance_id, ctx);
  if (find_node_by_id(working, instance_id))
  {
    out.error = failure.message.empty() ? eastl::string("the subgraph could not be spliced") : failure.message;
    return false;
  }

  // The body's centroid lands on the instance, so an instance Create placed explodes back in place.
  const eastl::hash_set<int> added(report.addedNodeIds.begin(), report.addedNodeIds.end());
  float cx = 0.0f;
  float cy = 0.0f;
  for (const GraphData::Node &n : working.nodes)
  {
    if (added.count(n.id))
    {
      cx += n.x;
      cy += n.y;
    }
  }
  const float count = static_cast<float>(eastl::max<size_t>(added.size(), 1));
  auto snapped = [snap](float v) { return snap > 0.0f ? roundf(v / snap) * snap : v; };
  // The centroid rounds as Create rounded the instance, so an unmoved one gives exactly 0 even at a half step.
  const float dx = snapped(instance->x - snapped(cx / count));
  const float dy = snapped(instance->y - snapped(cy / count));
  for (GraphData::Node &n : working.nodes)
  {
    if (added.count(n.id))
    {
      n.x += dx;
      n.y += dy;
      out.addedNodes.push_back(n);
    }
  }

  eastl::hash_set<int> instanceEdgeIds;
  for (const GraphData::Edge &e : gd.edges)
  {
    if (e.elemA == instance_id || e.elemB == instance_id)
    {
      instanceEdgeIds.insert(e.id);
    }
  }
  // A rewired parent edge keeps its id; through a pass-through it touches no body node at all.
  for (const GraphData::Edge &e : working.edges)
  {
    if (added.count(e.elemA) || added.count(e.elemB) || instanceEdgeIds.count(e.id))
    {
      out.addedEdges.push_back(e);
    }
  }
  const eastl::hash_set<int> transplanted(report.transplantedEdgeIds.begin(), report.transplantedEdgeIds.end());
  for (const GraphData::Edge &e : gd.edges)
  {
    if (instanceEdgeIds.count(e.id) && !transplanted.count(e.id) && !find_edge_by_id(working, e.id))
    {
      out.droppedParentEdges.push_back(e);
    }
  }
  return true;
}
