// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotAnimatedPhys.h>

struct AnimatedPhysAnnotation : das::ManagedStructureAnnotation<AnimatedPhys, false>
{
  AnimatedPhysAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimatedPhys", ml) { cppName = " ::AnimatedPhys"; }
};

namespace bind_dascript
{
class AnimatedPhysModule final : public das::Module
{
public:
  AnimatedPhysModule() : das::Module("AnimatedPhys")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("AnimV20"));
    addBuiltinDependency(lib, require("PhysVars"));

    addAnnotation(das::make_smart<AnimatedPhysAnnotation>(lib));

    das::addExtern<DAS_BIND_FUN(anim_phys_init)>(*this, lib, "anim_phys_init", das::SideEffects::modifyArgument,
      "bind_dascript::anim_phys_init");
    das::addExtern<DAS_BIND_FUN(anim_phys_update)>(*this, lib, "anim_phys_update", das::SideEffects::modifyArgument,
      "bind_dascript::anim_phys_update");
    das::addExtern<DAS_BIND_FUN(anim_phys_append_var)>(*this, lib, "anim_phys_append_var", das::SideEffects::modifyArgument,
      "bind_dascript::anim_phys_append_var");
    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotAnimatedPhys.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(AnimatedPhysModule, bind_dascript);
