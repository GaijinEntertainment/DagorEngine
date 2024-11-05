// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/daScript.h>
#include "dasModules/capsuleApproximation.h"


struct CapsuleDataAnnotation : das::ManagedStructureAnnotation<CapsuleData, false>
{
  CapsuleDataAnnotation(das::ModuleLibrary &ml) : das::ManagedStructureAnnotation<CapsuleData, false>("CapsuleData", ml)
  {
    cppName = " ::CapsuleData";
    addField<DAS_BIND_MANAGED_FIELD(a)>("a");
    addField<DAS_BIND_MANAGED_FIELD(rad)>("rad");
    addField<DAS_BIND_MANAGED_FIELD(b)>("b");
    addProperty<DAS_BIND_MANAGED_PROP(getNodeIndexAsInt)>("nodeIndex", "getNodeIndexAsInt");
  }
};

struct CapsuleApproximationAnnotation : das::ManagedStructureAnnotation<CapsuleApproximation, false>
{
  CapsuleApproximationAnnotation(das::ModuleLibrary &ml) :
    das::ManagedStructureAnnotation<CapsuleApproximation, false>("CapsuleApproximation", ml)
  {
    cppName = " ::CapsuleApproximation";
    addField<DAS_BIND_MANAGED_FIELD(capsuleDatas)>("capsuleDatas");
    addField<DAS_BIND_MANAGED_FIELD(inited)>("inited");
  }
};

namespace bind_dascript
{
class CapsuleApproximationModule final : public das::Module
{
public:
  CapsuleApproximationModule() : das::Module("CapsuleApproximation")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("math"));
    addBuiltinDependency(lib, require("DagorMath"));
    lib.addBuiltInModule();

    addAnnotation(das::make_smart<CapsuleDataAnnotation>(lib));
    addAnnotation(das::make_smart<CapsuleApproximationAnnotation>(lib));

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"dasModules/capsuleApproximation.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(CapsuleApproximationModule, bind_dascript);