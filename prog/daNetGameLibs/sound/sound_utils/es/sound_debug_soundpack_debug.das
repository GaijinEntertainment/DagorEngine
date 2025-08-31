options no_aot
require ecs
require ecs.common
require soundSystem
require soundEvent
require DagorSystem
require DagorConsole
require math.random
require app // get_sync_time


[console_cmd(name="snd.debug_soundpack")]
def sound_debug_soundpack_console_cmd(enable : bool)
  if enable
    createEntity("sound_debug_soundpack")
  else
    query() <| $ [es(REQUIRE=sound_debug_soundpack__inited)] (eid : EntityId)
      destroyEntity(eid)


[es(tag=sound, on_event=ParallelUpdateFrameDelayed)]
def sound_debug_soundpack_update(evt : Event;
                                 eid : EntityId;
                                 sound_debug_soundpack__banks : StringList;
                                 var sound_debug_soundpack__events : StringList&;
                                 var sound_debug_soundpack__eventBanks : StringList&;

                                 var sound_debug_soundpack__event1 : SoundEvent&;
                                 var sound_debug_soundpack__event2 : SoundEvent&;
                                 var sound_debug_soundpack__event3 : SoundEvent&;
                                 var sound_debug_soundpack__event4 : SoundEvent&;
                                 var sound_debug_soundpack__inited : bool&;

                                 sound_debug_soundpack__playDelay : float;
                                 var sound_debug_soundpack__nextEventAt : float&)

  if !sound_debug_soundpack__inited
    sound_debug_soundpack__inited = true
    sound_debug_soundpack__events |> clear()
    sound_debug_soundpack__eventBanks |> clear()
    for bank in sound_debug_soundpack__banks
      sound_debug_enum_events_in_bank(string(bank)) <| $(event : string)
        sound_debug_soundpack__events |> push(event)
        sound_debug_soundpack__eventBanks |> push(string(bank))

  if get_sync_time() >= sound_debug_soundpack__nextEventAt
    sound_debug_soundpack__nextEventAt = get_sync_time() + sound_debug_soundpack__playDelay
    let idx = int(length(sound_debug_soundpack__events)) - 1
    if idx >= 0
      let name : string = string(sound_debug_soundpack__events[idx])
      let bank : string = string(sound_debug_soundpack__eventBanks[idx])
      sound_debug("sound_debug: playing    #{idx}    '{bank}'      '{name}'")
      debug("[sound_debug_soundpack]: playing    #{idx}    '{bank}'      '{name}'")

      release(sound_debug_soundpack__event4)
      sound_debug_soundpack__event4 |> move(sound_debug_soundpack__event3)
      sound_debug_soundpack__event3 |> move(sound_debug_soundpack__event2)
      sound_debug_soundpack__event2 |> move(sound_debug_soundpack__event1)
      sound_debug_soundpack__event1 |> play(name, "", get_listener_pos() + float3(2.f - rnd_float(-1., 1.), rnd_float(-1., 1.), rnd_float(-1., 1.)))
      sound_debug_soundpack__events |> erase(idx)
      sound_debug_soundpack__eventBanks |> erase(idx)

      if is_oneshot(sound_debug_soundpack__event1)
        abandon(sound_debug_soundpack__event1)
    else
      destroyEntity(eid)
