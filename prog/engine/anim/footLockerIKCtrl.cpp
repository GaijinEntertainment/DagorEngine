// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <anim/dag_animPostBlendCtrl.h>
#include <anim/dag_animIrq.h>
#include <animChar/dag_animCharacter2.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_geomTree.h>
#include <math/dag_mathUtils.h>
#include <memory/dag_framemem.h>

void AnimV20::FootLockerIKCtrl::init(IPureAnimStateHolder &st, const GeomNodeTree &tree)
{
  if (legsNodeNames.empty())
    return;
  LegData *legsData = static_cast<LegData *>(st.getInlinePtr(legsDataVarId));
  dag::Index16 hipParent;
  for (int legNo = 0, numLegs = legsNodeNames.size(); legNo < numLegs; legNo++)
  {
    const LegNodeNames &legNames = legsNodeNames[legNo];
    LegData &leg = legsData[legNo];
    leg.hipNodeId = tree.findINodeIndex(legNames.hip);
    leg.kneeNodeId = tree.findINodeIndex(legNames.knee);
    leg.ankleNodeId = tree.findINodeIndex(legNames.ankle);
    leg.toeNodeId = tree.findINodeIndex(legNames.toe);
    if (legNo == 0)
      hipParent = tree.getParentNodeIdx(leg.hipNodeId);

    leg.lockedPosition = Point3::ZERO;
    leg.posOffset = Point3::ZERO;
    leg.isLocked = false;
    leg.needLock = false;

    G_ASSERTF_AND_DO(leg.hipNodeId && leg.kneeNodeId && leg.ankleNodeId && leg.toeNodeId,
      leg.hipNodeId = leg.kneeNodeId = leg.ankleNodeId = leg.toeNodeId = dag::Index16(),
      "FootLockerIKCtrl misses nodes:  %d,%d,%d,%d (%s,%s,%s,%s)", leg.hipNodeId.index(), leg.kneeNodeId.index(),
      leg.ankleNodeId.index(), leg.toeNodeId.index(), legNames.hip, legNames.knee, legNames.ankle, legNames.toe);
    G_ASSERTF(tree.getParentNodeIdx(leg.toeNodeId) == leg.ankleNodeId && tree.getParentNodeIdx(leg.ankleNodeId) == leg.kneeNodeId &&
                tree.getParentNodeIdx(leg.kneeNodeId) == leg.hipNodeId,
      "wrong nodes hierarchy (%s -> %s -> %s -> %s)", legNames.hip, legNames.knee, legNames.ankle, legNames.toe);
    G_ASSERTF(hipMoveDownVarId < 0 || hipParent == tree.getParentNodeIdx(leg.hipNodeId), "hip parent node is not same (%s != %s)",
      tree.getNodeName(hipParent), tree.getNodeName(tree.getParentNodeIdx(leg.hipNodeId)));
  }
}

static void solve_ik_two_bones(mat44f_cref bone_root, mat44f_cref bone_mid, mat44f_cref bone_end, vec3f target, vec3f fwd, float len0,
  float len1, float max_reach_scale, quat4f &root_rotate_out, quat4f &mid_rotate_out, vec3f &end_pos_out)
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

void AnimV20::FootLockerIKCtrl::process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx)
{
  TIME_PROFILE_DEV(FootLockerIKCtrl);

  int numLegs = legsNodeNames.size();
  LegData *legsData = static_cast<LegData *>(st.getInlinePtr(legsDataVarId));
  if (wt < 1e-3f)
  {
    for (int i = 0; i < numLegs; ++i)
      legsData[i].isLocked = false;
    return;
  }

  float hipMoveDown = hipMoveDownVarId >= 0 ? st.getParam(hipMoveDownVarId) : 0.f;
  if (hipMoveDown > 0.f)
  {
    dag::Index16 pelvisNode = tree.getParentNodeIdx(legsData[0].hipNodeId);
    mat44f &pelvis = tree.getNodeWtmRel(pelvisNode);
    pelvis.col3 = v_madd(V_C_UNIT_0100, v_splats(-hipMoveDown * wt), pelvis.col3);
    tree.markNodeTmInvalid(pelvisNode);
    tree.validateTm();
    tree.invalidateWtm(pelvisNode);
  }
  tree.calcWtm();
  vec3f worldOffset = v_add(ctx.worldTranslate, ctx.ac->getWtmOfs());

  dag::RelocatableFixedVector<AnimCharV20::LegsIkRay, 4, true /*OF*/, framemem_allocator> traces;
  traces.resize(numLegs * 2);
  memset(traces.data(), 0, sizeof(AnimCharV20::LegsIkRay) * traces.size());

  constexpr float TOE_UNDER_HIP_LIMIT = 0.2f;
  float dt = st.getParam(AnimationGraph::PID_GLOBAL_LAST_DT);
  for (int legNo = 0; legNo < numLegs; legNo++)
  {
    LegData &leg = legsData[legNo];
    int toeTraceIdx = 2 * legNo;
    int ankleTraceIdx = toeTraceIdx + 1;
    vec3f toePos = tree.getNodeWtmRel(leg.toeNodeId).col3;
    vec3f anklePos = tree.getNodeWtmRel(leg.ankleNodeId).col3;
    vec3f hipPos = tree.getNodeWtmRel(leg.hipNodeId).col3;
    if (!leg.isLocked)
    {
      // Do not allow to move the foot too high, close to the hip
      float traceFromY = min(v_extract_y(hipPos) - TOE_UNDER_HIP_LIMIT, v_extract_y(toePos) + maxFootUp) + hipMoveDown;
      vec3f posOffset = v_ldu(&leg.posOffset.x);
      vec3f toeTraceFrom = v_perm_xbzw(v_add(toePos, posOffset), v_splats(traceFromY));
      float traceDist = traceFromY - v_extract_y(toePos) + maxFootDown;
      v_stu(&traces[toeTraceIdx].pos.x, v_add(toeTraceFrom, worldOffset));
      traces[toeTraceIdx].t = traceDist;
      traces[toeTraceIdx].dir = Point3(0, -1, 0);
      traces[toeTraceIdx].legsDiff = traceDist; // `legsDiff` is not used, just store max trace distance here
      vec3f ankleTraceFrom = v_perm_xbzw(v_add(anklePos, posOffset), v_splats(traceFromY));
      v_stu(&traces[ankleTraceIdx].pos.x, v_add(ankleTraceFrom, worldOffset));
      traces[ankleTraceIdx].t = traceDist;
      traces[ankleTraceIdx].dir = Point3(0, -1, 0);
    }
    else
    {
      traces[toeTraceIdx].t = -1.f;
      traces[ankleTraceIdx].t = -1.f;
    }
  }

  intptr_t traceRes = ctx.irq(GIRQT_TraceFootStepMultiRay, (intptr_t)traces.data(), traces.size(), (intptr_t) true /*isTraceDown*/);
  float hipTargetMove = 0.f;

  for (int legNo = 0; legNo < numLegs; legNo++)
  {
    LegData &leg = legsData[legNo];

    mat44f &hip = tree.getNodeWtmRel(leg.hipNodeId);
    mat44f &knee = tree.getNodeWtmRel(leg.kneeNodeId);
    mat44f &ankle = tree.getNodeWtmRel(leg.ankleNodeId);
    mat44f &toe = tree.getNodeWtmRel(leg.toeNodeId);
    float len0 = v_extract_x(v_length3_x(v_sub(hip.col3, knee.col3)));
    float len1 = v_extract_x(v_length3_x(v_sub(knee.col3, ankle.col3)));
    float len2 = v_extract_x(v_length3_x(v_sub(ankle.col3, toe.col3)));

    int toeTraceIdx = 2 * legNo;
    int ankleTraceIdx = toeTraceIdx + 1;

    float maxTraceDist = traces[toeTraceIdx].legsDiff;
    bool toeTraceValid = traceRes == GIRQR_TraceOK && traces[toeTraceIdx].t < maxTraceDist;
    if (!leg.isLocked)
    {
      float worldOffsetY = v_extract_y(worldOffset);
      float toeTraceHitY = (traces[toeTraceIdx].pos.y - worldOffsetY) - traces[toeTraceIdx].t;

      float footVerticalMove = 0;
      // We want to move the leg down when it is close to the ground in animation and the ground under the trace
      // is lower than character root. Otherwise we will at first pause leg in air because of foot placement animation
      // and then will move leg again to the locked point on ground.
      float rootY = v_extract_y(tree.getRootWtmRel().col3);
      float moveDownStrength = cvt(v_extract_y(toe.col3) - rootY, toeNodeHeight, 0.05f, 0.f, 1.f);
      if (toeTraceValid && moveDownStrength > 0.f && toeTraceHitY < rootY)
        footVerticalMove = (toeTraceHitY - rootY) * moveDownStrength;

      // Move up toe and ankle nodes to prevent intersections with ground.
      float toeMoveUp = max(toeTraceHitY + toeNodeHeight - (v_extract_y(toe.col3) + footVerticalMove), 0.f);
      float ankleTraceHitY = (traces[ankleTraceIdx].pos.y - worldOffsetY) - traces[ankleTraceIdx].t;
      float ankleMoveUp = max(ankleTraceHitY + ankleNodeHeight - (v_extract_y(ankle.col3) + footVerticalMove), 0.f);

      // If we need to move toe and ankle on different height, then incline our foot first.
      leg.ankleTargetMove = clamp(ankleMoveUp - toeMoveUp, -maxToeMoveUp, maxToeMoveDown);
      ankleMoveUp -= leg.ankleVerticalMove;

      footVerticalMove += max(toeMoveUp, ankleMoveUp);
      // When `footVerticalMove` is positive it means that leg is under the ground and we want to raise it faster
      // than lock/unlock cases.
      if (footVerticalMove > 0.f)
        leg.posOffset.y = move_to(leg.posOffset.y, footVerticalMove, dt, footRaisingSpeed);
      else
        leg.posOffset.y = approach(leg.posOffset.y, footVerticalMove, dt, unlockViscosity);
      leg.posOffset.x = approach(leg.posOffset.x, 0.f, dt, unlockViscosity);
      leg.posOffset.z = approach(leg.posOffset.z, 0.f, dt, unlockViscosity);
    }
    else // if (leg.isLocked)
    {
      // leg.posOffset.x and .z are zeroes
      leg.posOffset.y = approach(leg.posOffset.y, 0.f, dt, unlockViscosity);
    }
    leg.ankleVerticalMove = approach(leg.ankleVerticalMove, leg.ankleTargetMove, dt, footInclineViscosity);

    // Do not lock when we didn't find ground or we are standing on ground only with heel
    bool lockAllowed = toeTraceValid && leg.ankleTargetMove < maxToeMoveDown - 0.01f;
    if (!leg.isLocked && leg.needLock && lockAllowed)
    {
      leg.isLocked = true;
      vec3f lockedPos = v_madd(V_C_UNIT_0100, v_splats(toeNodeHeight - traces[toeTraceIdx].t), v_ldu(&traces[toeTraceIdx].pos.x));
      v_stu_p3(&leg.lockedPosition.x, lockedPos);
      vec3f lockedPosRel = v_sub(lockedPos, worldOffset);
      v_stu_p3(&leg.posOffset.x, v_sub(v_add(toe.col3, v_ldu(&leg.posOffset.x)), lockedPosRel));
      // Recalculate `ankleTargetMove` because now we want to place foot on ground entirely, instead of only
      // preventing intersections with ground.
      leg.ankleTargetMove = clamp(traces[toeTraceIdx].t - traces[ankleTraceIdx].t, -maxToeMoveUp, maxToeMoveDown);
    }
    else if (leg.isLocked)
    {
      vec3f lockedPosRel = v_sub(v_ldu(&leg.lockedPosition.x), worldOffset);
      float horzDist = v_extract_x(v_length3_x(v_and(v_sub(lockedPosRel, toe.col3), V_CI_MASK1010)));
      float unreachableDist = v_extract_x(v_length3_x(v_sub(lockedPosRel, hip.col3))) - (len0 + len1 + len2);
      bool toeTooHigh = v_extract_y(v_sub(lockedPosRel, hip.col3)) > -TOE_UNDER_HIP_LIMIT;
      if (!leg.needLock || horzDist > unlockRadius || unreachableDist > unlockWhenUnreachableRadius || toeTooHigh)
      {
        leg.isLocked = false;
        v_stu_p3(&leg.posOffset.x, v_sub(v_add(lockedPosRel, v_ldu(&leg.posOffset.x)), toe.col3));
      }
    }

    vec3f targetPos = v_add(leg.isLocked ? v_sub(v_ldu(&leg.lockedPosition.x), worldOffset) : toe.col3, v_ldu(&leg.posOffset.x));
    vec3f toeToAnkle = v_norm3(v_madd(V_C_UNIT_0100, v_splats(leg.ankleVerticalMove), v_sub(ankle.col3, toe.col3)));
    vec3f ankleTargetPos = v_add(targetPos, v_mul(toeToAnkle, v_splats(len2)));
    if (hipMoveDownVarId >= 0)
    {
      float maxIKLen = (len0 + len1) * maxReachScale;
      vec3f targetToHip = v_sub(hip.col3, ankleTargetPos);
      float horzDistSq = v_extract_x(v_length2_sq_x(v_perm_xzxz(targetToHip)));
      if (horzDistSq < maxIKLen * maxIKLen)
      {
        float maxVerticalLen = sqrtf(maxIKLen * maxIKLen - horzDistSq);
        hipTargetMove = max(hipTargetMove, v_extract_y(targetToHip) - maxVerticalLen + hipMoveDown);
      }
    }
    quat4f hipRotate, kneeRotate;
    vec3f anklePos;
    solve_ik_two_bones(hip, knee, ankle, ankleTargetPos, knee.col1, len0, len1, maxReachScale, hipRotate, kneeRotate, anklePos);

    if (wt < 1)
    {
      hipRotate = v_quat_qslerp(wt, V_C_UNIT_0001, hipRotate);
      kneeRotate = v_quat_qslerp(wt, V_C_UNIT_0001, kneeRotate);
    }
    v_mat_rotate_with_quat(hip, hipRotate);
    v_mat_rotate_with_quat(knee, v_quat_mul_quat(hipRotate, kneeRotate));
    knee.col3 = v_mat44_mul_vec3p(hip, tree.getNodeTm(leg.kneeNodeId).col3);
    // rotate foot to look at target with toe
    vec3f ankleToToeTarget = v_norm3(v_sub(targetPos, anklePos));
    vec3f ankleToToe = v_div(v_sub(toe.col3, ankle.col3), v_splats(len2));
    quat4f ankleRotate = v_quat_from_unit_arc(ankleToToe, ankleToToeTarget);
    if (wt < 1)
    {
      ankleRotate = v_quat_qslerp(wt, V_C_UNIT_0001, ankleRotate);
      anklePos = v_mat44_mul_vec3p(knee, tree.getNodeTm(leg.ankleNodeId).col3);
    }
    v_mat_rotate_with_quat(ankle, ankleRotate);
    ankle.col3 = anklePos;

    vec3f ankleToKnee = v_norm3(v_sub(knee.col3, anklePos));
    float ankleAngleCos = v_extract_x(v_dot3_x(ankleToKnee, ankleToToeTarget));
    if (leg.isLocked && ankleAngleCos > maxAnkleAnlgeCos)
    {
      leg.ankleTargetMove = max(leg.ankleTargetMove, leg.ankleVerticalMove + (ankleAngleCos - maxAnkleAnlgeCos) * 0.1f);
    }

    tree.markNodeTmInvalid(leg.hipNodeId);
    tree.markNodeTmInvalid(leg.kneeNodeId);
    tree.markNodeTmInvalid(leg.ankleNodeId);
    tree.validateTm();

    tree.invalidateWtm(leg.ankleNodeId);
    tree.calcWtm();
  }

  if (hipMoveDownVarId >= 0)
  {
    if (hipMoveDown > hipTargetMove)
    {
      // add small offset to keep space for additive breath animation
      float hipMoveThreshold = min(hipTargetMove + 0.015f, hipTargetMove * 2.f);
      hipTargetMove = min(hipMoveThreshold, hipMoveDown);
    }
    hipTargetMove = min(hipTargetMove, maxHipMoveDown);
    hipMoveDown = approach(hipMoveDown, hipTargetMove, dt, hipMoveViscosity);
    st.setParam(hipMoveDownVarId, hipMoveDown);
  }
}

void AnimV20::FootLockerIKCtrl::createNode(AnimationGraph &graph, const DataBlock &blk)
{
  const char *name = blk.getStr("name", nullptr);
  G_ASSERTF_RETURN(name, , "not found 'name' param");

  FootLockerIKCtrl *node = new FootLockerIKCtrl(graph);
  for (int i = 0, leg_nid = blk.getNameId("leg"); i < blk.blockCount(); i++)
    if (blk.getBlock(i)->getBlockNameId() == leg_nid)
    {
      const DataBlock &legBlk = *blk.getBlock(i);
      LegNodeNames &legNodes = node->legsNodeNames.push_back();
      legNodes.hip = legBlk.getStr("hip", nullptr);
      legNodes.knee = legBlk.getStr("knee", nullptr);
      legNodes.ankle = legBlk.getStr("ankle", nullptr);
      legNodes.toe = legBlk.getStr("toe", nullptr);

      G_ASSERTF_AND_DO(!legNodes.hip.empty() && !legNodes.knee.empty() && !legNodes.ankle.empty() && !legNodes.toe.empty(),
        node->legsNodeNames.pop_back(), "hip=<%s> knee=<%s> ankle=<%s> toe=<%s>", legNodes.hip, legNodes.knee, legNodes.ankle,
        legNodes.toe);
    }
  int numLegs = node->legsNodeNames.size();
  G_ASSERTF(numLegs > 0, "no legs?");
  node->legsNodeNames.shrink_to_fit();

  node->unlockViscosity = blk.getReal("unlockViscosity", 0.1f / M_LN2);
  node->maxReachScale = blk.getReal("maxReachScale", 1.0f);
  node->unlockRadius = blk.getReal("unlockRadius", 0.2f);
  node->unlockWhenUnreachableRadius = blk.getReal("unlockWhenUnreachableRadius", 0.05f);
  node->toeNodeHeight = blk.getReal("toeNodeHeight", 0.025f);
  node->ankleNodeHeight = blk.getReal("ankleNodeHeight", 0.13f);
  node->maxFootUp = blk.getReal("maxFootUp", 0.5f);
  node->maxFootDown = blk.getReal("maxFootDown", 0.15f);
  node->maxToeMoveUp = blk.getReal("maxToeMoveUp", 0.1f);
  node->maxToeMoveDown = blk.getReal("maxToeMoveDown", 0.15f);
  node->footRaisingSpeed = blk.getReal("footRaisingSpeed", 2.5f);
  node->footInclineViscosity = blk.getReal("footInclineViscosity", 0.15f);
  node->maxAnkleAnlgeCos = cosf(DegToRad(blk.getReal("maxAnkleAnlge", 60.f)));
  node->hipMoveViscosity = blk.getReal("hipMoveViscosity", 0.1f);
  node->maxHipMoveDown = blk.getReal("maxHipMoveDown", 0.15f);
  node->legsDataVarId =
    graph.addInlinePtrParamId(String(0, "$%s", name), sizeof(LegData) * numLegs, IPureAnimStateHolder::PT_InlinePtr);
  if (const char *paramName = blk.getStr("hipMoveParamName", nullptr))
    node->hipMoveDownVarId = graph.addParamId(paramName, IPureAnimStateHolder::PT_ScalarParam);
  graph.registerBlendNode(node, name);
}
