require ecs
require DagorMath
require math.base
require soundEvent
require soundSystem
require danetlibs.renderer.includes.pufd_events
require RendInst


def get_sound_pos(ri_extra : RiExtraComponent;
                  transform : float3x4)
  if riex_isRiGenExtraValid(ri_extra.handle)
    let bbox = getRIGenBBox(RendInstDesc(ri_extra.handle))
    return transform * bbox.center

  return transform[3]

[es(tag=sound, on_appear, on_event=(EventOnMasterSoundPresetLoaded, EventRendinstsLoaded), after=(rendinst_axis_force_initial_rotation_es))]
def rotating_door_appear(evt : Event;
                         var rotating_door_sound__isInited : bool&;
                         var rotating_door_sound__movingEvent : SoundEvent&)
  rotating_door_sound__isInited = false
  rotating_door_sound__movingEvent.enabled = false
  release(rotating_door_sound__movingEvent)


[es(tag=sound, after=sound_begin_update_es, before=sound_end_update_es)]
def rotating_door_sound(evt : ParallelUpdateFrameDelayed)

  if !sound_banks_is_master_preset_loaded()
    return

  let listener = get_listener_pos()

  query() <| $ [es] (rendinst_axis_rotation__curAngle : float;
                     door_operations__closedAngle : float;
                     transform : float3x4;
                     ri_extra : RiExtraComponent;
                     rotating_door_sound__closedSpeedToVar : float2;
                     rotating_door_sound__movingSpeedToVar : float2;
                     rotating_door_sound__pushSpeedToVar : float2;
                     rotating_door_sound__openedPath : string;
                     rotating_door_sound__closedPath : string;
                     rotating_door_sound__movingPath : string;
                     rotating_door_sound__pushPath : string;
                     rotating_door_sound__movingSpeedThreshold : float;
                     rotating_door_sound__movingCooldown : float;
                     rotating_door_sound__pushCooldown : float;
                     rotating_door_sound__maxRange : float;
                     var rotating_door_sound__isInited : bool&;
                     var rotating_door_sound__isClosed : bool&;
                     var rotating_door_sound__speedHistory : float3&;
                     var rotating_door_sound__prevAngle : float&;
                     var rotating_door_sound__movingEvent : SoundEvent&;
                     var rotating_door_sound__speed : float&;
                     var rotating_door_sound__nextPushAt : float&;
                     var rotating_door_sound__nextMovingAt : float&)

    if distance_sq(listener, transform[3]) > square(rotating_door_sound__maxRange)
      rotating_door_sound__isInited = false
      if rotating_door_sound__movingEvent.enabled
        rotating_door_sound__movingEvent.enabled = false
        release(rotating_door_sound__movingEvent)
      return

    if !rotating_door_sound__isInited
      rotating_door_sound__isInited = true
      rotating_door_sound__isClosed = is_equal_float(rendinst_axis_rotation__curAngle, door_operations__closedAngle)
      rotating_door_sound__prevAngle = rendinst_axis_rotation__curAngle
      rotating_door_sound__speedHistory = float3(0., 0., 0.)

    assume speedHistory = rotating_door_sound__speedHistory
    speedHistory.x = speedHistory.y
    speedHistory.y = speedHistory.z
    speedHistory.z = safediv(abs(rendinst_axis_rotation__curAngle - rotating_door_sound__prevAngle), evt.dt)

    let speed = (speedHistory.x + speedHistory.y + speedHistory.z) * 0.333
    let isMoving = speed > (rotating_door_sound__movingEvent.enabled ? 0. : rotating_door_sound__movingSpeedThreshold)

    if rotating_door_sound__movingEvent.enabled != isMoving
      if isMoving
        if evt.curTime >= rotating_door_sound__nextMovingAt
          rotating_door_sound__movingEvent |> play(rotating_door_sound__movingPath, get_sound_pos(ri_extra, transform))
          rotating_door_sound__movingEvent.enabled = true
          rotating_door_sound__nextMovingAt = evt.curTime + rotating_door_sound__movingCooldown
      else
        rotating_door_sound__movingEvent |> abandon()
        rotating_door_sound__movingEvent.enabled = false

    if rotating_door_sound__movingEvent.enabled
      set_var_optional(rotating_door_sound__movingEvent, "velocity", cvt(speed, rotating_door_sound__movingSpeedToVar.x, rotating_door_sound__movingSpeedToVar.y, 0., 1.))
      set_pos(rotating_door_sound__movingEvent, get_sound_pos(ri_extra, transform))

    let isClosed = is_equal_float(rendinst_axis_rotation__curAngle, door_operations__closedAngle)
    if rotating_door_sound__isClosed != isClosed
      rotating_door_sound__isClosed = isClosed
      rotating_door_sound__nextPushAt = evt.curTime + rotating_door_sound__pushCooldown
      var handle = soundEvent::play("", rotating_door_sound__isClosed ? rotating_door_sound__closedPath : rotating_door_sound__openedPath, get_sound_pos(ri_extra, transform))
      set_var_optional(handle, "velocity", cvt(speed, rotating_door_sound__closedSpeedToVar.x, rotating_door_sound__closedSpeedToVar.y, 0., 1.))
      abandon(handle)

    elif !isClosed && evt.curTime >= rotating_door_sound__nextPushAt
      let speedDiff = speed - rotating_door_sound__speed
      if speedDiff > rotating_door_sound__pushSpeedToVar.x
        rotating_door_sound__nextPushAt = evt.curTime + rotating_door_sound__pushCooldown
        var handle = soundEvent::play("", rotating_door_sound__pushPath, get_sound_pos(ri_extra, transform))
        set_var(handle, "velocity", cvt(speedDiff, rotating_door_sound__pushSpeedToVar.x, rotating_door_sound__pushSpeedToVar.y, 0., 1.))
        abandon(handle)

    rotating_door_sound__prevAngle = rendinst_axis_rotation__curAngle
    rotating_door_sound__speed = speed


[es(tag=sound, on_appear, REQUIRE_NOT=(door_operations__curState, rotatingDoorSounds))]
def legacy_door_sound_appear(evt : Event;
                             rendinst_axis_rotation__curAngle : float;
                             door_operations__closedAngle : float;
                             var door_sound__isOpened : bool&)
  door_sound__isOpened = rendinst_axis_rotation__curAngle != door_operations__closedAngle


[es(tag=sound, on_event=ParallelUpdateFrameDelayed, after=sound_begin_update_es, before=sound_end_update_es, REQUIRE_NOT=(door_operations__curState, rotatingDoorSounds))]
def legacy_door_sound_update(evt : Event;
                             rendinst_axis_rotation__curAngle : float;
                             door_operations__closedAngle : float;
                             var door_sound__isOpened : bool&;
                             door_sound__openPath : string;
                             door_sound__closePath : string;
                             transform : float3x4;
                             ri_extra : RiExtraComponent)

  if rendinst_axis_rotation__curAngle != door_operations__closedAngle
    if !door_sound__isOpened
      door_sound__isOpened = true
      oneshot(door_sound__openPath, get_sound_pos(ri_extra, transform))

  elif door_sound__isOpened
    door_sound__isOpened = false
    oneshot(door_sound__closePath, get_sound_pos(ri_extra, transform))
