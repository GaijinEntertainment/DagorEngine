require app
require ecs
require Dacoll
options no_aot
require PhysMat
require strings
require pathfinder
require DagorConsole
require DagorDebug3D
require DagorDebug3DSolid
require danetlibs.pathfinder.main.pathfinder_common


[console_cmd(name="path.check_path")]
def path_check_path(sx, sy, sz, ex, ey, ez : float; dist_to_path : float = 10f;
                    horz_threshold : float = -1f; max_vert_dist : float = 10f; max_jump_up_height : float = FLT_MAX)
  let start = float3(sx, sy, sz)
  let end = float3(ex, ey, ez)
  let extents = float3(dist_to_path, FLT_MAX, dist_to_path)
  using(start, end, extents) <| $(var req : FindRequest#)
    req.includeFlags |= int(PolyFlag POLYFLAG_JUMP)
    req.maxJumpUpHeight = max_jump_up_height
    let res = check_path(req, horz_threshold, max_vert_dist)
    draw_debug_sphere_buffered(start, 1f, E3DCOLOR(0xFFFF0000), 12, 2000)
    draw_debug_sphere_buffered(end, 1f, E3DCOLOR(0xFF00FF00), 12, 2000)
    console_print("check_path({start}, {end}, {extents}, {horz_threshold}, {max_vert_dist}, {max_jump_up_height}) == {res}")


[console_cmd(name="path.calc_approx_path_length")]
def path_calc_approx_path_length(sx, sy, sz, ex, ey, ez : float; dist_to_path : float = 10f;
                                 horz_threshold : float = -1f; max_vert_dist : float = 10f; max_jump_up_height : float = FLT_MAX)
  let start = float3(sx, sy, sz)
  let end = float3(ex, ey, ez)
  let extents = float3(dist_to_path, FLT_MAX, dist_to_path)
  using(start, end, extents) <| $(var req : FindRequest#)
    req.includeFlags |= int(PolyFlag POLYFLAG_JUMP)
    req.maxJumpUpHeight = max_jump_up_height
    let res = calc_approx_path_length(req, horz_threshold, max_vert_dist)
    draw_debug_sphere_buffered(start, 1f, E3DCOLOR(0xFFFF0000), 12, 2000)
    draw_debug_sphere_buffered(end, 1f, E3DCOLOR(0xFF00FF00), 12, 2000)
    console_print("calc_approx_path_length({start}, {end}, {extents}, {horz_threshold}, {max_vert_dist}, {max_jump_up_height}) == {res}")


[console_cmd(name="path.find_path")]
def path_find_path(sx, sy, sz, ex, ey, ez : float; dist_to_path : float = 10f; step_size : float = 10f; slop : float = 2.5; max_jump_up_height = FLT_MAX)
  let start = float3(sx, sy, sz)
  let end = float3(ex, ey, ez)
  draw_debug_sphere_buffered(start, 1f, E3DCOLOR(0xFFFF0000), 12, 2000)
  draw_debug_sphere_buffered(end, 1f, E3DCOLOR(0xFF00FF00), 12, 2000)
  let extents = float3(dist_to_path, FLT_MAX, dist_to_path)
  using(start, end, extents) <| $(var req : FindRequest#)
    req.includeFlags |= int(PolyFlag POLYFLAG_JUMP)
    req.maxJumpUpHeight = max_jump_up_height
    let res = find_path(req, step_size, slop) <| $(path)
      console_print("path: len {length(path)}")
      if length(path) > 1
        var begin = path[0]
        print("{begin}")
        draw_debug_sphere_buffered(begin, 0.1f, E3DCOLOR(0xFF0000FF), 12, 2000)
        for i in range(1, length(path))
          let radius = saturate(0.1f + float(i) * 0.1f)
          draw_debug_sphere_buffered(path[i], radius, E3DCOLOR(0xFF0000FF), 12, 2000)
          draw_debug_line_buffered(begin, path[i], E3DCOLOR(0xFF0000FF), 2000)
          begin = path[i]
          print("{begin}")

    console_print("find_path({start}, {end}, {extents}, {step_size}, {slop}, {max_jump_up_height}) == {res}")


[console_cmd(name="path.project_to_nearest_navmesh_point")]
def path_project_to_nearest_navmesh_point(sx, sy, sz : float; horz_extents : float = 0.5)
  let start = float3(sx, sy, sz)
  var pos = start
  let res = project_to_nearest_navmesh_point(pos, horz_extents)
  draw_debug_sphere_buffered(start, 0.25f, E3DCOLOR(0xFF0000FF), 12, 2000)
  console_print("project_to_nearest_navmesh_point({start}, {horz_extents}) == {res}, result position = {pos}")
  draw_debug_sphere_buffered(pos, 0.3f, E3DCOLOR(0xFF00FF00), 12, 2000)

[console_cmd(name="path.project_to_nearest_navmesh_point_3d")]
def path_project_to_nearest_navmesh_point_3d(sx, sy, sz : float; ext_x = 0.5; ext_y = 0.5; ext_z = 0.5)
  let start = float3(sx, sy, sz)
  var pos = start
  let ext = float3(ext_x, ext_y, ext_z)
  var ref : dtPolyRef
  let res = project_to_nearest_navmesh_point(pos, ext, ref)
  draw_debug_sphere_buffered(start, 0.25f, E3DCOLOR(0xFF0000FF), 12, 2000)
  console_print("project_to_nearest_navmesh_point({start}, {ext}) == {res}, (ref {int64(ref)}) result position = {pos}")
  draw_debug_sphere_buffered(pos, 0.3f, E3DCOLOR(0xFF00FF00), 12, 2000)


[console_cmd(name="path.query_navmesh_projections")]
def path_query_navmesh_projections(sx, sy, sz : float; horz_extents_x = 0.5; horz_extents_y = 0.5; horz_extents_z = 0.5; points_num = 8)
  let start = float3(sx, sy, sz)
  let extents = float3(horz_extents_x, horz_extents_y, horz_extents_z)
  draw_debug_sphere_buffered(start, 0.25f, E3DCOLOR(0xFF0000FF), 12, 2000)
  let res = query_navmesh_projections(start, extents, points_num) <| $(positions)
    for idx, pos in iter_range(positions), positions
      print("query_navmesh_projections({start}, {extents}) pos {idx}/{length(positions)} = {pos}")
      draw_debug_sphere_buffered(pos, 0.3f, E3DCOLOR(0xFF00FF00), 12, 2000)
  console_print("query_navmesh_projections({start}, {extents}) == {res}")


[console_cmd(name="path.get_lower_navmesh_pos")]
def path_get_lower_navmesh_pos(sx, sy, sz : float; ex, ey, ez : float; horz_extents : float = 0.5; dist_to_path : float = 10f; horz_threshold : float = -1f; max_vert_dist : float = 10f)
  let start = float3(sx, sy, sz)
  let end = float3(ex, ey, ez)
  let res = get_lower_navmesh_pos(end, horz_extents) <| $(p)
    var res = false
    let extents = float3(dist_to_path, FLT_MAX, dist_to_path)
    using(start, p, extents) <| $(var req : FindRequest#)
      res = check_path(req, horz_threshold, max_vert_dist)
    return res
  draw_debug_sphere_buffered(end, 0.25f, E3DCOLOR(0xFF0000FF), 12, 2000)
  console_print("get_lower_navmesh_pos({start}, {horz_extents}) == {res}, result position = {res}")
  if res is pos
    draw_debug_sphere_buffered(res as pos, 0.3f, E3DCOLOR(0xFF00FF00), 12, 2000)


[console_cmd(name="corridor.find_path")]
def corridor_find_path(sx, sy, sz, ex, ey, ez : float; maxJumpUpHeight : float = FLT_MAX)
  let start = float3(sx, sy, sz)
  let end = float3(ex, ey, ez)
  using() <| $(var path_corridor : dtPathCorridor)
    path_corridor |> init_path_corridor()

    let extents = float3(.5, FLT_MAX, .5)
    using(start, end, extents) <| $(var inp : CorridorInput#)
      if maxJumpUpHeight < FLT_MAX
        inp.includeFlags |= int(PolyFlag POLYFLAG_JUMP)
      inp.maxJumpUpHeight = maxJumpUpHeight
      let res = path_corridor |> set_path_corridor(inp, null)

      draw_debug_sphere_buffered(start, 1f, E3DCOLOR(0xFFFF0000), 12, 2000)
      draw_debug_sphere_buffered(end, 1f, E3DCOLOR(0xFF00FF00), 12, 2000)

      path_corridor |> corridor_getPath() <| $(list)
        let line = build_string() <| $(str)
          for l in list
            str |> format("%lld", l)
            str |> write("  ")
        console_print(line)

      path_corridor |> find_corridor_corners(1000) <| $(corners)
        console_print("corridor: corners len {length(corners)}")
        if length(corners) > 0
          var begin = corners[0]
          draw_debug_sphere_buffered(begin, 0.1f, E3DCOLOR(0xFF0000FF), 12, 2000)
          draw_debug_line_buffered(begin, start, E3DCOLOR(0xFF0000FF), 2000)
          for i in range(1, length(corners))
            let radius = saturate(0.1f + float(i) * 0.1f)
            draw_debug_sphere_buffered(corners[i], radius, E3DCOLOR(0xFF0000FF), 12, 2000)
            draw_debug_line_buffered(begin, corners[i], E3DCOLOR(0xFF0000FF), 2000)
            begin = corners[i]

      console_print("corridor({start}, {end}) == {res}")


[console_cmd(name="path.find_random_point_around_circle")]
def path_find_random_point(sx, sy, sz, radius : float)
  let start = float3(sx, sy, sz)
  var resPos : float3
  let res = find_random_point_around_circle(start, radius, resPos)

  draw_debug_sphere_buffered(start, 0.25f, E3DCOLOR(0xFF0000FF), 12, 2000)
  if res
    draw_debug_sphere_buffered(resPos, 0.3f, E3DCOLOR(0xFF00FF00), 12, 2000)
  else
    draw_debug_sphere_buffered(resPos, 0.3f, E3DCOLOR(0xFFFF0000), 12, 2000)
  console_print("find_random_point_around_circle({start}, {radius}) == {res}, result position = {resPos} distance = {distance(start, resPos)}")


[console_cmd(name="path.find_random_point_inside_circle")]
def path_find_random_point_inside_circle(sx, sy, sz, radius, extents : float)
  let start = float3(sx, sy, sz)
  var resPos : float3
  let res = find_random_point_inside_circle(start, radius, extents, resPos)

  draw_debug_sphere_buffered(start, 0.25f, E3DCOLOR(0xFF0000FF), 12, 2000)
  if res
    draw_debug_sphere_buffered(resPos, 0.3f, E3DCOLOR(0xFF00FF00), 12, 2000)
  else
    draw_debug_sphere_buffered(resPos, 0.3f, E3DCOLOR(0xFFFF0000), 12, 2000)
  console_print("find_random_point_inside_circle({start}, {radius}) == {res}, result position = {resPos} distance = {distance(start, resPos)}")


[console_cmd(name="path.draw_poly")]
def navmesh_draw_poly(sx, sy, sz : float; poly_id : uint64; dist_threshold = 0.5)
  var res : NavMeshTriangle
  let start = float3(sx, sy, sz)
  let success = find_nearest_triangle_by_pos(start, poly_id, dist_threshold, res)
  draw_debug_line_buffered(res.p0, res.p1, E3DCOLOR(0xFF0000FF), 30)
  draw_debug_line_buffered(res.p1, res.p2, E3DCOLOR(0xFF0000FF), 30)
  draw_debug_line_buffered(res.p2, res.p0, E3DCOLOR(0xFF0000FF), 30)
  console_print("find_nearest_triangle_by_pos({start}, {poly_id}) == {success}, result tri = {res}")


[console_cmd(name="path.find_poly")]
def navmesh_find_poly(sx, sy, sz : float; nav_mesh_idx : int = 0; horz_dist : float = 1f)
  var res : NavMeshTriangle
  let start = float3(sx, sy, sz)
  let success = get_triangle_by_pos_ex(nav_mesh_idx, start, horz_dist, res)
  draw_debug_line_buffered(res.p0, res.p1, E3DCOLOR(0xFF0000FF), 30)
  draw_debug_line_buffered(res.p1, res.p2, E3DCOLOR(0xFF0000FF), 30)
  draw_debug_line_buffered(res.p2, res.p0, E3DCOLOR(0xFF0000FF), 30)
  console_print("get_triangle_by_pos({start}, {horz_dist}) == {success}, result tri = {res}")


def debug_polys_around_point(sx, sy, sz, radius : float)
  var center = float3(sx, sy, sz)
  let maxRad = 50.f
  var r = radius
  if radius > maxRad
    r = maxRad
    console_print("too big radius, scaled down to {r}")
  var success = false
  if project_to_nearest_navmesh_point(center, 0.1)
    success = find_polys_in_circle(center, r, 0.2) <| $(polyRefs)
      console_print("polyRefs: size {length(polyRefs)}")
      for i in range(0, length(polyRefs))
        var res : NavMeshTriangle
        if get_triangle_by_poly(dtPolyRef(polyRefs[i]), res)
          draw_debug_line_buffered(res.p0, res.p1, E3DCOLOR(0xFF0000FF), 7000)
          draw_debug_line_buffered(res.p1, res.p2, E3DCOLOR(0xFF0000FF), 7000)
          draw_debug_line_buffered(res.p2, res.p0, E3DCOLOR(0xFF0000FF), 7000)
  console_print("find_polys_in_circle({center}, {r}) == {success}")


[console_cmd(name="path.find_polys_in_circle")]
def navmesh_find_polys_in_circle(sx, sy, sz : float; radius : float = 3f)
  debug_polys_around_point(sx, sy, sz, radius)


[console_cmd(name="path.find_polys_around_camera")]
def navmesh_find_polys_around_camera(radius : float = 3f)
  find_query() <| $ [es] (camera__active : bool; transform : float3x4)
    if camera__active
      debug_polys_around_point(transform[3].x, transform[3].y, transform[3].z, radius)
    return camera__active


[console_cmd(name="path.set_poly_area")]
def navmesh_set_poly_area(sx, sy, sz : float; area : uint = 0u; horz_dist : float = 1f)
  var res : dtPolyRef
  var pos = float3(sx, sy, sz)
  let extents = float3(0.1, 2.0, 0.1)
  var success = false
  if project_to_nearest_navmesh_point(pos, extents, res)
    success = set_poly_area(res, uint8(area))
  console_print("set_poly_area({res}, {uint8(area)}) == {success}")


[console_cmd(name="nmesh.poly_info")]
def draw_poly_info()
  let found = find_query() <| $ [es(REQUIRE=nmeshPolyDebug)] (eid : EntityId)
    destroyEntity(eid)
    return true
  if !found
    createEntity("nmesh_poly_debug")


[es(tag=render, no_order, REQUIRE=nmeshPolyDebug)]
def draw_poly_info_es(evt : UpdateStageInfoRenderDebug)
  var tracePos : float3
  var traceDir : float3

  let cameraFound = find_query() <| $ [es(REQUIRE=camera_view)] (transform : float3x4; camera__active : bool = true)
    if camera__active
      traceDir = transform[2]
      tracePos = transform[3]
      return true
    return false

  if !cameraFound
    return

  var len = 4000.
  var norm : float3
  var color = E3DCOLOR(0xFFFFFFFF)
  var matId = PHYSMAT_INVALID
  var riDesc = RendInstDesc()
  let res = traceray_normalized(tracePos, traceDir, len, matId, norm, ETF_ALL, riDesc, -1)
  let polyPos = tracePos + traceDir * (res ? len : 4000.)
  var triangle : NavMeshTriangle
  let polyFound = get_triangle_by_pos(polyPos, 0.5, triangle)
  var desc = "n/a"
  if polyFound
    desc = "polyId : {int64(triangle.polyRef)}"
    var polyArea : uint8
    get_poly_area(triangle.polyRef, polyArea)
    desc += " area : {int(polyArea)}"
    var polyFlags : uint16
    get_poly_flags(triangle.polyRef, polyFlags)
    desc += " flags : {polyFlags}"

    color = E3DCOLOR(0xFF00FF00)
    begin_draw_cached_debug_lines(false, false, false)
    draw_cached_debug_line(triangle.p0, triangle.p1, E3DCOLOR(0xFF00FF00))
    draw_cached_debug_line(triangle.p1, triangle.p2, E3DCOLOR(0xFF00FF00))
    draw_cached_debug_line(triangle.p2, triangle.p0, E3DCOLOR(0xFF00FF00))
    let polyCenter = (triangle.p0 + triangle.p1 + triangle.p2) / 3.f
    draw_cached_debug_line(polyCenter, triangle.p0, E3DCOLOR(0xFF00FF00))
    draw_cached_debug_line(polyCenter, triangle.p1, E3DCOLOR(0xFF00FF00))
    draw_cached_debug_line(polyCenter, triangle.p2, E3DCOLOR(0xFF00FF00))
    end_draw_cached_debug_lines()
    print("{desc} ({triangle.p0}, {triangle.p1}, {triangle.p2})")

  add_debug_text_mark(polyPos, desc, -1, 0., color)


[console_cmd(name="nmesh.is_valid_poly_ref")]
def is_valid_polyref_test(id : uint64)
  let pos = float3()
  let extents = float3(0.5)
  using(pos, pos, extents) <| $(var req : FindRequest#)
    req.startPoly = id
    let res = is_valid_poly_ref(req)
    console_print("is_valid_poly_ref({id}) == {res}")

[console_cmd(name="nmesh.navmesh_is_valid_poly_ref")]
def navmesh_is_valid_polyref_test(id : uint64)
  let res = navmesh_is_valid_poly_ref(id)
  console_print("navmesh_is_valid_poly_ref({id}) == {res}")


[console_cmd(name="path.move_along_surface")]
def move_along_surface_test(sx, sy, sz, ex, ey, ez : float; startPoly = 0ul; endPoly = 0ul)
  let start = float3(sx, sy, sz)
  let end = float3(ex, ey, ez)
  let extents = float3(0.5)
  using(start, end, extents) <| $(var req : FindRequest#)
    req.startPoly = dtPolyRef(startPoly)
    req.endPoly = dtPolyRef(endPoly)
    let res = move_along_surface(req)
    let distToStart = distance(start, req.end)
    let distToEnd = distance(end, req.end)
    console_print("move_along_surface({start}, {end}) == {res} @{req.end} distToStart:{distToStart} distToEnd:{distToEnd} endPoly:{req.endPoly}")


//Note: dtPolyRef referes to poly, not a triangle. Sometimes they coincide. But often they don't.
[console_cmd(name="nmesh.draw_triangle")]
def console_draw_triangle(id : dtPolyRef)
  let duration = 7000
  var res : NavMeshTriangle
  var polyFlags = uint16(0)
  get_poly_flags(id, polyFlags)
  get_triangle_by_poly(id, res)
  if (int(polyFlags) & int(PolyFlag POLYFLAG_JUMP)) != 0
    draw_debug_line_buffered(res.p0, res.p1, E3DCOLOR(0xff5555ff), duration * 3)
    draw_debug_circle_buffered(res.p0, normalize(res.p1 - res.p0), 0.05, E3DCOLOR(0xff7777ff), 3, duration * 3)
    draw_debug_circle_buffered(res.p1, normalize(res.p1 - res.p0), 0.05, E3DCOLOR(0xff3333ff), 3, duration * 3)
  else
    draw_debug_triangle_buffered(res.p0, res.p1, res.p2, E3DCOLOR(0x770000ff), duration)


[console_cmd(name="nmesh.debug_nmesh_connections_around_camera")]
def console_debug_nmesh_connections_around_camera(depth : int = 3)
  clear_buffered_debug_lines()
  clear_buffered_debug_solids()
  query() <| $ [es(REQUIRE=buffered_text_mark__pos)] (eid : EntityId)
    destroyEntity(eid)
  find_query() <| $ [es] (camera__active : bool; transform : float3x4)
    if camera__active
      console_debug_nmesh_connections(transform[3].x, transform[3].y, transform[3].z, depth)
    return camera__active


[console_cmd(name="nmesh.debug_nmesh_connections")]
def console_debug_nmesh_connections(x, y, z : float; depth : int = 3)
  clear_buffered_debug_lines()
  clear_buffered_debug_solids()
  query() <| $ [es(REQUIRE=buffered_text_mark__pos)] (eid : EntityId)
    destroyEntity(eid)


  iterate_over_polys_recursively(float3(x, y, z), depth, float3(0.4, 1.0, 0.4)) <| $(thisPoly : dtPolyRef)
    var res : NavMeshTriangle
    var polyFlags = uint16(0)
    let elevation = float3(.0, 3.0, .0)
    let elevation2 = float3(.0, 3.005, .0)
    let duration = 7000
    get_triangle_by_poly(thisPoly, res)
    get_poly_flags(thisPoly, polyFlags)
    let center = get_poly_center(polyFlags, res)
    let thisPolyIsJumplink = (int(polyFlags) & int(PolyFlag POLYFLAG_JUMP)) != 0
    if !thisPolyIsJumplink && res.p0 != res.p1 && res.p1 != res.p2 && res.p2 != res.p0
      draw_diminished_triangle(res.p0 + elevation, res.p1 + elevation, res.p2 + elevation, duration)
      draw_debug_text_mark_buffered(center + elevation, "{thisPoly}", duration)
    else
      draw_debug_line_buffered(res.p0 + elevation, res.p1 + elevation, E3DCOLOR(0xff5555ff), duration * 3)
      draw_debug_circle_buffered(res.p0 + elevation, normalize(res.p1 - res.p0), 0.05, E3DCOLOR(0xff7777ff), 3, duration * 3)
      draw_debug_circle_buffered(res.p1 + elevation, normalize(res.p1 - res.p0), 0.05, E3DCOLOR(0xff3333ff), 3, duration * 3)
      draw_debug_text_mark_buffered(center + elevation, "({thisPoly})", duration)
    for_each_linked_poly(thisPoly) <| $(linkedPolyRef)
      get_poly_flags(linkedPolyRef, polyFlags)
      get_triangle_by_poly(linkedPolyRef, res)
      let linkedCenter = get_poly_center(polyFlags, res)
      draw_debug_arrow_buffered(center + elevation2, linkedCenter + elevation2, E3DCOLOR(0xffffffff), duration * 3)
      print("poly_connections: link {thisPoly} -> {linkedPolyRef}")
    return false


def draw_diminished_triangle(p1, p2, p3 : float3; duration : int)
  let center = (p1 + p2 + p3) / 3.0
  let c2p1l = length(p1 - center)
  let c2p2l = length(p2 - center)
  let c2p3l = length(p3 - center)
  let dp1 = center + (p1 - center) / c2p1l * (c2p1l - 0.05)
  let dp2 = center + (p2 - center) / c2p2l * (c2p2l - 0.05)
  let dp3 = center + (p3 - center) / c2p3l * (c2p3l - 0.05)
  draw_debug_triangle_buffered(dp1, dp2, dp3, E3DCOLOR(0x770000ff), duration)


def get_poly_center(flags : uint16; triangle : NavMeshTriangle)
  if (int(flags) & int(PolyFlag POLYFLAG_JUMP)) == 0
    return (triangle.p0 + triangle.p1 + triangle.p2) / 3.0
  else
    return (triangle.p0 + triangle.p1) / 2.0


[console_cmd(name="nmesh.nmesh_info_around_camera")]
def console_nmesh_info_around_camera()
  clear_buffered_debug_lines()
  clear_buffered_debug_solids()
  query() <| $ [es(REQUIRE=buffered_text_mark__pos)] (eid : EntityId)
    destroyEntity(eid)
  var pos = float3()
  find_query() <| $ [es] (camera__active : bool; transform : float3x4)
    if camera__active
      pos = transform[3]
    return camera__active

  iterate_over_polys_recursively(pos, 4, float3(0.4, 1.0, 0.4)) <| $(thisPoly : dtPolyRef)
    var res : NavMeshTriangle
    var polyFlags = uint16(0)
    var area : uint8
    let elevation = float3(.0, 3.0, .0)
    let duration = 7000
    get_triangle_by_poly(thisPoly, res)
    get_poly_flags(thisPoly, polyFlags)
    get_poly_area(thisPoly, area)
    let center = get_poly_center(polyFlags, res)
    draw_debug_text_mark_buffered(center + elevation, "({thisPoly}) F:{polyFlags} A:{area}", duration)
    let thisPolyIsJumplink = (int(polyFlags) & int(PolyFlag POLYFLAG_JUMP)) != 0
    if !thisPolyIsJumplink && res.p0 != res.p1 && res.p1 != res.p2 && res.p2 != res.p0
      draw_diminished_triangle(res.p0 + elevation, res.p1 + elevation, res.p2 + elevation, duration)
    else
      draw_debug_line_buffered(res.p0 + elevation, res.p1 + elevation, E3DCOLOR(0xff5555ff), duration * 3)
      draw_debug_circle_buffered(res.p0 + elevation, normalize(res.p1 - res.p0), 0.05, E3DCOLOR(0xff7777ff), 3, duration * 3)
      draw_debug_circle_buffered(res.p1 + elevation, normalize(res.p1 - res.p0), 0.05, E3DCOLOR(0xff3333ff), 3, duration * 3)
    return false


