module pathfinder_common shared
require ecs
require pathfinder
require math
require math.base
require DagorMath
require DagorMathUtils
require DagorRandom
require RendInst
require math.maybe_pos public


def get_lower_navmesh_pos(position : float3; horz_extents : float; is_valid : block<(pos : float3) : bool>) : MaybePos
  var minY = FLT_MAX
  var res = [[MaybePos invalid=null]]
  query_navmesh_projections(position, float3(horz_extents, FLT_MAX, horz_extents), /*points num*/8) <| $(positions)
    for pos in positions
      if pos.y < minY && invoke(is_valid, pos)
        res = [[MaybePos pos=pos]]
        minY = pos.y
  return res

def get_lower_navmesh_pos_in_radius(position : float3; radius : float; is_valid : block<(pos : float3) : bool>) : MaybePos
  var minY = FLT_MAX
  var res = [[MaybePos invalid=null]]
  query_navmesh_projections(position, float3(radius), /*points num*/8) <| $(positions)
    for pos in positions
      if pos.y < minY && invoke(is_valid, pos)
        res = [[MaybePos pos=pos]]
        minY = pos.y
  return res

def find_reachable_nav_point_around(start_pos, center : float3; radius : float; tries : int; is_valid : block<(pos : float3) : bool>) : MaybePos
  return find_nav_point_around(start_pos, center, radius, tries, is_valid, false)

def find_nav_point_around(start_pos, center : float3; radius : float; tries : int; is_valid : block<(pos : float3) : bool>; accept_partial : bool = false) : MaybePos
  var res = [[MaybePos invalid=null]]
  // TODO: project start pos to navmesh
  using() <| $(var path_corridor : dtPathCorridor)
    path_corridor |> init_path_corridor()
    let angleDelta = TWOPI * safeinv(float(tries))
    let startAngle = TWOPI * gfrnd()
    for i in range(tries)
      let angle = startAngle + float(i) * angleDelta
      var s, c : float
      sincos(angle, s, c)
      var pos = center + float3(c * radius, 0f, s * radius)
      var endRef : dtPolyRef
      if !project_to_nearest_navmesh_point(pos, float3(1., 3., 1.), endRef)
        continue
      if !invoke(is_valid, pos)
        continue
      let extents = float3(0.5, FLT_MAX, 0.5)
      using(start_pos, pos, extents) <| $(var inp : CorridorInput#)
        inp.targetPoly = endRef
        inp.includeFlags |= int(PolyFlag POLYFLAG_JUMP)
        let pathRes = set_path_corridor(path_corridor, inp, null)
        if pathRes != FindPathResult FPR_FAILED && (accept_partial || pathRes != FindPathResult FPR_PARTIAL)
          res = [[MaybePos pos=pos]]
      if res is pos
        break

  return res


def get_triangle_by_pos(pos : float3; hor_dist : float; var result : NavMeshTriangle&; max_vert_dist : float = FLT_MAX) : bool
  return get_triangle_by_pos(pos, result, hor_dist, int(PolyFlag POLYFLAG_GROUND) | int(PolyFlag POLYFLAG_OBSTACLE), 0, null, max_vert_dist)


def get_triangle_by_pos_ex(nav_mesh_idx : int; pos : float3; hor_dist : float; var result : NavMeshTriangle&; max_vert_dist : float = FLT_MAX) : bool
  return get_triangle_by_pos_ex(nav_mesh_idx, pos, result, hor_dist, int(PolyFlag POLYFLAG_GROUND) | int(PolyFlag POLYFLAG_OBSTACLE), 0, null, max_vert_dist)


def get_surface_normal_by_pos(pos : float3; poly : dtPolyRef)
  var res : NavMeshTriangle
  if get_triangle_by_poly(pos, poly, res)
    let x0 = res.p0 - res.p2
    let x1 = res.p1 - res.p2
    return cross(x0, x1) |> normalize()
  return float3(0, 1, 0)

def trace_single_ri_against_path(ri_eid : EntityId;
                                 startPos : float3;
                                 walker_agent__standShootLocalPos : float3;
                                 path_corners;
                                 max_path_index : int;
                                 trace_distance_budget : float) : int
  var result = -1
  var traceDistanceBudget = trace_distance_budget
  query(ri_eid) <| $ [es] (ri_extra : RiExtraComponent; transform : float3x4)
    let riType = handle_to_ri_type(ri_extra.handle)
    let riCollres = get_ri_gen_extra_collres(int(riType))
    if riCollres == null
      return
    var box = BBox3()
    box |> bbox3f_to_scalar_bbox3(riCollres.vFullBBox)
    let invTm = inverse(transform)
    var start = startPos
    for idx in range(max_path_index)
      let end = path_corners[idx]
      var traceDir = end - start
      let traceDirLen = length(traceDir)
      traceDir *= safeinv(traceDirLen)
      let currentTraceDist = min(traceDistanceBudget, traceDirLen)
      traceDistanceBudget -= currentTraceDist

      let traceRight = 0.3 * normalize(cross(float3(0, 1, 0), traceDir))
      //Use only Y to avoid problems with rotation
      var agentYOffset = float3(.0, walker_agent__standShootLocalPos.y, .0)
      let quat = dir_to_quat(traceDir)
      agentYOffset = quat * agentYOffset

      let traceFrom = [[float3[]
        start + agentYOffset + traceRight;
        start + agentYOffset - traceRight;
        start + agentYOffset * 0.5 + traceRight;
        start + agentYOffset * 0.5 - traceRight
      ]]
      //draw_debug_box_buffered(box, transform, E3DCOLOR(0xffffffff), 700)
      for trace in traceFrom
        //draw_debug_arrow_buffered(trace, trace + traceDir * currentTraceDist, E3DCOLOR(0xff0000ff), 700)
        let p1 = invTm * trace
        let p2 = invTm * (trace + traceDir * currentTraceDist)
        if test_segment_box_intersection(p1, p2, box)
          result = idx
          return
      if traceDistanceBudget <= 0.0
        return
      start = end
  return result


def iterate_over_polys_recursively(start : float3; depth : int; initial_extents : float3; action : block<(poly : dtPolyRef) : bool>)
  var visited : array<dtPolyRef>
  var frontier : array<tuple<poly : dtPolyRef; depth : int>>

  var ref : dtPolyRef
  var p = start
  if project_to_nearest_navmesh_point(p, initial_extents, ref)
    frontier |> emplace([[auto ref, depth]])
    while (length(frontier) > 0)
      advance_poly_frontier(frontier, visited)
      if invoke(action, visited[length(visited) - 1])
        break


def private advance_poly_frontier(var frontier : array<tuple<poly : dtPolyRef; depth : int>>; var visited : array<dtPolyRef>)
  let thisPoly = frontier[0].poly
  var depth = frontier[0].depth
  frontier |> erase(0)
  visited |> push(thisPoly)
  var polyFlags = uint16(0)
  get_poly_flags(thisPoly, polyFlags)
  if (int(polyFlags) & int(PolyFlag POLYFLAG_JUMP)) == 0
    depth--
  if depth > 0
    for_each_linked_poly(thisPoly) <| $(linkedPolyRef)
      let inFrontierIdx = find_index_if(frontier) <| $(item) { return item.poly == linkedPolyRef; }
      if find_index(visited, linkedPolyRef) == -1 && inFrontierIdx == -1
        frontier |> emplace([[auto linkedPolyRef, depth]])
      elif inFrontierIdx != -1
        frontier[inFrontierIdx].depth = max(frontier[inFrontierIdx].depth, depth)