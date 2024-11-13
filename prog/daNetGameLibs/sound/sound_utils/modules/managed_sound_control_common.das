module managed_sound_control_common shared

require ecs
require ecs.common


def managed_sound_control_impl(is_in_range : bool;
                               managed_sound_control__maxInstances : int;
                               managed_sound_control__cooldown : float;
                               var managed_sound_control__entities : EidList&;
                               var managed_sound_control__positions : Point3List&;

                               listener : float3;
                               curTime : float;

                               eid : EntityId;
                               var sound_control__allowSound : bool&;
                               managed_sound__nextTimeToAllow : float;
                               transform : float3x4)
  if !is_in_range
    sound_control__allowSound = false
    return

  let pos = transform[3]
  let idx = find_index(managed_sound_control__entities, eid)

  if idx >= 0
    sound_control__allowSound = true
    managed_sound_control__positions[idx] = pos
    return

  if curTime < managed_sound__nextTimeToAllow
    return

  if length(managed_sound_control__entities) < managed_sound_control__maxInstances
    sound_control__allowSound = true
    managed_sound_control__entities |> push(eid)
    managed_sound_control__positions |> push(pos)
    return

  let raceAvoidTemporalDesyncFixDistMul = 1.1
  let distSq = length_sq((pos - listener) * raceAvoidTemporalDesyncFixDistMul)

  var furthestIdx = -1
  var furthestDistSq = 0.

  for i in range(length(managed_sound_control__entities))
    let otherPos = managed_sound_control__positions[i]
    let otherDistSq = distance_sq(otherPos, listener)

    if otherDistSq > furthestDistSq && distSq < otherDistSq
      furthestDistSq = otherDistSq;
      furthestIdx = i

  if furthestIdx >= 0
    query(managed_sound_control__entities[furthestIdx]) <| $ [es] (var sound_control__allowSound : bool&;
                                                                   var managed_sound__nextTimeToAllow : float&)
      sound_control__allowSound = false
      managed_sound__nextTimeToAllow = curTime + managed_sound_control__cooldown

    sound_control__allowSound = true
    managed_sound_control__entities[furthestIdx] = eid
    managed_sound_control__positions[furthestIdx] = pos
  else
    sound_control__allowSound = false


def remove_managed_sound(eid : EntityId)
  query() <| $ [es] (var managed_sound_control__entities : EidList&;
                     var managed_sound_control__positions : Point3List&)
    let idx = find_index(managed_sound_control__entities, eid)

    if idx >= 0
      managed_sound_control__entities |> erase(idx)
      managed_sound_control__positions |> erase(idx)
