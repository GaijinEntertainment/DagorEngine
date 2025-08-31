require ecs
require soundEvent
require soundSystem
require DagorSystem
require strings


[es(tag=sound, on_appear, track=menu_music__theme, on_event=EventOnMasterSoundPresetLoaded)]
def menu_music_appear(evt : Event;
                      menu_music__theme : string;
                      menu_music__path : string;
                      menu_music__var : string;
                      menu_music__themes : Object;
                      var menu_music__event : SoundEvent&)
  if !empty(menu_music__theme) && sound_banks_is_master_preset_loaded()
    let idx = get_int(menu_music__themes, menu_music__theme) ?? -1
    if idx >= 0
      menu_music__event |> play(menu_music__path)
      set_var(menu_music__event, menu_music__var, float(idx))
      return
    logerr("menu_music theme '{menu_music__theme}' not found in menu_music.themes")
  abandon(menu_music__event)
