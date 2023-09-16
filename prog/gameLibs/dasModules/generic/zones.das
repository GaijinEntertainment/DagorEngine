require ecs
require zones

[generic]
def is_inside_truncated_sphere_zone(pos : float3; zone_tm : float3x4; zone_radius : float; zone_truncate_below : float const?; scale : float = 1.0)
  let scaledRadius = zone_radius * scale
  if length_sq(pos - zone_tm[3]) > scaledRadius * scaledRadius
    return false
  if zone_truncate_below != null
    let distBelow = dot(zone_tm[1], pos - zone_tm[3])
    if distBelow < *zone_truncate_below * scale
      return false
  return true
