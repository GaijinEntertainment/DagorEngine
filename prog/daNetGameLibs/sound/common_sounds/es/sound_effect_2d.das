require ecs
require soundEvent
require soundSystem


[es(tag=sound, on_appear)]
def sound_effect_2d_appear(evt : Event;
                           sound_banks_state__isPresetLoaded : bool;
                           sound_effect_2d__path : string;
                           sound_effect_2d__abandonOnReset : bool;
                           var sound_effect_2d__event : SoundEvent&)
  sound_effect_2d__event.abandonOnReset = sound_effect_2d__abandonOnReset
  if sound_banks_state__isPresetLoaded
    sound_effect_2d__event |> play(sound_effect_2d__path)


[es(tag=sound, track=sound_banks_state__isPresetLoaded)]
def sound_effect_2d_toggle(evt : Event;
                           sound_banks_state__isPresetLoaded : bool;
                           sound_effect_2d__path : string;
                           var sound_effect_2d__event : SoundEvent&)
  release(sound_effect_2d__event)
  if sound_banks_state__isPresetLoaded
    sound_effect_2d__event |> play(sound_effect_2d__path)
