// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dasModules/dasPhysObj.h"
#include <dasModules/aotGamePhys.h>

struct DasPhysObjControlsStateAnnotation : das::ManagedStructureAnnotation<DasPhysObjControlsState, false>
{
  DasPhysObjControlsStateAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("DasPhysObjControlsState", ml)
  {
    cppName = " ::DasPhysObjControlsState";
    addProperty<DAS_BIND_MANAGED_PROP(isControlsForged)>("isControlsForged");
    addProperty<DAS_BIND_MANAGED_PROP(controlsRaw)>("controlsRaw");
    addProperty<DAS_BIND_MANAGED_PROP(controlsRawRO)>("controlsRawRO");
  }
};

struct DasPhysObjStateAnnotation : das::ManagedStructureAnnotation<DasPhysObjState, false>
{
  DasPhysObjStateAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("DasPhysObjState", ml)
  {
    cppName = " ::DasPhysObjState";

    addField<DAS_BIND_MANAGED_FIELD(atTick)>("atTick");
    addField<DAS_BIND_MANAGED_FIELD(lastAppliedControlsForTick)>("lastAppliedControlsForTick");
    addField<DAS_BIND_MANAGED_FIELD(canBeCheckedForSync)>("canBeCheckedForSync");

    addField<DAS_BIND_MANAGED_FIELD(location)>("location");
    addField<DAS_BIND_MANAGED_FIELD(velocity)>("velocity");
    addField<DAS_BIND_MANAGED_FIELD(omega)>("omega");
    addField<DAS_BIND_MANAGED_FIELD(alpha)>("alpha");
    addField<DAS_BIND_MANAGED_FIELD(acceleration)>("acceleration");

    addProperty<DAS_BIND_MANAGED_PROP(physStateRaw)>("physStateRaw");
    addProperty<DAS_BIND_MANAGED_PROP(physStateRawRO)>("physStateRawRO");
  }
};

struct DasPhysObjAnnotation : das::ManagedStructureAnnotation<DasPhysObj, false>
{
  DasPhysObjAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("DasPhysObj", ml)
  {
    cppName = " ::DasPhysObj";

    addField<DAS_BIND_MANAGED_FIELD(previousState)>("previousState");
    addField<DAS_BIND_MANAGED_FIELD(currentState)>("currentState");

    addField<DAS_BIND_MANAGED_FIELD(producedCT)>("producedCT");
    addField<DAS_BIND_MANAGED_FIELD(appliedCT)>("appliedCT");
  }
};

struct DasPhysObjActorAnnotation : das::ManagedStructureAnnotation<DasPhysObjActor, false>
{
  DasPhysObjActorAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("DasPhysObjActor", ml)
  {
    cppName = " ::DasPhysObjActor";
    addField<DAS_BIND_MANAGED_FIELD(phys)>("phys");
    addProperty<DAS_BIND_MANAGED_PROP(isAsleep)>("isAsleep", "isAsleep");
  }
};

namespace bind_dascript
{
class DasPhysObjModule final : public das::Module
{
public:
  DasPhysObjModule() : das::Module("DasPhysObj")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("math"));
    addBuiltinDependency(lib, require("DagorMath"));
    addBuiltinDependency(lib, require("GamePhys"));

    addAnnotation(das::make_smart<DasPhysObjControlsStateAnnotation>(lib));
    addAnnotation(das::make_smart<DasPhysObjStateAnnotation>(lib));
    addAnnotation(das::make_smart<DasPhysObjAnnotation>(lib));
    addAnnotation(das::make_smart<DasPhysObjActorAnnotation>(lib));

    using method_allocControls = DAS_CALL_MEMBER(DasPhysObj::allocControls);
    das::addExtern<DAS_CALL_METHOD(method_allocControls)>(*this, lib, "allocControls", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(DasPhysObj::allocControls));
    using method_allocControlsEx = DAS_CALL_MEMBER(DasPhysObj::allocControlsEx);
    das::addExtern<DAS_CALL_METHOD(method_allocControlsEx)>(*this, lib, "allocControlsEx", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(DasPhysObj::allocControlsEx));

    using method_allocPhysState = DAS_CALL_MEMBER(DasPhysObj::allocPhysState);
    das::addExtern<DAS_CALL_METHOD(method_allocPhysState)>(*this, lib, "allocPhysState", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(DasPhysObj::allocPhysState));
    using method_allocPhysStateEx = DAS_CALL_MEMBER(DasPhysObj::allocPhysStateEx);
    das::addExtern<DAS_CALL_METHOD(method_allocPhysStateEx)>(*this, lib, "allocPhysStateEx", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(DasPhysObj::allocPhysStateEx));

    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"phys/dasPhysObj.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(DasPhysObjModule, bind_dascript);
