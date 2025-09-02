require ecs
require app
require soundEvent
require soundSystem
require danetlibs.renderer.includes.pufd_events
require DagorMath


[event(broadcast)]
struct EventOnWeaponSound
  pos : float3
  isLoop : bool


[es(tag=sound, on_appear, on_event=EventOnMasterSoundPresetLoaded)]
def birds_sound_appear(evt : Event;
                       birds_global_sound_var__name : string;
                       var birds_global_sound_var__id : SoundVarId&;
                       var birds_global_sound_var__value : float&)
  if sound_banks_is_master_preset_loaded()
    birds_global_sound_var__id = get_var_id_global(birds_global_sound_var__name)
    birds_global_sound_var__value = -1.
    if !birds_global_sound_var__id
      sound_debug("birds \"{birds_global_sound_var__name}\" global var is missing: should remove template from scene")
  else
    birds_global_sound_var__id = invalid_sound_var_id()
    birds_global_sound_var__value = -1.


[es(tag=sound)]
def birds_sound_shut_up(evt : EventOnWeaponSound;
                        var birds_global_sound_var__noiseTime : float&;
                        birds_global_sound_var__actionRange = 250.)
  if length_sq(get_listener_pos() - evt.pos) < birds_global_sound_var__actionRange * birds_global_sound_var__actionRange
    birds_global_sound_var__noiseTime = get_sync_time()


[es(tag=sound, after=sound_begin_update_es, before=sound_end_update_es)]
def birds_sound_update(info : ParallelUpdateFrameDelayed;
                       birds_global_sound_var__noiseTime : float;
                       birds_global_sound_var__id : SoundVarId;
                       birds_global_sound_var__timeRange : float2;
                       var birds_global_sound_var__value : float&)
  if !!birds_global_sound_var__id
    let action = cvt(info.curTime - birds_global_sound_var__noiseTime, birds_global_sound_var__timeRange.x, birds_global_sound_var__timeRange.y, 1., 0.)
    if birds_global_sound_var__value != action
      birds_global_sound_var__value = action
      set_var_global(birds_global_sound_var__id, action)
