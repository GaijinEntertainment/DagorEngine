// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "outline.h"

DAS_BASE_BIND_ENUM_98(OutlineMode, OutlineMode, ZTEST_FAIL, ZTEST_PASS, NO_ZTEST);

namespace bind_dascript
{
class OutlineModule final : public das::Module
{
public:
  OutlineModule() : das::Module("Outline")
  {
    das::ModuleLibrary lib(this);
    addEnumeration(das::make_smart<EnumerationOutlineMode>());
    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <outline/render/dasModules/outline.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript

AUTO_REGISTER_MODULE_IN_NAMESPACE(OutlineModule, bind_dascript);
size_t OutlineModule_pull = 0;