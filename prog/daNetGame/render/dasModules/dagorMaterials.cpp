// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/anim/anim.h>
#include "dagorMaterials.h"
#include <dasModules/dasShaders.h>
#include <dasModules/aotAnimchar.h>

namespace bind_dascript
{
class DagorMaterials final : public das::Module
{
public:
  DagorMaterials() : das::Module("DagorMaterials")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("DagorShaders"), true);
    addBuiltinDependency(lib, require("AnimV20"));

    das::addExtern<DAS_BIND_FUN(bind_dascript::recreate_material_with_new_params)>(*this, lib, "recreate_material",
      das::SideEffects::modifyArgument, "bind_dascript::recreate_material_with_new_params");
    das::addExtern<DAS_BIND_FUN(bind_dascript::recreate_material_with_new_params_1)>(*this, lib, "recreate_material",
      das::SideEffects::modifyArgument, "bind_dascript::recreate_material_with_new_params_1");
    das::addExtern<DAS_BIND_FUN(bind_dascript::recreate_material_with_new_params_2)>(*this, lib, "recreate_material",
      das::SideEffects::modifyArgument, "bind_dascript::recreate_material_with_new_params_2");

    das::addExtern<DAS_BIND_FUN(bind_dascript::scene_lods_gather_mats)>(*this, lib, "scene_lods_gather_mats",
      das::SideEffects::accessExternal, "::bind_dascript::scene_lods_gather_mats");

    verifyAotReady();
  }

  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"render/dasModules/dagorMaterials.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(DagorMaterials, bind_dascript);