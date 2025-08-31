require ecs
require soundEvent
require soundSystem
require math.base
require DagorMath
require DngPhysObj
require danetlibs.renderer.includes.pufd_events


[es(tag=sound, after=sound_begin_update_es, before=sound_end_update_es, REQUIRE_NOT=disableUpdate)]
def phys_obj_collision_sound(info : ParallelUpdateFrameDelayed;
                             phys_obj_net_phys : PhysObjActor;
                             phys_obj_collision_sound__enabled : bool;
                             phys_obj_collision_sound__path : string;
                             phys_obj_collision_sound__threshold : float2 const&;
                             phys_obj_collision_sound__omegaMul : float;
                             phys_obj_collision_sound__maxRange : float;
                             phys_obj_collision_sound__interval : float;
                             phys_obj_collision_sound__offset : float3;
                             var phys_obj_collision_sound__numSounds : int&;
                             var phys_obj_collision_sound__time : float&;
                             var phys_obj_collision_sound__maxImpulse : float&;
                             var phys_obj_collision_sound__event : SoundEvent&;
                             transform : float3x4)
  if !phys_obj_collision_sound__enabled || !should_play(transform[3], phys_obj_collision_sound__maxRange)
    if is_valid_handle_value(phys_obj_collision_sound__event)
      release(phys_obj_collision_sound__event)
      phys_obj_collision_sound__maxImpulse = 0.
    return

  phys_obj_collision_sound__time += info.dt
  if phys_obj_collision_sound__time < 0.
    return

  let dt = float(max(phys_obj_net_phys.phys.currentState.atTick - phys_obj_net_phys.phys.previousState.atTick, 1)) * phys_obj_net_phys.phys.timeStep
  var diffSq = length_sq((phys_obj_net_phys.phys.currentState.omega - phys_obj_net_phys.phys.previousState.omega) * phys_obj_collision_sound__omegaMul)
  if length_sq(phys_obj_net_phys.phys.currentState.velocity) < length_sq(phys_obj_net_phys.phys.previousState.velocity)
    diffSq = max(diffSq, length_sq(phys_obj_net_phys.phys.currentState.velocity - phys_obj_net_phys.phys.previousState.velocity))

  if diffSq > square(phys_obj_collision_sound__threshold.x * dt)
    let impulse = safediv(sqrt(diffSq), dt)
    if !is_valid_handle_value(phys_obj_collision_sound__event) || phys_obj_collision_sound__time > phys_obj_collision_sound__interval

      if phys_obj_collision_sound__numSounds > 0
        --phys_obj_collision_sound__numSounds
        abandon(phys_obj_collision_sound__event)
        if sound_banks_is_master_preset_loaded()
          phys_obj_collision_sound__event |> play(phys_obj_collision_sound__path, transform[3] + phys_obj_collision_sound__offset)
        phys_obj_collision_sound__maxImpulse = 0.
        phys_obj_collision_sound__time = 0.

    phys_obj_collision_sound__maxImpulse = max(phys_obj_collision_sound__maxImpulse, impulse)

  if is_valid_handle_value(phys_obj_collision_sound__event)
    set_pos(phys_obj_collision_sound__event, transform[3] + phys_obj_collision_sound__offset)
    if phys_obj_collision_sound__threshold.y > phys_obj_collision_sound__threshold.x
      let f = cvt(phys_obj_collision_sound__maxImpulse, phys_obj_collision_sound__threshold.x, phys_obj_collision_sound__threshold.y, 0.0, 1.0)
      set_var_optional(phys_obj_collision_sound__event, "force", f)
    if phys_obj_collision_sound__time > phys_obj_collision_sound__interval
      abandon(phys_obj_collision_sound__event)
      phys_obj_collision_sound__maxImpulse = 0.
      phys_obj_collision_sound__time = 0.


[es(tag=sound, on_appear, REQUIRE=disableUpdate)]
def phys_obj_collision_sound_disable(evt : Event;
                                     var phys_obj_collision_sound__time : float&;
                                     var phys_obj_collision_sound__maxImpulse : float&;
                                     var phys_obj_collision_sound__event : SoundEvent&)
  phys_obj_collision_sound__time = 0.
  phys_obj_collision_sound__maxImpulse = 0.
  release(phys_obj_collision_sound__event)
