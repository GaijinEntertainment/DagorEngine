options no_aot

require app
require ecs.fs
require DagorSystem
require strings
require ECSGlobalTags


[export]
def load_assets_import(base_path : string) : bool
  var ok = true

  if get_platform_string_id() |> starts_with("win") && typeinfo(builtin_module_exists AssetsImport)

    if ecs_has_tag("gameClient") || app::get_game_name() == "aot"
      ok = load_sub_folder(base_path, "main") && ok

    if typeinfo(builtin_module_exists DagorImgui) || app::get_game_name() == "aot"
      ok = load_sub_folder(base_path, "debug_ui") && ok

  return ok



