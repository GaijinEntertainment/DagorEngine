require ecs
require math.base
require soundSystem
require soundEvent
require danetlibs.renderer.includes.pufd_events
require sound_utils.modules.sound_player_common


[es(tag=sound, on_appear, on_event=EventOnMasterSoundPresetLoaded)]
def ambient_sound_appear(evt : Event;
                         ambient_sound__abandonOnReset : bool;
                         var ambient_sound__shouldPlay : bool&;
                         var ambient_sound__event : SoundEvent&)
  release_play_or(ambient_sound__event)
  ambient_sound__event.abandonOnReset = ambient_sound__abandonOnReset
  ambient_sound__shouldPlay = sound_banks_is_master_preset_loaded()


// made this way to do not play ambient while loading (ParallelUpdateFrameDelayed is not working at this moment on loading screen)
[es(tag=sound, on_event=ParallelUpdateFrameDelayed, after=sound_begin_update_es, before=sound_end_update_es)]
def ambient_sound_update(evt : Event;
                         ambient_sound__shouldPlay : bool;
                         ambient_sound__path : string;
                         var ambient_sound__event : SoundEvent&)
  ambient_sound__event |> play_or_abandon(ambient_sound__path, get_listener_pos(), ambient_sound__shouldPlay)
