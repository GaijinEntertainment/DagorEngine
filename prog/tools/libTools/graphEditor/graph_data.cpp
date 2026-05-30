// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <graphEditor/graph_data.h>

#include <debug/dag_debug.h>
#include <math/dag_Point4.h>
#include <osApiWrappers/dag_files.h>
#include <perfMon/dag_cpuFreq.h>
#include <util/dag_string.h>

#include <rapidjson/document.h>

#include <EASTL/hash_map.h>
#include <EASTL/hash_set.h>

#include <cstdlib>
#include <cstring>
#include <float.h>
#include <format>

namespace
{
String read_file_to_string(const char *file_name)
{
  file_ptr_t h = df_open(file_name, DF_READ | DF_IGNORE_MISSING);
  if (!h)
  {
    return String("");
  }

  const int len = df_length(h);
  String res;
  res.resize(len + 1);
  if (df_read(h, &res[0], len) < 0)
  {
    df_close(h);
    return String("");
  }
  res.back() = '\0';
  df_close(h);
  return res;
}

// Pin shape (type/role/hidden) is derived from the descriptor at render time, not stored on
// the loaded Node. Both rules that previously lived here -- the shader_editor "non-texture
// outputs hidden" rule and the per-(desc,pin) flat hidden table mirroring mainNodes.js's
// `hidden:true` flags -- now live on the descriptor side: base_nodes.blk encodes
// `pin{ hidden:b = yes }` directly, and the shader_editor visibility rule is evaluated by
// graph_panel.cpp against the descriptor's `types:t` / `role:t`.

// Stringify a JSON value for storage in Node::propertyValues. The descriptor's declared
// property `type` drives parsing at consumption time -- here we just preserve the value
// in a canonical text form so the consumer can re-interpret it.
String json_value_to_string(const rapidjson::Value &v)
{
  if (v.IsString())
  {
    return String(v.GetString());
  }
  if (v.IsBool())
  {
    return String(v.GetBool() ? "true" : "false");
  }
  if (v.IsNumber())
  {
    String s;
    s.printf(64, "%g", v.GetDouble());
    return s;
  }
  return String("");
}

// Extract content between two markers from a graph JSON file. The markers live inside JSON string
// values (the "code" field), so the captured slice is JSON-string-escaped -- undo \\, \n and \"
// the same way webui::GraphEditor::getSubstring did.
String extract_marker_section(const String &text, const char *begin_marker, const char *end_marker)
{
  const char *begin = strstr(text.str(), begin_marker);
  if (!begin)
  {
    return String("");
  }
  const char *end = strstr(begin, end_marker);
  if (!end)
  {
    return String("");
  }

  String res(tmpmem);
  res.setSubStr(begin + strlen(begin_marker), end);
  res.replaceAll("\\\\", "\x01");
  res.replaceAll("\\n", "\n");
  res.replaceAll("\\\"", "\"");
  res.replaceAll("\x01", "\\");
  return res;
}

// Walk the comma-delimited pin `types:t` value and append each token's PinType to `out`. Skips
// whitespace around tokens; unknown tokens map to PinType::Unknown (still appended so the slot
// position matches the JS editor's interpretation).
void parse_pin_types_csv(const char *types_csv, eastl::fixed_vector<PinType, 4, true> &out)
{
  out.clear();
  if (!types_csv || !types_csv[0])
  {
    return;
  }
  const char *p = types_csv;
  while (*p)
  {
    while (*p == ' ' || *p == '\t')
    {
      ++p;
    }
    const char *e = p;
    while (*e && *e != ',')
    {
      ++e;
    }
    const char *trimEnd = e;
    while (trimEnd > p && (trimEnd[-1] == ' ' || trimEnd[-1] == '\t'))
    {
      --trimEnd;
    }
    char buf[32];
    size_t n = eastl::min<size_t>(trimEnd - p, sizeof(buf) - 1);
    memcpy(buf, p, n);
    buf[n] = 0;
    out.push_back(parse_pin_type(buf));
    if (!*e)
    {
      break;
    }
    p = e + 1;
  }
}

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

// Inverse of parse_pin_type. Returns "" for PinType::Unknown so callers can
// skip emitting empty entries when building the CSV types list.
const char *pin_type_to_string(PinType t)
{
  switch (t)
  {
    case PinType::Unknown: return "";
    case PinType::Bool: return "bool";
    case PinType::Int: return "int";
    case PinType::Uint: return "uint";
    case PinType::Float: return "float";
    case PinType::Float2: return "float2";
    case PinType::Float3: return "float3";
    case PinType::Float4: return "float4";
    case PinType::Texture1D: return "texture1D";
    case PinType::Texture2D: return "texture2D";
    case PinType::Texture3D: return "texture3D";
    case PinType::Texture2DArray: return "texture2DArray";
    case PinType::Texture2DShdArray: return "texture2D_shdArray";
    case PinType::Particles: return "particles";
    case PinType::BiomeData: return "BiomeData";
    case PinType::NBSGbuffer: return "NBSGbuffer";
    case PinType::MaterialT: return "material_t";
    case PinType::LayerT: return "Layer_t";
    case PinType::MaskT: return "mask_t";
    case PinType::CtrlT: return "ctrl_t";
  }
  return "";
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
    for (uint32_t i = 0; i < base_nodes_blk->blockCount(); ++i)
    {
      const DataBlock *b = base_nodes_blk->getBlock(i);
      if (strcmp(b->getBlockName(), "node") != 0)
      {
        continue;
      }
      if (strcmp(b->getStr("templateUid", ""), node.templateUid.c_str()) == 0)
      {
        node.descName = b->getStr("name", "");
        return;
      }
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
  // pin shape inside the JSON instance and have no base_nodes.blk descriptor by
  // design -- silence the miss for those; resolve_node_pins handles them via the
  // "descriptor missing" branch.
  if (!node.plugin.empty())
  {
    return;
  }
  debug("graphEditor: node id=%d descName '%s' not found in base_nodes.blk (no templateUid, no name match)", node.id,
    node.descName.c_str());
}

// Item lists mirror mainNodes.js:2982-2999. BASE_SIZES/TYPES/WRAPS live in graph_data.h
// because PropertiesPanel reuses them verbatim for the graph-level "<graph properties>"
// view. The SIZE/TYPE/WRAP_PREFIXES are the relative-mode items prepended for per-node
// descriptors only; they have no meaning at the graph level so they stay private here.
// Order matters: the val string we write below is the items[0] of the non-graph branch,
// and combobox selection in PropertiesPanel matches by string equality.
constexpr const char *SIZE_PREFIXES[] = {
  "parent size", "graph size", "x 2", "x 4", "x 8", "x 16", "x 32", "/ 2", "/ 4", "/ 8", "/ 16", "/ 32"};
constexpr const char *TYPE_PREFIXES[] = {"parent type", "graph type"};
constexpr const char *WRAP_PREFIXES[] = {"parent wrap", "graph wrap"};

// Read the first comma-separated type token out of a pin's `types:t` value. Returns empty
// when the field is missing/empty.
eastl::string first_type_token(const char *types_csv)
{
  if (!types_csv || !*types_csv)
  {
    return {};
  }
  const char *comma = strchr(types_csv, ',');
  return comma ? eastl::string(types_csv, size_t(comma - types_csv)) : eastl::string(types_csv);
}

void push_size_items(DataBlock &items, bool include_width)
{
  if (include_width)
  {
    items.addStr("item", "width");
  }
  for (const char *p : SIZE_PREFIXES)
  {
    items.addStr("item", p);
  }
  for (const char *s : BASE_SIZES)
  {
    items.addStr("item", s);
  }
}

void push_type_items(DataBlock &items)
{
  for (const char *p : TYPE_PREFIXES)
  {
    items.addStr("item", p);
  }
  for (const char *t : BASE_TYPES)
  {
    items.addStr("item", t);
  }
}

void push_wrap_items(DataBlock &items)
{
  for (const char *p : WRAP_PREFIXES)
  {
    items.addStr("item", p);
  }
  for (const char *w : BASE_WRAPS)
  {
    items.addStr("item", w);
  }
}
} // namespace

PinType parse_pin_type(const char *type_name)
{
  if (!type_name || !type_name[0])
  {
    return PinType::Unknown;
  }

  if (strcmp(type_name, "bool") == 0)
  {
    return PinType::Bool;
  }
  if (strcmp(type_name, "int") == 0)
  {
    return PinType::Int;
  }
  if (strcmp(type_name, "uint") == 0)
  {
    return PinType::Uint;
  }
  if (strcmp(type_name, "float") == 0)
  {
    return PinType::Float;
  }
  if (strcmp(type_name, "float2") == 0)
  {
    return PinType::Float2;
  }
  if (strcmp(type_name, "float3") == 0)
  {
    return PinType::Float3;
  }
  if (strcmp(type_name, "float4") == 0)
  {
    return PinType::Float4;
  }
  if (strcmp(type_name, "texture1D") == 0)
  {
    return PinType::Texture1D;
  }
  if (strcmp(type_name, "texture2D") == 0)
  {
    return PinType::Texture2D;
  }
  if (strcmp(type_name, "texture3D") == 0)
  {
    return PinType::Texture3D;
  }
  if (strcmp(type_name, "texture2DArray") == 0)
  {
    return PinType::Texture2DArray;
  }
  if (strcmp(type_name, "texture2D_shdArray") == 0)
  {
    return PinType::Texture2DShdArray;
  }
  if (strcmp(type_name, "particles") == 0)
  {
    return PinType::Particles;
  }
  if (strcmp(type_name, "BiomeData") == 0)
  {
    return PinType::BiomeData;
  }
  if (strcmp(type_name, "NBSGbuffer") == 0)
  {
    return PinType::NBSGbuffer;
  }
  if (strcmp(type_name, "material_t") == 0)
  {
    return PinType::MaterialT;
  }
  if (strcmp(type_name, "Layer_t") == 0)
  {
    return PinType::LayerT;
  }
  if (strcmp(type_name, "mask_t") == 0)
  {
    return PinType::MaskT;
  }
  if (strcmp(type_name, "ctrl_t") == 0)
  {
    return PinType::CtrlT;
  }

  return PinType::Unknown;
}

bool is_texture_or_particles(PinType t)
{
  return t == PinType::Texture1D || t == PinType::Texture2D || t == PinType::Texture3D || t == PinType::Particles;
}

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

eastl::string param_as_string(const DataBlock *blk, const char *name)
{
  if (!blk || !name)
  {
    return {};
  }
  const int idx = blk->findParam(name);
  if (idx < 0)
  {
    return {};
  }
  char buf[128] = {};
  switch (blk->getParamType(idx))
  {
    case DataBlock::TYPE_STRING: return eastl::string(blk->getStr(idx));
    case DataBlock::TYPE_INT: snprintf(buf, sizeof(buf), "%d", blk->getInt(idx)); return buf;
    case DataBlock::TYPE_REAL: snprintf(buf, sizeof(buf), "%g", blk->getReal(idx)); return buf;
    case DataBlock::TYPE_BOOL: return blk->getBool(idx) ? eastl::string("true") : eastl::string("false");
    case DataBlock::TYPE_POINT4:
    {
      const Point4 p = blk->getPoint4(idx);
      snprintf(buf, sizeof(buf), "%g,%g,%g,%g", p.x, p.y, p.z, p.w);
      return buf;
    }
    default: return {};
  }
}

void resolve_node_pins(GraphData::Node &node, const DataBlock *base_nodes_blk)
{
  // Find the matching `node{}` block in the registry by templateUid. The loaders own
  // the legacy descName fallback (and populate node.templateUid before calling us); by
  // the time we run here templateUid is either set or the descriptor is genuinely
  // unreachable, and we should NOT silently re-resolve by name -- that would mask
  // template-rename divergence the uid scheme is meant to prevent. Linear scan is
  // fine: this runs at load / insert, not per frame, and the registry is bounded (~100).
  const DataBlock *desc = nullptr;
  if (base_nodes_blk && !node.templateUid.empty())
  {
    for (uint32_t i = 0; i < base_nodes_blk->blockCount(); ++i)
    {
      const DataBlock *b = base_nodes_blk->getBlock(i);
      if (strcmp(b->getBlockName(), "node") != 0)
      {
        continue;
      }
      if (strcmp(b->getStr("templateUid", ""), node.templateUid.c_str()) == 0)
      {
        desc = b;
        break;
      }
    }
  }

  const bool isShaderEditor = (node.plugin == "shader_editor");

  if (!desc)
  {
    // Descriptor missing from base_nodes.blk -- happens for shader_editor-generated nodes
    // (mask_from_index, multi_max, etc.) which carry their pin shape only in per-instance
    // JSON. The loader has already populated role / types / type / isInput / singleConnect
    // from JSON; we just need to mark pins visible and apply the shader_editor visibility
    // rule so output pins that aren't textures/particles stay hidden, matching the JS
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

  // Descriptor found -- it's the source of truth. Reset pin metadata (overriding what the
  // loader read out of JSON) and re-derive from the descriptor.
  for (GraphData::Pin &p : node.pins)
  {
    p.type = PinType::Unknown;
    p.types.clear();
    p.typeGroup.clear();
    p.role = PinRole::In;
    p.isInput = true;
    p.singleConnect = false;
    p.hidden = true;
  }
  node.allowLoop = desc->getBool("allowLoop", false);

  for (GraphData::Pin &p : node.pins)
  {
    // Match the descriptor pin by name. Descriptor pin counts are tiny (<= ~16) so a
    // linear scan is fine and avoids hash-map churn.
    const DataBlock *descPin = nullptr;
    for (uint32_t i = 0; i < desc->blockCount(); ++i)
    {
      const DataBlock *b = desc->getBlock(i);
      if (strcmp(b->getBlockName(), "pin") != 0)
      {
        continue;
      }
      if (strcmp(b->getStr("name", ""), p.name.c_str()) == 0)
      {
        descPin = b;
        break;
      }
    }
    if (!descPin)
    {
      continue;
    }

    p.role = parse_pin_role(descPin->getStr("role", "in"));
    p.isInput = (p.role != PinRole::Out);
    parse_pin_types_csv(descPin->getStr("types", ""), p.types);
    p.type = p.types.empty() ? PinType::Unknown : p.types.front();
    p.typeGroup.assign(descPin->getStr("typeGroup", ""));
    // singleConnect default mirrors graphEditor.js: outputs are single-connect, inputs are not.
    p.singleConnect = descPin->getBool("singleConnect", p.role == PinRole::Out);
    p.hidden = descPin->getBool("hidden", false);
    // shader_editor: outputs whose first type is not texture/particles are hidden -- mirrors
    // mainNodes.js GE_preprocessDescription.
    if (isShaderEditor && !p.isInput && !is_texture_or_particles(p.type))
    {
      p.hidden = true;
    }
  }
}

void preprocess_node_descriptor(DataBlock &node_desc)
{
  // The "<graph properties>" pseudo-node carries the graph-level texture defaults in the JS
  // editor (mainNodes.js:2974-3008 inflates it with texture/heightmap/dir properties). In the
  // C++ port those defaults live on GraphData fields (graphTextureWidth etc.) and are
  // rendered by PropertiesPanel::rebuildForGraph, so injecting per-node properties here
  // would duplicate the controls.
  if (strcmp(node_desc.getStr("name", ""), "<graph properties>") == 0)
  {
    return;
  }

  // Walk output pins to derive outDimension / outParticles (mainNodes.js:2931-2948). We
  // match the JS behaviour of looking only at types[0] -- a pin tagged "float,texture2D"
  // counts as a float output and contributes nothing.
  int outDimension = 0;
  bool outParticles = false;
  for (uint32_t i = 0; i < node_desc.blockCount(); ++i)
  {
    const DataBlock *pinBlk = node_desc.getBlock(i);
    if (strcmp(pinBlk->getBlockName(), "pin") != 0)
    {
      continue;
    }
    if (strcmp(pinBlk->getStr("role", "in"), "out") != 0)
    {
      continue;
    }
    const eastl::string firstType = first_type_token(pinBlk->getStr("types", ""));
    int pinDim = 0;
    if (firstType == "texture1D")
    {
      pinDim = 1;
    }
    else if (firstType == "texture2D")
    {
      pinDim = 2;
    }
    else if (firstType == "particles")
    {
      pinDim = 2;
      outParticles = true;
    }
    else if (firstType == "texture3D")
    {
      pinDim = 3;
    }
    if (pinDim > outDimension)
    {
      outDimension = pinDim;
    }
  }

  if (outDimension == 0)
  {
    return;
  }

  // Skip injection for properties the descriptor already declares (e.g. the "raw texture"
  // node in base_nodes.blk hard-codes texture width/height/type/wrap).
  bool haveTexWidth = false;
  bool haveTexHeight = false;
  bool haveTexDepth = false;
  bool haveTexType = false;
  bool haveTexWrap = false;
  for (uint32_t i = 0; i < node_desc.blockCount(); ++i)
  {
    const DataBlock *propBlk = node_desc.getBlock(i);
    if (strcmp(propBlk->getBlockName(), "property") != 0)
    {
      continue;
    }
    const char *propName = propBlk->getStr("name", "");
    if (strcmp(propName, "texture width") == 0)
    {
      haveTexWidth = true;
    }
    else if (strcmp(propName, "texture height") == 0)
    {
      haveTexHeight = true;
    }
    else if (strcmp(propName, "texture depth") == 0)
    {
      haveTexDepth = true;
    }
    else if (strcmp(propName, "texture type") == 0)
    {
      haveTexType = true;
    }
    else if (strcmp(propName, "texture wrap") == 0)
    {
      haveTexWrap = true;
    }
  }

  // Default values mirror mainNodes.js:3001-3021 for the non-graph-properties branch:
  // size defaults to SIZE_PREFIXES[0] ("parent size"), height/depth to "width", type to
  // TYPE_PREFIXES[0] ("parent type"), wrap to WRAP_PREFIXES[0] ("parent wrap"). Particles
  // outputs override width to "= 512" and height to "= 1" so they map naturally to a
  // 1D particle buffer rather than inheriting upstream texture dimensions.
  const char *defaultWidth = outParticles ? "= 512" : SIZE_PREFIXES[0];
  const char *defaultHeight = outParticles ? "= 1" : "width";
  const char *defaultDepth = "width";
  const char *defaultType = TYPE_PREFIXES[0];
  const char *defaultWrap = WRAP_PREFIXES[0];

  auto appendCombobox = [&](const char *name, const char *val) -> DataBlock * {
    DataBlock *p = node_desc.addNewBlock("property");
    p->setStr("name", name);
    p->setStr("type", "combobox");
    p->setStr("val", val);
    return p->addNewBlock("items");
  };

  if (outDimension >= 1 && !haveTexWidth)
  {
    push_size_items(*appendCombobox("texture width", defaultWidth), /*include_width=*/false);
  }
  if (outDimension >= 2 && !haveTexHeight)
  {
    push_size_items(*appendCombobox("texture height", defaultHeight), /*include_width=*/true);
  }
  if (outDimension >= 3 && !haveTexDepth)
  {
    push_size_items(*appendCombobox("texture depth", defaultDepth), /*include_width=*/true);
  }
  if (!haveTexType)
  {
    push_type_items(*appendCombobox("texture type", defaultType));
  }
  if (!haveTexWrap)
  {
    push_wrap_items(*appendCombobox("texture wrap", defaultWrap));
  }
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

void clear_graph_data(GraphData &out)
{
  out.nodes.clear();
  out.edges.clear();
  out.mainGraphBlk.reset();
  out.shaderListBlk.reset();
  out.textureRootDir.clear();
  out.renderDir.clear();
  out.entityDir.clear();
  out.heightmapScale = FLT_MAX;
  out.heightmapMin = FLT_MAX;
  out.heightmapCellSize = FLT_MAX;
  out.graphTextureWidth = 1024;
  out.graphTextureHeight = 1024;
  out.graphTextureDepth = 1024;
  out.graphTextureType = "R16F";
  out.graphTextureWrap = "wrap";
  out.sourcePath.clear();
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

  for (const GraphData::Node &n : gd.nodes)
  {
    const char *dn = n.descName.c_str();
    const bool isIn = strncmp(dn, "subgraph in:", 12) == 0;
    const bool isOut = strcmp(dn, "subgraph out") == 0;
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
      SubgraphSchemaError err;
      err.code = SubgraphSchemaError::EMPTY_INTERFACE_NAME;
      err.nodeId = n.id;
      err.isInputRole = isIn;
      out_errors.push_back(eastl::move(err));
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
      SubgraphSchemaError err;
      err.code = SubgraphSchemaError::NO_BOUNDARIES; // -V1048
      out_errors.push_back(eastl::move(err));
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
      SubgraphSchemaError err;
      err.code = SubgraphSchemaError::NO_OUTPUTS;
      out_errors.push_back(eastl::move(err));
    }
  }

  // Duplicate-name check: (role, name) must be unique across the whole boundary set
  for (size_t i = 0; i < boundaries.size(); ++i)
  {
    for (size_t j = 0; j < i; ++j)
    {
      if (boundaries[i].isInput == boundaries[j].isInput && boundaries[i].name == boundaries[j].name)
      {
        SubgraphSchemaError err;
        err.code = SubgraphSchemaError::DUPLICATE_NAME;
        err.nodeId = boundaries[i].nodeId;
        err.interfaceName = boundaries[i].name;
        err.isInputRole = boundaries[i].isInput;
        out_errors.push_back(eastl::move(err));
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
    {
      const std::string s = std::format(
        "node id={}: {} boundary has empty `name` property and no incident edges to derive a name from", err.nodeId, roleStr);
      return eastl::string(s.c_str(), s.size());
    }
    case SubgraphSchemaError::DUPLICATE_NAME:
    {
      const std::string s = std::format("node id={}: duplicate {} boundary named '{}' (another boundary already exposes this name)",
        err.nodeId, roleStr, err.interfaceName.c_str());
      return eastl::string(s.c_str(), s.size());
    }
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
    if (strncmp(startNode.descName.c_str(), "subgraph in:", 12) != 0)
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
      if (curId != startNode.id && cur.descName == "subgraph out")
      {
        reachesOutput = true;
        break;
      }

      // Walk outgoing edges (the ones leaving an out-role pin on `cur`).
      for (const GraphData::Edge &e : gd.edges)
      {
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
  const GraphData::Node *boundary = nullptr;
  for (const GraphData::Node &n : gd.nodes)
  {
    if (n.id == boundary_node_id)
    {
      boundary = &n;
      break;
    }
  }
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
    for (const GraphData::Node &n : gd.nodes)
    {
      if (n.id != otherNodeId)
      {
        continue;
      }
      if (otherPinIdx >= 0 && otherPinIdx < static_cast<int>(n.pins.size()))
      {
        const eastl::string &name = n.pins[otherPinIdx].name;
        if (!name.empty())
        {
          return name;
        }
      }
      break;
    }
  }

  return {};
}

bool load_graph_data(GraphData &out, const char *json_path, const char *shader_includes_dir, const DataBlock *base_nodes_blk)
{
  clear_graph_data(out);

  const int64_t tStart = ref_time_ticks();

  String text = read_file_to_string(json_path);
  if (text.empty())
  {
    debug("load_graph_data: failed to read '%s'", json_path);
    return false;
  }

  // Parse (not ParseInsitu): the marker-extraction pass below scans the original buffer with
  // strstr, and ParseInsitu would null-terminate string values mid-buffer and break the scan.
  rapidjson::Document root;
  root.Parse(text.str());
  if (root.HasParseError() || !root.IsObject())
  {
    debug("load_graph_data: '%s' is not a JSON object (parse error %d)", json_path, (int)root.GetParseError());
    clear_graph_data(out);
    return false;
  }

  if (root.HasMember("elems") && root["elems"].IsArray())
  {
    const rapidjson::Value &elems = root["elems"];
    out.nodes.reserve(elems.Size());
    for (rapidjson::SizeType i = 0; i < elems.Size(); ++i)
    {
      const rapidjson::Value &e = elems[i];
      if (!e.IsObject())
      {
        continue; // sparse array contains nulls
      }

      GraphData::Node n;
      if (e.HasMember("id") && e["id"].IsNumber())
      {
        n.id = e["id"].GetInt();
      }
      if (e.HasMember("descName") && e["descName"].IsString())
      {
        n.descName = e["descName"].GetString();
      }
      if (e.HasMember("plugin") && e["plugin"].IsString())
      {
        n.plugin = e["plugin"].GetString();
      }
      if (e.HasMember("view") && e["view"].IsObject())
      {
        const rapidjson::Value &v = e["view"];
        if (v.HasMember("x") && v["x"].IsNumber())
        {
          n.x = static_cast<float>(v["x"].GetDouble());
        }
        if (v.HasMember("y") && v["y"].IsNumber())
        {
          n.y = static_cast<float>(v["y"].GetDouble());
        }
      }
      if (e.HasMember("blockWidth") && e["blockWidth"].IsNumber())
      {
        n.blockWidth = static_cast<float>(e["blockWidth"].GetDouble());
      }
      if (e.HasMember("blockHeight") && e["blockHeight"].IsNumber())
      {
        n.blockHeight = static_cast<float>(e["blockHeight"].GetDouble());
      }
      if (e.HasMember("pins") && e["pins"].IsArray())
      {
        // Per-instance pin state. `name`, `role` and `types` are read so they can serve as
        // a fallback in resolve_node_pins for nodes whose descriptor isn't in base_nodes.blk
        // (shader_editor-generated nodes like mask_from_index / multi_max get their pin shape
        // only from per-instance JSON). When the descriptor IS present, resolve_node_pins
        // resets these and re-derives from the descriptor, so the JSON-loaded values are the
        // authoritative source only in the descriptor-missing case. customTextureName is
        // intentionally NOT read: it's a compile-output cache repopulated by every
        // compile_graph_to_blks via its write-back loop, and the texture-preview panel only
        // reads the field after a compile has run.
        const rapidjson::Value &pins = e["pins"];
        n.pins.reserve(pins.Size());
        for (rapidjson::SizeType j = 0; j < pins.Size(); ++j)
        {
          const rapidjson::Value &p = pins[j];
          GraphData::Pin pin;
          if (p.IsObject())
          {
            if (p.HasMember("name") && p["name"].IsString())
            {
              pin.name = p["name"].GetString();
            }
            if (p.HasMember("role") && p["role"].IsString())
            {
              pin.role = parse_pin_role(p["role"].GetString());
              pin.isInput = (pin.role != PinRole::Out);
            }
            if (p.HasMember("types") && p["types"].IsArray())
            {
              const rapidjson::Value &types = p["types"];
              for (rapidjson::SizeType k = 0; k < types.Size(); ++k)
              {
                if (types[k].IsString())
                {
                  pin.types.push_back(parse_pin_type(types[k].GetString()));
                }
              }
              if (!pin.types.empty())
              {
                pin.type = pin.types.front();
              }
            }
            // JSON has no singleConnect; mirror the descriptor-side default
            // (outputs are single-connect, inputs are not).
            pin.singleConnect = (pin.role == PinRole::Out);
            // Per-instance pin data ("data" object on the JSON pin). Stored as a
            // DataBlock so compile_graph_to_blks can use the same getStr / getBool
            // API regardless of whether the data ultimately came from per-instance
            // JSON (shader_editor sub-graph nodes) or from base_nodes.blk
            // (regular nodes' descriptor pins).
            if (p.HasMember("data") && p["data"].IsObject())
            {
              const rapidjson::Value &dataObj = p["data"];
              for (auto m = dataObj.MemberBegin(); m != dataObj.MemberEnd(); ++m)
              {
                if (!m->name.IsString())
                {
                  continue;
                }
                const char *key = m->name.GetString();
                const rapidjson::Value &v = m->value;
                if (v.IsString())
                {
                  pin.data.setStr(key, v.GetString());
                }
                else if (v.IsBool())
                {
                  pin.data.setBool(key, v.GetBool());
                }
                else if (v.IsInt())
                {
                  pin.data.setInt(key, v.GetInt());
                }
                else if (v.IsNumber())
                {
                  pin.data.setReal(key, static_cast<float>(v.GetDouble()));
                }
                // Nested objects / arrays in pin.data are not used by the compile
                // step today -- skip rather than serialise to a string.
              }
            }
          }
          n.pins.push_back(pin);
        }
      }
      if (e.HasMember("properties") && e["properties"].IsArray())
      {
        // Property *values* by name. Constraints (type / minVal / maxVal / items[]) live in
        // the descriptor; values stored as strings, parsed by consumers per descriptor type.
        const rapidjson::Value &props = e["properties"];
        n.propertyValues.reserve(props.Size());
        for (rapidjson::SizeType j = 0; j < props.Size(); ++j)
        {
          const rapidjson::Value &pp = props[j];
          if (!pp.IsObject() || !pp.HasMember("name") || !pp["name"].IsString())
          {
            continue;
          }
          eastl::string name = pp["name"].GetString();
          eastl::string val;
          if (pp.HasMember("value"))
          {
            val = json_value_to_string(pp["value"]).c_str();
          }
          n.propertyValues.emplace_back(eastl::move(name), eastl::move(val));
        }
      }
      // JSON files come from the legacy JS editor and never carry templateUid; the
      // reconciliation pass falls back to descName lookup and populates templateUid
      // in memory so the next save (to BLK) upgrades the file.
      reconcile_template_uid(n, base_nodes_blk);
      // Resolve descriptor-derived pin state once, here -- avoids per-frame strcmp scans
      // in the renderer. Pins whose name isn't in the descriptor stay `hidden = true`.
      resolve_node_pins(n, base_nodes_blk);
      out.nodes.push_back(eastl::move(n));
    }
  }

  if (root.HasMember("edges") && root["edges"].IsArray())
  {
    const rapidjson::Value &edgeArr = root["edges"];
    out.edges.reserve(edgeArr.Size());
    for (rapidjson::SizeType i = 0; i < edgeArr.Size(); ++i)
    {
      const rapidjson::Value &c = edgeArr[i];
      if (!c.IsObject())
      {
        continue;
      }

      GraphData::Edge ed;
      if (c.HasMember("id") && c["id"].IsNumber())
      {
        ed.id = c["id"].GetInt();
      }
      if (c.HasMember("elemA") && c["elemA"].IsNumber())
      {
        ed.elemA = c["elemA"].GetInt();
      }
      if (c.HasMember("pinA") && c["pinA"].IsNumber())
      {
        ed.pinA = c["pinA"].GetInt();
      }
      if (c.HasMember("elemB") && c["elemB"].IsNumber())
      {
        ed.elemB = c["elemB"].GetInt();
      }
      if (c.HasMember("pinB") && c["pinB"].IsNumber())
      {
        ed.pinB = c["pinB"].GetInt();
      }
      out.edges.push_back(ed);
    }
  }

  // Pull the embedded BLK sections + dirs out of the JSON. Use the original (non-ASCII-stripped)
  // text to keep the BLK content byte-identical.
  String mainGraphBlk = extract_marker_section(text, "/*MAIN_GRAPH_START*/", "/*MAIN_GRAPH_END*/");
  String shaderListBlk = extract_marker_section(text, "/*SHADER_LIST_START*/", "/*SHADER_LIST_END*/");
  String rootDir = extract_marker_section(text, "/*ROOT_DIR_START*/", "/*ROOT_DIR_END*/");
  String renderDir = extract_marker_section(text, "/*RENDER_DIR_START*/", "/*RENDER_DIR_END*/");
  String entityDir = extract_marker_section(text, "/*ENTITY_DIR_START*/", "/*ENTITY_DIR_END*/");

  if (!mainGraphBlk.empty())
  {
    out.mainGraphBlk.loadText(mainGraphBlk.str(), mainGraphBlk.length());
  }
  if (!shaderListBlk.empty())
  {
    // The shader list BLK uses `include _foo.blk` directives that resolve relative to the dirname
    // of the file argument. Synthesize a path under shader_includes_dir so the parser looks in
    // the right place; the file itself is never read.
    String syntheticFname(0, "%sshadersList.blk", shader_includes_dir ? shader_includes_dir : "");
    out.shaderListBlk.loadText(shaderListBlk.str(), shaderListBlk.length(), syntheticFname.str());
  }

  out.textureRootDir = rootDir.str();
  out.renderDir = renderDir.str();
  out.entityDir = entityDir.str();

  String heightScaleStr = extract_marker_section(text, "/*HEIGHT_SCALE_START*/", "/*HEIGHT_SCALE_END*/");
  String heightMinStr = extract_marker_section(text, "/*HEIGHT_MIN_START*/", "/*HEIGHT_MIN_END*/");
  String cellSizeStr = extract_marker_section(text, "/*CELL_SIZE_START*/", "/*CELL_SIZE_END*/");
  out.heightmapScale = heightScaleStr.empty() ? FLT_MAX : (float)atof(heightScaleStr.str());
  out.heightmapMin = heightMinStr.empty() ? FLT_MAX : (float)atof(heightMinStr.str());
  out.heightmapCellSize = cellSizeStr.empty() ? FLT_MAX : (float)atof(cellSizeStr.str());

  // Read graph-level texture properties from `graphElemProps` (the JS serialises
  // editor.graphElem.propValues here as a name->string dict, mainNodes.js around
  // graphEditor.js:4015-4023). New JSONs always have this; old ones may not.
  // Texture width / height / depth are stored in the JS form ("= 4096" / "width" /
  // "x 2" etc.). Translate to ints; only "= N" and "width" are meaningful at the
  // graph level (the rest only make sense for per-node propagation). Type / wrap
  // are stored as plain strings.
  if (root.HasMember("graphElemProps") && root["graphElemProps"].IsObject())
  {
    const rapidjson::Value &gp = root["graphElemProps"];
    auto getStr = [&](const char *key) -> const char * {
      if (!gp.HasMember(key) || !gp[key].IsString())
      {
        return nullptr;
      }
      return gp[key].GetString();
    };
    if (const char *w = getStr("texture width"))
    {
      out.graphTextureWidth = parse_graph_size(w, out.graphTextureWidth);
    }
    if (const char *h = getStr("texture height"))
    {
      out.graphTextureHeight = (strcmp(h, "width") == 0) ? out.graphTextureWidth : parse_graph_size(h, out.graphTextureHeight);
    }
    if (const char *d = getStr("texture depth"))
    {
      out.graphTextureDepth = (strcmp(d, "width") == 0) ? out.graphTextureWidth : parse_graph_size(d, out.graphTextureDepth);
    }
    if (const char *t = getStr("texture type"))
    {
      out.graphTextureType = t;
    }
    if (const char *a = getStr("texture wrap"))
    {
      out.graphTextureWrap = a;
    }

    // Same fields are also present in the marker sections above; graphElemProps
    // is the newer source of truth. Overwrite the marker-derived values when
    // graphElemProps carries them (the marker form is an older serialisation).
    if (const char *s = getStr("texture root dir"))
    {
      if (*s)
      {
        out.textureRootDir = s;
      }
    }
    if (const char *s = getStr("render dir"))
    {
      if (*s)
      {
        out.renderDir = s;
      }
    }
    if (const char *s = getStr("entity dir"))
    {
      if (*s)
      {
        out.entityDir = s;
      }
    }
    if (const char *s = getStr("height scale"))
    {
      if (*s)
      {
        out.heightmapScale = (float)atof(s);
      }
    }
    if (const char *s = getStr("height min"))
    {
      if (*s)
      {
        out.heightmapMin = (float)atof(s);
      }
    }
    if (const char *s = getStr("cell size"))
    {
      if (*s)
      {
        out.heightmapCellSize = (float)atof(s);
      }
    }
  }

  out.sourcePath = json_path;

  const int loadMs = static_cast<int>(ref_time_delta_to_usec(tStart) / 1000);
  debug("load_graph_data: loaded '%s' in %d ms (%d nodes, %d edges, mainBlk=%d blocks, shaderBlk=%d blocks)", json_path, loadMs,
    (int)out.nodes.size(), (int)out.edges.size(), out.mainGraphBlk.blockCount(), out.shaderListBlk.blockCount());
  return true;
}

bool save_graph_data_blk(const GraphData &d, const char *blk_path)
{
  const int64_t tStart = ref_time_ticks();

  DataBlock root;

  if (!d.textureRootDir.empty())
  {
    root.setStr("textureRootDir", d.textureRootDir.c_str());
  }
  if (!d.renderDir.empty())
  {
    root.setStr("renderDir", d.renderDir.c_str());
  }
  if (!d.entityDir.empty())
  {
    root.setStr("entityDir", d.entityDir.c_str());
  }

  // FLT_MAX is the sentinel `clear_graph_data` uses for "absent". Keep the sentinel
  // implicit in the saved file -- consumers default back to FLT_MAX on load.
  if (d.heightmapScale != FLT_MAX)
  {
    root.setReal("heightmapScale", d.heightmapScale);
  }
  if (d.heightmapMin != FLT_MAX)
  {
    root.setReal("heightmapMin", d.heightmapMin);
  }
  if (d.heightmapCellSize != FLT_MAX)
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
  // produced on demand by compile_graph_to_blks
  // (prog/tools/sceneTools/daEditorX/GraphEditor/graph_compile.cpp). The caller
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
  }

  if (!root.saveToTextFile(blk_path))
  {
    debug("save_graph_data_blk: failed to write '%s'", blk_path);
    return false;
  }

  const int saveMs = static_cast<int>(ref_time_delta_to_usec(tStart) / 1000);
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

  out.textureRootDir = root.getStr("textureRootDir", "");
  out.renderDir = root.getStr("renderDir", "");
  out.entityDir = root.getStr("entityDir", "");
  out.heightmapScale = root.getReal("heightmapScale", FLT_MAX);
  out.heightmapMin = root.getReal("heightmapMin", FLT_MAX);
  out.heightmapCellSize = root.getReal("heightmapCellSize", FLT_MAX);
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
          pin.role = parse_pin_role(c->getStr("role", "in"));
          pin.isInput = (pin.role != PinRole::Out);
          parse_pin_types_csv(c->getStr("types", ""), pin.types);
          pin.type = pin.types.empty() ? PinType::Unknown : pin.types.front();
          // JSON-loader parity: outputs default to single-connect, inputs do not.
          // resolve_node_pins below overrides this when the descriptor is present.
          pin.singleConnect = (pin.role == PinRole::Out);
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
      // Same one-shot descriptor-cache resolve the JSON loader does.
      // Pins whose name isn't in the descriptor stay hidden = true.
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
      out.edges.push_back(ed);
    }
  }

  out.sourcePath = blk_path;

  const int loadMs = static_cast<int>(ref_time_delta_to_usec(tStart) / 1000);
  debug("load_graph_data_blk: loaded '%s' in %d ms (%d nodes, %d edges)", blk_path, loadMs, (int)out.nodes.size(),
    (int)out.edges.size());
  return true;
}
