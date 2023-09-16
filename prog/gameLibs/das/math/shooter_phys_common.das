module shooter_phys_common shared
require math
require DagorMath

def phys_accelerate_directional(cur_vel, wish_dir : float3; wish_spd, accel, dt : float) : float3
  let curSpd = dot(cur_vel, wish_dir)
  if wish_spd <= curSpd // we don't need to accelerate in that direction further, we already have proper velocity in direction
    return cur_vel
  return cur_vel + wish_dir * min(accel * dt, wish_spd - curSpd)

def phys_calc_friction_mult(cur_spd, thres_spd, friction_k, dt : float) : float
  let fricSpd = max(cur_spd, thres_spd)
  let deltaSpd = min(fricSpd * friction_k * dt, cur_spd)
  return safediv(cur_spd - deltaSpd, cur_spd)

def phys_apply_friction(cur_vel : float3; thres_spd, friction_k, dt : float) : float3
  return cur_vel * phys_calc_friction_mult(length(cur_vel.xz), thres_spd, friction_k, dt)

def phys_apply_omni_friction(cur_vel : float3; thres_spd, friction_k, dt : float) : float3
  return cur_vel * phys_calc_friction_mult(length(cur_vel), thres_spd, friction_k, dt)

def floor_physics_tick_number(time, fixed_dt : float)
  return int(time / fixed_dt)

def nearest_physics_tick_number(time, fixed_dt : float) : int
  return int(time / fixed_dt + 0.5)

def ceil_physics_tick_number(time, fixed_dt : float)
  return floor_physics_tick_number(time, fixed_dt) + 1
