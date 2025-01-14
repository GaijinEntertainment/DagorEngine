options no_aot = true//since this is merely loader
require ecs
require ecs.fs
require app
require ECSGlobalTags
require danetlibs.frame_graph_renderer.frame_graph_renderer_init

def private load_input() : bool
  var ok = true
  if ecs_has_tag("input") || app::get_game_name() == "aot"
    ok = load_folder("%scripts/outer_space/es/input") && ok
    ok = load_folder("%scripts/outer_space/es/client") && ok
  return ok

def private load_sound() : bool
  var ok = true
  if ecs_has_tag("sound") || app::get_game_name() == "aot"
    ok = load_folder("%scripts/outer_space/es/sound") && ok
  return ok

[init]
def load
  var ok = true
  ok = load_folder("%scripts/outer_space/es") && ok
  ok = load_input() && ok
  ok = load_sound() && ok
  ok = load_frame_graph_renderer("%danetlibs/frame_graph_renderer") && ok

  if app::get_game_name() == "aot"
    assert(ok)
