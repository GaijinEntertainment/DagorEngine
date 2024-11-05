require ecs
require app


[es(tag=server, on_appear)]
def sound_control_disable_timer__appear(evt : Event;
                                        sound_control_disable_timer__duration : float;
                                        var sound_control_disable_timer__endTime : float&)
  sound_control_disable_timer__endTime = get_sync_time() + sound_control_disable_timer__duration
