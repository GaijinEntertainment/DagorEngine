require ecs
require soundEvent
require soundSystem


[es(tag=sound, on_appear, track=sound_preset__isLoaded)]
def sound_effect_2d_appear(evt : Event;
                           sound_preset__isLoaded : bool;
                           sound_effect_2d__path : string;
                           sound_effect_2d__abandonOnReset : bool;
                           var sound_effect_2d__event : SoundEvent&)
  if sound_preset__isLoaded
    sound_effect_2d__event.abandonOnReset = sound_effect_2d__abandonOnReset
    sound_effect_2d__event |> play(sound_effect_2d__path)
  else
    sound_effect_2d__event |> release()
