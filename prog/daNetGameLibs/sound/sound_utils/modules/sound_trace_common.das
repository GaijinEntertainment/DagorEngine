module sound_trace_common shared

require ecs
require PhysMat
require Dacoll
require GridCollision
require CollisionTraces


def get_trace_flags_for_biome_and_puddle_queries()
  return ETF_LMESH | ETF_HEIGHTMAP


def trace_floor(pos : float3;
                offset : float;
                var trace_len : float&;
                var pmid : int& = PHYSMAT_INVALID;
                dir_to_floor : float3;
                floorIsDown : bool;
                var trace_mesh_faces : TraceMeshFaces? = null;
                flags = ETF_LMESH | ETF_HEIGHTMAP | ETF_FRT | ETF_RI | ETF_STRUCTURES) : bool
  let from = pos - dir_to_floor * offset
  var norm : float3
  var riDesc = RendInstDesc()
  var t = trace_len + offset
  if floorIsDown
    if !tracedown_normalized(from, t, pmid, flags, trace_mesh_faces)
      pmid = PHYSMAT_INVALID
      return false
  elif !traceray_normalized(from, dir_to_floor, t, pmid, norm, flags, riDesc, -1)
    pmid = PHYSMAT_INVALID
    return false
  trace_len = t - offset
  return true


def is_floor_down(entity_up : float3) : bool
  return entity_up.y > 0.9


def make_dir_to_floor(entity_up : float3; floor_is_down : bool)
  if floor_is_down
    return float3(0., -1., 0.)
  return -normalize(entity_up)


def trace_floor(pos, entity_up : float3;
                offset : float;
                var trace_len : float&;
                var pmid : int& = PHYSMAT_INVALID;
                var dir_to_floor : float3&;
                var floorIsDown : bool&;
                var trace_mesh_faces : TraceMeshFaces? = null;
                flags = ETF_LMESH | ETF_HEIGHTMAP | ETF_FRT | ETF_RI | ETF_STRUCTURES) : bool
  floorIsDown = is_floor_down(entity_up)
  dir_to_floor = make_dir_to_floor(entity_up, floorIsDown)
  return trace_floor(pos, offset, trace_len, pmid, dir_to_floor, floorIsDown, trace_mesh_faces, flags)


def trace_vehicles(exclude_eid : EntityId;
                   pos : float3;
                   dir_to_floor : float3;
                   offset : float;
                   var trace_len : float&;
                   var pmid : int&)
  let from = pos - dir_to_floor * offset
  var t = trace_len + offset
  trace_entities_in_grid(ecs_hash("vehicles"), from, dir_to_floor, t, exclude_eid, SortIntersections.NO) <| $(intersections : array<IntersectedEntity>#)
    for it in intersections
      if it.t < t + FLT_EPSILON
        t = it.t
        trace_len = t - offset
        pmid = PHYSMAT_INVALID
        query(it.eid) <| $ [es] (material : string)
          pmid = get_material_id(material)
        if pmid == PHYSMAT_INVALID
          pmid = get_material_id("steel")

