require ecs
require soundSystem
require soundEvent
require soundHash
require sound_utils.modules.sound_utils_events


[es(tag=sound)]
def sound_preset_on_loaded(evt : EventOnSoundPresetLoaded;
                           sound_preset__name : string;
                           var sound_preset__isLoaded : bool&)
  if evt.hash == sound_hash(sound_preset__name)
    if sound_preset__isLoaded != evt.isLoaded
      sound_preset__isLoaded = evt.isLoaded


[es(tag=sound, on_appear)]
def sound_preset_on_appear(evt : Event;
                           var sound_preset__isLoaded : bool&;
                           sound_preset__name : string)
  let isLoaded = sound_banks_is_preset_loaded(sound_preset__name)
  if sound_preset__isLoaded != isLoaded
    sound_preset__isLoaded = isLoaded


[es(tag=sound, track=sound_preset__isLoaded)]
def sound_event_group_on_unloaded(evt : Event;
                                  sound_preset__isLoaded : bool;
                                  var sound_event_group  : SoundEventGroup&)
  if !sound_preset__isLoaded
    release_all_sounds(sound_event_group)
