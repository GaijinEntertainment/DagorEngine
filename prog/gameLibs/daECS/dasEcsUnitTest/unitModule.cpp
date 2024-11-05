// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/dasModulesCommon.h>
#include <EASTL/vector_map.h>
#include "unitModule.h"

static eastl::vector_map<eastl::string, int> values;
void set_test_value(const char *k, int v) { values[k] = v; }
int get_test_value(const char *k)
{
  auto it = values.find(k);
  return it == values.end() ? -1 : it->second;
}

namespace bind_dascript
{

class DasEcsUnitTest final : public das::Module
{
public:
  DasEcsUnitTest() : das::Module("DasEcsUnitTest")
  {
    das::ModuleLibrary lib(this);
    das::addExtern<DAS_BIND_FUN(set_test_value)>(*this, lib, "set_test_value", das::SideEffects::modifyExternal, "set_test_value");
    das::addExtern<DAS_BIND_FUN(get_test_value)>(*this, lib, "get_test_value", das::SideEffects::modifyExternal, "get_test_value");
    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"unitModule.h\"\n";
    return das::ModuleAotType::cpp;
  }
};

} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(DasEcsUnitTest, bind_dascript)