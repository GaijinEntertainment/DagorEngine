// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "aotSoundNetProps.h"

struct ShellSoundNetPropsAnnotation : das::ManagedStructureAnnotation<ShellSoundNetProps, false>
{
  ShellSoundNetPropsAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ShellSoundNetProps", ml)
  {
    cppName = " ShellSoundNetProps";
    addFieldEx("throwPhrase", "throwPhrase", offsetof(ShellSoundNetProps, throwPhrase), das::makeType<char *>(ml));
  }
};

namespace bind_dascript
{
class SoundNetPropsModule final : public das::Module
{
public:
  SoundNetPropsModule() : das::Module("soundNetProps")
  {
    das::ModuleLibrary lib(this);

    addAnnotation(das::make_smart<ShellSoundNetPropsAnnotation>(lib));

    das::addExtern<DAS_BIND_FUN(bind_dascript::das_get_props<ShellSoundNetProps>)>(*this, lib, "shell_sound_net_get_props",
      das::SideEffects::accessExternal, "bind_dascript::das_get_props<ShellSoundNetProps>");
    das::addExtern<DAS_BIND_FUN(bind_dascript::das_try_get_props<ShellSoundNetProps>)>(*this, lib, "shell_sound_net_try_get_props",
      das::SideEffects::accessExternal, "bind_dascript::das_try_get_props<ShellSoundNetProps>");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/sound_net/aotSoundNetProps.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(SoundNetPropsModule, bind_dascript);
