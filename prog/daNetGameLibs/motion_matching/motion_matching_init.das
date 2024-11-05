options no_aot

require app
require ecs.fs
require ECSGlobalTags

[export]
def load_motion_matching(base_path : string) : bool
  var ok = true

  if ecs_has_tag("render") || app::get_game_name() == "aot"
    ok = load_sub_folder(base_path, "es") && ok

    if ecs_has_tag("dev") || app::get_game_name() == "aot"
      ok = load_sub_folder(base_path, "debug_es") && ok

    if (ecs_has_tag("dev") && typeinfo(builtin_module_exists DagorImgui)) || app::get_game_name() == "aot"
      ok = load_sub_folder(base_path, "debug_ui") && ok
  return ok



