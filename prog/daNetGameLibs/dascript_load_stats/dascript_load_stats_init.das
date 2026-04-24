options no_aot

require ecs.fs


[export, unused_argument(base_path)]
def load_dascript_load_stats(base_path : string) : bool
  var ok = true

  ok = load_sub_folder(base_path, "main") && ok

  return ok
