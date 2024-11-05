// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotDagorDebug3d.h>

namespace bind_dascript
{
static char dagorDebug3d_das[] =
#include "dagorDebug3d.das.inl"
  ;

class DagorDebug3DModule final : public das::Module
{
public:
  DagorDebug3DModule() : das::Module("DagorDebug3D")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("math"));
    addBuiltinDependency(lib, require("DagorMath"));

    // buffered draw
    das::addExtern<DAS_BIND_FUN(bind_dascript::draw_debug_line_buffered)>(*this, lib, "_builtin_draw_debug_line_buffered",
      das::SideEffects::modifyExternal, "bind_dascript::draw_debug_line_buffered");
    das::addExtern<DAS_BIND_FUN(::clear_buffered_debug_lines)>(*this, lib, "clear_buffered_debug_lines",
      das::SideEffects::modifyExternal, "::clear_buffered_debug_lines");
    das::addExtern<DAS_BIND_FUN(bind_dascript::draw_debug_rect_buffered)>(*this, lib, "_builtin_draw_debug_rect_buffered",
      das::SideEffects::modifyExternal, "bind_dascript::draw_debug_rect_buffered");
    das::addExtern<DAS_BIND_FUN(bind_dascript::draw_debug_box_buffered)>(*this, lib, "_builtin_draw_debug_box_buffered",
      das::SideEffects::modifyExternal, "bind_dascript::draw_debug_box_buffered");
    das::addExtern<DAS_BIND_FUN(bind_dascript::draw_debug_box_buffered_axis)>(*this, lib, "_builtin_draw_debug_box_buffered",
      das::SideEffects::modifyExternal, "bind_dascript::draw_debug_box_buffered_axis");
    das::addExtern<DAS_BIND_FUN(bind_dascript::draw_debug_box_buffered_tm)>(*this, lib, "_builtin_draw_debug_box_buffered",
      das::SideEffects::modifyExternal, "bind_dascript::draw_debug_box_buffered_tm");
    das::addExtern<DAS_BIND_FUN(bind_dascript::draw_debug_circle_buffered)>(*this, lib, "_builtin_draw_debug_circle_buffered",
      das::SideEffects::modifyExternal, "bind_dascript::draw_debug_circle_buffered");
    das::addExtern<DAS_BIND_FUN(bind_dascript::draw_debug_elipse_buffered)>(*this, lib, "draw_debug_elipse_buffered",
      das::SideEffects::modifyExternal, "bind_dascript::draw_debug_elipse_buffered");
    das::addExtern<DAS_BIND_FUN(bind_dascript::draw_debug_sphere_buffered)>(*this, lib, "_builtin_draw_debug_sphere_buffered",
      das::SideEffects::modifyExternal, "bind_dascript::draw_debug_sphere_buffered");
    das::addExtern<DAS_BIND_FUN(bind_dascript::draw_debug_tube_buffered)>(*this, lib, "_builtin_draw_debug_tube_buffered",
      das::SideEffects::modifyExternal, "bind_dascript::draw_debug_tube_buffered");
    das::addExtern<DAS_BIND_FUN(bind_dascript::draw_debug_cone_buffered)>(*this, lib, "_builtin_draw_debug_cone_buffered",
      das::SideEffects::modifyExternal, "bind_dascript::draw_debug_cone_buffered");
    das::addExtern<DAS_BIND_FUN(bind_dascript::draw_debug_capsule_buffered)>(*this, lib, "_builtin_draw_debug_capsule_buffered",
      das::SideEffects::modifyExternal, "bind_dascript::draw_debug_capsule_buffered");
    das::addExtern<DAS_BIND_FUN(bind_dascript::draw_debug_arrow_buffered)>(*this, lib, "_builtin_draw_debug_arrow_buffered",
      das::SideEffects::modifyExternal, "bind_dascript::draw_debug_arrow_buffered");
    das::addExtern<DAS_BIND_FUN(bind_dascript::draw_debug_tetrapod_buffered)>(*this, lib, "_builtin_draw_debug_tetrapod_buffered",
      das::SideEffects::modifyExternal, "bind_dascript::draw_debug_tetrapod_buffered");
    das::addExtern<DAS_BIND_FUN(bind_dascript::draw_debug_tehedron_buffered)>(*this, lib, "_builtin_draw_debug_tehedron_buffered",
      das::SideEffects::modifyExternal, "bind_dascript::draw_debug_tehedron_buffered");

#define ADD_EXTERN(fun) das::addExtern<DAS_BIND_FUN(fun)>(*this, lib, #fun, das::SideEffects::modifyExternal, "::" #fun)
    // debug draw
    ADD_EXTERN(begin_draw_cached_debug_lines);
    ADD_EXTERN(begin_draw_cached_debug_lines_ex);
    ADD_EXTERN(end_draw_cached_debug_lines);
    ADD_EXTERN(end_draw_cached_debug_lines_ex);

    ADD_EXTERN(draw_cached_debug_cone);

    das::addExtern<DAS_BIND_FUN(::set_cached_debug_lines_wtm)>(*this, lib, "set_cached_debug_lines_wtm",
      das::SideEffects::modifyExternal, "::set_cached_debug_lines_wtm");
    das::addExtern<void (*)(const Point3 &, const Point3 &, E3DCOLOR), &::draw_cached_debug_line>(*this, lib, "draw_cached_debug_line",
      das::SideEffects::modifyExternal, "::draw_cached_debug_line");
    das::addExtern<void (*)(const Point3 *, int, E3DCOLOR), &::draw_cached_debug_line>(*this, lib, "draw_cached_debug_line",
      das::SideEffects::modifyExternal, "::draw_cached_debug_line");

    das::addExtern<void (*)(const TMatrix &, float, float, E3DCOLOR), &::draw_cached_debug_cylinder>(*this, lib,
      "draw_cached_debug_cylinder", das::SideEffects::modifyExternal, "::draw_cached_debug_cylinder");
    das::addExtern<void (*)(const TMatrix &, Point3 &, Point3 &, float, E3DCOLOR), &::draw_cached_debug_cylinder>(*this, lib,
      "draw_cached_debug_cylinder", das::SideEffects::modifyExternal, "::draw_cached_debug_cylinder");
    das::addExtern<void (*)(Point3 &, Point3 &, float, E3DCOLOR), &::draw_cached_debug_cylinder_w>(*this, lib,
      "draw_cached_debug_cylinder_w", das::SideEffects::modifyExternal, "::draw_cached_debug_cylinder_w");

    das::addExtern<void (*)(const BBox3 &, E3DCOLOR), draw_cached_debug_box>(*this, lib, "draw_cached_debug_box",
      das::SideEffects::modifyExternal, "::draw_cached_debug_box");
    das::addExtern<void (*)(const Point3 &, const Point3 &, const Point3 &, const Point3 &, E3DCOLOR), draw_cached_debug_box>(*this,
      lib, "draw_cached_debug_box", das::SideEffects::modifyExternal, "::draw_cached_debug_box");

    ADD_EXTERN(draw_cached_debug_sphere);
    ADD_EXTERN(draw_cached_debug_circle);
    ADD_EXTERN(draw_cached_debug_xz_circle);
    ADD_EXTERN(draw_skeleton_link);
    // ADD_EXTERN(draw_skeleton_tree);

    das::addExtern<void (*)(const Point3 &, const Point3 &, float, E3DCOLOR), &::draw_cached_debug_capsule_w>(*this, lib,
      "draw_cached_debug_capsule_w", das::SideEffects::modifyExternal, "::draw_cached_debug_capsule_w");
    das::addExtern<void (*)(const Capsule &, E3DCOLOR), &::draw_cached_debug_capsule_w>(*this, lib, "draw_cached_debug_capsule_w",
      das::SideEffects::modifyExternal, "::draw_cached_debug_capsule_w");

    ADD_EXTERN(draw_cached_debug_capsule);
    ADD_EXTERN(draw_cached_debug_hex);

    das::addExtern<DAS_BIND_FUN(bind_dascript::draw_cached_debug_quad)>(*this, lib, "_builtin_draw_cached_debug_quad",
      das::SideEffects::modifyExternal, "bind_dascript::draw_cached_debug_quad");
    das::addExtern<DAS_BIND_FUN(bind_dascript::draw_cached_debug_trilist)>(*this, lib, "_builtin_draw_cached_debug_trilist",
      das::SideEffects::modifyExternal, "bind_dascript::draw_cached_debug_trilist");

    das::addExtern<void (*)(const Point3 &, const char *, int, float, E3DCOLOR), &::add_debug_text_mark>(*this, lib,
      "add_debug_text_mark", das::SideEffects::modifyExternal, "::add_debug_text_mark");
    das::addExtern<void (*)(float, float, const char *, int, float, E3DCOLOR), &::add_debug_text_mark>(*this, lib,
      "add_debug_text_mark_screen", das::SideEffects::modifyExternal, "::add_debug_text_mark");

#undef ADD_EXTERN
    compileBuiltinModule("dagorDebug3d.das", (unsigned char *)dagorDebug3d_das, sizeof(dagorDebug3d_das));
    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotDagorDebug3d.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(DagorDebug3DModule, bind_dascript);
