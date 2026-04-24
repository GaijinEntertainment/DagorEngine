options no_aot

require ecs.fs
require ECSGlobalTags

[export]
def load_motionMatching(base_path : string) : bool
  var ok = true

  if ecs_has_tag("render") || is_in_aot()
    ok = load_sub_folder(base_path, "das") && ok

    if (ecs_has_tag("dev") && typeinfo builtin_module_exists(DngHuman)) || is_in_aot()
      ok = load_sub_folder(base_path, "debug/das/es") && ok

    if (ecs_has_tag("dev") && typeinfo builtin_module_exists(DagorImgui)) || is_in_aot()
      ok = load_sub_folder(base_path, "debug/das/ui") && ok

    if (ecs_has_tag("dev") && typeinfo builtin_module_exists(AssetsImport))
      ok = load_sub_folder(base_path, "debug/das/asset_mgr") && ok

  return ok



