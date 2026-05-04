options no_aot

require ecs.fs
require ECSGlobalTags

[export]
def load_anim_replay(base_path : string) : bool
  var ok = true

  let requiredModulesExists = typeinfo builtin_module_exists(DagorImgui) && typeinfo builtin_module_exists(DagorEditor)
  if ecs_has_tag("render") && ecs_has_tag("dev") && requiredModulesExists && !is_in_aot()
    ok = load_sub_folder(base_path, "main") && ok
    if typeinfo builtin_module_exists(stddlg)
      ok = load_sub_folder(base_path, "fileDialog") && ok
    if typeinfo builtin_module_exists(AssetsImport)
      ok = load_sub_folder(base_path, "animGraphReload") && ok

  return ok