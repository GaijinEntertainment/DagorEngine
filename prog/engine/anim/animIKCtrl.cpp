// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <anim/dag_animPostBlendCtrl.h>
#include <anim/dag_animIrq.h>
#include <animChar/dag_animCharacter2.h>
#include <memory/dag_framemem.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_Point4.h>
#include <debug/dag_fatal.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_geomTree.h>
#include <math/dag_2bonesIKSolver.h>
#include <perfMon/dag_statDrv.h>
#include <math/dag_mathUtils.h>
#include <debug/dag_debug.h>
#include <stdlib.h>
#include <generic/dag_relocatableFixedVector.h>

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
  if (wt < 1e-3f || tree.empty())
    return;

  TIME_PROFILE_DEV(LegsIKCtrl);

  NodeId *nodes = (NodeId *)st.getInlinePtr(varId);

  G_ASSERT_AND_DO(rec.size(), return);

  tree.calcWtm();
  vec3f wofs = v_add(ctx.worldTranslate, ctx.ac->getWtmOfs());
  dag::RelocatableFixedVector<AnimCharV20::LegsIkRay, 8, true /*OF*/, framemem_allocator> traces;
  traces.resize(isCrawl ? rec.size() : rec.size() * 2);
  memset(traces.data(), 0, sizeof(AnimCharV20::LegsIkRay) * traces.size());
  bool isTraceDown = !isCrawl;
  for (int i = 0; i < rec.size(); i++)
  {
    if (!nodes[i].footId)
      continue;
    auto foot = nodes[i].footId;
    auto foot_parent = tree.getParentNodeIdx(foot);
    const mat44f &foot_wtm = tree.getNodeWtmRel(foot);
    const mat44f &foot_tm = tree.getNodeTm(foot);
    const mat44f &leg_wtm = tree.getNodeWtmRel(nodes[i].legId);
    vec3f foot_p1 = transform_vec_upto_root(tree, foot_parent, foot_tm.col3);
    const mat44f &animcharTm = tree.getRootWtmRel();
    vec4f vup = V_C_UNIT_0100;
    if (rec[i].useAnimcharUpDir)
    {
      vup = v_norm3(animcharTm.col1);
      isTraceDown &= v_extract_y(vup) > 0.999f;
    }

    if (isCrawl)
    {
      auto knee_parent = tree.getParentNodeIdx(nodes[i].kneeId);
      mat44f &knee_tm = tree.getNodeTm(nodes[i].kneeId);
      vec3f knee_p = transform_vec_upto_root(tree, knee_parent, knee_tm.col3);
      mat44f animcharTmCorrected = animcharTm;
      animcharTmCorrected.col0 = animcharTm.col2;
      animcharTmCorrected.col2 = animcharTm.col0;
      vec3f dir = v_norm3(animcharTm.col2);
      Point3_vec4 kneeOffset = crawlKneeOffsetVec;
      kneeOffset[2] *= i == 0 ? -1 : 1;
      vec3f pt = v_add(v_mat44_mul_vec3p(tree.getRootWtmRel(), v_and(knee_p, v_cast_vec4f(V_CI_MASK1110))), wofs);
      pt = v_add(pt, v_mat44_mul_vec3p(animcharTmCorrected, v_ldu(&kneeOffset.x)));
      v_st(&traces[i].pos.x, v_perm_xyzd(pt, v_splats(crawlMaxRay)));
      v_st(&traces[i].dir.x, v_perm_xyzd(dir, v_zero()));
      v_st(&traces[i].footP1.x, v_perm_xyzd(foot_p1, v_zero()));
    }
    else // !isCrawl
    {
      bool move_foot = false;
      vec3f foot_p2 = transform_vec_upto_root(tree, foot, rec[i].vFootFwd);
      if (nodes[i].footStepId)
      {
        vec3f foot_p1s =
          transform_vec_upto_root(tree, tree.getParentNodeIdx(nodes[i].footStepId), tree.getNodeTm(nodes[i].footStepId).col3);
        vec3f new_foot_p1 = v_perm_xbzw(foot_p1s, foot_p1);
        if (v_extract_x(v_length3_sq(v_sub(new_foot_p1, foot_p1))) > 1e-6f)
        {
          foot_p2 = v_madd(v_sub(new_foot_p1, foot_p1), v_add(V_C_UNIT_1000, V_C_UNIT_0010), foot_p2);
          foot_p1 = new_foot_p1;
          move_foot = true;
        }
      }
      float leg_diff = v_extract_x(v_sub_x(v_dot3_x(leg_wtm.col3, vup), v_dot3_x(foot_wtm.col3, vup)));
      if (leg_diff > 0.f)
      {
        float maxDist = rec[i].maxFootUp + 0.15f;
        float footUpOfs = min(rec[i].maxFootUp, (leg_diff + 0.15f) * 0.9f);
        vec3f tr1 =
          v_madd(vup, v_splats(footUpOfs), v_add(v_mat44_mul_vec3p(animcharTm, v_and(foot_p1, v_cast_vec4f(V_CI_MASK1010))), wofs));
        vec3f tr2 =
          v_madd(vup, v_splats(footUpOfs), v_add(v_mat44_mul_vec3p(animcharTm, v_and(foot_p2, v_cast_vec4f(V_CI_MASK1010))), wofs));
        v_st(&traces[i * 2 + 0].pos.x, v_perm_xyzd(tr1, v_splats(maxDist)));
        v_st(&traces[i * 2 + 1].pos.x, v_perm_xyzd(tr2, v_splats(maxDist)));
        v_st(&traces[i * 2 + 0].dir.x, v_perm_xyzd(v_neg(vup), v_splats(leg_diff)));
        v_st(&traces[i * 2 + 1].dir.x, v_perm_xyzd(v_neg(vup), v_splats(leg_diff)));
      }
      else
      {
        traces[i * 2 + 0].t = -1.f;
        traces[i * 2 + 1].t = -1.f;
      }
      v_st(&traces[i * 2 + 0].footP1.x, v_perm_xyzd(foot_p1, v_cast_vec4f(v_splatsi(move_foot))));
      v_st(&traces[i * 2 + 1].footP1.x, v_perm_xyzd(foot_p1, v_cast_vec4f(v_splatsi(move_foot))));
    }
  }

  intptr_t traceRes = ctx.irq(GIRQT_TraceFootStepMultiRay, (intptr_t)traces.data(), traces.size(), isTraceDown);

  for (int i = 0; i < rec.size(); i++)
  {
    vec3f footNewPos = v_zero();
    if (isCrawl)
    {
      if (traceRes == GIRQR_TraceOK)
      {
        footNewPos = v_sub(v_madd(v_ld(&traces[i].dir.x), v_splats(traces[i].t - crawlFootOffset), v_ld(&traces[i].pos.x)), wofs);
        footNewPos = v_and(footNewPos, V_CI_MASK1110);
        nodes[i].da = crawlFootAngle + v_extract_x(v_acos_x(v_set_x((crawlMaxRay - traces[i].t) / crawlMaxRay)));
        traces[i].moveFoot = true;
      }
    }
    else
    {
      float trace_h1 = 0, trace_h2 = 0;
      if (traceRes == GIRQR_TraceOK && traces[i * 2].t >= 0)
      {
        float maxDist = rec[i].maxFootUp + 0.15f;
        float footUpOfs = min(rec[i].maxFootUp, (traces[i * 2].legsDiff + 0.15f) * 0.9f);
        float t1 = traces[i * 2 + 0].t;
        float t2 = traces[i * 2 + 1].t;
        trace_h1 = (t1 != maxDist ? t1 : t2) - footUpOfs;
        trace_h2 = (t2 != maxDist ? t2 : t1) - footUpOfs;
        if (t1 >= maxDist && t2 >= maxDist)
          trace_h1 = trace_h2 = 0;
      }

      float foot_len = v_extract_w(rec[i].vFootFwd);
      vec3f foot_p1 = v_ld(&traces[i * 2].footP1.x);
      float foot_h1 = v_extract_y(foot_p1);
      // float foot_h2 = v_extract_y(foot_p2);
      float move_wt = clamp<float>((rec[i].footStepRange[1] - foot_h1) * rec[i].footStepRange[0], 0.0f, 1.0f);
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
        if (fabsf(dh) > 1e-5f)
        {
          float as = v_extract_x(v_asin_x(v_set_x(dh / foot_len)));
          new_da = clamp(-as, rec[i].footRotRange[0], rec[i].footRotRange[1]) * move_wt * wt;
        }
      }

      {
        float dt = nodes[i].dt;
        float max_dy = dt * rec[i].maxDyRate;
        float max_da = dt * rec[i].maxDaRate;
        nodes[i].dy += clamp(new_dy - nodes[i].dy, -max_dy, max_dy);
        nodes[i].da += clamp(new_da - nodes[i].da, -max_da, max_da);
      }
    } // isCrawl

    bool move_foot = traces[isCrawl ? i : i * 2].moveFoot;
    if (alwaysSolve || nodes[i].dy || move_foot)
    {
      auto foot = nodes[i].footId;
      auto foot_parent = tree.getParentNodeIdx(foot);
      mat44f &foot_wtm = tree.getNodeWtmRel(foot);
      mat44f &foot_tm = tree.getNodeTm(foot);
      mat44f &leg_wtm = tree.getNodeWtmRel(nodes[i].legId);
      mat44f &knee_wtm = tree.getNodeWtmRel(nodes[i].kneeId);
      float len0 = v_extract_x(v_length3_x(v_sub(leg_wtm.col3, knee_wtm.col3)));
      float len1 = v_extract_x(v_length3_x(v_sub(foot_wtm.col3, knee_wtm.col3)));
      vec3f foot_p1 = v_ld(&traces[isCrawl ? i : i * 2].footP1.x);
      vec4f vup = V_C_UNIT_0100;
      if (rec[i].useAnimcharUpDir)
        vup = tree.getRootWtmRel().col1;
      if (isCrawl)
        foot_wtm.col3 = footNewPos;
      else
        foot_wtm.col3 = v_perm_xyzd(v_madd(vup, v_splats(nodes[i].dy), move_foot ? foot_p1 : foot_wtm.col3), V_C_UNIT_0001);
      solve_2bones_ik(leg_wtm, knee_wtm, foot_wtm, foot_wtm, len0, len1);
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
      if (fabsf(nodes[i].da) > 1e-5f)
      {
        mat44f tm = foot_tm;
        mat33f rm;
        v_mat33_make_rot_cw(rm, rec[i].vFootSide, v_splats(nodes[i].da));
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

      r.maxDyRate = GET_PROP(Real, "maxDyRate", 0.9f);            // m/s
      r.maxDaRate = DEG_TO_RAD * GET_PROP(Real, "maxDaRate", 60); // deg/s

      r.useAnimcharUpDir = GET_PROP(Bool, "useAnimcharUpDir", false);

      G_ASSERTF_AND_DO(!r.foot.empty() && !r.knee.empty() && !r.leg.empty(), node->rec.pop_back(), "foot=<%s> knee=<%s> leg=<%s>",
        r.foot, r.knee, r.leg);
#undef GET_PROP
    }
  G_ASSERTF(node->rec.size(), "no legs?");

  node->alwaysSolve = blk.getBool("alwaysSolve", false);
  node->isCrawl = blk.getBool("isCrawl", false);
  node->crawlKneeOffsetVec = blk.getPoint3("crawlKneeOffsetVec", Point3(0, 0.2f, 0.1f));
  node->crawlFootOffset = blk.getReal("crawlFootOffset", 0.15f);
  node->crawlFootAngle = blk.getReal("crawlFootAngle", 130.f);
  node->crawlMaxRay = blk.getReal("crawlMaxRay", 0.5f);
  node->varId =
    graph.addInlinePtrParamId(String(0, "$%s", name), sizeof(NodeId) * node->rec.size(), IPureAnimStateHolder::PT_InlinePtr);
  graph.registerBlendNode(node, name);
}
