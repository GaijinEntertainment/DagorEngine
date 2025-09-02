options no_aot

require ecs.fs
require ECSGlobalTags


[export]
def load_light_flicker(base_path : string) : bool
  var ok = true

  if ecs_has_tag("render") || is_in_aot()
    ok = load_sub_folder(base_path, "render") && ok

  if ecs_has_tag("sound") || is_in_aot()
    ok = load_sub_folder(base_path, "sound") && ok

  return ok



