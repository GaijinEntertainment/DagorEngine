// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotGrid.h>

DAS_BASE_BIND_ENUM(GridEntCheck, GridEntCheck, POS, BOUNDING);

namespace bind_dascript
{
static char grid_das[] =
#include "grid.das.inl"
  ;

class GridModule final : public das::Module
{
public:
  GridModule() : das::Module("Grid")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("ecs"));
    addBuiltinDependency(lib, require("DagorMath"));
    addEnumeration(das::make_smart<EnumerationGridEntCheck>());

    das::addExtern<DAS_BIND_FUN(_builtin_gather_entities_in_grid_box)>(*this, lib, "gather_entities_in_grid",
      das::SideEffects::accessExternal, "bind_dascript::_builtin_gather_entities_in_grid_box");
    das::addExtern<DAS_BIND_FUN(_builtin_gather_entities_in_grid_sphere)>(*this, lib, "gather_entities_in_grid",
      das::SideEffects::accessExternal, "bind_dascript::_builtin_gather_entities_in_grid_sphere");
    das::addExtern<DAS_BIND_FUN(_builtin_gather_entities_in_grid_capsule)>(*this, lib, "gather_entities_in_grid",
      das::SideEffects::accessExternal, "bind_dascript::_builtin_gather_entities_in_grid_capsule");
    das::addExtern<DAS_BIND_FUN(_builtin_gather_entities_in_grid_tm_box)>(*this, lib, "gather_entities_in_grid",
      das::SideEffects::accessExternal, "bind_dascript::_builtin_gather_entities_in_grid_tm_box");
    das::addExtern<DAS_BIND_FUN(_builtin_append_entities_in_grid_box)>(*this, lib, "append_entities_in_grid",
      das::SideEffects::accessExternal, "bind_dascript::_builtin_append_entities_in_grid_box");
    das::addExtern<DAS_BIND_FUN(_builtin_append_entities_in_grid_sphere)>(*this, lib, "append_entities_in_grid",
      das::SideEffects::accessExternal, "bind_dascript::_builtin_append_entities_in_grid_sphere");
    das::addExtern<DAS_BIND_FUN(_builtin_append_entities_in_grid_capsule)>(*this, lib, "append_entities_in_grid",
      das::SideEffects::accessExternal, "bind_dascript::_builtin_append_entities_in_grid_capsule");
    das::addExtern<DAS_BIND_FUN(_builtin_append_entities_in_grid_tm_box)>(*this, lib, "append_entities_in_grid",
      das::SideEffects::accessExternal, "bind_dascript::_builtin_append_entities_in_grid_tm_box");
    das::addExtern<DAS_BIND_FUN(_builtin_find_entity_in_grid_box)>(*this, lib, "find_entity_in_grid", das::SideEffects::accessExternal,
      "bind_dascript::_builtin_find_entity_in_grid_box");
    das::addExtern<DAS_BIND_FUN(_builtin_find_entity_in_grid_sphere)>(*this, lib, "find_entity_in_grid",
      das::SideEffects::accessExternal, "bind_dascript::_builtin_find_entity_in_grid_sphere");
    das::addExtern<DAS_BIND_FUN(_builtin_find_entity_in_grid_capsule)>(*this, lib, "find_entity_in_grid",
      das::SideEffects::accessExternal, "bind_dascript::_builtin_find_entity_in_grid_capsule");
    das::addExtern<DAS_BIND_FUN(_builtin_find_entity_in_grid_tm_box)>(*this, lib, "find_entity_in_grid",
      das::SideEffects::accessExternal, "bind_dascript::_builtin_find_entity_in_grid_tm_box");

    compileBuiltinModule("grid.das", (unsigned char *)grid_das, sizeof(grid_das));
    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotGrid.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(GridModule, bind_dascript);
