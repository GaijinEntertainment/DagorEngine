// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dasModules/dagorDebug3dSolid.h"

namespace bind_dascript
{
static char dagorDebug3dSolid_das[] =
#include "dagorDebug3dSolid.das.inl"
  ;

class DagorDebug3DSolidModule final : public das::Module
{
public:
  DagorDebug3DSolidModule() : das::Module("DagorDebug3DSolid")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("math"));
    addBuiltinDependency(lib, require("DagorMath"));

    das::addExtern<DAS_BIND_FUN(bind_dascript::draw_debug_cube_buffered)>(*this, lib, "_builtin_draw_debug_cube_buffered",
      das::SideEffects::modifyExternal, "bind_dascript::draw_debug_cube_buffered");
    das::addExtern<DAS_BIND_FUN(bind_dascript::draw_debug_triangle_buffered)>(*this, lib, "_builtin_draw_debug_triangle_buffered",
      das::SideEffects::modifyExternal, "bind_dascript::draw_debug_triangle_buffered");
    das::addExtern<DAS_BIND_FUN(bind_dascript::draw_debug_quad_buffered_4point)>(*this, lib, "_builtin_draw_debug_quad_buffered",
      das::SideEffects::modifyExternal, "bind_dascript::draw_debug_quad_buffered_4point");
    das::addExtern<DAS_BIND_FUN(bind_dascript::draw_debug_quad_buffered_halfvec)>(*this, lib, "_builtin_draw_debug_quad_buffered",
      das::SideEffects::modifyExternal, "bind_dascript::draw_debug_quad_buffered_halfvec");
    das::addExtern<DAS_BIND_FUN(bind_dascript::draw_debug_ball_buffered)>(*this, lib, "_builtin_draw_debug_ball_buffered",
      das::SideEffects::modifyExternal, "bind_dascript::draw_debug_ball_buffered");
    das::addExtern<DAS_BIND_FUN(bind_dascript::draw_debug_disk_buffered)>(*this, lib, "_builtin_draw_debug_disk_buffered",
      das::SideEffects::modifyExternal, "bind_dascript::draw_debug_disk_buffered");
    das::addExtern<DAS_BIND_FUN(::clear_buffered_debug_solids)>(*this, lib, "clear_buffered_debug_solids",
      das::SideEffects::modifyExternal, "::clear_buffered_debug_solids");

    compileBuiltinModule("dagorDebug3dSolid.das", (unsigned char *)dagorDebug3dSolid_das, sizeof(dagorDebug3dSolid_das));
    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"dasModules/dagorDebug3dSolid.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(DagorDebug3DSolidModule, bind_dascript);