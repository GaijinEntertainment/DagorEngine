// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameDebugRender/debugDraw.h>
#include "hudprim/dag_hudPrimitives.h"
#include <dag/dag_vector.h>
#include <math/dag_bounds3.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_shaderBlock.h>
#include <math/integer/dag_IPoint2.h>
#include <debug/dag_debug3d.h>
#include <memory/dag_framemem.h>
#include <util/dag_fastNameMapTS.h>
#include <util/dag_console.h>
#include <osApiWrappers/dag_spinlock.h>
#include <EASTL/string.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_viewScissor.h>
#include <shaders/dag_overrideStates.h>

namespace debug_draw
{
enum ElemType : int
{
  BOX = 0,
  SPHERE = 1,
  PROJ_TM = 2,
  TM_AXIS = 3,
  TEXT_2D = 4,
  TEXT_3D = 5,
  FUNC = 6,
  CYLINDER = 7,
  SQUARE = 8
};

struct Snapshot
{
  int w = 0;
  int h = 0;
  float scale = 0.33f;
  UniqueTex tex;
};

struct Elem
{
  ElemType type;

  BBox3 box;
  TMatrix4 tm;
  uint32_t color;
  float time;
  bool depth;
  dag::Vector<Point3> pos;
  dag::Vector<int> complexities;
  dag::Vector<eastl::string> text;
  eastl::function<void()> func;
  DebugMultiTextOverlay cfg;
};

struct System
{
  bool active;
  bool autoToggle;
  int sysId;
  int cmdId;
  dag::Vector<Elem> elems;
  dag::Vector<Snapshot> snapshots;
};

struct Context
{
  FastNameMapTS<false> names;
  dag::Vector<System> systems;
  int activeSystem = -1;
  int activeSystemRefcount = 0;
  float lastGlobalTime = 0.f;
  eastl::unique_ptr<PostFxRenderer> showTexRenderer;
  shaders::UniqueOverrideStateId scissorState;

  OSSpinlock critSec;
  eastl::vector<console::CommandOptions> cmdOptions = {}; // For better command tips
} g_ctx;

int get_active_nolock(eastl::string_view sys, eastl::string_view cmd)
{
  int sysId = g_ctx.names.getNameId(sys.data());
  int cmdId = cmd.empty() ? -1 : g_ctx.names.getNameId(cmd.data());

  for (int i = 0; i < g_ctx.systems.size(); ++i)
  {
    const System &s = g_ctx.systems[i];
    if (s.sysId == sysId && s.cmdId == cmdId && s.active)
      return i;
  }

  return -1;
}

bool active(eastl::string_view sys, eastl::string_view cmd)
{
  OSSpinlockScopedLock lock(g_ctx.critSec);
  return get_active_nolock(sys, cmd) != -1;
}

void register_cmd(eastl::string_view sys, eastl::string_view cmd, bool auto_toggle)
{
  OSSpinlockScopedLock lock(g_ctx.critSec);
  G_ASSERT_RETURN(!sys.empty(), );

  int sysId = g_ctx.names.addNameId(sys.data());
  int cmdId = cmd.empty() ? -1 : g_ctx.names.addNameId(cmd.data());
  for (const System &s : g_ctx.systems)
  {
    if (s.sysId == sysId && s.cmdId == cmdId)
      return;
  }

  g_ctx.systems.push_back({false, auto_toggle, sysId, cmdId});
  debug("render: debug: register new system: %s - %s| registered: %d", sys, cmd, g_ctx.systems.size());

  for (console::CommandOptions &it : g_ctx.cmdOptions)
  {
    if (it.name == sys.data())
    {
      it.subOptions.push_back(console::CommandOptions(cmd.data()));
      return;
    }
  }

  if (cmd.empty())
  {
    g_ctx.cmdOptions.push_back(console::CommandOptions(sys.data()));
    return;
  }

  g_ctx.cmdOptions.emplace_back(sys.data(), std::initializer_list<console::CommandOptions>({console::CommandOptions(cmd.data())}));
}

void on_console_cmd(const char *argv[], int argc)
{
  OSSpinlockScopedLock lock(g_ctx.critSec);
  if (argc == 1)
  {
    console::print_d("available commands:");
    dag::Vector<int, framemem_allocator> done;
    for (const System &s : g_ctx.systems)
    {
      if (eastl::find(done.begin(), done.end(), s.sysId) != done.end())
        continue;

      done.push_back(s.sysId);
      console::print_d("render.debug %s", g_ctx.names.getName(s.sysId));
    }

    return;
  }

  int sysId = g_ctx.names.getNameId(argv[1]);
  if (sysId == -1)
  {
    console::print_d("unknown system: %s", argv[1]);
    return;
  }

  int cmdId = -1;

  bool forceOn = false;
  bool forceOff = false;

  if (argc > 2 && (strcmp(argv[2], "on") == 0 || strcmp(argv[2], "1") == 0))
  {
    forceOn = true;
  }
  else if (argc > 2 && (strcmp(argv[2], "off") == 0 || strcmp(argv[2], "0") == 0))
  {
    forceOff = true;
  }
  else if (argc > 2)
  {
    cmdId = g_ctx.names.getNameId(argv[2]);
    if (cmdId == -1)
    {
      console::print_d("unknown command: %s", argv[2]);
      return;
    }
  }

  dag::Vector<int, framemem_allocator> commands;
  for (int i = 0; i < g_ctx.systems.size(); ++i)
  {
    const System &sys = g_ctx.systems[i];
    if (sys.sysId == sysId && (cmdId == -1 || cmdId == sys.cmdId))
      commands.push_back(i);
  }

  if (commands.empty())
  {
    console::print_d("command not found");
    return;
  }

  for (int i : commands)
  {
    System &sys = g_ctx.systems[i];
    if (cmdId >= 0 || sys.autoToggle)
      sys.active = forceOn ? true : (forceOff ? false : !sys.active);
    console::print_d("render.debug %s %s %s", g_ctx.names.getName(sys.sysId), sys.cmdId >= 0 ? g_ctx.names.getName(sys.cmdId) : "",
      sys.active ? "on" : "off");
  }
}

void draw_snapshots(const System &sys, const IPoint2 &target_size)
{
  if (sys.snapshots.empty())
    return;

  G_ASSERT(g_ctx.showTexRenderer);

  ShaderGlobal::set_int(::get_shader_variable_id("swizzled_texture_type"), 0);
  ShaderGlobal::set_float4(::get_shader_variable_id("swizzled_texture_face_mip"), Point4(0, 0, 0, 0));
  ShaderGlobal::set_float4(::get_shader_variable_id("tex_swizzling_r"), Point4(1, 0, 0, 0));
  ShaderGlobal::set_float4(::get_shader_variable_id("tex_swizzling_g"), Point4(0, 1, 0, 0));
  ShaderGlobal::set_float4(::get_shader_variable_id("tex_swizzling_b"), Point4(0, 0, 1, 0));
  ShaderGlobal::set_float4(::get_shader_variable_id("tex_swizzling_a"), Point4(0, 0, 0, 1));
  ShaderGlobal::set_float4(::get_shader_variable_id("tex_mod_r"), Point4(1, 0, 0, 0));
  ShaderGlobal::set_float4(::get_shader_variable_id("tex_mod_g"), Point4(1, 0, 0, 0));
  ShaderGlobal::set_float4(::get_shader_variable_id("tex_mod_b"), Point4(1, 0, 0, 0));
  ShaderGlobal::set_float4(::get_shader_variable_id("tex_mod_a"), Point4(0, 1, 0, 0));
  ShaderGlobal::set_float4(::get_shader_variable_id("swizzled_texture_tc_scale"), Point4(1.f, 1.f, 0.f, 0.f));

  shaders::overrides::set(g_ctx.scissorState);
  for (const Snapshot &i : sys.snapshots)
  {
    ShaderGlobal::set_texture(::get_shader_variable_id("swizzled_texture"), i.tex);

    int w = i.w > 0 ? i.w : target_size.x * i.scale;
    int h = i.h > 0 ? i.h : target_size.y * i.scale;

    Point2 s = Point2((float)w / target_size.x, (float)h / target_size.y);
    ShaderGlobal::set_float4(::get_shader_variable_id("swizzled_texture_size"), Point4(s.x, s.y, s.x - 1.f, 1.f - s.y));

    d3d::setscissor(0, 0, w, h);
    g_ctx.showTexRenderer->render();
  }
  shaders::overrides::reset();
}

void draw(const System &sys, HudPrimitives *imm_prim, const TMatrix4 &globtm, const IPoint2 &target_size)
{
  auto draw_func = [](const dag::Vector<Elem> &elems, bool depth) {
    begin_draw_cached_debug_lines(depth, false);
    set_cached_debug_lines_wtm(TMatrix::IDENT);

    for (const Elem &e : elems)
    {
      if (e.depth != depth)
        continue;

      if (e.type == ElemType::BOX)
      {
        set_cached_debug_lines_wtm(tmatrix(e.tm));
        draw_cached_debug_box(e.box, e.color);
        set_cached_debug_lines_wtm(TMatrix::IDENT);
      }

      if (e.type == ElemType::SPHERE)
        draw_cached_debug_sphere(e.box.center(), e.box.width().x * 0.5f, e.color);

      if (e.type == ElemType::CYLINDER)
      {
        draw_cached_debug_cylinder(tmatrix(e.tm), e.box.boxMin().x, e.box.boxMin().y, e.color);
      }

      if (e.type == ElemType::PROJ_TM)
        draw_cached_debug_proj_matrix(e.tm, e.color, e.color, e.color);

      if (e.type == ElemType::TM_AXIS)
      {
        Point3 pos = Point3::xyz(e.tm.getcol(3));
        Point3 xAsix = Point3::xyz(e.tm.getcol(0));
        Point3 yAsix = Point3::xyz(e.tm.getcol(1));
        Point3 zAsix = Point3::xyz(e.tm.getcol(2));

        float k = 0.5f;
        draw_cached_debug_line(pos, pos + xAsix * k, E3DCOLOR(255, 0, 0, 255));
        draw_cached_debug_line(pos, pos + yAsix * k, E3DCOLOR(0, 255, 0, 255));
        draw_cached_debug_line(pos, pos + zAsix * k, E3DCOLOR(0, 0, 255, 255));

        draw_cached_debug_line(pos, pos + normalize(xAsix) * k, E3DCOLOR(255, 0, 0, 64));
        draw_cached_debug_line(pos, pos + normalize(yAsix) * k, E3DCOLOR(0, 255, 0, 64));
        draw_cached_debug_line(pos, pos + normalize(zAsix) * k, E3DCOLOR(0, 0, 255, 64));
      }

      if (e.type == ElemType::SQUARE)
      {
        Point3 pos = e.box.center();
        Point3 xAxis = Point3::xyz(e.tm.getcol(0));
        Point3 zAxis = Point3::xyz(e.tm.getcol(2));
        float k = e.box.width().x * 0.5f;

        draw_cached_debug_line(pos + xAxis * k + zAxis * k, pos - xAxis * k + zAxis * k, e.color);
        draw_cached_debug_line(pos + xAxis * k - zAxis * k, pos - xAxis * k - zAxis * k, e.color);
        draw_cached_debug_line(pos + xAxis * k + zAxis * k, pos + xAxis * k - zAxis * k, e.color);
        draw_cached_debug_line(pos - xAxis * k + zAxis * k, pos - xAxis * k - zAxis * k, e.color);
      }
    }

    end_draw_cached_debug_lines();
  };

  draw_func(sys.elems, true);
  draw_func(sys.elems, false);

  imm_prim->beginRenderImm();
  float textPadding = imm_prim->getColoredTextBBox(" ", 0).y;
  for (const Elem &e : sys.elems)
  {
    if (e.type != ElemType::TEXT_2D)
      continue;

    G_ASSERT_CONTINUE(!e.text.empty());
    const eastl::string &src = e.text.back();
    dag::Vector<eastl::string, framemem_allocator> lines;

    size_t from = 0;
    size_t to = src.find("\n");
    size_t sz = src.size();
    while (to != eastl::string::npos)
    {
      lines.push_back(src.substr(from, to - from));
      from = to + 1;
      to = src.find("\n", from);
    }

    if (from < sz)
      lines.push_back(src.substr(from, sz - from));

    for (int l = 0; l < lines.size(); ++l)
    {
      int x = e.tm[3][0];
      int y = e.tm[3][1] + l * (textPadding * 1.2f);
      imm_prim->renderText(x + 1, y + 1, 0, 0xff000000, lines[l].data()); // shadow
      imm_prim->renderText(x, y, 0, e.color, lines[l].data());
    }
  }
  imm_prim->endRenderImm();

  dag::Vector<eastl::string_view, framemem_allocator> names;
  for (const Elem &e : sys.elems)
  {
    if (e.type != ElemType::TEXT_3D)
      continue;

    names.clear();
    for (const eastl::string_view sv : e.text)
      names.push_back(sv);
    draw_debug_multitext_overlay(e.pos, names, e.complexities, imm_prim, globtm, e.cfg);
  }

  for (const Elem &e : sys.elems)
  {
    if (e.type == ElemType::FUNC)
      e.func();
  }

  draw_snapshots(sys, target_size);
}

void cleanup(System &sys, float current_time)
{
  dag::Vector<Elem, framemem_allocator> alive;

  for (Elem &e : sys.elems)
  {
    if (e.time > current_time)
      alive.push_back(e);
  }

  sys.elems.clear();
  sys.elems.insert(sys.elems.begin(), alive.begin(), alive.end());
  sys.snapshots.clear();
}

void flush_render(HudPrimitives *imm_prim, const TMatrix4 &globtm, const IPoint2 &target_size, float current_time)
{
  OSSpinlockScopedLock lock(g_ctx.critSec);
  for (System &sys : g_ctx.systems)
  {
    if (sys.active)
      draw(sys, imm_prim, globtm, target_size);
  }

  for (System &i : g_ctx.systems)
    cleanup(i, current_time);

  g_ctx.lastGlobalTime = current_time;
}

void init()
{
  g_ctx.~Context();
  new (&g_ctx) Context();
}

void clear()
{
  OSSpinlockScopedLock lock(g_ctx.critSec);
  for (System &sys : g_ctx.systems)
    sys.elems.clear();

  g_ctx.lastGlobalTime = 0;
}

// NOTE: conditional locking is too complicated for thread safety analysis
bool begin_debug(eastl::string_view sys, eastl::string_view cmd) DAG_TS_NO_THREAD_SAFETY_ANALYSIS
{
  g_ctx.critSec.lock();
  int sysId = g_ctx.names.getNameId(sys.data());
  G_ASSERT_AND_DO(g_ctx.activeSystem == -1 || g_ctx.systems[g_ctx.activeSystem].sysId == sysId, {
    g_ctx.critSec.unlock();
    return false;
  });
  G_ASSERT(g_ctx.activeSystemRefcount >= 0);

  g_ctx.activeSystem = get_active_nolock(sys, cmd);
  bool systemActivated = g_ctx.activeSystem != -1;
  g_ctx.activeSystemRefcount += systemActivated;

  if (!systemActivated)
    g_ctx.critSec.unlock();

  return systemActivated; // -V1020
}

// NOTE: pointless to mark the definition as RELEASE, as it won't help us validate.
// TODO: consider marking the declaration as ACQUIRE/RELEASE with a fake capability declaration
void end_debug() DAG_TS_NO_THREAD_SAFETY_ANALYSIS
{
  G_ASSERT_AND_DO(g_ctx.activeSystem != -1, {
    g_ctx.critSec.unlock();
    return;
  });
  if (--g_ctx.activeSystemRefcount == 0)
    g_ctx.activeSystem = -1;
  G_ASSERT(g_ctx.activeSystemRefcount >= 0);
  g_ctx.critSec.unlock();
}

void render_box_tm(const TMatrix &tm, const BBox3 &box, uint32_t color, bool use_depth, float time)
{
  G_ASSERT_RETURN(g_ctx.activeSystem != -1, );
  System &sys = g_ctx.systems[g_ctx.activeSystem];

  Elem &e = sys.elems.push_back();
  e.type = ElemType::BOX;
  e.box = box;
  e.tm = tm;
  e.color = color;
  e.depth = use_depth;
  e.time = g_ctx.lastGlobalTime + time;
}

void render_box(const Point3 &pos, const BBox3 &box, uint32_t color, bool use_depth, float time)
{
  TMatrix tm = TMatrix::IDENT;
  tm.setcol(3, pos);
  render_box_tm(tm, box, color, use_depth, time);
}

void render_box(const BBox3 &box, uint32_t color, bool use_depth, float time)
{
  render_box_tm(TMatrix::IDENT, box, color, use_depth, time);
}

void render_square(const Point3 &pos, const Point3 &axis, float width, uint32_t color, bool use_depth, float time)
{
  G_ASSERT_RETURN(g_ctx.activeSystem != -1, );
  System &sys = g_ctx.systems[g_ctx.activeSystem];

  Elem &e = sys.elems.push_back();
  e.type = ElemType::SQUARE;
  TMatrix tm;
  Point3 y = normalize(axis);
  Point3 x = y.x < 0.95f ? Point3{1, 0, 0} : Point3{0, 0, 1};
  Point3 z = normalize(cross(x, y));
  x = normalize(cross(y, z));
  tm.setcol(0, x);
  tm.setcol(1, y);
  tm.setcol(2, z);
  tm.setcol(3, pos);
  e.tm = tm;
  e.box = BBox3(pos, width);
  e.color = color;
  e.depth = use_depth;
  e.time = g_ctx.lastGlobalTime + time;
}

void render_sphere(const Point3 &pos, float radius, uint32_t color, bool use_depth, float time)
{
  G_ASSERT_RETURN(g_ctx.activeSystem != -1, );
  System &sys = g_ctx.systems[g_ctx.activeSystem];

  Elem &e = sys.elems.push_back();
  e.type = ElemType::SPHERE;
  e.box += pos;
  e.box.inflate(radius);
  e.color = color;
  e.depth = use_depth;
  e.time = g_ctx.lastGlobalTime + time;
}

void render_cylinder(const Point3 &base_center, const Point3 &base_normal, float radius, float height, uint32_t color, bool use_depth,
  float time)
{
  G_ASSERT_RETURN(g_ctx.activeSystem != -1, );
  System &sys = g_ctx.systems[g_ctx.activeSystem];

  Elem &e = sys.elems.push_back();
  e.type = ElemType::CYLINDER;
  TMatrix tm;
  Point3 y = normalize(base_normal);
  Point3 x = y.x < 0.95f ? Point3{1, 0, 0} : Point3{0, 0, 1};
  Point3 z = normalize(cross(x, y));
  x = normalize(cross(y, z));
  tm.setcol(0, x);
  tm.setcol(1, y);
  tm.setcol(2, z);
  tm.setcol(3, base_center + y * height * 0.5);
  e.tm = tm;
  e.box.boxMin().x = radius;
  e.box.boxMin().y = height;
  e.color = color;
  e.depth = use_depth;
  e.time = g_ctx.lastGlobalTime + time;
}

void render_proj_tm(const TMatrix4 &tm, uint32_t color, bool use_depth, float time)
{
  G_ASSERT_RETURN(g_ctx.activeSystem != -1, );
  System &sys = g_ctx.systems[g_ctx.activeSystem];

  Elem &e = sys.elems.push_back();
  e.type = ElemType::PROJ_TM;
  e.tm = tm;
  e.color = color;
  e.depth = use_depth;
  e.time = g_ctx.lastGlobalTime + time;
}

void render_proj_tm(const mat44f &tm, uint32_t color, bool use_depth, float time)
{
  render_proj_tm(*((const TMatrix4_vec4 *)&tm), color, use_depth, time); //-V1027
}

void render_tm_axis(const TMatrix &tm, bool use_depth, float time)
{
  G_ASSERT_RETURN(g_ctx.activeSystem != -1, );
  System &sys = g_ctx.systems[g_ctx.activeSystem];
  Elem &e = sys.elems.push_back();

  e.type = ElemType::TM_AXIS;
  e.tm = TMatrix4(tm).transpose();
  e.depth = use_depth;
  e.time = g_ctx.lastGlobalTime + time;
}

void render_text(const Point3 &pos, eastl::string_view text, uint32_t color, float time)
{
  dag::ConstSpan<Point3> positions = make_span(&pos, 1);
  dag::ConstSpan<eastl::string_view> names = make_span(&text, 1);
  DebugMultiTextOverlay cfg;
  cfg.textColor = color;
  render_multi_text(positions, names, cfg, time);
}

void render_text2d(const Point2 &pos, eastl::string_view text, uint32_t color, float time)
{
  G_ASSERT_RETURN(g_ctx.activeSystem != -1, );
  System &sys = g_ctx.systems[g_ctx.activeSystem];
  Elem &e = sys.elems.push_back();
  e.type = ElemType::TEXT_2D;
  e.tm[3][0] = pos.x;
  e.tm[3][1] = pos.y;
  e.color = color;
  e.text.push_back() = text;
  e.time = g_ctx.lastGlobalTime + time;
}

void render_multi_text(dag::ConstSpan<Point3> positions, dag::ConstSpan<eastl::string_view> names, dag::ConstSpan<int> complexities,
  const DebugMultiTextOverlay &cfg, float time)
{
  G_ASSERT_RETURN(g_ctx.activeSystem != -1, );

  System &sys = g_ctx.systems[g_ctx.activeSystem];
  Elem &e = sys.elems.push_back();
  e.type = ElemType::TEXT_3D;
  e.text.insert(e.text.begin(), names.begin(), names.end());
  e.complexities.insert(e.complexities.begin(), complexities.begin(), complexities.end());
  e.pos = positions;
  e.cfg = cfg;
  e.time = g_ctx.lastGlobalTime + time;
}

void render_multi_text(dag::ConstSpan<Point3> positions, dag::ConstSpan<eastl::string_view> names, const DebugMultiTextOverlay &cfg,
  float time)
{
  render_multi_text(positions, names, make_span<int>(nullptr, 0), cfg, time);
}

void render_cb(eastl::function<void()> cb, float time)
{
  G_ASSERT_RETURN(g_ctx.activeSystem != -1, );

  System &sys = g_ctx.systems[g_ctx.activeSystem];
  Elem &e = sys.elems.push_back();
  e.type = ElemType::FUNC;
  e.func = cb;
  e.time = g_ctx.lastGlobalTime + time;
}

void render_snapshot(BaseTexture *tex, const char *name)
{
  G_ASSERT_RETURN(g_ctx.activeSystem != -1, );
  G_ASSERT_RETURN(tex, );
  System &sys = g_ctx.systems[g_ctx.activeSystem];

  TextureInfo ti;
  tex->getinfo(ti);
  Snapshot &e = sys.snapshots.push_back();
  e.tex = dag::create_tex(nullptr, ti.w, ti.h, ti.cflg, ti.mipLevels, name, RESTAG_DEBUG);
  e.tex->update(tex);

  if (!g_ctx.showTexRenderer)
  {
    g_ctx.showTexRenderer.reset(new PostFxRenderer());
    g_ctx.showTexRenderer->init("debug_tex_overlay");
    shaders::OverrideState state;
    state.set(shaders::OverrideState::SCISSOR_ENABLED);
    g_ctx.scissorState = shaders::overrides::create(state);
  }
}

eastl::vector<console::CommandOptions> get_cmd_options() { return g_ctx.cmdOptions; }
} // namespace debug_draw
