require ecs
require app
require soundEvent
require soundSystem
require DagorSystem
require strings


[es(tag=sound, on_appear, track=sound_preset__isLoaded)]
def sound_effect_2d_appear(evt : Event;
                           eid : EntityId;
                           sound_preset__isLoaded : bool;
                           sound_effect_2d__path : string;
                           sound_effect_2d__abandonOnReset : bool;
                           var sound_effect_2d__event : SoundEvent&)
  if empty(sound_effect_2d__path)
    logerr("empty sound_effect_2d__path in entity {eid} '{getEntityTemplateName(eid)}'")
    release_immediate(sound_effect_2d__event)
    return
  if sound_preset__isLoaded
    sound_effect_2d__event.abandonOnReset = sound_effect_2d__abandonOnReset
    sound_effect_2d__event |> play(sound_effect_2d__path)
  else
    sound_effect_2d__event |> release()
