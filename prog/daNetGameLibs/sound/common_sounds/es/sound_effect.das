require ecs
require app
require strings
require soundEvent
require soundSystem
require DagorSystem
require gameLibs.pufd_events.pufd_events
require sound_utils.modules.sound_control_common


[es(tag=sound, on_appear, track=sound_preset__isLoaded)]
def sound_effect_appear(evt : Event;
                        eid : EntityId;
                        sound_preset__isLoaded : bool;
                        sound_effect__autoRange : bool;
                        var sound_control__range : float&;
                        sound_effect__path : string;
                        replicated_sound_effect__path = "";
                        sound_effect__abandonOnReset : bool;
                        var sound_effect__event : SoundEvent&)
  let path = empty(replicated_sound_effect__path) ? sound_effect__path : replicated_sound_effect__path
  if empty(path)
    logerr("empty sound_effect path in entity {eid} '{getEntityTemplateName(eid)}'")
    return
  if sound_preset__isLoaded
    if sound_effect__autoRange
      sound_control__range = get_max_distance(path)
      if sound_control__range <= 0.
        sound_control__range = 35.
        sound_debug("unknown range for sound_effect '{path}'")
      elif sound_control__range > 8000.
        sound_debug("max distance {sound_control__range} for event '{path}' should be less than 8000")
    sound_effect__event.abandonOnReset = sound_effect__abandonOnReset
  else
    release_immediate(sound_effect__event)
    if sound_effect__autoRange
      sound_control__range = 0.


[es(tag=sound, on_appear, track=sound_control__state, after=sound_effect_appear)]
def sound_effect_toggle(evt : Event;
                        eid : EntityId;
                        sound_control__state : int;
                        sound_effect__path : string;
                        replicated_sound_effect__path = "";
                        sound_effect__localOffset : float3;
                        sound_effect__occlusionRadius : float;
                        var sound_effect__event : SoundEvent&;
                        transform : float3x4)
  let path = empty(replicated_sound_effect__path) ? sound_effect__path : replicated_sound_effect__path
  if empty(path)
    logerr("empty sound_effect path in entity {eid} '{getEntityTemplateName(eid)}'")
    release_immediate(sound_effect__event)
    return
  if is_hearable_sound_state(sound_control__state)
    abandon(sound_effect__event)
  else
    release(sound_effect__event)
  if have_sound(sound_control__state)
    sound_effect__event |> play(path, transform * sound_effect__localOffset)
    if sound_effect__occlusionRadius > 0.
      set_occlusion_radius(sound_effect__event, sound_effect__occlusionRadius)


[es(tag=sound, on_event=ParallelUpdateFrameDelayed, after=sound_begin_update_es, before=sound_end_update_es, REQUIRE=dynamicSoundEffect)]
def sound_effect_update_pos(evt : Event;
                            sound_effect__event : SoundEvent;
                            sound_effect__localOffset : float3;
                            transform : float3x4)
  if is_valid_handle_value(sound_effect__event)
    set_pos(sound_effect__event, transform * sound_effect__localOffset)
