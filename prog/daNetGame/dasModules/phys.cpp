// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/daScript.h>
#include "phys/physUtils.h"
#include "dasModules/aotDagorMath.h"
#include "dasModules/phys.h"
#include "dasModules/actor.h"

struct IPhysBaseAnnotation final : das::ManagedStructureAnnotation<IPhysBase, false>
{
  IPhysBaseAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("IPhysBase", ml, " ::IPhysBase") {}
};

namespace bind_dascript
{
class DngPhysModule final : public das::Module
{
public:
  DngPhysModule() : das::Module("DngPhys")
  {
    das::ModuleLibrary lib(this);

    addAnnotation(das::make_smart<IPhysBaseAnnotation>(lib));

    das::addExtern<DAS_BIND_FUN(::phys_time_to_seed)>(*this, lib, "phys_time_to_seed", das::SideEffects::none, "::phys_time_to_seed");
    das::addExtern<DAS_BIND_FUN(base_phys_actor_phys)>(*this, lib, "base_phys_actor_phys", das::SideEffects::none,
      "bind_dascript::base_phys_actor_phys");

    using method_getCurrentStateVelocity = DAS_CALL_MEMBER(IPhysBase::getCurrentStateVelocity);
    das::addExtern<DAS_CALL_METHOD(method_getCurrentStateVelocity), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib,
      "getCurrentStateVelocity", das::SideEffects::none, DAS_CALL_MEMBER_CPP(IPhysBase::getCurrentStateVelocity));

    das::addConstant(*this, "PHYS_ENABLE_INTERPOLATION", PHYS_ENABLE_INTERPOLATION);
    das::addConstant(*this, "PHYS_MIN_INTERP_DELAY_TICKS", PHYS_MIN_INTERP_DELAY_TICKS);
    das::addConstant(*this, "PHYS_MAX_INTERP_DELAY_TICKS", PHYS_MAX_INTERP_DELAY_TICKS);
    das::addConstant(*this, "PHYS_MAX_QUANTIZED_SLOW_SPEED", PHYS_MAX_QUANTIZED_SLOW_SPEED);
    das::addConstant(*this, "PHYS_MAX_QUANTIZED_FAST_SPEED", PHYS_MAX_QUANTIZED_FAST_SPEED);
    das::addConstant(*this, "PHYS_DEFAULT_TICKRATE", PHYS_DEFAULT_TICKRATE);
    das::addConstant(*this, "PHYS_DEFAULT_BOT_TICKRATE", PHYS_DEFAULT_BOT_TICKRATE);
    das::addConstant(*this, "PHYS_MAX_INTERP_FROM_TIMESPEED_DELAY_SEC", PHYS_MAX_INTERP_FROM_TIMESPEED_DELAY_SEC);
    das::addConstant(*this, "PHYS_MIN_TICKRATE", PHYS_MIN_TICKRATE);
    das::addConstant(*this, "PHYS_MAX_TICKRATE", PHYS_MAX_TICKRATE);
    das::addConstant(*this, "PHYS_CONTROLS_NET_REDUNDANCY", PHYS_CONTROLS_NET_REDUNDANCY);
    das::addConstant(*this, "PHYS_REMOTE_SNAPSHOTS_HISTORY_CAPACITY", PHYS_REMOTE_SNAPSHOTS_HISTORY_CAPACITY);
    das::addConstant(*this, "PHYS_MAX_CONTROLS_TICKS_DELTA_SEC", PHYS_MAX_CONTROLS_TICKS_DELTA_SEC);
    das::addConstant(*this, "PHYS_MAX_AAS_DELAY_K", PHYS_MAX_AAS_DELAY_K);

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"phys/physUtils.h\"\n";
    tw << "#include \"dasModules/phys.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(DngPhysModule, bind_dascript);
