require ecs
require soundEvent
require soundSystem
require Dacoll


[es(tag=sound, on_event=ParallelUpdateFrameDelayed, after=sound_begin_update_es, before=sound_end_update_es)]
def underwater_sound(evt : Event;
                     underwater_sound__path : string;
                     var underwater_sound__event : SoundEvent&;
                     var underwater_sound__frame : int&;
                     underwater_sound__aboveWaterThreshold = 0.1)
  if !sound_banks_is_master_preset_loaded()
    return
  ++underwater_sound__frame
  if (underwater_sound__frame & 3) == 0
    var waterLevel = 0.
    let pos = get_listener_pos()
    if traceht_water(pos, waterLevel)
      if underwater_sound__event.enabled && pos.y - waterLevel > underwater_sound__aboveWaterThreshold
        underwater_sound__event.enabled = false
        abandon(underwater_sound__event)
      elif !underwater_sound__event.enabled && pos.y < waterLevel
        underwater_sound__event.enabled = true
        abandon(underwater_sound__event)
        underwater_sound__event |> play(underwater_sound__path)
  if underwater_sound__event.enabled
    set_pos(underwater_sound__event, get_listener_pos())


[es(tag=sound, on_event=EventOnMasterSoundPresetLoaded)]
def underwater_sound_on_loaded(evt : Event;
                               var underwater_sound__event : SoundEvent&)
  release(underwater_sound__event)
  underwater_sound__event.enabled = false
