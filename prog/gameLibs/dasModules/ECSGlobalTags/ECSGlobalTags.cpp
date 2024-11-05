// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotECSGlobalTags.h>

namespace bind_dascript
{
class ECSGlobalTagsModule final : public das::Module
{
public:
  ECSGlobalTagsModule() : das::Module("ECSGlobalTags")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("ecs"));

    das::addExtern<DAS_BIND_FUN(ecs_has_tag)>(*this, lib, "ecs_has_tag", das::SideEffects::accessExternal,
      "bind_dascript::ecs_has_tag");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotECSGlobalTags.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(ECSGlobalTagsModule, bind_dascript);
