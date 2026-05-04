require math.base
require ecs
require ecs.safe
require DagorDataBlock
require DagorRandom
require DagorMath

def get_rnd(r : float2; var seed : int&) : float
  return r.y != 0.0 ? r.x + _srnd(seed) * r.y : r.x

def get_node_local_tm(node : Object; var seed : int&) : float3x4
  let explicitTm = get_TMatrix(node, "localTransform")
  if explicitTm != null
    return *explicitTm

  let ang_x = get_rnd(get_Point2(node, "rot_x") ?? float2(0.0, 0.0), seed) |> deg_to_rad
  let ang_y = get_rnd(get_Point2(node, "rot_y") ?? float2(0.0, 0.0), seed) |> deg_to_rad
  let ang_z = get_rnd(get_Point2(node, "rot_z") ?? float2(0.0, 0.0), seed) |> deg_to_rad
  let off_x = get_rnd(get_Point2(node, "offset_x") ?? float2(0.0, 0.0), seed)
  let off_y = get_rnd(get_Point2(node, "offset_y") ?? float2(0.0, 0.0), seed)
  let off_z = get_rnd(get_Point2(node, "offset_z") ?? float2(0.0, 0.0), seed)
  let scaleVal  = max(get_rnd(get_Point2(node, "scale")  ?? float2(1.0, 0.0), seed), 0.0)
  let xScaleVal = max(get_rnd(get_Point2(node, "xScale") ?? float2(1.0, 0.0), seed), 0.0)
  let yScaleVal = max(get_rnd(get_Point2(node, "yScale") ?? float2(1.0, 0.0), seed), 0.0)

  var rx, ry, rz : float3x4
  rotxTM(ang_x, rx); rotyTM(ang_y, ry); rotzTM(ang_z, rz)

  var lTm = ry * rz * rx
  lTm[3] = float3(off_x, off_y, off_z)
  lTm[0] *= scaleVal * xScaleVal
  lTm[1] *= scaleVal * yScaleVal
  lTm[2] *= scaleVal
  return lTm

def spawn_nodes(nodes : Array;
                accumulatedTm : float3x4;
                parentTm : float3x4;
                tmFromEntityParent : float3x4;
                parentEntityIdx : int;
                var subentityEids : EidList;
                var subentityTms : TMatrixList;
                var subentityLocalTms : TMatrixList;
                var subentityParentIdx : IntList;
                var subentityNames : StringList;
                var seed : int&;
                child_euler_map : Object)
  for it in nodes
    let node = get_ecs_object(it)

    var lTm = get_node_local_tm(*node, seed)
    let nodeName = get_string(*node, "name", "")
    if nodeName != ""
      let eulerAngles = get_Point3(child_euler_map, nodeName)
      if eulerAngles != null
        let angles = *eulerAngles
        let ax = angles.x |> deg_to_rad
        let ay = angles.y |> deg_to_rad
        let az = angles.z |> deg_to_rad
        var rx, ry, rz : float3x4
        rotxTM(ax, rx); rotyTM(ay, ry); rotzTM(az, rz)
        var eulerTm = ry * rz * rx
        eulerTm[3] = lTm[3]
        lTm = eulerTm
    let localTm = accumulatedTm * lTm
    let localFromEntityParent = tmFromEntityParent * lTm
    let tm = parentTm * localTm
    let select_one = get_ecs_array(node, "select_one")

    var newParentEntityIdx = parentEntityIdx
    var newTmFromEntityParent = localFromEntityParent

    if select_one != null
      var total_weight = 0.0
      for entity_it in *select_one
        total_weight += get_float(get_ecs_object(entity_it), "gen__weight") ?? 0.0

      if total_weight > 0.0
        var rnd = (_frnd(seed) * total_weight) % total_weight
        for entity_it in *select_one
          let entity_variant = get_ecs_object(entity_it)
          rnd -= get_float(entity_variant, "gen__weight") ?? 0.0
          if rnd <= 0.001
            let template_name = get_string(entity_variant, "gen__template", "")
            if template_name != ""
              let subentityEid = createEntity(template_name) <| $(var init)
                init |> set("transform", tm)
                for o in *entity_variant
                  init |> set(o.key, o.value)
              subentityEids |> push(subentityEid)
              subentityTms |> push(localTm)
              subentityLocalTms |> push(localFromEntityParent)
              subentityParentIdx |> push(parentEntityIdx)
              subentityNames |> push(nodeName)
              newParentEntityIdx = length(subentityEids) - 1
              newTmFromEntityParent = IDENT_TM
            break

    let nested = get_ecs_array(node, "nested")
    if nested != null
      spawn_nodes(*nested, localTm, parentTm, newTmFromEntityParent, newParentEntityIdx,
                  subentityEids, subentityTms, subentityLocalTms, subentityParentIdx, subentityNames, seed, child_euler_map)


[es(tag=server, on_appear)]
def composite_entity_created_es(evt : Event; @shared_comp nodes : Array; transform : float3x4;
                                var subentityEids : EidList;
                                var subentityTms : TMatrixList;
                                var subentityLocalTms : TMatrixList;
                                var subentityParentIdx : IntList;
                                var subentityNames : StringList;
                                var random_seed : int&;
                                child_euler_map : Object)
  if (random_seed == 0)
    random_seed = grnd()
  var seed = random_seed
  spawn_nodes(nodes, IDENT_TM, transform, IDENT_TM, -1,
              subentityEids, subentityTms, subentityLocalTms, subentityParentIdx, subentityNames, seed, child_euler_map)

[es(tag=server, track=transform)]
def composite_entity_transform_change_es(evt : Event; transform : float3x4; subentityEids : EidList; subentityTms : TMatrixList)
  let parentTM = transform
  for i, subentityEid in iter_range(subentityEids), subentityEids
    query(subentityEid) <| $ [es] (var transform : float3x4)
      transform = parentTM * subentityTms[i]

[es(tag=server, on_disappear)]
def composite_entity_removed_es(evt : Event; subentityEids : EidList)
  for subentityEid in subentityEids
    destroyEntity(subentityEid)
