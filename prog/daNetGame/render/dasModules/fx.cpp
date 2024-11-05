// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "render/dasModules/fx.h"


struct AcesEffectAnnotation : das::ManagedStructureAnnotation<AcesEffect, false>
{
  AcesEffectAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AcesEffect", ml) { cppName = " ::AcesEffect"; }
};


struct ScaledAcesEffectAnnotation : das::ManagedStructureAnnotation<ScaledAcesEffect, false>
{
  ScaledAcesEffectAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ScaledAcesEffect", ml)
  {
    cppName = " ::ScaledAcesEffect";
  }
};


struct TheEffectAnnotation : das::ManagedStructureAnnotation<TheEffect, false>
{
  TheEffectAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("TheEffect", ml) { cppName = " ::TheEffect"; }
};


struct GravityZoneBufferAnnotation : das::ManagedStructureAnnotation<acesfx::GravityZoneBuffer>
{
  GravityZoneBufferAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("GravityZoneBuffer", ml, " ::acesfx::GravityZoneBuffer")
  {}
};


static char dng_fx_das[] =
#include "fx.das.inl"
  ;

namespace bind_dascript
{
class FxModule final : public das::Module
{
public:
  FxModule() : das::Module("fx")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("math"));
    addBuiltinDependency(lib, require("ecs"));
    addBuiltinDependency(lib, require("DagorSystem"));
    addEnumeration(das::make_smart<EnumerationFxQuality>());
    addAnnotation(das::make_smart<AcesEffectAnnotation>(lib));
    addAnnotation(das::make_smart<ScaledAcesEffectAnnotation>(lib));
    addAnnotation(das::make_smart<TheEffectAnnotation>(lib));
    addAnnotation(das::make_smart<GravityZoneBufferAnnotation>(lib));
    das::addExtern<DAS_BIND_FUN(effect_set_emitter_tm)>(*this, lib, "effect_set_emitter_tm", das::SideEffects::modifyArgument,
      "bind_dascript::effect_set_emitter_tm");
    das::addExtern<DAS_BIND_FUN(effect_set_scale)>(*this, lib, "effect_set_scale", das::SideEffects::modifyArgument,
      "bind_dascript::effect_set_scale");
    das::addExtern<DAS_BIND_FUN(effect_set_velocity)>(*this, lib, "effect_set_velocity", das::SideEffects::modifyArgument,
      "bind_dascript::effect_set_velocity");
    das::addExtern<DAS_BIND_FUN(effect_set_spawn_rate)>(*this, lib, "effect_set_spawn_rate", das::SideEffects::modifyArgument,
      "bind_dascript::effect_set_spawn_rate");
    das::addExtern<DAS_BIND_FUN(effect_set_gravity_tm)>(*this, lib, "effect_set_gravity_tm", das::SideEffects::modifyArgument,
      "bind_dascript::effect_set_gravity_tm");
    das::addExtern<DAS_BIND_FUN(acesfx::set_gravity_zones)>(*this, lib, "acesfx_set_gravity_zones",
      das::SideEffects::modifyArgumentAndExternal, "::acesfx::set_gravity_zones");
    using method_effect_reset = DAS_CALL_MEMBER(TheEffect::reset);
    das::addExtern<DAS_CALL_METHOD(method_effect_reset)>(*this, lib, "effect_reset", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(TheEffect::reset));
    das::addExtern<DAS_BIND_FUN(start_effect)>(*this, lib, "start_effect", das::SideEffects::modifyExternal,
      "bind_dascript::start_effect");
    das::addExtern<DAS_BIND_FUN(start_effect_block)>(*this, lib, "start_effect", das::SideEffects::modifyExternal,
      "bind_dascript::start_effect_block");
    das::addExtern<DAS_BIND_FUN(acesfx::stop_effect)>(*this, lib, "stop_effect", das::SideEffects::modifyExternal,
      "acesfx::stop_effect");
    das::addExtern<DAS_BIND_FUN(effects_update)>(*this, lib, "effects_update", das::SideEffects::modifyExternal,
      "bind_dascript::effects_update");
    das::addExtern<DAS_BIND_FUN(acesfx::get_type_by_name)>(*this, lib, "get_type_by_name", das::SideEffects::accessExternal,
      "acesfx::get_type_by_name");
    das::addExtern<DAS_BIND_FUN(acesfx::get_effect_life_time)>(*this, lib, "get_effect_life_time", das::SideEffects::accessExternal,
      "acesfx::get_effect_life_time");
    das::addExtern<DAS_BIND_FUN(acesfx::prefetch_effect)>(*this, lib, "prefetch_effect", das::SideEffects::accessExternal,
      "acesfx::prefetch_effect");
    das::addExtern<DAS_BIND_FUN(acesfx::get_fx_target)>(*this, lib, "get_fx_target", das::SideEffects::accessExternal,
      "acesfx::get_fx_target");
    das::addExtern<DAS_BIND_FUN(acesfx::getFxQualityMask)>(*this, lib, "getFxQualityMask", das::SideEffects::accessExternal,
      "acesfx::getFxQualityMask");

    using method_lock = DAS_CALL_MEMBER(AcesEffect::lock);
    das::addExtern<DAS_CALL_METHOD(method_lock)>(*this, lib, "lock", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(AcesEffect::lock));
    using method_setFxScale = DAS_CALL_MEMBER(AcesEffect::setFxScale);
    das::addExtern<DAS_CALL_METHOD(method_setFxScale)>(*this, lib, "setFxScale", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(AcesEffect::setFxScale));
    using method_setVelocity = DAS_CALL_MEMBER(AcesEffect::setVelocity);
    das::addExtern<DAS_CALL_METHOD(method_setVelocity)>(*this, lib, "setVelocity", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(AcesEffect::setVelocity));
    using method_setEmitterTm = DAS_CALL_MEMBER(AcesEffect::setEmitterTm);
    das::addExtern<DAS_CALL_METHOD(method_setEmitterTm)>(*this, lib, "setEmitterTm", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(AcesEffect::setEmitterTm));
    using method_setSpawnRate = DAS_CALL_MEMBER(AcesEffect::setSpawnRate);
    das::addExtern<DAS_CALL_METHOD(method_setSpawnRate)>(*this, lib, "setSpawnRate", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(AcesEffect::setSpawnRate));
    using method_setColorMult = DAS_CALL_MEMBER(AcesEffect::setColorMult);
    das::addExtern<DAS_CALL_METHOD(method_setColorMult)>(*this, lib, "setColorMult", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(AcesEffect::setColorMult));
    using method_setGravityTm = DAS_CALL_MEMBER(AcesEffect::setGravityTm);
    das::addExtern<DAS_CALL_METHOD(method_setGravityTm)>(*this, lib, "setGravityTm", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(AcesEffect::setGravityTm));
    das::addUsing<::acesfx::GravityZoneBuffer>(*this, lib, "::acesfx::GravityZoneBuffer");
    das::addExtern<DAS_BIND_FUN(acesfx::push_gravity_zone)>(*this, lib, "push", das::SideEffects::modifyArgument,
      "::acesfx::push_gravity_zone")
      ->args({"buffer", "transform", "size", "shape", "type", "sq_distance_to_camera", "is_important"});
    compileBuiltinModule("fx.das", (unsigned char *)dng_fx_das, sizeof(dng_fx_das));
    verifyAotReady();
  }

  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"render/dasModules/fx.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(FxModule, bind_dascript);
