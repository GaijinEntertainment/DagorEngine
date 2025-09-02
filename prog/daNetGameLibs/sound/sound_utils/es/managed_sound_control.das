require ecs
require sound_utils.modules.managed_sound_control_common


[es(tag=sound, on_appear)]
def managed_sound_on_appear(evt : Event;
                            managed_sound__maxInstances : int;
                            managed_sound__type : string;
                            var managed_sound__typeIdx : int&)

  managed_sound__typeIdx = -1
  query() <| $ [es] (var managed_sound_control__types : StringList&;
                     var managed_sound_control__beginPosEnd : IPoint3List&;
                     var managed_sound_control__entities : EidList&;
                     var managed_sound_control__positions : Point3List&)

    append_managed_sound_type(managed_sound__maxInstances,
                              managed_sound__type,
                              managed_sound__typeIdx,

                              managed_sound_control__types,
                              managed_sound_control__beginPosEnd,
                              managed_sound_control__entities,
                              managed_sound_control__positions)



[es(tag=sound, on_appear)]
def managed_sound_control_on_appear(evt : Event;
                                    var managed_sound_control__types : StringList&;
                                    var managed_sound_control__beginPosEnd : IPoint3List&;
                                    var managed_sound_control__entities : EidList&;
                                    var managed_sound_control__positions : Point3List&)

  clear(managed_sound_control__types)
  clear(managed_sound_control__beginPosEnd)
  clear(managed_sound_control__entities)
  clear(managed_sound_control__positions)

  query() <| $ [es] (managed_sound__maxInstances : int;
                     managed_sound__type : string;
                     var managed_sound__typeIdx : int&)

    append_managed_sound_type(managed_sound__maxInstances,
                              managed_sound__type,
                              managed_sound__typeIdx,

                              managed_sound_control__types,
                              managed_sound_control__beginPosEnd,
                              managed_sound_control__entities,
                              managed_sound_control__positions)



[es(tag=sound, on_event=ParallelUpdateFrameDelayed, after=(animchar_before_render_es, sound_begin_update_es, sound_control_update), before=sound_end_update_es)]
def managed_sound_control_update(evt : Event;
                                 managed_sound_control__cooldown : float;
                                 var managed_sound_control__beginPosEnd : IPoint3List&;
                                 var managed_sound_control__entities : EidList&;
                                 var managed_sound_control__positions : Point3List&)

  update_managed_sound_control(managed_sound_control__cooldown,
                               managed_sound_control__beginPosEnd,
                               managed_sound_control__entities,
                               managed_sound_control__positions)


[es(tag=sound, track=(isAlive, managed_sound__excluded))]
def managed_sound_track_is_alive(evt : Event;
                                 eid : EntityId;
                                 isAlive = true;
                                 managed_sound__excluded : bool;
                                 managed_sound__typeIdx : int;
                                 var sound_control__allowSound : bool&)
  if !isAlive || managed_sound__excluded
    remove_managed_sound_instance(eid, managed_sound__typeIdx)
    sound_control__allowSound = false


[es(tag=sound, on_disappear)]
def managed_sound_disappear(evt : Event;
                            eid : EntityId;
                            managed_sound__typeIdx : int)
  remove_managed_sound_instance(eid, managed_sound__typeIdx)
