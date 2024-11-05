options no_aot
require ecs
require strings
require DagorConsole


[console_cmd(name="app.debug_collision", hint="collision_type: /no argument/, all, phys, trace, canopy")]
def app_debug_collision(collision_type = "default";
                        collision_render_distance = 200.;
                        collision_label_distance = 15.)
  var collType = collision_type
  let collDistSq = collision_render_distance * collision_render_distance
  let labelDistSq = collision_label_distance * collision_label_distance

  var drawAll = false
  var drawCanopy = false
  var drawCollidable = false
  var drawTraceable = false
  if "canopy" |> starts_with(collision_type)
    drawCanopy = true
    collType = "canopy"
  elif "phys" |> starts_with(collision_type)
    drawCollidable = true
    collType = "phys"
  elif "trace" |> starts_with(collision_type)
    drawTraceable = true
    collType = "trace"
  elif "all" |> starts_with(collision_type)
    drawAll = true
    collType = "all"
  elif collision_type == "default"
    drawAll = true
  else
    return

  console_print("debug_collision {collType}")

  let found = find_query() <| $ [es] (eid : EntityId;
                                      var collision_debug__collMaxDistSq : float&;
                                      var collision_debug__labelMaxDistSq : float&;
                                      var collision_debug__drawCanopy : bool&;
                                      var collision_debug__drawCollidable : bool&;
                                      var collision_debug__drawTraceable : bool&)
    if collision_type == "default"
      destroyEntity(eid)
      return true
    collision_debug__collMaxDistSq = collDistSq
    collision_debug__labelMaxDistSq = labelDistSq
    collision_debug__drawCanopy = drawAll || collision_debug__drawCanopy != drawCanopy
    collision_debug__drawCollidable = drawAll || collision_debug__drawCollidable != drawCollidable
    collision_debug__drawTraceable = drawAll || collision_debug__drawTraceable != drawTraceable
    return true

  if !found
    createEntity("collision_debug") <| $(init)
      init |> set("collision_debug__drawCanopy", drawAll || drawCanopy)
      init |> set("collision_debug__drawCollidable", drawAll || drawCollidable)
      init |> set("collision_debug__drawTraceable", drawAll || drawTraceable)
      init |> set("collision_debug__collMaxDistSq", collDistSq)
      init |> set("collision_debug__labelMaxDistSq", labelDistSq)
