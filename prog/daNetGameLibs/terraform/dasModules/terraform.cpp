// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>
#include "terraform.h"

DAS_BASE_BIND_ENUM_98(::TerraformComponent::PrimMode, TerraformPrimMode, DYN_REPLACE, DYN_ADDITIVE, DYN_MIN, DYN_MAX);

struct TerraformAnnotation : das::ManagedStructureAnnotation<TerraformComponent, false>
{
  TerraformAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("TerraformComponent", ml)
  {
    cppName = " ::TerraformComponent";
  }
};

namespace bind_dascript
{
class TerraformModule final : public das::Module
{
public:
  TerraformModule() : das::Module("terraform")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("ecs"));

    addAnnotation(das::make_smart<TerraformAnnotation>(lib));

    addEnumeration(das::make_smart<EnumerationTerraformPrimMode>());

    using method_update = DAS_CALL_MEMBER(TerraformComponent::update);
    das::addExtern<DAS_CALL_METHOD(method_update)>(*this, lib, "terraform_update", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(TerraformComponent::update));

    using method_storeSphereAlt = DAS_CALL_MEMBER(TerraformComponent::storeSphereAlt);
    das::addExtern<DAS_CALL_METHOD(method_storeSphereAlt)>(*this, lib, "terraform_storeSphereAlt", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(TerraformComponent::storeSphereAlt));

    using method_queueElevationChange = DAS_CALL_MEMBER(TerraformComponent::queueElevationChange);
    das::addExtern<DAS_CALL_METHOD(method_queueElevationChange)>(*this, lib, "terraform_queueElevationChange",
      das::SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(TerraformComponent::queueElevationChange));

    using method_makeBombCraterPart = DAS_CALL_MEMBER(TerraformComponent::makeBombCraterPart);
    das::addExtern<DAS_CALL_METHOD(method_makeBombCraterPart)>(*this, lib, "terraform_makeBombCraterPart",
      das::SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(TerraformComponent::makeBombCraterPart));

    using method_getHmapHeightOrigValAtPos = DAS_CALL_MEMBER(TerraformComponent::getHmapHeightOrigValAtPos);
    das::addExtern<DAS_CALL_METHOD(method_getHmapHeightOrigValAtPos)>(*this, lib, "terraform_getHmapHeightOrigValAtPos",
      das::SideEffects::none, DAS_CALL_MEMBER_CPP(TerraformComponent::getHmapHeightOrigValAtPos));

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"terraform/dasModules/terraform.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
AUTO_REGISTER_MODULE_IN_NAMESPACE(TerraformModule, bind_dascript);
size_t TerraformModule_pull = 0;