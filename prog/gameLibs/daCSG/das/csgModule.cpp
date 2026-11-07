// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "csgModule_api.h"
#include <daScript/daScript.h>
#include <math/dag_check_nan.h>
#include <osApiWrappers/dag_atomic.h>
#include <string.h>

namespace dacsg
{
static constexpr uint32_t MODEL_CONTEXT = 1u << 31; // a das category bit das does not use

ModelContext::ModelContext(uint32_t stack_size) : das::Context(stack_size) { category.value |= MODEL_CONTEXT; }

ModelContext &ModelContext::of(das::Context *ctx)
{
  if (!(ctx->category.value & MODEL_CONTEXT))
    ctx->throw_error("model() runs in a plain das::Context; the host must make a dacsg::ModelContext");
  return *static_cast<ModelContext *>(ctx);
}

uint32_t ModelContext::newSlot()
{
  static volatile int next = 0;
  return uint32_t(interlocked_increment(next)) - 1;
}

static bool in_model(const das::LineInfo *at, const char *module_das)
{
  return at && at->fileInfo && at->line && strcmp(at->fileInfo->name.c_str(), module_das) != 0;
}

// the module's das wrappers sit above the bound call
struct ModelCallSite final : das::StackWalker
{
  const char *moduleDas;
  const das::LineInfo *found = nullptr;

  explicit ModelCallSite(const char *module_das) : moduleDas(module_das) {}
  bool canWalkArguments() override { return false; }
  bool canWalkVariables() override { return false; }
  void onCallAt(das::Prologue *, das::FuncInfo *, das::LineInfo *line) override
  {
    if (!found && in_model(line, moduleDas))
      found = line;
  }
  bool onAfterCall(das::Prologue *) override { return !found; }
};

const das::LineInfo *model_call_site(das::Context *ctx, das::LineInfoArg *at, const char *module_das, const das::LineInfo *otherwise)
{
  if (in_model(at, module_das))
    return at;
  ModelCallSite walker(module_das);
  das::dapiStackWalk(&walker, *ctx, *at);
  return walker.found ? walker.found : otherwise;
}

const char *file_name(const char *path)
{
  const char *slash = strrchr(path, '/'), *back = strrchr(path, '\\');
  return back && (!slash || back > slash) ? back + 1 : slash ? slash + 1 : path;
}

namespace bind
{
using das::Context;
using das::LineInfoArg;

static constexpr const char *CSG_DAS = "csg.das";

OpResult node(int id, Context *ctx, LineInfoArg *at)
{
  if (id < 0 || uint32_t(id) >= csg_graph(ctx).nodeCount())
    ctx->throw_error_at(model_call_site(ctx, at, CSG_DAS, at), "csg: node %d does not exist", id);
  return OpResult{uint32_t(id)};
}

// site: the model call site when the caller resolved it already, one stack walk per node
int located(OpResult r, Context *ctx, LineInfoArg *at, const das::LineInfo *site = nullptr)
{
  if (const das::LineInfo *line = site ? site : model_call_site(ctx, at, CSG_DAS))
    csg_graph(ctx).setSource(r, file_name(line->fileInfo->name.c_str()), line->line);
  return r.id;
}

// a prelude guard's error at the model line, not the prelude's
void error(const char *text, Context *ctx, LineInfoArg *at)
{
  ctx->throw_error_at(model_call_site(ctx, at, CSG_DAS, at), "csg: %s", text ? text : "");
}

int box(float sx, float sy, float sz, Context *ctx, LineInfoArg *at)
{
  return located(csg_graph(ctx).box(Point3(sx, sy, sz)), ctx, at);
}
int rbox(float sx, float sy, float sz, float radius, int seg, Context *ctx, LineInfoArg *at)
{
  return located(csg_graph(ctx).rbox(Point3(sx, sy, sz), radius, seg), ctx, at);
}
int sphere(float r, int seg, Context *ctx, LineInfoArg *at) { return located(csg_graph(ctx).sphere(r, seg), ctx, at); }
int ellipsoid(float rx, float ry, float rz, int seg, Context *ctx, LineInfoArg *at)
{
  return located(csg_graph(ctx).ellipsoid(Point3(rx, ry, rz), seg), ctx, at);
}
int icosphere(float r, int level, Context *ctx, LineInfoArg *at) { return located(csg_graph(ctx).icosphere(r, level), ctx, at); }
int cubesphere(float r, int seg, Context *ctx, LineInfoArg *at) { return located(csg_graph(ctx).cubesphere(r, seg), ctx, at); }
int cylinder(float r, float h, int seg, Context *ctx, LineInfoArg *at) { return located(csg_graph(ctx).cylinder(r, h, seg), ctx, at); }
int cone(float r_bottom, float r_top, float h, int seg, Context *ctx, LineInfoArg *at)
{
  return located(csg_graph(ctx).cone(r_bottom, r_top, h, seg), ctx, at);
}
int ccyl(float r, float h, float chamfer, int seg, Context *ctx, LineInfoArg *at)
{
  return located(csg_graph(ctx).ccyl(r, h, chamfer, seg), ctx, at);
}
int capsule(float r, float h, int seg, Context *ctx, LineInfoArg *at) { return located(csg_graph(ctx).capsule(r, h, seg), ctx, at); }
int torus(float r_major, float r_minor, int seg, int seg_minor, Context *ctx, LineInfoArg *at)
{
  return located(csg_graph(ctx).torus(r_major, r_minor, seg, seg_minor), ctx, at);
}

// copied, since a cast is type punning
template <typename P, typename F>
dag::Vector<P> points(const das::TArray<F> &a)
{
  static_assert(sizeof(F) == sizeof(P));
  dag::Vector<P> p(a.size);
  if (a.size)
    memcpy(p.data(), a.data, a.size * sizeof(P));
  return p;
}

int convex(const das::TArray<das::float3> &pts, Context *ctx, LineInfoArg *at)
{
  return located(csg_graph(ctx).convex(make_span_const(points<Point3>(pts))), ctx, at);
}
int extrude(const das::TArray<das::float2> &poly, float h, Context *ctx, LineInfoArg *at)
{
  return located(csg_graph(ctx).extrude(make_span_const(points<Point2>(poly)), h), ctx, at);
}
int revolve(const das::TArray<das::float2> &profile, float angle, int seg, Context *ctx, LineInfoArg *at)
{
  return located(csg_graph(ctx).revolve(make_span_const(points<Point2>(profile)), angle, seg), ctx, at);
}
int tube(const das::TArray<das::float3> &path, float r, int seg, Context *ctx, LineInfoArg *at)
{
  return located(csg_graph(ctx).tube(make_span_const(points<Point3>(path)), r, seg), ctx, at);
}
int loft(const das::TArray<das::TArray<das::float3>> &sections, bool closed, Context *ctx, LineInfoArg *at)
{
  const uint32_t n = sections.size ? sections[0].size : 0;
  // every size before pts allocates: a build without exceptions skips its destructor on the throw
  for (uint32_t k = 0; k < sections.size; ++k)
    if (sections[k].size != n)
      ctx->throw_error_at(model_call_site(ctx, at, CSG_DAS, at), "csg: loft section %u has %u points, section 0 has %u", k,
        uint32_t(sections[k].size), n);
  static_assert(sizeof(das::float3) == sizeof(Point3));
  dag::Vector<Point3> pts(sections.size * n);
  for (uint32_t k = 0; k < sections.size && n; ++k)
    memcpy(pts.data() + k * n, sections[k].data, n * sizeof(Point3));
  return located(csg_graph(ctx).loft(make_span_const(pts), n, closed), ctx, at);
}
// a relative path is the calling file's: a model keeps its imports beside it
int mesh(const char *path, Context *ctx, LineInfoArg *at)
{
  const das::LineInfo *line = model_call_site(ctx, at, CSG_DAS);
  const bool relative = path && *path && path[0] != '/' && path[0] != '\\' && !strchr(path, ':');
  eastl::string full;
  if (relative && line)
    full.assign(line->fileInfo->name.c_str(), file_name(line->fileInfo->name.c_str()));
  return located(csg_graph(ctx).mesh((full + (path ? path : "")).c_str()), ctx, at, line);
}

// private: csg.das passes only a boolean op's kind
int op(int kind, int a, int b, Context *ctx, LineInfoArg *at)
{
  return located(csg_graph(ctx).op(NodeKind(kind), node(a, ctx, at), node(b, ctx, at)), ctx, at);
}

int cut(int a, das::float3 n, float d, Context *ctx, LineInfoArg *at)
{
  return located(csg_graph(ctx).cut(node(a, ctx, at), PlaneD{DPoint3(n.x, n.y, n.z), d}), ctx, at);
}

int transform(int a, const das::float3x4 &m, Context *ctx, LineInfoArg *at)
{
  TMatrixD tm;
  for (int i = 0; i < 4; ++i)
    tm.col[i] = DPoint3(m.m[i].x, m.m[i].y, m.m[i].z);
  return located(csg_graph(ctx).transform(node(a, ctx, at), tm), ctx, at);
}

int attr(int a, const AttrDesc &d, Context *ctx, LineInfoArg *at)
{
  return located(csg_graph(ctx).attr(node(a, ctx, at), d), ctx, at);
}

AttrDesc attr_desc(AttrKind kind, bool only_unset = false)
{
  AttrDesc d;
  d.kind = kind;
  d.onlyUnset = only_unset;
  return d;
}

int attr_color(int a, float r, float g, float b, bool only_unset, Context *ctx, LineInfoArg *at)
{
  AttrDesc d = attr_desc(AttrKind::Color, only_unset);
  d.color[0] = r;
  d.color[1] = g;
  d.color[2] = b;
  d.color[3] = 1.f;
  return attr(a, d, ctx, at);
}

int attr_mat(int a, int id, bool only_unset, Context *ctx, LineInfoArg *at)
{
  if (id < 0 || id > 0xFFFF)
    ctx->throw_error_at(model_call_site(ctx, at, CSG_DAS, at), "csg: material id %d is not in 0..65535", id);
  AttrDesc d = attr_desc(AttrKind::Mat, only_unset);
  d.mat = uint16_t(id);
  return attr(a, d, ctx, at);
}

int attr_flat(int a, Context *ctx, LineInfoArg *at) { return attr(a, attr_desc(AttrKind::Flat), ctx, at); }
int attr_smooth(int a, Context *ctx, LineInfoArg *at) { return attr(a, attr_desc(AttrKind::Smooth), ctx, at); }

int attr_projection(int a, AttrKind kind, float scale, Context *ctx, LineInfoArg *at, const Point3 &axis = AttrDesc().axis)
{
  AttrDesc d = attr_desc(kind);
  d.axis = axis;
  d.scale = scale;
  return attr(a, d, ctx, at);
}

int attr_uv_planar(int a, das::float3 axis, float scale, Context *ctx, LineInfoArg *at)
{
  return attr_projection(a, AttrKind::UvPlanar, scale, ctx, at, Point3(axis.x, axis.y, axis.z));
}
int attr_uv_box(int a, float scale, Context *ctx, LineInfoArg *at) { return attr_projection(a, AttrKind::UvBox, scale, ctx, at); }
int attr_uv_cyl(int a, float scale, Context *ctx, LineInfoArg *at) { return attr_projection(a, AttrKind::UvCyl, scale, ctx, at); }
int attr_uv_sphere(int a, Context *ctx, LineInfoArg *at) { return attr(a, attr_desc(AttrKind::UvSphere), ctx, at); }

int attr_uv_affine(int a, float uu, float uv, float u0, float vu, float vv, float v0, Context *ctx, LineInfoArg *at)
{
  AttrDesc d = attr_desc(AttrKind::UvAffine);
  const float rows[2][3] = {{uu, uv, u0}, {vu, vv, v0}};
  memcpy(d.uvAffine, rows, sizeof(rows));
  return attr(a, d, ctx, at);
}

void name(int a, const char *text, Context *ctx, LineInfoArg *at) { csg_graph(ctx).setName(node(a, ctx, at), text ? text : ""); }
void set_quality(float q, Context *ctx, LineInfoArg *at)
{
  if (!check_finite(q) || !(q > 0))
    ctx->throw_error_at(model_call_site(ctx, at, CSG_DAS, at), "csg: quality must be finite and above zero, not %g", q);
  csg_graph(ctx).setQuality(q);
}
} // namespace bind

static char csg_das[] =
#include "csg.das.inl"
  ;

class CsgModule final : public das::Module
{
public:
  CsgModule() : das::Module("csg")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("math"));
    das::addConstant(*this, "CSG_UNION", int(NodeKind::Union));
    das::addConstant(*this, "CSG_SUBTRACT", int(NodeKind::Subtract));
    das::addConstant(*this, "CSG_INTERSECT", int(NodeKind::Intersect));

    const das::SideEffects se = das::SideEffects::modifyExternal;
#define BIND(fn, ...)      das::addExtern<DAS_BIND_FUN(bind::fn)>(*this, lib, "csg_" #fn, se, "dacsg::bind::" #fn)->args({__VA_ARGS__})
#define BIND_NODE(fn, ...) BIND(fn, __VA_ARGS__, "context", "at")
    BIND_NODE(box, "sx", "sy", "sz");
    BIND_NODE(rbox, "sx", "sy", "sz", "radius", "seg");
    BIND_NODE(sphere, "r", "seg");
    BIND_NODE(ellipsoid, "rx", "ry", "rz", "seg");
    BIND_NODE(icosphere, "r", "level");
    BIND_NODE(cubesphere, "r", "seg");
    BIND_NODE(cylinder, "r", "h", "seg");
    BIND_NODE(cone, "rBottom", "rTop", "h", "seg");
    BIND_NODE(ccyl, "r", "h", "chamfer", "seg");
    BIND_NODE(capsule, "r", "h", "seg");
    BIND_NODE(torus, "R", "r", "seg", "segMinor");
    BIND_NODE(convex, "points");
    BIND_NODE(extrude, "poly", "h");
    BIND_NODE(revolve, "profile", "angle", "seg");
    BIND_NODE(tube, "path", "r", "seg");
    BIND_NODE(loft, "sections", "closed");
    BIND_NODE(mesh, "path");
    BIND_NODE(op, "kind", "a", "b");
    BIND_NODE(cut, "a", "n", "d");
    BIND_NODE(transform, "a", "m");
    BIND_NODE(attr_color, "a", "r", "g", "b", "onlyUnset");
    BIND_NODE(attr_mat, "a", "id", "onlyUnset");
    BIND_NODE(attr_flat, "a");
    BIND_NODE(attr_smooth, "a");
    BIND_NODE(attr_uv_planar, "a", "axis", "scale");
    BIND_NODE(attr_uv_box, "a", "scale");
    BIND_NODE(attr_uv_cyl, "a", "scale");
    BIND_NODE(attr_uv_sphere, "a");
    BIND_NODE(attr_uv_affine, "a", "uu", "uv", "u0", "vu", "vv", "v0");
    BIND(name, "a", "name", "context", "at");
    BIND(set_quality, "q", "context", "at")->needCallerStackFrame = true; // a fastcall wrapper hides the model line
    BIND(error, "text", "context", "at")->needCallerStackFrame = true;
#undef BIND_NODE
#undef BIND

    compileBuiltinModule(bind::CSG_DAS, (unsigned char *)csg_das, sizeof(csg_das));
    // csg.das compiles as a program of its own, so the externs turn private only now: a model sees the Solid DSL
    functions.foreach([](const das::FunctionPtr &fn) { fn->privateFunction |= fn->builtIn; });
    verifyAotReady();
  }
};
} // namespace dacsg

REGISTER_MODULE_IN_NAMESPACE(CsgModule, dacsg);
