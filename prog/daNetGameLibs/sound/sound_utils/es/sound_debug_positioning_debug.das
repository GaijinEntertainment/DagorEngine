options no_aot
require ecs
require soundSystem
require soundEvent
require DagorConsole
require app


[console_cmd(name="snd.debug_positioning")]
def sound_debug_positioning_console_cmd(enable : bool)
  query() <| $ [es(REQUIRE=sound_debug_positioning__event)] (eid : EntityId)
    destroyEntity(eid)
  if enable
    createEntity("sound_debug_positioning")


[es(tag=sound, on_event=ParallelUpdateFrameDelayed)]
def sound_debug_positioning_update(evt : Event;
                                   sound_debug_positioning__path : string;
                                   sound_debug_positioning__speed : float;
                                   sound_debug_positioning__radius : float;
                                   var sound_debug_positioning__event : SoundEvent&)
  let angle = get_sync_time() * sound_debug_positioning__speed
  let pos = get_listener_pos() + float3(cos(angle), 0., sin(angle)) * sound_debug_positioning__radius
  if !sound_debug_positioning__event.enabled
    sound_debug_positioning__event.enabled = true
    sound_debug_positioning__event |> play(sound_debug_positioning__path, pos)
  else
    set_pos(sound_debug_positioning__event, pos)
