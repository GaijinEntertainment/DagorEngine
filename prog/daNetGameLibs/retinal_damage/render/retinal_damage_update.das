require ecs
require DagorShaders

let
  retinal_damage_remaining_time_fractionVarId = get_shader_variable_id("retinal_damage_remaining_time_fraction", true)
  retinal_damage_opacityVarId                 = get_shader_variable_id("retinal_damage_opacity", true)

def smoothstep(edge0 : float; edge1 : float; x : float)
  let step = clamp((x - edge0) / (edge1 - edge0), 0., 1.)
  return step * step * (3. - 2. * step)

def get_retinal_damage_opacity(remaining_time : float; lifetime : float; fade_in_time : float;
                               fade_out_time : float; initial_opacity : float)
  if remaining_time > 0.
    let elapsed_time = lifetime - remaining_time
    if elapsed_time < fade_in_time
      let t = (elapsed_time - fade_in_time) / fade_in_time
      return (1. - initial_opacity) * sqrt(1. - t * t) + initial_opacity
    elif elapsed_time < lifetime - fade_out_time
      return 1.
    else
      let t = smoothstep(lifetime - fade_out_time, lifetime, elapsed_time)
      return 1. - t * t
  return 0.

[es(tag=render, no_order)]
def update_retinal_damage_effect(info : UpdateStageInfoAct;
                                 var retinal_damage__remaining_time : float&; retinal_damage__total_lifetime : float;
                                 retinal_damage__fade_in_time : float; retinal_damage__fade_out_time : float;
                                 retinal_damage__initial_opacity : float)
  if retinal_damage__remaining_time <= 0.0
    query() <| $ [es(REQUIRE=retinalDamageScreenEffect)] (eid : EntityId)
      destroyEntity(eid)
    return
  retinal_damage__remaining_time -= info.dt
  let opacity = get_retinal_damage_opacity(retinal_damage__remaining_time, retinal_damage__total_lifetime,
                                           retinal_damage__fade_in_time, retinal_damage__fade_out_time,
                                           retinal_damage__initial_opacity)
  DagorShaders::set_real(retinal_damage_remaining_time_fractionVarId,
                         retinal_damage__remaining_time / retinal_damage__total_lifetime)
  DagorShaders::set_real(retinal_damage_opacityVarId, opacity)
