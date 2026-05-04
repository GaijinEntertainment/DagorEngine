module light_switch_common shared

require ecs

def is_light_switch_on(group : string) -> bool
  var res = false
  query() <| $ [es] (light_switcher__state : Object)
    res = get_bool(light_switcher__state, group) ?? false
  return res

def light_switch(group : string; enabled : bool)
  query() <| $ [es] (var light_switcher__state : Object&)
    light_switcher__state |> set(group, enabled)