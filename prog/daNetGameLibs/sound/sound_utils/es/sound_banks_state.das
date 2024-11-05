require ecs
require soundHash
require soundSystem
require sound_utils.modules.sound_utils_events


[es(tag=sound)]
def sound_banks_state_on_loaded(evt : EventOnSoundPresetLoaded;
                                sound_banks_state__presetName : string;
                                var sound_banks_state__isPresetLoaded : bool&)
  if evt.hash == sound_hash(sound_banks_state__presetName)
    sound_banks_state__isPresetLoaded = evt.isLoaded


[es(tag=sound, on_appear)]
def sound_banks_state_on_appear(evt : Event;
                                var sound_banks_state__isPresetLoaded : bool&;
                                sound_banks_state__presetName : string)
  sound_banks_state__isPresetLoaded = sound_banks_is_preset_loaded(sound_banks_state__presetName)
