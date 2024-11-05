// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotRegExp.h>


struct RegExpAnnotation : das::ManagedStructureAnnotation<RegExp, false>
{
  RegExpAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("RegExp", ml) { cppName = " ::RegExp"; }
};

namespace bind_dascript
{
class RegExpModule final : public das::Module
{
public:
  RegExpModule() : das::Module("RegExp")
  {
    das::ModuleLibrary lib(this);

    addAnnotation(das::make_smart<RegExpAnnotation>(lib));

    das::addUsing<RegExp>(*this, lib, "RegExp");

    using method_compile = DAS_CALL_MEMBER(RegExp::compile);
    das::addExtern<DAS_CALL_METHOD(method_compile)>(*this, lib, "compile", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(RegExp::compile));

    using method_test = DAS_CALL_MEMBER(RegExp::test);
    das::addExtern<DAS_CALL_METHOD(method_test)>(*this, lib, "test", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(RegExp::test));

    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotRegExp.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(RegExpModule, bind_dascript);
