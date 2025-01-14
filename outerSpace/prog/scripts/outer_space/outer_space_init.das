options no_aot = true//since this is merely loader
require outer_space.events_outer_space
require app
require ecs
require DagorSystem
require ECSGlobalTags

[init]
def load
  var ok = true
  if app::get_game_name() == "aot" || (get_DAGOR_DBGLEVEL() > 0 && find_index(get_command_line_arguments(), "--das-no-linter") == -1)
    ok = load_das("%scripts/outer_space/linter.das") && ok

  if (!is_in_aot() && get_DAGOR_DBGLEVEL() > 0 && get_DAGOR_ADDRESS_SANITIZER() == 0 && get_DAGOR_THREAD_SANITIZER() == 0
      && ecs_has_tag("render") && !has_value(get_command_line_arguments(), "--das-no-debugger"))
    ok = load_das_debugger("%danetlibs/dascript_base/debugger_debug.das") && ok

  if app::get_game_name() == "aot"
    assert(ok)
