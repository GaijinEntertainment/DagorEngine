require math.base
require ecs
require ecs.safe
require DagorMath

// Recursively recompute accumulated TMs for entity at idx and all its descendants,
// then update their world transforms.
def recompute_subtree(idx : int;
                      parentCompositeTm : float3x4;
                      subentityEids : EidList;
                      var subentityTms : TMatrixList;
                      subentityLocalTms : TMatrixList;
                      subentityParentIdx : IntList)
  let parentIdx = subentityParentIdx[idx]
  let parentAccumTm = parentIdx >= 0 ? subentityTms[parentIdx] : IDENT_TM
  subentityTms[idx] = parentAccumTm * subentityLocalTms[idx]
  query(subentityEids[idx]) <| $ [es] (var transform : float3x4)
    transform = parentCompositeTm * subentityTms[idx]
  for i in iter_range(subentityParentIdx)
    if subentityParentIdx[i] == idx
      recompute_subtree(i, parentCompositeTm, subentityEids, subentityTms, subentityLocalTms, subentityParentIdx)

[es(tag=server, track=child_euler_map)]
def composite_child_euler_map_es(evt : Event;
                                 transform : float3x4;
                                 child_euler_map : Object;
                                 subentityEids : EidList;
                                 subentityNames : StringList;
                                 subentityParentIdx : IntList;
                                 var subentityTms : TMatrixList;
                                 var subentityLocalTms : TMatrixList)
  for o in child_euler_map
    var idx = -1
    for i, name in iter_range(subentityNames), subentityNames
      if name == o.key
        idx = i
        break
    if idx < 0
      continue

    let angles = get_Point3(o.value) ?? float3(0.0, 0.0, 0.0)
    let ax = angles.x |> deg_to_rad
    let ay = angles.y |> deg_to_rad
    let az = angles.z |> deg_to_rad

    var rx, ry, rz : float3x4
    rotxTM(ax, rx); rotyTM(ay, ry); rotzTM(az, rz)

    var newLocalTm = ry * rz * rx
    newLocalTm[3] = subentityLocalTms[idx][3]  // preserve original offset
    subentityLocalTms[idx] = newLocalTm

    recompute_subtree(idx, transform, subentityEids, subentityTms, subentityLocalTms, subentityParentIdx)

  let parentTM = transform
  for i, subentityEid in iter_range(subentityEids), subentityEids
    query(subentityEid) <| $ [es] (var transform : float3x4)
      transform = parentTM * subentityTms[i]
