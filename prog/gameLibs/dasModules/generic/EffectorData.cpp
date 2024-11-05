// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/anim/animCharEffectors.h>
#include "dasModules/aotEffectorData.h"

struct EffectorDataAnnotation : das::ManagedStructureAnnotation<EffectorData, false>
{
  EffectorDataAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("EffectorData", ml)
  {
    cppName = " ::EffectorData";
    addField<DAS_BIND_MANAGED_FIELD(wtm)>("wtm");
    addField<DAS_BIND_MANAGED_FIELD(position)>("position");
    addField<DAS_BIND_MANAGED_FIELD(effId)>("effId");
    addField<DAS_BIND_MANAGED_FIELD(effWtmId)>("effWtmId");
    addField<DAS_BIND_MANAGED_FIELD(weight)>("weight");
  }
};

namespace bind_dascript
{
class EffectorDataModule final : public das::Module
{
public:
  EffectorDataModule() : das::Module("EffectorData")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("ecs"));
    addBuiltinDependency(lib, require("math"));

    addAnnotation(das::make_smart<EffectorDataAnnotation>(lib));
    das::addExtern<DAS_BIND_FUN(bind_dascript::getNullableRW_EffectorData)>(*this, lib, "getNullableRW_EffectorData",
      das::SideEffects::modifyArgument, "bind_dascript::getNullableRW_EffectorData");
    das::addExtern<DAS_BIND_FUN(bind_dascript::getNullable_EffectorData)>(*this, lib, "getNullable_EffectorData",
      das::SideEffects::none, "bind_dascript::getNullable_EffectorData");

    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"dasModules/aotEffectorData.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(EffectorDataModule, bind_dascript);
