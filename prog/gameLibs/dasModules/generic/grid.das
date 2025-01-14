require ecs
require Grid

[generic]
def for_each_entity_in_grid(grid_name_hash : uint; bounding : BBox3 | BSphere3; check_type : GridEntCheck; cb : block<(eid : EntityId; pos : float3) : void>)
  find_entity_in_grid(grid_name_hash, bounding, check_type) <| $(eid : EntityId; pos : float3)
    invoke(cb, eid, pos)
    return false

[generic]
def for_each_entity_in_grid(grid_name_hash : uint; bounding : BBox3 | BSphere3; check_type : GridEntCheck; cb : block<(eid : EntityId) : void>)
  find_entity_in_grid(grid_name_hash, bounding, check_type) <| $ [unused_argument(pos)] (eid : EntityId; pos : float3)
    invoke(cb, eid)
    return false

[generic]
def for_each_entity_in_grid(grid_name_hash : uint; from, dir : float3; len, radius : float; check_type : GridEntCheck; cb : block<(eid : EntityId; pos : float3) : void>)
  find_entity_in_grid(grid_name_hash, from, dir, len, radius, check_type) <| $(eid : EntityId; pos : float3)
    invoke(cb, eid, pos)
    return false

[generic]
def for_each_entity_in_grid(grid_name_hash : uint; from, dir : float3; len, radius : float; check_type : GridEntCheck; cb : block<(eid : EntityId) : void>)
  find_entity_in_grid(grid_name_hash, from, dir, len, radius, check_type) <| $ [unused_argument(pos)] (eid : EntityId; pos : float3)
    invoke(cb, eid)
    return false

[generic]
def for_each_entity_in_grid(grid_name_hash : uint; transform : float3x4; bbox : BBox3; check_type : GridEntCheck; cb : block<(eid : EntityId; pos : float3) : void>)
  find_entity_in_grid(grid_name_hash, transform, bbox, check_type) <| $(eid : EntityId; pos : float3)
    invoke(cb, eid, pos)
    return false

[generic]
def for_each_entity_in_grid(grid_name_hash : uint; transform : float3x4; bbox : BBox3; check_type : GridEntCheck; cb : block<(eid : EntityId) : void>)
  find_entity_in_grid(grid_name_hash, transform, bbox, check_type) <| $ [unused_argument(pos)] (eid : EntityId; pos : float3)
    invoke(cb, eid)
    return false


[generic]
def for_each_entity_in_grid(grid_name_hashes : uint[]; bounding : BBox3 | BSphere3; check_type : GridEntCheck; cb : block<(eid : EntityId; pos : float3) : void>)
  for grid_name_hash in grid_name_hashes
    for_each_entity_in_grid(grid_name_hash, bounding, check_type, cb)

[generic]
def for_each_entity_in_grid(grid_name_hashes : uint[]; bounding : BBox3 | BSphere3; check_type : GridEntCheck; cb : block<(eid : EntityId) : void>)
  for grid_name_hash in grid_name_hashes
    for_each_entity_in_grid(grid_name_hash, bounding, check_type, cb)

[generic]
def for_each_entity_in_grid(grid_name_hashes : uint[]; from, dir : float3; len, radius : float; check_type : GridEntCheck; cb : block<(eid : EntityId; pos : float3) : void>)
  for grid_name_hash in grid_name_hashes
    for_each_entity_in_grid(grid_name_hash, from, dir, len, radius, check_type, cb)

[generic]
def for_each_entity_in_grid(grid_name_hashes : uint[]; from, dir : float3; len, radius : float; check_type : GridEntCheck; cb : block<(eid : EntityId) : void>)
  for grid_name_hash in grid_name_hashes
    for_each_entity_in_grid(grid_name_hash, from, dir, len, radius, check_type, cb)

[generic]
def for_each_entity_in_grid(grid_name_hashes : uint[]; transform : float3x4; bbox : BBox3; check_type : GridEntCheck; cb : block<(eid : EntityId; pos : float3) : void>)
  for grid_name_hash in grid_name_hashes
    for_each_entity_in_grid(grid_name_hash, transform, bbox, check_type, cb)

[generic]
def for_each_entity_in_grid(grid_name_hashes : uint[]; transform : float3x4; bbox : BBox3; check_type : GridEntCheck; cb : block<(eid : EntityId) : void>)
  for grid_name_hash in grid_name_hashes
    for_each_entity_in_grid(grid_name_hash, transform, bbox, check_type, cb)


[generic]
def find_entity_in_grid(grid_name_hash : uint; bounding : BBox3 | BSphere3; check_type : GridEntCheck; cb : block<(eid : EntityId) : bool>) : EntityId
  return find_entity_in_grid(grid_name_hash, bounding, check_type) <| $ [unused_argument(pos)] (eid : EntityId; pos : float3)
    return invoke(cb, eid)

[generic]
def find_entity_in_grid(grid_name_hash : uint; from, dir : float3; len, radius : float; check_type : GridEntCheck; cb : block<(eid : EntityId) : bool>) : EntityId
  return find_entity_in_grid(grid_name_hash, from, dir, len, radius, check_type) <| $ [unused_argument(pos)] (eid : EntityId; pos : float3)
    return invoke(cb, eid)

[generic]
def find_entity_in_grid(grid_name_hash : uint; transform : float3x4; bbox : BBox3; check_type : GridEntCheck; cb : block<(eid : EntityId) : bool>) : EntityId
  return find_entity_in_grid(grid_name_hash, transform, bbox, check_type) <| $ [unused_argument(pos)] (eid : EntityId; pos : float3)
    return invoke(cb, eid)


[generic]
def find_entity_in_grid(grid_name_hashes : uint[]; bounding : BBox3 | BSphere3; check_type : GridEntCheck; cb : block<(eid : EntityId; pos : float3) : bool>) : EntityId
  for grid_name_hash in grid_name_hashes
    let found = find_entity_in_grid(grid_name_hash, bounding, check_type, cb)
    if found != INVALID_ENTITY_ID
      return found
  return INVALID_ENTITY_ID

[generic]
def find_entity_in_grid(grid_name_hashes : uint[]; bounding : BBox3 | BSphere3; check_type : GridEntCheck; cb : block<(eid : EntityId) : bool>) : EntityId
  for grid_name_hash in grid_name_hashes
    let found = find_entity_in_grid(grid_name_hash, bounding, check_type, cb)
    if found != INVALID_ENTITY_ID
      return found
  return INVALID_ENTITY_ID

[generic]
def find_entity_in_grid(grid_name_hashes : uint[]; from, dir : float3; len, radius : float; check_type : GridEntCheck; cb : block<(eid : EntityId; pos : float3) : bool>) : EntityId
  for grid_name_hash in grid_name_hashes
    let found = find_entity_in_grid(grid_name_hash, from, dir, len, radius, check_type, cb)
    if found != INVALID_ENTITY_ID
      return found
  return INVALID_ENTITY_ID

[generic]
def find_entity_in_grid(grid_name_hashes : uint[]; from, dir : float3; len, radius : float; check_type : GridEntCheck; cb : block<(eid : EntityId) : bool>) : EntityId
  for grid_name_hash in grid_name_hashes
    let found = find_entity_in_grid(grid_name_hash, from, dir, len, radius, check_type, cb)
    if found != INVALID_ENTITY_ID
      return found
  return INVALID_ENTITY_ID

[generic]
def find_entity_in_grid(grid_name_hashes : uint[]; transform : float3x4; bbox : BBox3; check_type : GridEntCheck; cb : block<(eid : EntityId; pos : float3) : bool>) : EntityId
  for grid_name_hash in grid_name_hashes
    let found = find_entity_in_grid(grid_name_hash, transform, bbox, check_type, cb)
    if found != INVALID_ENTITY_ID
      return found
  return INVALID_ENTITY_ID

[generic]
def find_entity_in_grid(grid_name_hashes : uint[]; transform : float3x4; bbox : BBox3; check_type : GridEntCheck; cb : block<(eid : EntityId) : bool>) : EntityId
  for grid_name_hash in grid_name_hashes
    let found = find_entity_in_grid(grid_name_hash, transform, bbox, check_type, cb)
    if found != INVALID_ENTITY_ID
      return found
  return INVALID_ENTITY_ID


[generic]
def is_entity_found_in_grid(grid_name_hash : uint; bounding : BBox3 | BSphere3; check_type : GridEntCheck) : bool
  return INVALID_ENTITY_ID != find_entity_in_grid(grid_name_hash, bounding, check_type) <| $ [unused_argument(eid, pos)] (eid : EntityId; pos : float3)
    return true

[generic]
def is_entity_found_in_grid(grid_name_hashes : uint[]; bounding : BBox3 | BSphere3; check_type : GridEntCheck) : bool
  return INVALID_ENTITY_ID != find_entity_in_grid(grid_name_hashes, bounding, check_type) <| $ [unused_argument(eid, pos)] (eid : EntityId; pos : float3)
    return true

[generic]
def is_entity_found_in_grid(grid_name_hashes : uint[]; from, dir : float3; len, radius : float; check_type : GridEntCheck) : bool
  return INVALID_ENTITY_ID != find_entity_in_grid(grid_name_hashes, from, dir, len, radius, check_type) <| $ [unused_argument(eid, pos)] (eid : EntityId; pos : float3)
    return true

[generic]
def is_entity_found_in_grid(grid_name_hash : uint; transform : float3x4; bbox : BBox3; check_type : GridEntCheck) : bool
  return INVALID_ENTITY_ID != find_entity_in_grid(grid_name_hash, transform, bbox, check_type) <| $ [unused_argument(eid, pos)] (eid : EntityId; pos : float3)
    return true


[generic]
def gather_entities_in_grid(grid_name_hashes : uint[]; bounding : BBox3 | BSphere3; check_type : GridEntCheck; cb : block<(eids : array<EntityId>) : void>)
  var eids : array<EntityId>
  for grid_name_hash in grid_name_hashes
    append_entities_in_grid(grid_name_hash, bounding, check_type, eids)
  invoke(cb, eids)

[generic]
def gather_entities_in_grid(grid_name_hashes : uint[]; from, dir : float3; len, radius : float; check_type : GridEntCheck; cb : block<(eids : array<EntityId>) : void>)
  var eids : array<EntityId>
  for grid_name_hash in grid_name_hashes
    append_entities_in_grid(grid_name_hash, from, dir, len, radius, check_type, eids)
  invoke(cb, eids)

[generic]
def gather_entities_in_grid(grid_name_hashes : uint[]; transform : float3x4; bbox : BBox3; check_type : GridEntCheck; cb : block<(eids : array<EntityId>) : void>)
  var eids : array<EntityId>
  for grid_name_hash in grid_name_hashes
    append_entities_in_grid(grid_name_hash, transform, bbox, check_type, eids)
  invoke(cb, eids)
