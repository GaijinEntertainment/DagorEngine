// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "graph_compile.h"

#include <ioSys/dag_dataBlock.h>
#include <debug/dag_debug.h>
#include <util/dag_string.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>

#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <EASTL/hash_set.h>
#include <EASTL/hash_map.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

// Port of `generateHlslCode` from tools/graphEditor/mainNodes.js (~L2218-L2884).
// The JS produces a text BLK fragment wrapped in /*MAIN_GRAPH_START*/ markers so it
// can be embedded in a JSON file. The C++ pipeline never used that marker form -- it
// emits the DataBlock directly.

namespace
{

// ---------------- Lookup tables (mainNodes.js:2304-2329, :27-51, :22-25) ----------------

const char *pin_type_to_blk_type(PinType t)
{
  switch (t)
  {
    case PinType::Bool: return "b";
    case PinType::Int: return "i";
    case PinType::Float: return "r";
    case PinType::Float2: return "p2";
    case PinType::Float3: return "p3";
    case PinType::Float4: return "p4";
    default: return "t"; // strings, textures, particles, custom types all encode as text
  }
}

// Direct param emission for the params{} loop. Avoids the loadText round-trip
// (BLK lexer + name-id allocation + setParamsFrom copy) that dominates the
// per-node hot path; on a 1944-node graph the params loop runs tens of thousands
// of times. Returns true when the param was set; false means the type didn't
// have a fast path (string/texture etc.) and the caller should fall back to
// loadText so BLK's quoting / escape rules still apply. Inputs `val` are
// substitute() outputs -- numeric types arrive as "1.0" / "1.0, 2.0" forms.
bool try_set_param_direct(DataBlock &blk, const char *name, PinType type, const char *val)
{
  if (!val)
  {
    val = "";
  }
  switch (type)
  {
    case PinType::Bool:
    {
      // BLK bool literals: yes/no/true/false/on/off/1/0 (case-insensitive).
      const char *p = val;
      while (*p == ' ' || *p == '\t')
      {
        ++p;
      }
      const bool b = (p[0] == 'y' || p[0] == 'Y' || p[0] == 't' || p[0] == 'T' || p[0] == '1' ||
                      ((p[0] == 'o' || p[0] == 'O') && (p[1] == 'n' || p[1] == 'N')));
      blk.setBool(name, b);
      return true;
    }
    case PinType::Int: blk.setInt(name, atoi(val)); return true;
    case PinType::Float: blk.setReal(name, static_cast<float>(atof(val))); return true;
    case PinType::Float2:
    {
      float x = 0.0f, y = 0.0f;
      if (sscanf(val, "%f , %f", &x, &y) != 2)
      {
        return false;
      }
      blk.setPoint2(name, Point2(x, y));
      return true;
    }
    case PinType::Float3:
    {
      float x = 0.0f, y = 0.0f, z = 0.0f;
      if (sscanf(val, "%f , %f , %f", &x, &y, &z) != 3)
      {
        return false;
      }
      blk.setPoint3(name, Point3(x, y, z));
      return true;
    }
    case PinType::Float4:
    {
      float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;
      if (sscanf(val, "%f , %f , %f , %f", &x, &y, &z, &w) != 4)
      {
        return false;
      }
      blk.setPoint4(name, Point4(x, y, z, w));
      return true;
    }
    default: return false; // strings, textures: keep BLK's quoting semantics
  }
}

// GE_defaultValuesZero (mainNodes.js:27-38). Values consumed by substitute() when an
// input pin is not connected and has no `data.def_val`.
const char *default_value_zero(PinType t)
{
  switch (t)
  {
    case PinType::Int: return "0";
    case PinType::Float: return "0.0";
    case PinType::Float2: return "0.0, 0.0";
    case PinType::Float3: return "0.0, 0.0, 0.0";
    case PinType::Float4: return "0.0, 0.0, 0.0, 0.0";
    case PinType::Texture2D: return "tex:const:0";
    default: return "0";
  }
}

// GE_defaultValuesOne (mainNodes.js:40-51).
const char *default_value_one(PinType t)
{
  switch (t)
  {
    case PinType::Int: return "1";
    case PinType::Float: return "1.0";
    case PinType::Float2: return "1.0, 1.0";
    case PinType::Float3: return "1.0, 1.0, 1.0";
    case PinType::Float4: return "1.0, 1.0, 1.0, 1.0";
    case PinType::Texture2D: return "tex:const:1";
    default: return "1";
  }
}

// GE_conversions (mainNodes.js:22-25, 2350-2360). The JS table has one row today:
// ["int", "float", "$"]. The userData "$" is a template where "$" is replaced
// with `expression`, so for every current rule the result equals `expression`
// unchanged. If a future row uses non-trivial userData (e.g. "int_to_float($)"),
// this needs to perform the substitution.
eastl::string convert_type([[maybe_unused]] PinType from, [[maybe_unused]] PinType to, const char *expression)
{
  return eastl::string(expression);
}

// ---------------- Hash helpers (mainNodes.js:2364, graphEditor.js:3738) ----------------

// 32-bit string hash from mainNodes.js:2364-2373. Used by elem_graph_hash for
// the %graph_hash% substitution token.
int32_t hash_code(const char *s, int32_t hash)
{
  if (!s)
  {
    return hash;
  }
  for (; *s; ++s)
  {
    hash = (hash << 5) - hash + static_cast<unsigned char>(*s);
  }
  return hash;
}

// DJB2-style 16-bit truncated hash from graphEditor.js:3738. Used to mangle
// shader_editor pin parameter names so they're unique BLK param keys.
uint32_t fast_hash(const char *s)
{
  uint32_t hash = 5381u;
  if (!s)
  {
    return hash;
  }
  for (; *s; ++s)
  {
    hash = ((hash * 33u) ^ static_cast<unsigned char>(*s)) & 0xFFFFu;
  }
  return hash;
}

// graphEditor.js:3749-3772. Replace any character not in [A-Za-z0-9_] with '_'.
// Length-preserving.
eastl::string make_safe_filename(const char *s)
{
  eastl::string out;
  if (!s)
  {
    return out;
  }
  for (; *s; ++s)
  {
    unsigned char c = static_cast<unsigned char>(*s);
    const bool ok = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    out.push_back(ok ? static_cast<char>(c) : '_');
  }
  return out;
}

// ---------------- Per-compile sidecar (mainNodes.js:2228-2281 et al) ----------------

// The JS mutates pin objects directly. In C++ we keep GraphData::Pin clean and stash
// the per-compile state here. Indexed (nodeIdx, pinIdx) parallel to GraphData::nodes[].pins.
struct PinFinalEntry
{
  int elem = 0;
  int pin = 0;
};

struct PinCompileState
{
  int connectElem = -1;
  int connectPin = -1;
  PinType connectFromType = PinType::Unknown;
  PinType connectToType = PinType::Unknown;
  bool connected = false;
  bool final = false;
  eastl::vector<PinFinalEntry> finalList;
  eastl::string customTextureName;

  // Set by propagate_external_stuff for downstream input pins: what the upstream
  // node has resolved as its propagatedStuff. Only meaningful for input pins.
  bool hasExternalStuff = false;
  int extWidth = 0;
  int extHeight = 0;
  int extDepth = 0;
  eastl::string extType;
  eastl::string extWrap;
};

struct NodeSidecar
{
  eastl::vector<PinCompileState> pins;

  // True when this node was eliminated by the bypass pass (descriptor `bypass:b = yes`
  // and matching upstream propagatedStuff). Consumer pins have been re-routed past this
  // node; emit + propagation must skip it. Mirrors JS `removeAllBypassNodes`
  // (graphEditor.js:3214) which is called pre-compile and physically splices these
  // nodes out of `editor.elems`/`editor.edges`.
  bool bypassed = false;

  // propagate_external_stuff result -- what THIS node produces. outTexDimension is
  // 0 for non-producing nodes (e.g. `result`), 1/2/3 for 1D/2D/3D texture / particles.
  bool hasPropagated = false;
  int outTexDimension = 0;
  int propWidth = 0;
  int propHeight = 0;
  int propDepth = 0;
  eastl::string propType;
  eastl::string propWrap;

  // Cached descriptor pointers. find_descriptor / find_pin_in_desc are linear
  // scans through base_nodes.blk and its pin sub-blocks; called millions of
  // times during the fixed-point propagation loop. Populated once at the start
  // of compile_graph_to_blks. nullptr means descriptor / pin descriptor missing.
  const DataBlock *descriptor = nullptr;
  eastl::vector<const DataBlock *> pinDescriptors;
  // Effective per-pin data block: per-instance Pin::data when non-empty,
  // otherwise descriptor pin's `data` sub-block. nullptr when neither exists.
  // This is what the compile actually reads for blk_code / customParam /
  // final / def_val / particleTexture / etc. Required for shader_editor
  // sub-graph nodes whose descriptor isn't in base_nodes.blk -- their data
  // comes only from the per-instance saved data.
  eastl::vector<const DataBlock *> pinDataPtrs;
};

using NodeStates = eastl::vector<NodeSidecar>;

// Sidecar access by (nodeIdx, pinIdx). Returns nullptr for out-of-range.
PinCompileState *get_pin_state(NodeStates &states, int node_idx, int pin_idx)
{
  if (node_idx < 0 || node_idx >= static_cast<int>(states.size()))
  {
    return nullptr;
  }
  NodeSidecar &n = states[node_idx];
  if (pin_idx < 0 || pin_idx >= static_cast<int>(n.pins.size()))
  {
    return nullptr;
  }
  return &n.pins[pin_idx];
}

const PinCompileState *get_pin_state(const NodeStates &states, int node_idx, int pin_idx)
{
  if (node_idx < 0 || node_idx >= static_cast<int>(states.size()))
  {
    return nullptr;
  }
  const NodeSidecar &n = states[node_idx];
  if (pin_idx < 0 || pin_idx >= static_cast<int>(n.pins.size()))
  {
    return nullptr;
  }
  return &n.pins[pin_idx];
}

// Edges in GraphData are keyed by Node::id, not the position in nodes[]. The JS uses
// the same indexed form (elemA / elemB are vector indices, since elems[] is sparse-but-
// indexed-by-id in the JS object model). We need a stable id -> index map so we can
// translate edges into sidecar indices.
struct NodeIndex
{
  eastl::hash_map<int, int> idToIdx;
  void build(const GraphData &g)
  {
    idToIdx.clear();
    for (int i = 0; i < static_cast<int>(g.nodes.size()); ++i)
    {
      idToIdx[g.nodes[i].id] = i;
    }
  }
  int find(int id) const
  {
    auto it = idToIdx.find(id);
    return it == idToIdx.end() ? -1 : it->second;
  }
};

// Descriptor lookup by stable templateUid. Lazy linear scan matches base_nodes.blk's
// existing access pattern (see resolve_node_pins). Bounded (~100 entries), runs once
// per compile per node.
const DataBlock *find_descriptor(const DataBlock &base_nodes_blk, const eastl::string &template_uid)
{
  if (template_uid.empty())
  {
    return nullptr;
  }
  for (uint32_t i = 0; i < base_nodes_blk.blockCount(); ++i)
  {
    const DataBlock *b = base_nodes_blk.getBlock(i);
    if (strcmp(b->getBlockName(), "node") != 0)
    {
      continue;
    }
    if (strcmp(b->getStr("templateUid", ""), template_uid.c_str()) == 0)
    {
      return b;
    }
  }
  return nullptr;
}

const DataBlock *find_pin_in_desc(const DataBlock *desc, const eastl::string &pin_name)
{
  if (!desc || pin_name.empty())
  {
    return nullptr;
  }
  for (uint32_t i = 0; i < desc->blockCount(); ++i)
  {
    const DataBlock *b = desc->getBlock(i);
    if (strcmp(b->getBlockName(), "pin") != 0)
    {
      continue;
    }
    if (strcmp(b->getStr("name", ""), pin_name.c_str()) == 0)
    {
      return b;
    }
  }
  return nullptr;
}

const DataBlock *find_pin_data(const DataBlock *desc_pin)
{
  if (!desc_pin)
  {
    return nullptr;
  }
  return desc_pin->getBlockByName("data");
}

// Property value lookup on a node instance. Returns empty string if missing.
eastl::string get_property(const GraphData::Node &n, const char *prop_name)
{
  if (!prop_name)
  {
    return eastl::string();
  }
  for (const auto &p : n.propertyValues)
  {
    if (p.first == prop_name)
    {
      return p.second;
    }
  }
  return eastl::string();
}

// ---------------- Connectivity pass (mainNodes.js:2228-2281) ----------------

void build_connectivity(const GraphData &g, const NodeIndex &index, NodeStates &states)
{
  states.clear();
  states.resize(g.nodes.size());
  for (int i = 0; i < static_cast<int>(g.nodes.size()); ++i)
  {
    states[i].pins.resize(g.nodes[i].pins.size());
  }

  for (const GraphData::Edge &edge : g.edges)
  {
    const int a = index.find(edge.elemA);
    const int b = index.find(edge.elemB);
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
    const GraphData::Pin &pa = na.pins[edge.pinA];
    const GraphData::Pin &pb = nb.pins[edge.pinB];

    PinCompileState *sa = get_pin_state(states, a, edge.pinA);
    PinCompileState *sb = get_pin_state(states, b, edge.pinB);

    // JS path 1 (mainNodes.js:2237-2258): pinA is an input. Wire sa -> b/pinB.
    if (pa.role == PinRole::In)
    {
      sa->connectElem = b;
      sa->connectPin = edge.pinB;
      sa->connectFromType = pb.types.empty() ? PinType::Unknown : pb.types.front();
      sa->connectToType = pa.types.empty() ? PinType::Unknown : pa.types.front();
      sa->connected = true;
      sb->connected = true;

      // data.final propagation: if the input pin's descriptor data has final:b=yes,
      // mark the upstream pin as final and record this as a finalList entry on it.
    }
    // JS path 2 (mainNodes.js:2259-2280): pinB is an input. Wire sb -> a/pinA.
    if (pb.role == PinRole::In)
    {
      sb->connectElem = a;
      sb->connectPin = edge.pinA;
      sb->connectFromType = pa.types.empty() ? PinType::Unknown : pa.types.front();
      sb->connectToType = pb.types.empty() ? PinType::Unknown : pb.types.front();
      sb->connected = true;
      sa->connected = true;
    }
  }
}

// Second connectivity pass: walk edges again and apply the data.final propagation,
// because final-tracking needs the *descriptor* data of the *input* pin (which lives
// in base_nodes.blk). Done as a separate pass for clarity.
void propagate_final(const GraphData &g, const NodeIndex &index, const DataBlock &base_nodes_blk, NodeStates &states)
{
  for (const GraphData::Edge &edge : g.edges)
  {
    const int a = index.find(edge.elemA);
    const int b = index.find(edge.elemB);
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

    auto mark_final = [&](int input_node, int input_pin_idx, int upstream_node, int upstream_pin_idx) {
      const DataBlock *descData = states[input_node].pinDataPtrs[input_pin_idx];
      if (!descData || !descData->getBool("final", false))
      {
        return;
      }
      PinCompileState *upstream = get_pin_state(states, upstream_node, upstream_pin_idx);
      if (!upstream)
      {
        return;
      }
      upstream->final = true;
      PinFinalEntry entry;
      entry.elem = input_node;
      entry.pin = input_pin_idx;
      upstream->finalList.push_back(entry);
    };

    if (na.pins[edge.pinA].role == PinRole::In)
    {
      mark_final(a, edge.pinA, b, edge.pinB);
    }
    if (nb.pins[edge.pinB].role == PinRole::In)
    {
      mark_final(b, edge.pinB, a, edge.pinA);
    }
  }
}

// ---------------- Substitute (mainNodes.js:2407-2468) ----------------

eastl::string elem_graph_hash_impl(int elem_idx, const GraphData &g, const NodeStates &states, eastl::hash_set<int> &visited,
  int32_t hash)
{
  if (elem_idx < 0 || elem_idx >= static_cast<int>(g.nodes.size()))
  {
    return eastl::string();
  }
  if (visited.count(elem_idx))
  {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", hash);
    return eastl::string(buf);
  }
  visited.insert(elem_idx);
  const GraphData::Node &n = g.nodes[elem_idx];
  // JS uses `hashCode(elem.descName + elem.uid, hash)` (mainNodes.js:2382). Our
  // port keeps only the int id, so the seed key differs from JS but is stable
  // within a graph -- `%graph_hash%` consumers only need uniqueness, not parity.
  // Use templateUid (not descName) so renaming a template doesn't shift the hash
  // of every graph that references it.
  eastl::string seed = n.templateUid;
  char idStr[32];
  snprintf(idStr, sizeof(idStr), "%d", n.id);
  seed.append(idStr);
  hash = hash_code(seed.c_str(), hash);
  for (const auto &pv : n.propertyValues)
  {
    eastl::string kv = pv.first;
    kv.append("=");
    kv.append(pv.second);
    hash = hash_code(kv.c_str(), hash);
  }
  for (int i = 0; i < static_cast<int>(n.pins.size()); ++i)
  {
    if (n.pins[i].role != PinRole::In)
    {
      continue;
    }
    const PinCompileState *st = get_pin_state(states, elem_idx, i);
    if (!st)
    {
      continue;
    }
    char buf[64];
    snprintf(buf, sizeof(buf), "%d", st->connectElem);
    hash = hash_code(buf, hash);
    if (st->connectElem >= 0)
    {
      snprintf(buf, sizeof(buf), "%d-%d", st->connectElem, st->connectPin);
      hash = hash_code(buf, hash);
      visited.insert(elem_idx); // re-insert defensively before recursing
      eastl::string rec = elem_graph_hash_impl(st->connectElem, g, states, visited, hash);
      // The JS sets `hash = getElemGraphHash(...)`, so accumulate by re-running through
      // the returned string. Here we just keep going with the new hash returned via
      // the recursive call by parsing it back. Simpler: re-seed hash from the string.
      hash = hash_code(rec.c_str(), 0);
    }
  }
  char buf[32];
  snprintf(buf, sizeof(buf), "%d", hash);
  return eastl::string(buf);
}

void replace_all(eastl::string &s, const eastl::string &from, const eastl::string &to)
{
  if (from.empty())
  {
    return;
  }
  size_t pos = 0;
  while ((pos = s.find(from, pos)) != eastl::string::npos)
  {
    s.replace(pos, from.size(), to);
    pos += to.size();
  }
}

// substitute($pin$, %prop%) the JS way: replace by pin name first, then property name.
// For each pin: if connected, the replacement is the upstream output's register name
// (customTextureName if assigned, else `_t_<elem>_<pin>`), passed through convert_type.
// If not connected: data.def_val on the descriptor pin, or default-zero/-one based on
// data.def_1 flag.
eastl::string substitute(const char *code, int elem_idx, const GraphData &g, const NodeStates &states, const DataBlock &base_nodes_blk)
{
  if (!code)
  {
    return eastl::string();
  }
  eastl::string out(code);
  if (elem_idx < 0 || elem_idx >= static_cast<int>(g.nodes.size()))
  {
    return out;
  }
  const GraphData::Node &elem = g.nodes[elem_idx];

  for (int i = 0; i < static_cast<int>(elem.pins.size()); ++i)
  {
    const GraphData::Pin &p = elem.pins[i];
    eastl::string fromKey = "$";
    fromKey.append(p.name);
    fromKey.append("$");
    if (out.find(fromKey) == eastl::string::npos)
    {
      continue;
    }

    eastl::string toVal;
    const PinCompileState *st = get_pin_state(states, elem_idx, i);
    if (st && st->connectFromType != PinType::Unknown && st->connectElem >= 0)
    {
      const PinCompileState *up = get_pin_state(states, st->connectElem, st->connectPin);
      eastl::string upstreamName;
      if (up && !up->customTextureName.empty())
      {
        upstreamName = up->customTextureName;
      }
      else
      {
        char buf[64];
        snprintf(buf, sizeof(buf), "_t_%d_%d", st->connectElem, st->connectPin);
        upstreamName = buf;
      }
      toVal = convert_type(st->connectFromType, st->connectToType, upstreamName.c_str());
    }
    else
    {
      const DataBlock *descData = states[elem_idx].pinDataPtrs[i];
      const char *defVal = descData ? descData->getStr("def_val", nullptr) : nullptr;
      if (defVal && *defVal)
      {
        toVal = defVal;
      }
      else
      {
        const PinType t = p.types.empty() ? p.type : p.types.front();
        const bool useOne = descData && descData->getBool("def_1", false);
        toVal = useOne ? default_value_one(t) : default_value_zero(t);
      }
    }
    replace_all(out, fromKey, toVal);
  }

  for (const auto &pv : elem.propertyValues)
  {
    eastl::string fromKey = "%";
    fromKey.append(pv.first);
    fromKey.append("%");
    if (out.find(fromKey) == eastl::string::npos)
    {
      continue;
    }
    eastl::string toVal = pv.second;
    if (toVal == "%this_elem_id%")
    {
      // JS uses `elem.uid + "-" + width + "-" + height + "-" + type`
      // (mainNodes.js:2456); our port substitutes int id for uid.
      char buf[128];
      const NodeSidecar &s = states[elem_idx];
      snprintf(buf, sizeof(buf), "%d-%d-%d-%s", elem.id, s.propWidth, s.propHeight, s.propType.c_str());
      toVal = buf;
    }
    else if (toVal == "%graph_hash%")
    {
      eastl::hash_set<int> visited;
      toVal = elem_graph_hash_impl(elem_idx, g, states, visited, 0);
    }
    replace_all(out, fromKey, toVal);
  }

  return out;
}

// ---------------- Propagation pass (mainNodes.js:3023-3202 + graphEditor.js:1588-1648) ----------------
// The JS attaches GE_propagateExternalStuff to each descriptor; the driver in
// graphEditor.js calls it per-node and walks edges to push results into
// downstream externalStuffOnPins. Iterate to fixed point. Here we collapse all
// of that into a single C++ helper since the descriptor-attachment indirection
// adds no value once we're not driving a JS prototype chain.

// Reads "= N" / "x N" / "/ N" style prefixed-int property values. Returns true
// and `out_n` on a match; false for tokens like "parent size" / "graph size" /
// "width" / "parent type" etc. The JS uses parseInt(value.slice(1), 10) which
// ignores leading whitespace -- mirror that here.
bool parse_prefixed_int(const eastl::string &value, char prefix, int &out_n)
{
  if (value.empty() || value[0] != prefix)
  {
    return false;
  }
  const char *p = value.c_str() + 1;
  while (*p == ' ' || *p == '\t')
  {
    ++p;
  }
  if (*p == 0)
  {
    return false;
  }
  char *end = nullptr;
  long n = strtol(p, &end, 10);
  if (end == p)
  {
    return false;
  }
  out_n = static_cast<int>(n);
  return true;
}

int clamp_dim(int v) { return eastl::max(eastl::min(v, 8192), 1); }

// Returns true when this call succeeded in resolving propagatedStuff for `elem_idx`.
// Returns false if the node is not a texture producer (no out-pin with texture or
// particles type), or if it's waiting for upstream propagation. Idempotent: a node
// already propagated is left alone and returns false (so the fixed-point driver
// doesn't count it as "changed").
bool propagate_external_stuff(int elem_idx, const GraphData &g, const DataBlock &base_nodes_blk, NodeStates &states)
{
  NodeSidecar &sc = states[elem_idx];
  if (sc.hasPropagated)
  {
    return false;
  }

  const GraphData::Node &e = g.nodes[elem_idx];

  // Find the "main" connected input pin (descriptor pin with main:b=yes), falling
  // back to the first connected texture-typed in-pin. Mirrors mainNodes.js:3042-3068.
  int mainInputIndex = -1;
  int outTexDimension = 0;
  for (int i = 0; i < static_cast<int>(e.pins.size()); ++i)
  {
    const GraphData::Pin &p = e.pins[i];
    const PinCompileState *pst = get_pin_state(states, elem_idx, i);
    if (p.role == PinRole::In)
    {
      const DataBlock *descPin = states[elem_idx].pinDescriptors[i];
      const bool isMain = descPin && descPin->getBool("main", false);
      const bool isTexFirstType =
        !p.types.empty() &&
        (p.types.front() == PinType::Texture1D || p.types.front() == PinType::Texture2D || p.types.front() == PinType::Texture3D ||
          p.types.front() == PinType::Texture2DArray || p.types.front() == PinType::Texture2DShdArray);
      if (isMain && pst && pst->connected)
      {
        mainInputIndex = i;
      }
      if (mainInputIndex == -1 && pst && pst->connected && isTexFirstType)
      {
        mainInputIndex = i;
      }
    }
    else if (p.role == PinRole::Out)
    {
      if (!p.types.empty())
      {
        const PinType t = p.types.front();
        if (t == PinType::Particles)
        {
          outTexDimension = 2;
        }
        else if (t == PinType::Texture1D)
        {
          outTexDimension = 1;
        }
        else if (t == PinType::Texture2D)
        {
          outTexDimension = 2;
        }
        else if (t == PinType::Texture3D)
        {
          outTexDimension = 3;
        }
      }
    }
  }

  if (outTexDimension == 0)
  {
    return false;
  }

  // Waiting for upstream propagation: the main-input pin is connected but no upstream
  // node has filled externalStuffOnPins[mainInputIndex] yet.
  if (mainInputIndex != -1)
  {
    const PinCompileState *mst = get_pin_state(states, elem_idx, mainInputIndex);
    if (mst && mst->connected && !mst->hasExternalStuff)
    {
      return false;
    }
  }

  // Property string accessors with the JS-side fallback defaults.
  auto prop = [&](const char *name, const char *fallback) -> eastl::string {
    eastl::string v = get_property(e, name);
    return v.empty() ? eastl::string(fallback) : v;
  };
  eastl::string w = prop("texture width", "parent size");
  eastl::string h = prop("texture height", "width");
  eastl::string d = prop("texture depth", "width");
  eastl::string t = prop("texture type", "parent type");
  eastl::string a = prop("texture wrap", "parent wrap");

  int width = g.graphTextureWidth;
  int height = g.graphTextureHeight;
  int depth = g.graphTextureDepth;
  eastl::string type = g.graphTextureType;
  eastl::string wrap = g.graphTextureWrap;

  const PinCompileState *mst = mainInputIndex >= 0 ? get_pin_state(states, elem_idx, mainInputIndex) : nullptr;
  const bool fromExternal = (mst && mst->hasExternalStuff);

  if (!fromExternal)
  {
    int parsed = 0;
    if (parse_prefixed_int(w, '=', parsed))
    {
      width = parsed;
    }
    if (parse_prefixed_int(h, '=', parsed))
    {
      height = parsed;
    }
    if (parse_prefixed_int(d, '=', parsed))
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
    width = mst->extWidth;
    height = mst->extHeight;
    depth = mst->extDepth;
    type = mst->extType;
    wrap = mst->extWrap;
    int parsed = 0;
    if (parse_prefixed_int(w, 'x', parsed))
    {
      width *= parsed;
    }
    if (parse_prefixed_int(h, 'x', parsed))
    {
      height *= parsed;
    }
    if (parse_prefixed_int(d, 'x', parsed))
    {
      depth *= parsed;
    }
    if (parse_prefixed_int(w, '/', parsed) && parsed != 0)
    {
      width /= parsed;
    }
    if (parse_prefixed_int(h, '/', parsed) && parsed != 0)
    {
      height /= parsed;
    }
    if (parse_prefixed_int(d, '/', parsed) && parsed != 0)
    {
      depth /= parsed;
    }
    if (parse_prefixed_int(w, '=', parsed))
    {
      width = parsed;
    }
    if (parse_prefixed_int(h, '=', parsed))
    {
      height = parsed;
    }
    if (parse_prefixed_int(d, '=', parsed))
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

  sc.hasPropagated = true;
  sc.outTexDimension = outTexDimension;
  sc.propWidth = width;
  sc.propHeight = height;
  sc.propDepth = depth;
  sc.propType = type;
  sc.propWrap = wrap;
  return true;
}

// Walks every edge once, pushing the upstream node's propagatedStuff into the
// downstream input pin's externalStuffOnPins fields. Mirrors graphEditor.js:1636-1648.
// Returns true if any pin was newly populated.
bool propagate_edges(const GraphData &g, const NodeIndex &index, NodeStates &states)
{
  bool changed = false;
  for (const GraphData::Edge &edge : g.edges)
  {
    const int a = index.find(edge.elemA);
    const int b = index.find(edge.elemB);
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

    auto push = [&](int from_node, int to_node, int to_pin) {
      if (!states[from_node].hasPropagated)
      {
        return;
      }
      PinCompileState *toSt = get_pin_state(states, to_node, to_pin);
      if (!toSt || toSt->hasExternalStuff)
      {
        return;
      }
      toSt->hasExternalStuff = true;
      toSt->extWidth = states[from_node].propWidth;
      toSt->extHeight = states[from_node].propHeight;
      toSt->extDepth = states[from_node].propDepth;
      toSt->extType = states[from_node].propType;
      toSt->extWrap = states[from_node].propWrap;
      changed = true;
    };

    // Edge direction: whichever side is an out-pin is the upstream. The JS check is
    // `from.desc.pins[c.pinFrom].role === "out"`; mirror by role.
    if (na.pins[edge.pinA].role == PinRole::Out && nb.pins[edge.pinB].role == PinRole::In)
    {
      push(a, b, edge.pinB);
    }
    if (nb.pins[edge.pinB].role == PinRole::Out && na.pins[edge.pinA].role == PinRole::In)
    {
      push(b, a, edge.pinA);
    }
  }
  return changed;
}

// Splice out bypass nodes from the compile view, re-routing each consumer pin past the
// bypass node directly to its upstream source. Mirrors JS `removeAllBypassNodes` +
// `deleteBypassKeepConnections_` (graphEditor.js:3100-3230), which run in the editor
// *before* `generateHlslCode`. We can't mutate `GraphData` here (the editor needs the
// nodes for display), so the bypass state lives on `NodeSidecar.bypassed` and we adjust
// the cached `PinCompileState.connectElem/connectPin` of every downstream consumer.
//
// The JS guard: if the bypass node's propagatedStuff differs from the source's, leave it
// in place (removing would silently change the data type the downstream sees). For the
// `pass`/unary descriptor those should match by construction; the guard is mostly
// defensive against weird configurations.
void apply_bypass_removal(const GraphData &g, const DataBlock &base_nodes_blk, NodeStates &states)
{
  for (int b = 0; b < static_cast<int>(g.nodes.size()); ++b)
  {
    const DataBlock *desc = states[b].descriptor;
    if (!desc || !desc->getBool("bypass", false))
    {
      continue;
    }

    // Find the single in-pin's source. JS `pass` descriptor has exactly one in-pin.
    int srcElem = -1;
    int srcPin = -1;
    PinType srcType = PinType::Unknown;
    for (int j = 0; j < static_cast<int>(g.nodes[b].pins.size()); ++j)
    {
      if (g.nodes[b].pins[j].role != PinRole::In)
      {
        continue;
      }
      const PinCompileState &s = states[b].pins[j];
      if (s.connected && s.connectElem >= 0)
      {
        srcElem = s.connectElem;
        srcPin = s.connectPin;
        srcType = s.connectFromType;
        break;
      }
    }
    if (srcElem < 0)
    {
      continue; // floating input -- leave alone (JS does too: connectedIn === -1)
    }

    // Type-compat check: bypass's propagatedStuff vs source's. JS uses
    // `JSON.stringify(propagatedStuff)` equality; we compare the same fields.
    const NodeSidecar &sb = states[b];
    const NodeSidecar &ss = states[srcElem];
    if (sb.hasPropagated && ss.hasPropagated)
    {
      if (sb.propWidth != ss.propWidth || sb.propHeight != ss.propHeight || sb.propDepth != ss.propDepth ||
          sb.propType != ss.propType || sb.propWrap != ss.propWrap)
        continue;
    }

    // Re-route every consumer pin reading from this bypass to the upstream source.
    bool hadConsumer = false;
    for (int c = 0; c < static_cast<int>(g.nodes.size()); ++c)
    {
      for (int j = 0; j < static_cast<int>(states[c].pins.size()); ++j)
      {
        PinCompileState &s = states[c].pins[j];
        if (!s.connected || s.connectElem != b)
        {
          continue;
        }
        s.connectElem = srcElem;
        s.connectPin = srcPin;
        s.connectFromType = srcType;
        hadConsumer = true;
      }
    }
    if (!hadConsumer)
    {
      continue; // JS: connectedOut === -1, leave alone
    }

    // Transfer final + finalList from the bypass's out-pin to the upstream source's
    // out-pin. propagate_final ran against the original edges, so any downstream
    // `final` (e.g. "export texture" input) tagged the *bypass's* out-pin, not the
    // upstream's. Without this, the upstream emits `_t_<idx>_<j>` instead of the
    // export filename. JS doesn't need this step -- it physically splices the bypass
    // out of `editor.elems` before `propagateTypes`/compile, so propagate_final runs
    // on the already-collapsed graph.
    PinCompileState *srcOut = get_pin_state(states, srcElem, srcPin);
    if (srcOut)
    {
      for (int j = 0; j < static_cast<int>(states[b].pins.size()); ++j)
      {
        PinCompileState &bypassOut = states[b].pins[j];
        if (!bypassOut.final)
        {
          continue;
        }
        srcOut->final = true;
        for (const PinFinalEntry &fe : bypassOut.finalList)
        {
          srcOut->finalList.push_back(fe);
        }
      }
    }

    states[b].bypassed = true;
  }
}

// ---------------- Texture parameter helpers ----------------

int texture_width(int elem_idx, const GraphData &g, const NodeStates &states)
{
  if (elem_idx >= 0 && elem_idx < static_cast<int>(states.size()) && states[elem_idx].hasPropagated)
  {
    return states[elem_idx].propWidth;
  }
  return g.graphTextureWidth;
}

int texture_height(int elem_idx, const GraphData &g, const NodeStates &states)
{
  if (elem_idx >= 0 && elem_idx < static_cast<int>(states.size()) && states[elem_idx].hasPropagated)
  {
    return states[elem_idx].propHeight;
  }
  return g.graphTextureHeight;
}

const char *texture_type(int elem_idx, const GraphData &g, const NodeStates &states)
{
  if (elem_idx >= 0 && elem_idx < static_cast<int>(states.size()) && states[elem_idx].hasPropagated)
  {
    return states[elem_idx].propType.c_str();
  }
  return g.graphTextureType.c_str();
}

const char *texture_wrap(int elem_idx, const GraphData &g, const NodeStates &states)
{
  if (elem_idx >= 0 && elem_idx < static_cast<int>(states.size()) && states[elem_idx].hasPropagated)
  {
    return states[elem_idx].propWrap.c_str();
  }
  return g.graphTextureWrap.c_str();
}

// ---------------- Per-node emission ----------------

// makeInputName (mainNodes.js:2501-2527). For an output pin used as an input register,
// look up special property-driven naming patterns; otherwise return _t_<elem>_<pin>.
eastl::string make_input_name(int elem_idx, int pin_idx, const GraphData &g, const NodeStates &states, const DataBlock &base_nodes_blk)
{
  const GraphData::Node &n = g.nodes[elem_idx];
  const GraphData::Pin &p = n.pins[pin_idx];
  (void)p;
  const DataBlock *descData = states[elem_idx].pinDataPtrs[pin_idx];
  if (descData)
  {
    if (descData->getBool("isTextureName", false))
    {
      return eastl::string("tex:") + get_property(n, "texture name");
    }
    if (descData->getBool("isGradient", false))
    {
      return eastl::string("tex:gradient:") + get_property(n, "gradient");
    }
    if (descData->getBool("isCurve", false))
    {
      return eastl::string("tex:curve:") + get_property(n, "curve");
    }
    if (descData->getBool("isColor", false))
    {
      return eastl::string("tex:color:") + get_property(n, "color");
    }
    if (descData->getBool("isRawTextureName", false))
    {
      // The combobox "texture width"/"texture height" values carry a leading "= "
      // expression prefix that JS strips with .slice(2) (mainNodes.js:2515-2516); the
      // remaining props are used verbatim.
      eastl::string w = get_property(n, "texture width");
      eastl::string h = get_property(n, "texture height");
      if (w.size() >= 2)
      {
        w.erase(0, 2);
      }
      if (h.size() >= 2)
      {
        h.erase(0, 2);
      }
      eastl::string res("tex:");
      res += "w:i=";
      res += w;
      res += ";h:i=";
      res += h;
      res += ";offset:i=";
      res += get_property(n, "header (bytes)");
      res += ";fmt:t=\"";
      res += get_property(n, "texture type");
      res += "\";ch_order:t=\"";
      res += get_property(n, "channels order");
      res += "\";name:t=";
      res += get_property(n, "texture name");
      return res;
    }
  }
  const PinCompileState *st = get_pin_state(states, elem_idx, pin_idx);
  if (st && !st->customTextureName.empty())
  {
    return st->customTextureName;
  }
  char buf[64];
  snprintf(buf, sizeof(buf), "_t_%d_%d", elem_idx, pin_idx);
  return eastl::string(buf);
}

// `final_buf` accumulates final:t / selected:t / skip_saving:t entries to be flushed
// onto `out` after the topo loop, matching the JS finalCode pattern (L2865).
struct FinalAccumulator
{
  eastl::vector<eastl::string> finals;
  eastl::vector<eastl::string> skipSavings;
  eastl::vector<eastl::string> selected;
  // Counter for the "_t_0_temporary_reg_name_<N>" placeholder names used on a
  // final-name collision. Lives on the accumulator (one per compile call) to
  // match JS `var uniq_cnt = 0` reset-per-call semantics at mainNodes.js:2342 --
  // a `static int` here drifts the suffix across repeated compiles of the same
  // graph and breaks JS-vs-C++ output parity.
  int dupCounter = 0;
};

// Per-pass override values for the multipass path (mainNodes.js:2819-2852). The
// JS does this with `[[FIRST_PASS_INPUT_NAME]]` / `[[LAST_PASS_OUTPUT_NAME]]` /
// `[[BLK_PARAM_n]]` string-template surgery on the emitted text; we emit the BLK
// directly so we feed the resolved values straight into emission instead.
struct PassOverrides
{
  const char *blockNameSuffix = "";      // "" / "_1st_pass" / "_2nd_pass"
  bool useSecondPassParamValues = false; // pick data.second_pass_blk_code over data.blk_code
  int multipassInputPinIdx = -1;         // pin with data.second_pass_in (override its input.reg)
  eastl::string multipassInputReg;       // override reg name for that pin
  eastl::string multipassInputWrap;      // override wrap for that pin (clamp/wrap)
  bool overrideOutputReg = false;        // when true, all output{} blocks use outputRegOverride
  eastl::string outputRegOverride;       // temp reg used for first-pass outputs
  bool trackFinalsAndCustomName = true;  // single pass: yes. multipass: only the 2nd pass tracks.
};

// Particle bypass forward declaration (defined later in the file). Emits one
// `_bypass_<id>_<elem>_<pin>` block per connected pin with data.particleTexture.
void emit_particle_bypass(int elem_idx, int pin_idx, const GraphData &g, const NodeStates &states, const DataBlock &base_nodes_blk,
  DataBlock &out);

// Single-pass per-node emission. The dispatcher (emit_node_block) calls this once
// for non-multipass nodes and twice with different overrides for multipass nodes.
void emit_pass_block(int elem_idx, const GraphData &g, NodeStates &states, const DataBlock &base_nodes_blk, DataBlock &out,
  FinalAccumulator &finals, eastl::hash_set<eastl::string> &uniq_finals, const PassOverrides &cfg)
{
  const GraphData::Node &elem = g.nodes[elem_idx];
  const DataBlock *desc = states[elem_idx].descriptor;

  const bool isShaderEditor = (elem.plugin == "shader_editor");
  const char *nativeShader = desc ? desc->getStr("nativeShader", nullptr) : nullptr;

  // JS uses `_node_<uid>_<i>` (mainNodes.js:2615); our port substitutes int id.
  char nodeNameBuf[96];
  snprintf(nodeNameBuf, sizeof(nodeNameBuf), "_node_%d_%d%s", elem.id, elem_idx, cfg.blockNameSuffix);
  DataBlock *nb = out.addNewBlock(nodeNameBuf);

  // blkPreCode pins (mainNodes.js:2620-2623). Re-parse the substituted text as
  // BLK params and merge into the node block.
  for (int j = 0; j < static_cast<int>(elem.pins.size()); ++j)
  {
    const GraphData::Pin &p = elem.pins[j];
    (void)p;
    const PinCompileState *st = get_pin_state(states, elem_idx, j);
    if (!st || !st->connected)
    {
      continue;
    }
    const DataBlock *descData = states[elem_idx].pinDataPtrs[j];
    if (!descData)
    {
      continue;
    }
    const char *preCode = descData->getStr("blkPreCode", nullptr);
    if (!preCode || !*preCode)
    {
      continue;
    }
    eastl::string substituted = substitute(preCode, elem_idx, g, states, base_nodes_blk);
    DataBlock preBlock;
    if (preBlock.loadText(substituted.c_str(), static_cast<int>(substituted.size())))
    {
      nb->appendParamsFrom(&preBlock);
    }
  }

  // shader:t = ...
  if (isShaderEditor)
  {
    nb->setStr("shader", elem.descName.c_str());
  }
  else
  {
    eastl::string substituted = substitute(nativeShader, elem_idx, g, states, base_nodes_blk);
    nb->setStr("shader", substituted.c_str());
  }

  // params {} block.
  DataBlock *paramsBlk = nb->addNewBlock("params");
  for (int j = 0; j < static_cast<int>(elem.pins.size()); ++j)
  {
    const GraphData::Pin &p = elem.pins[j];
    const PinCompileState *st = get_pin_state(states, elem_idx, j);
    const DataBlock *descData = states[elem_idx].pinDataPtrs[j];
    if (!descData)
    {
      continue;
    }

    const char *customParam = descData->getStr("customParam", nullptr);
    if (customParam && *customParam)
    {
      eastl::string substituted = substitute(customParam, elem_idx, g, states, base_nodes_blk);
      DataBlock tmp;
      if (tmp.loadText(substituted.c_str(), static_cast<int>(substituted.size())))
      {
        paramsBlk->appendParamsFrom(&tmp);
      }
    }

    if (p.role != PinRole::In)
    {
      continue;
    }
    const char *blkCode = descData->getStr("blk_code", nullptr);
    if (!blkCode || !*blkCode)
    {
      continue;
    }

    eastl::string paramName = p.name.c_str();
    if (isShaderEditor)
    {
      eastl::string safe = make_safe_filename(p.name.c_str());
      char hashBuf[32];
      snprintf(hashBuf, sizeof(hashBuf), "%u", fast_hash(p.name.c_str()));
      paramName = eastl::string("_") + safe + "_" + hashBuf;
    }

    // Pick the pass-appropriate value: when this pin has data.second_pass_blk_code
    // AND we're emitting the second pass, use that; otherwise fall through to the
    // existing first-pass / single-pass path.
    eastl::string setVal;
    const char *secondPassBlkCode = descData->getStr("second_pass_blk_code", nullptr);
    if (cfg.useSecondPassParamValues && secondPassBlkCode && *secondPassBlkCode)
    {
      setVal = substitute(secondPassBlkCode, elem_idx, g, states, base_nodes_blk);
    }
    else if (st && st->connected && st->connectElem >= 0)
    {
      const DataBlock *upDescData = states[st->connectElem].pinDataPtrs[st->connectPin];
      const char *upBlkCode = upDescData ? upDescData->getStr("blk_code", nullptr) : nullptr;
      if (upBlkCode && *upBlkCode)
      {
        setVal = substitute(upBlkCode, st->connectElem, g, states, base_nodes_blk);
      }
      else
      {
        setVal = substitute(blkCode, elem_idx, g, states, base_nodes_blk);
      }
    }
    else
    {
      setVal = substitute(blkCode, elem_idx, g, states, base_nodes_blk);
    }

    const PinType blkPinType = p.types.empty() ? p.type : p.types.front();
    if (try_set_param_direct(*paramsBlk, paramName.c_str(), blkPinType, setVal.c_str()))
    {
      continue;
    }

    // Fallback: string/texture or numeric we couldn't parse -- run it through the
    // BLK lexer so quoting and escapes behave like the JS-baseline output.
    eastl::string line = paramName;
    line.append(":");
    line.append(pin_type_to_blk_type(blkPinType));
    line.append("=");
    line.append(setVal);
    DataBlock tmp;
    if (tmp.loadText(line.c_str(), static_cast<int>(line.size())))
    {
      paramsBlk->appendParamsFrom(&tmp);
    }
  }

  // input {} blocks.
  for (int j = 0; j < static_cast<int>(elem.pins.size()); ++j)
  {
    const GraphData::Pin &p = elem.pins[j];
    if (p.role != PinRole::In)
    {
      continue;
    }
    const PinCompileState *st = get_pin_state(states, elem_idx, j);
    const DataBlock *descData = states[elem_idx].pinDataPtrs[j];

    // Multipass second_pass_in override (mainNodes.js:2701-2706): the input reg
    // for this pin is parameterised by the pass.
    if (j == cfg.multipassInputPinIdx)
    {
      DataBlock *inb = nb->addNewBlock("input");
      inb->setStr("reg", cfg.multipassInputReg.c_str());
      inb->setStr("wrap_mode", cfg.multipassInputWrap.c_str());
      continue;
    }

    eastl::string inputName;
    const char *wrapMode = "clamp";
    const bool hasTexture2D = [&]() {
      for (PinType t : p.types)
      {
        if (t == PinType::Texture2D)
        {
          return true;
        }
      }
      return false;
    }();
    if (st && st->connected && st->connectToType == PinType::Texture2D)
    {
      inputName = make_input_name(st->connectElem, st->connectPin, g, states, base_nodes_blk);
      // Match JS at mainNodes.js:2690 -- input wrap mode comes from the upstream
      // node (not this one). connectElem is already the sidecar index.
      wrapMode = texture_wrap(st->connectElem, g, states);
    }
    else if (st && !st->connected && hasTexture2D)
    {
      eastl::string placeholder = "$";
      placeholder.append(p.name);
      placeholder.append("$");
      inputName = substitute(placeholder.c_str(), elem_idx, g, states, base_nodes_blk);
    }

    // Particle bypass reroute (mainNodes.js:2687-2688): when the descriptor pin
    // has data.particleTexture set, the main block's input reads from the bypass
    // output register (_p_<elem>_<pin>) instead of the upstream texture.
    if (descData && descData->getBool("particleTexture", false))
    {
      char buf[64];
      snprintf(buf, sizeof(buf), "_p_%d_%d", elem_idx, j);
      inputName = buf;
    }

    if (inputName.empty())
    {
      continue;
    }
    DataBlock *inb = nb->addNewBlock("input");
    inb->setStr("reg", inputName.c_str());
    inb->setStr("wrap_mode", wrapMode);
  }

  // output {} blocks. Also assigns customTextureName, drives final/selected/skip_saving.
  for (int j = 0; j < static_cast<int>(elem.pins.size()); ++j)
  {
    const GraphData::Pin &p = elem.pins[j];
    if (p.role != PinRole::Out)
    {
      continue;
    }
    bool isParticles = false;
    bool isTexture = false;
    for (PinType t : p.types)
    {
      if (t == PinType::Particles)
      {
        isParticles = true;
      }
      if (t == PinType::Texture2D)
      {
        isTexture = true;
      }
    }
    if (!isParticles && !isTexture)
    {
      continue;
    }

    PinCompileState *st = get_pin_state(states, elem_idx, j);
    if (!st)
    {
      continue;
    }
    const DataBlock *descData = states[elem_idx].pinDataPtrs[j];
    const bool descFinal = descData && descData->getBool("final", false);
    const bool final = st->final || descFinal;

    char defaultOutBuf[64];
    snprintf(defaultOutBuf, sizeof(defaultOutBuf), "_t_%d_%d", elem_idx, j);
    eastl::string outName = defaultOutBuf;

    if (cfg.trackFinalsAndCustomName && st->final)
    {
      eastl::vector<eastl::string> outNames;
      for (const PinFinalEntry &fe : st->finalList)
      {
        const GraphData::Node &finalNode = g.nodes[fe.elem];
        eastl::string name = substitute("%texture name%", fe.elem, g, states, base_nodes_blk);
        while (!name.empty() && (name.back() == ' ' || name.back() == '\t' || name.back() == '\n'))
        {
          name.pop_back();
        }
        eastl::string skipSaving = get_property(finalNode, "skip when exporting");
        if (skipSaving == "true")
        {
          finals.skipSavings.push_back(name);
        }

        eastl::string fileFormat = substitute("%file format%", fe.elem, g, states, base_nodes_blk);
        eastl::string channels = substitute("%channels%", fe.elem, g, states, base_nodes_blk);
        if (name.find('%') != 0 && channels.find('%') != 0)
        {
          eastl::string exported = "export: ";
          exported.append(name);
          exported.append(fileFormat);
          exported.append("; ");
          exported.append(channels);
          name = exported;
        }

        // JS mainNodes.js:2746-2750: when `%texture name%` didn't substitute (the
        // `export entities` descriptor has no `texture name` property), fall back
        // to `<blk name>:<names list>` with all whitespace stripped. The
        // export-prefix branch above leaves `name` unchanged in this case because
        // its guard requires `name` to not start with '%'.
        if (name.find('%') == 0)
        {
          name = substitute("%blk name%:%names list%", fe.elem, g, states, base_nodes_blk);
          while (!name.empty() && (name.front() == ' ' || name.front() == '\t' || name.front() == '\n'))
          {
            name.erase(name.begin());
          }
          while (!name.empty() && (name.back() == ' ' || name.back() == '\t' || name.back() == '\n'))
          {
            name.pop_back();
          }
          eastl::string stripped;
          stripped.reserve(name.size());
          for (char c : name)
          {
            if (c != ' ')
            {
              stripped.push_back(c);
            }
          }
          name = stripped;
        }

        if (uniq_finals.count(name))
        {
          char dupBuf[64];
          snprintf(dupBuf, sizeof(dupBuf), "_t_0_temporary_reg_name_%d", ++finals.dupCounter);
          name = dupBuf;
        }
        else
        {
          uniq_finals.insert(name);
        }
        if (!name.empty())
        {
          outNames.push_back(name);
        }
      }
      if (!outNames.empty())
      {
        outName.clear();
        for (size_t k = 0; k < outNames.size(); ++k)
        {
          if (k > 0)
          {
            outName.append("$delimiter$");
          }
          outName.append(outNames[k]);
        }
      }
    }
    else if (!cfg.trackFinalsAndCustomName)
    {
      // For the first multipass pass we still need outName to feed downstream
      // pins via customTextureName (already cached from the second pass when
      // emit_node_block runs the second pass after the first). Read the cached
      // value here so the emitted output reg matches.
      if (!st->customTextureName.empty())
      {
        outName = st->customTextureName;
      }
    }

    if (cfg.trackFinalsAndCustomName)
    {
      st->customTextureName = outName;
      const bool isTemporary = (outName.find("_t_0_temporary_reg_name_") == 0);
      if (final && !isTemporary)
      {
        finals.finals.push_back(outName);
      }
    }

    DataBlock *outb = nb->addNewBlock("output");
    if (cfg.overrideOutputReg)
    {
      outb->setStr("reg", cfg.outputRegOverride.c_str());
    }
    else
    {
      outb->setStr("reg", outName.c_str());
    }
    outb->setStr("wrap_mode", texture_wrap(elem_idx, g, states));
    outb->setStr("fmt", isParticles ? "particles" : texture_type(elem_idx, g, states));
    if (descData && descData->getBool("is_uav", false))
    {
      outb->setBool("is_uav", true);
    }
  }

  // shaderParticleInput + customBlkParams (mainNodes.js:2802-2810). Both come from pin
  // data flags scanned across the elem's pins. shader_particle_input drives a fixed
  // template; customBlkParams is itself a substitutable template string.
  bool shaderParticleInput = false;
  const char *customBlkParams = nullptr;
  for (int j = 0; j < static_cast<int>(elem.pins.size()); ++j)
  {
    const PinCompileState *st = get_pin_state(states, elem_idx, j);
    if (!st || !st->connected)
    {
      continue;
    }
    const DataBlock *descData = states[elem_idx].pinDataPtrs[j];
    if (!descData)
    {
      continue;
    }
    if (elem.pins[j].role == PinRole::In && descData->getBool("shader_particle_input", false))
    {
      shaderParticleInput = true;
    }
    const char *cbp = descData->getStr("customBlkParams", nullptr);
    if (cbp && *cbp)
    {
      customBlkParams = cbp;
    }
  }

  auto mergeText = [&](const char *text) {
    DataBlock tmp;
    if (!tmp.loadText(text, static_cast<int>(strlen(text))))
    {
      return;
    }
    nb->appendParamsFrom(&tmp);
    for (uint32_t k = 0; k < tmp.blockCount(); ++k)
    {
      const DataBlock *src = tmp.getBlock(k);
      nb->addNewBlock(src, src->getBlockName());
    }
  };

  if (shaderParticleInput)
  {
    eastl::string substituted =
      substitute("particles{reg:t=\"$particles$\";} blending:t=\"%blend type%\";", elem_idx, g, states, base_nodes_blk);
    mergeText(substituted.c_str());
  }

  if (customBlkParams)
  {
    eastl::string substituted = substitute(customBlkParams, elem_idx, g, states, base_nodes_blk);
    mergeText(substituted.c_str());
  }

  nb->setInt("width", texture_width(elem_idx, g, states));
  nb->setInt("height", texture_height(elem_idx, g, states));
}

void emit_node_block(int elem_idx, const GraphData &g, NodeStates &states, const DataBlock &base_nodes_blk, DataBlock &out,
  FinalAccumulator &finals, eastl::hash_set<eastl::string> &uniq_finals)
{
  const GraphData::Node &elem = g.nodes[elem_idx];
  const DataBlock *desc = states[elem_idx].descriptor;

  const bool isShaderEditor = (elem.plugin == "shader_editor");
  const char *nativeShader = desc ? desc->getStr("nativeShader", nullptr) : nullptr;
  const bool hasShader = (isShaderEditor || (nativeShader && *nativeShader));
  if (!hasShader || elem.pins.empty())
  {
    return;
  }

  // Particle-bypass pre-pass (mainNodes.js:2582-2611). One _bypass_<...> block
  // per connected in-pin with data.particleTexture; the main block then reads
  // from _p_<elem>_<pin> instead of the upstream texture.
  for (int j = 0; j < static_cast<int>(elem.pins.size()); ++j)
  {
    const GraphData::Pin &p = elem.pins[j];
    if (p.role != PinRole::In)
    {
      continue;
    }
    const PinCompileState *st = get_pin_state(states, elem_idx, j);
    if (!st || !st->connected)
    {
      continue;
    }
    const DataBlock *descData = states[elem_idx].pinDataPtrs[j];
    if (!descData)
    {
      continue;
    }
    if (descData->getBool("particleTexture", false))
    {
      emit_particle_bypass(elem_idx, j, g, states, base_nodes_blk, out);
    }
  }

  // Scan for multipass (mainNodes.js:2678-2679). Last pin with second_pass_in
  // wins -- matches the JS loop.
  int multipassPin = -1;
  for (int j = 0; j < static_cast<int>(elem.pins.size()); ++j)
  {
    const GraphData::Pin &p = elem.pins[j];
    if (p.role != PinRole::In)
    {
      continue;
    }
    const DataBlock *descData = states[elem_idx].pinDataPtrs[j];
    if (descData && descData->getBool("second_pass_in", false))
    {
      multipassPin = j;
    }
  }

  if (multipassPin < 0)
  {
    PassOverrides cfg;
    emit_pass_block(elem_idx, g, states, base_nodes_blk, out, finals, uniq_finals, cfg);
    return;
  }

  // Multipass split (mainNodes.js:2819-2852). Resolve the real upstream input
  // for the second_pass_in pin (used as the first-pass input), assign a temp
  // register for the first pass's output, then emit twice.
  const GraphData::Pin &mpPin = elem.pins[multipassPin];
  const PinCompileState *mpSt = get_pin_state(states, elem_idx, multipassPin);
  eastl::string firstPassReg;
  eastl::string firstPassWrap = "clamp";
  if (mpSt && mpSt->connected && mpSt->connectElem >= 0)
  {
    firstPassReg = make_input_name(mpSt->connectElem, mpSt->connectPin, g, states, base_nodes_blk);
    firstPassWrap = texture_wrap(mpSt->connectElem, g, states);
  }
  else
  {
    eastl::string placeholder = "$";
    placeholder.append(mpPin.name);
    placeholder.append("$");
    firstPassReg = substitute(placeholder.c_str(), elem_idx, g, states, base_nodes_blk);
  }

  char tmpRegBuf[64];
  snprintf(tmpRegBuf, sizeof(tmpRegBuf), "_2pass_tmp_reg_%d", elem_idx);

  // Emit in texgen-processing order: first pass writes the temp reg, second
  // pass reads it. The first pass also owns finals/customTextureName tracking
  // (computing the real outName) so the second pass's output block can read
  // the cached customTextureName for the real final register.
  PassOverrides first;
  first.blockNameSuffix = "_1st_pass";
  first.useSecondPassParamValues = false; // -V1048
  first.multipassInputPinIdx = multipassPin;
  first.multipassInputReg = firstPassReg;
  first.multipassInputWrap = firstPassWrap;
  first.overrideOutputReg = true;
  first.outputRegOverride = tmpRegBuf;
  first.trackFinalsAndCustomName = true; // -V1048
  emit_pass_block(elem_idx, g, states, base_nodes_blk, out, finals, uniq_finals, first);

  PassOverrides second;
  second.blockNameSuffix = "_2nd_pass";
  second.useSecondPassParamValues = true;
  second.multipassInputPinIdx = multipassPin;
  second.multipassInputReg = tmpRegBuf;
  second.multipassInputWrap = firstPassWrap;
  second.overrideOutputReg = false; // -V1048
  second.trackFinalsAndCustomName = false;
  emit_pass_block(elem_idx, g, states, base_nodes_blk, out, finals, uniq_finals, second);
}

// Particle-bypass block emission (mainNodes.js:2582-2611). One block per
// connected in-pin with data.particleTexture. The block routes the upstream
// texture through `data.particleShader` (default "unary") and emits its result
// into a private register `_p_<elem>_<pin>`. The main per-node block then reads
// from that register instead of the original upstream.
void emit_particle_bypass(int elem_idx, int pin_idx, const GraphData &g, const NodeStates &states, const DataBlock &base_nodes_blk,
  DataBlock &out)
{
  const GraphData::Node &elem = g.nodes[elem_idx];
  const GraphData::Pin &p = elem.pins[pin_idx];
  (void)p;
  const DataBlock *descData = states[elem_idx].pinDataPtrs[pin_idx];
  const PinCompileState *st = get_pin_state(states, elem_idx, pin_idx);
  if (!st || !st->connected)
  {
    return;
  }

  // JS: `_bypass_<uid>_<i>_<j>` at mainNodes.js:2588; our port substitutes int id.
  char nameBuf[96];
  snprintf(nameBuf, sizeof(nameBuf), "_bypass_%d_%d_%d", elem.id, elem_idx, pin_idx);
  DataBlock *bb = out.addNewBlock(nameBuf);

  const char *particleShader = descData ? descData->getStr("particleShader", nullptr) : nullptr;
  bb->setStr("shader", (particleShader && *particleShader) ? particleShader : "unary");

  // Other pins' data.particleCode parsed as inline BLK params -- mirrors the
  // customParam handling in emit_pass_block.
  for (int k = 0; k < static_cast<int>(elem.pins.size()); ++k)
  {
    const GraphData::Pin &p2 = elem.pins[k];
    if (p2.role != PinRole::In)
    {
      continue;
    }
    const PinCompileState *st2 = get_pin_state(states, elem_idx, k);
    if (!st2 || !st2->connected)
    {
      continue;
    }
    const DataBlock *desc2Data = states[elem_idx].pinDataPtrs[k];
    if (!desc2Data)
    {
      continue;
    }
    const char *particleCode = desc2Data->getStr("particleCode", nullptr);
    if (!particleCode || !*particleCode)
    {
      continue;
    }
    eastl::string substituted = substitute(particleCode, elem_idx, g, states, base_nodes_blk);
    DataBlock tmp;
    if (tmp.loadText(substituted.c_str(), static_cast<int>(substituted.size())))
    {
      bb->appendParamsFrom(&tmp);
    }
  }

  // Input is the real upstream texture (not the bypass output). Wrap mode is
  // hardcoded to "clamp" per the JS.
  eastl::string inputName = make_input_name(st->connectElem, st->connectPin, g, states, base_nodes_blk);
  DataBlock *inb = bb->addNewBlock("input");
  inb->setStr("reg", inputName.c_str());
  inb->setStr("wrap_mode", "clamp");

  // Output to the private bypass register.
  char outRegBuf[64];
  snprintf(outRegBuf, sizeof(outRegBuf), "_p_%d_%d", elem_idx, pin_idx);
  DataBlock *outb = bb->addNewBlock("output");
  outb->setStr("reg", outRegBuf);
  outb->setStr("wrap_mode", "clamp");
  outb->setStr("fmt", texture_type(elem_idx, g, states));

  bb->setInt("width", texture_width(elem_idx, g, states));
  bb->setInt("height", texture_height(elem_idx, g, states));
}

} // namespace

bool compile_graph_to_blks(const GraphData &g, const DataBlock &base_nodes_blk, const char *shader_includes_dir,
  DataBlock &out_main_graph_blk, DataBlock &out_shader_list_blk,
  eastl::vector<eastl::vector<eastl::string>> &out_pin_custom_texture_names)
{
  out_main_graph_blk.reset();
  out_shader_list_blk.reset();
  out_pin_custom_texture_names.clear();

  NodeIndex index;
  index.build(g);

  NodeStates states;
  build_connectivity(g, index, states);

  // Descriptor / pin-descriptor cache. Each find_descriptor call linear-scans
  // base_nodes.blk (~100 entries); each find_pin_in_desc scans a descriptor's
  // pin sub-blocks. Without caching this dominates compile time for graphs
  // with 1000+ nodes -- the propagation loop alone does ~30M strcmp.
  // Also pre-resolves the effective per-pin data block: prefer per-instance
  // Pin::data (only populated for shader_editor sub-graph nodes whose
  // descriptor isn't in base_nodes.blk), fall back to the descriptor's pin
  // data sub-block. Compile call sites read from pinDataPtrs directly.
  for (int i = 0; i < static_cast<int>(g.nodes.size()); ++i)
  {
    const GraphData::Node &n = g.nodes[i];
    states[i].descriptor = find_descriptor(base_nodes_blk, n.templateUid);
    states[i].pinDescriptors.resize(n.pins.size());
    states[i].pinDataPtrs.resize(n.pins.size());
    for (int j = 0; j < static_cast<int>(n.pins.size()); ++j)
    {
      states[i].pinDescriptors[j] = find_pin_in_desc(states[i].descriptor, n.pins[j].name);
      const DataBlock &instData = n.pins[j].data;
      if (instData.paramCount() > 0 || instData.blockCount() > 0)
      {
        states[i].pinDataPtrs[j] = &instData;
      }
      else
      {
        states[i].pinDataPtrs[j] = find_pin_data(states[i].pinDescriptors[j]);
      }
    }
  }

  propagate_final(g, index, base_nodes_blk, states);

  // Propagation pass: iterate to a fixed point. Each iteration tries to resolve
  // every node's propagatedStuff via propagate_external_stuff, then walks edges
  // to push results into downstream input pins' externalStuffOnPins. Nodes that
  // depend on upstream propagation just stay un-propagated until the next
  // iteration sees their upstream's result available. Same 400-iter guard as
  // the topo-sort loop below.
  {
    int propIteration = 0;
    bool propChanged = false;
    do
    {
      propChanged = false;
      for (int i = 0; i < static_cast<int>(g.nodes.size()); ++i)
      {
        if (propagate_external_stuff(i, g, base_nodes_blk, states))
        {
          propChanged = true;
        }
      }
      if (propagate_edges(g, index, states))
      {
        propChanged = true;
      }
      ++propIteration;
      if (propIteration > 400)
      {
        debug("compile_graph_to_blks: propagation loop hit 400-iteration guard");
        break;
      }
    } while (propChanged);
  }

  // Pre-emit bypass splicing (graphEditor.js:3214/5648). Bypass descriptors (e.g.
  // `pass`/unary) are removed and their consumers re-wired to the upstream source so
  // the compile emits neither a `_node_<bypass>_<i>` block nor `_t_<bypass>_<j>`
  // register references. Runs after propagation so the type-compat guard has values
  // to compare; doesn't trigger another propagation pass because bypass eligibility
  // requires matching propagatedStuff by construction.
  apply_bypass_removal(g, base_nodes_blk, states);

  // shader_editor include list (mainNodes.js:2289-2301). Dedup by descName. We
  // can't just addStr("include", ...) on the DataBlock -- that creates a string
  // param, not an include-directive expansion. Build the text form and parse it
  // via loadText with a synthetic filename rooted at shader_includes_dir so the
  // BLK parser resolves `_<descName>.blk` against that dir and inlines the
  // referenced shader blocks.
  {
    eastl::hash_set<eastl::string> listed;
    eastl::string includesText;
    for (const GraphData::Node &n : g.nodes)
    {
      if (n.plugin != "shader_editor" || n.pins.empty())
      {
        continue;
      }
      if (listed.count(n.descName))
      {
        continue;
      }
      listed.insert(n.descName);
      includesText.append("include _");
      includesText.append(n.descName);
      includesText.append(".blk\n");
    }
    if (!includesText.empty())
    {
      eastl::string syntheticFname = shader_includes_dir ? shader_includes_dir : "";
      syntheticFname.append("shadersList.blk");
      // Robust load suppresses "BLK parsed string for param 'X' is really long"
      // warnings -- the shader_editor `_<descName>.blk` includes legitimately carry
      // multi-KB DSHL shader sources as triple-quoted strings.
      dblk::load_text(out_shader_list_blk, dag::ConstSpan<char>(includesText.c_str(), static_cast<int>(includesText.size())),
        dblk::ReadFlag::ROBUST, syntheticFname.c_str());
    }
  }

  // Topo-sort loop (mainNodes.js:2531-2863). For each unprocessed node whose `in`-role
  // pins all point at already-processed nodes, emit the per-node block. Repeat to
  // fixed point with a hard iteration guard.
  eastl::vector<bool> processed(g.nodes.size(), false);
  // Bypassed nodes never emit; pre-mark so they don't block their (re-routed) consumers.
  for (int i = 0; i < static_cast<int>(g.nodes.size()); ++i)
  {
    if (states[i].bypassed)
    {
      processed[i] = true;
    }
  }
  FinalAccumulator finals;
  eastl::hash_set<eastl::string> uniqFinals;
  int iteration = 0;
  bool changed = false;
  do
  {
    changed = false;
    for (int i = 0; i < static_cast<int>(g.nodes.size()); ++i)
    {
      if (processed[i])
      {
        continue;
      }
      const GraphData::Node &e = g.nodes[i];
      bool allInProcessed = true;
      for (int j = 0; j < static_cast<int>(e.pins.size()); ++j)
      {
        const PinCompileState *st = get_pin_state(states, i, j);
        if (!st)
        {
          continue;
        }
        if (st->connectFromType != PinType::Unknown && e.pins[j].role == PinRole::In && st->connectElem >= 0 &&
            !processed[st->connectElem])
        {
          allInProcessed = false;
          break;
        }
      }
      if (allInProcessed)
      {
        processed[i] = true;
        changed = true;
        emit_node_block(i, g, states, base_nodes_blk, out_main_graph_blk, finals, uniqFinals);
      }
    }
    ++iteration;
    if (iteration > 400)
    {
      debug("compile_graph_to_blks: topo-sort fixed-point loop hit 400-iteration guard");
      break;
    }
  } while (changed);

  // Snapshot PinCompileState::customTextureName into the output parameter so the
  // caller can apply it onto GraphData::Pin::customTextureName from the main
  // thread. The texture-preview panel (graph_panel.cpp) reads that Pin field
  // without taking graphMutex; writing it from the worker thread (compile runs
  // here, plugin.cpp:84) would race against those reads. Bypassed nodes get an
  // empty inner vector since they never run emit_pass_block.
  out_pin_custom_texture_names.resize(g.nodes.size());
  for (int i = 0; i < static_cast<int>(g.nodes.size()); ++i)
  {
    if (states[i].bypassed)
    {
      continue;
    }
    const GraphData::Node &n = g.nodes[i];
    out_pin_custom_texture_names[i].resize(n.pins.size());
    for (int j = 0; j < static_cast<int>(n.pins.size()); ++j)
    {
      const PinCompileState *st = get_pin_state(states, i, j);
      if (!st)
      {
        continue;
      }
      out_pin_custom_texture_names[i][j] = st->customTextureName;
    }
  }

  for (const auto &s : finals.finals)
  {
    out_main_graph_blk.addStr("final", s.c_str());
  }
  for (const auto &s : finals.selected)
  {
    out_main_graph_blk.addStr("selected", s.c_str());
  }
  for (const auto &s : finals.skipSavings)
  {
    out_main_graph_blk.addStr("skip_saving", s.c_str());
  }

  return iteration <= 400;
}
