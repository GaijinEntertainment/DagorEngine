// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <graphEditor/graph_ir.h>
#include <graphEditor/graph_data.h>

#include <ioSys/dag_dataBlock.h>

#include <EASTL/algorithm.h>
#include <EASTL/hash_map.h>
#include <EASTL/sort.h>

#include <string.h>

namespace
{
constexpr const char *PROP_FINAL_NAME = "texture name";
constexpr const char *PROP_CHANNELS = "channels";
constexpr const char *PROP_SKIP_SAVING = "skip when exporting";

static_assert(texgen::GraphOptions::UNSET == UNSET_HEIGHT);

constexpr float LAYOUT_PITCH_X = 260.0f;
constexpr float LAYOUT_PITCH_Y = 140.0f;

int find_pin_by_name(const eastl::vector<const DataBlock *> &pins, const char *name)
{
  for (int i = 0; i < static_cast<int>(pins.size()); ++i)
  {
    if (strcmp(pins[i]->getStr("name", ""), name) == 0)
    {
      return i;
    }
  }
  return -1;
}

bool has_property(const DataBlock &desc, const char *name) { return find_property_desc(&desc, name) != nullptr; }

// The stored value only, as the compile reads it.
eastl::string property_text(const GraphData::Node &n, const char *name)
{
  const eastl::string *stored = find_property_value(n, name);
  return stored ? *stored : eastl::string();
}

// preprocess_node_descriptor adds them to a template that produces a texture.
constexpr const char *TEXTURE_RULE_PROPS[] = {
  PROP_TEXTURE_WIDTH, PROP_TEXTURE_HEIGHT, PROP_TEXTURE_DEPTH, PROP_TEXTURE_TYPE, PROP_TEXTURE_WRAP};

void read_texture_rules(const GraphData::Node &n, const DataBlock &desc, texgen::Node &out, eastl::string &errors)
{
  auto read = [&](const char *prop, auto parse) {
    if (!has_property(desc, prop))
    {
      return;
    }
    const eastl::string text = texture_property_value(n, &desc, prop);
    if (!parse(text))
    {
      errors.append_sprintf("node %d: '%s' has the unknown item '%s'\n", n.id, prop, text.c_str());
    }
  };
  read(PROP_TEXTURE_WIDTH, [&](const eastl::string &s) { return parse_size_rule(s, false, out.width); });
  read(PROP_TEXTURE_HEIGHT, [&](const eastl::string &s) { return parse_size_rule(s, true, out.height); });
  read(PROP_TEXTURE_DEPTH, [&](const eastl::string &s) { return parse_size_rule(s, true, out.depth); });
  read(PROP_TEXTURE_TYPE, [&](const eastl::string &s) { return parse_fmt_rule(s, out.fmt); });
  read(PROP_TEXTURE_WRAP, [&](const eastl::string &s) { return parse_wrap_rule(s, out.wrap); });
}

void write_texture_rules(const texgen::Node &n, const DataBlock &desc, GraphData::Node &out)
{
  auto put = [&](const char *prop, const eastl::string &text) {
    if (has_property(desc, prop))
    {
      set_property_value(out, prop, text);
    }
  };
  put(PROP_TEXTURE_WIDTH, texgen::size_rule_text(n.width));
  put(PROP_TEXTURE_HEIGHT, texgen::size_rule_text(n.height));
  put(PROP_TEXTURE_DEPTH, texgen::size_rule_text(n.depth));
  put(PROP_TEXTURE_TYPE, texgen::fmt_rule_text(n.fmt));
  put(PROP_TEXTURE_WRAP, texgen::wrap_rule_text(n.wrap));
}

// consumed names the properties the node or final carries in its own fields.
template <class Consumed>
void read_params(const GraphData::Node &n, const DataBlock &desc, Consumed &&consumed, eastl::vector<texgen::Param> &out)
{
  for (const auto &pv : n.propertyValues)
  {
    if (consumed(pv.first))
    {
      continue;
    }
    const texgen::Value::Type t = value_type_for(find_property_desc(&desc, pv.first.c_str()));
    out.push_back({pv.first, texgen::value_from_text(t, pv.second.c_str())});
  }
}

void write_params(const eastl::vector<texgen::Param> &params, GraphData::Node &out)
{
  for (const texgen::Param &p : params)
  {
    set_property_value(out, p.name.c_str(), texgen::value_to_text(p.value));
  }
}

// Where a GraphData node went in the IR, with its template pins for the edge pass.
struct IrSlot
{
  bool isFinal = false;
  int index = -1; // into texgen::Graph::nodes or ::finals
  TemplatePins pins;
};
} // namespace

void collect_template_pins(const DataBlock &desc, TemplatePins &out)
{
  for (uint32_t i = 0; i < desc.blockCount(); ++i)
  {
    const DataBlock *b = desc.getBlock(i);
    if (strcmp(b->getBlockName(), "pin") != 0 || b->getBool("separator", false))
    {
      continue;
    }
    (strcmp(b->getStr("role", "in"), "out") == 0 ? out.outs : out.ins).push_back(b);
  }
}

bool is_final_template(const TemplatePins &pins)
{
  if (!pins.outs.empty() || pins.ins.size() != 1)
  {
    return false;
  }
  const DataBlock *data = pins.ins[0]->getBlockByName("data");
  return data && data->getBool("final", false);
}

texgen::Value::Type value_type_for(const DataBlock *prop_desc)
{
  static const eastl::pair<const char *, texgen::Value::Type> TYPES[] = {{"int", texgen::Value::Int}, {"float", texgen::Value::Float},
    {"bool", texgen::Value::Bool}, {"float2", texgen::Value::Float2}, {"float3", texgen::Value::Float3},
    {"color", texgen::Value::Float4}, {"float4", texgen::Value::Float4}};
  const char *t = infer_prop_type(prop_desc);
  for (const auto &[name, type] : TYPES)
  {
    if (strcmp(t, name) == 0)
    {
      return type;
    }
  }
  return texgen::Value::String;
}

bool parse_size_rule(const eastl::string &s, bool allow_width, texgen::SizeRule &out)
{
  static const eastl::pair<char, texgen::SizeRule::Kind> NUMBERED[] = {
    {'=', texgen::SizeRule::Explicit}, {'x', texgen::SizeRule::ParentMul}, {'/', texgen::SizeRule::ParentDiv}};
  for (const auto &[prefix, kind] : NUMBERED)
  {
    int n = 0;
    if (parse_prefixed_int(s.c_str(), prefix, n) && n > 0)
    {
      out = {kind, n};
      return true;
    }
  }
  if (s == SIZE_PREFIXES[0])
  {
    out = {texgen::SizeRule::Parent, 0};
  }
  else if (s == SIZE_PREFIXES[1])
  {
    out = {texgen::SizeRule::Graph, 0};
  }
  else if (allow_width && s == SIZE_SAME_AS_WIDTH)
  {
    out = {texgen::SizeRule::Width, 0};
  }
  else
  {
    return false;
  }
  return true;
}

bool parse_fmt_rule(const eastl::string &s, texgen::FmtRule &out)
{
  if (s.empty())
  {
    return false;
  }
  if (s == TYPE_PREFIXES[0])
  {
    out = {texgen::FmtRule::Parent, {}};
  }
  else if (s == TYPE_PREFIXES[1])
  {
    out = {texgen::FmtRule::Graph, {}};
  }
  else
  {
    out = {texgen::FmtRule::Explicit, s};
  }
  return true;
}

bool parse_wrap_rule(const eastl::string &s, texgen::WrapRule &out)
{
  if (s == WRAP_PREFIXES[0])
  {
    out = {texgen::WrapRule::Parent, true};
  }
  else if (s == WRAP_PREFIXES[1])
  {
    out = {texgen::WrapRule::Graph, true};
  }
  else if (s == BASE_WRAPS[0] || s == BASE_WRAPS[1])
  {
    out = {texgen::WrapRule::Explicit, s == BASE_WRAPS[0]};
  }
  else
  {
    return false;
  }
  return true;
}

bool graph_data_to_ir(const GraphData &gd, const DataBlock &base_nodes_blk, texgen::Graph &out, eastl::string &errors)
{
  out = texgen::Graph{};
  const size_t errorsBefore = errors.size();

  texgen::GraphOptions &o = out.options;
  o.width = gd.graphTextureWidth;
  o.height = gd.graphTextureHeight;
  o.depth = gd.graphTextureDepth;
  o.fmt = gd.graphTextureType;
  texgen::WrapRule graphWrap;
  if (!parse_wrap_rule(gd.graphTextureWrap, graphWrap) || graphWrap.kind != texgen::WrapRule::Explicit)
  {
    errors.append_sprintf("graph: unknown texture wrap '%s'\n", gd.graphTextureWrap.c_str());
  }
  o.wrap = graphWrap.wrap;
  o.renderDir = gd.renderDir;
  o.entityDir = gd.entityDir;
  o.heightmapScale = gd.heightmapScale;
  o.heightmapMin = gd.heightmapMin;
  o.heightmapCellSize = gd.heightmapCellSize;

  eastl::hash_map<int, IrSlot> slots;
  for (const GraphData::Node &n : gd.nodes)
  {
    if (is_annotation_node(n) || is_separator_boundary(n))
    {
      continue;
    }
    const DataBlock *desc = find_node_desc(&base_nodes_blk, n.templateUid.c_str());
    if (!desc)
    {
      errors.append_sprintf("node %d: no template '%s' (%s)\n", n.id, n.templateUid.c_str(), n.descName.c_str());
      continue;
    }
    IrSlot slot;
    collect_template_pins(*desc, slot.pins);
    const texgen::NodeId id{n.id + 1};
    if (is_final_template(slot.pins))
    {
      texgen::Final f;
      f.id = id;
      f.tmpl.uid = n.templateUid;
      f.name = property_text(n, PROP_FINAL_NAME);
      f.name.rtrim(" \t\n"); // as the compile exports it
      const bool exports = has_property(*desc, PROP_FILE_FORMAT) && find_property_value(n, PROP_CHANNELS);
      if (exports)
      {
        f.exportSpec = texgen::Export{
          f.name + property_text(n, PROP_FILE_FORMAT), property_text(n, PROP_CHANNELS), property_text(n, PROP_SKIP_SAVING) == "true"};
      }
      read_params(
        n, *desc,
        [exports](const eastl::string &name) {
          return name == PROP_FINAL_NAME ||
                 (exports && (name == PROP_FILE_FORMAT || name == PROP_CHANNELS || name == PROP_SKIP_SAVING));
        },
        f.params);
      slot.isFinal = true;
      slot.index = static_cast<int>(out.finals.size());
      out.finals.push_back(eastl::move(f));
    }
    else
    {
      texgen::Node node;
      node.id = id;
      node.tmpl.uid = n.templateUid;
      node.inputs.resize(slot.pins.ins.size());
      read_texture_rules(n, *desc, node, errors);
      read_params(
        n, *desc,
        [desc](const eastl::string &name) {
          for (const char *prop : TEXTURE_RULE_PROPS)
          {
            if (name == prop)
            {
              return has_property(*desc, prop);
            }
          }
          return false;
        },
        node.params);
      slot.index = static_cast<int>(out.nodes.size());
      out.nodes.push_back(eastl::move(node));
    }
    slots.emplace(n.id, eastl::move(slot));
  }

  for (const GraphData::Edge &e : gd.edges)
  {
    const auto sa = slots.find(e.elemA);
    const auto sb = slots.find(e.elemB);
    if (sa == slots.end() || sb == slots.end())
    {
      continue; // a removed or canvas node, or one already reported
    }
    const GraphData::Pin *pa = find_pin(gd, e.elemA, e.pinA);
    const GraphData::Pin *pb = find_pin(gd, e.elemB, e.pinB);
    if (!pa || !pb || (pa->role == PinRole::Out) == (pb->role == PinRole::Out))
    {
      errors.append_sprintf("edge %d: does not join an out pin to an in pin\n", e.id);
      continue;
    }
    const bool aIsSrc = pa->role == PinRole::Out;
    const int srcId = aIsSrc ? e.elemA : e.elemB;
    const int dstId = aIsSrc ? e.elemB : e.elemA;
    const GraphData::Pin &srcPin = aIsSrc ? *pa : *pb;
    const GraphData::Pin &dstPin = aIsSrc ? *pb : *pa;
    const IrSlot &srcSlot = aIsSrc ? sa->second : sb->second;
    const IrSlot &dstSlot = aIsSrc ? sb->second : sa->second;
    const int outIdx = find_pin_by_name(srcSlot.pins.outs, srcPin.name.c_str());
    const int inIdx = find_pin_by_name(dstSlot.pins.ins, dstPin.name.c_str());
    if (outIdx < 0 || inIdx < 0)
    {
      errors.append_sprintf("edge %d: pin '%s' of node %d is not in its template\n", e.id,
        outIdx < 0 ? srcPin.name.c_str() : dstPin.name.c_str(), outIdx < 0 ? srcId : dstId);
      continue;
    }
    texgen::PinRef &ref = dstSlot.isFinal ? out.finals[dstSlot.index].src : out.nodes[dstSlot.index].inputs[inIdx];
    if (ref.node)
    {
      errors.append_sprintf("edge %d: pin '%s' of node %d is fed twice\n", e.id, dstPin.name.c_str(), dstId);
      continue;
    }
    ref.node = texgen::NodeId{srcId + 1};
    ref.pin = static_cast<uint16_t>(outIdx);
    ref.muted = e.muted;
  }

  texgen::validate(out, errors);
  return errors.size() == errorsBefore;
}

bool ir_to_graph_data(const texgen::Graph &g, const DataBlock &base_nodes_blk, GraphData &out, eastl::string &errors)
{
  clear_graph_data(out);
  const size_t errorsBefore = errors.size();
  texgen::validate(g, errors);

  const texgen::GraphOptions &o = g.options;
  out.graphTextureWidth = o.width;
  out.graphTextureHeight = o.height;
  out.graphTextureDepth = o.depth;
  out.graphTextureType = o.fmt;
  out.graphTextureWrap = o.wrap ? BASE_WRAPS[0] : BASE_WRAPS[1];
  out.renderDir = o.renderDir;
  out.entityDir = o.entityDir;
  out.heightmapScale = o.heightmapScale;
  out.heightmapMin = o.heightmapMin;
  out.heightmapCellSize = o.heightmapCellSize;

  // The nth connectable pin of a fresh node, which make_node_from_descriptor builds in descriptor order.
  auto pinIndex = [](const GraphData::Node &n, bool want_out, int nth) {
    for (int i = 0; i < static_cast<int>(n.pins.size()); ++i)
    {
      const GraphData::Pin &p = n.pins[i];
      if (!p.separator && (p.role == PinRole::Out) == want_out && nth-- == 0)
      {
        return i;
      }
    }
    return -1;
  };
  // Appends the node made from its template; returns the template, or null when it is not in base_nodes_blk.
  auto makeNode = [&](texgen::NodeId id, const texgen::TemplateId &tmpl, bool as_final) -> const DataBlock * {
    const DataBlock *desc = find_node_desc(&base_nodes_blk, tmpl.uid.c_str());
    TemplatePins pins;
    if (desc)
    {
      collect_template_pins(*desc, pins);
    }
    if (!desc || is_final_template(pins) != as_final)
    {
      const char *problem = !desc ? "is not in the registry" : as_final ? "is not a final template" : "is a final template";
      errors.append_sprintf("%s %d: template '%s' %s\n", as_final ? "final" : "node", id.v, tmpl.uid.c_str(), problem);
      return nullptr;
    }
    GraphData::Node &n = out.nodes.push_back();
    make_node_from_descriptor(*desc, 0.0f, 0.0f, n);
    n.id = id.v - 1;
    return desc;
  };

  for (const texgen::Node &in : g.nodes)
  {
    if (const DataBlock *desc = makeNode(in.id, in.tmpl, false))
    {
      write_params(in.params, out.nodes.back());
      write_texture_rules(in, *desc, out.nodes.back());
    }
  }
  for (const texgen::Final &f : g.finals)
  {
    const DataBlock *desc = makeNode(f.id, f.tmpl, true);
    if (!desc)
    {
      continue;
    }
    GraphData::Node *n = &out.nodes.back();
    // a stored "channels" makes the final an export
    eastl::erase_if(n->propertyValues, [](const auto &pv) { return pv.first == PROP_CHANNELS; });
    write_params(f.params, *n);
    if (!f.exportSpec)
    {
      if (has_property(*desc, PROP_FINAL_NAME))
      {
        set_property_value(*n, PROP_FINAL_NAME, f.name);
      }
      continue;
    }
    if (!has_property(*desc, PROP_FILE_FORMAT))
    {
      errors.append_sprintf("final %d: template '%s' does not export\n", f.id.v, f.tmpl.uid.c_str());
      continue;
    }
    const texgen::Export &e = *f.exportSpec;
    eastl::string name, format;
    texgen::split_export_file(e, f.name, name, format);
    set_property_value(*n, PROP_FINAL_NAME, eastl::move(name));
    set_property_value(*n, PROP_FILE_FORMAT, eastl::move(format));
    set_property_value(*n, PROP_CHANNELS, e.fmt);
    if (has_property(*desc, PROP_SKIP_SAVING) || e.skipSaving)
    {
      set_property_value(*n, PROP_SKIP_SAVING, e.skipSaving ? "true" : "false");
    }
  }

  eastl::hash_map<int, int> index;
  build_node_index(out, index);
  auto addEdge = [&](const texgen::PinRef &ref, int consumer_id, int consumer_slot) {
    if (!ref.node)
    {
      return;
    }
    const auto src = index.find(ref.node.v - 1);
    const auto dst = index.find(consumer_id - 1);
    if (src == index.end() || dst == index.end())
    {
      return; // the missing template is already reported
    }
    const int srcPin = pinIndex(out.nodes[src->second], true, ref.pin);
    const int dstPin = pinIndex(out.nodes[dst->second], false, consumer_slot);
    if (srcPin < 0 || dstPin < 0)
    {
      errors.append_sprintf("node %d: %s pin %d is not in the template\n", srcPin < 0 ? ref.node.v : consumer_id,
        srcPin < 0 ? "out" : "in", srcPin < 0 ? ref.pin : consumer_slot);
      return;
    }
    GraphData::Edge &e = out.edges.push_back();
    e.id = static_cast<int>(out.edges.size()) - 1;
    e.elemA = ref.node.v - 1;
    e.pinA = srcPin;
    e.elemB = consumer_id - 1;
    e.pinB = dstPin;
    e.muted = ref.muted;
  };
  for (const texgen::Node &n : g.nodes)
  {
    for (int k = 0; k < static_cast<int>(n.inputs.size()); ++k)
    {
      addEdge(n.inputs[k], n.id.v, k);
    }
  }
  for (const texgen::Final &f : g.finals)
  {
    addEdge(f.src, f.id.v, 0);
  }

  auto_layout(out);
  return errors.size() == errorsBefore;
}

void auto_layout(GraphData &gd)
{
  const int count = static_cast<int>(gd.nodes.size());
  eastl::hash_map<int, int> index;
  build_node_index(gd, index);

  eastl::vector<eastl::vector<int>> preds(count);
  eastl::vector<eastl::vector<int>> succs(count);
  for (const GraphData::Edge &e : gd.edges)
  {
    int srcId = 0, srcPin = 0;
    if (!edge_source_pin(gd, e, srcId, srcPin))
    {
      continue;
    }
    const int dstId = srcId == e.elemA ? e.elemB : e.elemA;
    const auto s = index.find(srcId);
    const auto d = index.find(dstId);
    if (s == index.end() || d == index.end() || s->second == d->second)
    {
      continue;
    }
    preds[d->second].push_back(s->second);
    succs[s->second].push_back(d->second);
  }

  // Kahn's order; a node left in a loop is taken in node order, with only its placed inputs counted.
  eastl::vector<int> layer(count, 0);
  eastl::vector<int> pending(count);
  eastl::vector<uint8_t> placed(count, 0);
  eastl::vector<int> ready;
  size_t readyHead = 0;
  for (int i = 0; i < count; ++i)
  {
    pending[i] = static_cast<int>(preds[i].size());
    if (pending[i] == 0)
    {
      ready.push_back(i);
    }
  }
  int nextLoopNode = 0;
  for (int done = 0; done < count; ++done)
  {
    int v = -1;
    if (readyHead < ready.size())
    {
      v = ready[readyHead++];
    }
    else
    {
      while (placed[nextLoopNode])
      {
        ++nextLoopNode;
      }
      v = nextLoopNode;
    }
    placed[v] = 1;
    for (int p : preds[v])
    {
      if (placed[p])
      {
        layer[v] = eastl::max(layer[v], layer[p] + 1);
      }
    }
    for (int s : succs[v])
    {
      if (!placed[s] && --pending[s] == 0)
      {
        ready.push_back(s);
      }
    }
  }

  eastl::vector<eastl::vector<int>> columns(count ? *eastl::max_element(layer.begin(), layer.end()) + 1 : 0);
  for (int i = 0; i < count; ++i)
  {
    columns[layer[i]].push_back(i);
  }
  eastl::vector<float> row(count, 0.0f), key(count, 0.0f);
  for (eastl::vector<int> &column : columns)
  {
    for (size_t k = 0; k < column.size(); ++k)
    {
      float sum = 0.0f;
      int n = 0;
      for (int p : preds[column[k]])
      {
        if (layer[p] < layer[column[k]])
        {
          sum += row[p];
          ++n;
        }
      }
      // A node with no input to the left keeps its node order after the others.
      key[column[k]] = n > 0 ? sum / n : static_cast<float>(count + k);
    }
    eastl::stable_sort(column.begin(), column.end(), [&](int a, int b) { return key[a] < key[b]; });
    for (size_t r = 0; r < column.size(); ++r)
    {
      const int v = column[r];
      row[v] = static_cast<float>(r);
      gd.nodes[v].x = layer[v] * LAYOUT_PITCH_X;
      gd.nodes[v].y = r * LAYOUT_PITCH_Y;
    }
  }
}
