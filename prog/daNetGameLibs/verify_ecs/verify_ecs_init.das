options no_aot

require ecs
require ecs.fs
require ECSGlobalTags


[export, unused_argument(base_path)]
def load_verify_ecs(base_path : string) : bool
  var ok = true

  if ecs_has_tag("dev") || is_in_aot()
    ok = load_sub_folder(base_path, "main") && ok

  return ok



