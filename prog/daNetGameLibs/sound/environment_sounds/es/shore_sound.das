require ecs
require app
require math.base
require DagorMath
require soundEvent
require soundSystem


[es(tag=sound, on_event=ParallelUpdateFrameDelayed, after=(birds_sound_update, sound_begin_update_es), before=sound_end_update_es)]
def shore_sound_update(evt : Event;
                       sound_banks_state__isPresetLoaded : bool;
                       shore_sound__event : SoundEvent&;
                       shore_sound__offsets : Point2List;
                       shore_sound__shoreDistanceToVar : float4;
                       transform : float3x4)
  if sound_banks_state__isPresetLoaded && length(shore_sound__offsets) >= 2
    let listener = inverse(transform) * soundSystem::get_listener_pos()
    var idx = 0
    while idx < length(shore_sound__offsets) - 2 && listener.x >= shore_sound__offsets[idx + 1].x
      ++idx
    let a = shore_sound__offsets[idx]
    let b = shore_sound__offsets[idx + 1]
    let t = lerp(a.y, b.y, safediv(listener.x - a.x, b.x - a.x));
    let pos = transform * float3(listener.x, 0., max(t, listener.z))
    set_pos(shore_sound__event, pos)
    set_var(shore_sound__event, "shore", cvt(listener.z - t, shore_sound__shoreDistanceToVar.x, shore_sound__shoreDistanceToVar.y, shore_sound__shoreDistanceToVar.z, shore_sound__shoreDistanceToVar.w))


[es(tag=sound, track=sound_banks_state__isPresetLoaded)]
def shore_sound_toggle(evt : Event;
                       sound_banks_state__isPresetLoaded : bool;
                       shore_sound__path : string;
                       var shore_sound__event : SoundEvent&)
  release_immediate(shore_sound__event)
  if sound_banks_state__isPresetLoaded
    shore_sound__event |> play(shore_sound__path)
