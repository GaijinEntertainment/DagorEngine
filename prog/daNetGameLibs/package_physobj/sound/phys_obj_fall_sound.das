require ecs
require app
require PhysObj
require DagorMath
require DngPhysObj
require soundEvent


[es(tag=sound, on_appear)]
def phys_obj_fall_sound_on_appear(evt : Event;
                                  phys_obj_fall_sound__cooldownOnAppear : float;
                                  var phys_obj_fall_sound__nextTime : float&)
  phys_obj_fall_sound__nextTime = get_sync_time() + phys_obj_fall_sound__cooldownOnAppear


[es(tag=sound, on_appear, REQUIRE=disableUpdate)]
def phys_obj_fall_sound_on_disable(evt : Event;
                                   var phys_obj_fall_sound__prevSpeed : float&)
  phys_obj_fall_sound__prevSpeed = 0.


[es(tag=sound, on_event=ParallelUpdateFrameDelayed, after=sound_begin_update_es, before=sound_end_update_es, REQUIRE_NOT=disableUpdate)]
def phys_obj_fall_sound_update(evt : Event;
                               transform : float3x4;
                               phys_obj_net_phys : PhysObjActor;
                               phys_obj_fall_sound__path : string;
                               phys_obj_fall_sound__cooldown : float;
                               phys_obj_fall_sound__minMaxSpeedDrop : float2;
                               var phys_obj_fall_sound__prevSpeed : float&;
                               var phys_obj_fall_sound__nextTime : float&)
  var curSpeed = length_sq(float3(phys_obj_net_phys.phys.currentState.velocity))
  let prevSpeed = phys_obj_fall_sound__prevSpeed
  if curSpeed == 0. && prevSpeed == 0.
    return
  curSpeed = sqrt(curSpeed)
  phys_obj_fall_sound__prevSpeed = curSpeed
  let drop = prevSpeed - curSpeed
  if drop < phys_obj_fall_sound__minMaxSpeedDrop.x || get_sync_time() < phys_obj_fall_sound__nextTime
    return
  phys_obj_fall_sound__nextTime = get_sync_time() + phys_obj_fall_sound__cooldown

  if phys_obj_fall_sound__minMaxSpeedDrop.y > phys_obj_fall_sound__minMaxSpeedDrop.x
    var handle = play("", phys_obj_fall_sound__path, transform[3])
    let force = cvt(drop, phys_obj_fall_sound__minMaxSpeedDrop.x, phys_obj_fall_sound__minMaxSpeedDrop.y, 0., 1.)
    set_var(handle, "force", force)
    abandon(handle)
  else
    oneshot(phys_obj_fall_sound__path, transform[3])
