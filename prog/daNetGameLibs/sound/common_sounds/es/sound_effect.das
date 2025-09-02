require ecs
require soundEvent
require soundSystem
require sound_utils.modules.sound_control_common
require danetlibs.renderer.includes.pufd_events
require strings
require DagorSystem


[es(tag=sound, on_appear, track=sound_control__state, REQUIRE_NOT=watchableSoundEffect)]
def sound_effect_track(evt : Event;
                       sound_control__state : int;
                       sound_effect__path : string;
                       sound_effect__localOffset : float3;
                       var sound_effect__event : SoundEvent&;
                       transform : float3x4)
  if is_hearable_sound_state(sound_control__state)
    abandon(sound_effect__event)
  else
    release(sound_effect__event)
  if have_sound(sound_control__state)
    sound_effect__event |> play(sound_effect__path, transform * sound_effect__localOffset)


[es(tag=sound, on_appear, track=sound_control__state, REQUIRE=watchableSoundEffect)]
def sound_effect_track_state(evt : Event;
                             sound_control__state : int;
                             sound_effect__path : string;
                             sound_effect__watchedPath : string;
                             sound_effect__localOffset : float3;
                             var sound_effect__event : SoundEvent&;
                             transform : float3x4)
  if is_hearable_sound_state(sound_control__state)
    abandon(sound_effect__event)
  else
    release(sound_effect__event)
  if have_sound(sound_control__state)
    let path = is_watched_sound_state(sound_control__state) ? sound_effect__watchedPath : sound_effect__path
    if !empty(path)
      sound_effect__event |> play(path, transform * sound_effect__localOffset)


[es(tag=sound, on_event=ParallelUpdateFrameDelayed, after=sound_begin_update_es, before=sound_end_update_es, REQUIRE=movableSoundEffect)]
def sound_effect_update_moveable(evt : Event;
                                 sound_effect__event : SoundEvent;
                                 sound_effect__localOffset : float3;
                                 transform : float3x4)
  if is_valid_handle_value(sound_effect__event)
    set_pos(sound_effect__event, transform * sound_effect__localOffset)


[es(tag=sound, on_appear)]
def replicated_sound_effect_on_appear(evt : Event;
                                      var sound_effect__path : das_string&;
                                      replicated_sound_effect__path : string)
  sound_effect__path := replicated_sound_effect__path


[es(tag=sound, on_appear, track=sound_preset__isLoaded, after=replicated_sound_effect_on_appear)]
def sound_effect_appear(evt : Event;
                        sound_preset__isLoaded : bool;
                        sound_effect__autoRange : bool;
                        var sound_control__range : float&;
                        sound_effect__path : string;
                        sound_effect__abandonOnReset : bool;
                        var sound_effect__event : SoundEvent&)
  if sound_preset__isLoaded
    if sound_effect__autoRange
      sound_control__range = get_max_distance(sound_effect__path)
      if sound_control__range <= 0.
        sound_control__range = 35.
        sound_debug("unknown range for {sound_effect__path}")
      elif sound_control__range > 8000.
        sound_debug("max distance {sound_control__range} for event {sound_effect__path} should be less than 8000")
    sound_effect__event.abandonOnReset = sound_effect__abandonOnReset
  else
    release_immediate(sound_effect__event)
    if sound_effect__autoRange
      sound_control__range = 0.
