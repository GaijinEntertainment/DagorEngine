options no_aot

require ecs.fs
require ECSGlobalTags


[export]
def load_sound_utils(base_path : string) : bool
  var ok = true

  if ecs_has_tag("sound") || is_in_aot()
    ok = load_sub_folder(base_path, "es") && ok
    ok = load_sub_folder(base_path, "modules") && ok
    if typeinfo builtin_module_exists(gpuReadbackQuery) && typeinfo builtin_module_exists(puddleQueryManager)
      ok = load_sub_folder(base_path, "es/gpu_sound_queries") && ok
  return ok



