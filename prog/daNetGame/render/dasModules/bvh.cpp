// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bvh.h"

namespace bind_dascript
{
class BVHModule final : public das::Module
{
public:
  BVHModule() : das::Module("BVH")
  {
    das::ModuleLibrary lib(this);

    das::addExtern<DAS_BIND_FUN(is_bvh_enabled)>(*this, lib, "is_bvh_enabled", das::SideEffects::accessExternal,
      "::bind_dascript::is_bvh_enabled");
    das::addExtern<DAS_BIND_FUN(is_rr_enabled)>(*this, lib, "is_rr_enabled", das::SideEffects::accessExternal,
      "::bind_dascript::is_rr_enabled");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <render/dasModules/bvh.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(BVHModule, bind_dascript);