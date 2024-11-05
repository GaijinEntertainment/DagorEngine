options no_aot
require ecs
require ecs.common
require DagorConsole

def disable_screen_frost()
  find_query() <| $ [es(REQUIRE=screenFrostEffect)] (eid : EntityId)
    destroyEntity(eid)
    return true

def enable_screen_frost(maxIntensity : float; rate : float)
  disable_screen_frost()
  createEntity("screen_frost_effect") <| $(var init : ComponentsInitializer)
    set(init, "screen_frost__maxIntensity", maxIntensity)
    set(init, "screen_frost__rate", rate)
    set(init, "screen_frost__intensity", 0.)

[console_cmd(name="screen_frost.enable")]
def console_enable_screen_frost(maxIntensity : float; rate : float)
  enable_screen_frost(maxIntensity, rate)

[console_cmd(name="screen_frost.disable")]
def console_disable_screen_frost()
  disable_screen_frost()
