require app
require ecs
require soundEvent

def csu_engine_sound_update(var event : SoundEvent&; thrust : float; min_thres : float; local_pos : float3; tm : float3x4; volume : float)
  if !event.enabled && thrust > min_thres
    event |> reset(play("sfx/jetpack", tm * local_pos))
    set_volume(event, volume)
    event.enabled = true
  elif event.enabled && thrust <= min_thres
    abandon(event)
    event.enabled = false

  if event.enabled
    set_var(event, "thrust", thrust)
    set_pos(event, tm * local_pos)

[es(tag=sound, on_event=CmdPostPhysUpdate, after=spaceship_thrust_es_event_handler)]
def csu_engines_sound_update(evt : Event;
                             csu_engine_mainA__thrust : float;
                             csu_engine_mainD__thrust : float;
                             csu_engine_agileU__thrust : float3;
                             csu_engine_agileD__thrust : float3;
                             csu_engine_agileL__thrust : float3;
                             csu_engine_agileR__thrust : float3;
                             var csu_engine_mainA_sound__event : SoundEvent&;
                             var csu_engine_mainD_sound__event : SoundEvent&;
                             var csu_engine_agileU_sound__event : SoundEvent&;
                             var csu_engine_agileD_sound__event : SoundEvent&;
                             var csu_engine_agileL_sound__event : SoundEvent&;
                             var csu_engine_agileR_sound__event : SoundEvent&;
                             spaceship_nozzle_pos__mainA : float3;
                             spaceship_nozzle_pos__mainD : float3;
                             spaceship_nozzle_pos__agileU : float3;
                             spaceship_nozzle_pos__agileD : float3;
                             spaceship_nozzle_pos__agileL : float3;
                             spaceship_nozzle_pos__agileR : float3;
                             transform : float3x4)
  csu_engine_sound_update(csu_engine_mainA_sound__event, abs(csu_engine_mainA__thrust), 10.f, spaceship_nozzle_pos__mainA, transform, 1.f)
  csu_engine_sound_update(csu_engine_mainD_sound__event, abs(csu_engine_mainD__thrust), 10.f, spaceship_nozzle_pos__mainD, transform, 1.f)
  csu_engine_sound_update(csu_engine_agileU_sound__event, length_sq(csu_engine_agileU__thrust), 1.f, spaceship_nozzle_pos__agileU, transform, 0.2f)
  csu_engine_sound_update(csu_engine_agileD_sound__event, length_sq(csu_engine_agileD__thrust), 1.f, spaceship_nozzle_pos__agileD, transform, 0.2f)
  csu_engine_sound_update(csu_engine_agileL_sound__event, length_sq(csu_engine_agileL__thrust), 1.f, spaceship_nozzle_pos__agileL, transform, 0.2f)
  csu_engine_sound_update(csu_engine_agileR_sound__event, length_sq(csu_engine_agileR__thrust), 1.f, spaceship_nozzle_pos__agileR, transform, 0.2f)
