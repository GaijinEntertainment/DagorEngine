options no_aot
require DagorConsole
require danetlibs.light_flicker.render.light_switch_common

[console_cmd(name="light.switch")]
def light_switch_cmd(group : string; enabled : bool?)
  light_switch(group, enabled ?? !is_light_switch_on(group))