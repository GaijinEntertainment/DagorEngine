require ecs
require soundEvent
require soundSystem
require DagorSystem
require strings
require outer_space.events_outer_space
require soundHash


[es(tag=sound, track=(sound_banks_state__isPresetLoaded, music))]
def music_toggle(evt : Event; music : string; sound_banks_state__isPresetLoaded : bool)
  print("music_toggle")
  if sound_banks_state__isPresetLoaded && !empty(music)
    print("send EventSetMusic")
    broadcastEvent(EventSetMusic(music = music))


[es(tag=sound)]
def cur_music_toggle(evt : EventSetMusic;
                     var music__event : SoundEvent&)
  print("get EventSetMusic")
  abandon(music__event)
  if !empty(evt.music)
    print("music: {evt.music}")
    music__event |> play("", evt.music, get_listener_pos())

[es(tag=sound)]
def sound_banks_state_on_loaded(evt : EventOnSoundPresetLoaded;
                                sound_banks_state__presetName : string;
                                var sound_banks_state__isPresetLoaded : bool&)
  print("EventOnSoundPresetLoaded")
  if evt.hash == sound_hash(sound_banks_state__presetName)
    sound_banks_state__isPresetLoaded = evt.isLoaded


[es(tag=sound, on_appear)]
def sound_banks_state_on_appear(evt : Event;
                                var sound_banks_state__isPresetLoaded : bool&;
                                sound_banks_state__presetName : string)
  sound_banks_state__isPresetLoaded = sound_banks_is_preset_loaded(sound_banks_state__presetName)
