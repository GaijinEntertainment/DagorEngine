options no_aot

require ecs.fs
require ECSGlobalTags


[export]
def load_package_motorbike(base_path : string) : bool
  var ok = load_sub_folder(base_path, "main")

  if ecs_has_tag("input") || is_in_aot()
    ok = load_sub_folder(base_path, "input") && ok

  return ok
