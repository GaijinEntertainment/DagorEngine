// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <graphEditor/graph_subgraph_extract.h>

#include <graphEditor/graph_subgraph_expand.h>

#include <ioSys/dag_dataBlock.h>

#include <EASTL/algorithm.h>
#include <EASTL/hash_map.h>
#include <EASTL/hash_set.h>

#include <cstring>

namespace
{
// Where a synthesized boundary lands, relative to the inner node it serves.
constexpr float IN_BOUNDARY_DX = -800.0f;
constexpr float OUT_BOUNDARY_DX = 450.0f;
constexpr float BOUNDARY_DY = 80.0f;

int first_pin_with_role(const GraphData::Node &n, PinRole role)
{
  for (int i = 0; i < static_cast<int>(n.pins.size()); ++i)
  {
    if (n.pins[i].role == role)
    {
      return i;
    }
  }
  return -1;
}

void set_boundary_name(GraphData::Node &n, const eastl::string &name)
{
  for (eastl::pair<eastl::string, eastl::string> &pv : n.propertyValues)
  {
    if (pv.first == "name")
    {
      pv.second = name;
      return;
    }
  }
  n.propertyValues.emplace_back(eastl::string("name"), name);
}

eastl::string trimmed(const eastl::string &s)
{
  size_t begin = 0;
  size_t end = s.size();
  while (begin < end && (s[begin] == ' ' || s[begin] == '\t'))
  {
    ++begin;
  }
  while (end > begin && (s[end - 1] == ' ' || s[end - 1] == '\t'))
  {
    --end;
  }
  return s.substr(begin, end - begin);
}

// Pins match by name only, so an input and an output may not share one either.
eastl::string take_unique_name(eastl::hash_set<eastl::string> &used, const eastl::string &base)
{
  eastl::string name = base;
  for (int suffix = 2; used.count(name) != 0; ++suffix)
  {
    name.sprintf("%s_%d", base.c_str(), suffix);
  }
  used.insert(name);
  return name;
}

// The type the producer carries when that is known, else the first type both ends accept; either way one
// that has a boundary node. Pins store no propagated type.
const DataBlock *find_input_boundary_desc(const DataBlock &base_nodes_blk, const GraphData::Pin &producer,
  const GraphData::Pin &consumer, PinType carried)
{
  auto boundaryFor = [&](PinType t) -> const DataBlock * {
    const bool accepted =
      consumer.types.empty() || eastl::find(consumer.types.begin(), consumer.types.end(), t) != consumer.types.end();
    const char *token = pin_type_to_string(t);
    if (!accepted || !*token)
    {
      return nullptr;
    }
    const eastl::string descName(eastl::string::CtorSprintf(), "subgraph in: %s", token);
    return find_node_desc_by_name(base_nodes_blk, descName.c_str());
  };
  if (const DataBlock *desc = boundaryFor(carried))
  {
    return desc;
  }
  for (PinType t : producer.types.empty() ? consumer.types : producer.types)
  {
    if (const DataBlock *desc = boundaryFor(t))
    {
      return desc;
    }
  }
  return nullptr;
}

// A parent edge with exactly one end in the extracted set.
struct Candidate
{
  const GraphData::Edge *edge = nullptr;
  int innerNode = -1;
  int innerPin = -1;
  int outsideNode = -1;
  int outsidePin = -1;
  bool isInput = false; // the producer is outside
};

ExtractCrossing crossing_of(const Candidate &c, int boundary_id, bool muted)
{
  ExtractCrossing x;
  x.parentEdgeId = c.edge->id;
  x.outsideNode = c.outsideNode;
  x.outsidePin = c.outsidePin;
  x.boundaryNodeId = boundary_id;
  x.isInput = c.isInput;
  x.muted = muted;
  return x;
}
} // namespace

bool extract_subgraph(const GraphData &parent, dag::ConstSpan<int> node_ids, const DataBlock &base_nodes_blk,
  const char *graphs_root_dir, GraphData &out_child, ExtractPlan &out_plan)
{
  out_plan = ExtractPlan();
  clear_graph_data(out_child);

  eastl::hash_map<int, int> nodeIndex;
  build_node_index(parent, nodeIndex);
  auto nodeOf = [&](int id) -> const GraphData::Node * {
    auto it = nodeIndex.find(id);
    return it != nodeIndex.end() ? &parent.nodes[it->second] : nullptr;
  };

  // Expanded on the first instance that feeds the set: only its body knows the one type an output carries.
  GraphData flat;
  eastl::hash_map<uint64_t, uint64_t> outputSource;
  bool expanded = false;
  auto carriedType = [&](int node_id, int pin_index) {
    const GraphData::Node *const node = nodeOf(node_id);
    if (!node || !is_subgraph_instance(*node) || !graphs_root_dir || !*graphs_root_dir)
    {
      return PinType::Unknown;
    }
    if (!expanded)
    {
      expanded = true;
      // Nodes and edges only: all the expansion reads, and the compiled BLKs belong to the texgen worker.
      flat.nodes = parent.nodes;
      flat.edges = parent.edges;
      expand_subgraphs(flat, base_nodes_blk, graphs_root_dir, &outputSource);
    }
    uint64_t key = make_pin_id(node_id, pin_index);
    for (size_t hop = 0; hop < outputSource.size(); ++hop)
    {
      auto it = outputSource.find(key);
      if (it == outputSource.end())
      {
        break;
      }
      key = it->second;
    }
    int srcNode = -1;
    int srcPin = -1;
    decode_pin_id(key, srcNode, srcPin);
    const GraphData::Pin *const src = find_pin(flat, srcNode, srcPin);
    return src && src->types.size() == 1 ? src->types.front() : PinType::Unknown;
  };

  eastl::hash_set<int> extracted;
  bool anyPin = false;
  for (int id : node_ids)
  {
    const GraphData::Node *const n = nodeOf(id);
    if (!n)
    {
      continue;
    }
    if (is_input_boundary(*n) || is_output_boundary(*n) || n->descName == "subgraph separator")
    {
      out_plan.error = "A subgraph boundary node (subgraph in, out or separator) cannot go into a new subgraph.";
      return false;
    }
    extracted.insert(id);
    anyPin = anyPin || !n->pins.empty();
  }
  if (extracted.empty())
  {
    out_plan.error = "No node is selected.";
    return false;
  }
  if (!anyPin)
  {
    out_plan.error = "None of the selected nodes has a pin.";
    return false;
  }
  const DataBlock *const outDesc = find_node_desc_by_name(base_nodes_blk, "subgraph out");
  if (!outDesc)
  {
    out_plan.error = "base_nodes.blk has no 'subgraph out' node.";
    return false;
  }

  out_child.graphTextureWidth = parent.graphTextureWidth;
  out_child.graphTextureHeight = parent.graphTextureHeight;
  out_child.graphTextureDepth = parent.graphTextureDepth;
  out_child.graphTextureType = parent.graphTextureType;
  out_child.graphTextureWrap = parent.graphTextureWrap;
  for (const GraphData::Node &n : parent.nodes)
  {
    if (extracted.count(n.id))
    {
      out_child.nodes.push_back(n);
      out_plan.centroidX += n.x;
      out_plan.centroidY += n.y;
    }
  }
  out_plan.centroidX /= static_cast<float>(out_child.nodes.size());
  out_plan.centroidY /= static_cast<float>(out_child.nodes.size());

  eastl::hash_map<uint64_t, eastl::vector<Candidate>> byInnerPin;
  for (const GraphData::Edge &e : parent.edges)
  {
    const bool inA = extracted.count(e.elemA) != 0;
    const bool inB = extracted.count(e.elemB) != 0;
    if (inA && inB)
    {
      out_child.edges.push_back(e);
      continue;
    }
    if (!inA && !inB)
    {
      continue;
    }
    Candidate c;
    c.edge = &e;
    c.innerNode = inA ? e.elemA : e.elemB;
    c.innerPin = inA ? e.pinA : e.pinB;
    c.outsideNode = inA ? e.elemB : e.elemA;
    c.outsidePin = inA ? e.pinB : e.pinA;
    int producerNode = -1;
    int producerPin = -1;
    if (!nodeOf(c.outsideNode) || !edge_source_pin(parent, e, producerNode, producerPin))
    {
      out_plan.warnings.push_back(eastl::string(eastl::string::CtorSprintf(), "edge %d has no output end, dropped", e.id));
      continue;
    }
    c.isInput = producerNode == c.outsideNode && producerPin == c.outsidePin;
    byInnerPin[make_pin_id(c.innerNode, c.innerPin)].push_back(c);
  }

  // Above every parent id, so they clash neither with kept ids nor with copied boundaries.
  int nextNodeId = 0;
  for (const GraphData::Node &n : parent.nodes)
  {
    nextNodeId = eastl::max(nextNodeId, n.id + 1);
  }
  int nextEdgeId = next_edge_id(parent);

  struct Boundary
  {
    int index = -1;         // into out_child.nodes, which only grows
    eastl::string baseName; // empty for a copied one: it carries its own
    bool copied = false;
  };
  eastl::vector<Boundary> boundaries;
  eastl::hash_set<int> copiedIds;
  eastl::vector<ExtractCrossing> inputs;
  eastl::vector<ExtractCrossing> outputs;
  bool hasOutputs = false; // not !outputs.empty(): a hidden texture output has a boundary and no crossing

  auto copyBoundary = [&](int boundary_id) {
    if (!copiedIds.insert(boundary_id).second)
    {
      return false;
    }
    boundaries.push_back(Boundary{static_cast<int>(out_child.nodes.size()), {}, true});
    out_child.nodes.push_back(*nodeOf(boundary_id));
    return true;
  };
  auto synthesizeBoundary = [&](const DataBlock &desc, float x, float y, const char *base_name) {
    GraphData::Node node;
    make_node_from_descriptor(base_nodes_blk, desc, x, y, node);
    node.id = nextNodeId++;
    boundaries.push_back(Boundary{static_cast<int>(out_child.nodes.size()), eastl::string(base_name), false});
    out_child.nodes.push_back(eastl::move(node));
    return out_child.nodes.back().id;
  };

  // One pass in (node, pin, edge) order: the boundaries land in the child in that order, so the
  // registration's y sort breaks its ties the same way every time.
  for (const GraphData::Node &n : parent.nodes)
  {
    if (!extracted.count(n.id))
    {
      continue;
    }
    for (int p = 0; p < static_cast<int>(n.pins.size()); ++p)
    {
      const GraphData::Pin &pin = n.pins[p];
      eastl::vector<const Candidate *> outs;
      eastl::vector<const Candidate *> ins;
      auto it = byInnerPin.find(make_pin_id(n.id, p));
      if (it != byInnerPin.end())
      {
        for (const Candidate &c : it->second)
        {
          (c.isInput ? ins : outs).push_back(&c);
        }
      }
      eastl::string pinName = trimmed(pin.comment);
      if (pinName.empty())
      {
        pinName = trimmed(pin.name);
      }

      // A hidden texture output is exported even when nothing reads it.
      const bool hiddenTexture =
        pin.role == PinRole::Out && pin.hidden && !pin.types.empty() && is_texture_or_particles(pin.types.front());
      if (!outs.empty() || hiddenTexture)
      {
        int boundaryId = -1;
        for (const Candidate *c : outs)
        {
          // The parent's own subgraph out names the interface.
          if (is_output_boundary(*nodeOf(c->outsideNode)) && copyBoundary(c->outsideNode))
          {
            boundaryId = c->outsideNode;
            GraphData::Edge copy = *c->edge;
            copy.muted = false; // the mute stays on the parent edge
            out_child.edges.push_back(copy);
            break;
          }
        }
        if (boundaryId < 0)
        {
          boundaryId =
            synthesizeBoundary(*outDesc, n.x + OUT_BOUNDARY_DX, n.y + p * BOUNDARY_DY, pinName.empty() ? "out" : pinName.c_str());
          const int boundaryPin = first_pin_with_role(out_child.nodes.back(), PinRole::In);
          out_child.edges.push_back(GraphData::Edge{nextEdgeId++, n.id, p, boundaryId, boundaryPin, false});
        }
        for (const Candidate *c : outs)
        {
          outputs.push_back(crossing_of(*c, boundaryId, c->edge->muted));
        }
        hasOutputs = true;
      }

      int stacked = 0;
      for (const Candidate *c : ins)
      {
        const GraphData::Node &outside = *nodeOf(c->outsideNode);
        if (is_input_boundary(outside))
        {
          if (!copyBoundary(outside.id))
          {
            continue; // an earlier pin fed by it took its one crossing
          }
          // Its edges into the set come along and one parent edge feeds the instance, so a mute stays
          // on the parent side unless it was mixed, which only the child can keep.
          eastl::vector<GraphData::Edge> fanOut;
          for (const GraphData::Edge &e : parent.edges)
          {
            if ((e.elemA == outside.id && extracted.count(e.elemB)) || (e.elemB == outside.id && extracted.count(e.elemA)))
            {
              fanOut.push_back(e);
            }
          }
          const bool allMuted = eastl::all_of(fanOut.begin(), fanOut.end(), [](const GraphData::Edge &e) { return e.muted; });
          for (GraphData::Edge &e : fanOut)
          {
            e.muted = e.muted && !allMuted;
            out_child.edges.push_back(e);
          }
          inputs.push_back(crossing_of(*c, outside.id, allMuted));
          continue;
        }

        const GraphData::Pin *const producer = find_pin(parent, c->outsideNode, c->outsidePin);
        const DataBlock *const desc =
          producer ? find_input_boundary_desc(base_nodes_blk, *producer, pin, carriedType(c->outsideNode, c->outsidePin)) : nullptr;
        if (!desc)
        {
          out_plan.warnings.push_back(
            eastl::string(eastl::string::CtorSprintf(), "edge %d: no subgraph input takes its type, cut", c->edge->id));
          continue;
        }
        const int boundaryId = synthesizeBoundary(*desc, n.x + IN_BOUNDARY_DX, n.y + (p + stacked++) * BOUNDARY_DY,
          pinName.empty() ? "in" : pinName.c_str());
        const int boundaryPin = first_pin_with_role(out_child.nodes.back(), PinRole::Out);
        out_child.edges.push_back(GraphData::Edge{nextEdgeId++, boundaryId, boundaryPin, n.id, p, false});
        inputs.push_back(crossing_of(*c, boundaryId, c->edge->muted));
      }
    }
  }
  if (!hasOutputs)
  {
    out_plan.error = "The selection feeds nothing outside it. A subgraph needs at least one output.";
    return false;
  }

  // Copied boundaries are named first: theirs are the parent's interface names.
  eastl::hash_set<eastl::string> used;
  eastl::hash_map<int, eastl::string> nameOf;
  for (int pass = 0; pass < 2; ++pass)
  {
    for (const Boundary &b : boundaries)
    {
      if (b.copied != (pass == 0))
      {
        continue;
      }
      GraphData::Node &node = out_child.nodes[b.index];
      // A copy keeps a name it derives from its edge; only a clash writes one.
      const eastl::string current = b.copied ? effective_subgraph_boundary_name(out_child, node.id) : eastl::string();
      eastl::string base = b.copied ? current : b.baseName;
      if (base.empty())
      {
        base = is_input_boundary(node) ? "in" : "out";
      }
      const eastl::string name = take_unique_name(used, base);
      if (name != current)
      {
        set_boundary_name(node, name);
      }
      nameOf[node.id] = name;
    }
  }
  for (ExtractCrossing &x : inputs)
  {
    x.ifaceName = nameOf[x.boundaryNodeId];
  }
  for (ExtractCrossing &x : outputs)
  {
    x.ifaceName = nameOf[x.boundaryNodeId];
  }
  out_plan.crossings = eastl::move(inputs);
  out_plan.crossings.insert(out_plan.crossings.end(), outputs.begin(), outputs.end());
  return true;
}
