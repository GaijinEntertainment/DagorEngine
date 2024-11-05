// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "aotSoundProps.h"

struct SoundActionPropsAnnotation : das::ManagedStructureAnnotation<sound::ActionProps, false>
{
  SoundActionPropsAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("SoundActionProps", ml)
  {
    cppName = " sound::ActionProps";
    addFieldEx("humanHitSoundPath", "humanHitSoundPath", offsetof(sound::ActionProps, humanHitSoundPath), das::makeType<char *>(ml));
    addFieldEx("humanHitSoundName", "humanHitSoundName", offsetof(sound::ActionProps, humanHitSoundName), das::makeType<char *>(ml));
  }
};

namespace bind_dascript
{
class SoundPropsModule final : public das::Module
{
public:
  SoundPropsModule() : das::Module("soundProps")
  {
    das::ModuleLibrary lib(this);
    addAnnotation(das::make_smart<SoundActionPropsAnnotation>(lib));

    das::addExtern<DAS_BIND_FUN(bind_dascript::das_get_props<sound::ActionProps>)>(*this, lib, "action_sound_get_props",
      das::SideEffects::accessExternal, "bind_dascript::das_get_props<sound::ActionProps>");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/sound/aotSoundProps.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(SoundPropsModule, bind_dascript);
