// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotDaphys.h>

struct ResolveContactParamsAnnotation : das::ManagedStructureAnnotation<daphys::ResolveContactParams, false>
{
  ResolveContactParamsAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ResolveContactParams", ml)
  {
    cppName = " ::daphys::ResolveContactParams";
    addField<DAS_BIND_MANAGED_FIELD(friction)>("friction");
    addField<DAS_BIND_MANAGED_FIELD(minFriction)>("minFriction");
    addField<DAS_BIND_MANAGED_FIELD(maxFriction)>("maxFriction");
    addField<DAS_BIND_MANAGED_FIELD(bounce)>("bounce");
    addField<DAS_BIND_MANAGED_FIELD(minSpeedForBounce)>("minSpeedForBounce");
    addField<DAS_BIND_MANAGED_FIELD(energyConservation)>("energyConservation");
  }
};


namespace bind_dascript
{
static char daphys_das[] =
#include "daphys.das.inl"
  ;

class DaphysModule final : public das::Module
{
public:
  DaphysModule() : das::Module("Daphys")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("Dacoll"));
    addBuiltinDependency(lib, require("DagorMath"));
    addAnnotation(das::make_smart<ResolveContactParamsAnnotation>(lib));
    das::addCtorAndUsing<daphys::ResolveContactParams>(*this, lib, "ResolveContactParams", " ::daphys::ResolveContactParams");
    das::addExtern<DAS_BIND_FUN(daphys_resolve_penetration)>(*this, lib, "resolve_penetration",
      das::SideEffects::modifyArgumentAndExternal, "bind_dascript::daphys_resolve_penetration");

    das::addExtern<DAS_BIND_FUN(daphys_resolve_contacts)>(*this, lib, "resolve_contacts", das::SideEffects::modifyArgumentAndExternal,
      "bind_dascript::daphys_resolve_contacts");

    compileBuiltinModule("daphys.das", (unsigned char *)daphys_das, sizeof(daphys_das));
    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotDaphys.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(DaphysModule, bind_dascript);