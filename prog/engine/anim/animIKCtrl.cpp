#include <anim/dag_animPostBlendCtrl.h>
#include <anim/dag_animIrq.h>
#include <animChar/dag_animCharacter2.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_Point4.h>
#include <debug/dag_fatal.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_geomTree.h>
#include <math/dag_2bonesIKSolver.h>
#include <math/dag_mathUtils.h>
#include <debug/dag_debug.h>
#include <stdlib.h>

static vec3f transform_vec_upto_root(const GeomNodeTree &tree, dag::Index16 n, vec3f v)
{
  for (; tree.getParentNodeIdx(n); n = tree.getParentNodeIdx(n))
    v = v_mat44_mul_vec3p(tree.getNodeTm(n), v);
  return v;
}

void AnimV20::LegsIKCtrl::init(IPureAnimStateHolder &st, const GeomNodeTree &tree)
{
  if (!rec.size())
    return;
  NodeId *nodes = (NodeId *)st.getInlinePtr(varId);
  for (int i = 0; i < rec.size(); i++)
  {
    auto footstep = rec[i].footStep.empty() ? dag::Index16() : tree.findINodeIndex(rec[i].footStep);
    auto foot = tree.findINodeIndex(rec[i].foot);
    auto knee = tree.findINodeIndex(rec[i].knee);
    auto leg = tree.findINodeIndex(rec[i].leg);
    nodes[i].footId = foot, nodes[i].kneeId = knee, nodes[i].legId = leg;
    nodes[i].footStepId = footstep;
    G_ASSERTF_AND_DO(nodes[i].footId && nodes[i].kneeId && nodes[i].legId && (nodes[i].footStepId || rec[i].footStep.empty()),
      nodes[i].footId = nodes[i].kneeId = nodes[i].legId = dag::Index16(), "LegsIKCtrl misses nodes:  %d,%d,%d,%s (%s,%s,%s,%s)",
      (int)foot, (int)knee, (int)leg, (int)footstep, rec[i].foot, rec[i].knee, rec[i].leg, rec[i].footStep);
    nodes[i].dy = nodes[i].da = nodes[i].dt = 0;
  }
}
void AnimV20::LegsIKCtrl::advance(IPureAnimStateHolder &st, real dt)
{
  NodeId *nodes = (NodeId *)st.getInlinePtr(varId);
  for (int i = 0; i < rec.size(); i++)
    nodes[i].dt = dt;
}
void AnimV20::LegsIKCtrl::process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx)
{
  if (wt < 1e-3 || tree.empty())
    return;
  NodeId *nodes = (NodeId *)st.getInlinePtr(varId);

  G_ASSERT_AND_DO(rec.size(), return);

  tree.calcWtm();
  vec3f wofs = v_add(ctx.worldTranslate, ctx.ac->getWtmOfs());
  for (int i = 0; i < rec.size(); i++)
  {
    if (!nodes[i].footId)
      continue;
    auto foot = nodes[i].footId;
    auto foot_parent = tree.getParentNodeIdx(foot);
    mat44f &foot_wtm = tree.getNodeWtmRel(foot);
    mat44f &foot_tm = tree.getNodeTm(foot);
    mat44f &leg_wtm = tree.getNodeWtmRel(nodes[i].legId);
    vec3f foot_p1 = transform_vec_upto_root(tree, foot_parent, foot_tm.col3);
    Point3_vec4 footNewPos(0.f, 0.f, 0.f);
    bool move_foot = false;
    const mat44f &animcharTm = tree.getRootWtmRel();
    Point3_vec4 up = Point3_vec4(0, 1, 0);
    if (rec[i].useAnimcharUpDir)
      up = as_point3(&animcharTm.col1);
    bool up_is_world_up = up.y > 0.999f;

    if (isCrawl)
    {
      auto knee_parent = tree.getParentNodeIdx(nodes[i].kneeId);
      mat44f &knee_tm = tree.getNodeTm(nodes[i].kneeId);
      vec3f knee_p = transform_vec_upto_root(tree, knee_parent, knee_tm.col3);
      mat44f animcharTmCorrected = animcharTm;
      animcharTmCorrected.col0 = animcharTm.col2;
      animcharTmCorrected.col2 = animcharTm.col0;
      Point3_vec4 dir = as_point3(&animcharTm.col2);
      Point3_vec4 kneeOffset = crawlKneeOffsetVec;
      kneeOffset[2] *= i == 0 ? -1 : 1;
      v_st(&kneeOffset, v_mat44_mul_vec3p(animcharTmCorrected, v_ldu(&kneeOffset.x)));
      vec4f wofsVec4 = v_and(wofs, v_cast_vec4f(V_CI_MASK1110));
      Point3_vec4 pt;
      float maxDist = crawlMaxRay;
      v_st(&pt, v_add(v_mat44_mul_vec3p(tree.getRootWtmRel(), v_and(knee_p, v_cast_vec4f(V_CI_MASK1110))), wofs));
      pt += kneeOffset;
      if (ctx.irq(GIRQT_TraceFootStepDir, (intptr_t)(void *)&pt, (intptr_t)(void *)&dir, (intptr_t)(void *)&maxDist) == GIRQR_TraceOK)
      {
        footNewPos = pt + dir * (maxDist - crawlFootOffset) - as_point3(&wofsVec4);
        nodes[i].da = crawlFootAngle + acosf((crawlMaxRay - maxDist) / crawlMaxRay);
        move_foot = true;
      }
    }
    else
    {
      vec3f foot_p2 = transform_vec_upto_root(tree, foot, rec[i].vFootFwd);
      if (nodes[i].footStepId)
      {
        vec3f foot_p1s =
          transform_vec_upto_root(tree, tree.getParentNodeIdx(nodes[i].footStepId), tree.getNodeTm(nodes[i].footStepId).col3);
        vec3f new_foot_p1 = v_perm_xbzw(foot_p1s, foot_p1);
        if (v_extract_x(v_length3_sq(v_sub(new_foot_p1, foot_p1))) > 1e-6)
        {
          foot_p2 = v_madd(v_sub(new_foot_p1, foot_p1), v_add(V_C_UNIT_1000, V_C_UNIT_0010), foot_p2);
          foot_p1 = new_foot_p1;
          move_foot = true;
        }
      }
      float foot_h1 = v_extract_y(foot_p1);
      // float foot_h2 = v_extract_y(foot_p2);
      float move_wt = clamp<float>((rec[i].footStepRange[1] - foot_h1) * rec[i].footStepRange[0], 0.0f, 1.0f);
      float foot_len = as_point4(&rec[i].vFootFwd).w;

      float trace_h1 = 0, trace_h2 = 0;
      float leg_height = as_point3(&leg_wtm.col3) * up;
      float foot_height = as_point3(&foot_wtm.col3) * up;
      if (leg_height > foot_height)
      {
        float maxDist = rec[i].maxFootUp + 0.15;
        float footUpOfs = min(rec[i].maxFootUp, (leg_height - foot_height - 0.15f) * 0.9f);

        Point3_vec4 pt;
        Point3_vec4 dir = -up;
        v_st(&pt, v_add(v_mat44_mul_vec3p(animcharTm, v_and(foot_p1, v_cast_vec4f(V_CI_MASK1010))), wofs));
        pt += footUpOfs * up;
        trace_h1 = maxDist;
        if (ctx.irq(up_is_world_up ? GIRQT_TraceFootStepDown : GIRQT_TraceFootStepDir, (intptr_t)(void *)&pt,
              (intptr_t)(up_is_world_up ? (void *)&maxDist : (void *)&dir), (intptr_t)(void *)&trace_h1) == GIRQR_TraceOK)
          trace_h1 -= footUpOfs;

        v_st(&pt, v_add(v_mat44_mul_vec3p(animcharTm, v_and(foot_p2, v_cast_vec4f(V_CI_MASK1010))), wofs));
        pt += footUpOfs * up;

        trace_h2 = maxDist;
        if (ctx.irq(up_is_world_up ? GIRQT_TraceFootStepDown : GIRQT_TraceFootStepDir, (intptr_t)(void *)&pt,
              (intptr_t)(up_is_world_up ? (void *)&maxDist : (void *)&dir), (intptr_t)(void *)&trace_h2) == GIRQR_TraceOK)
          trace_h2 -= footUpOfs;
        else
          trace_h2 = trace_h1;
      }

      float dh = trace_h2 - trace_h1;
      if (!move_wt)
        dh = 0;
      else if (dh > foot_len / 2)
        dh = foot_len / 2;
      else if (dh < -foot_len / 2)
        dh = -foot_len / 2;

      trace_h2 -= dh;

      float gnd_lev = trace_h1;
      if (trace_h2 < 0 && trace_h2 < gnd_lev)
        gnd_lev = trace_h2;
      if (gnd_lev < 0)
        move_wt = 1.0f;
      float new_dy = 0;
      float new_da = 0;
      if (move_wt > 0)
      {
        new_dy = -gnd_lev * move_wt * wt;
        if (fabsf(dh) > 1e-5)
          new_da = clamp(-asinf(dh / foot_len), rec[i].footRotRange[0], rec[i].footRotRange[1]) * move_wt * wt;
      }

      {
        float dt = nodes[i].dt;
        float max_dy = dt * rec[i].maxDyRate;
        float max_da = dt * rec[i].maxDaRate;
        nodes[i].dy += clamp(new_dy - nodes[i].dy, -max_dy, max_dy);
        nodes[i].da += clamp(new_da - nodes[i].da, -max_da, max_da);
      }
    }

    if (alwaysSolve || nodes[i].dy || move_foot)
    {
      mat44f &knee_wtm = tree.getNodeWtmRel(nodes[i].kneeId);
      float len0 = v_extract_x(v_length3_x(v_sub(leg_wtm.col3, knee_wtm.col3)));
      float len1 = v_extract_x(v_length3_x(v_sub(foot_wtm.col3, knee_wtm.col3)));
      if (isCrawl)
        as_point3(&foot_wtm.col3) = footNewPos;
      else
      {
        if (move_foot)
          foot_wtm.col3 = foot_p1;
        as_point3(&foot_wtm.col3) += nodes[i].dy * up;
      }
      solve_2bones_ik(leg_wtm, knee_wtm, foot_wtm, foot_wtm, len0, len1, as_point3(&knee_wtm.col1));
      if (ctx.acScale)
      {
        vec3f s = *ctx.acScale;
        leg_wtm.col0 = v_mul(leg_wtm.col0, s);
        leg_wtm.col1 = v_mul(leg_wtm.col1, s);
        leg_wtm.col2 = v_mul(leg_wtm.col2, s);
        knee_wtm.col0 = v_mul(knee_wtm.col0, s);
        knee_wtm.col1 = v_mul(knee_wtm.col1, s);
        knee_wtm.col2 = v_mul(knee_wtm.col2, s);
      }
      tree.markNodeTmInvalid(nodes[i].legId);
      tree.markNodeTmInvalid(nodes[i].kneeId);
      tree.markNodeTmInvalid(nodes[i].footId);
      tree.validateTm();
      if (fabsf(nodes[i].da) > 1e-5)
      {
        mat44f tm = foot_tm;
        mat33f rm;
        v_mat33_make_rot_cw(rm, rec[i].vFootSide, v_splat4(&nodes[i].da));
        v_mat44_mul33(foot_tm, tm, rm);
        v_mat44_mul43(foot_wtm, tree.getNodeWtmRel(foot_parent), foot_tm);
      }
      tree.invalidateWtm(nodes[i].footId);
      tree.calcWtm();
    }
  }
}

void AnimV20::LegsIKCtrl::createNode(AnimationGraph &graph, const DataBlock &blk)
{
  const char *name = blk.getStr("name", NULL);
  G_ASSERTF_AND_DO(name != NULL, return, "not found '%s' param", "name");

  LegsIKCtrl *node = new LegsIKCtrl(graph);
  const DataBlock &def = *blk.getBlockByNameEx("def");
  for (int j = 0, leg_nid = blk.getNameId("leg"); j < blk.blockCount(); j++)
    if (blk.getBlock(j)->getBlockNameId() == leg_nid)
    {
#define GET_PROP(TYPE, NM, DEF) b.get##TYPE(NM, def.get##TYPE(NM, DEF))
      const DataBlock &b = *blk.getBlock(j);
      IKRec &r = node->rec.push_back();
      r.footStep = GET_PROP(Str, "foot_step", NULL);
      r.foot = GET_PROP(Str, "foot", NULL);
      r.knee = GET_PROP(Str, "knee", NULL);
      r.leg = GET_PROP(Str, "leg", NULL);

      r.footStepRange = GET_PROP(Point2, "footStepRange", Point2(0.0f, 0.1f));
      if (r.footStepRange[1] > r.footStepRange[0])
        r.footStepRange[0] = 1.0f / (r.footStepRange[1] - r.footStepRange[0]);
      else
        G_ASSERTF_AND_DO(r.footStepRange[1] > r.footStepRange[0], r.footStepRange[0] = 0, "footStepRange=%@", r.footStepRange);
      r.footRotRange = GET_PROP(Point2, "footRotRange", Point2(-30.f, 30.f)) * DEG_TO_RAD;
      r.maxFootUp = GET_PROP(Real, "maxFootUp", 0.5f);

      Point3 v = GET_PROP(Point3, "footFwd", Point3(1, 0, 0));
      r.vFootFwd = v_make_vec4f(v.x, v.y, v.z, v.length());
      v = GET_PROP(Point3, "footSide", Point3(1, 0, 0));
      r.vFootSide = v_make_vec4f(v.x, v.y, v.z, 0);

      r.maxDyRate = GET_PROP(Real, "maxDyRate", 0.9);             // m/s
      r.maxDaRate = DEG_TO_RAD * GET_PROP(Real, "maxDaRate", 60); // deg/s

      r.useAnimcharUpDir = GET_PROP(Bool, "useAnimcharUpDir", false);

      G_ASSERTF_AND_DO(!r.foot.empty() && !r.knee.empty() && !r.leg.empty(), node->rec.pop_back(), "foot=<%s> knee=<%s> leg=<%s>",
        r.foot, r.knee, r.leg);
#undef GET_PROP
    }
  G_ASSERTF(node->rec.size(), "no legs?");

  node->alwaysSolve = blk.getBool("alwaysSolve", false);
  node->isCrawl = blk.getBool("isCrawl", false);
  node->crawlKneeOffsetVec = blk.getPoint3("crawlKneeOffsetVec", Point3(0, 0.2, 0.1));
  node->crawlFootOffset = blk.getReal("crawlFootOffset", 0.15f);
  node->crawlFootAngle = blk.getReal("crawlFootAngle", 130.f);
  node->crawlMaxRay = blk.getReal("crawlMaxRay", 0.5f);
  node->varId =
    graph.addInlinePtrParamId(String(0, "$%s", name), sizeof(NodeId) * node->rec.size(), IPureAnimStateHolder::PT_InlinePtr);
  graph.registerBlendNode(node, name);
}
