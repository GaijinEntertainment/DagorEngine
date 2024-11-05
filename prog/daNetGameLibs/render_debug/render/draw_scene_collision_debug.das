options no_aot
require ecs
require DagorDriver3D
require RendInst
require WorldRenderer

def draw_collision_debug_common(phys : bool;
                                trace : bool;
                                canopy : bool;
                                view_pos : float3;
                                max_coll_dist_sq : float;
                                max_label_dist_sq : float)
  //view mode
  var flags = int(DrawCollisionsFlags Opacity) | int(DrawCollisionsFlags Wireframe)

  //rendist or canopy
  if phys || trace
    flags |= int(DrawCollisionsFlags All)

  if canopy
    flags |= int(DrawCollisionsFlags RendInstCanopy)

  //node filter for rendists
  if trace && !phys
    flags |= int(DrawCollisionsFlags TraceOnly)

  if phys && !trace
    flags |= int(DrawCollisionsFlags PhysOnly)

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

