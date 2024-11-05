// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "input/dasModules/inputControls.h"


struct SensScaleAnnotation : das::ManagedStructureAnnotation<controls::SensScale, false>
{
  SensScaleAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("SensScale", ml)
  {
    cppName = " ::controls::SensScale";
    addField<DAS_BIND_MANAGED_FIELD(humanAiming)>("humanAiming");
    addField<DAS_BIND_MANAGED_FIELD(humanTpsCam)>("humanTpsCam");
    addField<DAS_BIND_MANAGED_FIELD(humanFpsCam)>("humanFpsCam");
    addField<DAS_BIND_MANAGED_FIELD(vehicleCam)>("vehicleCam");
    addField<DAS_BIND_MANAGED_FIELD(planeCam)>("planeCam");
  }
};

namespace bind_dascript
{
class DngInputControlsModule final : public das::Module
{
public:
  DngInputControlsModule() : das::Module("DngInputControls")
  {
    das::ModuleLibrary lib(this);

    addAnnotation(das::make_smart<SensScaleAnnotation>(lib));

    das::addExtern<DAS_BIND_FUN(bind_dascript::get_sens_scale), das::SimNode_ExtFuncCallRef>(*this, lib, "get_sens_scale",
      das::SideEffects::accessExternal, "bind_dascript::get_sens_scale");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"input/dasModules/inputControls.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(DngInputControlsModule, bind_dascript);
