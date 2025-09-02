options no_aot

require ecs.fs
require ECSGlobalTags
require RenderLibsAllowed

[export]
def load_pathfinder(base_path : string) : bool
  if !is_render_lib_allowed("pathfinder")
    return true

  var ok = true

  ok = load_sub_folder(base_path, "main") && ok

  return ok



