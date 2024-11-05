// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotCameraShaker.h>

struct CameraShakerAnnotation : das::ManagedStructureAnnotation<CameraShaker, false>
{
  CameraShakerAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("CameraShaker", ml)
  {
    cppName = " ::CameraShaker";
    addField<DAS_BIND_MANAGED_FIELD(shakeFadeCoeff)>("shakeFadeCoeff");
    addField<DAS_BIND_MANAGED_FIELD(shakeSmoothFadeCoeff)>("shakeSmoothFadeCoeff");
  }
};

namespace bind_dascript
{
class CameraShakerModule final : public das::Module
{
public:
  CameraShakerModule() : das::Module("CameraShaker")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("ecs"));
    addBuiltinDependency(lib, require("math"));

    addAnnotation(das::make_smart<CameraShakerAnnotation>(lib));

    das::addExtern<DAS_BIND_FUN(bind_dascript::camera_shaker_update)>(*this, lib, "camera_shaker_update",
      das::SideEffects::modifyArgument, "bind_dascript::camera_shaker_update");
    das::addExtern<DAS_BIND_FUN(bind_dascript::camera_shaker_shakeMatrix)>(*this, lib, "camera_shaker_shakeMatrix",
      das::SideEffects::modifyArgument, "bind_dascript::camera_shaker_shakeMatrix");
    das::addExtern<DAS_BIND_FUN(bind_dascript::camera_shaker_setShake)>(*this, lib, "camera_shaker_setShake",
      das::SideEffects::modifyArgument, "bind_dascript::camera_shaker_setShake");
    das::addExtern<DAS_BIND_FUN(bind_dascript::camera_shaker_setShakeHiFreq)>(*this, lib, "camera_shaker_setShakeHiFreq",
      das::SideEffects::modifyArgument, "bind_dascript::camera_shaker_setShakeHiFreq");
    das::addExtern<DAS_BIND_FUN(bind_dascript::camera_shaker_setSmoothShakeHiFreq)>(*this, lib, "camera_shaker_setSmoothShakeHiFreq",
      das::SideEffects::modifyArgument, "bind_dascript::camera_shaker_setSmoothShakeHiFreq");
    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotCameraShaker.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(CameraShakerModule, bind_dascript);
