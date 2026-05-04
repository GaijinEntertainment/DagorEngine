// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "daScript/daScriptBind.h"
#include <dasModules/aotProjectiveDecals.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotDagorMath.h>

namespace bind_dascript
{

struct ProjectiveDecalsBaseAnnotation : das::ManagedStructureAnnotation<ProjectiveDecalsBase>
{
  ProjectiveDecalsBaseAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("ProjectiveDecalsBase", ml, "ProjectiveDecalsBase")
  {}
};

struct RingBufferDecalsBaseAnnotation : das::ManagedStructureAnnotation<RingBufferDecalsBase>
{
  RingBufferDecalsBaseAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("RingBufferDecalsBase", ml, "RingBufferDecalsBase")
  {
    addProperty<DAS_BIND_MANAGED_PROP(getDecalsNum)>("decalsCount", "getDecalsNum");
  }
};

struct RingBufferDecalsBaseManagerAnnotation : das::ManagedStructureAnnotation<RingBufferDecalManager>
{
  RingBufferDecalsBaseManagerAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("RingBufferDecalManager", ml, "RingBufferDecalManager")
  {
    addProperty<DAS_BIND_MANAGED_PROP(getDecalsNum)>("decalsCount", "getDecalsNum");
  }
};

struct ResizableDecalsBaseAnnotation : das::ManagedStructureAnnotation<ResizableDecalsBase>
{
  ResizableDecalsBaseAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ResizableDecalsBase", ml)
  {
    cppName = "ResizableDecalsBase";
    addProperty<DAS_BIND_MANAGED_PROP(getDecalsNum)>("decalsCount", "getDecalsNum");
  }
};

struct ResizableDecalManagerAnnotation : das::ManagedStructureAnnotation<ResizableDecalManager>
{
  ResizableDecalManagerAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("ResizableDecalManager", ml, "ResizableDecalManager")
  {
    addProperty<DAS_BIND_MANAGED_PROP(getDecalsNum)>("decalsCount", "getDecalsNum");
  }
};

struct DecalDataBaseAnnotation : das::ManagedStructureAnnotation<DecalDataBase, false>
{
  DecalDataBaseAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("DecalDataBase", ml, "DecalDataBase")
  {
    cppName = "DecalDataBase";
  }
};

struct DefaultDecalDataAnnotation : das::ManagedStructureAnnotation<DefaultDecalData, false>
{
  DefaultDecalDataAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("DefaultDecalData", ml, "DefaultDecalData")
  {
    cppName = "DefaultDecalData";
  }
};

class ProjectiveDecalsModule final : public das::Module
{
public:
  ProjectiveDecalsModule() : das::Module("ProjectiveDecals")
  {
    das::ModuleLibrary lib(this);
    auto projectiveDecals = das::make_smart<ProjectiveDecalsBaseAnnotation>(lib);
    addAnnotation(projectiveDecals);
    add_annotation(this, das::make_smart<RingBufferDecalsBaseAnnotation>(lib), projectiveDecals);
    add_annotation(this, das::make_smart<ResizableDecalsBaseAnnotation>(lib), projectiveDecals);
    addAnnotation(das::make_smart<RingBufferDecalsBaseManagerAnnotation>(lib));
    addAnnotation(das::make_smart<ResizableDecalManagerAnnotation>(lib));
    auto decalDataBase = das::make_smart<DecalDataBaseAnnotation>(lib);
    addAnnotation(decalDataBase);
    add_annotation(this, das::make_smart<DefaultDecalDataAnnotation>(lib), decalDataBase);
#define CLASS_MEMBER(FUNC_NAME, SYNONIM, SIDE_EFFECT)                                                                \
  {                                                                                                                  \
    using memberMethod = DAS_CALL_MEMBER(FUNC_NAME);                                                                 \
    das::addExtern<DAS_CALL_METHOD(memberMethod)>(*this, lib, SYNONIM, SIDE_EFFECT, DAS_CALL_MEMBER_CPP(FUNC_NAME)); \
  }
#define CLASS_MEMBER_SIGNATURE(FUNC_NAME, SYNONIM, SIDE_EFFECT, SIGNATURE)          \
  {                                                                                 \
    using memberMethod = das::das_call_member<SIGNATURE, &FUNC_NAME>;               \
    das::addExtern<DAS_CALL_METHOD(memberMethod)>(*this, lib, SYNONIM, SIDE_EFFECT, \
      "das_call_member<" #SIGNATURE ", &" #FUNC_NAME ">::invoke");                  \
  }


    DAS_ADD_METHOD_BIND_VALUE_RET("addDecal", modifyArgument, RingBufferDecalManager::addDecal);
    CLASS_MEMBER(RingBufferDecalManager::clear, "clear", das::SideEffects::modifyArgument)
    CLASS_MEMBER(RingBufferDecalManager::afterReset, "afterReset", das::SideEffects::modifyArgument)
    CLASS_MEMBER(RingBufferDecalManager::init, "init", das::SideEffects::modifyArgument)
    CLASS_MEMBER(RingBufferDecalManager::init_buffer, "init_buffer", das::SideEffects::modifyArgument)
    CLASS_MEMBER(RingBufferDecalManager::prepareRender, "prepareRender", das::SideEffects::modifyArgument)
    CLASS_MEMBER(RingBufferDecalManager::render, "render", das::SideEffects::modifyArgument)

    CLASS_MEMBER(RingBufferDecalManager::updateDecal, "updateDecal", das::SideEffects::modifyArgument)
    CLASS_MEMBER(ResizableDecalManager::updateDecal, "updateDecal", das::SideEffects::modifyArgument)
    CLASS_MEMBER(ResizableDecalManager::partialUpdate, "partialUpdate", das::SideEffects::modifyArgument)


    das::addExtern<DAS_BIND_FUN(bind_dascript::make_default_decal_data), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib,
      "DefaultDecalData", das::SideEffects::none, "bind_dascript::make_default_decal_data");
    das::addExtern<DAS_BIND_FUN(bind_dascript::make_update_params_decal_data), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib,
      "UpdateParamZData", das::SideEffects::none, "bind_dascript::make_update_params_decal_data");

    das::addConstant<uint16_t>(*this, "UPDATE_ALL", UPDATE_ALL);
    das::addConstant<uint16_t>(*this, "UPDATE_PARAM_Z", UPDATE_PARAM_Z);
    verifyAotReady();
  }

  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotProjectiveDecals.h>\n";
    tw << "#include <ecs/render/decalsES.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(ProjectiveDecalsModule, bind_dascript);
