require ecs
require soundEvent
require DagorMath
require gameLibs.pufd_events.pufd_events


[es(tag=sound, after=sound_begin_update_es, before=sound_end_update_es)]
def sound_effect_speed_var_update(info : ParallelUpdateFrameDelayed;
                                  var sound_effect_speed_var__pos : float3&;
                                  sound_effect_speed_var__speedToVar : float4;

                                  var sound_effect_speed_var__speedSq : float&;
                                  sound_effect_speed_var__thresholdSq : float;
                                  sound_effect_speed_var__name : string;
                                  sound_effect__event : SoundEvent;
                                  transform : float3x4)
  if !is_valid_handle_value(sound_effect__event)
    sound_effect_speed_var__speedSq = -1.
    sound_effect_speed_var__pos = transform[3]
    return
  let speedSq = length_sq(transform[3] - sound_effect_speed_var__pos)
  if sound_effect_speed_var__speedSq < 0. || abs(speedSq - sound_effect_speed_var__speedSq) >= sound_effect_speed_var__thresholdSq
    sound_effect_speed_var__speedSq = speedSq
    let value = cvt(safediv(sqrt(speedSq), info.dt), sound_effect_speed_var__speedToVar.x, sound_effect_speed_var__speedToVar.y, sound_effect_speed_var__speedToVar.z, sound_effect_speed_var__speedToVar.w)
    set_var(sound_effect__event, sound_effect_speed_var__name, value)
  sound_effect_speed_var__pos = transform[3]


[es(tag=sound, on_appear, track=sound_control__state, REQUIRE=sound_control__state)]
def sound_effect_speed_var_track(evt : Event;
                                 var sound_effect_speed_var__speedSq : float&;
                                 var sound_effect_speed_var__pos : float3&;
                                 transform : float3x4)
  sound_effect_speed_var__speedSq = -1.
  sound_effect_speed_var__pos = transform[3]
