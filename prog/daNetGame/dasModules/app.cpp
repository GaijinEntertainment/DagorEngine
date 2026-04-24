// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/daScript.h>
#include "dasModules/app.h"

#include "net/time.h"
#include "net/userid.h"
#include "main/app.h"

MAKE_TYPE_FACTORY(sceneload::UserGameModeContext, sceneload::UserGameModeContext);

namespace bind_dascript
{
struct UserGameModeContextAnnotation : das::ManagedStructureAnnotation<sceneload::UserGameModeContext, false>
{
  UserGameModeContextAnnotation(das::ModuleLibrary &ml) :
    das::ManagedStructureAnnotation<sceneload::UserGameModeContext, false>("UserGameModeContext", ml)
  {
    cppName = " sceneload::UserGameModeContext";
    addField<DAS_BIND_MANAGED_FIELD(modId)>("modId");
  }
};


class DngAppModule final : public das::Module
{
public:
  DngAppModule() : das::Module("app")
  {
    das::ModuleLibrary lib(this);
    addAnnotation(das::make_smart<UserGameModeContextAnnotation>(lib));

    addBuiltinDependency(lib, require("AppTime"), true);

    das::addExtern<DAS_BIND_FUN(get_timespeed)>(*this, lib, "get_timespeed", das::SideEffects::accessExternal, "::get_timespeed");
    das::addExtern<DAS_BIND_FUN(set_timespeed)>(*this, lib, "set_timespeed", das::SideEffects::modifyExternal, "::set_timespeed");
    das::addExtern<DAS_BIND_FUN(toggle_pause)>(*this, lib, "toggle_pause", das::SideEffects::modifyExternal, "::toggle_pause");

    das::addExtern<DAS_BIND_FUN(get_exe_version_str)>(*this, lib, "get_exe_version_str", das::SideEffects::accessExternal,
      "::get_exe_version_str");

    das::addExtern<DAS_BIND_FUN(app_profile_get_app_id)>(*this, lib, "get_app_id", das::SideEffects::accessExternal,
      "::app_profile_get_app_id");
    das::addExtern<DAS_BIND_FUN(get_current_scene)>(*this, lib, "get_current_scene", das::SideEffects::accessExternal,
      "bind_dascript::get_current_scene");

    das::addExtern<DAS_BIND_FUN(::get_build_number)>(*this, lib, "get_build_number", das::SideEffects::accessExternal,
      "::get_build_number");

    das::addExtern<DAS_BIND_FUN(sceneload::load_game_scene)>(*this, lib, "load_game_scene", das::SideEffects::modifyExternal,
      "sceneload::load_game_scene")
      ->arg_init(/*import_depth*/ 1, das::make_smart<das::ExprConstInt>(1))
      ->arg_init(/*load_type*/ 2, das::make_smart<das::ExprConstUInt>(3 /*IMPORT*/));
    das::addExtern<DAS_BIND_FUN(das_switch_scene_1)>(*this, lib, "switch_scene", das::SideEffects::modifyExternal,
      "bind_dascript::das_switch_scene_1");
    das::addExtern<DAS_BIND_FUN(das_switch_scene_2)>(*this, lib, "switch_scene", das::SideEffects::modifyExternal,
      "bind_dascript::das_switch_scene_2");
    das::addExtern<DAS_BIND_FUN(das_switch_scene)>(*this, lib, "switch_scene", das::SideEffects::modifyExternal,
      "bind_dascript::das_switch_scene");
    das::addExtern<DAS_BIND_FUN(das_switch_scene_and_update)>(*this, lib, "switch_scene_and_update", das::SideEffects::modifyExternal,
      "bind_dascript::das_switch_scene_and_update");
    das::addExtern<DAS_BIND_FUN(das_connect_to_session_1)>(*this, lib, "connect_to_session", das::SideEffects::modifyExternal,
      "bind_dascript::das_connect_to_session_1");
    das::addExtern<DAS_BIND_FUN(das_connect_to_session)>(*this, lib, "connect_to_session", das::SideEffects::modifyExternal,
      "bind_dascript::das_connect_to_session");
    das::addExtern<DAS_BIND_FUN(bind_dascript::exit_game_safe)>(*this, lib, "exit_game", das::SideEffects::modifyExternal,
      "bind_dascript::exit_game_safe");
    das::addExternTempRef<DAS_BIND_FUN(dgs_get_game_params)>(*this, lib, "dgs_get_game_params", das::SideEffects::accessExternal,
      "bind_dascript::dgs_get_game_params");
    das::addExtern<DAS_BIND_FUN(bind_dascript::dgs_has_arg)>(*this, lib, "dgs_has_arg", das::SideEffects::accessExternal,
      "bind_dascript::dgs_has_arg");
    // Warn: return empty string on existing arg without value which in das is indistinguishable from null;
    // use `dgs_has_arg` for if you need that
    // FIXME: return temp string somehow (gc-friendly)
    das::addExtern<const char *(*)(const char *, const char *), &::dgs_get_argv>(*this, lib, "dgs_get_argv",
      das::SideEffects::accessExternal, "::dgs_get_argv")
      ->arg_init(1 /*def_val*/, das::make_smart<das::ExprConstString>());
    das::addExtern<DAS_BIND_FUN(bind_dascript::dgs_get_argv_all)>(*this, lib, "dgs_get_argv_all", das::SideEffects::accessExternal,
      "bind_dascript::dgs_get_argv_all");

    das::addExtern<DAS_BIND_FUN(physmat::init)>(*this, lib, "physmat_init", das::SideEffects::modifyExternal, "physmat::init");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"net/time.h\"\n";
    tw << "#include \"net/userid.h\"\n";
    tw << "#include \"main/app.h\"\n";
    tw << "#include \"main/appProfile.h\"\n";
    tw << "#include \"main/gameLoad.h\"\n";
    tw << "#include \"main/physMat.h\"\n";
    tw << "#include \"dasModules/app.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(DngAppModule, bind_dascript);
