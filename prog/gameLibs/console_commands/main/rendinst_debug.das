options no_aot
require ecs
require strings
require math.base
require DagorMath
require DagorConsole
require DagorDebug3D
require RendInst

[console_cmd(name="ri.highlight", hint="draw the bounding boxes of every rendinst with that name around the camera; a trailing * matches a prefix")]
def ri_highlight(name : string; radius : float = 100.0; frames : int = 600)
  var camPos = float3()
  let hasCamera = find_query() <| $ [es] (camera__active : bool; transform aka camera_transform : float3x4)
    if camera__active
      camPos = camera_transform[3]
    return camera__active
  if !hasCamera
    console_print("ri.highlight: no active camera")
    return

  let byPrefix = ends_with(name, "*")
  let prefix = byPrefix ? slice(name, 0, length(name) - 1) : ""
  var count = 0
  let maxCallbacks = setMaxNumRiCollisionCb(500000u)
  rendinst_foreachInSphere(BSphere3(camPos, radius), int(GatherRiTypeFlags.RiGenAndExtra)) <| $ [unused_argument(is_tm)] (desc : RendInstDesc; tm : float3x4; is_tm : bool)
    let resName = getRIGenResName(desc)
    let matches = byPrefix ? (empty(prefix) || starts_with(resName, prefix)) : resName == name
    if !matches
      return
    draw_debug_box_buffered(getRIGenBBox(desc), tm, E3DCOLOR(0xFF00FF00), frames)
    count++
  setMaxNumRiCollisionCb(maxCallbacks)
  console_print("ri.highlight: {count} x {name} within {radius}m")
