// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dasModules/netPhys.h"
#include <daScript/daScript.h>
#include <dasModules/dasEvent.h>
#include <dasModules/aotDacoll.h>
#include <dasModules/dasModulesCommon.h>

namespace bind_dascript
{

class DngNetPhysModule final : public das::Module
{
public:
  DngNetPhysModule() : das::Module("DngNetPhys")
  {
    das::ModuleLibrary lib(this);

    das::addExtern<DAS_BIND_FUN(getCurInterpDelayTicksPacked)>(*this, lib, "getCurInterpDelayTicksPacked",
      das::SideEffects::accessExternal, "bind_dascript::getCurInterpDelayTicksPacked");

    das::addExtern<DAS_BIND_FUN(test_obj_to_phys_collision)>(*this, lib, "test_obj_to_phys_collision",
      das::SideEffects::modifyExternal, "bind_dascript::test_obj_to_phys_collision");

    verifyAotReady();
  }

  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"dasModules/netPhys.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(DngNetPhysModule, bind_dascript);
