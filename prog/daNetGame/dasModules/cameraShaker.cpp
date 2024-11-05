// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dasModules/cameraShaker.h"

namespace bind_dascript
{
class DngCameraShakerModule final : public das::Module
{
public:
  DngCameraShakerModule() : das::Module("DngCameraShaker")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("ecs"));
    addBuiltinDependency(lib, require("math"));
    addBuiltinDependency(lib, require("CameraShaker"));

    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"dasModules/cameraShaker.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(DngCameraShakerModule, bind_dascript);
