options no_aot

require app
require ecs.fs
require ECSGlobalTags


[export, unused_argument(base_path)]
def load_frame_graph_renderer(base_path : string) : bool
  var ok = true
  if ecs_has_tag("render") || app::get_game_name() == "aot"
    ok = load_sub_folder(base_path, "render") && ok
  return ok
