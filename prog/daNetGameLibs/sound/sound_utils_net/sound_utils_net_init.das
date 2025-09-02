options no_aot

require ecs.fs
require ECSGlobalTags


[export]
def load_sound_utils_net(base_path : string) : bool
  var ok = true

  if ecs_has_tag("soundNet") || is_in_aot()
    ok = load_sub_folder(base_path, "modules") && ok
    ok = load_sub_folder(base_path, "es") && ok

  return ok



