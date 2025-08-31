options no_aot
require ecs
require ecs.ecs_template
require DagorConsole
require DagorMath
require DagorDebug3D
require DagorDriver3D
require RendInst
require WorldRenderer

[ecs_template]
struct draw_transparency_trace_debug
  draw_transparency_trace_debug__enabled : bool
  draw_transparency_trace_debug__fromCamera : bool
  draw_transparency_trace_debug__maxDist : float
  draw_transparency_trace_debug__transparencyThreshold : float


def draw_collision_debug_common(phys : bool;
                                trace : bool;
                                canopy : bool;
                                view_pos : float3;
                                max_coll_dist_sq : float;
                                max_label_dist_sq : float)
  //view mode
  var flags = int(DrawCollisionsFlags.Opacity) | int(DrawCollisionsFlags.Wireframe)

  //rendist or canopy
  if phys || trace
    flags |= int(DrawCollisionsFlags.All)

  if canopy
    flags |= int(DrawCollisionsFlags.RendInstCanopy)

  //node filter for rendists
  if trace && !phys
    flags |= int(DrawCollisionsFlags.TraceOnly)

  if phys && !trace
    flags |= int(DrawCollisionsFlags.PhysOnly)

  rendinst_drawDebugCollisions(flags, view_pos, true, max_coll_dist_sq, max_label_dist_sq)

[es(tag=(dev, render), no_order, REQUIRE=collision_debug, on_event=RenderDebugWithJitter)]
def draw_collision_debug(evt : Event;
                         collision_debug__collMaxDistSq : float;
                         collision_debug__labelMaxDistSq : float;
                         collision_debug__drawCanopy : bool;
                         collision_debug__drawCollidable : bool;
                         collision_debug__drawTraceable : bool)

  var cameraPos = float3(0)
  find_query() <| $ [es] (camera__active : bool; transform : float3x4&)
    if !camera__active
      return false
    cameraPos = transform[3]
    return true

  draw_collision_debug_common(collision_debug__drawCollidable, collision_debug__drawTraceable, collision_debug__drawCanopy,
    cameraPos, collision_debug__collMaxDistSq, collision_debug__labelMaxDistSq)


[es(tag=render, no_order)]
def draw_transparency_trace_debug(info : UpdateStageInfoRenderDebug;
                                  draw_transparency_trace_debug__enabled : bool;
                                  draw_transparency_trace_debug__fromCamera : bool;
                                  draw_transparency_trace_debug__maxDist : float;
                                  draw_transparency_trace_debug__transparencyThreshold : float)
  if !draw_transparency_trace_debug__enabled
    return
  using() <| $(var riDesc : RendInstDesc#)
    var start = float3()
    var dir = float3()
    if draw_transparency_trace_debug__fromCamera
      let cameraWasFound = find_query() <| $ [es] (camera__active : bool; transform : float3x4)
        if !camera__active
          return false
        dir = transform[2]
        start = transform[3]
        return true
      if !cameraWasFound
        return
    else
      let heroWasFound = find_query() <| $ [es(REQUIRE=hero)] (human__aimTm : float3x4)
        dir = human__aimTm[0]
        start = human__aimTm[3]
        return true
      if !heroWasFound
        return

    var t = draw_transparency_trace_debug__maxDist
    var transparency = 0.0
    var pmid = -1
    let canopyTraceThreshold = draw_transparency_trace_debug__transparencyThreshold
    let res = traceTransparencyRayRIGenNormalized(start, dir, t, canopyTraceThreshold,
      riDesc, pmid, transparency, -1, /*check_canopy*/ true)
    let end = start + dir * t
    add_debug_text_mark(end, "{t}:{transparency}", /*max len*/ -1, 0.0, E3DCOLOR(0xFF68D3C5))
    draw_debug_sphere_buffered(end, 0.2, E3DCOLOR(0xFF00FF00), 12, 1)
    draw_debug_line_buffered(start, end, E3DCOLOR(0xFF00FF00), 1)


[console_cmd(name="debug.draw_transparency_trace")]
def toggle_draw_transparency_trace_debug(enabled : bool; from_camera : bool = false; max_dist : float = 20.0; transparency_threshold : float = 100.0)
  let alreadyExists = find_query() <| $ [es] (var draw_transparency_trace_debug__enabled : bool&;
                                              var draw_transparency_trace_debug__fromCamera : bool&;
                                              var draw_transparency_trace_debug__maxDist : float&;
                                              var draw_transparency_trace_debug__transparencyThreshold : float&)
    draw_transparency_trace_debug__enabled = enabled
    draw_transparency_trace_debug__fromCamera = from_camera
    draw_transparency_trace_debug__maxDist = max_dist
    draw_transparency_trace_debug__transparencyThreshold = transparency_threshold
    return true
  if !alreadyExists
    createEntity("draw_transparency_trace_debug") <| $(var init)
      init |> set("draw_transparency_trace_debug__enabled", enabled)
      init |> set("draw_transparency_trace_debug__fromCamera", from_camera)
      init |> set("draw_transparency_trace_debug__maxDist", max_dist)
      init |> set("draw_transparency_trace_debug__transparencyThreshold", transparency_threshold)
