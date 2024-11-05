options no_aot

require ecs
require ecs.fs
require app
require ECSGlobalTags


[export]
def load_ri_phys(base_path : string) : bool
  var ok = true

  ok = load_sub_folder(base_path, "main") && ok

  return ok



