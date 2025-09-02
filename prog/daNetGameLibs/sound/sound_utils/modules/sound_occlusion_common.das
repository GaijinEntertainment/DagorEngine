module sound_occlusion_common shared

require math

def prevent_wall_sound_occlusion(pos, norm : float3; offset = 0.15) // move sound away from the wall to prevent sound from becoming occluded by collision
  return pos + norm * offset

def prevent_wall_sound_occlusion(pos, norm, listener : float3; offset = 0.15)
  let nrm = dot(listener - pos, norm) > 0. ? norm : -norm
  return pos + nrm * offset
