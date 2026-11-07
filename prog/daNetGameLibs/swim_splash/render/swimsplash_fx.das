require ecs
require math.base
require debug.logerr_once
require AnimV20
require GeomNodeTree
require CapsuleApproximation
require Dacoll
require DngCamera
require gameLibs.pufd_events.pufd_events


[es(tag=render, on_appear)]
def swim_spash_fx_on_appear(evt : Event;
                            @shared_comp capsule_approximation : CapsuleApproximation;
                            animchar : AnimcharBaseComponent;
                            var swimsplash__nodeIds : IntList;
                            var swimsplash__prevPositions : Point3List;
                            var swimsplash__timeSinceLastSplashes : FloatList)
  reserve(swimsplash__prevPositions, length(capsule_approximation.capsuleDatas) + 1) // reserve bytes for safe v_ldu
  for capsule in capsule_approximation.capsuleDatas
    push(swimsplash__nodeIds, capsule.nodeIndex)
    push(swimsplash__prevPositions, geomtree_getNodeWpos(*animchar.nodeTree, capsule.nodeIndex))
    push(swimsplash__timeSinceLastSplashes, 100000.0)


def update_trace(curr_pos : float3;
                 dt : float;
                 min_t : float;
                 var prev_pos : float3&;
                 var time_since_last_splash : float&;
                 var intersect : float3&;
                 var speed : float&)
  time_since_last_splash += dt

  if time_since_last_splash < min_t
    return false

  let deltaPos = curr_pos - prev_pos

  var prevHeight = 0.0
  var currHeight = 0.0
  var doSwimSplash = (dt > 1e-5 &&
                      abs(deltaPos.y) > 1e-5 &&
                      traceht_water(prev_pos, prevHeight) &&
                      traceht_water(curr_pos, currHeight))

  prevHeight = prev_pos.y - prevHeight
  currHeight = curr_pos.y - currHeight
  doSwimSplash = doSwimSplash && sign(prevHeight) != sign(currHeight)

  if doSwimSplash
    speed = length(deltaPos) / time_since_last_splash
    intersect = curr_pos - float3(0.0, currHeight, 0.0)

  time_since_last_splash = 0.0
  prev_pos = curr_pos

  return doSwimSplash


[es(tag=render, REQUIRE_NOT=deadEntity, after=start_async_phys_sim_es)]
def swimsplash_on_update(evt : ParallelUpdateFrameDelayed;
                         transform : float3x4;
                         animchar : AnimcharBaseComponent;
                         swimsplash__offset : float3;
                         swimsplash__scaleMul : float;
                         swimsplash__scaleFx : bool;
                         swimsplash__fxTemplate : string;
                         swimsplash__maxRenderingSpeed : float;
                         swimsplash__minTimeBetweenSplashes : float;
                         swimsplash__maxDistanceFromCameraToUpdateSq : float;
                         animchar__visible : bool = true;
                         isInVehicle : bool = false;
                         var swimsplash__nodeIds : IntList;
                         var swimsplash__prevPositions : Point3List;
                         var swimsplash__timeSinceLastSplashes : FloatList)
  if !animchar__visible || isInVehicle
    return

  if swimsplash__maxDistanceFromCameraToUpdateSq > 0.0
    let camPos = (get_cur_cam_entity() |> get_TMatrix("transform") ?? IDENT_TM)[3]
    let distSqToCam = distance_sq(camPos, transform[3])
    if distSqToCam > swimsplash__maxDistanceFromCameraToUpdateSq
      return

  for nodeId, prevPos, timeSinceLastSplash in swimsplash__nodeIds, swimsplash__prevPositions, swimsplash__timeSinceLastSplashes
    let wpos = geomtree_getNodeWpos(*animchar.nodeTree, nodeId)

    if is_nan(wpos) || length_sq(wpos) >= 1e10
      let nodeName = geomtree_getNodeName(*animchar.nodeTree, nodeId)
      let animcharResName = animchar_get_res_name(animchar)
      logerr_once("Bad node {nodeId}({nodeName}) pos={wpos} of animchar '{animcharResName}'")
      continue

    var intersect : float3
    var speed : float
    if update_trace(wpos, evt.dt, swimsplash__minTimeBetweenSplashes, prevPos, timeSinceLastSplash, intersect, speed)
      if speed >= swimsplash__maxRenderingSpeed
        return

      let scale = pow(speed, 2.0 / 3.0) * swimsplash__scaleMul // energy of impact scales with speed^2 and water mass with scale^3
      let worldPos = intersect + swimsplash__offset * scale

      var tm = IDENT_TM
      if swimsplash__scaleFx // constraint: effect entities take the fx scale from the transform axis length
        tm[0] *= scale
        tm[1] *= scale
        tm[2] *= scale
      tm[3] = worldPos

      createEntity(swimsplash__fxTemplate) <| $ [es] (init)
        set(init, "transform", tm)
