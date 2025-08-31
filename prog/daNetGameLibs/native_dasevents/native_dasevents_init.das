options no_aot

require ecs
require ecs.fs
require ECSGlobalTags


[export, unused_argument(base_path)]
def load_native_dasevents(base_path : string) : bool
  var ok = true

  ok = load_sub_folder(base_path, "main") && ok

  return ok
