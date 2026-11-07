// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "physKind.h"

struct PhysKindStateAnnotation final : das::ManagedStructureAnnotation<PhysKindState, false>
{
  PhysKindStateAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("PhysKindState", ml)
  {
    cppName = " ::PhysKindState";
    addField<DAS_BIND_MANAGED_FIELD(atTick)>("atTick");
    addField<DAS_BIND_MANAGED_FIELD(lastAppliedControlsForTick)>("lastAppliedControlsForTick");
    addField<DAS_BIND_MANAGED_FIELD(location)>("location");
    addField<DAS_BIND_MANAGED_FIELD(velocity)>("velocity");
    addField<DAS_BIND_MANAGED_FIELD(omega)>("omega");
    addField<DAS_BIND_MANAGED_FIELD(alpha)>("alpha");
    addField<DAS_BIND_MANAGED_FIELD(acceleration)>("acceleration");
    addField<DAS_BIND_MANAGED_FIELD(gravDirection)>("gravDirection");
  }
};

struct PhysKindControlsAnnotation final : das::ManagedStructureAnnotation<PhysKindControls, false>
{
  PhysKindControlsAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("PhysKindControls", ml)
  {
    cppName = " ::PhysKindControls";
    addField<DAS_BIND_MANAGED_FIELD(producedAtTick)>("producedAtTick");
    addField<DAS_BIND_MANAGED_FIELD(isForged)>("isForged");
  }
};

struct PhysKindWireFieldAnnotation final : das::ManagedStructureAnnotation<PhysKindWireField, true>
{
  PhysKindWireFieldAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("PhysKindWireField", ml)
  {
    cppName = " ::PhysKindWireField";
    addField<DAS_BIND_MANAGED_FIELD(offset)>("offset");
    addField<DAS_BIND_MANAGED_FIELD(size)>("size");
    addField<DAS_BIND_MANAGED_FIELD(codec)>("codec");
    addField<DAS_BIND_MANAGED_FIELD(bits)>("bits");
    addField<DAS_BIND_MANAGED_FIELD(isSigned)>("isSigned");
    addField<DAS_BIND_MANAGED_FIELD(snapLod)>("snapLod");
    addField<DAS_BIND_MANAGED_FIELD(min)>("min");
    addField<DAS_BIND_MANAGED_FIELD(max)>("max");
    addField<DAS_BIND_MANAGED_FIELD(step)>("step");
  }
};

struct PhysKindAnnotation final : das::ManagedStructureAnnotation<PhysKind, false>
{
  PhysKindAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("PhysKind", ml)
  {
    cppName = " ::PhysKind";
    addField<DAS_BIND_MANAGED_FIELD(currentState)>("currentState");
    addField<DAS_BIND_MANAGED_FIELD(previousState)>("previousState");
    addField<DAS_BIND_MANAGED_FIELD(appliedCT)>("appliedCT");
    addField<DAS_BIND_MANAGED_FIELD(producedCT)>("producedCT");
    addField<DAS_BIND_MANAGED_FIELD(visualLocation)>("visualLocation");
    addField<DAS_BIND_MANAGED_FIELD(mass)>("mass");
    addField<DAS_BIND_MANAGED_FIELD(centerOfMass)>("centerOfMass");
    addField<DAS_BIND_MANAGED_FIELD(momentOfInertia)>("momentOfInertia");
    addField<DAS_BIND_MANAGED_FIELD(external)>("external");
    addField<DAS_BIND_MANAGED_FIELD(shadowSim)>("shadowSim");
  }
};

struct PhysKindActorAnnotation final : das::ManagedStructureAnnotation<PhysKindActor, false>
{
  das::TypeDeclPtr parentType;

  PhysKindActorAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("PhysKindActor", ml)
  {
    cppName = " ::PhysKindActor";
    addField<DAS_BIND_MANAGED_FIELD(phys)>("phys");
    parentType = das::makeType<BasePhysActor>(ml);
  }

  bool canBeSubstituted(TypeAnnotation *pass_type) const override { return parentType->annotation == pass_type; }

  void gc_collect(das::gc_root *target, das::gc_root *from) override
  {
    das::ManagedStructureAnnotation<PhysKindActor, false>::gc_collect(target, from);
    if (parentType)
      parentType->gc_collect(target, from);
  }
  void visitTypeDecls(const das::function<void(das::TypeDecl *)> &callback) override
  {
    if (parentType)
      callback(parentType);
  }
};

namespace bind_dascript
{
class DngPhysKindModule final : public das::Module
{
public:
  DngPhysKindModule() : das::Module("DngPhysKind")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("math"));
    addBuiltinDependency(lib, require("DagorMath"));
    addBuiltinDependency(lib, require("GamePhys"));
    addBuiltinDependency(lib, require("DngActor"));

    addEnumeration(new EnumerationPhysKindLod());
    addEnumeration(new EnumerationPhysKindWireCodec());
    addAnnotation(new PhysKindStateAnnotation(lib));
    addAnnotation(new PhysKindControlsAnnotation(lib));
    addAnnotation(new PhysKindWireFieldAnnotation(lib));
    addAnnotation(new PhysKindAnnotation(lib));
    addAnnotation(new PhysKindActorAnnotation(lib));

    das::addExtern<DAS_BIND_FUN(bind_dascript::phys_kind_state_ext)>(*this, lib, "phys_kind_state_ext",
      das::SideEffects::modifyArgument, "bind_dascript::phys_kind_state_ext");
    das::addExtern<DAS_BIND_FUN(bind_dascript::phys_kind_controls_ext)>(*this, lib, "phys_kind_controls_ext",
      das::SideEffects::modifyArgument, "bind_dascript::phys_kind_controls_ext");
    das::addExtern<DAS_BIND_FUN(bind_dascript::phys_kind_desync_stats)>(*this, lib, "phys_kind_desync_stats", das::SideEffects::none,
      "bind_dascript::phys_kind_desync_stats");
    das::addExtern<DAS_BIND_FUN(bind_dascript::phys_kind_owner_snapshot_stats)>(*this, lib, "phys_kind_owner_snapshot_stats",
      das::SideEffects::none, "bind_dascript::phys_kind_owner_snapshot_stats");
    das::addExtern<DAS_BIND_FUN(bind_dascript::phys_kind_set_snap_ext_sizes)>(*this, lib, "phys_kind_set_snap_ext_sizes",
      das::SideEffects::modifyArgument, "bind_dascript::phys_kind_set_snap_ext_sizes");
    das::addExtern<DAS_BIND_FUN(bind_dascript::phys_kind_set_wire_fields)>(*this, lib, "phys_kind_set_wire_fields",
      das::SideEffects::modifyArgument, "bind_dascript::phys_kind_set_wire_fields");

    using method_addCollisionBox = DAS_CALL_MEMBER(PhysKind::addCollisionBox);
    das::addExtern<DAS_CALL_METHOD(method_addCollisionBox)>(*this, lib, "phys_kind_add_collision_box",
      das::SideEffects::modifyArgumentAndExternal, DAS_CALL_MEMBER_CPP(PhysKind::addCollisionBox));
    using method_putToSleep = DAS_CALL_MEMBER(PhysKind::putToSleep);
    das::addExtern<DAS_CALL_METHOD(method_putToSleep)>(*this, lib, "phys_kind_put_to_sleep", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(PhysKind::putToSleep));
    using method_wakeUp = DAS_CALL_MEMBER(PhysKind::wakeUp);
    das::addExtern<DAS_CALL_METHOD(method_wakeUp)>(*this, lib, "phys_kind_wake_up", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(PhysKind::wakeUp));
    using method_isAsleep = DAS_CALL_MEMBER(PhysKind::isAsleep);
    das::addExtern<DAS_CALL_METHOD(method_isAsleep)>(*this, lib, "phys_kind_is_asleep", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(PhysKind::isAsleep));

    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"phys_kind/dasModules/physKind.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(DngPhysKindModule, bind_dascript);
