// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <graphEditor/graph_data.h>

#include <debug/dag_debug.h>
#include <osApiWrappers/dag_files.h>
#include <perfMon/dag_cpuFreq.h>
#include <util/dag_string.h>

#include <rapidjson/document.h>

#include <cstdlib>
#include <cstring>
#include <float.h>

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
static void reconcile_template_uid(GraphData::Node &node, const DataBlock *base_nodes_blk)
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
        // Per-instance pin state. `name` and `customTextureName` are always read; `role` and
        // `types` are read so they can serve as a fallback in resolve_node_pins for nodes
        // whose descriptor isn't in base_nodes.blk (shader_editor-generated nodes like
        // mask_from_index / multi_max get their pin shape only from per-instance JSON).
        // When the descriptor IS present, resolve_node_pins resets these and re-derives
        // from the descriptor, so the JSON-loaded values are the authoritative source only
        // in the descriptor-missing case.
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
            if (p.HasMember("customTextureName") && p["customTextureName"].IsString())
            {
              pin.customTextureName = p["customTextureName"].GetString();
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
    auto parseSize = [](const char *s, int fallback) -> int {
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
    };

    if (const char *w = getStr("texture width"))
    {
      out.graphTextureWidth = parseSize(w, out.graphTextureWidth);
    }
    if (const char *h = getStr("texture height"))
    {
      out.graphTextureHeight = (strcmp(h, "width") == 0) ? out.graphTextureWidth : parseSize(h, out.graphTextureHeight);
    }
    if (const char *d = getStr("texture depth"))
    {
      out.graphTextureDepth = (strcmp(d, "width") == 0) ? out.graphTextureWidth : parseSize(d, out.graphTextureDepth);
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
      if (!p.customTextureName.empty())
      {
        pb->setStr("customTextureName", p.customTextureName.c_str());
      }
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
          pin.customTextureName = c->getStr("customTextureName", "");
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
          eastl::string val = c->getStr("val", "");
          if (name.empty())
          {
            continue;
          }
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
