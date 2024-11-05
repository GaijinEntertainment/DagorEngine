require ecs
require DagorShaders

let
  flash_blind_remaining_timeVarId     = get_shader_variable_id("flash_blind_remaining_time", true)
  flash_blind_delta_timeVarId         = get_shader_variable_id("flash_blind_delta_time", true)
  flash_blind_decay_factorVarId       = get_shader_variable_id("flash_blind_decay_factor", true)
  flash_blind_decay_colorVarId        = get_shader_variable_id("flash_blind_decay_color", true)
  flash_blind_desaturation_scaleVarId = get_shader_variable_id("flash_blind_desaturation_scale", true)

[es(tag=render, on_appear)]
def on_appear_flash_blind_renderer(evt : Event;
                                   flash_blind__decay_color : float4;
                                   flash_blind__decay_factor, flash_blind__desaturation_scale : float)
  DagorShaders::set_color4(flash_blind_decay_colorVarId, flash_blind__decay_color)
  DagorShaders::set_real(flash_blind_decay_factorVarId, flash_blind__decay_factor)
  DagorShaders::set_real(flash_blind_desaturation_scaleVarId, flash_blind__desaturation_scale)


[es(tag=render, no_order)]
def update_flash_blind_effect(info : UpdateStageInfoAct; var flash_blind__remaining_time : float&)
  if flash_blind__remaining_time <= 0.0
    query() <| $ [es(REQUIRE=flashBlindScreenEffect)] (eid : EntityId)
      destroyEntity(eid)
    return
  flash_blind__remaining_time -= info.dt
  DagorShaders::set_real(flash_blind_delta_timeVarId, info.dt)
  DagorShaders::set_real(flash_blind_remaining_timeVarId, flash_blind__remaining_time)
