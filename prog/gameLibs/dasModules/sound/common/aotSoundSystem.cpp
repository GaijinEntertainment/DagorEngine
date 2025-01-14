// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotSoundSystem.h>

#define SND_BIND_FUN_EX(FUN, NAME, SIDE_EFFECTS) \
  das::addExtern<DAS_BIND_FUN(FUN)>(*this, lib, NAME, SIDE_EFFECTS, "soundsystem_bind_dascript::" #FUN)
#define SND_BIND_FUN(FUN, SIDE_EFFECTS) SND_BIND_FUN_EX(FUN, #FUN, SIDE_EFFECTS)

namespace soundsystem_bind_dascript
{
class SoundSystemModule final : public das::Module
{
public:
  SoundSystemModule() : das::Module("soundSystem")
  {
    das::ModuleLibrary lib(this);

    // functions
    SND_BIND_FUN(have_sound, das::SideEffects::accessExternal);
    SND_BIND_FUN(get_listener_pos, das::SideEffects::accessExternal);
    SND_BIND_FUN(sound_update_listener, das::SideEffects::modifyExternal);
    SND_BIND_FUN(sound_reset_3d_listener, das::SideEffects::modifyExternal);
    SND_BIND_FUN(sound_banks_is_preset_loaded, das::SideEffects::accessExternal);
    SND_BIND_FUN(sound_debug, das::SideEffects::modifyExternal);
    SND_BIND_FUN(sound_enable_distant_delay, das::SideEffects::modifyExternal);
    SND_BIND_FUN(sound_release_delayed_events, das::SideEffects::modifyExternal);
    SND_BIND_FUN(sound_override_time_speed, das::SideEffects::modifyExternal);
    SND_BIND_FUN(sound_banks_enable_preset, das::SideEffects::modifyExternal);
    SND_BIND_FUN(sound_banks_enable_preset_starting_with, das::SideEffects::modifyExternal);
    SND_BIND_FUN(sound_banks_is_preset_enabled, das::SideEffects::accessExternal);
    SND_BIND_FUN(sound_banks_is_preset_exist, das::SideEffects::accessExternal);
    SND_BIND_FUN(sound_debug_enum_events, das::SideEffects::accessExternal);
    SND_BIND_FUN(sound_debug_enum_events_in_bank, das::SideEffects::modifyArgumentAndAccessExternal);

    verifyAotReady();
  }

  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotSoundSystem.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace soundsystem_bind_dascript

REGISTER_MODULE_IN_NAMESPACE(SoundSystemModule, soundsystem_bind_dascript);
