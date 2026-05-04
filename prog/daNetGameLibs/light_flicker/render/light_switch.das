require ecs

[es(tag=render, track=light_switch__group, on_appear)]
def light_switchable_init(evt : Event; var light_switch__on : bool&; light_switch__group : string)
  if light_switch__group != ""
    query() <| $ [es] (var light_switcher__state : Object&)
      if light_switcher__state[light_switch__group] == null
        light_switcher__state |> set(light_switch__group, light_switch__on)
      else
        light_switch__on = get_bool(light_switcher__state, light_switch__group) ?? light_switch__on

[es(tag=render, track=light_switcher__state)]
def light_switchable_update(evt : Event; light_switcher__state : Object)
  query() <| $ [es] (var light_switch__on : bool&; light_switch__group : string)
    light_switch__on = get_bool(light_switcher__state, light_switch__group) ?? light_switch__on