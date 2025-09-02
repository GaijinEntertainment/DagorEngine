options no_aot

require ecs.fs
require ECSGlobalTags


[export]
def load_flash_blind(base_path : string) : bool
  var ok = true
  if ecs_has_tag("render") || is_in_aot()
    ok = load_sub_folder(base_path, "render") && ok

  return ok



