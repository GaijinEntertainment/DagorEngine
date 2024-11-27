require ecs
require app
require sound_utils_net.modules.sound_utils_net_events


[es(tag=server, on_appear)]
def sound_destroy_timer__appear(evt : Event;
                                sound_destroy_timer__duration : float;
                                var sound_destroy_timer__endTime : float&)
  sound_destroy_timer__endTime = get_sync_time() + sound_destroy_timer__duration


[es(tag=server, no_order)]
def sound_destroy_timer__update(info : ParallelUpdateFrameDelayed;
                                eid : EntityId;
                                sound_destroy_timer__endTime : float)
  if info.curTime >= sound_destroy_timer__endTime
    destroyEntity(eid)
