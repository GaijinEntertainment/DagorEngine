require ecs
require DagorMath
require DagorRandom
require das.effects.effect_controller_common

[es(tag=server, on_appear, REQUIRE=effect_controller__randomizeStartState)]
def effect_controller_randomize_state(evt : Event; var effect_controller__state : int&; effect_controller__states : Array)
  effect_controller__state = rnd_int(0, length(effect_controller__states) - 1)

[es(tag=server, no_order)]
def effect_controller(info : UpdateStageInfoAct;
                      var effect_controller__state : int&;
                      var effect_controller__stateTimes : float2&;
                      effect_controller__states : Array)
  if info.curTime >= effect_controller__stateTimes.y
    effect_controller__state = (effect_controller__state + 1) % length(effect_controller__states)
    let stateParams = get_effect_state_params(effect_controller__states, effect_controller__state)
    effect_controller__stateTimes.x = effect_controller__stateTimes.y // prev end is new start
    effect_controller__stateTimes.y = effect_controller__stateTimes.x + rnd_float(stateParams.time.x, stateParams.time.y)

[es(tag=render, no_order)]
def effect_spawnrate_controller(info : UpdateStageInfoAct;
                                effect_controller__state : int;
                                effect_controller__stateTimes : float2;
                                effect_controller__states : Array;
                                var effect__spawnRate : float&)
  let stateParams = get_effect_state_params(effect_controller__states, effect_controller__state)
  effect__spawnRate = cvt(info.curTime, effect_controller__stateTimes.x, effect_controller__stateTimes.y,
                                           stateParams.spawnRate.x,            stateParams.spawnRate.y)
