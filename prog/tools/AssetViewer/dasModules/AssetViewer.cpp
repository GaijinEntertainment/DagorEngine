// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/daScript.h>
#include <dasModules/aotAnimchar.h>

extern ::AnimV20::AnimcharBaseComponent *try_get_entity_animchar_base_comp();
extern void add_anim_state_to_history(int state_idx);

namespace bind_dascript
{
class AssetViewerModule final : public das::Module
{
public:
  AssetViewerModule() : das::Module("AssetViewer")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("AnimV20"));

    das::addExtern<DAS_BIND_FUN(try_get_entity_animchar_base_comp)>(*this, lib, "try_get_entity_animchar_base_comp",
      das::SideEffects::worstDefault, "::try_get_entity_animchar_base_comp");
    das::addExtern<DAS_BIND_FUN(add_anim_state_to_history)>(*this, lib, "add_anim_state_to_history", das::SideEffects::worstDefault,
      "::add_anim_state_to_history");

    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &) const override { return das::ModuleAotType::cpp; }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(AssetViewerModule, bind_dascript);
