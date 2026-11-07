// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <graphEditor/graph_das_gen.h>
#include <graphEditor/graph_data.h>
#include <graphEditor/graph_ir.h>

#include <ioSys/dag_dataBlock.h>

#include <EASTL/algorithm.h>
#include <EASTL/sort.h>
#include <ska_hash_map/flat_hash_map2.hpp>

#include <string.h>

namespace
{
constexpr const char *DAS_KEYWORDS[] = {"abstract", "addr", "aka", "array", "as", "assume", "auto", "bitfield", "block", "bool",
  "break", "byte16", "byte2", "byte3", "byte4", "byte8", "capture", "cast", "class", "const", "continue", "def", "default", "delete",
  "deref", "double", "elif", "else", "enum", "expect", "explicit", "false", "finally", "fixed_array", "float", "float16", "float2",
  "float3", "float4", "for", "function", "generator", "goto", "half2", "half3", "half4", "half8", "if", "implicit", "in", "include",
  "inscope", "int", "int16", "int2", "int3", "int4", "int64", "int8", "is", "iterator", "label", "lambda", "let", "module", "new",
  "null", "operator", "options", "override", "pass", "private", "public", "range", "range64", "recover", "reinterpret", "require",
  "return", "sealed", "shared", "short2", "short3", "short4", "short8", "smart_ptr", "static", "static_elif", "static_if", "string",
  "struct", "table", "template", "true", "try", "tuple", "type", "typedecl", "typedef", "typeinfo", "ubyte16", "ubyte2", "ubyte3",
  "ubyte4", "ubyte8", "uint", "uint16", "uint2", "uint3", "uint4", "uint64", "uint8", "uninitialized", "unsafe", "upcast", "urange",
  "urange64", "ushort2", "ushort3", "ushort4", "ushort8", "var", "variant", "void", "where", "while", "with", "yield"};

constexpr const char *CORE_FUNCTIONS[] = {"texgraph", "heightmap", "muted", "parent", "graph_size", "px", "as_width", "size", "depth",
  "fmt", "fmt_graph", "wrap", "wrap_graph", "rules", "node", "out", "val", "param", "input", "final_output", "final_export", "output",
  "export", "graph", "Tex", "Val", "Size"};

constexpr const char *BODY_NAMES[] = {"n_", "node", "param", "input", "rules", "out", "val", "as_width", "parent", "px", "graph_size",
  "Tex", "Val", "Size", "muted", "width", "height", "depth", "fmt", "wrap_mode"};

template <size_t N>
bool in_list(const char *const (&list)[N], const eastl::string &s)
{
  for (const char *w : list)
  {
    if (s == w)
    {
      return true;
    }
  }
  return false;
}

bool is_ident_char(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'; }

eastl::string ident_of(const char *text, bool sanitize)
{
  eastl::string s;
  for (const char *p = text; *p; ++p)
  {
    const char c = *p == ' ' || *p == '-' ? '_' : *p;
    if (!is_ident_char(c))
    {
      if (!sanitize)
      {
        return {};
      }
      s.push_back('_');
      continue;
    }
    s.push_back(c);
  }
  if (s.empty())
  {
    return s;
  }
  if (s[0] >= '0' && s[0] <= '9')
  {
    s.insert(s.begin(), 'n');
  }
  if (in_list(DAS_KEYWORDS, s))
  {
    s.push_back('_');
  }
  return s;
}

bool is_scalar(PinType t)
{
  switch (t)
  {
    case PinType::Bool:
    case PinType::Int:
    case PinType::Uint:
    case PinType::Float:
    case PinType::Float2:
    case PinType::Float3:
    case PinType::Float4: return true;
    default: return false;
  }
}

enum class Handle : uint8_t
{
  Tex,
  Val,
  Any,
};

// The das type of a value of the handle, and the parameter that takes it: Val derives from Tex.
const char *handle_type(Handle h) { return h == Handle::Val ? "Val" : "Tex"; }
const char *param_type(Handle h) { return h == Handle::Tex ? "Tex explicit" : h == Handle::Val ? "Val" : "Tex"; }
const char *sheet_type(Handle h) { return h == Handle::Tex ? "Tex" : h == Handle::Val ? "Val" : "Tex|Val"; }

void split_types(const char *types, eastl::vector<PinType> &out)
{
  eastl::string token;
  for (const char *p = types;; ++p)
  {
    if (*p == ',' || *p == 0)
    {
      if (!token.empty())
      {
        out.push_back(parse_pin_type(token.c_str()));
      }
      token.clear();
      if (*p == 0)
      {
        break;
      }
    }
    else if (*p != ' ' && *p != '\t')
    {
      token.push_back(*p);
    }
  }
}

Handle in_pin_handle(const DataBlock &pin)
{
  eastl::vector<PinType> types;
  split_types(pin.getStr("types", ""), types);
  bool scalar = false, other = false;
  for (PinType t : types)
  {
    (is_scalar(t) ? scalar : other) = true;
  }
  return scalar && !other ? Handle::Val : other && !scalar ? Handle::Tex : Handle::Any;
}

// The first type decides, as preprocess_node_descriptor does.
Handle out_pin_handle(const DataBlock &pin)
{
  eastl::vector<PinType> types;
  split_types(pin.getStr("types", ""), types);
  return !types.empty() && is_scalar(types[0]) ? Handle::Val : Handle::Tex;
}

eastl::string das_float(float f)
{
  eastl::string s = texgen::value_to_text(texgen::Value::makeFloat(f));
  if (s.find('.') != eastl::string::npos)
  {
    return s;
  }
  const size_t e = s.find_first_of("eE");
  if (e == eastl::string::npos)
  {
    return s + ".0";
  }
  s.insert(e, ".0");
  return s;
}

eastl::string das_string(const char *text)
{
  eastl::string s = "\"";
  for (const char *p = text; *p; ++p)
  {
    switch (*p)
    {
      case '"': s += "\\\""; break;
      case '\\': s += "\\\\"; break;
      case '{': s += "\\{"; break;
      case '}': s += "\\}"; break;
      case '\n': s += "\\n"; break;
      case '\r': s += "\\r"; break;
      case '\t': s += "\\t"; break;
      default: s.push_back(*p);
    }
  }
  s += "\"";
  return s;
}

eastl::string das_value(const texgen::Value &v)
{
  switch (v.type)
  {
    case texgen::Value::Int: return eastl::string(eastl::string::CtorSprintf(), "%d", v.i);
    case texgen::Value::Float: return das_float(v.f[0]);
    case texgen::Value::Bool: return v.b ? "true" : "false";
    case texgen::Value::Float2:
    case texgen::Value::Float3:
    case texgen::Value::Float4:
    {
      const int n = 2 + (v.type - texgen::Value::Float2);
      eastl::string s(eastl::string::CtorSprintf(), "float%d(", n);
      for (int k = 0; k < n; ++k)
      {
        s += (k ? ", " : "") + das_float(v.f[k]);
      }
      return s + ")";
    }
    case texgen::Value::String: return das_string(v.s.c_str());
  }
  return das_string(v.s.c_str());
}

const char *das_type(texgen::Value::Type t)
{
  switch (t)
  {
    case texgen::Value::Int: return "int";
    case texgen::Value::Float: return "float";
    case texgen::Value::Bool: return "bool";
    case texgen::Value::Float2: return "float2";
    case texgen::Value::Float3: return "float3";
    case texgen::Value::Float4: return "float4";
    case texgen::Value::String: return "string";
  }
  return "string";
}

eastl::string das_size(const texgen::SizeRule &r)
{
  switch (r.kind)
  {
    case texgen::SizeRule::Explicit: return eastl::string(eastl::string::CtorSprintf(), "px(%d)", r.value);
    case texgen::SizeRule::Parent: return "parent()";
    case texgen::SizeRule::Graph: return "graph_size()";
    case texgen::SizeRule::ParentMul: return eastl::string(eastl::string::CtorSprintf(), "parent() * %d", r.value);
    case texgen::SizeRule::ParentDiv: return eastl::string(eastl::string::CtorSprintf(), "parent() / %d", r.value);
    case texgen::SizeRule::Width: return "as_width()";
  }
  return "parent()";
}

// The rules() text of a format or wrap rule: "" parent, "graph" the graph's, else the explicit item.
eastl::string fmt_text(const texgen::FmtRule &r)
{
  return r.kind == texgen::FmtRule::Parent ? "" : r.kind == texgen::FmtRule::Graph ? "graph" : r.fmt;
}

eastl::string wrap_text(const texgen::WrapRule &r)
{
  return r.kind == texgen::WrapRule::Parent ? "" : r.kind == texgen::WrapRule::Graph ? "graph" : r.wrap ? "wrap" : "clamp";
}

struct Arg
{
  enum Kind : uint8_t
  {
    Pin,
    Prop,
    Width,
    Height,
    Depth,
    Fmt,
    Wrap,
  };
  texgen::Value def; // Prop: the default, typed as the parameter
  eastl::string ident;
  eastl::string prop;    // Prop: the descriptor property name
  eastl::string text;    // Fmt, Wrap: the default as rules() reads it
  texgen::SizeRule size; // Width, Height, Depth: the default
  int pin = 0;           // pins: the in pin index
  Kind kind = Prop;
  Handle handle = Handle::Tex; // pins
  bool texPin() const { return kind == Pin && handle != Handle::Val; }
};

struct Out
{
  eastl::string ident;
  Handle handle = Handle::Tex;
};

struct NodeSpec
{
  eastl::string uid;
  eastl::string ident;
  eastl::string category;
  eastl::string op;
  eastl::vector<Arg> args;
  eastl::vector<Out> outs;
  int inCount = 0;
  bool rules = false;
  bool hasHeight = false;
  bool hasDepth = false;
};

struct Specs
{
  eastl::vector<NodeSpec> nodes;
  ska::flat_hash_map<eastl::string, int> byUid;
  eastl::string outputUid;
  eastl::string exportUid;

  const NodeSpec *find(const eastl::string &uid) const
  {
    const auto it = byUid.find(uid);
    return it == byUid.end() ? nullptr : &nodes[it->second];
  }
};

bool is_texture_prop(const char *name)
{
  return strcmp(name, PROP_TEXTURE_WIDTH) == 0 || strcmp(name, PROP_TEXTURE_HEIGHT) == 0 || strcmp(name, PROP_TEXTURE_DEPTH) == 0 ||
         strcmp(name, PROP_TEXTURE_TYPE) == 0 || strcmp(name, PROP_TEXTURE_WRAP) == 0;
}

eastl::string claim(ska::flat_hash_set<eastl::string> &taken, eastl::string name)
{
  if (name.empty())
  {
    name = "arg";
  }
  if (in_list(BODY_NAMES, name))
  {
    name.push_back('_');
  }
  eastl::string candidate = name;
  for (int k = 2; taken.count(candidate); ++k)
  {
    candidate = name + eastl::string(eastl::string::CtorSprintf(), "_%d", k);
  }
  taken.insert(candidate);
  return candidate;
}

void build_node_spec(const DataBlock &desc, NodeSpec &spec, const eastl::vector<const DataBlock *> &ins,
  const eastl::vector<const DataBlock *> &outs)
{
  ska::flat_hash_set<eastl::string> taken;
  eastl::vector<Arg> props;
  for (uint32_t i = 0; i < desc.blockCount(); ++i)
  {
    const DataBlock *b = desc.getBlock(i);
    const char *name = b->getStr("name", "");
    if (strcmp(b->getBlockName(), "property") != 0 || !*name || is_texture_prop(name))
    {
      continue;
    }
    Arg a;
    a.prop = name;
    a.ident = claim(taken, ident_of(a.prop.c_str(), true));
    // A default that does not parse as the declared type makes a string parameter, as graph_data_to_ir keeps it.
    a.def = texgen::value_from_text(value_type_for(b), param_as_string(b, "val").c_str());
    props.push_back(eastl::move(a));
  }
  ska::flat_hash_set<eastl::string> propNames = taken;

  eastl::vector<Arg> texPins, valPins;
  for (int k = 0; k < static_cast<int>(ins.size()); ++k)
  {
    Arg a;
    a.pin = k;
    a.handle = in_pin_handle(*ins[k]);
    a.kind = Arg::Pin;
    eastl::string name = ident_of(ins[k]->getStr("name", ""), true);
    if (propNames.count(name))
    {
      name += "_in";
    }
    a.ident = claim(taken, name);
    (a.texPin() ? texPins : valPins).push_back(eastl::move(a));
  }
  spec.inCount = static_cast<int>(ins.size());

  spec.args = eastl::move(texPins);
  spec.args.insert(spec.args.end(), eastl::make_move_iterator(props.begin()), eastl::make_move_iterator(props.end()));
  spec.args.insert(spec.args.end(), eastl::make_move_iterator(valPins.begin()), eastl::make_move_iterator(valPins.end()));

  const DataBlock *w = find_property_desc(&desc, PROP_TEXTURE_WIDTH);
  if (w)
  {
    spec.rules = true;
    auto addSize = [&](Arg::Kind kind, const char *name, const DataBlock *prop, texgen::SizeRule::Kind fallback) {
      Arg a;
      a.kind = kind;
      a.ident = name;
      a.size = {fallback, 0};
      parse_size_rule(param_as_string(prop, "val"), kind != Arg::Width, a.size);
      spec.args.push_back(eastl::move(a));
    };
    addSize(Arg::Width, "width", w, texgen::SizeRule::Parent);
    if (const DataBlock *h = find_property_desc(&desc, PROP_TEXTURE_HEIGHT))
    {
      spec.hasHeight = true;
      addSize(Arg::Height, "height", h, texgen::SizeRule::Width);
    }
    if (const DataBlock *d = find_property_desc(&desc, PROP_TEXTURE_DEPTH))
    {
      spec.hasDepth = true;
      addSize(Arg::Depth, "depth", d, texgen::SizeRule::Width);
    }
    Arg f;
    f.kind = Arg::Fmt;
    f.ident = "fmt";
    texgen::FmtRule fmtRule;
    parse_fmt_rule(param_as_string(find_property_desc(&desc, PROP_TEXTURE_TYPE), "val"), fmtRule);
    f.text = fmt_text(fmtRule);
    spec.args.push_back(eastl::move(f));
    Arg wr;
    wr.kind = Arg::Wrap;
    wr.ident = "wrap_mode";
    texgen::WrapRule rule;
    const eastl::string wrapDefault = param_as_string(find_property_desc(&desc, PROP_TEXTURE_WRAP), "val");
    wr.text = parse_wrap_rule(wrapDefault, rule) ? wrap_text(rule) : wrapDefault;
    spec.args.push_back(eastl::move(wr));
  }

  ska::flat_hash_set<eastl::string> outNames;
  for (const DataBlock *o : outs)
  {
    Out out;
    out.handle = out_pin_handle(*o);
    out.ident = claim(outNames, ident_of(o->getStr("name", ""), true));
    spec.outs.push_back(out);
  }
}

void build_specs(const DataBlock &base, Specs &specs, eastl::string &errors)
{
  ska::flat_hash_set<eastl::string> idents;
  for (uint32_t i = 0; i < base.blockCount(); ++i)
  {
    const DataBlock &desc = *base.getBlock(i);
    const char *uid = desc.getStr("templateUid", "");
    const char *name = desc.getStr("name", "");
    if (strcmp(desc.getBlockName(), "node") != 0 || !uid[0] || desc.getBool("hidden", false) || strcmp(name, "block") == 0 ||
        strcmp(name, "comment") == 0 || strcmp(name, "<graph properties>") == 0)
    {
      continue;
    }
    TemplatePins pins;
    collect_template_pins(desc, pins);
    if (is_final_template(pins))
    {
      const bool isExport = find_property_desc(&desc, PROP_FILE_FORMAT) != nullptr;
      eastl::string &slot = isExport ? specs.exportUid : specs.outputUid;
      if (slot.empty())
      {
        slot = uid;
      }
      continue;
    }
    const char *alias = desc.getStr("alias", "");
    eastl::string ident = ident_of(alias[0] ? alias : name, false);
    if (!ident.empty() && in_list(CORE_FUNCTIONS, ident))
    {
      ident.push_back('_');
    }
    if (ident.empty() || idents.count(ident))
    {
      errors.append_sprintf("node '%s' (%s): %s\n", name, uid, ident.empty() ? "no identifier form" : "the identifier repeats");
      continue;
    }
    idents.insert(ident);
    NodeSpec &spec = specs.nodes.push_back();
    spec.uid = uid;
    spec.ident = ident;
    spec.category = desc.getStr("category", "");
    spec.op = desc.getStr("operator", "");
    build_node_spec(desc, spec, pins.ins, pins.outs);
    const bool binary = spec.args.size() >= 2 && spec.args[0].texPin() && spec.args[1].texPin();
    if (!spec.op.empty() && (!binary || spec.outs.size() != 1 || spec.outs[0].handle != Handle::Tex))
    {
      errors.append_sprintf("node '%s': operator %s needs two texture inputs first and one texture output\n", uid, spec.op.c_str());
      spec.op.clear();
    }
    specs.byUid[spec.uid] = static_cast<int>(specs.nodes.size()) - 1;
  }
}

eastl::string arg_default(const Arg &a)
{
  switch (a.kind)
  {
    case Arg::Pin: return a.handle == Handle::Val ? "Val()" : "Tex()";
    case Arg::Prop: return das_value(a.def);
    case Arg::Width:
    case Arg::Height:
    case Arg::Depth: return das_size(a.size);
    case Arg::Fmt:
    case Arg::Wrap: return das_string(a.text.c_str());
  }
  return {};
}

eastl::string arg_type(const Arg &a, bool sheet)
{
  switch (a.kind)
  {
    case Arg::Pin: return sheet ? sheet_type(a.handle) : param_type(a.handle);
    case Arg::Prop: return das_type(a.def.type);
    case Arg::Width:
    case Arg::Height:
    case Arg::Depth: return "Size";
    case Arg::Fmt:
    case Arg::Wrap: return "string";
  }
  return {};
}

eastl::string return_type(const NodeSpec &s)
{
  if (s.outs.size() <= 1)
  {
    return s.outs.empty() ? "Tex" : handle_type(s.outs[0].handle);
  }
  eastl::string t = "tuple<";
  for (size_t k = 0; k < s.outs.size(); ++k)
  {
    t += (k ? "; " : "") + s.outs[k].ident + " : " + handle_type(s.outs[k].handle);
  }
  return t + ">";
}

eastl::string out_ref(const char *var, int pin, Handle h)
{
  if (h == Handle::Val)
  {
    return eastl::string(eastl::string::CtorSprintf(), "%s.val(%d)", var, pin);
  }
  return pin == 0 ? eastl::string(var) : eastl::string(eastl::string::CtorSprintf(), "%s.out(%d)", var, pin);
}

void write_node_function(const NodeSpec &s, eastl::string &out)
{
  out += "def " + s.ident + "(";
  for (size_t k = 0; k < s.args.size(); ++k)
  {
    const Arg &a = s.args[k];
    out += (k ? "; " : "") + a.ident + " : " + arg_type(a, false) + " = " + arg_default(a);
  }
  out += ") : " + return_type(s) + " {\n";
  out.append_sprintf("    let n_ = node(%s, \"\", %d)\n", das_string(s.uid.c_str()).c_str(), s.inCount);
  for (const Arg &a : s.args)
  {
    if (a.kind == Arg::Prop)
    {
      out += "    n_ |> param(" + das_string(a.prop.c_str()) + ", " + a.ident + ")\n";
    }
    else if (a.kind == Arg::Pin)
    {
      out.append_sprintf("    n_ |> input(%d, %s)\n", a.pin, a.ident.c_str());
    }
  }
  if (s.rules)
  {
    out.append_sprintf("    n_ |> rules(width, %s, %s, fmt, wrap_mode)\n", s.hasHeight ? "height" : "as_width()",
      s.hasDepth ? "depth" : "as_width()");
  }
  if (s.outs.size() <= 1)
  {
    out += "    return " + out_ref("n_", 0, s.outs.empty() ? Handle::Tex : s.outs[0].handle) + "\n}\n\n";
    return;
  }
  out += "    return (";
  for (size_t k = 0; k < s.outs.size(); ++k)
  {
    out += (k ? ", " : "") + s.outs[k].ident + " = " + out_ref("n_", static_cast<int>(k), s.outs[k].handle);
  }
  out += ")\n}\n\n";
}

eastl::string sheet_line(const NodeSpec &s)
{
  eastl::string line = "- `" + s.ident + "(";
  bool first = true;
  for (const Arg &a : s.args)
  {
    if (a.kind >= Arg::Width)
    {
      break;
    }
    line += (first ? "" : ", ") + a.ident + " : " + arg_type(a, true);
    if (a.kind == Arg::Prop)
    {
      line += " = " + arg_default(a);
    }
    first = false;
  }
  line += ") : " + return_type(s) + "`";
  if (s.rules)
  {
    line += s.hasDepth ? " *3D" : s.hasHeight ? " *" : " *1D";
  }
  if (!s.op.empty())
  {
    line += " `" + s.args[0].ident + " " + s.op + " " + s.args[1].ident + "`";
  }
  return line + "\n";
}

const char *SHEET_HEAD = R"(# texgen DSL sheet

A texture graph is a daslang file that builds a graph and executes nothing:
`require texgen`, `require texgen_nodes`, then `[export] def graph()`. A node call
returns a handle to its out pin: `Tex` for a texture, `Val` for a scalar, a tuple
named by the out pins for a node with several (`b.res`). Loops and helper functions
run in the script; only the nodes they add reach the graph.

)";

const char *SHEET_RULES = R"(## Rules

- Arguments: the main texture input positional, every other one named, in the
  order of the signature. Named arguments are an ordered suffix: a name out of order
  is "no matching functions". An argument left out keeps its default.
- A handle connects a pin; a literal sets a property. An input left out is open.
  `muted(h)` connects a muted edge. A `Val` goes only to a `Val` or `Tex|Val` pin.
- Floats are `1.0`, never `1`; vectors `float2(1.0, 0.5)`.
- A node marked `*` makes a texture and also takes, after its own arguments,
  `width`, `height` (not `*1D`), `depth` (`*3D`) `: Size` and `fmt`, `wrap_mode : string`, with
  its template's defaults (mostly `parent()`, `as_width()`, `""`: the parent's).
  `"graph"` is the graph's; else `fmt` is a format, `wrap_mode` `"wrap"` or `"clamp"`.
  `Size`: `parent()`, `graph_size()`, `px(n)`, `parent() * n`, `parent() / n`,
  `as_width()` (height and depth). The same as calls on a handle: `.size(w, h)`,
  `.depth(d)`, `.fmt("ARGB8")`, `.fmt_graph()`, `.wrap(false)`, `.wrap_graph()`.
- `node(uid, name = "", inputs = 0)` adds a node of any template; `.param(name, v)`,
  `.input(pin, h)`, `.out(pin)`, `.val(pin)` fill and read it.

## Graph

| call | meaning |
|---|---|
| `texgraph(size = 1024, height = 0, fmt = "R16F", wrap = true, render_dir = "", entity_dir = "", depth = 0)` | graph options; `height`, `depth` 0 is `size` |
| `heightmap(scale, min, cell)` | heightmap preview options; `min`, `cell` left out stay unset |
)";

void write_sheet(const Specs &specs, eastl::string &out)
{
  out = SHEET_HEAD;
  out += "```das\noptions gen2\nrequire texgen\nrequire texgen_nodes\n\n[export]\ndef graph() {\n    texgraph(2048)\n";
  const NodeSpec *texNode = nullptr;
  for (const NodeSpec &s : specs.nodes)
  {
    if (!s.outs.empty() && s.outs[0].handle == Handle::Tex && s.rules)
    {
      texNode = &s;
      break;
    }
  }
  if (texNode)
  {
    out += "    let a = " + texNode->ident + "()" + (texNode->outs.size() > 1 ? "." + texNode->outs[0].ident : "") + "\n";
    if (!specs.outputUid.empty())
    {
      out += "    output(\"height\", a.size(parent() / 2))\n";
    }
    if (!specs.exportUid.empty())
    {
      out += "    export(\"mask.tif\", \"ARGB8\", a)\n";
    }
  }
  out += "}\n```\n\n";
  out += SHEET_RULES;
  if (!specs.outputUid.empty())
  {
    out += "| `output(name : string; t : Tex)` | a final texture of the graph |\n";
  }
  if (!specs.exportUid.empty())
  {
    out += "| `export(file, fmt : string; t : Tex; skip = false)` | a file the graph exports, `fmt` its channels |\n";
  }
  out += "| `final_output(uid, name, t)` `final_export(uid, file, fmt, skip, t)` | a final of any template |\n";

  eastl::vector<eastl::string> categories;
  for (const NodeSpec &s : specs.nodes)
  {
    const eastl::string c = s.category.empty() ? "Other" : s.category;
    if (eastl::find(categories.begin(), categories.end(), c) == categories.end())
    {
      categories.push_back(c);
    }
  }
  eastl::sort(categories.begin(), categories.end());
  out += "\n## Nodes\n";
  for (const eastl::string &c : categories)
  {
    out += "\n### " + c + "\n\n";
    for (const NodeSpec &s : specs.nodes)
    {
      if ((s.category.empty() ? "Other" : s.category) == c)
      {
        out += sheet_line(s);
      }
    }
  }
}
} // namespace

void generate_das_nodes(const DataBlock &base_nodes_blk, eastl::string &out_das, eastl::string &out_sheet, eastl::string &errors)
{
  Specs specs;
  build_specs(base_nodes_blk, specs, errors);

  out_das = "options gen2\n\nrequire texgen\n\n";
  for (const NodeSpec &s : specs.nodes)
  {
    write_node_function(s, out_das);
  }
  if (!specs.outputUid.empty())
  {
    out_das += "def output(name : string; t : Tex) : Tex => final_output(" + das_string(specs.outputUid.c_str()) + ", name, t)\n\n";
  }
  if (!specs.exportUid.empty())
  {
    out_das += "def export(file, fmt : string; t : Tex; skip : bool = false) : Tex => final_export(" +
               das_string(specs.exportUid.c_str()) + ", file, fmt, skip, t)\n\n";
  }
  for (const NodeSpec &s : specs.nodes)
  {
    if (!s.op.empty())
    {
      out_das += "def operator " + s.op + "(a, b : Tex) : Tex => " + s.ident + "(a, " + s.args[1].ident + " = b)\n\n";
    }
  }
  write_sheet(specs, out_sheet);
}

namespace
{
// How the script names an IR node.
struct NodeForm
{
  const NodeSpec *spec = nullptr; // the generated function, null for the raw node() form
  eastl::string var;
};

Handle source_handle(const texgen::PinRef &ref, const Specs &specs, const texgen::Graph &g)
{
  const NodeSpec *s = specs.find(g.findNode(ref.node)->tmpl.uid);
  return s && ref.pin < s->outs.size() ? s->outs[ref.pin].handle : Handle::Tex;
}

eastl::string ref_text(const texgen::PinRef &ref, const ska::flat_hash_map<int, NodeForm> &forms, const Specs &specs,
  const texgen::Graph &g)
{
  if (!ref.node)
  {
    return "Tex()";
  }
  const NodeForm &f = forms.find(ref.node.v)->second;
  const eastl::string s = !f.spec                   ? out_ref(f.var.c_str(), ref.pin, source_handle(ref, specs, g))
                          : f.spec->outs.size() > 1 ? f.var + "." + f.spec->outs[ref.pin].ident
                                                    : f.var;
  return ref.muted ? "muted(" + s + ")" : s;
}

const texgen::Param *find_param(const texgen::Node &n, const eastl::string &name)
{
  for (const texgen::Param &p : n.params)
  {
    if (p.name == name)
    {
      return &p;
    }
  }
  return nullptr;
}

// Whether the generated function carries every param, input and rule of n, with the highest out pin it is read at.
bool fits_function(const texgen::Node &n, const NodeSpec &s, int top_read_pin)
{
  if (!n.label.empty() || static_cast<int>(n.inputs.size()) > s.inCount ||
      top_read_pin >= eastl::max<int>(1, static_cast<int>(s.outs.size())))
  {
    return false;
  }
  int propCount = 0;
  for (const Arg &a : s.args)
  {
    if (a.kind != Arg::Prop)
    {
      continue;
    }
    ++propCount;
    const texgen::Param *p = find_param(n, a.prop);
    if (!p || p->value.type != a.def.type)
    {
      return false;
    }
  }
  if (propCount != static_cast<int>(n.params.size()))
  {
    return false;
  }
  if (!s.rules)
  {
    const texgen::Node defaults;
    return n.width == defaults.width && n.height == defaults.height && n.depth == defaults.depth && n.fmt == defaults.fmt &&
           n.wrap == defaults.wrap;
  }
  const texgen::SizeRule asWidth{texgen::SizeRule::Width, 0};
  return (s.hasHeight || n.height == asWidth) && (s.hasDepth || n.depth == asWidth);
}

void write_function_call(const texgen::Node &n, const NodeSpec &s, const ska::flat_hash_map<int, NodeForm> &forms, const Specs &specs,
  const texgen::Graph &g, eastl::string &out)
{
  eastl::vector<eastl::string> parts;
  for (size_t k = 0; k < s.args.size(); ++k)
  {
    const Arg &a = s.args[k];
    eastl::string value;
    switch (a.kind)
    {
      case Arg::Pin:
        if (a.pin < static_cast<int>(n.inputs.size()) && n.inputs[a.pin].node)
        {
          value = ref_text(n.inputs[a.pin], forms, specs, g);
        }
        break;
      case Arg::Prop:
      {
        const texgen::Param *p = find_param(n, a.prop);
        if (p && p->value != a.def)
        {
          value = das_value(p->value);
        }
        break;
      }
      case Arg::Width:
      case Arg::Height:
      case Arg::Depth:
      {
        const texgen::SizeRule &r = a.kind == Arg::Width ? n.width : a.kind == Arg::Height ? n.height : n.depth;
        if (!(r == a.size))
        {
          value = das_size(r);
        }
        break;
      }
      case Arg::Fmt:
        if (fmt_text(n.fmt) != a.text)
        {
          value = das_string(fmt_text(n.fmt).c_str());
        }
        break;
      case Arg::Wrap:
        if (wrap_text(n.wrap) != a.text)
        {
          value = das_string(wrap_text(n.wrap).c_str());
        }
        break;
    }
    if (value.empty())
    {
      continue;
    }
    if (k == 0 && a.texPin())
    {
      parts.push_back(value);
    }
    else
    {
      parts.push_back(a.ident + " = " + value);
    }
  }
  out += s.ident + "(";
  for (size_t k = 0; k < parts.size(); ++k)
  {
    out += (k ? ", " : "") + parts[k];
  }
  out += ")";
}

void write_raw_node(const texgen::Node &n, const NodeForm &f, const ska::flat_hash_map<int, NodeForm> &forms, const Specs &specs,
  const texgen::Graph &g, eastl::string &out)
{
  out += "    let " + f.var + " = node(" + das_string(n.tmpl.uid.c_str());
  if (!n.label.empty() || !n.inputs.empty())
  {
    out += ", " + das_string(n.label.c_str());
  }
  if (!n.inputs.empty())
  {
    out.append_sprintf(", %d", static_cast<int>(n.inputs.size()));
  }
  out += ")\n";
  for (const texgen::Param &p : n.params)
  {
    out += "    " + f.var + " |> param(" + das_string(p.name.c_str()) + ", " + das_value(p.value) + ")\n";
  }
  for (int k = 0; k < static_cast<int>(n.inputs.size()); ++k)
  {
    if (n.inputs[k].node)
    {
      out.append_sprintf("    %s |> input(%d, %s)\n", f.var.c_str(), k, ref_text(n.inputs[k], forms, specs, g).c_str());
    }
  }
  out += "    " + f.var + " |> rules(" + das_size(n.width) + ", " + das_size(n.height) + ", " + das_size(n.depth) + ", " +
         das_string(fmt_text(n.fmt).c_str()) + ", " + das_string(wrap_text(n.wrap).c_str()) + ")\n";
}

bool topological_order(const texgen::Graph &g, eastl::vector<int> &order, eastl::string &errors)
{
  const int count = static_cast<int>(g.nodes.size());
  ska::flat_hash_map<int, int> index;
  for (int i = 0; i < count; ++i)
  {
    index[g.nodes[i].id.v] = i;
  }
  eastl::vector<int> pending(count, 0);
  eastl::vector<eastl::vector<int>> succs(count);
  for (int i = 0; i < count; ++i)
  {
    for (const texgen::PinRef &r : g.nodes[i].inputs)
    {
      if (r.node)
      {
        ++pending[i];
        succs[index[r.node.v]].push_back(i);
      }
    }
  }
  // The lowest node index that is ready goes first, so the order follows the IR where it can. A placed node has -1.
  for (int placed = 0; placed < count; ++placed)
  {
    int v = -1;
    for (int i = 0; i < count && v < 0; ++i)
    {
      if (pending[i] == 0)
      {
        v = i;
      }
    }
    if (v < 0)
    {
      for (int i = 0; i < count; ++i)
      {
        if (pending[i] > 0)
        {
          errors.append_sprintf("node %d: is in or after a loop, which a straight-line script cannot build\n", g.nodes[i].id.v);
        }
      }
      return false;
    }
    pending[v] = -1;
    order.push_back(v);
    for (int s : succs[v])
    {
      --pending[s];
    }
  }
  return true;
}

} // namespace

bool graph_to_das(const texgen::Graph &g, const DataBlock &base_nodes_blk, eastl::string &out, eastl::string &errors)
{
  out.clear();
  if (!texgen::validate(g, errors))
  {
    return false;
  }
  Specs specs;
  eastl::string leftOut;
  build_specs(base_nodes_blk, specs, leftOut);
  eastl::vector<int> order;
  if (!topological_order(g, order, errors))
  {
    return false;
  }

  ska::flat_hash_map<int, int> topReadPin;
  auto noteRead = [&](const texgen::PinRef &r) {
    if (r.node)
    {
      int &top = topReadPin[r.node.v];
      top = eastl::max(top, int(r.pin));
    }
  };
  for (const texgen::Node &n : g.nodes)
  {
    for (const texgen::PinRef &r : n.inputs)
    {
      noteRead(r);
    }
  }
  for (const texgen::Final &f : g.finals)
  {
    noteRead(f.src);
  }

  ska::flat_hash_map<int, NodeForm> forms;
  for (const texgen::Node &n : g.nodes)
  {
    NodeForm &f = forms[n.id.v];
    f.var.sprintf("v%d", n.id.v);
    const NodeSpec *s = specs.find(n.tmpl.uid);
    f.spec = s && fits_function(n, *s, topReadPin[n.id.v]) ? s : nullptr;
    // A function input takes only the handle kind of its parameter; a mismatch keeps the consumer raw.
    for (size_t k = 0; f.spec && k < f.spec->args.size(); ++k)
    {
      const Arg &a = f.spec->args[k];
      if (a.kind == Arg::Pin && a.pin < static_cast<int>(n.inputs.size()) && n.inputs[a.pin].node && a.handle != Handle::Any &&
          a.handle != source_handle(n.inputs[a.pin], specs, g))
      {
        f.spec = nullptr;
      }
    }
  }

  const texgen::GraphOptions &o = g.options;
  out = "options gen2\n\nrequire texgen\nrequire texgen_nodes\n\n[export]\ndef graph() {\n";
  out.append_sprintf("    texgraph(%d", o.width);
  if (o.height != o.width)
  {
    out.append_sprintf(", height = %d", o.height);
  }
  out += ", fmt = " + das_string(o.fmt.c_str()) + ", wrap = " + (o.wrap ? "true" : "false");
  if (!o.renderDir.empty())
  {
    out += ", render_dir = " + das_string(o.renderDir.c_str());
  }
  if (!o.entityDir.empty())
  {
    out += ", entity_dir = " + das_string(o.entityDir.c_str());
  }
  if (o.depth != o.width)
  {
    out.append_sprintf(", depth = %d", o.depth);
  }
  out += ")\n";
  const float UNSET = texgen::GraphOptions::UNSET;
  if (o.heightmapScale != UNSET || o.heightmapMin != UNSET || o.heightmapCellSize != UNSET)
  {
    out += "    heightmap(" + das_float(o.heightmapScale);
    if (o.heightmapMin != UNSET)
    {
      out += ", min = " + das_float(o.heightmapMin);
    }
    if (o.heightmapCellSize != UNSET)
    {
      out += ", cell = " + das_float(o.heightmapCellSize);
    }
    out += ")\n";
  }

  for (int i : order)
  {
    const texgen::Node &n = g.nodes[i];
    const NodeForm &f = forms[n.id.v];
    if (f.spec)
    {
      out += "    let " + f.var + " = ";
      write_function_call(n, *f.spec, forms, specs, g, out);
      out += "\n";
    }
    else
    {
      write_raw_node(n, f, forms, specs, g, out);
    }
  }

  for (const texgen::Final &fin : g.finals)
  {
    const eastl::string src = ref_text(fin.src, forms, specs, g);
    const bool plain = fin.params.empty();
    eastl::string call;
    if (fin.exportSpec)
    {
      const texgen::Export &e = *fin.exportSpec;
      if (plain && fin.tmpl.uid == specs.exportUid)
      {
        call = "export(" + das_string(e.file.c_str()) + ", " + das_string(e.fmt.c_str()) + ", " + src +
               (e.skipSaving ? ", skip = true" : "") + ")";
      }
      else
      {
        call = "final_export(" + das_string(fin.tmpl.uid.c_str()) + ", " + das_string(e.file.c_str()) + ", " +
               das_string(e.fmt.c_str()) + ", " + (e.skipSaving ? "true" : "false") + ", " + src + ")";
      }
    }
    else if (plain && fin.tmpl.uid == specs.outputUid)
    {
      call = "output(" + das_string(fin.name.c_str()) + ", " + src + ")";
    }
    else
    {
      call = "final_output(" + das_string(fin.tmpl.uid.c_str()) + ", " + das_string(fin.name.c_str()) + ", " + src + ")";
    }
    if (fin.params.empty())
    {
      out += "    " + call + "\n";
      continue;
    }
    out.append_sprintf("    let f%d = %s\n", fin.id.v, call.c_str());
    for (const texgen::Param &p : fin.params)
    {
      out.append_sprintf("    f%d |> param(%s, %s)\n", fin.id.v, das_string(p.name.c_str()).c_str(), das_value(p.value).c_str());
    }
  }
  out += "}\n";
  return true;
}
