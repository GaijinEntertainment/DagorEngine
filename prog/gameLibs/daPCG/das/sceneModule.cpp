// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "sceneModule_api.h"
#include <gameLibs/daCSG/das/csgModule_api.h>
#include <daScript/daScript.h>

namespace dapcg
{
struct SceneState final : dacsg::ModelState
{
  dag::Vector<SceneNode> nodes;
};

dag::ConstSpan<SceneNode> scene_nodes(dacsg::ModelContext &ctx) { return make_span_const(ctx.state<SceneState>().nodes); }

namespace bind
{
using dacsg::model_call_site;
using das::Context;
using das::LineInfoArg;

static constexpr const char *SCENE_DAS = "scene.das";

SceneState &current(Context *ctx) { return dacsg::ModelContext::of(ctx).state<SceneState>(); }

SceneNode &node_at(int id, Context *ctx, LineInfoArg *at)
{
  dag::Vector<SceneNode> &nodes = current(ctx).nodes;
  if (id < 0 || uint32_t(id) >= nodes.size())
    ctx->throw_error_at(model_call_site(ctx, at, SCENE_DAS, at), "scene: node %d does not exist", id);
  return nodes[id];
}

int add(int parent, const char *name, int mesh, const dacsg::TMatrixD &wtm, Context *ctx, LineInfoArg *at)
{
  const das::LineInfo *line = model_call_site(ctx, at, SCENE_DAS, at);
  dag::Vector<SceneNode> &nodes = current(ctx).nodes;
  name = name ? name : "";
  if (const char *bad = check_node_name(make_span_const(nodes), name))
    ctx->throw_error_at(line, "scene: node \"%s\": %s", name, bad);
  if (mesh < 0 || uint32_t(mesh) >= dacsg::csg_graph(ctx).nodeCount())
    ctx->throw_error_at(line, "scene: node \"%s\": mesh %d is not a Solid of this model", name, mesh);
  nodes.push_back({.name = name, .parent = parent, .wtm = wtm, .mesh = {uint32_t(mesh)}});
  if (line && line->fileInfo)
    nodes.back().site.sprintf("%s:%d", dacsg::file_name(line->fileInfo->name.c_str()), int(line->line));
  return int(nodes.size() - 1);
}

int node(const char *name, int mesh, Context *ctx, LineInfoArg *at) { return add(-1, name, mesh, TMD_IDENT, ctx, at); }

int child(int parent, const char *name, int mesh, das::float3 p, das::float3 axis, int kind, Context *ctx, LineInfoArg *at)
{
  node_at(parent, ctx, at);
  dacsg::TMatrixD wtm;
  if (const char *bad = pivot_frame(kind, DPoint3(p.x, p.y, p.z), DPoint3(axis.x, axis.y, axis.z), wtm))
    ctx->throw_error_at(model_call_site(ctx, at, SCENE_DAS, at), "scene: node \"%s\": %s", name ? name : "", bad);
  return add(parent, name, mesh, wtm, ctx, at);
}

int collision(int n, int kind, Context *ctx, LineInfoArg *at)
{
  static_assert(int(CollisionKind::Count) == 7, "scene.das enum Collision lists each kind");
  SceneNode &node = node_at(n, ctx, at);
  if (kind < int(CollisionKind::None) || kind >= int(CollisionKind::Count))
    ctx->throw_error_at(model_call_site(ctx, at, SCENE_DAS, at),
      "scene: collision kind %d is not none, mesh, box, convex, capsule or sphere", kind);
  node.collision = CollisionKind(kind);
  return n;
}

int animated(int n, Context *ctx, LineInfoArg *at)
{
  node_at(n, ctx, at).animated = true;
  return n;
}

int script(int n, const char *text, Context *ctx, LineInfoArg *at)
{
  SceneNode &node = node_at(n, ctx, at);
  if (!node.model_script.empty())
    node.model_script += SCRIPT_EOL;
  node.model_script += text ? text : "";
  return n;
}
} // namespace bind

static char scene_das[] =
#include "scene.das.inl"
  ;

class PcgSceneModule final : public das::Module
{
public:
  PcgSceneModule() : das::Module("scene")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("csg"));
    const das::SideEffects se = das::SideEffects::modifyExternal;
#define BIND(fn, ...) \
  das::addExtern<DAS_BIND_FUN(bind::fn)>(*this, lib, "scene_" #fn, se, "dapcg::bind::" #fn)->args({__VA_ARGS__, "context", "at"})
    BIND(node, "name", "mesh");
    BIND(child, "parent", "name", "mesh", "p", "axis", "kind");
    BIND(collision, "n", "kind");
    BIND(animated, "n");
    BIND(script, "n", "text");
#undef BIND

    compileBuiltinModule(bind::SCENE_DAS, (unsigned char *)scene_das, sizeof(scene_das));
    // scene.das compiles as a program of its own, so the externs turn private only now
    functions.foreach([](const das::FunctionPtr &fn) { fn->privateFunction |= fn->builtIn; });
    verifyAotReady();
  }
};
} // namespace dapcg

REGISTER_MODULE_IN_NAMESPACE(PcgSceneModule, dapcg);
