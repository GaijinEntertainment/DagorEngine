// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dasModules/collisionTraces.h"

struct IntersectedEntityDataAnnotation : das::ManagedStructureAnnotation<IntersectedEntityData, false>
{
  IntersectedEntityDataAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("IntersectedEntityData", ml)
  {
    cppName = " ::IntersectedEntityData";
    addField<DAS_BIND_MANAGED_FIELD(t)>("t");
    addField<DAS_BIND_MANAGED_FIELD(collNodeId)>("collNodeId");
    addField<DAS_BIND_MANAGED_FIELD(pos)>("pos");
    addField<DAS_BIND_MANAGED_FIELD(norm)>("norm");
    addField<DAS_BIND_MANAGED_FIELD(depth)>("depth");
  }
};

struct IntersectedEntityAnnotation : das::ManagedStructureAnnotation<IntersectedEntity, false>
{
  IntersectedEntityAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("IntersectedEntity", ml)
  {
    cppName = " ::IntersectedEntity";
    addField<DAS_BIND_MANAGED_FIELD(eid)>("eid");
    addField<DAS_BIND_MANAGED_FIELD(t)>("t");
    addField<DAS_BIND_MANAGED_FIELD(collNodeId)>("collNodeId");
    addField<DAS_BIND_MANAGED_FIELD(pos)>("pos");
    addField<DAS_BIND_MANAGED_FIELD(norm)>("norm");
    addField<DAS_BIND_MANAGED_FIELD(depth)>("depth");
  }
  bool hasNonTrivialCopy() const override { return false; } // for emplace(push_clone) to the containers in das
  bool canBePlacedInContainer() const override { return true; }
};

namespace bind_dascript
{
class CollisionTracesModule final : public das::Module
{
public:
  CollisionTracesModule() : das::Module("CollisionTraces")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("ecs"));

    addEnumeration(das::make_smart<EnumerationSortIntersections>());
    addAnnotation(das::make_smart<IntersectedEntityDataAnnotation>(lib));
    addAnnotation(das::make_smart<IntersectedEntityAnnotation>(lib));

    das::addExtern<DAS_BIND_FUN(das_trace_to_collision_nodes)>(*this, lib, "trace_to_collision_nodes",
      das::SideEffects::accessExternal, "bind_dascript::das_trace_to_collision_nodes");
    das::addExtern<DAS_BIND_FUN(das_trace_to_capsule_approximation)>(*this, lib, "trace_to_capsule_approximation",
      das::SideEffects::accessExternal, "bind_dascript::das_trace_to_capsule_approximation");
    das::addExtern<DAS_BIND_FUN(rayhit_to_collision_nodes)>(*this, lib, "rayhit_to_collision_nodes", das::SideEffects::accessExternal,
      "rayhit_to_collision_nodes");
    das::addExtern<DAS_BIND_FUN(rayhit_to_capsule_approximation)>(*this, lib, "rayhit_to_capsule_approximation",
      das::SideEffects::accessExternal, "rayhit_to_capsule_approximation");

    das::addCtorAndUsing<IntersectedEntity>(*this, lib, "IntersectedEntity", "::IntersectedEntity");
    das::addCtorAndUsing<IntersectedEntities>(*this, lib, "IntersectedEntities", "::IntersectedEntities");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &) const override { return das::ModuleAotType::cpp; }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(CollisionTracesModule, bind_dascript);
