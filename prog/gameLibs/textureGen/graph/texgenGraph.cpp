// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameLibs/textureGen/graph/texgenGraph_api.h>

#include <EASTL/algorithm.h>
#include <EASTL/hash_set.h>

#include <math/dag_check_nan.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace texgen
{
namespace
{
int float_count(Value::Type t) { return t >= Value::Float && t <= Value::Float4 ? t - Value::Float + 1 : 0; }

// Plain %.9g round-trips too, but pads clean values (0.333 -> 0.333000004).
void append_shortest(eastl::string &out, float v)
{
  char buf[32];
  for (int precision = 6; precision <= 9; ++precision)
  {
    snprintf(buf, sizeof(buf), "%.*g", precision, v);
    if (strtof(buf, nullptr) == v)
    {
      break;
    }
  }
  out += buf;
}

const char *skip_spaces(const char *p)
{
  while (*p == ' ' || *p == '\t')
  {
    ++p;
  }
  return p;
}

bool parse_float_list(const char *text, int count, float *out)
{
  const char *p = text;
  for (int k = 0; k < count; ++k)
  {
    if (k > 0)
    {
      p = skip_spaces(p);
      if (*p != ',')
      {
        return false;
      }
      ++p;
    }
    p = skip_spaces(p);
    char *end = nullptr;
    out[k] = strtof(p, &end);
    if (end == p || !check_finite(out[k]))
    {
      return false;
    }
    p = end;
  }
  return *skip_spaces(p) == 0;
}
} // namespace

Value Value::makeInt(int v)
{
  Value r;
  r.type = Int;
  r.i = v;
  return r;
}

Value Value::makeFloat(float v) { return makeFloats(Float, &v); }

Value Value::makeBool(bool v)
{
  Value r;
  r.type = Bool;
  r.b = v;
  return r;
}

Value Value::makeFloats(Type t, const float *v)
{
  Value r;
  r.type = t;
  for (int k = 0; k < float_count(t); ++k)
  {
    r.f[k] = v[k];
  }
  return r;
}

Value Value::makeString(const char *v)
{
  Value r;
  r.s = v ? v : "";
  return r;
}

bool Value::operator==(const Value &o) const
{
  return type == o.type && i == o.i && b == o.b && s == o.s && eastl::equal(f, f + 4, o.f);
}

eastl::string value_to_text(const Value &v)
{
  eastl::string out;
  switch (v.type)
  {
    case Value::Int: out.sprintf("%d", v.i); break;
    case Value::Bool: out = v.b ? "true" : "false"; break;
    case Value::String: out = v.s; break;
    default:
      for (int k = 0; k < float_count(v.type); ++k)
      {
        if (k > 0)
        {
          out.push_back(',');
        }
        append_shortest(out, v.f[k]);
      }
      break;
  }
  return out;
}

Value value_from_text(Value::Type type, const char *text)
{
  switch (type)
  {
    case Value::Int:
    {
      const char *p = skip_spaces(text);
      char *end = nullptr;
      const long long n = strtoll(p, &end, 10);
      if (end != p && *skip_spaces(end) == 0 && n == static_cast<int>(n))
      {
        return Value::makeInt(static_cast<int>(n));
      }
      break;
    }
    case Value::Bool:
      if (strcmp(text, "true") == 0 || strcmp(text, "false") == 0)
      {
        return Value::makeBool(text[0] == 't');
      }
      break;
    case Value::String: break;
    default:
    {
      float f[4] = {};
      if (parse_float_list(text, float_count(type), f))
      {
        return Value::makeFloats(type, f);
      }
      break;
    }
  }
  return Value::makeString(text);
}

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
    p = skip_spaces(p);
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

int texture_out_dimension(PinType t)
{
  switch (t)
  {
    case PinType::Texture1D: return 1;
    case PinType::Particles:
    case PinType::Texture2D: return 2;
    case PinType::Texture3D: return 3;
    default: return 0;
  }
}

bool is_texture_or_particles(PinType t) { return texture_out_dimension(t) != 0; }

bool is_texture_type_for_inherit(PinType t)
{
  return t == PinType::Texture1D || t == PinType::Texture2D || t == PinType::Texture3D || t == PinType::Texture2DArray ||
         t == PinType::Texture2DShdArray;
}

eastl::string size_rule_text(const SizeRule &r)
{
  switch (r.kind)
  {
    case SizeRule::Explicit: return eastl::string(eastl::string::CtorSprintf(), "= %d", r.value);
    case SizeRule::ParentMul: return eastl::string(eastl::string::CtorSprintf(), "x %d", r.value);
    case SizeRule::ParentDiv: return eastl::string(eastl::string::CtorSprintf(), "/ %d", r.value);
    case SizeRule::Parent: return SIZE_PREFIXES[0];
    case SizeRule::Graph: return SIZE_PREFIXES[1];
    case SizeRule::Width: return SIZE_SAME_AS_WIDTH;
  }
  return SIZE_PREFIXES[0];
}

eastl::string fmt_rule_text(const FmtRule &r)
{
  switch (r.kind)
  {
    case FmtRule::Parent: return TYPE_PREFIXES[0];
    case FmtRule::Graph: return TYPE_PREFIXES[1];
    case FmtRule::Explicit: return r.fmt;
  }
  return TYPE_PREFIXES[0];
}

eastl::string wrap_rule_text(const WrapRule &r)
{
  switch (r.kind)
  {
    case WrapRule::Parent: return WRAP_PREFIXES[0];
    case WrapRule::Graph: return WRAP_PREFIXES[1];
    case WrapRule::Explicit: return r.wrap ? BASE_WRAPS[0] : BASE_WRAPS[1];
  }
  return WRAP_PREFIXES[0];
}

void split_export_file(const Export &e, const eastl::string &final_name, eastl::string &name, eastl::string &format)
{
  const eastl::string &file = e.file;
  size_t split = file.size();
  if (!final_name.empty() && file.size() >= final_name.size() && file.compare(0, final_name.size(), final_name) == 0)
  {
    split = final_name.size();
  }
  else if (const size_t dot = file.rfind('.'); dot != eastl::string::npos)
  {
    split = dot;
  }
  name = file.substr(0, split);
  format = file.substr(split);
}

NodeId Graph::newNodeId() const
{
  int maxId = 0;
  for (const Node &n : nodes)
  {
    maxId = eastl::max(maxId, n.id.v);
  }
  for (const Final &f : finals)
  {
    maxId = eastl::max(maxId, f.id.v);
  }
  return NodeId{maxId + 1};
}

const Node *Graph::findNode(NodeId id) const
{
  auto it = eastl::find_if(nodes.begin(), nodes.end(), [id](const Node &n) { return n.id == id; });
  return it != nodes.end() ? &*it : nullptr;
}

const Final *Graph::findFinal(NodeId id) const
{
  auto it = eastl::find_if(finals.begin(), finals.end(), [id](const Final &f) { return f.id == id; });
  return it != finals.end() ? &*it : nullptr;
}

bool validate(const Graph &g, eastl::string &errors)
{
  const size_t errorsBefore = errors.size();
  eastl::hash_set<int> ids;
  auto checkId = [&](NodeId id, const char *what) {
    if (!id)
    {
      errors.append_sprintf("%s with id 0\n", what);
    }
    else if (!ids.insert(id.v).second)
    {
      errors.append_sprintf("%s %d: duplicate id\n", what, id.v);
    }
  };
  // the text form and das have no non-finite float literal
  auto checkParams = [&](const eastl::vector<Param> &params, const char *what, int owner) {
    for (const Param &p : params)
    {
      for (int k = 0; k < float_count(p.value.type); ++k)
      {
        if (!check_finite(p.value.f[k]))
        {
          errors.append_sprintf("%s %d param '%s': not finite\n", what, owner, p.name.c_str());
          break;
        }
      }
    }
  };
  for (const Node &n : g.nodes)
  {
    checkId(n.id, "node");
    checkParams(n.params, "node", n.id.v);
  }
  const eastl::pair<const char *, float> heightmap[] = {
    {"scale", g.options.heightmapScale}, {"min", g.options.heightmapMin}, {"cell", g.options.heightmapCellSize}};
  for (const auto &[name, v] : heightmap)
  {
    if (!check_finite(v))
    {
      errors.append_sprintf("graph: heightmap option is not finite: '%s'\n", name);
    }
  }

  // A final has no out pins, so only a node can feed a ref; ids holds the node ids until the finals join it.
  auto checkRef = [&](const PinRef &ref, const char *what, int owner, int slot) {
    if (!ref.node)
    {
      if (ref.muted)
      {
        errors.append_sprintf("%s %d input %d: muted but unconnected\n", what, owner, slot);
      }
      return;
    }
    if (!ids.count(ref.node.v))
    {
      errors.append_sprintf("%s %d input %d: no node %d\n", what, owner, slot, ref.node.v);
    }
  };
  for (const Node &n : g.nodes)
  {
    for (int k = 0; k < static_cast<int>(n.inputs.size()); ++k)
    {
      checkRef(n.inputs[k], "node", n.id.v, k);
    }
  }
  for (const Final &f : g.finals)
  {
    checkRef(f.src, "final", f.id.v, 0);
    checkParams(f.params, "final", f.id.v);
  }
  for (const Final &f : g.finals)
  {
    checkId(f.id, "final");
  }
  for (const Subgraph &s : g.subgraphs)
  {
    eastl::string sub;
    if (!validate(s.graph, sub))
    {
      errors.append_sprintf("subgraph '%s':\n%s", s.uid.c_str(), sub.c_str());
    }
  }
  return errors.size() == errorsBefore;
}
} // namespace texgen
