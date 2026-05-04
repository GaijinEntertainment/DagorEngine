module das_debug_common shared
require ecs
require DagorConsole

def print_hero()
  let found = find_query() <| $ [es(REQUIRE=hero)] (eid : EntityId)
    console_print("hero: {eid} {getEntityTemplateName(eid)}")
    return true
  if !found
    console_print("hero not found")
