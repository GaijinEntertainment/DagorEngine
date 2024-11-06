require ecs
require app
require math
require math.base
require soundEvent
require soundSystem
require DagorMath
require Dacoll
require DngCamera
require strings
require gameObject


[es(tag=sound, on_event=ParallelUpdateFrameDelayed, after=sound_begin_update_es, before=sound_end_update_es)]
def spline_sound_update(evt : Event;
                        sound_banks_state__isPresetLoaded : bool;
                        spline_sound__path : string;
                        spline_sound__points : Point4List;
                        spline_sound__range : float;
                        spline_sound__floorOnWater : bool;
                        spline_sound__distToSoundSize : float4&;
                        spline_sound__soundSizeVar : string;

                        var spline_sound_update__listener : float3&;
                        var spline_sound_update__idx : int&;
                        var spline_sound_update__closestPt : float3&;
                        var spline_sound_update__closestPtValid : bool&;
                        var spline_sound_update__splineClosestPt : float3&;
                        var spline_sound_update__splineClosestPtValid : bool&;
                        var spline_sound_update__isInside : bool&;
                        var spline_sound_update__midPt : float3&;
                        var spline_sound_update__weight : float&;

                        var spline_sound__eventPos : float3&;
                        var spline_sound__isInside : bool&;

                        var spline_sound__event : SoundEvent&)
  if !sound_banks_state__isPresetLoaded || length(spline_sound__points) < 2
    return

  let pointsToUpdate = clamp(length(spline_sound__points) / 8, 1, 16)
  let nextPoint = min(spline_sound_update__idx + pointsToUpdate, length(spline_sound__points) - 1)
  assume listener = spline_sound_update__listener

  if spline_sound_update__idx == 0
    listener = (get_cur_cam_entity() |> get_TMatrix("transform") ?? IDENT_TM)[3]

  while spline_sound_update__idx < nextPoint
    let p0 = spline_sound__points[spline_sound_update__idx]
    let p1 = spline_sound__points[spline_sound_update__idx + 1]
    ++spline_sound_update__idx

    if p1.w < 0. // next spline
      ++spline_sound_update__idx

      if spline_sound_update__splineClosestPtValid && !empty(spline_sound__soundSizeVar)
        let weight = 1. - saturate(safediv(distance(spline_sound_update__splineClosestPt, listener), spline_sound__range))
        spline_sound_update__midPt += spline_sound_update__splineClosestPt * weight
        spline_sound_update__weight += weight
      spline_sound_update__splineClosestPtValid = false
      continue

    // search best point on segment/inside capsule

    let dir = p1.xyz - p0.xyz
    var t = 0.
    let c1 = dot(listener - p0.xyz, dir)
    if c1 > FLT_EPSILON
      let c2 = dot(dir, dir)
      t = c1 + FLT_EPSILON < c2 ? c1 / c2 : 1.
    let radius = lerp(p0.w, p1.w, t)
    var p = p0.xyz + dir * t

    var isInside = false
    if radius > 0.
      if distance_sq(p, listener) >= square(spline_sound__range + radius)
        continue

      if spline_sound__floorOnWater
        let diff = listener.xz - p.xz
        isInside = length_sq(diff) < square(radius)
        if isInside
          p = listener
          traceht_water(p, p.y)
        else
          traceht_water(p, p.y)
          let offset = normalize(diff) * radius
          p.x += offset.x
          p.z += offset.y
      else
        isInside = distance_sq(listener, p) < square(radius)
        if isInside
          p = listener
        else
          p += normalize(listener - p) * radius

    let distSq = distance_sq(p, listener)
    if distSq < square(spline_sound__range)

      if !spline_sound_update__splineClosestPtValid || distSq < distance_sq(spline_sound_update__splineClosestPt, listener)
        spline_sound_update__splineClosestPt = p
        spline_sound_update__splineClosestPtValid = true

      if !spline_sound_update__closestPtValid || distSq < distance_sq(spline_sound_update__closestPt, listener)
        spline_sound_update__closestPt = p
        spline_sound_update__closestPtValid = true
        spline_sound_update__isInside = isInside


  if spline_sound_update__idx + 1 >= length(spline_sound__points) // end
    let enable = spline_sound_update__closestPtValid && (spline_sound__event.enabled ? (distance_sq(spline_sound_update__closestPt, listener) < square(spline_sound__range)) :
                                                                                       (distance_sq(spline_sound_update__closestPt, listener) < square(spline_sound__range * 0.9)))
    if spline_sound__event.enabled != enable
      spline_sound__event.enabled = enable
      release_immediate(spline_sound__event)
      if spline_sound__event.enabled
        spline_sound__event |> play(spline_sound__path, spline_sound_update__closestPt)

    if spline_sound__event.enabled
      spline_sound__eventPos = spline_sound_update__closestPt
      spline_sound__isInside = spline_sound_update__isInside
      if !spline_sound__isInside
        set_pos(spline_sound__event, spline_sound_update__closestPt)

      if !empty(spline_sound__soundSizeVar)
        let midPos = spline_sound_update__midPt * safeinv(spline_sound_update__weight)
        assume d2ss = spline_sound__distToSoundSize
        let soundSize = cvt(distance(midPos, spline_sound_update__closestPt), d2ss.x, d2ss.y, d2ss.z, d2ss.w)
        set_var(spline_sound__event, spline_sound__soundSizeVar, soundSize)

    spline_sound_update__idx = 0
    spline_sound_update__closestPtValid = false
    spline_sound_update__splineClosestPtValid = false
    spline_sound_update__isInside = false
    spline_sound_update__midPt = float3(0., 0., 0.)
    spline_sound_update__weight = 0.


  if spline_sound__event.enabled && spline_sound__isInside
    let curListener = get_listener_pos()
    if spline_sound__floorOnWater
      let pos = float3(curListener.x, spline_sound__eventPos.y, curListener.z)
      set_pos(spline_sound__event, pos)
    else
      set_pos(spline_sound__event, curListener)



[es(tag=sound, on_appear, track=sound_banks_state__isPresetLoaded)]
def spline_sound_setup(evt : Event;
                       var spline_sound__event : SoundEvent&;
                       var spline_sound_update__idx : int&;
                       var spline_sound_update__closestPtValid : bool&;
                       var spline_sound_update__splineClosestPtValid : bool&;
                       var spline_sound_update__midPt : float3&;
                       var spline_sound_update__weight : float&;
                       var spline_sound__range : float&;
                       spline_sound__path : string;
                       sound_banks_state__isPresetLoaded : bool)
  release_immediate(spline_sound__event)
  spline_sound__event.enabled = false
  spline_sound_update__idx = 0
  spline_sound_update__closestPtValid = false
  spline_sound_update__splineClosestPtValid = false
  spline_sound_update__midPt = float3(0., 0., 0.)
  spline_sound_update__weight = 0.
  if sound_banks_state__isPresetLoaded
    spline_sound__range = get_max_distance(spline_sound__path)


[es(tag=sound, on_appear, track=sound_banks_state__isPresetLoaded, REQUIRE=sound_banks_state__isPresetLoaded, on_event=EventGameObjectsCreated, after=spline_sound_setup)]
def game_object_spline_sound_generator(evt : Event;
                                       game_object_spline_sound_generator__gameObjects : StringList;
                                       game_object_spline_sound_generator__pointRadiusMul : float;
                                       var spline_sound__points : Point4List&)
  assume points = spline_sound__points
  clear(points)

  find_query() <| $ [es] (game_objects : GameObjects)
    for gameObject in game_object_spline_sound_generator__gameObjects
      let scene = game_objects |> get_scene_game_objects_by_name(string(gameObject))
      if scene != null
        var isFirstPointInSpline = true
        scene |> find_scene_game_objects() <| $ [unused_argument(ni)] (ni : uint; tm : float3x4#)
          points |> reserve(16)
          if isFirstPointInSpline && !empty(points)
            points |> push(float4(0., 0., 0., -1.)) // separator for a new spline
          isFirstPointInSpline = false
          let pos = tm[3]
          let radius = length(tm[0]) * game_object_spline_sound_generator__pointRadiusMul
          points |> push(float4(pos.x, pos.y, pos.z, radius))
          return false
    return true

  if !empty(points)
    points |> push(float4(0., 0., 0., -1.)) // should end with separator
    sound_debug("generated {length(points)} spline sound points using gameobjects {game_object_spline_sound_generator__gameObjects}")
