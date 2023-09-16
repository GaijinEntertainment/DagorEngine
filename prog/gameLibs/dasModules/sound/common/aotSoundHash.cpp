#include <dasModules/aotSoundHash.h>

#define SND_BIND_FUN_EX(FUN, NAME, SIDE_EFFECTS) \
  das::addExtern<DAS_BIND_FUN(FUN)>(*this, lib, NAME, SIDE_EFFECTS, "bind_dascript::" #FUN)
#define SND_BIND_FUN(FUN, SIDE_EFFECTS) SND_BIND_FUN_EX(FUN, #FUN, SIDE_EFFECTS)

namespace bind_dascript
{
class SoundHashModule final : public das::Module
{
public:
  SoundHashModule() : das::Module("soundHash")
  {
    das::ModuleLibrary lib(this);

    // functions
    SND_BIND_FUN(sound_hash, das::SideEffects::none);

    verifyAotReady();
  }

  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotSoundHash.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(SoundHashModule, bind_dascript);
