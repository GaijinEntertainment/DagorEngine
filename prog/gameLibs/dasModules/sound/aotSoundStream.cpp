// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotSoundStream.h>

#define SND_BIND_FUN_EX(FUN, NAME, SIDE_EFFECTS) \
  das::addExtern<DAS_BIND_FUN(FUN)>(*this, lib, NAME, SIDE_EFFECTS, "soundstream_bind_dascript::" #FUN)
#define SND_BIND_FUN(FUN, SIDE_EFFECTS) SND_BIND_FUN_EX(FUN, #FUN, SIDE_EFFECTS)

DAS_BASE_BIND_ENUM(SoundStreamState, SoundStreamState, ERROR, CLOSED, OPENED, CONNECTING, BUFFERING, STOPPED, PAUSED, PLAYING)

namespace soundstream_bind_dascript
{
struct SoundStreamAnnotation final : das::ManagedStructureAnnotation<SoundStream, false>
{
  SoundStreamAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("SoundStream", ml) { cppName = " ::SoundStream"; }
  void walk(das::DataWalker &walker, void *data) override
  {
    if (!walker.reading)
    {
      const SoundStream *t = (SoundStream *)data;
      int32_t eidV = int32_t(sndsys::sound_handle_t(t->handle));
      walker.Int(eidV);
    }
  }
  bool canCopy() const override { return false; }
  bool canMove() const override { return false; }
  bool canClone() const override { return false; }
};

class SoundStreamModule final : public das::Module
{
public:
  SoundStreamModule() : das::Module("soundStream")
  {
    das::ModuleLibrary lib(this);
    addEnumeration(das::make_smart<EnumerationSoundStreamState>());
    addAnnotation(das::make_smart<SoundStreamAnnotation>(lib));

    // functions
    SND_BIND_FUN(init, das::SideEffects::modifyArgumentAndExternal);
    SND_BIND_FUN(open, das::SideEffects::modifyExternal);
    SND_BIND_FUN(release, das::SideEffects::modifyArgumentAndExternal);
    SND_BIND_FUN(set_fader, das::SideEffects::modifyExternal);
    SND_BIND_FUN(set_pos, das::SideEffects::modifyExternal);
    SND_BIND_FUN(set_volume, das::SideEffects::modifyExternal);
    SND_BIND_FUN(get_state, das::SideEffects::accessExternal);
    SND_BIND_FUN(is_valid, das::SideEffects::accessExternal);

    verifyAotReady();
  }

  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotSoundStream.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace soundstream_bind_dascript

REGISTER_MODULE_IN_NAMESPACE(SoundStreamModule, soundstream_bind_dascript);
