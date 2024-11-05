options no_aot

require app
require ecs.fs
require ECSGlobalTags


[export]
def load_pathfinder(base_path : string) : bool
  var ok = true

  ok = load_sub_folder(base_path, "main") && ok

  return ok



