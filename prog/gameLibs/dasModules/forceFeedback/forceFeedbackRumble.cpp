// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotForceFeedbackRumble.h>

namespace bind_dascript
{

struct RumbleEventParamsAnnotation : das::ManagedStructureAnnotation<RumbleEventParams, false>
{
  RumbleEventParamsAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("RumbleEventParams", ml)
  {
    cppName = " ::RumbleEventParams";

    addField<DAS_BIND_MANAGED_FIELD(band)>("band", "band");
    addField<DAS_BIND_MANAGED_FIELD(freq)>("freq", "freq");
    addField<DAS_BIND_MANAGED_FIELD(durationMsec)>("durationMsec", "durationMsec");
  }
};

class ForceFeedbackRumbleModule : public das::Module
{
public:
  ForceFeedbackRumbleModule() : das::Module("ForceFeedbackRumble")
  {
    das::ModuleLibrary lib(this);

    addEnumeration(das::make_smart<EnumerationRumbleBand>());
    addAnnotation(das::make_smart<RumbleEventParamsAnnotation>(lib));

    das::addCtorAndUsing<RumbleEventParams>(*this, lib, "RumbleEventParams", " ::RumbleEventParams");
    das::addExtern<DAS_BIND_FUN(::force_feedback::rumble::add_event)>(*this, lib, "force_feedback_rumble_add_event",
      das::SideEffects::modifyExternal, "::force_feedback::rumble::add_event");

    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotForceFeedbackRumble.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(ForceFeedbackRumbleModule, bind_dascript);
