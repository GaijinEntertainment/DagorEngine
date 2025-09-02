options no_aot

require ecs.fs
require ECSGlobalTags


[export]
def load_door_sounds(base_path : string) : bool
  var ok = true

  if ecs_has_tag("sound") || is_in_aot()
    ok = load_sub_folder(base_path, "es") && ok

  return ok



