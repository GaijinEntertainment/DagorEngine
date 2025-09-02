// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>
#include <ecs/scripts/dasEcsEntity.h>
#include "render/cameraUtils.h"
#include "camera/sceneCam.h"
#include <dasModules/aotEcs.h>

DAS_BIND_ENUM_CAST_98(CreateCameraFlags);
DAS_BASE_BIND_ENUM_98(CreateCameraFlags, CreateCameraFlags, CCF_SET_TARGET_TEAM, CCF_SET_ENTITY);

namespace bind_dascript
{
class DngCameraModule final : public das::Module
{
public:
  DngCameraModule() : das::Module("DngCamera")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("ecs"));

    addEnumeration(das::make_smart<EnumerationCreateCameraFlags>());

    das::addExtern<DAS_BIND_FUN(get_cur_cam_entity)>(*this, lib, "get_cur_cam_entity", das::SideEffects::accessExternal,
      "get_cur_cam_entity");
    das::addExtern<DAS_BIND_FUN(set_scene_camera_entity)>(*this, lib, "set_scene_camera_entity", das::SideEffects::modifyExternal,
      "set_scene_camera_entity");
    das::addExtern<DAS_BIND_FUN(::enable_spectator_camera)>(*this, lib, "enable_spectator_camera", das::SideEffects::modifyExternal,
      "::enable_spectator_camera");
    das::addExtern<DAS_BIND_FUN(toggle_free_camera)>(*this, lib, "toggle_free_camera", das::SideEffects::modifyExternal,
      "::toggle_free_camera");
    das::addExtern<DAS_BIND_FUN(enable_free_camera)>(*this, lib, "enable_free_camera", das::SideEffects::modifyExternal,
      "::enable_free_camera");
    das::addExtern<DAS_BIND_FUN(disable_free_camera)>(*this, lib, "disable_free_camera", das::SideEffects::modifyExternal,
      "::disable_free_camera");
    das::addExtern<DAS_BIND_FUN(set_cam_itm)>(*this, lib, "set_cam_itm", das::SideEffects::modifyExternal, "::set_cam_itm");
    das::addExtern<DAS_BIND_FUN(get_free_cam_speeds)>(*this, lib, "get_free_cam_speeds", das::SideEffects::modifyExternal,
      "::get_free_cam_speeds");

    das::addExtern<void (*)(const Point2 &, Point3 &, Point3 &), screen_to_world>(*this, lib, "screen_to_world",
      das::SideEffects::modifyArgumentAndAccessExternal, "::screen_to_world");

    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"camera/sceneCam.h\"\n";
    tw << "#include <ecs/scripts/dasEcsEntity.h>\n";
    tw << "#include \"render/cameraUtils.h\"\n";
    tw << "#include <dasModules/aotEcs.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(DngCameraModule, bind_dascript);
