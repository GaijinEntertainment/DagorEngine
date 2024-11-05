require ecs
require soundEvent
require soundSystem
require DagorSystem
require strings


[es(tag=sound, track=(sound_banks_state__isPresetLoaded, menu_music__theme))]
def menu_music_toggle(evt : Event;
                      sound_banks_state__isPresetLoaded : bool;
                      menu_music__theme : string;
                      menu_music__path : string;
                      menu_music__var : string;
                      menu_music__themes : Object;
                      var menu_music__event : SoundEvent&)
  if sound_banks_state__isPresetLoaded && !empty(menu_music__theme)
    let idx = get_int(menu_music__themes, menu_music__theme) ?? -1
    if idx >= 0
      if !is_valid_handle_value(menu_music__event)
        menu_music__event |> play(menu_music__path)
      set_var(menu_music__event, menu_music__var, float(idx))
      return
    logerr("menu_music theme '{menu_music__theme}' not found in menu_music.themes")
  abandon(menu_music__event)
