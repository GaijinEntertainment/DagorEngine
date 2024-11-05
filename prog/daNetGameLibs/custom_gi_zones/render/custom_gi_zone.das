require ecs
require ecs.common
require DagorMath
require DagorSystem
require math.base
require DagorDebug3D


[es(tag=render, no_order, REQUIRE=custom_gi_zone)]
def is_camera_inside_custom_gi_box_updater(info : UpdateStageInfoAct;
                                           transform aka zone_transform : float3x4;
                                           var is_camera_inside_box : bool&
                                           )
  var camPos : float3
  let isCamActive = find_query() <| $ [es] (camera__active : bool; transform : float3x4)
    if camera__active
      camPos = transform[3]
    return camera__active
  if isCamActive
    let zoneBbox = zone_transform * BBox3(float3(), 1.0f)
    is_camera_inside_box = zoneBbox & camPos