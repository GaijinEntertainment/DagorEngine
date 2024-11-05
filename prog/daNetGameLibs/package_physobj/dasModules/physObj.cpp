// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "physObj.h"

struct PhysObjActorAnnotation final : das::ManagedStructureAnnotation<PhysObjActor, false>
{
  das::TypeDeclPtr parentType;

  PhysObjActorAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("PhysObjActor", ml)
  {
    cppName = " ::PhysObjActor";
    addField<DAS_BIND_MANAGED_FIELD(phys)>("phys");
    addProperty<DAS_BIND_MANAGED_PROP(isAsleep)>("isAsleep", "isAsleep");

    parentType = das::makeType<BasePhysActor>(ml);
  }

  bool canBeSubstituted(TypeAnnotation *pass_type) const override { return parentType->annotation == pass_type; }
};


namespace bind_dascript
{
class DngPhysObjModule final : public das::Module
{
public:
  DngPhysObjModule() : das::Module("DngPhysObj")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("PhysObj"));
    addBuiltinDependency(lib, require("DngActor"));

    addAnnotation(das::make_smart<PhysObjActorAnnotation>(lib));

    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"package_physobj/physObj.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(DngPhysObjModule, bind_dascript);
