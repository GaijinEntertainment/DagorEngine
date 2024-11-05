options no_aot
require ecs
require math
require AnimV20
require DagorMath
require DagorDebug3D
require GeomNodeTree
require MotionMatching

def transform_upto_root(controller : MotionMatchingController; root_id, node_id : int; tree : GeomNodeTree)
  //resultAnimation already modified by foot locker, need to calc offset+current again
  assume curBones = controller.currentAnimation;
  assume offsetBones = controller.offset;
  var pos = offsetBones.position[node_id].xyz + curBones.position[node_id].xyz
  var parent_id = geomtree_getParentNodeIdx(tree, node_id)
  while parent_id != root_id
    pos = quat_mul_vec(offsetBones.rotation[parent_id], quat_mul_vec(curBones.rotation[parent_id], pos))
    pos += offsetBones.position[parent_id].xyz + curBones.position[parent_id].xyz
    parent_id = geomtree_getParentNodeIdx(tree, parent_id)
  return pos

[es(tag=(dev, render), no_order)]
def draw_foot_ik_locker_debug(evt : UpdateStageInfoRenderDebug;
                              mm_visualization_show_foot_ik_locked_positions : bool;
                              mm_visualization_show_foot_ik_target_positions : bool)
  begin_draw_cached_debug_lines(false, false, false)
  query() <| $ [es] (motion_matching__controller : MotionMatchingController;
                     animchar : AnimcharBaseComponent;
                     motion_matching__dataBaseEid : EntityId)

    if motion_matching__controller.dataBase == null
      return
    var legId = 0
    assume tree = *animchar.nodeTree
    motion_matching__controller |> iterate_foot_ik_locker_states <| $(islocked : bool;
                                                                      lockedPos : float3;
                                                                      posOffset : float3)
      if mm_visualization_show_foot_ik_locked_positions && islocked
        draw_cached_debug_sphere(lockedPos, 0.03, E3DCOLOR(0xFFFF0000), 4)
      if mm_visualization_show_foot_ik_target_positions && hasActiveAnimation(motion_matching__controller)
        var toeNodeId, heelNodeId, rootNodeId : int
        query(motion_matching__dataBaseEid) <| $ [es] (dataBase : AnimationDataBase;
                                                       foot_ik_locker_nodeNames : ecs::Array)
          let legNodeNames = get_ecs_object(foot_ik_locker_nodeNames[legId])
          toeNodeId = geomtree_findNodeIndex(tree, get_string(legNodeNames, "toeNodeName", ""))
          heelNodeId = geomtree_findNodeIndex(tree, get_string(legNodeNames, "heelNodeName", ""))
          rootNodeId = get_root_node_id(dataBase)

        var heelTargetPos, toeTargetPos : float3
        let heelLocalPos = transform_upto_root(motion_matching__controller, rootNodeId, heelNodeId, tree)
        let toeLocalPos = transform_upto_root(motion_matching__controller, rootNodeId, toeNodeId, tree)
        // should work if root node wasn't modified by post blend controllers, otherwise need to get root tm
        // from MM contoller
        var rootTm : float3x4
        geomtree_getNodeWtmScalar(tree, rootNodeId, rootTm)
        if islocked
          toeTargetPos = lockedPos + posOffset
        else
          toeTargetPos = rootTm * toeLocalPos + posOffset
        let toeToHeel = rotate(rootTm, heelLocalPos - toeLocalPos)
        heelTargetPos = toeTargetPos + toeToHeel
        let ikTargetColor = E3DCOLOR(0xFFFF00FF)
        draw_cached_debug_sphere(heelTargetPos, 0.03, ikTargetColor, 4)
        draw_cached_debug_sphere(toeTargetPos, 0.03, ikTargetColor, 4)
        draw_cached_debug_line(toeTargetPos, heelTargetPos, ikTargetColor)
      legId++

    assume db = motion_matching__controller.dataBase
    anim_state_holder_iterate_foot_locker_legs_const(animchar.animState, *db) <| $(leg : FootLockerIKCtrlLegData)
      if mm_visualization_show_foot_ik_locked_positions && leg.isLocked
        draw_cached_debug_sphere(leg.lockedPosition, 0.03, E3DCOLOR(0xFFFF0000), 4)
  end_draw_cached_debug_lines()

