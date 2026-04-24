require ecs
require ecs.common
require ecs.safe
require DagorSystem
require GeomNodeTree
require AnimV20
require gameLibs.pufd_events.pufd_events
require danetlibs.animchar_attach.main.attach_common
require danetlibs.animchar_attach.main.attach_events


def skeleton_attach_with_remap_parent_impl(eid : EntityId;
                                           skeleton_attach__remapParentSlots : StringList;
                                           human_equipment__slots : Object;
                                           var animchar_attach__attachedTo : EntityId&;
                                           invalid_parent_eid = INVALID_ENTITY_ID)
  for slotName in skeleton_attach__remapParentSlots
    let possibleParent = human_equipment__slots[string(slotName)] ?as Object
    let possibleParentEid = possibleParent?.item ?? INVALID_ENTITY_ID
    if possibleParentEid == eid
      logerr("Entity try to attach to itself. Invalid slots in {skeleton_attach__remapParentSlots} for {eid} {getEntityTemplateName(eid)}")
      return
    if (!!possibleParentEid &&
        possibleParentEid != invalid_parent_eid &&
        has(possibleParentEid, "skeleton_attach__remapAffectSlots"))
      if animchar_attach__attachedTo != possibleParentEid

        var list = getRW_ecs_EidList(animchar_attach__attachedTo, "attaches_list")
        if list != null // remove eid from list for fix twitching attaches on change
          let idx = find_index(*list, eid)
          if idx >= 0
            erase(*list, idx)

        animchar_attach__attachedTo = possibleParentEid
        sendEventImmediate(eid, CmdInitSkeletonAttach())
      return


def remap_parent_reinit_impl(parent_attached_to : EntityId;
                             skeleton_attach__remapAffectSlots : StringList;
                             invalid_parent_eid = INVALID_ENTITY_ID)
  let baseParentEid = find_base_attach_parent(parent_attached_to)
  query(baseParentEid) <| $ [es] (human_equipment__slots : Object)
    for slotName in skeleton_attach__remapAffectSlots
      let affectSlot = human_equipment__slots[string(slotName)] ?as Object
      let affectEid = affectSlot?.item ?? INVALID_ENTITY_ID
      query(affectEid) <| $ [es(REQUIRE=skeletonAttachInited)] (skeleton_attach__remapParentSlots : StringList;
                                                                var animchar_attach__attachedTo aka child_animchar_attach__attachedTo : EntityId&)
        skeleton_attach_with_remap_parent_impl(affectEid,
                                               skeleton_attach__remapParentSlots,
                                               human_equipment__slots,
                                               child_animchar_attach__attachedTo,
                                               invalid_parent_eid)


[es(tag=server, REQUIRE=skeletonAttachInited, on_appear)]
def skeleton_attach_with_remap_parent_reinit(evt : Event;
                                             animchar_attach__attachedTo : EntityId;
                                             skeleton_attach__remapAffectSlots : StringList)
  remap_parent_reinit_impl(animchar_attach__attachedTo, skeleton_attach__remapAffectSlots)


def remove_node_from_remap_list(nodeName : string;
                                var animchar : AnimcharBaseComponent&;
                                var skeleton_attach__skeletonRecalcBranch : UInt8List;
                                var skeleton_attach__skeletonRemap : UInt16List)
  assume origNodeTree = *animchar.originalNodeTree
  assume nodeTree = *animchar.nodeTree
  let nodeIdx = nodeTree |> geomtree_findNodeIndex(nodeName)
  if nodeIdx < 0
    return
  geomtree_getNodeTm(nodeTree, nodeIdx) = geomtree_getNodeTm(origNodeTree, nodeIdx)
  let remapedNodesCount = length(skeleton_attach__skeletonRemap)
  for i in range(remapedNodesCount)
    if get_node_no(skeleton_attach__skeletonRemap[i]) == nodeIdx
      skeleton_attach__skeletonRemap |> erase(i)
      skeleton_attach__skeletonRecalcBranch |> push(uint8(nodeIdx))
      break// we have only one require idx!

def skeleton_attach_with_remap_parent_remap_impl(attaches_list : EidList;
                                                 var animchar : AnimcharBaseComponent&;
                                                 var skeleton_attach__skeletonRecalcBranch : UInt8List;
                                                 var skeleton_attach__skeletonRemap : UInt16List)
  for attachEid in attaches_list
    query(attachEid) <| $ [es] (skeleton_attach__skipParentsRemapNodes : StringList)
      for nodeName in skeleton_attach__skipParentsRemapNodes
        remove_node_from_remap_list(string(nodeName), animchar, skeleton_attach__skeletonRecalcBranch, skeleton_attach__skeletonRemap)

[es(tag=gameClient, on_appear, on_event=CmdInitSkeletonAttach, after=(skeleton_attach_init_es, skeleton_attach_reinit_es), REQUIRE=skeletonAttachInited)]
def skeleton_attach_remove_remap_node_es(evt : Event;
                                         skeleton_attach__skipRemapNodes : StringList;
                                         var animchar : AnimcharBaseComponent&;
                                         var skeleton_attach__skeletonRecalcBranch : UInt8List;
                                         var skeleton_attach__skeletonRemap : UInt16List)
  for nodeName in skeleton_attach__skipRemapNodes
    remove_node_from_remap_list(string(nodeName), animchar, skeleton_attach__skeletonRecalcBranch, skeleton_attach__skeletonRemap)

[es(tag=gameClient, track=attaches_list, REQUIRE=skeleton_attach__skeletonRemap)]
def skeleton_attach_with_remap_parent_reinit_remap_track(evt : Event;
                                                         eid : EntityId;
                                                         attaches_list : EidList;
                                                         animchar_attach__attachedTo : EntityId;
                                                         var animchar : AnimcharBaseComponent&;
                                                         var skeleton_attach__skeletonRecalcBranch : UInt8List;
                                                         var skeleton_attach__skeletonRemap : UInt16List)
  query(animchar_attach__attachedTo) <| $ [es(REQUIRE=animchar)] ()
    sendEventImmediate(eid, CmdInitSkeletonAttach())
  skeleton_attach_with_remap_parent_remap_impl(attaches_list, animchar,
                                               skeleton_attach__skeletonRecalcBranch,
                                               skeleton_attach__skeletonRemap)


[es(tag=gameClient, REQUIRE=skeleton_attach__attached, track=animchar_attach__attachedTo, REQUIRE=(skeleton_attach__skeletonRemap, skeleton_attach__remapParentSlots))]
def skeleton_attach_with_remap_parent_reinit_attach_track(evt : Event;
                                                          eid : EntityId;
                                                          animchar_attach__attachedTo : EntityId)
  var parentFound = false
  query(animchar_attach__attachedTo) <| $ [es(REQUIRE=animchar)] ()
    sendEventImmediate(eid, CmdInitSkeletonAttach())
    parentFound = true
  if !parentFound // for case we change parent when async added sub template
    removeSubTemplate(eid, "base_skeleton_attach")

[es(tag=gameClient, on_appear, after=(skeleton_attach_init_es, skeleton_attach_reinit_es), REQUIRE=skeleton_attach__skeletonRemap)]
def skeleton_attach_with_remap_parent_reinit_remap_init(evt : Event;
                                                        attaches_list : EidList;
                                                        var animchar : AnimcharBaseComponent&;
                                                        var skeleton_attach__skeletonRecalcBranch : UInt8List;
                                                        var skeleton_attach__skeletonRemap : UInt16List)
  skeleton_attach_with_remap_parent_remap_impl(attaches_list, animchar,
                                               skeleton_attach__skeletonRecalcBranch,
                                               skeleton_attach__skeletonRemap)

[es(tag=server, REQUIRE=skeletonAttachInited, on_disappear)]
def skeleton_attach_with_remap_parent_disappear(evt : Event;
                                                eid : EntityId;
                                                animchar_attach__attachedTo : EntityId;
                                                skeleton_attach__remapAffectSlots : StringList)
  remap_parent_reinit_impl(animchar_attach__attachedTo, skeleton_attach__remapAffectSlots, eid)


[es(tag=server, on_appear, REQUIRE=skeletonAttachInited, track=animchar_attach__attachedTo)]
def skeleton_attach_with_remap_parent(evt : Event;
                                      eid : EntityId;
                                      skeleton_attach__remapParentSlots : StringList;
                                      var animchar_attach__attachedTo : EntityId&)
  let baseParentEid = find_base_attach_parent(animchar_attach__attachedTo)
  query(baseParentEid) <| $ [es] (human_equipment__slots : Object)
    skeleton_attach_with_remap_parent_impl(eid,
                                           skeleton_attach__remapParentSlots,
                                           human_equipment__slots,
                                           animchar_attach__attachedTo)