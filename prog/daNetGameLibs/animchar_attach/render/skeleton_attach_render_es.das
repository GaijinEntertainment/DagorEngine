require ecs
require DagorMath
require AnimV20
require DagorSystem
require DagorMath
require GeomNodeTree
require gameLibs.pufd_events.pufd_events
require danetlibs.animchar_attach.main.attach_common


def copy_nodes_with_remap(var node_tree : GeomNodeTree&; att_node_tree : GeomNodeTree; skeleton_remap : UInt16List; base_eid, att_eid : EntityId)
  let nodeTreeCount = node_tree.nodeCount
  let attNodeTreeCount = att_node_tree.nodeCount
  for skelNode in skeleton_remap
    let nodeNo = get_node_no(skelNode)
    let remappedNodeNo = get_remapped_node_no(skelNode)
    if uint(nodeNo) < nodeTreeCount && uint(remappedNodeNo) < attNodeTreeCount
      geomtree_getNodeTm(node_tree, nodeNo) = geomtree_getNodeTm(att_node_tree, remappedNodeNo)
      geomtree_getNodeWtmRel(node_tree, nodeNo) = geomtree_getNodeWtmRel(att_node_tree, remappedNodeNo)
    else
      logerr("Failed to copy nodes {att_eid} {getEntityTemplateName(att_eid)} from parent {base_eid} {getEntityTemplateName(base_eid)}.
       ({nodeNo} < {nodeTreeCount} && {remappedNodeNo} < {attNodeTreeCount}")
      query(att_eid) <| $ [es] (animchar_attach__attachedTo : EntityId;
                                slot_attach__inAttachedList : EntityId)
        logerr("Failed to copy nodes info {animchar_attach__attachedTo} {getEntityTemplateName(animchar_attach__attachedTo)}.
         {slot_attach__inAttachedList} {getEntityTemplateName(slot_attach__inAttachedList)}")
      destroyEntity(att_eid)


def recalc_branches(var node_tree : GeomNodeTree&; skeleton_recalc_branch : UInt8List)
  for nodeNo in skeleton_recalc_branch
    let intNodeNo = int(nodeNo)
    let parentNodeIdx = geomtree_getParentNodeIdx(node_tree, intNodeNo)
    let parentNodeWtm = float3x4(geomtree_getNodeWtmRel(node_tree, parentNodeIdx))
    let curNodeWtm = float3x4(geomtree_getNodeTm(node_tree, intNodeNo))
    geomtree_getNodeWtmRel(node_tree, intNodeNo) = mat44f(parentNodeWtm * curNodeWtm)
    geomtree_calcWtmForBranch(node_tree, intNodeNo)


def skeleton_attached_to(base_eid : EntityId;
                         base_animchar : AnimcharBaseComponent&;
                         animchar__visible : bool;
                         base_attaches_list : EidList;
                         dt : float;
                         base_animchar_bbox : bbox3f;
                         var base_animchar_bsph_precalculated : vec4f&;
                         var base_animchar_attaches_bbox_precalculated : bbox3f?)
  if base_animchar_attaches_bbox_precalculated != null
    *base_animchar_attaches_bbox_precalculated = base_animchar_bbox

  for attachEid in base_attaches_list
    query(attachEid) <| $ [es(trust_access, REQUIRE=animchar_attach__attachedTo)] (skeleton_attach__attached : bool;
                                                                                   skeleton_attach__skeletonRemap : UInt16List;
                                                                                   skeleton_attach__skeletonRecalcBranch : UInt8List;
                                                                                   attaches_list : EidList const?;
                                                                                   animchar_bbox : bbox3f;
                                                                                   var animchar : AnimcharBaseComponent&;
                                                                                   animchar_render : AnimcharRendComponent;
                                                                                   var animchar__usePrecalculatedData : bool&;
                                                                                   var animchar_bsph_precalculated : vec4f&;
                                                                                   var animchar_bbox_precalculated : BBox3&;
                                                                                   var animchar_shadow_cull_bbox_precalculated : BBox3&;
                                                                                   var animchar_node_wtm : AnimcharNodesMat44;
                                                                                   var animchar_render__root_pos : vec4f&;
                                                                                   var animchar_attaches_bbox_precalculated : bbox3f?;
                                                                                   animchar_render__enabled : bool = true)
      if !animchar_render__enabled || !skeleton_attach__attached
        return

      animchar__usePrecalculatedData = true

      assume attNodeTree = *base_animchar.nodeTree
      assume nodeTree = *animchar.nodeTree

      let wtmOfs = geomtree_getWtmOfs(attNodeTree)
      geomtree_setWtmOfs(nodeTree, wtmOfs)

      copy_nodes_with_remap(nodeTree, attNodeTree, skeleton_attach__skeletonRemap, base_eid, attachEid)
      recalc_branches(nodeTree, skeleton_attach__skeletonRecalcBranch)

      if animchar__visible
        animchar_forcePostRecalcWtm(animchar, dt)

      animchar_copy_nodes(animchar, animchar_node_wtm, animchar_render__root_pos)
      animchar_bsph_precalculated = animchar_render_prepareSphere(animchar_render, animchar_node_wtm)
      animchar_bbox_precalculated = animchar_render_calcWorldBox(animchar_render, animchar_node_wtm, true)
      animchar_shadow_cull_bbox_precalculated = animchar_render_calcWorldBox(animchar_render, animchar_node_wtm, false)

      if attaches_list != null
        skeleton_attached_to(base_eid, animchar, animchar__visible, *attaches_list, dt, animchar_bbox, animchar_bsph_precalculated, animchar_attaches_bbox_precalculated)

      let attachDir = animchar_bsph_precalculated.xyz - base_animchar_bsph_precalculated.xyz
      let maxAttachDistance = length(attachDir) + animchar_bsph_precalculated.w
      if maxAttachDistance > base_animchar_bsph_precalculated.w
        base_animchar_bsph_precalculated.w = maxAttachDistance

      if base_animchar_attaches_bbox_precalculated != null
        bbox3f_add_box(*base_animchar_attaches_bbox_precalculated, animchar_bbox)


[es(tag=gameClient, after=visual_offset_updater_es, before=(slot_attach_init_tms_es, vehicle_wheels_prepare_es, node_attached_itm, node_attached_pos, start_async_phys_sim_es), parallel_for=4, REQUIRE_NOT=attachedToParent)]
def skeleton_attach_es(info : ParallelUpdateFrameDelayed;
                       eid : EntityId;
                       animchar : AnimcharBaseComponent&;
                       animchar__visible : bool;
                       animchar_bbox : bbox3f;
                       attaches_list : EidList;
                       var animchar_bsph_precalculated : vec4f&;
                       var animchar_bbox_precalculated : BBox3&;
                       var animchar_shadow_cull_bbox_precalculated : BBox3&;
                       var animchar_attaches_bbox_precalculated : bbox3f?;
                       var animchar__usePrecalculatedData : bool&;
                       animchar_render : AnimcharRendComponent;
                       animchar_node_wtm : AnimcharNodesMat44;
                       var attaches_list__additionalUpdateFrames : int?;
                       animchar__updateAttachesWhenNotUpdatable : Tag const?;
                       animchar__updatable = true)
  if !animchar__updatable && animchar__updateAttachesWhenNotUpdatable == null && (attaches_list__additionalUpdateFrames == null || *attaches_list__additionalUpdateFrames <= 0)
    return

  if attaches_list__additionalUpdateFrames != null && *attaches_list__additionalUpdateFrames > 0
    *attaches_list__additionalUpdateFrames -= 1

  animchar__usePrecalculatedData = true

  animchar_bsph_precalculated = animchar_render_prepareSphere(animchar_render, animchar_node_wtm)
  animchar_bbox_precalculated = animchar_render_calcWorldBox(animchar_render, animchar_node_wtm, true)
  animchar_shadow_cull_bbox_precalculated = animchar_render_calcWorldBox(animchar_render, animchar_node_wtm, false)
  skeleton_attached_to(eid, animchar, animchar__visible, attaches_list, info.dt, animchar_bbox, animchar_bsph_precalculated, animchar_attaches_bbox_precalculated)

//for independent created attaches
//we update animchars in animchar_non_updatable_es and don't recalculate bsph e.t.c. components
[es(tag=gameClient, track=transform, on_appear, after=animchar_non_updatable_es, REQUIRE_NOT=attachedToParent, REQUIRE=(disableUpdate, attachmentUpdate, transform))]
def skeleton_attach_not_updatable(evt : Event;
                                  eid : EntityId;
                                  animchar : AnimcharBaseComponent&;
                                  animchar__visible : bool;
                                  animchar_bbox : bbox3f;
                                  attaches_list : EidList;
                                  var animchar_bsph_precalculated : vec4f&;
                                  var animchar_bbox_precalculated : BBox3&;
                                  var animchar_shadow_cull_bbox_precalculated : BBox3&;
                                  var animchar_attaches_bbox_precalculated : bbox3f?;
                                  var animchar__usePrecalculatedData : bool&;
                                  animchar_render : AnimcharRendComponent;
                                  animchar_node_wtm : AnimcharNodesMat44)
  animchar__usePrecalculatedData = true

  animchar_bsph_precalculated = animchar_render_prepareSphere(animchar_render, animchar_node_wtm)
  animchar_bbox_precalculated = animchar_render_calcWorldBox(animchar_render, animchar_node_wtm, true)
  animchar_shadow_cull_bbox_precalculated = animchar_render_calcWorldBox(animchar_render, animchar_node_wtm, false)
  skeleton_attached_to(eid, animchar, animchar__visible, attaches_list, 0.0, animchar_bbox, animchar_bsph_precalculated, animchar_attaches_bbox_precalculated)


[es(tag=gameClient, on_appear, after=(skeleton_attach_init_es, skeleton_attach_reinit_es), REQUIRE=attachedToParent)]
def init_skeleton_attachment_wtm_ofs(evt : Event;
                                     eid : EntityId;
                                     skeleton_attach__skeletonRemap : UInt16List;
                                     skeleton_attach__skeletonRecalcBranch : UInt8List;
                                     animchar_attach__attachedTo : EntityId;
                                     var animchar aka attachment_animchar : AnimcharBaseComponent&)
  query(animchar_attach__attachedTo) <| $ [es] (animchar aka attached_to_animchar : AnimcharBaseComponent)
    assume attNodeTree = *attached_to_animchar.nodeTree
    assume nodeTree = *attachment_animchar.nodeTree

    let wtmOfs = geomtree_getWtmOfs(attNodeTree)
    geomtree_setWtmOfs(nodeTree, wtmOfs)

    copy_nodes_with_remap(nodeTree, attNodeTree, skeleton_attach__skeletonRemap, animchar_attach__attachedTo, eid)
    recalc_branches(nodeTree, skeleton_attach__skeletonRecalcBranch)

    attachment_animchar |> animchar_resetFastPhysWtmOfs(wtmOfs)
    // simulate fastphys for some frames to let fastphys items settle
    let fastPhysSimTimeOnReset = 1.5
    attachment_animchar |> animchar_updateFastPhys(fastPhysSimTimeOnReset)


def save_cur_geom_node_tree(animchar : AnimcharBaseComponent&;
                            var saved_animchar : AnimcharBaseComponent?)
  if saved_animchar != null
    assume nodeTree = *(animchar.nodeTree)
    assume savedNodeTree = *((*saved_animchar).nodeTree)
    let nodeCount = nodeTree.nodeCount
    if savedNodeTree.nodeCount != nodeCount
      logerr("Incorrect animchar res inited for saved animchar")
      return
    for i in range(0, int(nodeCount))
      geomtree_getNodeTm(savedNodeTree, i) = geomtree_getNodeTm(nodeTree, i)
      geomtree_getNodeWtmRel(savedNodeTree, i) = geomtree_getNodeWtmRel(nodeTree, i)


def save_attaches_animchar_tree(attaches_list : EidList const?)
  if attaches_list == null
    return
  for attach in *attaches_list
    query(attach) <| $ [es] (animchar : AnimcharBaseComponent&;
                             attaches_list : EidList const?;
                             var saved_animchar : AnimcharBaseComponent?)
      save_cur_geom_node_tree(animchar, saved_animchar)
      save_attaches_animchar_tree(attaches_list)


[es(tag=gameClient, on_appear, REQUIRE=deadEntity)]
def skeleton_attach_save_on_dead_es(evt : Event;
                                    animchar : AnimcharBaseComponent&;
                                    attaches_list : EidList const?;
                                    var saved_animchar : AnimcharBaseComponent?)
  save_cur_geom_node_tree(animchar, saved_animchar)
  save_attaches_animchar_tree(attaches_list)
  // error("ANIMCHAR SAVED {saved_animchar != null}")

[es(tag=gameClient, on_event=CmdShowHitcamera, REQUIRE_NOT=deadEntity)]
def skeleton_attach_save_on_hit_cam_es(evt : Event;
                                       hitcamera__target : EntityId)
  query(hitcamera__target) <| $ [es] (animchar : AnimcharBaseComponent&;
                                      attaches_list : EidList const?;
                                      var saved_animchar : AnimcharBaseComponent?)
    save_cur_geom_node_tree(animchar, saved_animchar)
    save_attaches_animchar_tree(attaches_list)


[es(tag=gameClient, after=(animated_visible_attachments_act_animchar_es, skeleton_attach_es), parallel_for=4)]
def skeleton_hide_node_attach_es(info : ParallelUpdateFrameDelayed;
                                 var animchar : AnimcharBaseComponent&;
                                 var animchar_node_wtm : AnimcharNodesMat44;
                                 var animchar_render__root_pos : vec4f&;
                                 animchar__scaledWtmNodeIds : IntList;
                                 animchar__scaledWtmScales : Point3List;
                                 animchar__scaledWtmRecalc : BoolList;
                                 animchar__visible : bool = true)
  if !animchar__visible
    return
  assume nodeTree = *animchar.nodeTree
  for nodeId, scale, recalc in animchar__scaledWtmNodeIds, animchar__scaledWtmScales, animchar__scaledWtmRecalc
    var curNodeWtm : float3x4
    nodeTree |> geomtree_getNodeWtmRelScalar(nodeId, curNodeWtm)

    curNodeWtm[0] *= scale.x
    curNodeWtm[1] *= scale.y
    curNodeWtm[2] *= scale.z

    geomtree_setNodeWtmRelScalar(nodeTree, nodeId, curNodeWtm)
    if recalc
      geomtree_calcWtmForBranch(nodeTree, nodeId)

  if length(animchar__scaledWtmNodeIds) > 0
    animchar_copy_nodes(animchar, animchar_node_wtm, animchar_render__root_pos)
