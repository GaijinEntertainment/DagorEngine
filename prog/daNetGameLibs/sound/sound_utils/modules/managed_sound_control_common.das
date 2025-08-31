module managed_sound_control_common shared

require ecs
require ecs.common
require app // get_sync_time
require soundSystem
require sound_utils.modules.sound_control_common
require DagorSystem


def find_index(eid : EntityId; entities : EidList; begin, end : int)
  for i in range(begin, end)
    if entities[i] == eid
      return i
  return -1


def remove_managed_sound_instance(eid : EntityId;
                                  managed_sound__typeIdx : int)
  if managed_sound__typeIdx >= 0
    query() <| $ [es] (var managed_sound_control__beginPosEnd : IPoint3List&;
                       var managed_sound_control__entities : EidList&;
                       var managed_sound_control__positions : Point3List&)

      let beginPosEnd = managed_sound_control__beginPosEnd[managed_sound__typeIdx]
      assume begin = beginPosEnd.x
      assume pos = beginPosEnd.y
      assume end = beginPosEnd.z
      let idx = find_index(eid, managed_sound_control__entities, begin, pos)
      if idx >= 0
        assert(pos > begin)
        managed_sound_control__entities[idx] = managed_sound_control__entities[pos - 1]
        managed_sound_control__positions[idx] = managed_sound_control__positions[pos - 1]
        --managed_sound_control__beginPosEnd[managed_sound__typeIdx].y


def append_managed_sound_type(managed_sound__maxInstances : int;
                              managed_sound__type : string;
                              var managed_sound__typeIdx : int&;

                              var managed_sound_control__types : StringList&;
                              var managed_sound_control__beginPosEnd : IPoint3List&;
                              var managed_sound_control__entities : EidList&;
                              var managed_sound_control__positions : Point3List&)

  assert(length(managed_sound_control__types) == length(managed_sound_control__beginPosEnd))
  assert(length(managed_sound_control__entities) == length(managed_sound_control__positions))

  managed_sound__typeIdx = find_index(managed_sound_control__types, managed_sound__type)
  if managed_sound__typeIdx == -1
    managed_sound__typeIdx = length(managed_sound_control__types)

    let begin = length(managed_sound_control__entities)
    let pos = begin
    let end = begin + managed_sound__maxInstances

    assert(managed_sound__maxInstances > 0)

    push(managed_sound_control__beginPosEnd, int3(begin, pos, end))
    push(managed_sound_control__types, managed_sound__type)

    resize(managed_sound_control__entities, end)
    resize(managed_sound_control__positions, end)
  else
    let maxInstances = managed_sound_control__beginPosEnd[managed_sound__typeIdx].z - managed_sound_control__beginPosEnd[managed_sound__typeIdx].x
    if managed_sound__maxInstances != maxInstances
      logerr("different new maxInstances({managed_sound__maxInstances} vs existing {maxInstances}) for same managed_sound__type='{managed_sound__type}'")



def update_managed_sound_control(managed_sound_control__cooldown : float;
                                 var managed_sound_control__beginPosEnd : IPoint3List&;
                                 var managed_sound_control__entities : EidList&;
                                 var managed_sound_control__positions : Point3List&)

  let listener = get_listener_pos()
  let curTime = get_sync_time()

  query() <| $ [es] (eid : EntityId;
                     managed_sound__excluded : bool;
                     managed_sound__typeIdx : int;
                     managed_sound__nextTimeToAllow : float;
                     sound_control__state : int;
                     var sound_control__allowSound : bool&;
                     transform : float3x4;
                     isAlive : bool = true)
    if managed_sound__typeIdx == -1
      sound_control__allowSound = false
      return

    if !is_hearable_sound_state(sound_control__state) || !isAlive || managed_sound__excluded
      if sound_control__allowSound // (optimization) seems if !allowSound then should not be in list
        remove_managed_sound_instance(eid, managed_sound__typeIdx)
      sound_control__allowSound = false
      return

    let beginPosEnd = managed_sound_control__beginPosEnd[managed_sound__typeIdx]
    assume begin = beginPosEnd.x
    assume pos = beginPosEnd.y
    assume end = beginPosEnd.z

    let idx = find_index(eid, managed_sound_control__entities, begin, end)
    if idx >= 0
      managed_sound_control__positions[idx] = transform[3]
      sound_control__allowSound = true
      return

    if curTime < managed_sound__nextTimeToAllow
      return

    if pos < end
      sound_control__allowSound = true
      managed_sound_control__entities[pos] = eid
      managed_sound_control__positions[pos] = transform[3]
      ++managed_sound_control__beginPosEnd[managed_sound__typeIdx].y
      return

    let raceAvoidTemporalDesyncFixDistMul = 1.1
    let distSq = length_sq((transform[3] - listener) * raceAvoidTemporalDesyncFixDistMul)

    var furthestIdx = -1
    var furthestDistSq = 0.

    for i in range(begin, pos)
      let otherPos = managed_sound_control__positions[i]
      let otherDistSq = distance_sq(otherPos, listener)
      if otherDistSq > furthestDistSq && distSq < otherDistSq
        furthestDistSq = otherDistSq
        furthestIdx = i

    if furthestIdx >= 0
      query(managed_sound_control__entities[furthestIdx]) <| $ [es] (var sound_control__allowSound : bool&;
                                                                     var managed_sound__nextTimeToAllow : float&)
        managed_sound__nextTimeToAllow = curTime + managed_sound_control__cooldown
        sound_control__allowSound = false

      managed_sound_control__entities[furthestIdx] = eid
      managed_sound_control__positions[furthestIdx] = transform[3]
      sound_control__allowSound = true
    else
      sound_control__allowSound = false
