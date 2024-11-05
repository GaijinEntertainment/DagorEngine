options no_aot

require app
require ecs.fs
require ECSGlobalTags


[export]
def load_retinal_damage(base_path : string) : bool
  var ok = true
  if ecs_has_tag("render") || app::get_game_name() == "aot"
    ok = load_sub_folder(base_path, "render") && ok

    if ecs_has_tag("dev") || app::get_game_name() == "aot"
      ok = load_sub_folder(base_path, "debug") && ok

  return ok



