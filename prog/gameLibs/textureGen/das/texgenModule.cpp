// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "texgenModule_api.h"
#include <gameLibs/textureGen/graph/texgenGraph_api.h>
#include <daScript/daScript.h>
#include <string.h>

namespace texgen
{
static thread_local Graph current_graph;

Graph &texgen_graph() { return current_graph; }

void texgen_graph_reset() { current_graph = Graph{}; }

namespace bind
{
using das::Context;
using das::LineInfoArg;

static constexpr const char *TEXGEN_DAS = "texgen.das";
static constexpr int MAX_PIN = 1023;

// Both library files carry bare names, so a script path that ends in one is not taken for it.
static bool is_library_file(const char *name) { return strcmp(name, TEXGEN_DAS) == 0 || strcmp(name, NODES_DAS) == 0; }

static bool in_script(const das::LineInfo *at)
{
  return at && at->fileInfo && at->line && !is_library_file(at->fileInfo->name.c_str());
}

// texgen.das and texgen_nodes.das wrappers sit above the bound call
struct ScriptCallSite final : das::StackWalker
{
  const das::LineInfo *found = nullptr;

  bool canWalkArguments() override { return false; }
  bool canWalkVariables() override { return false; }
  void onCallAt(das::Prologue *, das::FuncInfo *, das::LineInfo *line) override
  {
    if (!found && in_script(line))
      found = line;
  }
  bool onAfterCall(das::Prologue *) override { return !found; }
};

static const das::LineInfo *script_call_site(Context *ctx, LineInfoArg *at)
{
  if (in_script(at))
    return at;
  ScriptCallSite walker;
  das::dapiStackWalk(&walker, *ctx, *at);
  return walker.found ? walker.found : at;
}

#define TEXGEN_FAIL(...) ctx->throw_error_at(script_call_site(ctx, at), "texgen: " __VA_ARGS__)

static Node *find_node(int id) { return const_cast<Node *>(texgen_graph().findNode(NodeId{id})); }
static Final *find_final(int id) { return const_cast<Final *>(texgen_graph().findFinal(NodeId{id})); }

static Node &node_of(int id, Context *ctx, LineInfoArg *at)
{
  Node *n = find_node(id);
  if (!n)
    TEXGEN_FAIL("node %d %s", id, find_final(id) ? "is a final" : "does not exist");
  return *n;
}

static PinRef source_ref(int src_id, int src_pin, bool muted, Context *ctx, LineInfoArg *at)
{
  if (src_id == 0)
    return PinRef{};
  if (!find_node(src_id))
    TEXGEN_FAIL("source node %d %s", src_id, find_final(src_id) ? "is a final" : "does not exist");
  if (src_pin < 0 || src_pin > MAX_PIN)
    TEXGEN_FAIL("out pin %d of node %d is not in 0..%d", src_pin, src_id, MAX_PIN);
  return PinRef{NodeId{src_id}, uint16_t(src_pin), muted};
}

static void graph_options(int w, int h, int d, const char *fmt, bool wrap, const char *render_dir, const char *entity_dir,
  Context *ctx, LineInfoArg *at)
{
  if (w <= 0 || h <= 0 || d <= 0)
    TEXGEN_FAIL("graph size %d x %d x %d is not above zero", w, h, d);
  GraphOptions &o = texgen_graph().options;
  o.width = w;
  o.height = h;
  o.depth = d;
  o.fmt = fmt ? fmt : "";
  o.wrap = wrap;
  o.renderDir = render_dir ? render_dir : "";
  o.entityDir = entity_dir ? entity_dir : "";
}

static void heightmap(float scale, float min, float cell)
{
  GraphOptions &o = texgen_graph().options;
  o.heightmapScale = scale;
  o.heightmapMin = min;
  o.heightmapCellSize = cell;
}

static int node(const char *uid, const char *label, int inputs, Context *ctx, LineInfoArg *at)
{
  if (!uid || !*uid)
    TEXGEN_FAIL("a node needs a template uid");
  if (inputs < 0 || inputs > MAX_PIN + 1)
    TEXGEN_FAIL("a node has 0..%d in pins, not %d", MAX_PIN + 1, inputs);
  Graph &g = texgen_graph();
  Node &n = g.nodes.push_back();
  n.id = NodeId{int(g.nodes.size() + g.finals.size())};
  n.tmpl.uid = uid;
  n.label = label ? label : "";
  n.inputs.resize(inputs);
  return n.id.v;
}

static void set_param(int id, const char *name, const Value &v, Context *ctx, LineInfoArg *at)
{
  eastl::vector<Param> *params = nullptr;
  if (Node *n = find_node(id))
    params = &n->params;
  else if (Final *f = find_final(id))
    params = &f->params;
  else
    TEXGEN_FAIL("node %d does not exist", id);
  const char *key = name ? name : "";
  for (Param &p : *params)
    if (p.name == key)
    {
      p.value = v;
      return;
    }
  params->push_back({eastl::string(key), v});
}

static void param_int(int id, const char *name, int v, Context *ctx, LineInfoArg *at)
{
  set_param(id, name, Value::makeInt(v), ctx, at);
}
static void param_float(int id, const char *name, float v, Context *ctx, LineInfoArg *at)
{
  set_param(id, name, Value::makeFloat(v), ctx, at);
}
static void param_bool(int id, const char *name, bool v, Context *ctx, LineInfoArg *at)
{
  set_param(id, name, Value::makeBool(v), ctx, at);
}
static void param_floats(int id, const char *name, int count, das::float4 v, Context *ctx, LineInfoArg *at)
{
  const float f[4] = {v.x, v.y, v.z, v.w};
  set_param(id, name, Value::makeFloats(Value::Type(Value::Float2 + count - 2), f), ctx, at);
}
static void param_string(int id, const char *name, const char *v, Context *ctx, LineInfoArg *at)
{
  set_param(id, name, Value::makeString(v), ctx, at);
}

static void connect(int id, int in_pin, int src_id, int src_pin, bool muted, Context *ctx, LineInfoArg *at)
{
  Node &n = node_of(id, ctx, at);
  if (in_pin < 0 || in_pin > MAX_PIN)
    TEXGEN_FAIL("in pin %d of node %d is not in 0..%d", in_pin, id, MAX_PIN);
  const PinRef ref = source_ref(src_id, src_pin, muted, ctx, at);
  if (n.inputs.size() <= size_t(in_pin))
    n.inputs.resize(in_pin + 1);
  n.inputs[in_pin] = ref;
}

static void size(int id, int which, int kind, int value, Context *ctx, LineInfoArg *at)
{
  Node &n = node_of(id, ctx, at);
  if (which < 0 || which > 2 || kind < SizeRule::Explicit || kind > SizeRule::Width || (which == 0 && kind == SizeRule::Width))
    TEXGEN_FAIL("size rule %d for dimension %d is not valid; as_width() is for height and depth only", kind, which);
  const bool valued = kind == SizeRule::Explicit || kind == SizeRule::ParentMul || kind == SizeRule::ParentDiv;
  if (valued && value <= 0)
    TEXGEN_FAIL("size %d is not above zero", value);
  (which == 0 ? n.width : which == 1 ? n.height : n.depth) = SizeRule{SizeRule::Kind(kind), valued ? value : 0};
}

static void fmt(int id, int kind, const char *f, Context *ctx, LineInfoArg *at)
{
  Node &n = node_of(id, ctx, at);
  if (kind < FmtRule::Explicit || kind > FmtRule::Graph)
    TEXGEN_FAIL("format rule %d is not valid", kind);
  if (kind == FmtRule::Explicit && (!f || !*f))
    TEXGEN_FAIL("an explicit format needs a name");
  n.fmt = FmtRule{FmtRule::Kind(kind), kind == FmtRule::Explicit ? f : ""};
}

static void wrap(int id, int kind, bool w, Context *ctx, LineInfoArg *at)
{
  Node &n = node_of(id, ctx, at);
  if (kind < WrapRule::Explicit || kind > WrapRule::Graph)
    TEXGEN_FAIL("wrap rule %d is not valid", kind);
  n.wrap = WrapRule{WrapRule::Kind(kind), kind == WrapRule::Explicit ? w : true};
}

static Final &add_final(const char *uid, int src_id, int src_pin, bool muted, Context *ctx, LineInfoArg *at)
{
  if (!uid || !*uid)
    TEXGEN_FAIL("a final needs a template uid");
  const PinRef src = source_ref(src_id, src_pin, muted, ctx, at);
  Graph &g = texgen_graph();
  Final &f = g.finals.push_back();
  f.id = NodeId{int(g.nodes.size() + g.finals.size())};
  f.tmpl.uid = uid;
  f.src = src;
  return f;
}

static int final(const char *uid, const char *name, int src_id, int src_pin, bool muted, Context *ctx, LineInfoArg *at)
{
  Final &f = add_final(uid, src_id, src_pin, muted, ctx, at);
  f.name = name ? name : "";
  return f.id.v;
}

// The final name is the file without its extension, as the editor splits "texture name" and "file format".
static int export_(const char *uid, const char *file, const char *f, bool skip, int src_id, int src_pin, bool muted, Context *ctx,
  LineInfoArg *at)
{
  if (!file || !*file)
    TEXGEN_FAIL("an export needs a file name");
  Final &fin = add_final(uid, src_id, src_pin, muted, ctx, at);
  const char *dot = strrchr(file, '.');
  const char *slash = strpbrk(dot ? dot : file, "/\\");
  fin.name = dot && !slash && dot != file ? eastl::string(file, dot) : eastl::string(file);
  fin.exportSpec = Export{eastl::string(file), eastl::string(f ? f : ""), skip};
  return fin.id.v;
}

static void error(const char *text, Context *ctx, LineInfoArg *at) { TEXGEN_FAIL("%s", text ? text : ""); }

#undef TEXGEN_FAIL
} // namespace bind

static char texgen_das[] =
#include "texgen.das.inl"
  ;

class TexgenModule final : public das::Module
{
public:
  TexgenModule() : das::Module("texgen")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("math"));
    das::addConstant(*this, "TEXGEN_SIZE_EXPLICIT", int(SizeRule::Explicit));
    das::addConstant(*this, "TEXGEN_SIZE_PARENT", int(SizeRule::Parent));
    das::addConstant(*this, "TEXGEN_SIZE_GRAPH", int(SizeRule::Graph));
    das::addConstant(*this, "TEXGEN_SIZE_PARENT_MUL", int(SizeRule::ParentMul));
    das::addConstant(*this, "TEXGEN_SIZE_PARENT_DIV", int(SizeRule::ParentDiv));
    das::addConstant(*this, "TEXGEN_SIZE_WIDTH", int(SizeRule::Width));
    das::addConstant(*this, "TEXGEN_RULE_EXPLICIT", int(FmtRule::Explicit));
    das::addConstant(*this, "TEXGEN_RULE_PARENT", int(FmtRule::Parent));
    das::addConstant(*this, "TEXGEN_RULE_GRAPH", int(FmtRule::Graph));
    static_assert(int(FmtRule::Explicit) == int(WrapRule::Explicit) && int(FmtRule::Parent) == int(WrapRule::Parent) &&
                  int(FmtRule::Graph) == int(WrapRule::Graph));

    const das::SideEffects se = das::SideEffects::modifyExternal;
    // a fastcall wrapper would hide the script line from the stack walk
#define BIND(fn, ...)                                                                         \
  das::addExtern<DAS_BIND_FUN(bind::fn)>(*this, lib, "texgen_" #fn, se, "texgen::bind::" #fn) \
    ->args({__VA_ARGS__})                                                                     \
    ->needCallerStackFrame = true
    BIND(graph_options, "w", "h", "d", "fmt", "wrap", "render_dir", "entity_dir", "context", "at");
    das::addExtern<DAS_BIND_FUN(bind::heightmap)>(*this, lib, "texgen_heightmap", se, "texgen::bind::heightmap")
      ->args({"scale", "min", "cell"});
    BIND(node, "uid", "name", "inputs", "context", "at");
    BIND(param_int, "id", "name", "v", "context", "at");
    BIND(param_float, "id", "name", "v", "context", "at");
    BIND(param_bool, "id", "name", "v", "context", "at");
    BIND(param_floats, "id", "name", "count", "v", "context", "at");
    BIND(param_string, "id", "name", "v", "context", "at");
    BIND(connect, "id", "in_pin", "src_id", "src_pin", "muted", "context", "at");
    BIND(size, "id", "which", "kind", "value", "context", "at");
    BIND(fmt, "id", "kind", "fmt", "context", "at");
    BIND(wrap, "id", "kind", "wrap", "context", "at");
    BIND(final, "uid", "name", "src_id", "src_pin", "muted", "context", "at");
    BIND(export_, "uid", "file", "fmt", "skip", "src_id", "src_pin", "muted", "context", "at");
    BIND(error, "text", "context", "at");
#undef BIND

    compileBuiltinModule(bind::TEXGEN_DAS, (unsigned char *)texgen_das, sizeof(texgen_das));
    // texgen.das compiles as a program of its own, so the externs and constants turn private only now
    functions.foreach([](const das::FunctionPtr &fn) { fn->privateFunction |= fn->builtIn; });
    globals.foreach([](const das::VariablePtr &v) { v->private_variable = true; });
    verifyAotReady();
  }
};
} // namespace texgen

REGISTER_MODULE_IN_NAMESPACE(TexgenModule, texgen);
