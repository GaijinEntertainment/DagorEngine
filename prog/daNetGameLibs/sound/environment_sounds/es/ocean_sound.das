require ecs
require soundEvent
require soundSystem
require ecs.common
require DagorMath
require math.base
require strings
require gameObject

/*
// first spline, linestrip
p:p3=40,10,0 // segment start: p.x, p.z, segment length (computed on init)
p:p3=50,20,0
// ...
p:p3=40,10,0 // same as first point in spline
// next spline
// ...
*/



def update(pt : float2;
           ocean_sound__points : Point3List;

           var ocean_sound_update__idx : int&;
           var ocean_sound_update__firstSplinePointIdx : int&;
           var ocean_sound_update__numIntersections : int&;

           var ocean_sound_update__closestPtOnSpline : float2&;
           var ocean_sound_update__closestPtOnSplineInited : bool&;

           var ocean_sound_update__closestPtOnShore : float2&;
           var ocean_sound_update__closestPtOnShoreInited : bool&;

           var ocean_sound__closestShorePt : float2&;
           var ocean_sound__closestPtOnSpline : float2&;
           var ocean_sound__isInside : bool&)

  if ocean_sound_update__idx < 0 // restart
    ocean_sound_update__idx = 0
    ocean_sound_update__firstSplinePointIdx = 0
    ocean_sound_update__numIntersections = 0
    ocean_sound_update__closestPtOnSplineInited = false
    ocean_sound_update__closestPtOnShoreInited = false

  let numPoints = length(ocean_sound__points)
  let maxLookupPoints = clamp(numPoints / 8, 8, 32)

  for _n in range(maxLookupPoints)

    if ++ocean_sound_update__idx >= numPoints
      // no more splines
      ocean_sound__isInside = false
      ocean_sound__closestPtOnSpline = ocean_sound_update__closestPtOnShore
      ocean_sound__closestShorePt = ocean_sound_update__closestPtOnShore
      ocean_sound_update__idx = -1 // restart
      return

    let a = ocean_sound__points[ocean_sound_update__idx - 1]
    let b = ocean_sound__points[ocean_sound_update__idx]

    let p0 = a.xy
    let p1 = b.xy

    let len = a.z
    if len > 0.1
      let dir = (p1 - p0) / len
      let res = p0 + dir * clamp(dot(dir, pt - p0), 0., len)
      if !ocean_sound_update__closestPtOnSplineInited || length_sq(res - pt) < length_sq(ocean_sound_update__closestPtOnSpline - pt)
        ocean_sound_update__closestPtOnSpline = res
      ocean_sound_update__closestPtOnSplineInited = true

      if !ocean_sound_update__closestPtOnShoreInited || length_sq(res - pt) < length_sq(ocean_sound_update__closestPtOnShore - pt)
        ocean_sound_update__closestPtOnShore = res
      ocean_sound_update__closestPtOnShoreInited = true

    if a.y < b.y
      if pt.y >= a.y && pt.y < b.y
        if cvt(pt.y, a.y, b.y, a.x, b.x) > pt.x
          ++ocean_sound_update__numIntersections
    elif a.y > b.y
      if pt.y >= b.y && pt.y < a.y
        if cvt(pt.y, b.y, a.y, b.x, a.x) > pt.x
          ++ocean_sound_update__numIntersections

    if b.xy == ocean_sound__points[ocean_sound_update__firstSplinePointIdx].xy
      // end of spline
      if (ocean_sound_update__numIntersections & 1) != 0
        ocean_sound__isInside = true
        ocean_sound__closestPtOnSpline = ocean_sound_update__closestPtOnSpline
        ocean_sound__closestShorePt = ocean_sound_update__closestPtOnShore
        ocean_sound_update__idx = -1 // restart
        return

      // next spline
      ocean_sound_update__numIntersections = 0
      ocean_sound_update__firstSplinePointIdx = ++ocean_sound_update__idx
      ocean_sound_update__closestPtOnSplineInited = false



[es(tag=sound, on_event=ParallelUpdateFrameDelayed, after=sound_begin_update_es, before=sound_end_update_es)]
def ocean_sound_update(evt : Event;
                       sound_banks_state__isPresetLoaded : bool;
                       var ocean_sound__event : SoundEvent&;
                       ocean_sound__path : string;
                       ocean_sound__range : float;
                       ocean_sound__points : Point3List;
                       ocean_sound__waterLevel : float;
                       ocean_sound__shoreVar : string;
                       ocean_sound__playSoundInside : bool;

                       ocean_sound__shoreDistanceToVar : float4;

                       var ocean_sound_update__idx : int&;
                       var ocean_sound_update__firstSplinePointIdx : int&;
                       var ocean_sound_update__numIntersections : int&;
                       var ocean_sound_update__shoreParam : float&;

                       var ocean_sound_update__closestPtOnSpline : float2&;
                       var ocean_sound_update__closestPtOnSplineInited : bool&;

                       var ocean_sound_update__closestPtOnShore : float2&;
                       var ocean_sound_update__closestPtOnShoreInited : bool&;

                       var ocean_sound__closestShorePt : float2&;
                       var ocean_sound__closestPtOnSpline : float2&;
                       var ocean_sound__isInside : bool&)

  if !sound_banks_state__isPresetLoaded
    return

  let listener3d = get_listener_pos()
  let listener = listener3d.xz
  update(listener, ocean_sound__points, ocean_sound_update__idx, ocean_sound_update__firstSplinePointIdx, ocean_sound_update__numIntersections, ocean_sound_update__closestPtOnSpline, ocean_sound_update__closestPtOnSplineInited, ocean_sound_update__closestPtOnShore, ocean_sound_update__closestPtOnShoreInited, ocean_sound__closestShorePt, ocean_sound__closestPtOnSpline, ocean_sound__isInside)

  var eventPos : float3
  var shoreParam = 1.

  if ocean_sound__playSoundInside
    if ocean_sound__isInside // above water
      eventPos = float3(listener.x, ocean_sound__waterLevel, listener.y)
      shoreParam = cvt(distance(listener, ocean_sound__closestShorePt), ocean_sound__shoreDistanceToVar.x, ocean_sound__shoreDistanceToVar.y, ocean_sound__shoreDistanceToVar.z, ocean_sound__shoreDistanceToVar.w)
    else
      eventPos = float3(ocean_sound__closestShorePt.x, ocean_sound__waterLevel, ocean_sound__closestShorePt.y)
  else
    if ocean_sound__isInside // above land
      eventPos = float3(ocean_sound__closestPtOnSpline.x, ocean_sound__waterLevel, ocean_sound__closestPtOnSpline.y)
    else
      eventPos = float3(listener.x, ocean_sound__waterLevel, listener.y)
      shoreParam = cvt(distance(listener, ocean_sound__closestShorePt), ocean_sound__shoreDistanceToVar.x, ocean_sound__shoreDistanceToVar.y, ocean_sound__shoreDistanceToVar.z, ocean_sound__shoreDistanceToVar.w)

  let enable = length_sq(eventPos) != 0. && (ocean_sound__event.enabled ? (distance_sq(eventPos, listener3d) < square(ocean_sound__range)) :
                                                                           distance_sq(eventPos, listener3d) < square(ocean_sound__range * 0.9))

  if ocean_sound__event.enabled != enable
    ocean_sound__event.enabled = enable
    if enable
      ocean_sound__event |> play(ocean_sound__path, eventPos)
    else
      release(ocean_sound__event)
    ocean_sound_update__shoreParam = -1.

  if enable
    set_pos(ocean_sound__event, eventPos)
    if !empty(ocean_sound__shoreVar) && ocean_sound_update__shoreParam != shoreParam
      ocean_sound_update__shoreParam = shoreParam
      set_var(ocean_sound__event, ocean_sound__shoreVar, shoreParam)



[es(tag=sound, track=sound_banks_state__isPresetLoaded)]
def ocean_sound_toggle(evt : Event;
                       var ocean_sound__event : SoundEvent&;
                       var ocean_sound_update__idx : int&;
                       var ocean_sound__range : float&;
                       ocean_sound__path : string;
                       sound_banks_state__isPresetLoaded : bool)
  release_immediate(ocean_sound__event)
  ocean_sound__event.enabled = false
  ocean_sound_update__idx = -1
  if sound_banks_state__isPresetLoaded
    ocean_sound__range = get_max_distance(ocean_sound__path)


def init_points(var ocean_sound__points : Point3List&)
  var idx = 0
  let numPoints = length(ocean_sound__points)
  while ++idx < numPoints
    ocean_sound__points[idx - 1].z = distance(ocean_sound__points[idx - 1], ocean_sound__points[idx])


[es(tag=sound, on_appear, REQUIRE_NOT=game_object_ocean_sound_generator__gameObjects)]
def ocean_sound_appear(evt : Event;
                       var ocean_sound__points : Point3List&;
                       var ocean_sound_update__idx : int&)
  init_points(ocean_sound__points)
  ocean_sound_update__idx = -1


[es(tag=sound, on_appear, on_event=EventGameObjectsCreated)]
def game_object_ocean_sound_generator(evt : Event;
                                      game_object_ocean_sound_generator__gameObjects : StringList;
                                      var ocean_sound__event : SoundEvent&;
                                      var ocean_sound__points : Point3List&;
                                      var ocean_sound_update__idx : int&)

  release_immediate(ocean_sound__event)
  ocean_sound__event.enabled = false
  ocean_sound_update__idx = -1

  assume points = ocean_sound__points
  clear(points)

  find_query() <| $ [es] (game_objects : GameObjects)
    for gameObject in game_object_ocean_sound_generator__gameObjects
      let scene = game_objects |> get_scene_game_objects_by_name(string(gameObject))
      if scene != null
        let firstPointIdx = length(points)
        scene |> find_scene_game_objects() <| $ [unused_argument(ni)] (ni : uint; tm : float3x4#)
          points |> push(float3(tm[3].x, tm[3].z, 0.))
          return false
        if length(points) > firstPointIdx
          let pt = points[firstPointIdx]
          points |> push(pt)
    return true

  if !empty(points)
    sound_debug("generated {length(points)} ocean sound points using gameobjects {game_object_ocean_sound_generator__gameObjects}")

  init_points(points)
