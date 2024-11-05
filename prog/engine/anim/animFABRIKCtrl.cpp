// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <anim/dag_animPostBlendCtrl.h>
#include <anim/dag_animIrq.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_Point4.h>
#include <debug/dag_fatal.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_geomTree.h>
#include <math/dag_geomNodeUtils.h>
#include <math/dag_mathUtils.h>
#include <math/dag_fabrikChain.h>
#include <debug/dag_debug.h>
#include <stdlib.h>

#define ALLOW_DBG_CHAIN_STORE (!_TARGET_STATIC_LIB) // tools only

static float getWeightMul(AnimV20::IPureAnimStateHolder &st, int wscale_pid, bool inv)
{
  if (wscale_pid < 0)
    return 1.0f;
  float v = st.getParam(wscale_pid);
  return clamp(inv ? (1 - v) : v, 0.0f, 1.0f);
}

static constexpr int MAX_NODES_PER_CHAIN = 8;
typedef AnimV20::MultiChainFABRIKCtrl::chain_node_t chain_node_t;
typedef AnimV20::MultiChainFABRIKCtrl::chain_tab_t chain_tab_t;
typedef StaticTab<vec3f, MAX_NODES_PER_CHAIN> chain_vec_tab_t;

static bool make_chain(AnimV20::MultiChainFABRIKCtrl::ChainSlice &cs, chain_tab_t &n, const char *name0, const char *name1,
  const GeomNodeTree &tree)
{
  cs.ofs = n.size();
  cs.cnt = 0;
  auto n1 = tree.findNodeIndex(name1);
  if (!n1)
  {
    LOGERR_CTX("node1=<%s> not found", name1);
    return false;
  }
  n.push_back(n1);
  while ((n1 = tree.getParentNodeIdx(n1)).valid())
  {
    n.push_back(n1);
    if (strcmp(tree.getNodeName(n1), name0) == 0) //-V575
      break;
  }
  if (!tree.getParentNodeIdx(n1))
  {
    LOGERR_CTX("node0=<%s> not found upstream from <%s>", name0, name1);
    n.resize(cs.ofs);
    return false;
  }

  for (int i = cs.ofs, ie = n.size() - 1; i < ie; i++, ie--)
  {
    auto tmp = n[i];
    n[i] = n[ie];
    n[ie] = tmp;
  }
  cs.cnt = n.size() - cs.ofs;
  return true;
}

static void apply_chain(GeomNodeTree *g1, dag::Span<chain_node_t> nodeIds, dag::ConstSpan<vec3f> chain, int c_ofs)
{
  for (int j = 1; j < nodeIds.size(); j++)
  {
    auto pnode = g1->getParentNodeIdx(nodeIds[j]);
    mat44f &node0_wtm = g1->getNodeWtmRel(nodeIds[j]);
    mat44f &node0_tm = g1->getNodeTm(nodeIds[j]);
    mat44f &pnode_wtm = g1->getNodeWtmRel(pnode);
    vec3f np = chain[(j + 1 + c_ofs)];

    if (j > 1)
      v_mat44_mul43(node0_wtm, pnode_wtm, node0_tm);

    vec3f col3 = pnode_wtm.col3;
    vec3f dir = v_sub(np, col3);
    if (v_test_vec_x_le(v_length3_sq_x(dir), V_C_EPS_VAL)) // v_quat_from_arc(zero) will give NaN
      continue;
    quat4f q = v_quat_from_arc(v_sub(node0_wtm.col3, col3), dir);
    pnode_wtm.col0 = v_quat_mul_vec3(q, pnode_wtm.col0);
    pnode_wtm.col1 = v_quat_mul_vec3(q, pnode_wtm.col1);
    pnode_wtm.col2 = v_quat_mul_vec3(q, pnode_wtm.col2);
    compute_tm_from_wtm(*g1, pnode);
    v_mat44_mul43(node0_wtm, pnode_wtm, node0_tm);
  }
  if (!nodeIds.empty())
    g1->invalidateWtm(nodeIds.back());
}

static void add_bone(chain_vec_tab_t &chain, vec3f p0, vec3f p1) { chain.push_back() = v_perm_xyzd(p0, v_length3(v_sub(p1, p0))); }
static void add_last_bone_point(chain_vec_tab_t &chain, vec3f p0) { chain.push_back() = v_perm_xyzd(p0, v_zero()); }
static void reposition_base_location(dag::Span<vec3f> chain, vec3f new_p0)
{
  vec3f d = v_perm_xyzd(v_sub(new_p0, chain[0]), v_zero());
  chain[0] = v_perm_xyzd(new_p0, chain[0]);
  for (int i = 1; i < chain.size(); i++)
    chain[i] = v_add(chain[i], d);
}
static void solve_for_target(dag::Span<vec3f> chain, vec3f target)
{
  simple_solve_fabrik_chain(chain.data(), chain.size(), target, 4, 0.001f, 0.0005f, -1);
}

void AnimV20::MultiChainFABRIKCtrl::reset(IPureAnimStateHolder &st)
{
#if ALLOW_DBG_CHAIN_STORE
  for (int i = 0; i < 2; i++)
    if (dbgVarId[i] >= 0)
    {
      debug_state_t *dbg = (debug_state_t *)st.getInlinePtr(dbgVarId[i]);
      if (dag::get_allocator(*dbg))
        clear_and_shrink(*dbg);
    }
#endif
  if (perAnimStateDataVarId >= 0)
    memset(st.getInlinePtr(perAnimStateDataVarId), 0, sizeof(PerAnimStateData));
}

void AnimV20::MultiChainFABRIKCtrl::init(IPureAnimStateHolder &st, const GeomNodeTree &tree)
{
  G_ASSERT(perAnimStateDataVarId >= 0);
  PerAnimStateData &S = *reinterpret_cast<PerAnimStateData *>(st.getInlinePtr(perAnimStateDataVarId));

  S.bodyChainSlice.cnt = 0;
  S.chainSliceCnt = 0;
  S.nodeIds.clear();

  if (!bodyChainEndsNames[0].empty() && !bodyChainEndsNames[1].empty())
  {
    make_chain(S.bodyChainSlice, S.nodeIds, bodyChainEndsNames[0], bodyChainEndsNames[1], tree);
    if (S.bodyChainSlice.cnt > 0 && bodyChainEffectorVarId >= 0)
    {
      if (bodyChainEffectorEnd)
        st.paramEffector(bodyChainEffectorVarId).nodeId = S.bodyChainNodeIds().back();
      else
        st.paramEffector(bodyChainEffectorVarId).nodeId = S.bodyChainNodeIds().front();
    }

    for (int i = 0; i < 2; i++)
    {
      if (bodyTriNames[i].node0.empty() && bodyTriNames[i].node1.empty() && bodyTriNames[i].add.empty() && bodyTriNames[i].sub.empty())
      {
        S.bodyTri[i].node0 = S.bodyTri[i].node1 = S.bodyTri[i].add = S.bodyTri[i].sub = dag::Index16();
        continue;
      }
      S.bodyTri[i].node0 = tree.findNodeIndex(bodyTriNames[i].node0);
      S.bodyTri[i].node1 = tree.findNodeIndex(bodyTriNames[i].node1);
      S.bodyTri[i].add = tree.findNodeIndex(bodyTriNames[i].add);
      S.bodyTri[i].sub = tree.findNodeIndex(bodyTriNames[i].sub);
      if (!S.bodyTri[i].node0 || !S.bodyTri[i].node1 || !S.bodyTri[i].add || !S.bodyTri[i].sub)
      {
        DAG_FATAL("MultiChainFABRIKCtrl<%s> bad bodyTri[%d]: %s->%d %s->%d %s->%d", graph.getBlendNodeName(this), i,
          bodyTriNames[i].node0, (int)S.bodyTri[i].node0, bodyTriNames[i].node1, (int)S.bodyTri[i].node1, bodyTriNames[i].add,
          (int)S.bodyTri[i].add, bodyTriNames[i].sub, (int)S.bodyTri[i].sub);
        // disable tri
        S.bodyTri[i].node0 = S.bodyTri[i].node1 = S.bodyTri[i].add = S.bodyTri[i].sub = dag::Index16();
      }
    }
  }

  for (int i = 0; i < chainEndsNames.size(); i += 2)
  {
    make_chain(S.chainSlice[S.chainSliceCnt], S.nodeIds, chainEndsNames[i + 0], chainEndsNames[i + 1], tree);
    if (S.chainSlice[S.chainSliceCnt].cnt > 1 && effectorVarId[i / 2].eff >= 0)
    {
      if (!S.chainNodeIds(S.chainSliceCnt).empty())
        st.paramEffector(effectorVarId[i / 2].eff).nodeId = S.chainNodeIds(S.chainSliceCnt).back();
    }
    if (S.chainSlice[S.chainSliceCnt].cnt > 0)
      S.chainSliceCnt++;
  }

  for (int i = 0; i < 2; i++)
    if (S.bodyTri[i].node0)
      for (int j = 0, ord = 0; j < S.chainSliceCnt; j++)
        if (S.chainNodeIds(j).size() > 1)
        {
          if (S.bodyTri[i].node0 == S.chainNodeIds(j)[0])
            S.bodyTri[i].chain0 = ord;
          if (S.bodyTri[i].node1 == S.chainNodeIds(j)[0])
            S.bodyTri[i].chain1 = ord;
          ord++;
        }
}
void AnimV20::MultiChainFABRIKCtrl::advance(IPureAnimStateHolder & /*st*/, real /*dt*/) {}

void AnimV20::MultiChainFABRIKCtrl::process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx)
{
  if (wt < 1e-3 || tree.empty())
  {
  skip_processing:
#if ALLOW_DBG_CHAIN_STORE
    for (int i = 0; i < 2; i++)
      if (dbgVarId[i] >= 0)
      {
        debug_state_t *dbg = (debug_state_t *)st.getInlinePtr(dbgVarId[i]);
        if (dag::get_allocator(*dbg))
          clear_and_shrink(*dbg);
      }
#endif
    return;
  }

  bool has_active_effectors = (bodyChainEffectorVarId >= 0)
                                ? st.getParamEffector(bodyChainEffectorVarId).type == IAnimStateHolder::EffectorVar::T_useEffector
                                : false;
  if (!has_active_effectors)
    for (int i = 0; i < effectorVarId.size(); i++)
      if (st.getParamEffector(effectorVarId[i].eff).type == IAnimStateHolder::EffectorVar::T_useEffector)
      {
        has_active_effectors = true;
        break;
      }
  if (!has_active_effectors)
    goto skip_processing;

  PerAnimStateData &S = *reinterpret_cast<PerAnimStateData *>(st.getInlinePtr(perAnimStateDataVarId));
  G_ASSERT_AND_DO(S.chainSliceCnt, return);

  StaticTab<StaticTab<vec3f, MAX_NODES_PER_CHAIN>, MAX_CHAINS> chains;
  StaticTab<vec3f, MAX_NODES_PER_CHAIN> mainChain;
  vec3f targets[MAX_CHAINS];
  int effType[MAX_CHAINS];
  int targets_cnt = 0;
  vec3f body_p[2];
  float body_w[2];

#if ALLOW_DBG_CHAIN_STORE
  // prepare cleared debug vars (orgiginal and final chains)
  debug_state_t *dbgDataPtr[2] = {NULL, NULL};
  for (int i = 0; i < 2; i++)
    if (dbgVarId[i] >= 0)
    {
      dbgDataPtr[i] = (debug_state_t *)st.getInlinePtr(dbgVarId[i]);
      if (!dag::get_allocator(*dbgDataPtr[i]))
        dag::set_allocator(*dbgDataPtr[i], midmem);
      clear_and_shrink(*dbgDataPtr[i]);
    }
#endif

  // prepare tree and get geom-node-fixed effectors
  tree.calcWtm();
  for (int i = 0; i < effectorVarId.size(); i++)
  {
    IAnimStateHolder::EffectorVar &eff = st.paramEffector(effectorVarId[i].eff);
    if (eff.type == eff.T_useGeomNode && eff.nodeId)
    {
      vec3f p = tree.getNodeWtmRel(eff.nodeId).col3;
      eff.x = v_extract_x(p);
      eff.y = v_extract_y(p);
      eff.z = v_extract_z(p);
    }
  }

  // move whole body to body-chain-start effector
  if (bodyChainEffectorVarId >= 0 && !bodyChainEffectorEnd && S.bodyChainSlice.cnt > 0)
  {
    const IAnimStateHolder::EffectorVar &eff = st.getParamEffector(bodyChainEffectorVarId);
    if (eff.type == eff.T_useEffector)
    {
      auto node0 = S.bodyChainNodeIds().front();
      vec3f &node0_pos = tree.getNodeWtmRel(node0).col3;
      node0_pos =
        v_lerp_vec4f(v_splats(wt), node0_pos, v_sub(v_ldu(&eff.x), v_and(ctx.worldTranslate, v_bool_to_mask(!eff.isLocalEff()))));
      compute_tm_from_wtm(tree, node0);
      tree.invalidateWtm(S.bodyChainNodeIds().front());
      tree.calcWtm();
    }
  }

  // build secondary chains
  for (int i = 0; i < S.chainSliceCnt; i++)
    if (S.chainNodeIds(i).size() > 1)
    {
      chain_vec_tab_t &chain = chains.push_back();
      dag::Span<chain_node_t> nodeIds = S.chainNodeIds(i);
      for (int j = 1; j < nodeIds.size(); j++)
      {
        add_bone(chain, tree.getNodeWposRel(nodeIds[j - 1]), tree.getNodeWposRel(nodeIds[j]));
        // debug("add bone* %s - %s", tree.getNodeName(nodeIds[j-1]), tree.getNodeName(nodeIds[j]));
      }
      mat44f &n0_wtm = tree.getNodeWtmRel(nodeIds.back());
      add_last_bone_point(chain, n0_wtm.col3);
#if ALLOW_DBG_CHAIN_STORE
      if (dbgDataPtr[0]) // copy original chains to debug var
        dbgDataPtr[0]->push_back() = chain;
#endif

      G_ASSERT_CONTINUE(targets_cnt < MAX_CHAINS);
      const IAnimStateHolder::EffectorVar &eff = st.getParamEffector(effectorVarId[i].eff);
      float w = wt * getWeightMul(st, effectorVarId[i].wScale, effectorVarId[i].wScaleInverted);
      effType[targets_cnt] = eff.type;
      if (eff.type == eff.T_useEffector)
        targets[targets_cnt] =
          v_lerp_vec4f(v_splats(w), n0_wtm.col3, v_sub(v_ldu(&eff.x), v_and(ctx.worldTranslate, v_bool_to_mask(!eff.isLocalEff()))));
      else if (eff.type == eff.T_useGeomNode && eff.nodeId)
        targets[targets_cnt] = v_lerp_vec4f(v_splats(w), n0_wtm.col3, v_ldu(&eff.x));
      else
      {
        targets[targets_cnt] = v_zero();
        effType[targets_cnt] = IAnimStateHolder::EffectorVar::T_looseEnd;
      }
      targets_cnt++;
    }

  // solve secondary chains linked to body root
  if (S.bodyChainSlice.cnt)
    for (int i = 0; i < chains.size(); i++)
      if (effType[i] != IAnimStateHolder::EffectorVar::T_looseEnd && S.chainNodeIds(i).front() == S.bodyChainNodeIds().front())
      {
        tree.partialCalcWtm(S.chainNodeIds(i).front());
        reposition_base_location(make_span(chains[i]), tree.getNodeWposRel(S.chainNodeIds(i).front()));
        solve_for_target(make_span(chains[i]), targets[i]);
        apply_chain(&tree, S.chainNodeIds(i), chains[i], -1);

        effType[i] = IAnimStateHolder::EffectorVar::T_looseEnd; // don't solve this chain no more
      }

  // prepare body triangles data
  tree.calcWtm();
  for (int i = 0; i < countof(S.bodyTri); i++)
    if (S.bodyTri[i].node0)
    {
      body_p[i] = v_add(v_mul(v_add(tree.getNodeWposRel(S.bodyTri[i].node0), tree.getNodeWposRel(S.bodyTri[i].node1)), V_C_HALF),
        v_sub(tree.getNodeWposRel(S.bodyTri[i].add), tree.getNodeWposRel(S.bodyTri[i].sub)));
      body_w[i] = v_extract_x(v_length3_x(v_sub(tree.getNodeWposRel(S.bodyTri[i].node0), tree.getNodeWposRel(S.bodyTri[i].node1))));
    }
    else
      body_p[i] = v_zero(), body_w[i] = 1;

  // build body chain
  if (S.bodyChainSlice.cnt > 1)
  {
    dag::Span<chain_node_t> nodeIds = S.bodyChainNodeIds();
    for (int i = 0; i <= nodeIds.size(); i++)
    {
      if (!((i == 0 && !S.bodyTri[0].node0) || (i == nodeIds.size() && !S.bodyTri[1].node0)))
        add_bone(mainChain, i > 0 ? tree.getNodeWposRel(nodeIds[i - 1]) : body_p[0],
          i < nodeIds.size() ? tree.getNodeWposRel(nodeIds[i]) : body_p[1]);
      // debug("add bone %s - %s  %@ - %@",
      //   i > 0 ? tree.getNodeName(nodeIds[i-1]) : "-0-",
      //   i < nodeIds.size() ? tree.getNodeName(nodeIds[i]) : "-1-",
      //   i > 0 ? tree.getNodeWposRelScalar(nodeIds[i-1]) : body_p[0],
      //   i < nodeIds.size() ? tree.getNodeWposRelScalar(nodeIds[i]) : body_p[1]);
    }
    add_last_bone_point(mainChain, body_p[1]);
#if ALLOW_DBG_CHAIN_STORE
    if (dbgDataPtr[0]) // copy original chains to debug var
      dbgDataPtr[0]->push_back() = mainChain;
#endif
  }

  // solve secondary chains
  for (int i = 0; i < chains.size(); i++)
    if (effType[i] != IAnimStateHolder::EffectorVar::T_looseEnd)
    {
      solve_for_target(make_span(chains[i]), targets[i]);
      if (!chains[i].empty())
        if (v_extract_x(v_length3_sq_x(v_sub(chains[i].back(), targets[i]))) > 1e-6f)
        {
          vec3f d = v_perm_xyzd(v_sub(targets[i], chains[i].back()), v_zero());
          for (int j = 0; j < chains[i].size(); j++)
            chains[i][j] = v_add(chains[i][j], d);
        }
    }

  if (S.bodyChainSlice.cnt > 1)
  {
    vec3f nd[2], np[2];
    // build ends of main chain using body triangles
    for (int i = 0; i < 2; i++)
      if (S.bodyTri[i].node0)
      {
        nd[i] = v_mul(v_norm3_safe(v_sub(chains[S.bodyTri[i].chain1][0], chains[S.bodyTri[i].chain0][0]), v_zero()), V_C_HALF);
        np[i] = v_add(v_mul(v_add(chains[S.bodyTri[i].chain0][0], chains[S.bodyTri[i].chain1][0]), V_C_HALF),
          v_sub(tree.getNodeWposRel(S.bodyTri[i].add), tree.getNodeWposRel(S.bodyTri[i].sub)));
      }
      else
        np[i] = body_p[i], nd[i] = v_zero();

    // solve main chain
    mainChain[0] = v_perm_xyzd(np[0], mainChain[0]);
    solve_for_target(make_span(mainChain), np[1]);

    // reposition and resolve other chains
    for (int i = 0; i < 2; i++)
      if (S.bodyTri[i].node0)
      {
        np[i] = v_add(v_sub(v_sel_b(mainChain.back(), mainChain[0], i == 0), tree.getNodeWposRel(S.bodyTri[i].add)),
          tree.getNodeWposRel(S.bodyTri[i].sub));

        reposition_base_location(make_span(chains[S.bodyTri[i].chain0]), v_nmsub(nd[i], v_splats(body_w[i]), np[i]));
        reposition_base_location(make_span(chains[S.bodyTri[i].chain1]), v_madd(nd[i], v_splats(body_w[i]), np[i]));
      }

    // apply main chain transformation onto nodes
    mat44f &node0_wtm = tree.getNodeWtmRel(S.bodyChainNodeIds().front());
    node0_wtm.col3 = mainChain[1];
    compute_tm_from_wtm(tree, S.bodyChainNodeIds().front());
    tree.invalidateWtm(S.bodyChainNodeIds().front());
    tree.calcWtm();

    apply_chain(&tree, S.bodyChainNodeIds(), mainChain, 0);
    tree.calcWtm();
  }

  for (int i = 0; i < chains.size(); i++)
    if (effType[i] != IAnimStateHolder::EffectorVar::T_looseEnd)
    {
      if (S.bodyChainSlice.cnt > 1)
      {
        reposition_base_location(make_span(chains[i]), tree.getNodeWposRel(S.chainNodeIds(i).front()));
        solve_for_target(make_span(chains[i]), targets[i]);
      }
      apply_chain(&tree, S.chainNodeIds(i), chains[i], -1);
    }

#if ALLOW_DBG_CHAIN_STORE
  // copy final chains to debug var
  if (dbgDataPtr[1])
  {
    dbgDataPtr[1]->resize(chains.size() + 1);
    for (int i = 0; i < chains.size(); i++)
      (*dbgDataPtr[1])[i] = make_span_const(chains[i]);
    (*dbgDataPtr[1])[chains.size()] = mainChain;
  }
#endif
}

void AnimV20::MultiChainFABRIKCtrl::createNode(AnimationGraph &graph, const DataBlock &blk)
{
  const char *name = blk.getStr("name", NULL);
  G_ASSERTF_AND_DO(name != NULL, return, "not found '%s' param", "name");

  MultiChainFABRIKCtrl *node = new MultiChainFABRIKCtrl(graph);
  for (int i = 0, nid = blk.getNameId("chain"); i < blk.blockCount(); i++)
  {
    const DataBlock &b = *blk.getBlock(i);
    if (b.getBlockNameId() != nid)
      continue;
    const char *eff_nm = b.getStr("effector", NULL);
    const char *start_nm = b.getStr("start", NULL);
    const char *end_nm = b.getStr("end", NULL);
    if ((!start_nm || !*start_nm) || (!end_nm || !*end_nm) || (!eff_nm || !*eff_nm))
    {
      DAG_FATAL("MultiChainFABRIKCtrl<%s> bad chain params: start=%s end=%s effector=%s", name, start_nm, end_nm, eff_nm);
      continue;
    }
    if (strcmp(eff_nm, "start") == 0)
      eff_nm = start_nm;
    else if (strcmp(eff_nm, "end") == 0)
      eff_nm = end_nm;
    else if (strncmp(eff_nm, "start:", 6) == 0)
      eff_nm = eff_nm + 6;
    else if (strncmp(eff_nm, "end:", 4) == 0)
      eff_nm = eff_nm + 4;
    else
    {
      DAG_FATAL("MultiChainFABRIKCtrl<%s> bad chain params: effector=%s", name, eff_nm);
      continue;
    }
    node->chainEndsNames.push_back() = start_nm;
    node->chainEndsNames.push_back() = end_nm;

    VarId &ev = node->effectorVarId.push_back();
    ev.wScale = graph.addParamIdEx(b.getStr("wtModulate", blk.getStr("wtModulate", NULL)), IPureAnimStateHolder::PT_ScalarParam);
    ev.wScaleInverted = b.getBool("wtModulateInverse", blk.getBool("wtModulateInverse", false));
    ev.eff = graph.addEffectorParamId(eff_nm);
  }

  if (const DataBlock *b = blk.getBlockByName("mainChain"))
  {
    const char *eff_nm = b->getStr("effector", NULL);
    const char *start_nm = b->getStr("start", NULL);
    const char *end_nm = b->getStr("end", NULL);
    if ((!start_nm || !*start_nm) || (!end_nm || !*end_nm) || (!eff_nm || !*eff_nm))
    {
      DAG_FATAL("MultiChainFABRIKCtrl<%s> bad main chain params: start=%s end=%s effector=%s", name, start_nm, end_nm, eff_nm);
      goto skip_main_chain;
    }
    node->bodyChainEffectorEnd = 0;
    if (strcmp(eff_nm, "start") == 0)
      eff_nm = start_nm;
    else if (strcmp(eff_nm, "end") == 0)
      eff_nm = end_nm, node->bodyChainEffectorEnd = 1;
    else if (strncmp(eff_nm, "start:", 6) == 0)
      eff_nm = eff_nm + 6;
    else if (strncmp(eff_nm, "end:", 4) == 0)
      eff_nm = eff_nm + 4, node->bodyChainEffectorEnd = 1;
    else
    {
      DAG_FATAL("MultiChainFABRIKCtrl<%s> bad chain params: effector=%s", name, eff_nm);
      goto skip_main_chain;
    }
    node->bodyChainEndsNames[0] = start_nm;
    node->bodyChainEndsNames[1] = end_nm;
    node->bodyChainEffectorVarId = graph.addEffectorParamId(eff_nm);

    if (const DataBlock *b2 = b->getBlockByName("startTriangle"))
    {
      node->bodyTriNames[0].node0 = b2->getStr("node0", NULL);
      node->bodyTriNames[0].node1 = b2->getStr("node1", NULL);
      node->bodyTriNames[0].add = b2->getStr("add", NULL);
      node->bodyTriNames[0].sub = b2->getStr("sub", NULL);
    }
    if (const DataBlock *b2 = b->getBlockByName("endTriangle"))
    {
      node->bodyTriNames[1].node0 = b2->getStr("node0", NULL);
      node->bodyTriNames[1].node1 = b2->getStr("node1", NULL);
      node->bodyTriNames[1].add = b2->getStr("add", NULL);
      node->bodyTriNames[1].sub = b2->getStr("sub", NULL);
    }

  skip_main_chain:;
  }

  node->perAnimStateDataVarId = graph.addInlinePtrParamId(String(0, "%s$treeCtx", name), sizeof(PerAnimStateData));
#if ALLOW_DBG_CHAIN_STORE
  if (blk.getBool("use_debug_vars", false))
  {
    node->dbgVarId[0] =
      graph.addInlinePtrParamId(String(0, "%s$dbg0", name), sizeof(debug_state_t), IPureAnimStateHolder::PT_InlinePtrCTZ);
    node->dbgVarId[1] =
      graph.addInlinePtrParamId(String(0, "%s$dbg1", name), sizeof(debug_state_t), IPureAnimStateHolder::PT_InlinePtrCTZ);
  }
#endif

  graph.registerBlendNode(node, name);
}
