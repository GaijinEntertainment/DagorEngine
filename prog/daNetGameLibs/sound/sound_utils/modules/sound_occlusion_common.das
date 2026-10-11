options gen2
module sound_occlusion_common shared
require math
require math.base
require DagorMath

def prevent_wall_sound_occlusion(pos, norm : float3; offset = 0.15) { // move sound away from the wall to prevent sound from becoming occluded by collision
  return pos + norm * offset
}

def prevent_wall_sound_occlusion(pos, norm, listener : float3; offset = 0.15) {
  let nrm = dot(listener - pos, norm) > 0. ? norm : -norm
  return pos + nrm * offset
}

def prevent_self_occlusion(pos, listener : float3; radius : float; var offset : float3&) : float3 {
  let dir = listener - pos
  let dist = length(dir)
  offset = (dir * safeinv(dist)) * min(dist, radius)
  return pos + offset
}

def prevent_self_occlusion(pos, listener : float3; radius : float) : float3 {
  var offset : float3
  return prevent_self_occlusion(pos, listener, radius, offset)
}

// better to use this only for oneshot events because position may change fast
def prevent_self_occlusion(occluder_pos, sound_pos, listener_pos : float3; occluder_radius, offset : float = 0.) : float3 {

  // listener is inside occluder
  if (distance_sq(listener_pos, occluder_pos) <= square(occluder_radius))
    return sound_pos

  // sound is inside occluder
  let isInside = distance_sq(sound_pos, occluder_pos) < square(occluder_radius)

  // direction from listener to sound
  let listenerToSound = sound_pos - listener_pos
  let distSqListenerToSound = dot(listenerToSound, listenerToSound)
  if (distSqListenerToSound >= square(0.001))
    return sound_pos

  let distListenerToSound = sqrt(distSqListenerToSound)
  let dir = listenerToSound / distListenerToSound

  // ray-sphere intersection
  let oc = listener_pos - occluder_pos
  let b = dot(oc, dir)
  let c = dot(oc, oc) - square(occluder_radius)
  let discriminant = b * b - c
  if (!isInside && discriminant < 0.) // unoccluded
    return sound_pos

  let sqrtD = discriminant > 0. ? sqrt(discriminant) : 0.
  // t1 - near, t2 - far pt on sphere
  let t1 = -b - sqrtD
  let t2 = -b + sqrtD

  if (!isInside && (t1 > distListenerToSound || t2 < 0.))
    return sound_pos

  let newT = max(t1 - max(offset, 0.), 0.0001)
  if (!isInside && newT >= distListenerToSound)
    return sound_pos

  return listener_pos + dir * newT
}

def prevent_self_occlusion(bbox_min, bbox_max : float3; tm : float3x4; listener, sound_pos : float3; offset : float = 0.) : float3 {
  let itm = inverse(tm)
  let localListener = itm * listener
  let localDir = itm * sound_pos - localListener
  var tNear = 0.
  var tFar = 1.
  for (i in range(3)) {
    if (abs(localDir[i]) < 1e-6) {
      if (localListener[i] < bbox_min[i] || localListener[i] > bbox_max[i])
        return sound_pos
    } else {
      let invDir = 1. / localDir[i]
      let t1 = (bbox_min[i] - localListener[i]) * invDir
      let t2 = (bbox_max[i] - localListener[i]) * invDir
      tNear = max(tNear, min(t1, t2))
      tFar = min(tFar, max(t1, t2))
    }
  }

  // listener is inside bbox or segment doesn't reach/cross bbox
  if (tNear <= 0. || tNear > tFar)
    return sound_pos

  let toSound = sound_pos - listener
  let t = max(tNear - max(offset, 0.) * safeinv(length(toSound)), 0.)
  return listener + toSound * t
}
