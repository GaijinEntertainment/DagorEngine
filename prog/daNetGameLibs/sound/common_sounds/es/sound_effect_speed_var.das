require ecs
require soundEvent
require DagorMath
require danetlibs.renderer.includes.pufd_events


[es(tag=sound, after=sound_begin_update_es, before=sound_end_update_es)]
def sound_effect_speed_var(info : ParallelUpdateFrameDelayed;
                           var sound_effect_speed_var__pos : float3&;
                           sound_effect_speed_var__speedToVar : float4;
                           sound_effect__event : SoundEvent;
                           transform : float3x4)
  let speed = safediv(length(transform[3] - sound_effect_speed_var__pos), info.dt)
  sound_effect_speed_var__pos = transform[3]
  set_var(sound_effect__event, "speed", cvt(speed, sound_effect_speed_var__speedToVar.x, sound_effect_speed_var__speedToVar.y, sound_effect_speed_var__speedToVar.z, sound_effect_speed_var__speedToVar.w))
