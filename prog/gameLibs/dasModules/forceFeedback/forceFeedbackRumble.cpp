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

    addEnumeration(new EnumerationRumbleBand());
    addAnnotation(new RumbleEventParamsAnnotation(lib));

    das::addCtorAndUsing<RumbleEventParams>(*this, lib, "RumbleEventParams", " ::RumbleEventParams");
    das::addExtern<DAS_BIND_FUN(::force_feedback::rumble::add_event)>(*this, lib, "force_feedback_rumble_add_event",
      das::SideEffects::modifyExternal, "::force_feedback::rumble::add_event");

    das::addConstant(*this, "TRIGGER_EFFECT_MASK_L2", int(HumanInput::TRIGGER_EFFECT_MASK_L2));
    das::addConstant(*this, "TRIGGER_EFFECT_MASK_R2", int(HumanInput::TRIGGER_EFFECT_MASK_R2));
    das::addExtern<DAS_BIND_FUN(force_feedback_trigger_vibration)>(*this, lib, "force_feedback_trigger_vibration",
      das::SideEffects::modifyExternal, "bind_dascript::force_feedback_trigger_vibration");
    das::addExtern<DAS_BIND_FUN(force_feedback_trigger_feedback)>(*this, lib, "force_feedback_trigger_feedback",
      das::SideEffects::modifyExternal, "bind_dascript::force_feedback_trigger_feedback");
    das::addExtern<DAS_BIND_FUN(force_feedback_trigger_weapon)>(*this, lib, "force_feedback_trigger_weapon",
      das::SideEffects::modifyExternal, "bind_dascript::force_feedback_trigger_weapon");
    das::addExtern<DAS_BIND_FUN(force_feedback_trigger_off)>(*this, lib, "force_feedback_trigger_off",
      das::SideEffects::modifyExternal, "bind_dascript::force_feedback_trigger_off");

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
