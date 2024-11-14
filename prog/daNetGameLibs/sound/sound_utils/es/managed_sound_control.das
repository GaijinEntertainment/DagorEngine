require app
require ecs
require soundSystem
require sound_utils.modules.managed_sound_control_common


[es(tag=sound, on_event=ParallelUpdateFrameDelayed, REQUIRE=vehicleManageSoundControl, after=(animchar_before_render_es, sound_begin_update_es, sound_control_update), before=sound_end_update_es)]
def vehicle_managed_sound_control(evt : Event;
                                  managed_sound_control__maxInstances : int;
                                  managed_sound_control__cooldown : float;
                                  var managed_sound_control__entities : EidList&;
                                  var managed_sound_control__positions : Point3List&)

  let listener = get_listener_pos()
  let curTime = get_sync_time()

  query() <| $ [es(REQUIRE=vehicleManageSoundControl)] (eid : EntityId;
                                                        sound_control__inRange : bool;
                                                        var sound_control__allowSound : bool&;
                                                        managed_sound__nextTimeToAllow : float;
                                                        transform : float3x4)
    managed_sound_control_impl(sound_control__inRange,
                               managed_sound_control__maxInstances,
                               managed_sound_control__cooldown,
                               managed_sound_control__entities,
                               managed_sound_control__positions,
                               listener,
                               curTime,
                               eid,
                               sound_control__allowSound,
                               managed_sound__nextTimeToAllow,
                               transform)



[es(tag=sound, on_event=ParallelUpdateFrameDelayed, REQUIRE=sfxManageSoundControl, after=(animchar_before_render_es, sound_begin_update_es, sound_control_update), before=sound_end_update_es)]
def sfx_managed_sound_control(evt : Event;
                              managed_sound_control__maxInstances : int;
                              managed_sound_control__cooldown : float;
                              var managed_sound_control__entities : EidList&;
                              var managed_sound_control__positions : Point3List&)

  let listener = get_listener_pos()
  let curTime = get_sync_time()

  query() <| $ [es(REQUIRE=sfxManageSoundControl)] (eid : EntityId;
                                                    sound_control__inRange : bool;
                                                    var sound_control__allowSound : bool&;
                                                    managed_sound__nextTimeToAllow : float;
                                                    transform : float3x4)
    managed_sound_control_impl(sound_control__inRange,
                               managed_sound_control__maxInstances,
                               managed_sound_control__cooldown,
                               managed_sound_control__entities,
                               managed_sound_control__positions,
                               listener,
                               curTime,
                               eid,
                               sound_control__allowSound,
                               managed_sound__nextTimeToAllow,
                               transform)


[es(tag=sound, on_disappear, REQUIRE=managedSound)]
def managed_sound_disappear(evt : Event; eid : EntityId)
  remove_managed_sound(eid)
