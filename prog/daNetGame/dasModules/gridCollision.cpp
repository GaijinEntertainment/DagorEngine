// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dasModules/gridCollision.h"

DAS_BASE_BIND_ENUM(GridHideFlag, GridHideFlag, EGH_HIDDEN_POS, EGH_IN_VEHICLE, EGH_GAME_EFFECT);

namespace bind_dascript
{
class GridCollisionModule final : public das::Module
{
public:
  GridCollisionModule() : das::Module("GridCollision")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("ecs"));
    addBuiltinDependency(lib, require("Grid"));
    addBuiltinDependency(lib, require("CollisionTraces"));

    addEnumeration(das::make_smart<EnumerationGridHideFlag>());

    das::addExtern<DAS_BIND_FUN(das_trace_entities_in_grid)>(*this, lib, "trace_entities_in_grid", das::SideEffects::modifyArgument,
      "bind_dascript::das_trace_entities_in_grid");
    das::addExtern<bool (*)(uint32_t, const Point3 &, const Point3 &, float &, ecs::EntityId, IntersectedEntities &,
                     SortIntersections),
      &::trace_entities_in_grid>(*this, lib, "trace_entities_in_grid", das::SideEffects::modifyArgumentAndAccessExternal,
      "::trace_entities_in_grid");
    das::addExtern<bool (*)(uint32_t, const Point3 &, const Point3 &, float &, ecs::EntityId), &::trace_entities_in_grid>(*this, lib,
      "trace_entities_in_grid", das::SideEffects::modifyArgumentAndAccessExternal, "::trace_entities_in_grid");

    das::addExtern<bool (*)(uint32_t, const Point3 &, const Point3 &, float &, float, ecs::EntityId, IntersectedEntities &,
                     SortIntersections),
      &::trace_entities_in_grid_by_capsule>(*this, lib, "trace_entities_in_grid_by_capsule",
      das::SideEffects::modifyArgumentAndAccessExternal, "::trace_entities_in_grid_by_capsule");
    das::addExtern<bool (*)(uint32_t, const Point3 &, const Point3 &, float &, float, ecs::EntityId),
      &::trace_entities_in_grid_by_capsule>(*this, lib, "trace_entities_in_grid_by_capsule",
      das::SideEffects::modifyArgumentAndAccessExternal, "::trace_entities_in_grid_by_capsule");
    das::addExtern<DAS_BIND_FUN(das_trace_entities_in_grid_by_capsule)>(*this, lib, "trace_entities_in_grid_by_capsule",
      das::SideEffects::modifyArgumentAndAccessExternal, "bind_dascript::das_trace_entities_in_grid_by_capsule");

    das::addExtern<DAS_BIND_FUN(rayhit_entities_in_grid)>(*this, lib, "rayhit_entities_in_grid", das::SideEffects::accessExternal,
      "::rayhit_entities_in_grid");
    das::addExtern<DAS_BIND_FUN(das_query_entities_intersections_in_grid)>(*this, lib, "query_entities_intersections_in_grid",
      das::SideEffects::accessExternal, "bind_dascript::das_query_entities_intersections_in_grid");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"dasModules/gridCollision.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(GridCollisionModule, bind_dascript);
