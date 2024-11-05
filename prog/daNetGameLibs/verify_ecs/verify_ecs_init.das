options no_aot

require ecs
require ecs.fs
require app
require ECSGlobalTags


[export, unused_argument(base_path)]
def load_verify_ecs(base_path : string) : bool
  var ok = true

  if ecs_has_tag("dev") || app::get_game_name() == "aot"
    ok = load_sub_folder(base_path, "main") && ok

  return ok



