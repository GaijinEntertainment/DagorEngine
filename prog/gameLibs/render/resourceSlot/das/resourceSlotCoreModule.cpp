// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/resourceSlot/das/resourceSlotCoreModule.h>
#include <daECS/core/componentType.h>
#include <detail/storage.h>
#include <detail/unregisterAccess.h>


namespace bind_dascript
{

struct NodeHandleWithSlotsAccessAnnotation final : das::ManagedStructureAnnotation<::resource_slot::NodeHandleWithSlotsAccess>
{
  NodeHandleWithSlotsAccessAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("NodeHandleWithSlotsAccess", ml, "::resource_slot::NodeHandleWithSlotsAccess")
  {
    addProperty<DAS_BIND_MANAGED_PROP(valid)>("valid", "valid");
  }

  // required for move on das side.
  // das_move is memcpy + memzero, that's fine for this class,
  // because zeroing isValid is basically moving of handle.
  bool canMove() const override { return true; }
};

struct StateAnnotation final : das::ManagedStructureAnnotation<::resource_slot::State>
{
  StateAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("State", ml, "::resource_slot::State") {}

  // should be moved to lambda, because cpp value stored in vector and can be reallocated - cannot use reference here
  bool isLocal() const override { return true; }
  bool canMove() const override { return true; }
};

struct ActionListAnnotation final : das::ManagedStructureAnnotation<::resource_slot::detail::ActionList>
{
  ActionListAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ActionList", ml, "::resource_slot::detail::ActionList") {}
};


class ResourceSlotCoreModule : public das::Module
{
public:
  ResourceSlotCoreModule() : das::Module("ResourceSlotCore")
  {
    das::ModuleLibrary lib(this);
    das::onDestroyCppDebugAgent(name.c_str(), bind_dascript::clear_resource_slot);
    addBuiltinDependency(lib, require("daBfgCore"));

    addAnnotation(das::make_smart<NodeHandleWithSlotsAccessAnnotation>(lib));
    das::addCtor<::resource_slot::NodeHandleWithSlotsAccess>(*this, lib, "NodeHandleWithSlotsAccess",
      "::resource_slot::NodeHandleWithSlotsAccess");
    das::typeFactory<::resource_slot::NodeHandleWithSlotsAccessVector>::make(lib);

    addAnnotation(das::make_smart<ActionListAnnotation>(lib));
    das::addUsing<::resource_slot::detail::ActionList>(*this, lib, "::resource_slot::detail::ActionList");

    das::addExtern<DAS_BIND_FUN(::bind_dascript::ActionList_create)>(*this, lib, "create", das::SideEffects::modifyArgument,
      "::bind_dascript::ActionList_create")
      ->args({"list", "slot_name", "resource_name"});
    das::addExtern<DAS_BIND_FUN(::bind_dascript::ActionList_update)>(*this, lib, "update", das::SideEffects::modifyArgument,
      "::bind_dascript::ActionList_update")
      ->args({"list", "slot_name", "resource_name", "priority"});
    das::addExtern<DAS_BIND_FUN(::bind_dascript::ActionList_read)>(*this, lib, "read", das::SideEffects::modifyArgument,
      "::bind_dascript::ActionList_read")
      ->args({"list", "slot_name", "priority"});

    addAnnotation(das::make_smart<StateAnnotation>(lib));

    das::addExtern<DAS_BIND_FUN(::bind_dascript::NodeHandleWithSlotsAccess_reset)>(*this, lib, "reset",
      das::SideEffects::modifyArgumentAndExternal, "::bind_dascript::NodeHandleWithSlotsAccess_reset")
      ->args({"handle"});

    das::addExtern<DAS_BIND_FUN(::bind_dascript::prepare_access), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "prepare_access",
      das::SideEffects::invoke, "::bind_dascript::prepare_access");
    das::addExtern<DAS_BIND_FUN(::bind_dascript::register_access)>(*this, lib, "register_access",
      das::SideEffects::modifyArgumentAndExternal, "::bind_dascript::register_access");

    das::addExtern<DAS_BIND_FUN(::bind_dascript::resourceToReadFrom)>(*this, lib, "resourceToReadFrom",
      das::SideEffects::accessExternal, "::bind_dascript::resourceToReadFrom")
      ->args({"state", "slot_name"});

    das::addExtern<DAS_BIND_FUN(::bind_dascript::resourceToCreateFor)>(*this, lib, "resourceToCreateFor",
      das::SideEffects::accessExternal, "::bind_dascript::resourceToCreateFor")
      ->args({"state", "slot_name"});

    verifyAotReady();
  }

  das::ModuleAotType aotRequire(das::TextWriter &tw) const
  {
    tw << "#include <render/resourceSlot/das/resourceSlotCoreModule.h>\n";
    return das::ModuleAotType::cpp;
  }
};

} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(ResourceSlotCoreModule, bind_dascript);