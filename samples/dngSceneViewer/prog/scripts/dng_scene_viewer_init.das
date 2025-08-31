options no_aot = true//since this is merely loader
require app
require ecs
require DagorSystem
require danetlibs.frame_graph_renderer.frame_graph_renderer_init
require ECSGlobalTags

[init]
def load
  var ok = true
  if is_in_aot() || (get_DAGOR_DBGLEVEL() > 0 && find_index(get_command_line_arguments(), "--das-no-linter") == -1)
    ok = load_das("%danetlibs/dascript_base/linter.das") && ok

  if is_in_aot()
    assert(ok)
