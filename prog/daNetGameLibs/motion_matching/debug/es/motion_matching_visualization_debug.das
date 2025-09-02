options no_aot
require ecs
require ecs.common
require AnimV20
require DagorMath
require DagorDebug3D
require MotionMatching
require GeomNodeTree


[es(tag=(dev, render), no_order)]
def mm_prepare_debug_visualization(evt : UpdateStageInfoAct;
                                   mm_imguiAnimcharEid : EntityId;
                                   mm_visualization_show_feature_nodes : bool;
                                   mm_visualization_show_trajectory : bool;
                                   var mm_visualization__predictedTrajectoryPos : Point3List;
                                   var mm_visualization__predictedTrajectoryDir : Point3List;
                                   var mm_visualization__animcharTm : float3x4&;
                                   var mm_visualization__rootTm : float3x4&;
                                   var mm_visualization__currentClipIdx : int&;
                                   var mm_visualization__currentFrameIdx : int&)
  if !mm_visualization_show_feature_nodes && !mm_visualization_show_trajectory
    return
  query(mm_imguiAnimcharEid) <| $ [es] (animchar : AnimcharBaseComponent;
                                        motion_matching__controller : MotionMatchingController;
                                        mm_trajectory__featurePositions : Point3List;
                                        mm_trajectory__featureDirections : Point3List)
    if !motion_matching__controller.hasActiveAnimation()
      mm_visualization__currentClipIdx = -1
      mm_visualization__currentFrameIdx = -1
      return
    mm_visualization__currentClipIdx = motion_matching__controller.getCurrentClip()
    mm_visualization__currentFrameIdx = motion_matching__controller.getCurrentFrame()
    mm_visualization__predictedTrajectoryPos := mm_trajectory__featurePositions
    mm_visualization__predictedTrajectoryDir := mm_trajectory__featureDirections
    animchar.animchar_get_tm(mm_visualization__animcharTm)
    // Can differ from gameplay animchar transform when synchronization is disabled because root node
    // will be moved according trajectory from animations
    make_tm(quat(motion_matching__controller.rootRotation), motion_matching__controller.rootPosition, mm_visualization__rootTm)

[es(tag=(dev, render), no_order)]
def draw_foot_locker_debug(evt : UpdateStageInfoRenderDebug;
                           mm_params__footLockerEnabled : bool;
                           mm_visualization_show_foot_locker_positions : bool)
  if !mm_visualization_show_foot_locker_positions
    return
  begin_draw_cached_debug_lines(false, false, false)
  query() <| $ [es] (motion_matching__controller : MotionMatchingController;
                     animchar : AnimcharBaseComponent)

    if motion_matching__controller.dataBase == null
      return

    assume db = *motion_matching__controller.dataBase
    if mm_params__footLockerEnabled
      anim_state_holder_iterate_foot_locker_legs_const(animchar.animState, db) <| $(leg : FootLockerIKCtrlLegData)
        if leg.isLocked
          draw_cached_debug_sphere(leg.lockedPosition, 0.03, E3DCOLOR(0xFFFF0000), 4)
    elif motion_matching__controller.hasActiveAnimation()
      let clipIdx = motion_matching__controller.getCurrentClip()
      let frameIdx = motion_matching__controller.getCurrentFrame()
      for footNode, legIdx in db.footLockerNodes, count()
        if animation_database_need_lock_foot(db, clipIdx, frameIdx, legIdx)
          let footPos = geomtree_getNodeWpos(*animchar.nodeTree, int(footNode))
          draw_cached_debug_sphere(footPos, 0.03, E3DCOLOR(0xFFFF0000), 4)


  end_draw_cached_debug_lines()

def draw_debug_line(p1, p2 : float3; color : E3DCOLOR)
  draw_cached_debug_line(p1, p2, color)

[es(tag=(dev, render), no_order)]
def mm_draw_feature_nodes_debug(evt : UpdateStageInfoRenderDebug;
                                mm_imguiAnimcharEid : EntityId;
                                mm_visualization_show_feature_nodes : bool;
                                mm_visualization__currentClipIdx : int;
                                mm_visualization__currentFrameIdx : int;
                                mm_visualization__rootTm : float3x4)
  if !mm_visualization_show_feature_nodes || mm_visualization__currentClipIdx < 0 || mm_visualization__currentFrameIdx < 0
    return
  let clipIdx = mm_visualization__currentClipIdx
  let frameIdx = mm_visualization__currentFrameIdx
  query(mm_imguiAnimcharEid) <| $ [es] (motion_matching__controller : MotionMatchingController)
    assume dataBase = *motion_matching__controller.dataBase
    begin_draw_cached_debug_lines(false, true, false);
    for i in range(dataBase.nodeCount)
      let pos = mm_visualization__rootTm * dataBase.getNodePositionFeature(clipIdx, frameIdx, i)
      let vel = rotate(mm_visualization__rootTm, dataBase.getNodeVelocityFeature(clipIdx, frameIdx, i))
      let VELOCITY_SCALE = 0.15f
      draw_cached_debug_sphere(pos, 0.15f, E3DCOLOR(0xFFFFFFFF), 10)
      draw_debug_line(pos, pos + vel * VELOCITY_SCALE, E3DCOLOR(0xFF00FFFF))
    end_draw_cached_debug_lines()

[es(tag=(dev, render), no_order)]
def mm_draw_trajectory_debug(evt : UpdateStageInfoRenderDebug;
                             mm_imguiAnimcharEid : EntityId;
                             mm_visualization_show_trajectory : bool;
                             mm_visualization__currentClipIdx : int;
                             mm_visualization__currentFrameIdx : int;
                             mm_visualization__selectedClipIdx : int;
                             mm_visualization__selectedFrameIdx : int;
                             mm_visualization__animcharTm : float3x4;
                             mm_visualization__rootTm : float3x4;
                             mm_visualization__predictedTrajectoryPos : Point3List;
                             mm_visualization__predictedTrajectoryDir : Point3List)
  if !mm_visualization_show_trajectory || mm_visualization__currentClipIdx < 0 || mm_visualization__currentFrameIdx < 0
    return
  let curClipOverride = mm_visualization__selectedClipIdx >= 0
  let clipIdx = curClipOverride ? mm_visualization__selectedClipIdx : mm_visualization__currentClipIdx
  let frameIdx = curClipOverride ? mm_visualization__selectedFrameIdx : mm_visualization__currentFrameIdx
  query(mm_imguiAnimcharEid) <| $ [es] (motion_matching__controller : MotionMatchingController)
    assume dataBase = *motion_matching__controller.dataBase

    begin_draw_cached_debug_lines(false, true, false)
    let currentColor = E3DCOLOR(uint4(50, 150, 255, 255))
    let predictionColor = E3DCOLOR(uint4(0, 0, 255, 255))
    let animationColor = curClipOverride ? E3DCOLOR(uint4(177, 177, 177, 255)) : E3DCOLOR(uint4(0, 255, 0, 255))
    let DIRECTION_SCALE = 0.35f
    let SPHERE_RADIUS = 0.03f
    let SPHERE_SEGMENTS = 10

    let currentPos = mm_visualization__animcharTm[3]
    let currentDir = -mm_visualization__animcharTm[2]
    let currentAnimPos = mm_visualization__rootTm[3]
    let currentAnimDir = -mm_visualization__rootTm[2]
    draw_cached_debug_sphere(currentPos, SPHERE_RADIUS, currentColor, SPHERE_SEGMENTS)
    draw_cached_debug_sphere(currentAnimPos, SPHERE_RADIUS, animationColor, SPHERE_SEGMENTS)
    draw_debug_line(currentPos, currentPos + currentDir * DIRECTION_SCALE, currentColor)
    draw_debug_line(currentAnimPos, currentAnimPos + currentAnimDir * DIRECTION_SCALE, animationColor)

    for i in range(length(mm_visualization__predictedTrajectoryPos))
      let clipPos = dataBase.getTrajectoryPositionFeature(clipIdx, frameIdx, i)
      let clipDir = dataBase.getTrajectoryDirectionFeature(clipIdx, frameIdx, i)
      let animatedPos = mm_visualization__rootTm * float3(clipPos.x, 0, clipPos.y)
      let animatedDir = rotate(mm_visualization__rootTm, float3(clipDir.x, 0, clipDir.y))
      let predictedPos = mm_visualization__rootTm * mm_visualization__predictedTrajectoryPos[i]
      let predictedDir = rotate(mm_visualization__rootTm, mm_visualization__predictedTrajectoryDir[i])
      draw_cached_debug_sphere(predictedPos, SPHERE_RADIUS, predictionColor, SPHERE_SEGMENTS)
      draw_cached_debug_sphere(animatedPos, SPHERE_RADIUS, animationColor, SPHERE_SEGMENTS)
      draw_debug_line(predictedPos, predictedPos + predictedDir * DIRECTION_SCALE, predictionColor)
      draw_debug_line(animatedPos, animatedPos + animatedDir * DIRECTION_SCALE, animationColor)

    end_draw_cached_debug_lines()

def draw_skeleton_links(tree : GeomNodeTree;
                        node_idx : int;
                        node_wpos : Point3List;
                        show_labels : bool;
                        node_filter : block<(node_id : int) : bool>)
  let nodePos = node_wpos[node_idx]
  if invoke(node_filter, node_idx)
    let nodeName = tree.geomtree_getNodeName(node_idx)
    if (show_labels && !empty(nodeName))
      add_debug_text_mark(nodePos, "{nodeName} ({node_idx})", -1, 0f, E3DCOLOR(0xFFFFFFFF))
    draw_cached_debug_sphere(nodePos, 0.03, E3DCOLOR(uint4(0, 0, 255, 255)), 10)
  for i in range(tree.geomtree_getChildCount(node_idx))
    let childNodeIdx = tree.geomtree_getChildNodeIdx(node_idx, uint(i))
    let childPos = node_wpos[childNodeIdx]
    draw_cached_debug_line(nodePos, childPos, E3DCOLOR(uint4(128, 128, 255, 255)))
    draw_skeleton_links(tree, childNodeIdx, node_wpos, show_labels, node_filter)

[es(tag=(dev, render), no_order)]
def mm_draw_skeleton_debug(evt : UpdateStageInfoRenderDebug;
                           mm_imguiAnimcharEid : EntityId;
                           mm_visualization_show_skeleton : bool;
                           mm_visualization_show_skeleton_original : bool;
                           mm_visualization_show_skeleton_node_labels : bool;
                           mm_visualization__skeletonNodeWpos : Point3List;
                           mm_visualization__skeletonNodeFilter : IntList)
  if !mm_visualization_show_skeleton || empty(mm_visualization__skeletonNodeWpos)
    return
  query(mm_imguiAnimcharEid) <| $ [es] (animchar : AnimcharBaseComponent)
    assume tree = *animchar.originalNodeTree
    if mm_visualization_show_skeleton_original
      let node_filter = $[unused_argument(node_idx)] (node_idx : int) => true
      draw_skeleton_links(tree, 0, mm_visualization__skeletonNodeWpos, mm_visualization_show_skeleton_node_labels, node_filter)
    else
      draw_skeleton_links(tree, 0, mm_visualization__skeletonNodeWpos, mm_visualization_show_skeleton_node_labels) <| $(node_idx : int)
        return find_index(mm_visualization__skeletonNodeFilter, node_idx) >= 0
