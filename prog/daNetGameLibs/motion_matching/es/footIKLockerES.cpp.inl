// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <anim/dag_animPostBlendCtrl.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/entityManager.h>
#include <ecs/delayedAct/actInThread.h>
#include <ecs/anim/anim.h>
#include <math/dag_geomTree.h>
#include <memory/dag_framemem.h>

#include "animation/foot_ik_locker.h"
#include "animation/inertial_blending.h"
#include "animation/motion_matching_controller.h"
#include "animation/animation_data_base.h"

static dag::Index16 get_node_id_with_validation(const GeomNodeTree &tree, const char *node_name)
{
  dag::Index16 nodeId = tree.findNodeIndex(node_name);
  if (!nodeId)
    logerr("Can't find '%s' node", node_name);
  return nodeId;
}

static void init_foot_ik_locker_nodes(FootIKLockerNodes &foot_ik_locker_nodes,
  const ecs::Array &foot_ik_locker_nodeNames,
  const GeomNodeTree &tree,
  bool &foot_ik_locker_enabled)
{
  G_ASSERT(foot_ik_locker_nodeNames.size() == foot_ik_locker_nodes.NUM_LEGS);
  foot_ik_locker_enabled = true;
  for (int i = 0, ie = foot_ik_locker_nodes.NUM_LEGS; i < ie; ++i)
  {
    const ecs::Object &legNodeNames = foot_ik_locker_nodeNames[i].get<ecs::Object>();
    FootIKLockerNodes::LegNodes &leg = foot_ik_locker_nodes.legs[i];
    leg.toeNodeId = get_node_id_with_validation(tree, legNodeNames.getMemberOr(ECS_HASH("toeNodeName"), ""));
    leg.heelNodeId = get_node_id_with_validation(tree, legNodeNames.getMemberOr(ECS_HASH("heelNodeName"), ""));
    leg.kneeNodeId = get_node_id_with_validation(tree, legNodeNames.getMemberOr(ECS_HASH("kneeNodeName"), ""));
    leg.hipNodeId = get_node_id_with_validation(tree, legNodeNames.getMemberOr(ECS_HASH("hipNodeName"), ""));

    if (!(leg.toeNodeId && leg.heelNodeId && leg.kneeNodeId && leg.hipNodeId))
      foot_ik_locker_enabled = false;
  }
}

template <typename T>
void foot_ik_locker_nodes_ecs_query(ecs::EntityId, T);

FootIKLockerNodes get_foot_ik_locker_nodes(ecs::EntityId data_base_eid, const GeomNodeTree &tree)
{
  FootIKLockerNodes nodes;
  foot_ik_locker_nodes_ecs_query(data_base_eid,
    [&](FootIKLockerNodes &foot_ik_locker_nodes, const ecs::Array &foot_ik_locker_nodeNames, bool &foot_ik_locker_inited,
      bool &foot_ik_locker_enabled) {
      if (!foot_ik_locker_inited)
      {
        init_foot_ik_locker_nodes(foot_ik_locker_nodes, foot_ik_locker_nodeNames, tree, foot_ik_locker_enabled);
        foot_ik_locker_inited = true;
      }
      nodes = foot_ik_locker_nodes;
    });
  return nodes;
}

ECS_BROADCAST_EVENT_TYPE(InvalidateAnimationDataBase);

ECS_TAG(render)
ECS_BEFORE(init_animations_es)
static void mm_foot_ik_locker_invalidate_es(const InvalidateAnimationDataBase &, bool &foot_ik_locker_inited)
{
  foot_ik_locker_inited = false;
}

static void calc_global_bone(mat44f &node_wtm, const BoneInertialInfo &bones, mat44f_cref parent_wtm, dag::Index16 node_id)
{
  v_mat44_compose(node_wtm, bones.position[node_id.index()], bones.rotation[node_id.index()], V_C_ONE);
  v_mat44_mul43(node_wtm, parent_wtm, node_wtm);
}

static void calc_global_bones(const MotionMatchingController &controller,
  const GeomNodeTree &tree,
  dag::Index16 node_id,
  dag::Index16 root_id,
  dag::Vector<mat44f, framemem_allocator> &global_bones)
{
  const BoneInertialInfo &bones = controller.resultAnimation;
  if (node_id == root_id)
  {
    v_mat44_compose(global_bones[root_id.index()], v_quat_mul_vec3(controller.rootRotation, bones.position[root_id.index()]),
      v_quat_mul_quat(controller.rootRotation, bones.rotation[root_id.index()]), controller.rootPRS.scale);
    return;
  }
  dag::Index16 parent = tree.getParentNodeIdx(node_id);
  G_ASSERT(parent);
  calc_global_bones(controller, tree, parent, root_id, global_bones);
  calc_global_bone(global_bones[node_id.index()], bones, global_bones[parent.index()], node_id);
}

static vec3f get_current_animation_node_world_pos(
  const MotionMatchingController &controller, const GeomNodeTree &tree, dag::Index16 node_id, dag::Index16 root_id)
{
  const BoneInertialInfo &bones = controller.currentAnimation;
  vec3f v = bones.position[node_id.index()];
  for (dag::Index16 parent = tree.getParentNodeIdx(node_id); parent; parent = tree.getParentNodeIdx(parent))
  {
    if (parent == root_id)
      v = v_mul(v, controller.rootPRS.scale);
    v = v_add(bones.position[parent.index()], v_quat_mul_vec3(bones.rotation[parent.index()], v));
    if (parent == root_id)
      break;
  }
  return v_add(v_quat_mul_vec3(controller.rootRotation, v), controller.rootPosition);
}

static void update_bone(
  BoneInertialInfo &bones, dag::Index16 node_id, dag::Index16 parent_id, dag::Vector<mat44f, framemem_allocator> &global_bones)
{
  mat44f invTm, localTm;
  v_mat44_inverse43(invTm, global_bones[parent_id.index()]);
  v_mat44_mul43(localTm, invTm, global_bones[node_id.index()]);
  int nodeId = node_id.index();
  bones.rotation[nodeId] = v_quat_from_mat43(localTm);
  G_ASSERT(!v_test_xyzw_nan(bones.rotation[nodeId]));
}

static void solve_ik_two_bones(mat44f_cref bone_root,
  mat44f_cref bone_mid,
  mat44f_cref bone_end,
  vec3f target,
  vec3f fwd,
  float len0,
  float len1,
  float max_reach_scale,
  quat4f &root_rotate_out,
  quat4f &mid_rotate_out,
  vec3f &end_pos_out)
{
  // initialize with defaults in case there will be some asserts further
  root_rotate_out = mid_rotate_out = V_C_UNIT_0001;
  end_pos_out = bone_end.col3;
  G_ASSERTF_RETURN(len0 > 0.0001f && len1 > 0.0001f, , "len0=%f len1=%f", len0, len1);
  float maxExtension = (len0 + len1) * max_reach_scale;

  vec3f rootToTarget = v_sub(target, bone_root.col3);
  float rootToTargetLen = v_extract_x(v_length3_x(rootToTarget));
  if (rootToTargetLen > maxExtension)
  {
    rootToTarget = v_mul(rootToTarget, v_splats(maxExtension / rootToTargetLen));
    rootToTargetLen = maxExtension;
  }
  G_ASSERT_RETURN(rootToTargetLen > 0.0001f, );

  // triangle: A - root, B - mid, C - end. Need modify AC edge to have rootToTarget length
  // and then find new angles at A and B points. T - target.
  float lenAB = len0;
  float lenBC = len1;
  float lenAT = rootToTargetLen;

  vec3f ACnorm = v_norm3(v_sub(bone_end.col3, bone_root.col3));
  vec3f ABnorm = v_div(v_sub(bone_mid.col3, bone_root.col3), v_splats(lenAB));
  vec3f BAnorm = v_neg(ABnorm);
  vec3f BCnorm = v_div(v_sub(bone_end.col3, bone_mid.col3), v_splats(lenBC));

  vec4f cosAFromTo =
    v_perm_xaxa(v_dot3_x(ACnorm, ABnorm), v_set_x((lenAB * lenAB + lenAT * lenAT - lenBC * lenBC) / (2.0f * lenAB * lenAT)));
  vec4f cosBFromTo =
    v_perm_xaxa(v_dot3_x(BAnorm, BCnorm), v_set_x((lenAB * lenAB + lenBC * lenBC - lenAT * lenAT) / (2.0f * lenAB * lenBC)));
  vec4f anglesFromTo = v_acos(v_clamp(v_perm_xyab(cosAFromTo, cosBFromTo), v_neg(V_C_ONE), V_C_ONE));
  vec4f angleA_angleB = v_sub(v_perm_yyww(anglesFromTo), anglesFromTo);

  bool fwdCollinear = fabsf(v_extract_x(v_dot3_x(ACnorm, fwd))) > 0.99f;
  vec3f ABCnormal = v_norm3(v_cross3(ACnorm, fwdCollinear ? ABnorm : fwd));
  root_rotate_out = v_quat_from_unit_vec_ang(ABCnormal, v_splat_x(angleA_angleB));
  mid_rotate_out = v_quat_from_unit_vec_ang(ABCnormal, v_splat_z(angleA_angleB));
  end_pos_out = v_add(bone_root.col3, rootToTarget);

  vec3f ATnorm = v_div(rootToTarget, v_splats(rootToTargetLen));
  quat4f rootRotation2 = v_quat_from_unit_arc(ACnorm, ATnorm);
  root_rotate_out = v_quat_mul_quat(rootRotation2, root_rotate_out);
}

VECTORCALL static inline void v_mat_rotate_with_quat(mat44f &m, quat4f q)
{
  mat33f rotTm;
  v_mat33_from_quat(rotTm, q);
  v_mat33_mul33r(rotTm, rotTm, m);
  m.set33(rotTm);
}

static void reset_foot_ik_locker(MotionMatchingController &controller)
{
  for (FootIKLockerControllerState::LegState &state : controller.footLockState.legs)
    if (state.isLocked)
    {
      state.isLocked = false;
      state.posOffset = v_zero();
    }
}

template <typename T>
void foot_ik_locker_ecs_query(ecs::EntityId, T);

ECS_TAG(render)
ECS_BEFORE(animchar_es)
ECS_AFTER(wait_motion_matching_job_es)
static void motion_matching_apply_foot_ik_locker_es(const ParallelUpdateFrameDelayed &act,
  MotionMatchingController &motion_matching__controller,
  const AnimV20::AnimcharBaseComponent &animchar,
  ecs::EntityId motion_matching__dataBaseEid)
{
  const MotionMatchingController &controller = motion_matching__controller;
  if (controller.motionMatchingWeight == 0.0f || !controller.hasActiveAnimation())
  {
    reset_foot_ik_locker(motion_matching__controller);
    return;
  }
  G_ASSERT(controller.dataBase);

  const MotionMatchingController::CurrentClipInfo &curAnim = controller.currentClipInfo;
  const AnimationDataBase &dataBase = *controller.dataBase;
  const GeomNodeTree &tree = animchar.getNodeTree();
  if (dataBase.footLockerParamId >= 0)
    return;

  foot_ik_locker_ecs_query(motion_matching__dataBaseEid,
    [&](const FootIKLockerNodes &foot_ik_locker_nodes, float foot_ik_locker_blendTime, float foot_ik_locker_maxReachScale,
      bool foot_ik_locker_enabled, bool foot_ik_locker_inited, float foot_ik_locker_unlockByAnimRadius,
      float foot_ik_locker_unlockWhenUnreachableRadius, float foot_ik_locker_toeNodeHeight) {
      if (!foot_ik_locker_inited || !foot_ik_locker_enabled)
        return;
      for (int legId = 0; legId < foot_ik_locker_nodes.NUM_LEGS; ++legId)
      {
        FootIKLockerControllerState::LegState &state = motion_matching__controller.footLockState.legs[legId];
        const FootIKLockerNodes::LegNodes &legNodes = foot_ik_locker_nodes.legs[legId];
        bool needLock = dataBase.clips[curAnim.clip].footLockerStates.needLock(legId, curAnim.frame);

        dag::Vector<mat44f, framemem_allocator> globalBones;
        globalBones.resize(legNodes.toeNodeId.index() + 1);
        // `controller.rootPosition` offset is not applied to bones to solve IK in local coordinates.
        calc_global_bones(controller, tree, legNodes.toeNodeId, dataBase.rootNode, globalBones);

        mat44f &hip = globalBones[legNodes.hipNodeId.index()];
        mat44f &knee = globalBones[legNodes.kneeNodeId.index()];
        mat44f &heel = globalBones[legNodes.heelNodeId.index()];
        mat44f &toe = globalBones[legNodes.toeNodeId.index()];
        float len0 = v_extract_x(v_length3_x(v_sub(hip.col3, knee.col3)));
        float len1 = v_extract_x(v_length3_x(v_sub(knee.col3, heel.col3)));
        float len2 = v_extract_x(v_length3_x(v_sub(heel.col3, toe.col3)));
        vec3f toeWorldPos = v_add(toe.col3, controller.rootPosition);

        if (!state.isLocked && needLock && state.lockAllowed)
        {
          state.isLocked = true;
          vec3f lockedPos = get_current_animation_node_world_pos(controller, tree, legNodes.toeNodeId, dataBase.rootNode);
          state.lockedPosition = lockedPos;
          state.posOffset = v_sub(v_add(toeWorldPos, state.posOffset), lockedPos);
        }
        else if (state.isLocked)
        {
          float distToAnim = v_extract_x(v_length3_x(v_sub(toeWorldPos, state.lockedPosition)));

          vec3f targetPos = v_add(state.lockedPosition, state.posOffset);
          vec3f hipWorldPos = v_add(hip.col3, controller.rootPosition);
          float unreachableDist = v_extract_x(v_length3_x(v_sub(targetPos, hipWorldPos))) - (len0 + len1 + len2);
          if (!needLock || distToAnim > foot_ik_locker_unlockByAnimRadius ||
              unreachableDist > foot_ik_locker_unlockWhenUnreachableRadius)
          {
            state.isLocked = false;
            state.posOffset = v_sub(v_add(state.lockedPosition, state.posOffset), toeWorldPos);
          }
        }

        // LegsIKCtrl is enabled most of the time and will move leg along Y axis, we need only to do
        // this correction to make it work as expected.
        // TODO: In future it is better to use legs IK only in one place, here or there.
        if (state.isLocked)
          state.lockedPosition = v_insert_y(state.lockedPosition, v_extract_y(controller.rootPosition) + foot_ik_locker_toeNodeHeight);

        // Lock is possible only when previous animation frame doesn't need a lock. It prevents cycling
        // when we need lock from animation data, but immedeately unlock it by distance.
        state.lockAllowed = !needLock;
        // And for single frame animations too, because there is no previous frame with needLock=false
        if (dataBase.clips[curAnim.clip].tickDuration == 1 && !state.isLocked && needLock)
        {
          vec4f animPos = get_current_animation_node_world_pos(controller, tree, legNodes.toeNodeId, dataBase.rootNode);
          float distToAnim = v_extract_x(v_length3_x(v_sub(toeWorldPos, animPos)));
          state.lockAllowed = distToAnim < foot_ik_locker_unlockByAnimRadius;
        }

        state.posOffset = damp_adjustment_exact_v3(state.posOffset, foot_ik_locker_blendTime, act.dt);

        vec3f targetPos = v_add(state.isLocked ? v_sub(state.lockedPosition, controller.rootPosition) : toe.col3, state.posOffset);
        vec3f heelTargetPos = v_add(targetPos, v_sub(heel.col3, toe.col3));
        quat4f hipRotate, kneeRotate;
        vec3f heelPos;
        solve_ik_two_bones(hip, knee, heel, heelTargetPos, knee.col1, len0, len1, foot_ik_locker_maxReachScale, hipRotate, kneeRotate,
          heelPos);

        BoneInertialInfo &resultBones = motion_matching__controller.resultAnimation;

        // knee intentionally not affected by hip rotation here
        v_mat_rotate_with_quat(knee, kneeRotate);
        update_bone(resultBones, legNodes.kneeNodeId, legNodes.hipNodeId, globalBones);

        v_mat_rotate_with_quat(hip, hipRotate);
        update_bone(resultBones, legNodes.hipNodeId, tree.getParentNodeIdx(legNodes.hipNodeId), globalBones);

        // recalc knee wtm with both hip and knee rotations
        calc_global_bone(knee, resultBones, hip, legNodes.kneeNodeId);
        // rotate heel to look at target with toe
        vec3f heelToToeTarget = v_norm3(v_sub(targetPos, heelPos));
        vec3f heelToToe = v_div(v_sub(toe.col3, heel.col3), v_splats(len2));
        v_mat_rotate_with_quat(heel, v_quat_from_unit_arc(heelToToe, heelToToeTarget));
        update_bone(resultBones, legNodes.heelNodeId, legNodes.kneeNodeId, globalBones);
      }
    });
}

ECS_TAG(render)
ECS_BEFORE(animchar_es)
ECS_AFTER(wait_motion_matching_job_es)
static void motion_matching_update_anim_tree_foot_locker_es(const ParallelUpdateFrameDelayed &,
  AnimV20::AnimcharBaseComponent &animchar,
  float motion_matching__animationBlendTime,
  const MotionMatchingController &motion_matching__controller)
{
  const MotionMatchingController &controller = motion_matching__controller;
  if (!controller.dataBase)
    return;

  const MotionMatchingController::CurrentClipInfo &curAnim = controller.currentClipInfo;
  const AnimationDataBase &dataBase = *controller.dataBase;
  if (dataBase.footLockerParamId < 0)
    return;

  AnimV20::FootLockerIKCtrl::LegData *legsData =
    static_cast<AnimV20::FootLockerIKCtrl::LegData *>(animchar.getAnimState()->getInlinePtr(dataBase.footLockerParamId));
  for (int legNo = 0; legNo < FootIKLockerNodes::NUM_LEGS; ++legNo)
  {
    AnimV20::FootLockerIKCtrl::LegData &leg = legsData[legNo];

    if (controller.hasActiveAnimation())
    {
      const AnimationClip &clip = dataBase.clips[curAnim.clip];
      bool needLock = clip.footLockerStates.needLock(legNo, curAnim.frame);
      bool lockAllowed = true;
      // Lock is possible only when previous animation frame doesn't need a lock. It prevents cycling
      // when we need lock from animation data, but immedeately unlock it by distance.
      if (controller.lastTransitionTime > (1.f / TICKS_PER_SECOND))
        lockAllowed = !clip.footLockerStates.needLock(legNo, curAnim.frame > 0 ? curAnim.frame - 1 : clip.tickDuration - 1);
      // And for single frame animations too, because there is no previous frame with needLock=false
      if (dataBase.clips[curAnim.clip].tickDuration == 1 && needLock)
        lockAllowed = controller.lastTransitionTime >= motion_matching__animationBlendTime;

      leg.needLock = needLock && (leg.isLocked || lockAllowed);
    }
    else
      leg.needLock = false;
  }
}
