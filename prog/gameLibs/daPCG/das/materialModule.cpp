// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "materialModule_api.h"
#include <gameLibs/daCSG/das/csgModule_api.h>
#include <daScript/daScript.h>

namespace dapcg
{
struct MaterialState final : dacsg::ModelState
{
  dag::Vector<Material> table;
};

dag::ConstSpan<Material> material_table(dacsg::ModelContext &ctx) { return make_span_const(ctx.state<MaterialState>().table); }

namespace material_bind
{
using dacsg::model_call_site;
using das::Context;
using das::LineInfoArg;

static constexpr const char *MATERIAL_DAS = "material.das";

static const char *text(const char *s) { return s ? s : ""; }

MaterialState &current(Context *ctx) { return dacsg::ModelContext::of(ctx).state<MaterialState>(); }

Material &row_at(int row, Context *ctx, LineInfoArg *at, const char *bad = nullptr)
{
  dag::Vector<Material> &table = current(ctx).table;
  if (row < 0 || uint32_t(row) >= table.size())
    ctx->throw_error_at(model_call_site(ctx, at, MATERIAL_DAS, at), "material: row %d does not exist", row);
  Material &m = table[row];
  if (bad)
    ctx->throw_error_at(model_call_site(ctx, at, MATERIAL_DAS, at), "material %d \"%s\": %s", m.id, m.name.c_str(), bad);
  return m;
}

int add(int id, const char *name, const char *shader, Context *ctx, LineInfoArg *at)
{
  name = text(name);
  dag::Vector<Material> &table = current(ctx).table;
  if (const char *bad = check_new_material(make_span_const(table), id, name, text(shader)))
    ctx->throw_error_at(model_call_site(ctx, at, MATERIAL_DAS, at), "material %d \"%s\": %s", id, name, bad);
  table.push_back({.id = id, .name = name, .shader = text(shader)});
  return int(table.size() - 1);
}

int tex(int row, int slot, const char *name, Context *ctx, LineInfoArg *at)
{
  row_at(row, ctx, at, check_texture(slot, text(name))).tex[slot] = text(name);
  return row;
}

int script(int row, const char *line, Context *ctx, LineInfoArg *at)
{
  row_at(row, ctx, at, check_script_line(text(line))).script.push_back(text(line));
  return row;
}

int two_sided(int row, Context *ctx, LineInfoArg *at)
{
  row_at(row, ctx, at).twoSided = true;
  return row;
}

int look(int row, const char *name, Context *ctx, LineInfoArg *at)
{
  row_at(row, ctx, at, check_look(text(name))).look = text(name);
  return row;
}

int diffuse(int row, float r, float g, float b, Context *ctx, LineInfoArg *at)
{
  row_at(row, ctx, at, check_diffuse(Point3(r, g, b))).diffuse = Point3(r, g, b);
  return row;
}

// csg's mat would record material.das's line
int mat(int a, const char *name, Context *ctx, LineInfoArg *at)
{
  const das::LineInfo *line = model_call_site(ctx, at, MATERIAL_DAS, at);
  name = text(name);
  const dag::Vector<Material> &table = current(ctx).table;
  const Material *m = eastl::find_if(table.begin(), table.end(), [&](const Material &r) { return r.name == name; });
  if (m == table.end())
    ctx->throw_error_at(line, "material: no row named \"%s\" comes before this call", name);
  dacsg::Graph &g = dacsg::csg_graph(ctx);
  if (a < 0 || uint32_t(a) >= g.nodeCount())
    ctx->throw_error_at(line, "material: node %d does not exist", a);
  const dacsg::OpResult r = g.attr(dacsg::OpResult{uint32_t(a)}, {.kind = dacsg::AttrKind::Mat, .mat = uint16_t(m->id)});
  if (line && line->fileInfo)
    g.setSource(r, dacsg::file_name(line->fileInfo->name.c_str()), line->line);
  return int(r.id);
}
} // namespace material_bind

static char material_das[] =
#include "material.das.inl"
  ;

class PcgMaterialModule final : public das::Module
{
public:
  PcgMaterialModule() : das::Module("material")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("csg"));
    const das::SideEffects se = das::SideEffects::modifyExternal;
#define BIND(fn, ...)                                                                                            \
  das::addExtern<DAS_BIND_FUN(material_bind::fn)>(*this, lib, "material_" #fn, se, "dapcg::material_bind::" #fn) \
    ->args({__VA_ARGS__, "context", "at"})
    BIND(add, "id", "name", "shader");
    BIND(tex, "row", "slot", "name");
    BIND(script, "row", "line");
    BIND(two_sided, "row");
    BIND(look, "row", "name");
    BIND(diffuse, "row", "r", "g", "b");
    BIND(mat, "a", "name");
#undef BIND

    compileBuiltinModule(material_bind::MATERIAL_DAS, (unsigned char *)material_das, sizeof(material_das));
    // externs go private after material.das compiles
    functions.foreach([](const das::FunctionPtr &fn) { fn->privateFunction |= fn->builtIn; });
    verifyAotReady();
  }
};
} // namespace dapcg

REGISTER_MODULE_IN_NAMESPACE(PcgMaterialModule, dapcg);
