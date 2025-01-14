// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotVideoPlayer.h>
#include <dasModules/aotDagorDriver3d.h>


namespace bind_dascript
{

struct IGenVideoPlayerAnnotation : das::ManagedStructureAnnotation<IGenVideoPlayer, false>
{
  IGenVideoPlayerAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("IGenVideoPlayer", ml)
  {
    cppName = " ::IGenVideoPlayer";
  }
};

struct VideoPlayer final : public das::Module
{
  VideoPlayer() : das::Module("VideoPlayer")
  {
    das::ModuleLibrary lib(this);
    lib.addBuiltInModule();
    addBuiltinDependency(lib, require("DagorShaders"));
    addBuiltinDependency(lib, require("DagorDriver3D"));

    addAnnotation(das::make_smart<IGenVideoPlayerAnnotation>(lib));

#define BIND(fn, se) das::addExtern<DAS_BIND_FUN(IGenVideoPlayer::fn)>(*this, lib, #fn, se, "::IGenVideoPlayer::" #fn)
    BIND(create_av1_video_player, das::SideEffects::accessExternal);
    BIND(create_ogg_video_player, das::SideEffects::accessExternal);
#undef BIND

#define ADD_METHOD(name, side_effects)                          \
  using method_##name = DAS_CALL_MEMBER(IGenVideoPlayer::name); \
  das::addExtern<DAS_CALL_METHOD(method_##name)>(*this, lib, #name, side_effects, DAS_CALL_MEMBER_CPP(IGenVideoPlayer::name));

    ADD_METHOD(setAutoRewind, das::SideEffects::modifyArgument)
    ADD_METHOD(start, das::SideEffects::modifyArgument)
    ADD_METHOD(stop, das::SideEffects::modifyArgument)
    ADD_METHOD(advance, das::SideEffects::modifyArgument)
    ADD_METHOD(destroy, das::SideEffects::modifyArgument)
    ADD_METHOD(getFrame, das::SideEffects::none)
    ADD_METHOD(getFrameSize, das::SideEffects::none)

#undef ADD_METHOD

    verifyAotReady();
  }

  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotVideoPlayer.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(VideoPlayer, bind_dascript);