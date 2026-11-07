// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <graphEditor/graph_data.h>
#include <gameLibs/textureGen/registry/texgenRegistry_api.h>

#include <debug/dag_debug.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <perfMon/dag_cpuFreq.h>
#include <util/dag_string.h>

#include <EASTL/algorithm.h>
#include <EASTL/hash_map.h>
#include <EASTL/hash_set.h>
#include <EASTL/sort.h>

#include <cstdlib>
#include <cstring>
#include <float.h>

namespace
{
constexpr const char *UNNAMED_NODE = "<unnamed>";

// Pin shape (type/role/hidden) is derived from the descriptor at render time, not stored on
// the loaded Node. Both rules that previously lived here -- the shader_editor "non-texture
// outputs hidden" rule and the per-(desc,pin) flat hidden table mirroring mainNodes.js's
// `hidden:true` flags -- now live on the descriptor side: base_nodes.blk encodes
// `pin{ hidden:b = yes }` directly, and the shader_editor visibility rule is evaluated by
// resolve_node_pins below, against the descriptor's `types:t` / `role:t`.

PinRole parse_pin_role(const char *role)
{
  if (!role)
  {
    return PinRole::In;
  }
  if (strcmp(role, "out") == 0)
  {
    return PinRole::Out;
  }
  if (strcmp(role, "any") == 0)
  {
    return PinRole::Any;
  }
  if (strcmp(role, "ctrl") == 0)
  {
    return PinRole::Ctrl;
  }
  return PinRole::In;
}

const char *pin_role_to_string(PinRole r)
{
  switch (r)
  {
    case PinRole::In: return "in";
    case PinRole::Out: return "out";
    case PinRole::Any: return "any";
    case PinRole::Ctrl: return "ctrl";
  }
  return "in";
}

// Reconciles a freshly-loaded Node's templateUid + descName against the descriptor
// registry. Runs once per node, after parse and before resolve_node_pins.
//
// Three paths:
//   1. templateUid set + descriptor found -> refresh node.descName from descriptor's
//      current name:t (so renamed templates show their new name in saved files on
//      round-trip; the uid is the stable lookup key).
//   2. templateUid empty + base_nodes_blk non-null -> legacy migration: linear-scan
//      by descName, populate templateUid from the matched descriptor's templateUid:t.
//      Silent auto-migrate: next save upgrades the BLK to carry templateUid.
//   3. Neither resolves -> leave both fields as-loaded; resolve_node_pins will render
//      pins hidden, properties panel shows "(no descriptor)".
//
// debug() logs the migrate and the miss so unexpected divergence is grep-able from
// the editor console without noisy con*().
void reconcile_template_uid(GraphData::Node &node, const DataBlock *base_nodes_blk)
{
  if (!base_nodes_blk)
  {
    return;
  }

  if (!node.templateUid.empty())
  {
    if (const DataBlock *const desc = find_node_desc(base_nodes_blk, node.templateUid.c_str()))
    {
      node.descName = desc->getStr("name", "");
      return;
    }
    debug("graphEditor: node id=%d templateUid '%s' (descName '%s') not found in base_nodes.blk", node.id, node.templateUid.c_str(),
      node.descName.c_str());
    return;
  }

  if (node.descName.empty())
  {
    return;
  }

  for (uint32_t i = 0; i < base_nodes_blk->blockCount(); ++i)
  {
    const DataBlock *b = base_nodes_blk->getBlock(i);
    if (strcmp(b->getBlockName(), "node") != 0)
    {
      continue;
    }
    if (strcmp(b->getStr("name", ""), node.descName.c_str()) == 0)
    {
      node.templateUid = b->getStr("templateUid", "");
      debug("graphEditor: migrated node id=%d descName '%s' -> templateUid '%s'", node.id, node.descName.c_str(),
        node.templateUid.c_str());
      return;
    }
  }
  // shader_editor sub-graph nodes (mask_from_index, multi_max, etc.) carry their
  // pin shape inside the saved instance and have no base_nodes.blk descriptor by
  // design -- silence the miss for those; resolve_node_pins handles them via the
  // "descriptor missing" branch.
  if (!node.plugin.empty())
  {
    return;
  }
  debug("graphEditor: node id=%d descName '%s' not found in base_nodes.blk (no templateUid, no name match)", node.id,
    node.descName.c_str());
}

void resolve_pin(GraphData::Pin &p, const DataBlock &desc_pin, bool is_shader_editor)
{
  p.role = parse_pin_role(desc_pin.getStr("role", "in"));
  p.isInput = (p.role != PinRole::Out);
  texgen::parse_pin_types_csv(desc_pin.getStr("types", ""), p.types);
  p.type = p.types.empty() ? PinType::Unknown : p.types.front();
  p.typeGroup.assign(desc_pin.getStr("typeGroup", ""));
  p.separator = desc_pin.getBool("separator", false);
  // Multi-connect by default; base_nodes.blk sets it on every pin, so only descriptor-less nodes rely on this.
  p.singleConnect = desc_pin.getBool("singleConnect", false);
  p.hidden = desc_pin.getBool("hidden", false);
  p.boundaryId = desc_pin.getInt("boundaryId", -1);
  // shader_editor: outputs not texture/particles are hidden (mirrors mainNodes.js GE_preprocessDescription).
  // Separators have no types and are exempt.
  if (is_shader_editor && !p.isInput && !p.separator && !is_texture_or_particles(p.type))
  {
    p.hidden = true;
  }
}

// Name and shape: what a pin shows of its descriptor. Per-instance fields and boundaryId are not compared.
bool same_pin_interface(const GraphData::Pin &a, const GraphData::Pin &b)
{
  return a.name == b.name && a.role == b.role && a.types == b.types && a.typeGroup == b.typeGroup &&
         a.singleConnect == b.singleConnect && a.hidden == b.hidden && a.separator == b.separator;
}

// Which old pins match by boundary node before names: none (file ids may be reused), all (live interface), or those the file has.
enum class BoundaryMatch
{
  NONE,
  ALL,
  FROM_FILE,
};

bool has_same_pin(const eastl::vector<GraphData::Pin> &pins, const GraphData::Pin &p)
{
  return eastl::any_of(pins.begin(), pins.end(),
    [&p](const GraphData::Pin &q) { return q.boundaryId == p.boundaryId && q.role == p.role && q.name == p.name; });
}

// For each old pin, the fresh pin it takes, or -1: by boundary node first as `boundary` allows, then by name over what is
// left, the role always equal and each fresh pin taken once.
eastl::vector<int> match_instance_pins(const eastl::vector<GraphData::Pin> &old_pins, const eastl::vector<GraphData::Pin> &fresh_pins,
  BoundaryMatch boundary)
{
  eastl::vector<int> taken(old_pins.size(), -1);
  eastl::vector<bool> available(fresh_pins.size(), true);
  auto takeFirst = [&](int i, bool boundary_pass) {
    const GraphData::Pin &old = old_pins[i];
    for (int j = 0; j < static_cast<int>(fresh_pins.size()); ++j)
    {
      const GraphData::Pin &fresh = fresh_pins[j];
      if (available[j] && fresh.role == old.role && (boundary_pass ? fresh.boundaryId == old.boundaryId : fresh.name == old.name))
      {
        taken[i] = j;
        available[j] = false;
        return;
      }
    }
  };
  for (int i = 0; boundary != BoundaryMatch::NONE && i < static_cast<int>(old_pins.size()); ++i)
  {
    const GraphData::Pin &old = old_pins[i];
    // A marked pin is the file's only under its own name: its node id may be a deleted boundary's, reused.
    if (old.boundaryId >= 0 && (boundary == BoundaryMatch::ALL || !old.addedLive || has_same_pin(fresh_pins, old)))
    {
      takeFirst(i, true);
    }
  }
  for (int i = 0; i < static_cast<int>(old_pins.size()); ++i)
  {
    if (taken[i] < 0)
    {
      takeFirst(i, false);
    }
  }
  return taken;
}

void keep_instance_fields(const GraphData::Pin &old, GraphData::Pin &fresh)
{
  fresh.comment = old.comment;
  fresh.customTextureName = old.customTextureName;
  fresh.data = old.data;
}

// The instance with fresh_pins in their order: per-instance fields kept, incident edges remapped, the edges of pins with no
// match dropped. Returns the fresh index each old pin took, or -1.
eastl::vector<int> rebuild_instance(const GraphData &gd, const GraphData::Node &node, eastl::vector<GraphData::Pin> fresh_pins,
  BoundaryMatch boundary, GraphData::Node &out_node, eastl::vector<GraphData::Edge> &out_incident_edges,
  eastl::vector<DroppedInstanceEdge> &out_dropped)
{
  const eastl::vector<int> freshIndexOf = match_instance_pins(node.pins, fresh_pins, boundary);
  for (int i = 0; i < static_cast<int>(node.pins.size()); ++i)
  {
    if (freshIndexOf[i] >= 0)
    {
      keep_instance_fields(node.pins[i], fresh_pins[freshIndexOf[i]]);
    }
  }

  auto freshPin = [&](int pin_index) {
    return pin_index >= 0 && pin_index < static_cast<int>(freshIndexOf.size()) ? freshIndexOf[pin_index] : -1;
  };
  for (const GraphData::Edge &e : gd.edges)
  {
    const bool atA = e.elemA == node.id;
    const bool atB = e.elemB == node.id;
    if (!atA && !atB)
    {
      continue;
    }
    GraphData::Edge moved = e;
    moved.pinA = atA ? freshPin(e.pinA) : e.pinA;
    moved.pinB = atB ? freshPin(e.pinB) : e.pinB;
    if (moved.pinA < 0 || moved.pinB < 0)
    {
      const int lostPin = atA && moved.pinA < 0 ? e.pinA : e.pinB;
      const bool known = lostPin >= 0 && lostPin < static_cast<int>(node.pins.size());
      out_dropped.push_back({e.id, known ? node.pins[lostPin].name : eastl::string("?")});
      continue;
    }
    out_incident_edges.push_back(moved);
  }
  out_node = node;
  out_node.pins = eastl::move(fresh_pins);
  return freshIndexOf;
}
} // namespace

const char *infer_prop_type(const DataBlock *prop_desc)
{
  if (!prop_desc)
  {
    return "";
  }
  const char *t = prop_desc->getStr("type", "");
  if (t[0])
  {
    return t;
  }
  const int idx = prop_desc->findParam("val");
  if (idx < 0)
  {
    return "";
  }
  // Inverse of the converter's type-tag mapping. val:t -> "string" because :t
  // is BLK's string tag, so a val:t with no explicit type:t reads as plain
  // string. combobox / curve / gradient also use val:t but always carry an
  // explicit type:t for disambiguation, so they don't reach this default.
  switch (prop_desc->getParamType(idx))
  {
    case DataBlock::TYPE_BOOL: return "bool";
    case DataBlock::TYPE_INT: return "int";
    case DataBlock::TYPE_REAL: return "float";
    case DataBlock::TYPE_POINT4: return "color";
    case DataBlock::TYPE_STRING: return "string";
    default: return "";
  }
}

bool edge_opposite_end(const GraphData::Edge &edge, int node_id, int pin_index, int &out_node, int &out_pin)
{
  if (edge.elemA == node_id && edge.pinA == pin_index)
  {
    out_node = edge.elemB;
    out_pin = edge.pinB;
    return true;
  }
  if (edge.elemB == node_id && edge.pinB == pin_index)
  {
    out_node = edge.elemA;
    out_pin = edge.pinA;
    return true;
  }
  return false;
}

bool edge_source_pin(const GraphData &gd, const GraphData::Edge &edge, int &out_node, int &out_pin)
{
  const int endNodes[2] = {edge.elemA, edge.elemB};
  const int endPins[2] = {edge.pinA, edge.pinB};
  for (int i = 0; i < 2; ++i)
  {
    const GraphData::Pin *const pin = find_pin(gd, endNodes[i], endPins[i]);
    if (pin && pin->role == PinRole::Out)
    {
      out_node = endNodes[i];
      out_pin = endPins[i];
      return true;
    }
  }
  return false;
}

bool is_pin_reachable(const GraphData::Node &node, int pin_index)
{
  return pin_index >= 0 && pin_index < static_cast<int>(node.pins.size()) && !node.pins[pin_index].hidden &&
         !node.pins[pin_index].separator;
}

bool is_pin_reachable(const GraphData &gd, int node_id, int pin_index)
{
  const GraphData::Node *const n = find_node_by_id(gd, node_id);
  return n && is_pin_reachable(*n, pin_index);
}

const char *node_display_name(const GraphData &gd, int node_id)
{
  const GraphData::Node *const n = find_node_by_id(gd, node_id);
  if (!n || n->descName.empty())
  {
    return UNNAMED_NODE;
  }
  return n->descName.c_str();
}

const char *pin_comment(const GraphData &gd, int node_id, int pin_index)
{
  const GraphData::Node *const n = find_node_by_id(gd, node_id);
  if (!n || !is_pin_reachable(*n, pin_index))
  {
    return "";
  }
  return n->pins[pin_index].comment.c_str();
}

namespace
{
// One edge seen from pin (node_id, pin_index): false when the edge does not touch that pin, and `out`
// is then untouched. The one place the three answers about an edge are composed -- where it leads,
// whether that node is still there, and whether the view can go to it.
bool resolve_pin_edge(const GraphData &gd, const GraphData::Edge &edge, int node_id, int pin_index, PinEdge &out)
{
  int oppositeNode = -1;
  int oppositePin = -1;
  if (!edge_opposite_end(edge, node_id, pin_index, oppositeNode, oppositePin))
  {
    return false;
  }

  const GraphData::Node *const destination = find_node_by_id(gd, oppositeNode);
  out.edgeId = edge.id;
  out.oppositeNode = oppositeNode;
  out.oppositePin = oppositePin;
  out.muted = edge.muted;
  out.oppositeNodeExists = destination != nullptr;
  out.oppositePinReachable = destination && is_pin_reachable(*destination, oppositePin);
  return true;
}
} // namespace

void collect_pin_edges(const GraphData &gd, int node_id, int pin_index, eastl::vector<PinEdge> &out)
{
  out.clear();
  if (node_id < 0 || pin_index < 0)
  {
    return;
  }

  for (const GraphData::Edge &e : gd.edges)
  {
    PinEdge pinEdge;
    if (resolve_pin_edge(gd, e, node_id, pin_index, pinEdge))
    {
      out.push_back(pinEdge);
    }
  }
}

bool has_reachable_destination(const GraphData &gd, int node_id, int pin_index)
{
  if (node_id < 0 || pin_index < 0)
  {
    return false;
  }

  for (const GraphData::Edge &e : gd.edges)
  {
    // The PinEdge is a stack value, so the per-frame caller still allocates nothing.
    PinEdge pinEdge;
    if (resolve_pin_edge(gd, e, node_id, pin_index, pinEdge) && pinEdge.oppositePinReachable)
    {
      return true;
    }
  }
  return false;
}

void resolve_node_pins(GraphData::Node &node, const DataBlock *base_nodes_blk)
{
  // Find the matching `node{}` block in the registry by templateUid. The loaders own
  // the legacy descName fallback (and populate node.templateUid before calling us); by
  // the time we run here templateUid is either set or the descriptor is genuinely
  // unreachable, and we should NOT silently re-resolve by name -- that would mask
  // template-rename divergence the uid scheme is meant to prevent. Linear scan is
  // fine: this runs at load / insert, not per frame, and the registry is bounded (~100).
  const DataBlock *const desc = find_node_desc(base_nodes_blk, node.templateUid.c_str());

  const bool isShaderEditor = (node.plugin == "shader_editor");

  if (!desc)
  {
    // Descriptor missing from base_nodes.blk -- happens for shader_editor-generated nodes
    // (mask_from_index, multi_max, etc.) which carry their pin shape only in the per-instance
    // saved data. The loader has already populated role / types / type / isInput from it and left
    // singleConnect multi-connect; we just need to mark pins visible and apply the shader_editor
    // visibility rule so output pins that aren't textures/particles stay hidden, matching the JS
    // editor's rendering.
    for (GraphData::Pin &p : node.pins)
    {
      p.hidden = false;
      if (isShaderEditor && !p.isInput && !is_texture_or_particles(p.type))
      {
        p.hidden = true;
      }
    }
    node.allowLoop = false;
    return;
  }

  // Descriptor found -- it's the source of truth. Re-derive each pin from the block of its name and saved role, overriding
  // what the loader read from the saved graph; reset a pin the descriptor lacks.
  for (GraphData::Pin &p : node.pins)
  {
    if (const DataBlock *const descPin = find_pin_desc(desc, p.name.c_str(), p.role))
    {
      resolve_pin(p, *descPin, isShaderEditor);
      continue;
    }
    p.type = PinType::Unknown;
    p.types.clear();
    p.typeGroup.clear();
    p.role = PinRole::In;
    p.isInput = true;
    p.singleConnect = false;
    p.hidden = true;
    p.separator = false;
    p.boundaryId = -1;
  }
  node.allowLoop = desc->getBool("allowLoop", false);
}

const DataBlock *find_pin_desc(const DataBlock *node_desc, const char *pin_name, PinRole role)
{
  if (!node_desc || !pin_name)
  {
    return nullptr;
  }
  for (uint32_t i = 0; i < node_desc->blockCount(); ++i)
  {
    const DataBlock *b = node_desc->getBlock(i);
    if (strcmp(b->getBlockName(), "pin") == 0 && strcmp(b->getStr("name", ""), pin_name) == 0 &&
        parse_pin_role(b->getStr("role", "in")) == role)
    {
      return b;
    }
  }
  return nullptr;
}

void descriptor_pins(const DataBlock &desc, eastl::vector<GraphData::Pin> &out)
{
  out.clear();
  const bool isShaderEditor = strcmp(desc.getStr("plugin", ""), "shader_editor") == 0;
  for (uint32_t i = 0; i < desc.blockCount(); ++i)
  {
    const DataBlock *child = desc.getBlock(i);
    if (strcmp(child->getBlockName(), "pin") != 0)
    {
      continue;
    }
    GraphData::Pin &pin = out.push_back();
    pin.name = child->getStr("name", "");
    resolve_pin(pin, *child, isShaderEditor);
  }
}

void make_node_from_descriptor(const DataBlock &desc, float x, float y, GraphData::Node &out)
{
  out = GraphData::Node{};
  out.x = x;
  out.y = y;
  // descName is a snapshot; the loader refreshes it from the descriptor by templateUid.
  out.templateUid = desc.getStr("templateUid", "");
  out.descName = desc.getStr("name", "");
  out.plugin = desc.getStr("plugin", "");

  // Descriptor order is for a fresh node only; the loader keeps the saved pin order because edges hold pin indices.
  descriptor_pins(desc, out.pins);
  out.allowLoop = desc.getBool("allowLoop", false);

  // A descriptor val is typed (val:i / val:r / val:b / val:p4 / val:t); propertyValues holds strings.
  for (uint32_t i = 0; i < desc.blockCount(); ++i)
  {
    const DataBlock *child = desc.getBlock(i);
    if (strcmp(child->getBlockName(), "property") != 0)
    {
      continue;
    }
    eastl::string name = child->getStr("name", "");
    if (name.empty())
    {
      continue;
    }
    out.propertyValues.emplace_back(eastl::move(name), param_as_string(child, "val"));
  }
}

bool parse_prefixed_int(const char *value, char prefix, int &out_n)
{
  if (!value || value[0] != prefix)
  {
    return false;
  }
  const char *p = value + 1;
  while (*p == ' ' || *p == '\t')
  {
    ++p;
  }
  if (*p == 0)
  {
    return false;
  }
  char *end = nullptr;
  const long n = strtol(p, &end, 10);
  if (end == p)
  {
    return false;
  }
  out_n = static_cast<int>(n);
  return true;
}

int parse_graph_size(const char *s, int fallback)
{
  if (!s || !*s)
  {
    return fallback;
  }
  const char *p = s;
  while (*p && !((*p >= '0' && *p <= '9') || *p == '-'))
  {
    ++p;
  }
  if (!*p)
  {
    return fallback;
  }
  return atoi(p);
}

bool erase_node_and_incident_edges(GraphData &gd, int node_id)
{
  bool erased = false;
  auto edgeEnd = eastl::remove_if(gd.edges.begin(), gd.edges.end(),
    [node_id](const GraphData::Edge &e) { return e.elemA == node_id || e.elemB == node_id; });
  if (edgeEnd != gd.edges.end())
  {
    gd.edges.erase(edgeEnd, gd.edges.end());
    erased = true;
  }
  auto nodeIt = eastl::find_if(gd.nodes.begin(), gd.nodes.end(), [node_id](const GraphData::Node &n) { return n.id == node_id; });
  if (nodeIt != gd.nodes.end())
  {
    gd.nodes.erase(nodeIt);
    erased = true;
  }
  return erased;
}

bool erase_edge(GraphData &gd, int edge_id)
{
  auto it = eastl::find_if(gd.edges.begin(), gd.edges.end(), [edge_id](const GraphData::Edge &e) { return e.id == edge_id; });
  if (it == gd.edges.end())
  {
    return false;
  }
  gd.edges.erase(it);
  return true;
}

void clear_graph_data(GraphData &out)
{
  out.nodes.clear();
  out.edges.clear();
  out.mainGraphBlk.reset();
  out.shaderListBlk.reset();
  out.mainGraphStamp = 0;
  out.renderDir.clear();
  out.entityDir.clear();
  out.heightmapScale = UNSET_HEIGHT;
  out.heightmapMin = UNSET_HEIGHT;
  out.heightmapCellSize = UNSET_HEIGHT;
  out.graphTextureWidth = 1024;
  out.graphTextureHeight = 1024;
  out.graphTextureDepth = 1024;
  out.graphTextureType = "R16F";
  out.graphTextureWrap = "wrap";
  out.sourcePath.clear();
}

size_t approx_node_size(const GraphData::Node &node)
{
  return sizeof(GraphData::Node) + node.pins.size() * sizeof(GraphData::Pin) +
         node.propertyValues.size() * sizeof(eastl::pair<eastl::string, eastl::string>);
}

bool validate_subgraph_schema(const GraphData &gd, eastl::vector<SubgraphSchemaError> &out_errors)
{
  // Snapshot the starting error count so the return value reflects whether THIS call
  // added anything -- callers can pre-populate `out_errors` with their own context and
  // still get a meaningful boolean back.
  const size_t startSize = out_errors.size();

  struct Seen
  {
    eastl::string name;
    bool isInput;
    int nodeId;
  };
  eastl::vector<Seen> boundaries;
  eastl::vector<Seen> separators;

  for (const GraphData::Node &n : gd.nodes)
  {
    if (is_separator_boundary(n))
    {
      eastl::string label = effective_subgraph_boundary_name(gd, n.id);
      if (!label.empty())
      {
        separators.push_back(Seen{eastl::move(label), false, n.id});
      }
      continue;
    }
    const bool isIn = is_input_boundary(n);
    const bool isOut = is_output_boundary(n);
    if (!isIn && !isOut)
    {
      continue;
    }

    // Effective name = `name` property if set, else the name of the pin on the other end
    // of the boundary's first edge. An unnamed boundary connected to (say) a `shape_height`
    // consumer still has a usable interface name -- so we only flag EMPTY_INTERFACE_NAME
    // when the boundary is BOTH unnamed AND unconnected.
    eastl::string ifaceName = effective_subgraph_boundary_name(gd, n.id);
    if (ifaceName.empty())
    {
      out_errors.push_back({SubgraphSchemaError::EMPTY_INTERFACE_NAME, n.id, {}, isIn});
      continue;
    }

    boundaries.push_back(Seen{ifaceName, isIn, n.id});
  }

  if (boundaries.empty())
  {
    // Could either be a graph with no boundary nodes at all (which is the typical
    // "user hit Save as subgraph by accident" case) OR a graph where every boundary
    // had an empty name (already reported above). In the latter case we still want
    // the user to see NO_BOUNDARIES is implied -- but only if no EMPTY_INTERFACE_NAME
    // was added in this call (otherwise it's redundant noise).
    if (out_errors.size() == startSize)
    {
      out_errors.push_back({SubgraphSchemaError::NO_BOUNDARIES});
    }
  }
  else
  {
    // At least one boundary -- the subgraph IS attempting to be useful. Require at least
    // one `subgraph out` so the parent has something to read; an outputs-zero subgraph is
    // pure side-effect-on-nothing as far as the texgen pipeline cares, since every
    // interior compile result gets discarded at expansion time.
    bool hasOutput = false;
    for (const Seen &b : boundaries)
    {
      if (!b.isInput)
      {
        hasOutput = true;
        break;
      }
    }
    if (!hasOutput)
    {
      out_errors.push_back({SubgraphSchemaError::NO_OUTPUTS});
    }
  }

  // Duplicate-name check: unique across inputs and outputs too, so an instance shows no two pins of one name.
  eastl::hash_set<eastl::string> boundaryNames;
  for (const Seen &b : boundaries)
  {
    if (!boundaryNames.insert(b.name).second)
    {
      out_errors.push_back({SubgraphSchemaError::DUPLICATE_NAME, b.nodeId, b.name, b.isInput});
    }
  }

  // A separator pin block has no role and reads as an input, so a separator named like a boundary can take that pin's place.
  for (const Seen &s : separators)
  {
    for (const Seen &b : boundaries)
    {
      if (s.name == b.name)
      {
        out_errors.push_back({SubgraphSchemaError::SEPARATOR_NAME_CLASH, s.nodeId, s.name});
        break;
      }
    }
  }

  return out_errors.size() == startSize;
}

eastl::string format_subgraph_schema_error(const SubgraphSchemaError &err)
{
  const char *roleStr = err.isInputRole ? "input" : "output";
  switch (err.code)
  {
    case SubgraphSchemaError::NO_BOUNDARIES:
      return eastl::string("graph has no `subgraph in:` / `subgraph out` boundary nodes -- nothing to expose");
    case SubgraphSchemaError::NO_OUTPUTS:
      return eastl::string("graph has no `subgraph out` boundary node -- parent cannot consume anything from this subgraph");
    case SubgraphSchemaError::EMPTY_INTERFACE_NAME:
      return eastl::string(eastl::string::CtorSprintf(),
        "node id=%d: %s boundary has empty `name` property and no incident edges to derive a name from", err.nodeId, roleStr);
    case SubgraphSchemaError::DUPLICATE_NAME:
      return eastl::string(eastl::string::CtorSprintf(),
        "node id=%d: %s boundary named '%s' repeats a boundary name (names must be unique across inputs and outputs)", err.nodeId,
        roleStr, err.interfaceName.c_str());
    case SubgraphSchemaError::SEPARATOR_NAME_CLASH:
      return eastl::string(eastl::string::CtorSprintf(), "node id=%d: separator label '%s' repeats a boundary name", err.nodeId,
        err.interfaceName.c_str());
  }
  return eastl::string("unknown subgraph schema error");
}

void find_dead_input_boundaries(const GraphData &gd, eastl::vector<DeadInputBoundary> &out)
{
  // Build id -> node-index map once; the BFS below does up to N lookups per input boundary
  // and a hash hit beats the linear scan effective_subgraph_boundary_name uses (the helper
  // is fine for one-shot calls but this loop runs O(boundaries * edges) at worst).
  eastl::hash_map<int, int> idToIdx;
  idToIdx.reserve(gd.nodes.size());
  for (int i = 0; i < static_cast<int>(gd.nodes.size()); ++i)
  {
    idToIdx[gd.nodes[i].id] = i;
  }

  for (const GraphData::Node &startNode : gd.nodes)
  {
    if (!is_input_boundary(startNode))
    {
      continue;
    }

    // Forward BFS following data flow direction: an edge contributes a step from src to
    // dst only when the src-side pin is an out-role pin (so the edge actually carries data
    // FROM src TO dst). visited prevents both cycles and re-visiting through a different
    // edge to the same node.
    eastl::hash_set<int> visited;
    eastl::vector<int> stack;
    visited.insert(startNode.id);
    stack.push_back(startNode.id);

    bool reachesOutput = false;
    while (!stack.empty())
    {
      const int curId = stack.back();
      stack.pop_back();
      auto idxIt = idToIdx.find(curId);
      if (idxIt == idToIdx.end())
      {
        continue;
      }
      const GraphData::Node &cur = gd.nodes[idxIt->second];

      // `subgraph out` boundary reached -- this input has a path to an output.
      if (curId != startNode.id && is_output_boundary(cur))
      {
        reachesOutput = true;
        break;
      }

      // Walk outgoing edges (the ones leaving an out-role pin on `cur`).
      for (const GraphData::Edge &e : gd.edges)
      {
        // A muted edge carries no data, so a boundary reachable only through one really is dead.
        if (e.muted)
        {
          continue;
        }
        int nextId = -1;
        if (e.elemA == curId && e.pinA >= 0 && e.pinA < static_cast<int>(cur.pins.size()) && cur.pins[e.pinA].role == PinRole::Out)
        {
          nextId = e.elemB;
        }
        else if (
          e.elemB == curId && e.pinB >= 0 && e.pinB < static_cast<int>(cur.pins.size()) && cur.pins[e.pinB].role == PinRole::Out)
        {
          nextId = e.elemA;
        }
        if (nextId >= 0 && visited.insert(nextId).second)
        {
          stack.push_back(nextId);
        }
      }
    }

    if (reachesOutput)
    {
      continue;
    }

    // This input boundary is dead -- record it for the warning.
    DeadInputBoundary entry;
    entry.nodeId = startNode.id;
    entry.interfaceName = effective_subgraph_boundary_name(gd, startNode.id);
    // descName "subgraph in: <type>" -- slice the type token for the fallback label when
    // the interface name is unavailable (rare; would already have been flagged by the
    // schema validator before this function ran).
    const char *colon = strchr(startNode.descName.c_str(), ':');
    if (colon)
    {
      const char *t = colon + 1;
      while (*t == ' ')
      {
        ++t;
      }
      entry.type = t;
    }
    out.push_back(eastl::move(entry));
  }
}

eastl::string effective_subgraph_boundary_name(const GraphData &gd, int boundary_node_id)
{
  // Locate the boundary node by id. Linear scan -- boundary lookup is cold-path (synthesis,
  // validation, expander pre-pass) and the node count for a subgraph file stays small even
  // for index_coloring_64 (489 nodes).
  const GraphData::Node *const boundary = find_node_by_id(gd, boundary_node_id);
  if (!boundary)
  {
    return {};
  }

  // Step 1: explicit `name` property wins. Most boundaries in the converted subgraphs are
  // authored with a meaningful name (e.g. "indicies input"); only the occasional unnamed
  // float / int parameter falls through to step 2.
  for (const auto &pv : boundary->propertyValues)
  {
    if (pv.first == "name" && !pv.second.empty())
    {
      return pv.second;
    }
  }

  // Step 2: derive from the first edge touching the boundary. Both `subgraph in: TYPE`
  // (out-role pin) and `subgraph out` (in-role pin) boundaries have exactly one pin in
  // base_nodes.blk, so we look at any edge whose endpoint is this node id and pull the
  // "other side" pin's name. If a `subgraph in` fans out to multiple interior consumers
  // we pick the first by edge-vector order -- deterministic across reloads since edges
  // are persisted in stable order, and the consumer names are usually the same anyway
  // (the boundary connects to one named param on the downstream node).
  //
  // Muted edges deliberately still count here. This name is the splice key that
  // expand_subgraphs matches parent edges against, so letting a mute change it would silently
  // unwire the parent instead of just starving it.
  for (const GraphData::Edge &e : gd.edges)
  {
    int otherNodeId = -1;
    int otherPinIdx = -1;
    if (e.elemA == boundary_node_id)
    {
      otherNodeId = e.elemB;
      otherPinIdx = e.pinB;
    }
    else if (e.elemB == boundary_node_id)
    {
      otherNodeId = e.elemA;
      otherPinIdx = e.pinA;
    }
    if (otherNodeId < 0)
    {
      continue;
    }
    const GraphData::Node *const other = find_node_by_id(gd, otherNodeId);
    if (other && otherPinIdx >= 0 && otherPinIdx < static_cast<int>(other->pins.size()))
    {
      const eastl::string &name = other->pins[otherPinIdx].name;
      if (!name.empty())
      {
        return name;
      }
    }
  }

  return {};
}

void collect_subgraph_boundaries(const GraphData &gd, bool with_separators, eastl::vector<const GraphData::Node *> &out)
{
  out.clear();
  for (const GraphData::Node &n : gd.nodes)
  {
    if (is_input_boundary(n) || (with_separators && is_separator_boundary(n)))
    {
      out.push_back(&n);
    }
  }
  const size_t inputCount = out.size();
  for (const GraphData::Node &n : gd.nodes)
  {
    if (is_output_boundary(n))
    {
      out.push_back(&n);
    }
  }
  auto byY = [](const GraphData::Node *a, const GraphData::Node *b) { return a->y < b->y; };
  eastl::stable_sort(out.begin(), out.begin() + inputCount, byY);
  eastl::stable_sort(out.begin() + inputCount, out.end(), byY);
}

bool synthesize_subgraph_descriptor(const GraphData &child, const char *stem, const char *source_path, const char *location_label,
  DataBlock &out_node_blk, eastl::vector<SubgraphDescriptorMessage> &out_messages)
{
  // Any schema error keeps the file out of the palette: the compile could not inline it.
  eastl::vector<SubgraphSchemaError> schemaErrors;
  const bool schemaOk = validate_subgraph_schema(child, schemaErrors);
  for (const SubgraphSchemaError &err : schemaErrors)
  {
    out_messages.push_back({err.code != SubgraphSchemaError::NO_BOUNDARIES, format_subgraph_schema_error(err)});
  }
  if (!schemaOk)
  {
    return false;
  }

  // A dead input is only a warning: the other pins still work.
  eastl::vector<DeadInputBoundary> dead;
  find_dead_input_boundaries(child, dead);
  for (const DeadInputBoundary &d : dead)
  {
    const char *label = !d.interfaceName.empty() ? d.interfaceName.c_str() : (!d.type.empty() ? d.type.c_str() : "<unnamed>");
    out_messages.push_back(
      {false, eastl::string(eastl::string::CtorSprintf(), "input '%s' (id=%d) does not propagate to any output", label, d.nodeId)});
  }

  eastl::string templateUid("subgraph::");
  templateUid.append(stem);
  out_node_blk.setStr("templateUid", templateUid.c_str());
  out_node_blk.setStr("name", stem);
  out_node_blk.setStr("category", "Subgraphs");
  out_node_blk.setStr("plugin", SUBGRAPH_PLUGIN); // the expander finds instances by it
  out_node_blk.setBool("generated", true);
  out_node_blk.setInt("width", 200);
  // Display only: the compile finds an instance's file by its stem.
  out_node_blk.setStr("sourcePath", source_path);
  out_node_blk.setStr("location", location_label);

  // The validation above makes every name non-empty and unique across inputs and outputs.
  eastl::vector<const GraphData::Node *> boundaries;
  collect_subgraph_boundaries(child, /*with_separators=*/true, boundaries);
  for (const GraphData::Node *n : boundaries)
  {
    DataBlock *pinBlk = out_node_blk.addNewBlock("pin");
    pinBlk->setStr("name", effective_subgraph_boundary_name(child, n->id).c_str());
    if (is_separator_boundary(*n))
    {
      // A label only: the validator skips it, so it can be unnamed or repeated, and a pin found by name would take another's id.
      pinBlk->setBool("separator", true);
      continue;
    }
    pinBlk->setInt("boundaryId", n->id);
    const bool isInput = is_input_boundary(*n);
    if (isInput)
    {
      // descName is "subgraph in: <type>".
      const char *colon = strchr(n->descName.c_str(), ':');
      const char *t = colon ? colon + 1 : "";
      while (*t == ' ')
      {
        ++t;
      }
      pinBlk->setStr("types", t);
    }
    else
    {
      // Any type the parent can connect: the compile finds the real one from the child's producer.
      pinBlk->setStr("types", "texture2D,particles,bool,int,float,float2,float3,float4");
    }
    pinBlk->setStr("role", isInput ? "in" : "out");
    // The expander takes one parent edge per input, but rewires every consumer of an output.
    pinBlk->setBool("singleConnect", isInput);
  }
  return true;
}

bool same_instance_pins(const eastl::vector<GraphData::Pin> &a, const eastl::vector<GraphData::Pin> &b)
{
  return a.size() == b.size() && eastl::equal(a.begin(), a.end(), b.begin(), [](const GraphData::Pin &x, const GraphData::Pin &y) {
    return same_pin_interface(x, y) && x.boundaryId == y.boundaryId;
  });
}

bool resync_subgraph_instance(const GraphData &gd, int node_id, const DataBlock &base_nodes_blk, GraphData::Node &out_node,
  eastl::vector<GraphData::Edge> &out_incident_edges, eastl::vector<DroppedInstanceEdge> &out_dropped, bool from_live)
{
  out_incident_edges.clear();
  out_dropped.clear();
  const GraphData::Node *const node = find_node_by_id(gd, node_id);
  if (!node || !is_subgraph_instance(*node))
  {
    return false;
  }
  const DataBlock *const desc = find_node_desc(&base_nodes_blk, node->templateUid.c_str());
  if (!desc)
  {
    return false;
  }

  GraphData::Node fresh;
  make_node_from_descriptor(*desc, node->x, node->y, fresh);
  // A boundary id alone counts too: the live sync trusts the ids this leaves.
  bool changed = fresh.pins.size() != node->pins.size();
  for (size_t i = 0; !changed && i < node->pins.size(); ++i)
  {
    const GraphData::Pin &old = node->pins[i];
    changed = !same_pin_interface(old, fresh.pins[i]) || old.boundaryId != fresh.pins[i].boundaryId || (from_live && old.addedLive);
  }
  if (!changed)
  {
    return false;
  }

  // Without from_live, by name only: a file cannot tell a renamed boundary from a new one that reused a deleted one's id.
  rebuild_instance(gd, *node, eastl::move(fresh.pins), from_live ? BoundaryMatch::FROM_FILE : BoundaryMatch::NONE, out_node,
    out_incident_edges, out_dropped);
  return true;
}

bool sync_subgraph_instance(const GraphData &gd, int node_id, const eastl::vector<GraphData::Pin> &interface_pins,
  const eastl::vector<GraphData::Pin> *file_pins, GraphData::Node &out_node, eastl::vector<GraphData::Edge> &out_incident_edges,
  eastl::vector<DroppedInstanceEdge> &out_dropped)
{
  out_incident_edges.clear();
  out_dropped.clear();
  const GraphData::Node *const node = find_node_by_id(gd, node_id);
  if (!node || !is_subgraph_instance(*node))
  {
    return false;
  }
  const eastl::vector<int> freshIndexOf =
    rebuild_instance(gd, *node, interface_pins, BoundaryMatch::ALL, out_node, out_incident_edges, out_dropped);
  // The mark comes off only once the file has the pin, and never comes back on a pin the file had.
  auto inFile = [file_pins](const GraphData::Pin &pin) { return file_pins && has_same_pin(*file_pins, pin); };
  for (GraphData::Pin &pin : out_node.pins)
  {
    pin.addedLive = !inFile(pin);
  }
  for (int i = 0; i < static_cast<int>(node->pins.size()); ++i)
  {
    if (freshIndexOf[i] >= 0)
    {
      const GraphData::Pin &old = node->pins[i];
      out_node.pins[freshIndexOf[i]].addedLive = old.addedLive && !inFile(old);
    }
  }
  return !same_instance_pins(out_node.pins, node->pins);
}

eastl::string graph_file_stem(const char *path, bool *out_is_subgraph)
{
  if (out_is_subgraph)
  {
    *out_is_subgraph = false;
  }
  // The return value skips the directory the buffer still holds, so a full path is fine here.
  char buf[DAGOR_MAX_PATH] = {};
  eastl::string stem(dd_get_fname_without_path_and_ext(buf, sizeof(buf), path));
  constexpr char SUBGRAPH_SUFFIX[] = ".subgraph";
  constexpr size_t SUFFIX_LEN = sizeof(SUBGRAPH_SUFFIX) - 1;
  if (stem.size() > SUFFIX_LEN && stem.compare(stem.size() - SUFFIX_LEN, SUFFIX_LEN, SUBGRAPH_SUFFIX) == 0)
  {
    stem.resize(stem.size() - SUFFIX_LEN);
    if (out_is_subgraph)
    {
      *out_is_subgraph = true;
    }
  }
  return stem;
}

bool save_graph_data_blk(const GraphData &d, const char *blk_path)
{
  const int64_t tStart = ref_time_ticks();

  DataBlock root;

  if (!d.renderDir.empty())
  {
    root.setStr("renderDir", d.renderDir.c_str());
  }
  if (!d.entityDir.empty())
  {
    root.setStr("entityDir", d.entityDir.c_str());
  }

  // Keep the sentinel implicit in the saved file -- consumers default back to it on load.
  if (d.heightmapScale != UNSET_HEIGHT)
  {
    root.setReal("heightmapScale", d.heightmapScale);
  }
  if (d.heightmapMin != UNSET_HEIGHT)
  {
    root.setReal("heightmapMin", d.heightmapMin);
  }
  if (d.heightmapCellSize != UNSET_HEIGHT)
  {
    root.setReal("heightmapCellSize", d.heightmapCellSize);
  }

  // Graph-level texture defaults consumed by compile_graph_to_blks' propagation
  // pass. Always write -- the compile pipeline needs them, and defaults are cheap.
  root.setInt("graphTextureWidth", d.graphTextureWidth);
  root.setInt("graphTextureHeight", d.graphTextureHeight);
  root.setInt("graphTextureDepth", d.graphTextureDepth);
  root.setStr("graphTextureType", d.graphTextureType.c_str());
  root.setStr("graphTextureWrap", d.graphTextureWrap.c_str());

  // mainGraphBlk / shaderListBlk are intentionally NOT persisted. They are
  // compiled output of (nodes + edges + propertyValues + descriptor info)
  // produced on demand by compile_graph_to_blks (graph_compile.cpp). The caller
  // must re-run the compile after load to repopulate them.

  for (const GraphData::Node &n : d.nodes)
  {
    DataBlock *nb = root.addNewBlock("node");
    nb->setInt("id", n.id);
    nb->setReal("x", n.x);
    nb->setReal("y", n.y);
    // templateUid is the lookup key on reload; descName is the readable hint and gets
    // refreshed from the descriptor's current name on the next load. Write templateUid
    // only when non-empty so unmigrated nodes (descriptor missing on load) don't claim
    // a uid they never had.
    if (!n.templateUid.empty())
    {
      nb->setStr("templateUid", n.templateUid.c_str());
    }
    nb->setStr("descName", n.descName.c_str());
    if (!n.plugin.empty())
    {
      nb->setStr("plugin", n.plugin.c_str());
    }
    if (n.descName == "block")
    {
      nb->setReal("blockWidth", n.blockWidth);
      nb->setReal("blockHeight", n.blockHeight);
    }

    for (const GraphData::Pin &p : n.pins)
    {
      DataBlock *pb = nb->addNewBlock("pin");
      pb->setStr("name", p.name.c_str());
      pb->setStr("role", pin_role_to_string(p.role));

      // CSV types list mirrors base_nodes.blk's `types:t = "float,texture2D"` convention,
      // so `parse_pin_types_csv` on the load side reads it back without a new code path.
      // Skip PinType::Unknown entries (helper returns "") so we never emit a stray comma.
      eastl::string typesCsv;
      for (PinType t : p.types)
      {
        const char *s = pin_type_to_string(t);
        if (!s[0])
        {
          continue;
        }
        if (!typesCsv.empty())
        {
          typesCsv.push_back(',');
        }
        typesCsv.append(s);
      }
      if (!typesCsv.empty())
      {
        pb->setStr("types", typesCsv.c_str());
      }
      if (!p.comment.empty())
      {
        pb->setStr("comment", p.comment.c_str());
      }
      // Per-instance pin data, only present for shader_editor sub-graph nodes
      // and other pins whose source-of-truth data isn't in base_nodes.blk.
      if (p.data.paramCount() > 0 || p.data.blockCount() > 0)
      {
        pb->addNewBlock(&p.data, "data");
      }
    }

    // Elem-level property values are per-instance current values; the
    // descriptor (resolved via templateUid + property name at consumption
    // time) carries the type. Keeping val:t (string) here makes save/load
    // descriptor-independent -- shader-editor base nodes (texValue, lerp, ...)
    // and other elems without a matching descriptor would otherwise silently
    // lose type information on a typed save.
    for (const eastl::pair<eastl::string, eastl::string> &pv : n.propertyValues)
    {
      DataBlock *qb = nb->addNewBlock("property");
      qb->setStr("name", pv.first.c_str());
      qb->setStr("val", pv.second.c_str());
    }
  }

  for (const GraphData::Edge &e : d.edges)
  {
    DataBlock *eb = root.addNewBlock("edge");
    eb->setInt("id", e.id);
    eb->setInt("elemA", e.elemA);
    eb->setInt("pinA", e.pinA);
    eb->setInt("elemB", e.elemB);
    eb->setInt("pinB", e.pinB);
    if (e.muted)
    {
      eb->setBool("muted", true);
    }
  }

  if (!root.saveToTextFile(blk_path))
  {
    debug("save_graph_data_blk: failed to write '%s'", blk_path);
    return false;
  }

  const int saveMs = get_time_usec(tStart) / 1000;
  debug("save_graph_data_blk: wrote '%s' in %d ms (%d nodes, %d edges)", blk_path, saveMs, (int)d.nodes.size(), (int)d.edges.size());
  return true;
}

bool load_graph_data_blk(GraphData &out, const char *blk_path, const char * /*shader_includes_dir*/, const DataBlock *base_nodes_blk)
{
  clear_graph_data(out);

  const int64_t tStart = ref_time_ticks();

  DataBlock root;
  if (!root.load(blk_path))
  {
    debug("load_graph_data_blk: failed to read '%s'", blk_path);
    return false;
  }

  out.renderDir = root.getStr("renderDir", "");
  out.entityDir = root.getStr("entityDir", "");
  out.heightmapScale = root.getReal("heightmapScale", UNSET_HEIGHT);
  out.heightmapMin = root.getReal("heightmapMin", UNSET_HEIGHT);
  out.heightmapCellSize = root.getReal("heightmapCellSize", UNSET_HEIGHT);
  out.graphTextureWidth = root.getInt("graphTextureWidth", 1024);
  out.graphTextureHeight = root.getInt("graphTextureHeight", 1024);
  out.graphTextureDepth = root.getInt("graphTextureDepth", 1024);
  out.graphTextureType = root.getStr("graphTextureType", "R16F");
  out.graphTextureWrap = root.getStr("graphTextureWrap", "wrap");

  // mainGraphBlk / shaderListBlk are intentionally NOT read here. They are
  // compiled output (see save_graph_data_blk); the caller must invoke
  // compile_graph_to_blks after this loader returns to repopulate them.
  // clear_graph_data already reset them at the top of the function.

  for (uint32_t i = 0; i < root.blockCount(); ++i)
  {
    const DataBlock *b = root.getBlock(i);
    const char *bn = b->getBlockName();

    if (strcmp(bn, "node") == 0)
    {
      GraphData::Node n;
      n.id = b->getInt("id", 0);
      n.x = b->getReal("x", 0.0f);
      n.y = b->getReal("y", 0.0f);
      n.templateUid = b->getStr("templateUid", "");
      n.descName = b->getStr("descName", "");
      n.plugin = b->getStr("plugin", "");
      n.blockWidth = b->getReal("blockWidth", n.blockWidth);
      n.blockHeight = b->getReal("blockHeight", n.blockHeight);

      for (uint32_t j = 0; j < b->blockCount(); ++j)
      {
        const DataBlock *c = b->getBlock(j);
        const char *cn = c->getBlockName();
        if (strcmp(cn, "pin") == 0)
        {
          GraphData::Pin pin;
          pin.name = c->getStr("name", "");
          pin.comment = c->getStr("comment", "");
          pin.role = parse_pin_role(c->getStr("role", "in"));
          pin.isInput = (pin.role != PinRole::Out);
          texgen::parse_pin_types_csv(c->getStr("types", ""), pin.types);
          pin.type = pin.types.empty() ? PinType::Unknown : pin.types.front();
          // Final for shader_editor sub-graph nodes, which have no descriptor to override it: their
          // outputs feed many consumers in saved graphs, so single-connect would evict real wiring.
          pin.singleConnect = false; // -V1048
          if (const DataBlock *pinData = c->getBlockByName("data"))
          {
            pin.data.setFrom(pinData);
          }
          n.pins.push_back(pin);
        }
        else if (strcmp(cn, "property") == 0)
        {
          eastl::string name = c->getStr("name", "");
          if (name.empty())
          {
            continue;
          }
          eastl::string val = c->getStr("val", "");
          n.propertyValues.emplace_back(eastl::move(name), eastl::move(val));
        }
      }

      // Reconcile templateUid / descName against the descriptor registry before pin
      // resolution. Legacy BLK files (no templateUid) get auto-migrated here; recent
      // files get descName refreshed from the descriptor's current `name:t`.
      reconcile_template_uid(n, base_nodes_blk);
      // One-shot descriptor-cache resolve.
      // Pins whose name and role aren't in the descriptor stay hidden = true.
      resolve_node_pins(n, base_nodes_blk);
      out.nodes.push_back(eastl::move(n));
    }
    else if (strcmp(bn, "edge") == 0)
    {
      GraphData::Edge ed;
      ed.id = b->getInt("id", 0);
      ed.elemA = b->getInt("elemA", 0);
      ed.pinA = b->getInt("pinA", 0);
      ed.elemB = b->getInt("elemB", 0);
      ed.pinB = b->getInt("pinB", 0);
      ed.muted = b->getBool("muted", false);
      out.edges.push_back(ed);
    }
  }

  out.sourcePath = blk_path;

  const int loadMs = get_time_usec(tStart) / 1000;
  debug("load_graph_data_blk: loaded '%s' in %d ms (%d nodes, %d edges)", blk_path, loadMs, (int)out.nodes.size(),
    (int)out.edges.size());
  return true;
}
