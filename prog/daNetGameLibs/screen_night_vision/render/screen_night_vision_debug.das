options no_aot
require ecs
require ecs.common
require DagorConsole

[console_cmd(name="screen_night_vision.enable")]
def console_enable_screen_night_vision()
  ecs::query() <| $ [es(REQUIRE=screenNightVisionEffect)] (var screen_night_vision__enabled : bool&)
    screen_night_vision__enabled = true

[console_cmd(name="screen_night_vision.disable")]
def console_disable_screen_night_vision()
  ecs::query() <| $ [es(REQUIRE=screenNightVisionEffect)] (var screen_night_vision__enabled : bool&)
    screen_night_vision__enabled = false

[console_cmd(name="screen_night_vision.switch")]
def console_switch_screen_night_vision()
  ecs::query() <| $ [es(REQUIRE=screenNightVisionEffect)] (var screen_night_vision__enabled : bool&)
    screen_night_vision__enabled = !screen_night_vision__enabled

[console_cmd(name="screen_night_vision.preset")]
def console_switch_screen_night_vision(preset : string)
  ecs::query() <| $ [es(REQUIRE=screenNightVisionEffect)] (var screen_night_vision__preset : das_string&)
    screen_night_vision__preset := preset
