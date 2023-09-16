options no_aot = true//since this is merely loader
require ecs
require DagorSystem
require ECSGlobalTags


[init]
def load
  var ok = true
  if get_DAGOR_DBGLEVEL() > 0
    ok = load_das("%danetlibs/dascript_base/linter.das") && ok


