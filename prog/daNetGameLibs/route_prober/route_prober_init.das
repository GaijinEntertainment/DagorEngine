options no_aot

require ecs.fs
require ECSGlobalTags
require RenderLibsAllowed

[export]
def load_route_prober(base_path : string) : bool
  if !is_render_lib_allowed("route_prober")
    return true

  var ok = true

  if ecs_has_tag("gameClient") || is_in_aot()
    ok = load_sub_folder(base_path, "main") && ok

  return ok



