// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bloodPuddles.h"
#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotEcs.h>

DAS_BASE_BIND_ENUM(BloodPuddles::DecalGroup,
  BloodPuddlesGroup,
  BLOOD_DECAL_GROUP_PUDDLE,
  BLOOD_DECAL_GROUP_SPLASH,
  BLOOD_DECAL_GROUP_SPRAY,
  BLOOD_DECAL_GROUP_LEAK,
  BLOOD_DECAL_GROUP_FOOTPRINT,
  BLOOD_DECAL_GROUPS_COUNT);

struct BloodPuddlesAnnotation final : das::ManagedStructureAnnotation<BloodPuddles, false>
{
  BloodPuddlesAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("BloodPuddles", ml)
  {
    cppName = " ::BloodPuddles";

    addProperty<DAS_BIND_MANAGED_PROP(getMaxSplashesPerFrame)>("maxSplashesPerFrame", "getMaxSplashesPerFrame");
    addProperty<DAS_BIND_MANAGED_PROP(getCount)>("count", "getCount");
  }
};

struct PuddleCtxAnnotation final : das::ManagedStructureAnnotation<BloodPuddles::PuddleCtx, false>
{
  PuddleCtxAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("PuddleCtx", ml)
  {
    cppName = " BloodPuddles::PuddleCtx";

    addField<DAS_BIND_MANAGED_FIELD(pos)>("pos");
    addField<DAS_BIND_MANAGED_FIELD(normal)>("normal");
    addField<DAS_BIND_MANAGED_FIELD(riEx)>("riEx");
    addField<DAS_BIND_MANAGED_FIELD(puddleDist)>("puddleDist");
    addField<DAS_BIND_MANAGED_FIELD(dir)>("dir");
  }
};

namespace bind_dascript
{

static char blood_puddles_das[] =
#include "bloodPuddles.das.inl"
  ;
class BloodPuddlesModule final : public das::Module
{
public:
  BloodPuddlesModule() : das::Module("BloodPuddles")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("RendInst"));
    addEnumeration(das::make_smart<EnumerationBloodPuddlesGroup>());
    addAnnotation(das::make_smart<BloodPuddlesAnnotation>(lib));
    addAnnotation(das::make_smart<PuddleCtxAnnotation>(lib));
    addConstant(*this, "INVALID_VARIANT", static_cast<int>(BloodPuddles::INVALID_VARIANT));

    das::addExtern<DAS_BIND_FUN(add_hit_blood_effect)>(*this, lib, "add_hit_blood_effect", das::SideEffects::modifyExternal,
      "add_hit_blood_effect");
    das::addExtern<void (*)(const ecs::EntityId, const int), create_blood_puddle_emitter>(*this, lib, "create_blood_puddle_emitter",
      das::SideEffects::modifyExternal, "create_blood_puddle_emitter");

    das::addExtern<DAS_BIND_FUN(init_blood_puddles_mgr)>(*this, lib, "init_blood_puddles_mgr", das::SideEffects::modifyExternal,
      "::init_blood_puddles_mgr");
    das::addExtern<DAS_BIND_FUN(close_blood_puddles_mgr)>(*this, lib, "close_blood_puddles_mgr", das::SideEffects::modifyExternal,
      "::close_blood_puddles_mgr");
    das::addExtern<DAS_BIND_FUN(get_blood_puddles_mgr)>(*this, lib, "get_blood_puddles_mgr", das::SideEffects::accessExternal,
      "::get_blood_puddles_mgr");

    using method_erasePuddles = das::das_call_member<void (BloodPuddles::*)(), &BloodPuddles::erasePuddles>;
    das::addExtern<DAS_CALL_METHOD(method_erasePuddles)>(*this, lib, "erasePuddles", das::SideEffects::accessExternal,
      "das::das_call_member< void(BloodPuddles::*)(), &BloodPuddles::erasePuddles >::invoke");

    using method_update = DAS_CALL_MEMBER(BloodPuddles::update);
    das::addExtern<DAS_CALL_METHOD(method_update)>(*this, lib, "update", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(BloodPuddles::update));
    using method_beforeRender = DAS_CALL_MEMBER(BloodPuddles::beforeRender);
    das::addExtern<DAS_CALL_METHOD(method_beforeRender)>(*this, lib, "beforeRender", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(BloodPuddles::beforeRender));
    using method_addSplashEmitter = DAS_CALL_MEMBER(BloodPuddles::addSplashEmitter);
    das::addExtern<DAS_CALL_METHOD(method_addSplashEmitter)>(*this, lib, "addSplashEmitter", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(BloodPuddles::addSplashEmitter));
    using method_putDecal = das::das_call_member<void (BloodPuddles::*)(int, const Point3 &, const Point3 &, const Point3 &, float,
                                                   rendinst::riex_handle_t, const Point3 &, bool, int, float),
      &BloodPuddles::putDecal>;
    das::addExtern<DAS_CALL_METHOD(method_putDecal)>(*this, lib, "__put_decal", das::SideEffects::modifyArgument,
      "das_call_member<void (BloodPuddles::*)(int, const Point3 &, const Point3 &, const Point3 &, float,"
      "rendinst::riex_handle_t, const Point3 &, bool, int, float), &BloodPuddles::putDecal>::invoke");
    using method_tryPlacePuddle = DAS_CALL_MEMBER(BloodPuddles::tryPlacePuddle);
    das::addExtern<DAS_CALL_METHOD(method_tryPlacePuddle)>(*this, lib, "tryPlacePuddle", das::SideEffects::accessExternal,
      DAS_CALL_MEMBER_CPP(BloodPuddles::tryPlacePuddle));
    using method_addPuddleAt = DAS_CALL_MEMBER(BloodPuddles::addPuddleAt);
    das::addExtern<DAS_CALL_METHOD(method_addPuddleAt)>(*this, lib, "__addPuddleAt", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(BloodPuddles::addPuddleAt));
    using method_addFootprint = DAS_CALL_MEMBER(BloodPuddles::addFootprint);
    das::addExtern<DAS_CALL_METHOD(method_addFootprint)>(*this, lib, "addFootprint", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(BloodPuddles::addFootprint));
    using method_addSplash = DAS_CALL_MEMBER(BloodPuddles::addSplash);
    das::addExtern<DAS_CALL_METHOD(method_addSplash)>(*this, lib, "addSplash", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(BloodPuddles::addSplash));

    das::addUsing<BloodPuddles::PuddleCtx>(*this, lib, "BloodPuddles::PuddleCtx");
    compileBuiltinModule("bloodPuddles.das", (unsigned char *)blood_puddles_das, sizeof(blood_puddles_das));
    verifyAotReady();
  }

  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <blood_puddles/private/dasModules/bloodPuddles.h>\n";
    return das::ModuleAotType::cpp;
  }
};

} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(BloodPuddlesModule, bind_dascript);
