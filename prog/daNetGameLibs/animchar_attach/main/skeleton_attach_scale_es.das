require ecs
require AnimV20
require DagorSystem
require GeomNodeTree
require danetlibs.animchar_attach.main.attach_common
require danetlibs.animchar_attach.main.attach_events
require ecs.safe
require ecs.common


[es(tag=server, on_appear)]
def skeleton_hide_node_attach_created_es(evt : Event;
                                         animchar_attach__attachedTo : EntityId)
  let parentEid = find_base_attach_parent(animchar_attach__attachedTo)
  query(parentEid) <| $ [es(REQUIRE=human_equipment__slots)] ()
    sendEvent(parentEid, CmdInitScaleSkeletonAttach())

[es(tag=server, on_appear, on_event=CmdInitScaleSkeletonAttach, track=human_equipment__slots)]
def skeleton_hide_node_attach_init_es(evt : Event;
                                      human_equipment__slots : Object)
  for it in human_equipment__slots
    let item = (it.value as Object)?.item ?? INVALID_ENTITY_ID
    query(item) <| $ [es] (var animchar__scaledWtmNodeIds : IntList;
                           var animchar__scaledWtmScales : Point3List;
                           var animchar__scaledWtmRecalc : BoolList)
      clear(animchar__scaledWtmNodeIds)
      clear(animchar__scaledWtmScales)
      clear(animchar__scaledWtmRecalc)

  for it in human_equipment__slots
    let slot = it.key
    let item = (it.value as Object)?.item ?? INVALID_ENTITY_ID
    if !!item
      query(item) <| $ [es] (equipment__scaleAttaches : Object)
        for hideIt in equipment__scaleAttaches
          let affectSlotId = hideIt.key
          if slot == affectSlotId
            logerr("slot can't affect himself")
          let hideData = hideIt.value as Object
          if hideData != null
            let affectSlot = human_equipment__slots[string(affectSlotId)] ?as Object
            let affectEid = affectSlot?.item ?? INVALID_ENTITY_ID
            query(affectEid) <| $ [es] (animchar : AnimcharBaseComponent;
                                        var animchar__scaledWtmNodeIds : IntList;
                                        var animchar__scaledWtmScales : Point3List;
                                        var animchar__scaledWtmRecalc : BoolList)
              for node in *hideData
                let nodeName = node.key
                let hideNodeData = node.value as Object
                if hideNodeData != null
                  let intNodeNo = geomtree_findNodeIndex(*animchar.nodeTree, string(nodeName))
                  if intNodeNo < 0
                    return
                  let idx = find_index(animchar__scaledWtmNodeIds, intNodeNo)
                  if idx >= 0
                    let scale = (*hideNodeData).scale ?as float3
                    if scale != null
                      animchar__scaledWtmScales[idx].x = min(animchar__scaledWtmScales[idx].x, (*scale).x)
                      animchar__scaledWtmScales[idx].y = min(animchar__scaledWtmScales[idx].x, (*scale).y)
                      animchar__scaledWtmScales[idx].z = min(animchar__scaledWtmScales[idx].x, (*scale).z)
                    animchar__scaledWtmRecalc[idx] = (*hideNodeData)["recalc"] ?? false
                  else
                    animchar__scaledWtmNodeIds |> push(intNodeNo)
                    animchar__scaledWtmScales |> push((*hideNodeData)["scale"] ?? float3(1., 1., 1))
                    animchar__scaledWtmRecalc |> push((*hideNodeData)["recalc"] ?? false)