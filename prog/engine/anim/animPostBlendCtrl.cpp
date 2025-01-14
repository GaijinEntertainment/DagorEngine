// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <anim/dag_animPostBlendCtrl.h>
#include <anim/dag_animKeyInterp.h>
#include <math/dag_quatInterp.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_Point4.h>
#include <math/dag_mathAng.h>
#include <debug/dag_fatal.h>
#include <ioSys/dag_dataBlock.h>
#include <gameRes/dag_gameResSystem.h> // is_ignoring_unavailable_resources
#include <math/dag_geomTree.h>
#include <math/dag_mathUtils.h>
#include <math/random/dag_random.h>
#include <regExp/regExp.h>
#include <animChar/dag_animCharacter2.h>
#include <debug/dag_debug.h>
#include <stdlib.h>
#include <util/dag_lookup.h>
#include <math/twistCtrl.h>
#include "animInternal.h"

using namespace AnimV20;

static int __cdecl cmp_grow_up(const void *p1, const void *p2)
{
  int i1 = *(int *)p1;
  int i2 = *(int *)p2;
  if (i1 == -1 && i2 == -1)
    return 0;

  if (i1 == -1)
    return 1;
  if (i2 == -1)
    return -1;

  return i1 - i2;
}

static dag::Index16 resolve_node_by_name(const GeomNodeTree &tree, const char *node)
{
  return *node ? ((strcmp(node, "::root") == 0) ? dag::Index16(0) : tree.findINodeIndex(node)) : dag::Index16();
}

template <typename T, typename N>
static void init_target_nodes(const GeomNodeTree &tree, T &target_node, N &node_id)
{
  for (int i = 0; i < target_node.size(); i++)
  {
    node_id[i].target = resolve_node_by_name(tree, target_node[i].name);
    node_id[i].nodeRemap = i;
  }

  qsort(node_id, target_node.size(), sizeof(node_id[0]), cmp_grow_up);
}

static float get_blk_param_val(const DataBlock &blk, const char *vname, float def)
{
  int pidx = blk.findParam(vname);
  if (pidx < 0)
    return def;
  if (blk.getParamType(pidx) == DataBlock::TYPE_STRING)
    return AnimV20::getEnumValueByName(blk.getStr(pidx));
  return blk.getReal(pidx);
}

enum
{
  OPC_INACTIVE = -2,
  OPC_INVALID = -1,
  OPC_EQ,
  OPC_NEQ,
  OPC_GT,
  OPC_GTE,
  OPC_LT,
  OPC_LTE,
  OPC_INSIDE,
  OPC_OUTSIDE,
  OPC_DIST
};
static bool load_comp_condition(const DataBlock &blk, int &op, float &p0, float &p1)
{
  const char *opStr = blk.getStr("op", NULL);
  p0 = get_blk_param_val(blk, "p0", 0);
  p1 = get_blk_param_val(blk, "p1", 0);
  if (strcmp(opStr, "=") == 0)
    op = OPC_EQ;
  else if (strcmp(opStr, "!=") == 0)
    op = OPC_NEQ;
  else if (strcmp(opStr, ">") == 0)
    op = OPC_GT;
  else if (strcmp(opStr, ">=") == 0)
    op = OPC_GTE;
  else if (strcmp(opStr, "<") == 0)
    op = OPC_LT;
  else if (strcmp(opStr, "<=") == 0)
    op = OPC_LTE;
  else if (strcmp(opStr, "inside") == 0)
    op = OPC_INSIDE;
  else if (strcmp(opStr, "outside") == 0)
    op = OPC_OUTSIDE;
  else if (strcmp(opStr, "dist") == 0)
    op = OPC_DIST;
  else
  {
    op = OPC_INVALID;
    return false;
  }
  return true;
}

static bool check_comp_condition(int op, float v, float p0, float p1)
{
  switch (op)
  {
    case OPC_EQ: return v == p0;
    case OPC_NEQ: return v != p0;
    case OPC_GT: return v > p0;
    case OPC_GTE: return v >= p0;
    case OPC_LT: return v < p0;
    case OPC_LTE: return v <= p0;
    case OPC_INSIDE: return v >= p0 && v <= p1;
    case OPC_OUTSIDE: return v < p0 || v > p1;
    case OPC_DIST: return rabs(v - p0) < p1;
  }
  return false;
}

enum
{
  OPB_INVALID = -1,
  OPB_AND,
  OPB_OR,
  OPB_NOT
};
static bool load_bool_operation(const DataBlock &blk, int &op)
{
  const char *opStr = blk.getStr("op", NULL);
  if (strcmp(opStr, "&") == 0)
    op = OPB_AND;
  else if (strcmp(opStr, "|") == 0)
    op = OPB_OR;
  else if (strcmp(opStr, "!") == 0)
    op = OPB_NOT;
  else
  {
    op = OPB_INVALID;
    return false;
  }
  return true;
}

enum
{
  OPM_MUL = 0,
  OPM_ASSIGN,
  OPM_ADD,
  OPM_SUB,
  OPM_MOD,
  OPM_CLAMP,
  OPM_DIV,
  OPM_RND,
  OPM_APPROACH,
  OPM_FLOOR,
  OPM_SIN
};

static bool load_math_op(const DataBlock &blk, int &op, float &p0, float &p1)
{
  const char *opStr = blk.getStr("op", NULL);
  const char *opNames[] = {"*", "=", "+", "-", "%", "[]", "/", "rnd", "approach", "floor", "sin"};
  op = lup(opStr, opNames, countof(opNames), -1);
  if (const char *v = blk.getStr("named_p0", NULL))
    p0 = AnimV20::getEnumValueByName(v);
  else if (const char *v = blk.getStr("slot_p0", NULL))
    p0 = AnimCharV20::addSlotId(v);
  else
    p0 = blk.getReal("p0", 0);
  if (const char *v = blk.getStr("named_p1", NULL))
    p1 = AnimV20::getEnumValueByName(v);
  else if (const char *v = blk.getStr("slot_p1", NULL))
    p1 = AnimCharV20::addSlotId(v);
  else
    p1 = blk.getReal("p1", 0);
  return op >= 0;
}

static float process_math_op(int op, float v, float p0, float p1, float dt)
{
  switch (op)
  {
    case OPM_MUL: return v * p0;
    case OPM_ASSIGN: return p0;
    case OPM_ADD: return v + p0;
    case OPM_SUB: return v - p0;
    case OPM_MOD: return fmodf(v, p0);
    case OPM_CLAMP: return clamp(v, p0, p1);
    case OPM_DIV: return safediv(v, p0);
    case OPM_RND: return rnd_float(p0, p1);
    case OPM_APPROACH: return approach(v, p0, dt, p1);
    case OPM_FLOOR: return floorf(v);
    case OPM_SIN: return sinf(p0);
  }
  return v;
}

static float get_var_or_val(IPureAnimStateHolder &st, int var_pid, float val)
{
  if (var_pid >= 0)
    return st.getParam(var_pid);
  return val;
}

#define BLEND_FINAL_TM(NEW_M3, OLD_M3, WT, MARKER, LEFT_M_EXPR)                                                 \
  if (WT < 0.999f)                                                                                              \
  {                                                                                                             \
    bool left_m = LEFT_M_EXPR;                                                                                  \
    if (left_m)                                                                                                 \
    {                                                                                                           \
      OLD_M3.col2 = v_neg(OLD_M3.col2);                                                                         \
      NEW_M3.col2 = v_neg(NEW_M3.col2);                                                                         \
    }                                                                                                           \
    quat4f r0, r1;                                                                                              \
    vec3f s0, s1;                                                                                               \
    v_mat33_decompose(OLD_M3, r0, s0);                                                                          \
    v_mat33_decompose(NEW_M3, r1, s1);                                                                          \
    v_mat33_compose(NEW_M3, v_quat_qslerp(WT, v_norm4(r0), v_norm4(r1)), v_lerp_vec4f(v_splats((WT)), s0, s1)); \
    if (left_m)                                                                                                 \
      NEW_M3.col2 = v_neg(NEW_M3.col2);                                                                         \
  }

#define BLEND_FINAL_WTM(NEW_M3, OLD_M3, WT, MARKER) BLEND_FINAL_TM(NEW_M3, OLD_M3, WT, MARKER, v_extract_x(v_mat33_det(OLD_M3)) < 0)

static void blend_mat44f(mat44f &new_m4, const mat44f &old_m4, float w)
{
  if (w > 0.999f)
    return;
  mat33f nm3;
  nm3.col0 = new_m4.col0;
  nm3.col1 = new_m4.col1;
  nm3.col2 = new_m4.col2;
  mat33f om3;
  om3.col0 = old_m4.col0;
  om3.col1 = old_m4.col1;
  om3.col2 = old_m4.col2;
  BLEND_FINAL_WTM(nm3, om3, w, blend_mat44f)

  new_m4.col0 = nm3.col0;
  new_m4.col1 = nm3.col1;
  new_m4.col2 = nm3.col2;
  new_m4.col3 = v_lerp_vec4f(v_splats(w), old_m4.col3, new_m4.col3);
}

static float getWeightMul(AnimV20::IPureAnimStateHolder &st, int wscale_pid, bool inv)
{
  if (wscale_pid < 0)
    return 1.0f;
  float v = st.getParam(wscale_pid);
  return clamp(inv ? (1.0f - v) : v, 0.0f, 1.0f);
}

// Basic post-blend controller implementation
AnimPostBlendCtrl::AnimPostBlendCtrl(AnimationGraph &g) : graph(g) { pbcId = graph.registerPostBlendCtrl(this); }
AnimPostBlendCtrl::~AnimPostBlendCtrl()
{
  if (pbcId != -1)
  {
    graph.unregisterPostBlendCtrl(pbcId, this);
    pbcId = -1;
  }
}

// Controller to compute node's rotation and shift implementation
void DeltaRotateShiftCtrl::init(IPureAnimStateHolder &st, const GeomNodeTree &tree)
{
  LocalData &ldata = *(LocalData *)st.getInlinePtr(localVarId);
  ldata.targetNode = resolve_node_by_name(tree, targetNode);
}

void DeltaRotateShiftCtrl::process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx)
{
  if (wt <= 0)
  {
    if (varId.yaw >= 0)
      st.setParam(varId.yaw, 0);
    if (varId.pitch >= 0)
      st.setParam(varId.pitch, 0);
    if (varId.lean >= 0)
      st.setParam(varId.lean, 0);
    if (varId.ofsX >= 0)
      st.setParam(varId.ofsX, 0);
    if (varId.ofsY >= 0)
      st.setParam(varId.ofsY, 0);
    if (varId.ofsZ >= 0)
      st.setParam(varId.ofsZ, 0);
    return;
  }

  LocalData &ldata = *(LocalData *)st.getInlinePtr(localVarId);
  TMatrix nodeTm;
  tree.getNodeTmScalar(ldata.targetNode, nodeTm);
  float yaw, pitch, lean;

  if (relativeToOrig)
  {
    TMatrix origNodeTm;
    ctx.origTree.getNodeTmScalar(ldata.targetNode, origNodeTm);
    const TMatrix origNodeItm = orthonormalized_inverse(origNodeTm);
    nodeTm = origNodeItm * nodeTm;
  }

  if (varId.yaw >= 0 || varId.pitch >= 0 || varId.lean >= 0)
  {
    matrix_to_euler(nodeTm, yaw, pitch, lean);

    if (varId.yaw >= 0)
      st.setParam(varId.yaw, yaw * 180 / PI);
    if (varId.pitch >= 0)
      st.setParam(varId.pitch, pitch * 180 / PI);
    if (varId.lean >= 0)
      st.setParam(varId.lean, lean * 180 / PI);
  }

  if (varId.ofsX >= 0)
    st.setParam(varId.ofsX, nodeTm.col[3][0]);
  if (varId.ofsY >= 0)
    st.setParam(varId.ofsY, nodeTm.col[3][1]);
  if (varId.ofsZ >= 0)
    st.setParam(varId.ofsZ, nodeTm.col[3][2]);
}

void DeltaRotateShiftCtrl::createNode(AnimationGraph &graph, const DataBlock &blk)
{
  const char *name = blk.getStr("name", NULL);
  G_ASSERTF_RETURN(name, , "no name for %s", __FUNCTION__);

  DeltaRotateShiftCtrl *node = new DeltaRotateShiftCtrl(graph);

  node->targetNode = blk.getStr("targetNode", NULL);
#define SETUP_PARAM(NM)                          \
  if (const char *pname = blk.getStr(#NM, NULL)) \
  node->varId.NM = graph.addParamId(pname, IPureAnimStateHolder::PT_ScalarParam)
  SETUP_PARAM(yaw);
  SETUP_PARAM(pitch);
  SETUP_PARAM(lean);
  SETUP_PARAM(ofsX);
  SETUP_PARAM(ofsY);
  SETUP_PARAM(ofsZ);
#undef SETUP_PARAM

  node->relativeToOrig = blk.getBool("relativeToOrig", false);

  node->localVarId = graph.addInlinePtrParamId(String("var") + name, sizeof(LocalData), IPureAnimStateHolder::PT_InlinePtr);
  graph.registerBlendNode(node, name);
}

//
// Controller to compute delta angles
//
void DeltaAnglesCtrl::reset(IPureAnimStateHolder &st)
{
  if (destRotateVarId >= 0)
    st.setParam(destRotateVarId, 0);
  if (destPitchVarId >= 0)
    st.setParam(destPitchVarId, 0);
  if (destLeanVarId >= 0)
    st.setParam(destLeanVarId, 0);
}
void DeltaAnglesCtrl::init(IPureAnimStateHolder &st, const GeomNodeTree &tree)
{
  st.setParamInt(varId, (int)resolve_node_by_name(tree, nodeName));
  G_ASSERTF(st.getParamInt(varId) >= 0, "missing nodeName=%s", nodeName);
}
void DeltaAnglesCtrl::process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &)
{
  if (wt <= 0)
  {
    if (destRotateVarId >= 0)
      st.setParam(destRotateVarId, 0);
    if (destPitchVarId >= 0)
      st.setParam(destPitchVarId, 0);
    if (destLeanVarId >= 0)
      st.setParam(destLeanVarId, 0);
    return;
  }

  dag::Index16 node_id(st.getParamInt(varId));
  if (tree.empty() || !node_id)
    return;
  mat44f &n_wtm = tree.getNodeWtmRel(node_id);
  mat44f &nr_wtm = tree.getRootWtmRel();

  tree.partialCalcWtm(node_id);
  if (destRotateVarId >= 0 || destPitchVarId >= 0)
  {
    vec3f node_dir = (&n_wtm.col0)[fwdAxisIdx];
    vec4f root_fwd = v_dir_to_angles(nr_wtm.col2);
    vec4f node_fwd = v_dir_to_angles(v_xor(node_dir, v_bool_to_msbit(invFwd)));
    vec4f sAng = v_norm_s_angle(v_sub(root_fwd, node_fwd));
    if (destRotateVarId >= 0)
      st.setParam(destRotateVarId, v_extract_x(sAng) * scaleR * wt);
    if (destPitchVarId >= 0)
      st.setParam(destPitchVarId, v_extract_y(sAng) * scaleP * wt);
  }
  if (destLeanVarId >= 0)
  {
    vec3f node_dir = (&n_wtm.col0)[sideAxisIdx];
    vec4f root_side = v_dir_to_angles(nr_wtm.col0);
    vec4f node_side = v_dir_to_angles(v_xor(node_dir, v_bool_to_msbit(invFwd)));
    vec4f sAng = v_norm_s_angle(v_sub(node_side, root_side));
    st.setParam(destLeanVarId, v_extract_y(sAng) * scaleL * wt);
  }
}
void DeltaAnglesCtrl::createNode(AnimationGraph &graph, const DataBlock &blk)
{
  const char *name = blk.getStr("name", NULL);
  G_ASSERTF_AND_DO(name != NULL, return, "not found '%s' param", "name");

  DeltaAnglesCtrl *node = new DeltaAnglesCtrl(graph);
  node->nodeName = blk.getStr("refNode", "");
  node->fwdAxisIdx = blk.getInt("fwdAxisIdx", 0);
  G_ASSERTF(node->fwdAxisIdx >= 0 && node->fwdAxisIdx <= 2, "fwdAxisIdx=%d", node->fwdAxisIdx);
  node->sideAxisIdx = blk.getInt("sideAxisIdx", 1);
  G_ASSERTF(node->sideAxisIdx >= 0 && node->sideAxisIdx <= 2, "sideAxisIdx=%d", node->sideAxisIdx);
  node->invFwd = blk.getBool("fwdAxisInverse", false);
  node->invSide = blk.getBool("sideAxisInverse", false);

  if (const char *pn = blk.getStr("destRotateVar", NULL))
    node->destRotateVarId = graph.addParamId(pn, IPureAnimStateHolder::PT_ScalarParam);
  if (const char *pn = blk.getStr("destPitchVar", NULL))
    node->destPitchVarId = graph.addParamId(pn, IPureAnimStateHolder::PT_ScalarParam);
  if (const char *pn = blk.getStr("destLeanVar", NULL))
    node->destLeanVarId = graph.addParamId(pn, IPureAnimStateHolder::PT_ScalarParam);
  G_ASSERTF(node->destRotateVarId >= 0 || node->destPitchVarId >= 0 || node->destLeanVarId >= 0, "%s=%d %s=%d %s=%d",
    blk.getStr("destRotateVar", "?"), node->destRotateVarId, blk.getStr("destPitchVar", "?"), node->destPitchVarId,
    blk.getStr("destLeanVar", "?"), node->destLeanVarId);

  node->scaleR = blk.getReal("scaleRotate", 1.0f);
  node->scaleP = blk.getReal("scalePitch", 1.0f);
  node->scaleL = blk.getReal("scaleLean", 1.0f);
  if (blk.getBool("degrees", true))
  {
    node->scaleR *= RAD_TO_DEG;
    node->scaleP *= RAD_TO_DEG;
    node->scaleL *= RAD_TO_DEG;
  }

  node->varId = graph.addParamId(String(0, ":%s", name), IPureAnimStateHolder::PT_ScalarParamInt);
  graph.registerBlendNode(node, name);
}

//
// Node align controller
//
AnimPostBlendAlignCtrl::AnimPostBlendAlignCtrl(AnimationGraph &g, const char *var_name) : AnimPostBlendCtrl(g)
{
  paramId = graph.addInlinePtrParamId(var_name, sizeof(NodeId), IPureAnimStateHolder::PT_InlinePtr);
  v_mat33_ident(tmRot);
  v_mat33_ident_swapxz(lr);
}

void AnimPostBlendAlignCtrl::init(IPureAnimStateHolder &st, const GeomNodeTree &tree)
{
  NodeId *nodeId = (NodeId *)st.getInlinePtr(paramId);
  nodeId->target = resolve_node_by_name(tree, targetNodeName);
  nodeId->lastRefUid = INVALID_ATTACHMENT_UID;
  nodeId->src = dag::Index16();

  if (nodeId->target.index() >= tree.nodeCount())
    nodeId->target = dag::Index16();
}
void AnimPostBlendAlignCtrl::process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx)
{
  wt *= getWeightMul(st, wScaleVarId, wScaleInverted);
  if (wt <= 1e-4f)
    return;

  NodeId *nodeId = (NodeId *)st.getInlinePtr(paramId);
  unsigned s_uid = srcSlotId < 0 ? 0xFFFFFFFF : ctx.ac->getAttachmentUid(srcSlotId);
  GeomNodeTree *s_tree = srcSlotId < 0 ? &tree : ctx.ac->getAttachedSkeleton(srcSlotId);
  if (nodeId->lastRefUid != s_uid)
  {
    nodeId->src = s_tree ? resolve_node_by_name(*s_tree, srcNodeName) : dag::Index16();
    nodeId->lastRefUid = s_uid;
  }

  if (!nodeId->target || !nodeId->src)
    return;

  tree.partialCalcWtm(nodeId->target);
  s_tree->partialCalcWtm(nodeId->src);

  tree.invalidateWtm(nodeId->target);

  mat44f &sn_wtm = s_tree->getNodeWtmRel(nodeId->src);
  mat44f &dn_wtm = tree.getNodeWtmRel(nodeId->target);
  mat33f tm, ctm, m;

  tm.col0 = sn_wtm.col0;
  tm.col1 = sn_wtm.col1;
  tm.col2 = sn_wtm.col2;

  ctm.col0 = dn_wtm.col0;
  ctm.col1 = dn_wtm.col1;
  ctm.col2 = dn_wtm.col2;

  v_mat33_mul(m, tm, tmRot);
  if (binaryWt)
    wt = 1;
  if (useLocal)
    v_mat33_mul(tm, m, ctm);
  else
    tm = m;

  v_mat33_mul(m, tm, lr);
  BLEND_FINAL_WTM(m, ctm, wt, align);

  dn_wtm.col0 = m.col0;
  dn_wtm.col1 = m.col1;
  dn_wtm.col2 = m.col2;
  if (copyPos)
  {
    vec3f sn_pos = sn_wtm.col3;
    if (srcSlotId >= 0)
      sn_pos = v_sub(sn_pos, ctx.worldTranslate);
    dn_wtm.col3 = v_lerp_vec4f(v_splats(wt), dn_wtm.col3, sn_pos);
  }
  mat44f iwtm;
  v_mat44_inverse43(iwtm, tree.getNodeWtmRel(tree.getParentNodeIdx(nodeId->target)));
  v_mat44_mul43(tree.getNodeTm(nodeId->target), iwtm, dn_wtm);
  if (copyBlendResultToSrc)
  {
    sn_wtm = dn_wtm;
    if (srcSlotId >= 0)
      sn_wtm.col3 = v_add(sn_wtm.col3, ctx.worldTranslate);
    v_mat44_inverse43(iwtm, s_tree->getNodeWtmRel(s_tree->getParentNodeIdx(nodeId->src)));
    v_mat44_mul43(s_tree->getNodeTm(nodeId->src), iwtm, sn_wtm);
  }
}

void AnimPostBlendAlignCtrl::createNode(AnimationGraph &graph, const DataBlock &blk)
{
  const char *name = blk.getStr("name", NULL);

  if (!name)
  {
    DAG_FATAL("not found 'name' param");
    return;
  }

  String var_name = String("var") + name;

  AnimPostBlendAlignCtrl *node = new AnimPostBlendAlignCtrl(graph, var_name);
  node->targetNodeName = blk.getStr("targetNode", "");
  node->srcNodeName = blk.getStr("srcNode", "");

  Point3 euler = blk.getPoint3("rot_euler", Point3(0, 0, 0)) * PI / 180;

  v_mat33_make_rot_cw_zyx(node->tmRot, v_neg(v_ldu(&euler.x)));

  node->useLocal = blk.getBool("useLocal", false);

  Point3 scale = blk.getPoint3("scale", Point3(1, 1, 1));

  if (blk.getBool("leftTm", true))
  {
    node->lr.col0 = v_mul(V_C_UNIT_0010, v_splats(scale[2]));
    node->lr.col1 = v_mul(V_C_UNIT_0100, v_splats(scale[1]));
    node->lr.col2 = v_mul(V_C_UNIT_1000, v_splats(scale[0]));
  }
  else
  {
    node->lr.col0 = v_mul(V_C_UNIT_1000, v_splats(scale[0]));
    node->lr.col1 = v_mul(V_C_UNIT_0100, v_splats(scale[1]));
    node->lr.col2 = v_mul(V_C_UNIT_0010, v_splats(scale[2]));
  }
  node->binaryWt = blk.getBool("binaryWt", false);
  node->copyBlendResultToSrc = blk.getBool("copyBlendResultToSrc", false);
  node->copyPos = blk.getBool("copyPos", false);
  if (const char *slot = blk.getStr("srcSlot", NULL))
    node->srcSlotId = AnimCharV20::addSlotId(slot);
  node->wScaleVarId = graph.addParamIdEx(blk.getStr("wtModulate", NULL), IPureAnimStateHolder::PT_ScalarParam);
  node->wScaleInverted = blk.getBool("wtModulateInverse", false);

  graph.registerBlendNode(node, name);
}

//
// Advanced align controller: aligns helper node to root and distributes rotation among target nodes
//
void AnimPostBlendAlignExCtrl::init(IPureAnimStateHolder &st, const GeomNodeTree &tree)
{
  NodeId *nodeId = (NodeId *)st.getInlinePtr(varId);

  nodeId->hlp = resolve_node_by_name(tree, hlpNodeName);
  init_target_nodes(tree, targetNode, nodeId->id);
}

void AnimPostBlendAlignExCtrl::process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &)
{
  NodeId &nodeId = *(NodeId *)st.getInlinePtr(varId);
  if (tree.empty() || !nodeId.hlp)
    return;
  mat44f &n_wtm = tree.getNodeWtmRel(nodeId.hlp);
  mat44f &nr_wtm = tree.getRootWtmRel();

  mat33f rootTm;
  rootTm.col0 = nr_wtm.col0;
  rootTm.col1 = nr_wtm.col1;
  rootTm.col2 = nr_wtm.col2;
  for (int i = 0; i < targetNode.size(); i++)
  {
    if (!nodeId.id[i].target)
      continue;

    mat44f &tn_wtm = tree.getNodeWtmRel(nodeId.id[i].target);
    mat33f helpTm;
    mat44f rot;

    tree.partialCalcWtm(nodeId.hlp);
    helpTm.col0 = hlpCol[0] > 0 ? (&n_wtm.col0)[hlpCol[0] - 1] : v_neg((&n_wtm.col0)[-hlpCol[0] - 1]);
    helpTm.col1 = hlpCol[1] > 0 ? (&n_wtm.col0)[hlpCol[1] - 1] : v_neg((&n_wtm.col0)[-hlpCol[1] - 1]);
    helpTm.col2 = hlpCol[2] > 0 ? (&n_wtm.col0)[hlpCol[2] - 1] : v_neg((&n_wtm.col0)[-hlpCol[2] - 1]);

    quat4f qs = v_quat_from_mat(v_norm3(helpTm.col0), v_norm3(helpTm.col1), v_norm3(helpTm.col2));
    quat4f qd = v_quat_from_mat(v_norm3(rootTm.col0), v_norm3(rootTm.col1), v_norm3(rootTm.col2));
    qd = v_quat_mul_quat(qd, v_quat_conjugate(qs));

    float w = wt * (i + 1 < targetNode.size() ? targetNode[nodeId.id[i].nodeRemap].wt : 1.0f);
    if (w < 1)
      qd = v_quat_qslerp(w, V_C_UNIT_0001, qd);
    v_mat44_from_quat(rot, qd, v_zero());

    vec4f pos = tn_wtm.col3;
    v_mat44_mul33r(tn_wtm, rot, tn_wtm);
    tn_wtm.col3 = pos;

    tree.recalcTm(nodeId.id[i].target);
    tree.invalidateWtm(nodeId.id[i].target);
  }
}

static int parse_hlp_col(const char *s)
{
  int mul = 1;
  if (*s == '-')
    mul = -1, s++;
  else if (*s == '+')
    s++;
  switch (*s)
  {
    case 'x':
    case 'X': return mul * 1;
    case 'y':
    case 'Y': return mul * 2;
    case 'z':
    case 'Z': return mul * 3;
  }
  return 0;
}
void AnimPostBlendAlignExCtrl::createNode(AnimationGraph &graph, const DataBlock &blk)
{
  const char *name = blk.getStr("name", NULL);
  const char *hlp_node = blk.getStr("hlpNode", NULL);
  G_ASSERTF_AND_DO(name && *name, return, "not found '%s' param", "name");
  G_ASSERTF_AND_DO(hlp_node && *hlp_node, return, "not found '%s' param", "hlpNode");

  AnimPostBlendAlignExCtrl *node = new AnimPostBlendAlignExCtrl(graph);
  node->hlpNodeName = hlp_node;

  // read node list
  float def_wt = blk.getReal("targetNodeWt", 1.0f);
  for (int j = 0, nid = blk.getNameId("targetNode"); j < blk.paramCount(); j++)
    if (blk.getParamType(j) == DataBlock::TYPE_STRING && blk.getParamNameId(j) == nid)
    {
      NodeDesc &nd = node->targetNode.push_back();
      nd.name = blk.getStr(j);
      nd.wt = blk.getReal(String(0, "targetNodeWt%d", node->targetNode.size()), def_wt);
    }
  node->hlpCol[0] = parse_hlp_col(blk.getStr("hlpX", "x"));
  node->hlpCol[1] = parse_hlp_col(blk.getStr("hlpY", "y"));
  node->hlpCol[2] = parse_hlp_col(blk.getStr("hlpZ", "z"));
  G_ASSERTF(node->hlpCol[0] >= -3 && node->hlpCol[0] <= 3 && node->hlpCol[1] >= -3 && node->hlpCol[1] <= 3 && node->hlpCol[2] >= -3 &&
              node->hlpCol[2] <= 3 && abs(node->hlpCol[0]) + abs(node->hlpCol[1]) + abs(node->hlpCol[2]) == 1 + 2 + 3 &&
              node->hlpCol[0] && node->hlpCol[1] && node->hlpCol[2],
    "hlp=%d, %d, %d (%s, %s, %s)", node->hlpCol[0], node->hlpCol[1], node->hlpCol[2], blk.getStr("hlpX", "x"), blk.getStr("hlpY", "y"),
    blk.getStr("hlpZ", "z"));

  node->varId = graph.addInlinePtrParamId(String(0, ":%s", name), sizeof(int) + node->targetNode.size() * sizeof(NodeId::id),
    IPureAnimStateHolder::PT_InlinePtr);

  graph.registerBlendNode(node, name);
}

//
// Node rotate controller
//
AnimPostBlendRotateCtrl::AnimPostBlendRotateCtrl(AnimationGraph &g, const char *param_name, const char *ac_pname,
  const char *add_pname) :
  AnimPostBlendCtrl(g), targetNode(midmem)
{
  varId = -1;
  leftTm = true;
  swapYZ = false;
  relRot = false;
  saveScale = false;
  paramId = graph.addParamId(param_name, IPureAnimStateHolder::PT_ScalarParam);
  addParam2Id = add_pname && *add_pname ? graph.addParamId(add_pname, IPureAnimStateHolder::PT_ScalarParam) : -1;
  addParam3Id = -1;
  axisCourseParamId = ac_pname ? graph.addParamId(ac_pname, IPureAnimStateHolder::PT_ScalarParam) : -1;
  kAdd = 0;
  kAdd2 = 0;
  kCourseAdd = 0;
  kMul = 1.0f;
  kMul2 = 1.0f;
  rotAxis = V_C_UNIT_0100;
  dirAxis = V_C_UNIT_0010;
  axisFromCharIndex = -1;
}

void AnimPostBlendRotateCtrl::init(IPureAnimStateHolder &st, const GeomNodeTree &tree)
{
  NodeId *nodeId = (NodeId *)st.getInlinePtr(varId);

  init_target_nodes(tree, targetNode, nodeId);
}
void AnimPostBlendRotateCtrl::process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx)
{
  if (wt <= 0.001f || targetNode.size() < 1)
    return;

  NodeId *nodeId = (NodeId *)st.getInlinePtr(varId);
  real p0 = st.getParam(paramId);
  if (addParam2Id >= 0)
    p0 += st.getParam(addParam2Id) * kMul2 + kAdd2;
  if (addParam3Id >= 0)
    p0 += st.getParam(addParam3Id) * kMul2 + kAdd2;
  p0 = p0 * kMul + kAdd;
  if (axisCourseParamId < 0)
  {
    // ROTATION IN LOCAL SPACE
    mat33f tm;

    for (int i = 0; i < targetNode.size(); i++)
    {
      if (!nodeId[i].target)
        continue;
      auto n_idx = nodeId[i].target;
      mat44f &n_tm = tree.getNodeTm(n_idx);
      if (v_test_all_bits_zeros(n_tm.col0))
        continue;

      real p = p0 * targetNode[nodeId[i].nodeRemap].wt;
      vec4f vp = v_splats(p);
      v_mat33_make_rot_cw(tm, dirAxis, vp);
      mat44f nodeMat = ctx.origTree.getNodeTm(n_idx);
      if (saveScale)
      {
        quat4f rot, originRot;
        vec4f scl, originScl;
        vec3f pos, originPos;
        v_mat4_decompose(nodeMat, originPos, originRot, originScl);
        v_mat4_decompose(n_tm, pos, rot, scl);
        originScl = v_add(originScl, v_splats(-1));
        scl = v_sub(scl, originScl);
        int mask = v_signmask(v_cmp_gt(v_sub(scl, V_C_ONE), v_splats(0.001f)));
        if (mask & 1)
          nodeMat.col0 = v_mul(nodeMat.col0, v_splat_x(scl));
        if (mask & 2)
          nodeMat.col1 = v_mul(nodeMat.col1, v_splat_y(scl));
        if (mask & 4)
          nodeMat.col2 = v_mul(nodeMat.col2, v_splat_z(scl));
      }
      if (relRot)
      {
        mat33f nodeMat33;
        v_mat33_from_mat44(nodeMat33, nodeMat);
        v_mat33_mul(tm, nodeMat33, tm);
      }
      else if (swapYZ)
      {
        vec4f t = tm.col1;
        tm.col1 = tm.col2;
        tm.col2 = t;
      }
      if (leftTm)
        tm.col2 = v_neg(tm.col2);
      BLEND_FINAL_TM(tm, ((mat33f &)n_tm), wt, rotateLocal, leftTm);
      n_tm.col0 = tm.col0;
      n_tm.col1 = tm.col1;
      n_tm.col2 = tm.col2;
      tree.invalidateWtm(n_idx.preceeding());
    }
    return;
  }

  float ang = -(st.getParam(axisCourseParamId) + kCourseAdd) * PI / 180.0f;
  vec3f axis = v_norm3(v_quat_mul_vec3(v_quat_from_unit_vec_ang(rotAxis, v_splats(ang)), dirAxis));
  if (axisFromCharIndex >= 0)
  {
    mat44f animcharTm;
    ctx.ac->getTm(animcharTm);
    if (axisFromCharIndex == 0)
      axis = animcharTm.col0;
    else if (axisFromCharIndex == 1)
      axis = animcharTm.col1;
    else if (axisFromCharIndex == 2)
      axis = animcharTm.col2;
  }

  // ROTATION IN CHARACTER SPACE
  for (int i = 0; i < targetNode.size(); i++)
  {
    if (!nodeId[i].target)
      continue;
    auto n_idx = nodeId[i].target;
    mat44f &n_wtm = tree.getNodeWtmRel(n_idx);
    if (v_test_all_bits_zeros(n_wtm.col0))
      continue;

    real p = p0 * targetNode[nodeId[i].nodeRemap].wt * wt;
    vec4f vp = v_splats(p);
    mat33f wtm, m2;

    tree.partialCalcWtm(n_idx);

    quat4f rotQuat = v_quat_from_unit_vec_ang(axis, vp);

    wtm.col0 = n_wtm.col0;
    wtm.col1 = swapYZ ? n_wtm.col2 : n_wtm.col1;
    wtm.col2 = swapYZ ? n_wtm.col1 : n_wtm.col2;

    bool left_m = v_extract_x(v_mat33_det(wtm));
    if (left_m)
      wtm.col2 = v_neg(wtm.col2);

    quat4f wtmQuat;
    vec3f wtmScale;
    v_mat33_decompose(wtm, wtmQuat, wtmScale);

    quat4f resQuat = v_quat_mul_quat(rotQuat, wtmQuat);
    v_mat33_compose(m2, resQuat, wtmScale);

    if (left_m)
      m2.col2 = v_neg(m2.col2);

    n_wtm.col0 = m2.col0;
    n_wtm.col1 = m2.col1;
    n_wtm.col2 = m2.col2;

    tree.recalcTm(n_idx, false);
    tree.invalidateWtm(n_idx);
  }
}

void AnimPostBlendRotateCtrl::createNode(AnimationGraph &graph, const DataBlock &blk)
{
  const char *name = blk.getStr("name", NULL);
  const char *pname = blk.getStr("param", NULL);
  const char *ac_name = blk.getStr("axis_course", NULL);
  if (ac_name && !ac_name[0])
    ac_name = NULL;

  if (!name)
  {
    DAG_FATAL("not found 'name' param");
    return;
  }
  if (!pname)
  {
    DAG_FATAL("not found 'param' param");
    return;
  }

  String var_name = String("var") + name;

  AnimPostBlendRotateCtrl *node = new AnimPostBlendRotateCtrl(graph, pname, ac_name, blk.getStr("paramAdd", NULL));
  node->kMul = blk.getReal("kMul", 1.0f);
  node->kAdd = blk.getReal("kAdd", 0.0f);
  node->kMul2 = blk.getReal("kMul2", 1.0f);
  node->kAdd2 = blk.getReal("kAdd2", 0.0f);
  node->saveScale = blk.getBool("saveScale", false);
  if (blk.getBool("inRadian", false))
  {
    node->kMul2 *= 180 / PI;
    node->kAdd2 *= 180 / PI;
  }
  else
  {
    node->kMul *= PI / 180;
    node->kAdd *= PI / 180;
  }
  if (const char *pn = blk.getStr("paramAdd2", NULL))
    node->addParam3Id = *pn ? graph.addParamId(pn, IPureAnimStateHolder::PT_ScalarParam) : -1;
  node->kCourseAdd = blk.getReal("kCourseAdd", 0.0f);
  Point4 ra, da;
  ra.set_xyz0(blk.getPoint3("rotAxis", Point3(0, 1, 0)));
  da.set_xyz0(blk.getPoint3("dirAxis", Point3(0, 0, 1)));
  node->rotAxis = v_norm3(v_ldu(&ra.x));
  node->dirAxis = v_norm3(v_ldu(&da.x));
  node->swapYZ = blk.getBool("swapYZ", false);
  node->relRot = blk.getBool("relativeRot", false);
  node->leftTm = blk.getBool("leftTm", !node->swapYZ && !node->relRot);
  node->axisFromCharIndex = blk.getInt("axisFromCharIndex", -1);

  // read node list
  int nid = blk.getNameId("targetNode");
  float def_wt = blk.getReal("targetNodeWt", 1.0f);
  for (int j = 0; j < blk.paramCount(); j++)
    if (blk.getParamType(j) == DataBlock::TYPE_STRING && blk.getParamNameId(j) == nid)
    {
      NodeDesc &nd = node->targetNode.push_back();
      nd.name = blk.getStr(j);
      nd.wt = blk.getReal(String(0, "targetNodeWt%d", node->targetNode.size()), def_wt);
    }

  // allocate state variable
  node->varId = graph.addInlinePtrParamId(var_name, sizeof(NodeId) * node->targetNode.size(), IPureAnimStateHolder::PT_InlinePtr);

  graph.registerBlendNode(node, name);
}


//
// Rotate around node controller
//
AnimPostBlendRotateAroundCtrl::AnimPostBlendRotateAroundCtrl(AnimationGraph &g, const char *param_name) :
  AnimPostBlendCtrl(g), targetNode(midmem)
{
  paramId = graph.addParamId(param_name, IPureAnimStateHolder::PT_ScalarParam);
  rotAxis = V_C_UNIT_0100;
}

void AnimPostBlendRotateAroundCtrl::init(IPureAnimStateHolder &st, const GeomNodeTree &tree)
{
  NodeId *nodeId = (NodeId *)st.getInlinePtr(varId);

  init_target_nodes(tree, targetNode, nodeId);
}

void AnimPostBlendRotateAroundCtrl::process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &)
{
  if (wt <= 0.001f || targetNode.size() < 1)
    return;

  NodeId *nodeId = (NodeId *)st.getInlinePtr(varId);
  float rotationRad = st.getParam(paramId) * kMul + kAdd;
  ;

  // ROTATION IN CHARACTER SPACE
  for (int i = 0; i < targetNode.size(); i++)
  {
    if (!nodeId[i].target)
      continue;
    auto n_idx = nodeId[i].target;
    auto parent = tree.getParentNodeIdx(n_idx);
    mat44f &node_wtm = tree.getNodeWtmRel(n_idx);
    if (v_test_all_bits_zeros(node_wtm.col0) || !parent)
      continue;

    float weightedRotation = rotationRad * targetNode[nodeId[i].nodeRemap].wt * wt;
    vec4f vRotation = v_splats(weightedRotation);
    quat4f rotQuat = v_quat_from_unit_vec_ang(rotAxis, vRotation);

    tree.partialCalcWtm(n_idx);

    const mat44f &parentWtm = tree.getNodeWtmRel(parent);
    mat44f parentItm;
    v_mat44_inverse43(parentItm, parentWtm);

    vec3f localPos = v_mat44_mul_vec3p(parentItm, node_wtm.col3);
    localPos = v_quat_mul_vec3(rotQuat, localPos);
    node_wtm.col3 = v_mat44_mul_vec3p(parentWtm, localPos);

    tree.recalcTm(n_idx, false);
    tree.invalidateWtm(n_idx);
  }
}

void AnimPostBlendRotateAroundCtrl::createNode(AnimationGraph &graph, const DataBlock &blk)
{
  const char *name = blk.getStr("name", NULL);
  const char *pname = blk.getStr("param", NULL);

  if (!name)
  {
    DAG_FATAL("not found 'name' param");
    return;
  }
  if (!pname)
  {
    DAG_FATAL("not found 'param' param");
    return;
  }

  String var_name = String("var") + name;

  AnimPostBlendRotateAroundCtrl *node = new AnimPostBlendRotateAroundCtrl(graph, pname);
  node->kMul = blk.getReal("kMul", 1.0f);
  node->kAdd = blk.getReal("kAdd", 0.0f);
  if (!blk.getBool("inRadian", false))
  {
    node->kMul *= PI / 180;
    node->kAdd *= PI / 180;
  }
  Point4 ra;
  ra.set_xyz0(blk.getPoint3("rotAxis", Point3(0, 1, 0)));
  node->rotAxis = v_norm3(v_ldu(&ra.x));

  // read node list
  int nid = blk.getNameId("targetNode");
  float def_wt = blk.getReal("targetNodeWt", 1.0f);
  for (int j = 0; j < blk.paramCount(); j++)
    if (blk.getParamType(j) == DataBlock::TYPE_STRING && blk.getParamNameId(j) == nid)
    {
      NodeDesc &nd = node->targetNode.push_back();
      nd.name = blk.getStr(j);
      nd.wt = blk.getReal(String(0, "targetNodeWt%d", node->targetNode.size()), def_wt);
    }

  // allocate state variable
  node->varId = graph.addInlinePtrParamId(var_name, sizeof(NodeId) * node->targetNode.size(), IPureAnimStateHolder::PT_InlinePtr);

  graph.registerBlendNode(node, name);
}


//
// Node scale controller
//
AnimPostBlendScaleCtrl::AnimPostBlendScaleCtrl(AnimationGraph &g, const char *param_name, const char *, const char *) :
  AnimPostBlendCtrl(g), targetNode(midmem)
{
  varId = -1;
  paramId = graph.addParamId(param_name, IPureAnimStateHolder::PT_ScalarParam);
  scaleAxis = V_CI_0;
  defaultValue = 1.f;
}

void AnimPostBlendScaleCtrl::init(IPureAnimStateHolder &st, const GeomNodeTree &tree)
{
  NodeId *nodeId = (NodeId *)st.getInlinePtr(varId);
  init_target_nodes(tree, targetNode, nodeId);
  st.setParam(paramId, defaultValue);
}
void AnimPostBlendScaleCtrl::process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx)
{
  if (wt <= 0.001f || targetNode.size() < 1)
    return;
  NodeId *nodeId = (NodeId *)st.getInlinePtr(varId);
  float p0 = st.getParam(paramId);
  for (int i = 0; i < targetNode.size(); i++)
  {
    if (!nodeId[i].target)
      continue;
    auto n_idx = nodeId[i].target;
    mat44f &n_tm = tree.getNodeTm(n_idx);
    if (v_test_all_bits_zeros(n_tm.col0))
      continue;
    mat44f originMat = ctx.origTree.getNodeTm(n_idx);
    mat44f nodeMat = tree.getNodeTm(n_idx);
    vec4f scaleVec = v_mul(v_splats(p0 * wt), scaleAxis);
    float *scv = (float *)&scaleAxis;
    if (scv[0] != 0.f)
    {
      quat4f xR = v_quat_from_arc(v_norm4(originMat.col0), v_norm4(nodeMat.col0));
      originMat.col0 = v_mul(originMat.col0, v_splat_x(scaleVec));
      n_tm.col0 = v_quat_mul_vec3(xR, originMat.col0);
    }
    if (scv[1] != 0.f)
    {
      quat4f yR = v_quat_from_arc(v_norm4(originMat.col1), v_norm4(nodeMat.col1));
      originMat.col1 = v_mul(originMat.col1, v_splat_y(scaleVec));
      n_tm.col1 = v_quat_mul_vec3(yR, originMat.col1);
    }
    if (scv[2] != 0.f)
    {
      quat4f zR = v_quat_from_arc(v_norm4(originMat.col2), v_norm4(nodeMat.col2));
      originMat.col2 = v_mul(originMat.col2, v_splat_z(scaleVec));
      n_tm.col2 = v_quat_mul_vec3(zR, originMat.col2);
    }
    tree.invalidateWtm(n_idx.preceeding());
  }
}

void AnimPostBlendScaleCtrl::createNode(AnimationGraph &graph, const DataBlock &blk)
{
  const char *name = blk.getStr("name", NULL);
  const char *pname = blk.getStr("param", NULL);

  if (!name)
  {
    DAG_FATAL("not found 'name' param");
    return;
  }
  if (!pname)
  {
    DAG_FATAL("not found 'param' param");
    return;
  }

  String var_name = String("var") + name;

  AnimPostBlendScaleCtrl *node = new AnimPostBlendScaleCtrl(graph, pname, nullptr, blk.getStr("paramAdd", NULL));
  Point4 sc;
  sc.set_xyz0(blk.getPoint3("scaleAxis", Point3(0, 0, 0)));
  node->scaleAxis = v_ldu(&sc.x);
  node->defaultValue = blk.getReal("defaultValue", 1.0f);
  int nid = blk.getNameId("targetNode");
  for (int j = 0; j < blk.paramCount(); j++)
    if (blk.getParamType(j) == DataBlock::TYPE_STRING && blk.getParamNameId(j) == nid)
    {
      NodeDesc &nd = node->targetNode.push_back();
      nd.name = blk.getStr(j);
      nd.wt = blk.getReal(String(0, "targetNodeWt%d", node->targetNode.size()), 1.0f);
    }
  node->varId = graph.addInlinePtrParamId(var_name, sizeof(NodeId) * node->targetNode.size(), IPureAnimStateHolder::PT_InlinePtr);
  graph.registerBlendNode(node, name);
}


//
// Node move controller
//
AnimPostBlendMoveCtrl::AnimPostBlendMoveCtrl(AnimationGraph &g, const char *param_name, const char *ac_pname) :
  AnimPostBlendCtrl(g), targetNode(midmem)
{
  varId = -1;
  paramId = graph.addParamId(param_name, IPureAnimStateHolder::PT_ScalarParam);
  axisCourseParamId = ac_pname ? graph.addParamId(ac_pname, IPureAnimStateHolder::PT_ScalarParam) : -1;
  kCourseAdd = 0;
  kAdd = 0;
  kMul = 1.0f;
  rotAxis = V_C_UNIT_0100;
  dirAxis = V_C_UNIT_0010;
  frameRef = FRAME_REF_PARENT;
  additive = false;
  saveOtherAxisMove = false;
}

void AnimPostBlendMoveCtrl::init(IPureAnimStateHolder &st, const GeomNodeTree &tree)
{
  NodeId *nodeId = (NodeId *)st.getInlinePtr(varId);
  init_target_nodes(tree, targetNode, nodeId);
}
void AnimPostBlendMoveCtrl::process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx)
{
  if (wt <= 0.001f || targetNode.size() < 1)
    return;

  NodeId *nodeId = (NodeId *)st.getInlinePtr(varId);
  if (axisCourseParamId < 0)
  {
    // TRANSLATION IN LOCAL SPACE
    real p0 = (st.getParam(paramId) * kMul + kAdd);
    if (additive && !float_nonzero(p0))
      return;
    for (int i = 0; i < targetNode.size(); i++)
    {
      if (!nodeId[i].target)
        continue;
      auto n_idx = nodeId[i].target;
      mat44f &n_tm = tree.getNodeTm(n_idx);
      if (v_test_all_bits_zeros(n_tm.col0))
        continue;

      vec4f pos = additive ? n_tm.col3 : ctx.origTree.getNodeTm(n_idx).col3;
      vec4f dir;
      if (frameRef == FRAME_REF_LOCAL)
        dir = v_mat44_mul_vec3v(n_tm, dirAxis);
      else if (frameRef == FRAME_REF_GLOBAL)
      {
        mat33f &parentWtm = (mat33f &)(tree.getNodeWtmRel(tree.getParentNodeIdx(n_idx)));
        mat33f parentWtmInv;
        v_mat33_inverse(parentWtmInv, parentWtm);
        dir = v_mat33_mul_vec3(parentWtmInv, dirAxis);
      }
      else
        dir = dirAxis;

      real p = p0 * targetNode[nodeId[i].nodeRemap].wt * wt;
      if (saveOtherAxisMove && !additive)
      {
        n_tm.col3 = v_sel(v_madd(dir, v_splats(p), pos), n_tm.col3, v_cmp_eq(dir, v_zero()));
      }
      else
      {
        vec4f vp = v_splats(p);
        n_tm.col3 = v_madd(dir, vp, pos);
      }
      tree.invalidateWtm(n_idx.preceeding());
    }
    return;
  }

  float ang = -(st.getParam(axisCourseParamId) + kCourseAdd) * PI / 180.0f;
  real p0 = (st.getParam(paramId) * kMul + kAdd);
  vec3f axis = v_norm3(v_quat_mul_vec3(v_quat_from_unit_vec_ang(rotAxis, v_splats(ang)), dirAxis));

  // TRANSLATION IN CHARACTER SPACE
  for (int i = 0; i < targetNode.size(); i++)
  {
    if (!nodeId[i].target)
      continue;
    auto n_idx = nodeId[i].target;
    mat44f &n_wtm = tree.getNodeWtmRel(n_idx);
    if (v_test_all_bits_zeros(n_wtm.col0))
      continue;

    real p = p0 * targetNode[nodeId[i].nodeRemap].wt * wt;
    vec4f vp = v_splats(p);

    tree.partialCalcWtm(n_idx);
    n_wtm.col3 = v_madd(axis, vp, n_wtm.col3);
    tree.recalcTm(n_idx, false);
    tree.invalidateWtm(n_idx);
  }
}

void AnimPostBlendMoveCtrl::createNode(AnimationGraph &graph, const DataBlock &blk)
{
  const char *name = blk.getStr("name", NULL);
  const char *pname = blk.getStr("param", NULL);
  const char *ac_name = blk.getStr("axis_course", NULL);
  if (ac_name && !ac_name[0])
    ac_name = NULL;

  if (!name)
  {
    DAG_FATAL("not found 'name' param");
    return;
  }
  if (!pname)
  {
    DAG_FATAL("not found 'param' param");
    return;
  }

  String var_name = String("var") + name;

  AnimPostBlendMoveCtrl *node = new AnimPostBlendMoveCtrl(graph, pname, ac_name);
  node->kCourseAdd = blk.getReal("kCourseAdd", 0.0f);
  node->kMul = blk.getReal("kMul", 1.0f);
  node->kAdd = blk.getReal("kAdd", 0.0f);
  Point4 ra, da;
  ra.set_xyz0(blk.getPoint3("rotAxis", Point3(0, 1, 0)));
  da.set_xyz0(blk.getPoint3("dirAxis", Point3(0, 0, 1)));
  node->rotAxis = v_norm3(v_ldu(&ra.x));
  node->dirAxis = v_norm3(v_ldu(&da.x));
  const char *frameRefNames[] = {"Local", "Parent", "Global"};
  G_ASSERT(countof(frameRefNames) == FRAME_REF_MAX);
  const FrameRef frameRefDef = blk.getBool("localAdditive", false) ? FRAME_REF_LOCAL : FRAME_REF_PARENT;
  node->frameRef =
    (FrameRef)lup(blk.getStr("frameRef", frameRefNames[frameRefDef]), frameRefNames, countof(frameRefNames), frameRefDef);
  node->additive = blk.getBool("additive", blk.getBool("localAdditive", false));
  node->saveOtherAxisMove = blk.getBool("saveOtherAxisMove", blk.getBool("localAdditive2", false));
  G_ASSERTF(!node->additive || !node->saveOtherAxisMove, "additive = %s, but saveOtherAxisMove = %s", //-V547
    node->additive ? "yes" : "no", node->saveOtherAxisMove ? "yes" : "no");                           //-V547

  // read node list
  int nid = blk.getNameId("targetNode");
  float def_wt = blk.getReal("targetNodeWt", 1.0f);
  for (int j = 0; j < blk.paramCount(); j++)
    if (blk.getParamType(j) == DataBlock::TYPE_STRING && blk.getParamNameId(j) == nid)
    {
      NodeDesc &nd = node->targetNode.push_back();
      nd.name = blk.getStr(j);
      nd.wt = blk.getReal(String(0, "targetNodeWt%d", node->targetNode.size()), def_wt);
    }

  // allocate state variable
  node->varId = graph.addInlinePtrParamId(var_name, sizeof(NodeId) * node->targetNode.size(), IPureAnimStateHolder::PT_InlinePtr);

  graph.registerBlendNode(node, name);
}


//
// Controller to sample animations directly to nodes
//
void ApbAnimateCtrl::init(IPureAnimStateHolder &st, const GeomNodeTree &tree)
{
  NodeId *nodeId = (NodeId *)st.getInlinePtr(varId);
  for (int i = 0; i < anim.size(); i++)
    nodeId[i] = tree.findINodeIndex(anim[i].name);
}
void ApbAnimateCtrl::process(IPureAnimStateHolder &st, real /*w*/, GeomNodeTree &tree, AnimPostBlendCtrl::Context &)
{
  NodeId *nodeId = (NodeId *)st.getInlinePtr(varId);
  vec3f p, s;
  quat4f r;

  for (int i = 0; i < rec.size(); i++)
  {
    if (true)
    {
      if (!st.getParamFlags(rec[i].pid, st.PF_Changed))
        continue;
      st.setParamFlags(rec[i].pid, 0, st.PF_Changed);
    }
    int t = (int)floorf(st.getParam(rec[i].pid) * rec[i].pMul + rec[i].pAdd);

    for (int j = rec[i].animIdx, je = j + rec[i].animCnt; j < je; j++)
    {
      auto n_idx = nodeId[j];
      if (!n_idx)
        continue;

      AnimV20Math::PrsAnimNodeSampler<AnimV20Math::OneShotConfig> sampler(anim[j].prs, t);
      sampler.sampleTransform(&p, &r, &s);

      v_mat44_compose(tree.getNodeTm(n_idx), p, r, s);
      tree.invalidateWtm(n_idx.preceeding());
    }
  }
}

static void gather_node_names_by_re(Tab<const char *> &filtered_names, AnimDataChan *ad, RegExp &re)
{
  PatchablePtr<char> *nodeName = ad->nodeName;
  for (int i = 0, ie = ad->nodeNum; i < ie; i++)
    if (re.test(nodeName[i]) && find_value_idx(filtered_names, nodeName[i]) < 0)
      filtered_names.push_back(nodeName[i]);
}

void ApbAnimateCtrl::createNode(AnimationGraph &graph, const DataBlock &blk)
{
  const char *name = blk.getStr("name", NULL);

  if (!name)
  {
    DAG_FATAL("not found 'name' param");
    return;
  }

  ApbAnimateCtrl *node = new ApbAnimateCtrl(graph);
  const char *def_bnl_nm = blk.getStr("bnl", NULL);
  int anim_nid = blk.getNameId("anim");
  int node_nid = blk.getNameId("node");
  int nodeRE_nid = blk.getNameId("nodeRE");
  for (int j = 0; j < blk.blockCount(); j++)
    if (blk.getBlock(j)->getBlockNameId() == anim_nid)
    {
      const DataBlock &b = *blk.getBlock(j);
      const char *nm = b.getStr("param", NULL);
      const char *bnl_nm = b.getStr("bnl", def_bnl_nm);

      if (!nm || !*nm)
      {
        DAG_FATAL("bad param in ApbAnimateCtrl '%s', block %d", nm, name, j);
        continue;
      }
      IAnimBlendNode *n = bnl_nm ? graph.getBlendNodePtr(bnl_nm) : NULL;
      if (!bnl_nm || !*bnl_nm)
      {
        DAG_FATAL("bad BNL ref in ApbAnimateCtrl '%s', block %d", bnl_nm, name, j);
        continue;
      }
      if (!n || !n->isSubOf(AnimBlendNodeLeafCID))
      {
        if (!is_ignoring_unavailable_resources())
          DAG_FATAL("bad BNL ref in ApbAnimateCtrl '%s', block %d", bnl_nm, name, j);
        continue;
      }

      int t0 = b.getInt("sKey", blk.getInt("sKey", 0)) * TIME_TicksPerSec / 30;
      int t1 = b.getInt("eKey", blk.getInt("eKey", 100)) * TIME_TicksPerSec / 30;
      bool mandatoryNodes = b.getBool("mandatoryNodes", false);
      bool arrayNodesOnly = b.getBool("arrayNodesOnly", false);

      AnimData *ad = static_cast<AnimBlendNodeLeaf *>(n)->anim;
      AnimRec &r = node->rec.push_back();
      r.pMul = b.getReal("pMul", 1) * (t1 - t0);
      r.pAdd = t0 + b.getReal("pAdd", 0) * (t1 - t0);
      r.animIdx = node->anim.size();

      Tab<const char *> filtered_names(tmpmem);
      for (int k = 0; k < b.paramCount(); k++)
        if (b.getParamType(k) == DataBlock::TYPE_STRING && b.getParamNameId(k) == nodeRE_nid)
        {
          RegExp re;
          if (!re.compile(b.getStr(k), "i"))
          {
            G_ASSERT_LOG(0, "bad regexp=/%s/ in ApbAnimateCtrl '%s', block %d", b.getStr(k), name, j);
            continue;
          }
          gather_node_names_by_re(filtered_names, ad->dumpData.getChanPoint3(AnimV20::CHTYPE_POSITION), re);
          gather_node_names_by_re(filtered_names, ad->dumpData.getChanPoint3(AnimV20::CHTYPE_SCALE), re);
          gather_node_names_by_re(filtered_names, ad->dumpData.getChanQuat(AnimV20::CHTYPE_ROTATION), re);
        }

      for (int k = 0; k < b.paramCount(); k++)
        if (b.getParamType(k) == DataBlock::TYPE_STRING && b.getParamNameId(k) == node_nid)
        {
          NodeAnim &na = node->anim.push_back();
          na.name = b.getStr(k);
          na.prs = ad->getPrsAnim(na.name);
          if (!na.prs.valid())
          {
            if (mandatoryNodes)
              G_ASSERT_LOG(0, "failed to find node <%s> in BNL '%s', block %d", na.name, bnl_nm, j);
            node->anim.pop_back();
          }
          else if (arrayNodesOnly && !na.prs.hasAnimation())
            node->anim.pop_back();
          else
            for (int m = 0; m < filtered_names.size(); m++)
              if (dd_stricmp(filtered_names[m], na.name) == 0)
              {
                erase_items(filtered_names, m, 1);
                break;
              }
        }
      for (int k = 0; k < filtered_names.size(); k++)
      {
        NodeAnim &na = node->anim.push_back();
        na.name = filtered_names[k];
        na.prs = ad->getPrsAnim(na.name);
        if (!na.prs.valid())
        {
          if (mandatoryNodes)
            G_ASSERT_LOG(0, "failed to find node <%s> (re) in BNL '%s', block %d", na.name, bnl_nm, j);
          node->anim.pop_back();
        }
        else if (arrayNodesOnly && !na.prs.hasAnimation())
          node->anim.pop_back();
      }
      clear_and_shrink(filtered_names);


      r.animCnt = node->anim.size() - r.animIdx;
      if (!r.animCnt)
      {
        if (b.getBool("mandatoryAnim", blk.getBool("mandatoryAnim", false)))
          G_ASSERT_LOG(0, "no nodes in ApbAnimateCtrl '%s', block %d", name, j);
        node->rec.pop_back();
        continue;
      }
      r.pid = graph.addParamId(nm, IPureAnimStateHolder::PT_ScalarParam);
      if (find_value_idx(node->usedAnim, ad) < 0)
        node->usedAnim.push_back(ad);
    }
  node->anim.shrink_to_fit();
  node->rec.shrink_to_fit();

  node->varId =
    graph.addInlinePtrParamId(String(0, "var%s", name), sizeof(NodeId) * node->anim.size(), IPureAnimStateHolder::PT_InlinePtr);
  graph.registerBlendNode(node, name);
}

//
// Create ApbAnimateCtrl and AnimPostBlendRotateCtrl, but keep only one of them active
//


void AnimV20::createNodeApbAnimateAndPostBlendProc(AnimationGraph &graph, const DataBlock &blk, const char *nm_suffix,
  CreateAnimNodeFunc create_anim_node_func)
{
  const int animateNodeBlockNum = blk.findBlock("animateNode");
  if (animateNodeBlockNum < 0)
  {
    DAG_FATAL("'animateNode' block is missed in '%s' block", blk.getBlockName());
    return;
  }

  const DataBlock &animateNodeBlk = *blk.getBlock(animateNodeBlockNum);
  const char *animNodeName = animateNodeBlk.getStr("name", nullptr);
  if (animNodeName == nullptr)
  {
    DAG_FATAL("'name' is missed in '%s' block %d of '%s'", animateNodeBlk.getBlockName(), animateNodeBlockNum, blk.getBlockName());
    return;
  }

  ApbAnimateCtrl::createNode(graph, animateNodeBlk);
  ApbAnimateCtrl *animateNode = static_cast<ApbAnimateCtrl *>(graph.getBlendNodePtr(animNodeName));
  if (animateNode != nullptr && animateNode->isEmpty())
  {
    for (int i = 0; i < blk.blockCount(); ++i)
    {
      if (i == animateNodeBlockNum)
        continue;
      const DataBlock *procNodeBlk = blk.getBlock(i);
      if (procNodeBlk->findParam("targetNode") < 0)
        DAG_FATAL("targetNode:t is missed in '%s' block %d of '%s'", procNodeBlk->getBlockName(), i, blk.getBlockName());
      AnimPostBlendRotateCtrl::createNode(graph, *procNodeBlk);
      (*create_anim_node_func)(graph, *procNodeBlk, nm_suffix);
    }
  }
  else
  {
    for (int i = 0; i < blk.blockCount(); ++i)
    {
      if (i == animateNodeBlockNum)
        continue;
      const DataBlock *procNodeBlk = blk.getBlock(i);
      if (procNodeBlk->findParam("targetNode") < 0)
      {
        DAG_FATAL("targetNode:t is missed in '%s' block %d of '%s'", procNodeBlk->getBlockName(), i, blk.getBlockName());
        continue;
      }
      DataBlock emptyProcNodeBlk = *procNodeBlk;
      emptyProcNodeBlk.removeParam("targetNode");
      (*create_anim_node_func)(graph, emptyProcNodeBlk, nm_suffix);
    }
  }
}

//
// Simple hide controller: zero tm for node when coditions met
//

bool AnimPostBlendCondHideCtrl::loadCondition(const DataBlock &blk, AnimationGraph &graph, const char *name, int index,
  AnimPostBlendCondHideCtrl::Condition &out_condition)
{
  out_condition.op = OPB_INVALID;
  if (load_comp_condition(blk, out_condition.leaf.op, out_condition.leaf.p0, out_condition.leaf.p1))
  {
    const char *pnm = blk.getStr("param", NULL);
    if (!pnm || !*pnm)
    {
      DAG_FATAL("bad param '%s' in AnimPostBlendCondHideCtrl '%s', block %d", pnm, name, index);
      out_condition.leaf.pid = -1;
    }
    else
      out_condition.leaf.pid = graph.addParamId(pnm, IPureAnimStateHolder::PT_ScalarParam);
    return out_condition.leaf.pid >= 0;
  }
  else if (load_bool_operation(blk, out_condition.op))
  {
    int i = 0;
    while (const DataBlock *boolOperandBlk = blk.getBlockByName(String(8, "p%d", i).str()))
    {
      AnimPostBlendCondHideCtrl::Condition *condition = new AnimPostBlendCondHideCtrl::Condition();
      out_condition.branches.push_back() = condition;
      loadCondition(*boolOperandBlk, graph, name, index, *condition);
      ++i;
    }
    switch (out_condition.op)
    {
      case OPB_AND:
      case OPB_OR:
        if (out_condition.branches.size() < 2)
        {
          DAG_FATAL("less than 2 operands for condition '%s' in AnimPostBlendCondHideCtrl %s, block %d", blk.getStr("op", ""), name,
            index);
          return false;
        }
        break;
      case OPB_NOT:
        if (out_condition.branches.size() < 1)
        {
          DAG_FATAL("no operands for condition '%s' in AnimPostBlendCondHideCtrl %s, block %d", blk.getStr("op", ""), name, index);
          return false;
        }
        break;
      default: return false;
    }
    return true;
  }
  else
    return false;
}

void AnimPostBlendCondHideCtrl::deleteCondition(Condition &condition)
{
  for (int i = 0; i < condition.branches.size(); ++i)
  {
    deleteCondition(*condition.branches[i]);
    delete condition.branches[i];
  }
  clear_and_shrink(condition.branches);
}

bool AnimPostBlendCondHideCtrl::checkCondMet(const IPureAnimStateHolder &st, const Condition &condition)
{
  switch (condition.op)
  {
    case OPB_AND:
    {
      for (int i = 0; i < condition.branches.size(); ++i)
        if (!checkCondMet(st, *condition.branches[i]))
          return false;
      return true;
    }
    case OPB_OR:
    {
      for (int i = 0; i < condition.branches.size(); ++i)
        if (checkCondMet(st, *condition.branches[i]))
          return true;
      return false;
    }
    case OPB_NOT:
    {
      for (int i = 0; i < condition.branches.size(); ++i)
        if (checkCondMet(st, *condition.branches[i]))
          return false;
      return true;
    }
    default:
    {
      if (condition.leaf.op == OPC_INACTIVE)
        return true;
      else if (condition.leaf.op == OPC_INVALID)
        return false;
      else if (condition.leaf.pid >= 0)
      {
        const float arg = st.getParam(condition.leaf.pid);
        return check_comp_condition(condition.leaf.op, arg, condition.leaf.p0, condition.leaf.p1);
      }
      else
        return false;
    }
  }
  return false;
}

AnimPostBlendCondHideCtrl::AnimPostBlendCondHideCtrl(AnimationGraph &g) : AnimPostBlendCtrl(g), targetNode(midmem) { varId = -1; }

AnimPostBlendCondHideCtrl::~AnimPostBlendCondHideCtrl()
{
  for (int i = 0; i < targetNode.size(); ++i)
    deleteCondition(targetNode[i].condition);
  clear_and_shrink(targetNode);
}

void AnimPostBlendCondHideCtrl::init(IPureAnimStateHolder &st, const GeomNodeTree &tree)
{
  NodeId *nodeId = (NodeId *)st.getInlinePtr(varId);
  init_target_nodes(tree, targetNode, nodeId);
}

void AnimPostBlendCondHideCtrl::process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx)
{
  if (wt <= 0.999f || targetNode.size() < 1)
    return;

  NodeId *nodeId = (NodeId *)st.getInlinePtr(varId);
  for (int i = 0; i < targetNode.size(); i++)
  {
    auto n_idx = nodeId[i].target;
    if (!n_idx)
      continue;
    mat44f &n_wtm = tree.getNodeWtmRel(n_idx);
    mat44f &n_tm = tree.getNodeTm(n_idx);

    const NodeDesc &desc = targetNode[nodeId[i].nodeRemap];
    if (checkCondMet(st, desc.condition))
    {
      // hide node (set matrix to zero)
      n_wtm.col0 = n_wtm.col1 = n_wtm.col2 = n_tm.col0 = n_tm.col1 = n_tm.col2 = v_zero();
      tree.invalidateWtm(n_idx.preceeding());
    }
    else if (!desc.hideOnly && v_test_all_bits_zeros(n_tm.col0))
    {
      // unhide (restore original tm) only hidden node
      n_tm = ctx.origTree.getNodeTm(n_idx);
      n_wtm = ctx.origTree.getNodeWtmRel(n_idx);
      tree.invalidateWtm(n_idx.preceeding());
    }
  }
}

void AnimPostBlendCondHideCtrl::createNode(AnimationGraph &graph, const DataBlock &blk)
{
  const char *name = blk.getStr("name", NULL);

  if (!name)
  {
    DAG_FATAL("not found 'name' param");
    return;
  }

  String var_name = String("var") + name;

  AnimPostBlendCondHideCtrl *node = new AnimPostBlendCondHideCtrl(graph);

  // read node list
  int nid = blk.getNameId("targetNode");
  for (int j = 0; j < blk.blockCount(); j++)
    if (blk.getBlock(j)->getBlockNameId() == nid)
    {
      const DataBlock &b = *blk.getBlock(j);
      const char *nm = b.getStr("node", NULL);
      bool uncond = b.getBool("always", false);
      if (!nm || !*nm)
      {
        DAG_FATAL("bad dest node %s in AnimPostBlendCondHideCtrl '%s', block %d", nm, name, j);
        continue;
      }

      NodeDesc &nd = node->targetNode.push_back();
      nd.name = nm;
      nd.hideOnly = b.getBool("hideOnly", false);
      if (uncond)
      {
        nd.condition.leaf.pid = -1;
        nd.condition.leaf.op = OPC_INACTIVE;
        nd.condition.leaf.p0 = nd.condition.leaf.p1 = 0;
        nd.condition.op = OPB_INVALID;
        continue;
      }

      if (!loadCondition(b, graph, name, j, nd.condition))
      {
        G_ASSERTF(nd.condition.leaf.op != OPC_INVALID || nd.condition.op != OPB_INVALID,
          "bad condition %s in AnimPostBlendCondHideCtrl '%s', block %d", b.getStr("op", NULL), name, j);
        node->targetNode.pop_back();
        continue;
      }
    }

  // allocate state variable
  node->varId = graph.addInlinePtrParamId(var_name, sizeof(NodeId) * node->targetNode.size(), IPureAnimStateHolder::PT_InlinePtr);

  graph.registerBlendNode(node, name);
}


//
// Controller to change state values procedurally
//
ApbParamCtrl::ApbParamCtrl(AnimationGraph &g, const char *param_name) : AnimPostBlendCtrl(g), rec(midmem), mapRec(midmem)
{
  wtPid = graph.addParamId(param_name, IPureAnimStateHolder::PT_ScalarParam);
}

void ApbParamCtrl::reset(IPureAnimStateHolder &st)
{
  st.setParam(wtPid, 0);
  for (int i = 0; i < rec.size(); i++)
  {
    if (rec[i].chRatePid > 0)
      st.setParam(rec[i].chRatePid, rec[i].chRate);
    if (rec[i].scalePid > 0)
      st.setParam(rec[i].scalePid, 1.0f);
  }
}
enum
{
  OPTYPE_IF,
  OPTYPE_MATH
};
void ApbParamCtrl::process(IPureAnimStateHolder &st, real w, GeomNodeTree &, AnimPostBlendCtrl::Context &)
{
  st.setParam(wtPid, w);
  float lastDt = st.getParam(AnimationGraph::PID_GLOBAL_LAST_DT);
  if (w > 0)
  {
    for (int i = 0; i < ops.size(); ++i)
    {
      float v = st.getParam(ops[i].destPid);
      switch (ops[i].type)
      {
        case OPTYPE_IF:
          if (!check_comp_condition(ops[i].op, v, get_var_or_val(st, ops[i].v0Pid, ops[i].p0),
                get_var_or_val(st, ops[i].v1Pid, ops[i].p1)))
            i += ops[i].skipNum; // skip next expr if not met
          break;
        case OPTYPE_MATH:
        {
          float res = process_math_op(ops[i].op, v, get_var_or_val(st, ops[i].v0Pid, ops[i].p0),
            get_var_or_val(st, ops[i].v1Pid, ops[i].p1), lastDt);
          if (ops[i].weighted)
            res = lerp(v, res, w);
          st.setParam(ops[i].destPid, res);
        }
        break;
      };
    }
  }
}
void ApbParamCtrl::advance(IPureAnimStateHolder &st, real dt)
{
  float w = st.getParam(wtPid);
  if (w > 0)
  {
    for (int i = 0; i < rec.size(); i++)
    {
      float v = st.getParam(rec[i].destPid);
      float cr = rec[i].chRatePid < 0 ? rec[i].chRate : st.getParam(rec[i].chRatePid);
      float s = rec[i].scalePid < 0 ? w : w * st.getParam(rec[i].scalePid);
      v += cr * s * dt;
      if (rec[i].modVal > 0)
        v = fmodf(v, rec[i].modVal);
      st.setParam(rec[i].destPid, v);
    }

    for (int i = 0; i < mapRec.size(); ++i)
    {
      float v = st.getParam(mapRec[i].recPid);
      v = mapRec[i].remapVal.interpolate(v);
      st.setParam(mapRec[i].destPid, v);
    }
  }
}

static void load_ops(const DataBlock &blk, AnimationGraph &graph, Tab<ApbParamCtrl::ParamOp> &ops, const char *name)
{
  int ifNid = blk.getNameId("if");
  int mathNid = blk.getNameId("math");
  for (int j = 0; j < blk.blockCount(); j++)
  {
    const DataBlock &b = *blk.getBlock(j);
    const char *nm = b.getStr("param", NULL);
    if (b.getBlockNameId() == ifNid || b.getBlockNameId() == mathNid)
    {
      if (!nm || !*nm)
      {
        DAG_FATAL("bad dest param in ApbParamCtrl '%s', block %d", nm, name, j);
        continue;
      }
      ApbParamCtrl::ParamOp &r = ops.push_back();
      r.destPid = graph.addParamId(nm, IPureAnimStateHolder::PT_ScalarParam);
      const char *v0Name = b.getStr("v0", NULL);
      r.v0Pid = r.v1Pid = -1;
      if (v0Name)
        r.v0Pid = graph.addParamId(v0Name, IPureAnimStateHolder::PT_ScalarParam);
      const char *v1Name = b.getStr("v1", NULL);
      if (v1Name)
        r.v1Pid = graph.addParamId(v1Name, IPureAnimStateHolder::PT_ScalarParam);
      bool loaded = false;
      if (b.getBlockNameId() == ifNid)
      {
        r.type = OPTYPE_IF;
        loaded = load_comp_condition(b, r.op, r.p0, r.p1);
        if (loaded)
        {
          int prevCount = ops.size();
          load_ops(b, graph, ops, name);
          ops[prevCount - 1].skipNum = ops.size() - prevCount;
        }
      }
      else if (b.getBlockNameId() == mathNid)
      {
        r.type = OPTYPE_MATH;
        r.weighted = b.getBool("out", false);
        loaded = load_math_op(b, r.op, r.p0, r.p1);
      }

      if (!loaded)
        ops.pop_back();
    }
  }
}

void ApbParamCtrl::createNode(AnimationGraph &graph, const DataBlock &blk)
{
  const char *name = blk.getStr("name", NULL);

  if (!name)
  {
    DAG_FATAL("not found 'name' param");
    return;
  }

  ApbParamCtrl *node = new ApbParamCtrl(graph, String(0, "%s.wt", name));
  int nid = blk.getNameId("changeParam");
  int remapId = blk.getNameId("paramRemap");
  for (int j = 0; j < blk.blockCount(); j++)
  {
    const DataBlock &b = *blk.getBlock(j);
    if (b.getBlockNameId() == nid)
    {
      const char *nm = b.getStr("param", NULL);
      if (!nm || !*nm)
      {
        DAG_FATAL("bad dest param in ApbParamCtrl '%s', block %d", nm, name, j);
        continue;
      }

      ChangeRec &r = node->rec.push_back();
      r.destPid = graph.addParamId(nm, IPureAnimStateHolder::PT_ScalarParam);
      r.chRatePid = r.scalePid = -1;
      r.chRate = b.getReal("changeRate", 0);
      r.modVal = b.getReal("mod", 0);
      const char *rateParamName = b.getStr("rateParam", nullptr);
      if (rateParamName != nullptr)
        r.chRatePid = graph.addParamId(rateParamName, IPureAnimStateHolder::PT_ScalarParam);
      else if (b.getBool("variableChangeRate", false))
        r.chRatePid = graph.addParamId(String(0, "%s:CR", nm), IPureAnimStateHolder::PT_ScalarParam);
      const char *scaleParamName = b.getStr("scaleParam", nullptr);
      if (scaleParamName != nullptr)
        r.scalePid = graph.addParamId(scaleParamName, IPureAnimStateHolder::PT_ScalarParam);
      else if (b.getBool("variableScale", false))
        r.scalePid = graph.addParamId(String(0, "%s:CRS", nm), IPureAnimStateHolder::PT_ScalarParam);
    }
    else if (b.getBlockNameId() == remapId)
    {
      const char *nm = b.getStr("param", NULL);
      if (!nm || !*nm)
      {
        DAG_FATAL("bad source param in ApbParamCtrl '%s', block %d", nm, name, j);
        continue;
      }
      const char *destNm = b.getStr("destParam", NULL);
      if (!destNm || !*destNm)
      {
        DAG_FATAL("bad dest param in ApbParamCtrl '%s', block %d", destNm, name, j);
        continue;
      }
      RemapRec &r = node->mapRec.push_back();
      r.remapVal.clear();
      int paramMappingNameId = b.getNameId("paramMapping");
      for (int i = 0; i < b.paramCount(); ++i)
      {
        if (b.getParamNameId(i) != paramMappingNameId)
          continue;
        if (b.getParamType(i) == DataBlock::TYPE_POINT4)
        {
          const Point4 paramMapping = b.getPoint4(i);
          r.remapVal.add(paramMapping.x, paramMapping.z);
          r.remapVal.add(paramMapping.y, paramMapping.w);
        }
        else
        {
          const Point2 val = b.getPoint2(i);
          r.remapVal.add(val.x, val.y);
        }
      }
      r.recPid = graph.addParamId(nm, IPureAnimStateHolder::PT_ScalarParam);      // recipient variable
      r.destPid = graph.addParamId(destNm, IPureAnimStateHolder::PT_ScalarParam); // dest variable
    }
  }
  load_ops(blk, graph, node->ops, name);

  graph.registerBlendNode(node, name);
}


DefClampParamCtrl::DefClampParamCtrl(AnimationGraph &g, const char *param_name) : AnimPostBlendCtrl(g)
{
  wtPid = graph.addParamId(param_name, IPureAnimStateHolder::PT_ScalarParam);
}

void DefClampParamCtrl::reset(IPureAnimStateHolder &st) { st.setParam(wtPid, 0); }

void DefClampParamCtrl::init(IPureAnimStateHolder &st, const GeomNodeTree &)
{
  for (int i = 0; i < ps.size(); ++i)
    st.setParam(ps[i].destPid, ps[i].defaultValue);
}


void DefClampParamCtrl::process(IPureAnimStateHolder &st, real w, GeomNodeTree &, AnimPostBlendCtrl::Context &)
{
  st.setParam(wtPid, w);
  if (w > 0)
  {
    for (int i = 0; i < ps.size(); ++i)
    {
      float v = st.getParam(ps[i].destPid);
      if (ps[i].clampValue.x != 0.f || ps[i].clampValue.y != 0.f)
        st.setParam(ps[i].destPid, clamp(v, ps[i].clampValue.x, ps[i].clampValue.y));
    }
  }
}

void DefClampParamCtrl::createNode(AnimationGraph &graph, const DataBlock &blk)
{
  const char *name = blk.getStr("name", NULL);

  if (!name)
  {
    DAG_FATAL("not found 'name' param");
    return;
  }

  DefClampParamCtrl *node = new DefClampParamCtrl(graph, String(0, "%s.wt", name));
  int paramContr = blk.getNameId("paramContr");
  for (int j = 0; j < blk.blockCount(); j++)
  {
    const DataBlock &b = *blk.getBlock(j);
    const char *nm = b.getStr("param", NULL);
    if (b.getBlockNameId() == paramContr)
    {
      if (!nm || !*nm)
      {
        DAG_FATAL("bad dest param in DefClampParamCtrl '%s', block %d", nm, name, j);
        continue;
      }
      DefClampParamCtrl::Params &r = node->ps.push_back();
      r.destPid = graph.addParamId(nm, IPureAnimStateHolder::PT_ScalarParam);
      r.defaultValue = b.getReal("default", 0.f);
      r.clampValue = b.getPoint2("clamp", Point2(0, 0));
    }
  }
  graph.registerBlendNode(node, name);
}

//
// Simple aim controller
//
AnimPostBlendAimCtrl::AnimPostBlendAimCtrl(AnimationGraph &g) : AnimPostBlendCtrl(g) { varId = -1; }

void AnimPostBlendAimCtrl::init(IPureAnimStateHolder &st, const GeomNodeTree &tree)
{
  AimState &aim = *(AimState *)st.getInlinePtr(varId);
  aim.init();
  aim.nodeId = tree.findINodeIndex(aimDesc.name);
  aim.yaw = aim.yawDst = aimDesc.defaultYaw;
  aim.pitch = aim.pitchDst = aimDesc.defaultPitch;
  aim.clampAngles(aimDesc, aim.yawDst, aim.pitchDst);
  aim.clampAngles(aimDesc, aim.yaw, aim.pitch);
  if (!aim.nodeId)
    logerr("aim.nodeId: node \"%s\" not resolved", aimDesc.name);
}

void AnimPostBlendAimCtrl::AngleLimits::clamp(float &y, float &p) const
{
  y = ::clamp(y, yaw.x, yaw.y);
  p = ::clamp(p, pitch.x, pitch.y);
}

static void solve_1d_acceleration(float target, float &pos, float &spd, float maxAcc, float maxSpd, float dt,
  bool normalize_ang = true)
{
  float delta = target - pos;
  if (normalize_ang)
    delta = norm_s_ang_deg(delta);
  float accToHalfDelta = rabs(safediv(0.5f * delta - spd * dt, sqr(dt) * 0.5f));
  float accToMaxSpd = sign(delta) * min(safediv(maxSpd - sign(delta) * spd, dt), min(accToHalfDelta, maxAcc));
  float accToStop = safediv(0.5f * spd * rabs(spd), delta);
  float deltaAfterFullThrottle = target - (pos + spd * dt + accToMaxSpd * sqr(dt) * 0.5f);
  if (normalize_ang)
    deltaAfterFullThrottle = norm_s_ang_deg(deltaAfterFullThrottle);
  float accToStopNextFrame = safediv(0.5f * spd * rabs(spd), deltaAfterFullThrottle);

  float acceleration = 0.f;
  if (accToStopNextFrame < maxAcc && (delta * deltaAfterFullThrottle) > 0.f)
  {
    acceleration = accToMaxSpd;
  }
  else
  {
    acceleration = clamp(-sign(delta) * accToStop, -maxAcc, maxAcc);
    if (rabs(spd) <= rabs(acceleration * dt) && rabs(delta) <= maxAcc * sqr(dt) * 0.5f)
    {
      pos = target;
      spd = 0.f;
      return;
    }
  }

  pos += spd * dt + acceleration * sqr(dt) * 0.5f;
  spd = clamp(spd + acceleration * dt, -maxSpd, maxSpd);
}

static float apply_stablilization(float error, float stabilizer_angle, float mult, float current_angle)
{
  const float parentSpeed = norm_s_ang_deg(current_angle - stabilizer_angle);
  return clamp(current_angle - parentSpeed * mult, stabilizer_angle - error, stabilizer_angle + error);
}

void AnimPostBlendAimCtrl::AimState::init()
{
  dt = 0.f;
  yaw = 0.f;
  pitch = 0.f;
  yawVel = 0.f;
  pitchVel = 0.f;
  yawDst = 0.f;
  pitchDst = 0.f;
  nodeId = dag::Index16();
  lockYaw = false;
  lockPitch = false;
}

void AnimPostBlendAimCtrl::AimState::updateDstAngles(vec4f target_local_pos)
{
  vec3f angles = v_rad_to_deg(v_dir_to_angles(target_local_pos));
  yawDst = v_extract_x(angles);
  pitchDst = v_extract_y(angles);
}

void AnimPostBlendAimCtrl::AimState::clampAngles(const AimDesc &desc, float &yaw_in_out, float &pitch_in_out)
{
  limits.clamp(yaw_in_out, pitch_in_out);
  for (int limitNo = 0; limitNo < desc.limitsTable.size(); limitNo++)
    if (yaw_in_out > desc.limitsTable[limitNo].yaw.x && yaw_in_out < desc.limitsTable[limitNo].yaw.y)
      desc.limitsTable[limitNo].clamp(yaw_in_out, pitch_in_out);
}

void AnimPostBlendAimCtrl::AimState::updateRotation(const AimDesc &desc, float stab_error, bool stab_yaw, bool stab_pitch,
  float stab_yaw_mult, float stab_pitch_mult, float &prev_yaw, float &prev_pitch)
{
  const bool shouldNormalizeYaw = (desc.limits.yaw.y - desc.limits.yaw.x) >= 360.f;
  if (stab_yaw && rabs(norm_s_ang_deg(prev_yaw - yaw)) < desc.stabilizerMaxDelta)
    yaw = apply_stablilization(stab_error, prev_yaw, stab_yaw_mult, yaw);
  solve_1d_acceleration(yawDst, yaw, yawVel, yawAccel, yawSpeed * yawSpeedMul, dt, shouldNormalizeYaw);
  if (!stab_yaw || rabs(norm_s_ang_deg(yawDst - yaw)) < rabs(norm_s_ang_deg(yawDst - prev_yaw)))
    prev_yaw = yaw;

  const bool shouldNormalizePitch = (desc.limits.pitch.y - desc.limits.pitch.x) >= 360.f;
  if (stab_pitch && rabs(norm_s_ang_deg(prev_pitch - pitch)) < desc.stabilizerMaxDelta)
    pitch = apply_stablilization(stab_error, prev_pitch, stab_pitch_mult, pitch);
  solve_1d_acceleration(pitchDst, pitch, pitchVel, pitchAccel, pitchSpeed * pitchSpeedMul, dt, shouldNormalizePitch);
  if (!stab_pitch || rabs(norm_s_ang_deg(pitchDst - pitch)) < rabs(norm_s_ang_deg(pitchDst - prev_pitch)))
    prev_pitch = pitch;
}

void AnimPostBlendAimCtrl::advance(IPureAnimStateHolder &st, real dt)
{
  AimState &aim = *(AimState *)st.getInlinePtr(varId);
  aim.dt = dt;
}

void AnimPostBlendAimCtrl::process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx)
{
  if (wt <= 0.999f)
    return;

  const float yaw = targetYawId >= 0 ? st.getParam(targetYawId) : 0.f;
  const float pitch = targetPitchId >= 0 ? st.getParam(targetPitchId) : 0.f;

  vec3f dir = v_angles_to_dir(v_deg_to_rad(v_make_vec4f(-yaw, pitch, 0, 0)));

  AimState &aim = *(AimState *)st.getInlinePtr(varId);

  if (!aim.nodeId)
    return;

  const mat44f &origNode_tm = ctx.origTree.getNodeTm(aim.nodeId);

  float yawSpeedParam = yawSpeedId >= 0 ? st.getParam(yawSpeedId) : -1.f;
  float pitchSpeedParam = pitchSpeedId >= 0 ? st.getParam(pitchSpeedId) : -1.f;

  aim.limits.yaw.x = aimDesc.minYawAngleId < 0 ? aimDesc.limits.yaw.x : st.getParam(aimDesc.minYawAngleId);
  aim.limits.yaw.y = aimDesc.maxYawAngleId < 0 ? aimDesc.limits.yaw.y : st.getParam(aimDesc.maxYawAngleId);
  aim.limits.pitch.x = aimDesc.minPitchAngleId < 0 ? aimDesc.limits.pitch.x : st.getParam(aimDesc.minPitchAngleId);
  aim.limits.pitch.y = aimDesc.maxPitchAngleId < 0 ? aimDesc.limits.pitch.y : st.getParam(aimDesc.maxPitchAngleId);

  aim.yawSpeed = yawSpeedParam >= 0.f ? yawSpeedParam : aimDesc.yawSpeed;
  aim.pitchSpeed = pitchSpeedParam >= 0.f ? pitchSpeedParam : aimDesc.pitchSpeed;

  float yawAccelParam = yawAccelId >= 0 ? st.getParam(yawAccelId) : -1.f;
  float pitchAccelParam = pitchAccelId >= 0 ? st.getParam(pitchAccelId) : -1.f;
  aim.yawAccel = yawAccelParam >= 0.f ? yawAccelParam : aimDesc.yawAccel >= 0.f ? aimDesc.yawAccel : aim.yawSpeed;
  aim.pitchAccel = pitchAccelParam >= 0.f ? pitchAccelParam : aimDesc.pitchAccel >= 0.f ? aimDesc.pitchAccel : aim.pitchSpeed;

  if (yawSpeedMulId >= 0)
    aim.yawSpeedMul = st.getParam(yawSpeedMulId);
  if (pitchSpeedMulId >= 0)
    aim.pitchSpeedMul = st.getParam(pitchSpeedMulId);

  mat44f tm, itm;
  if (auto pnode_idx = tree.getParentNodeIdx(aim.nodeId))
  {
    v_mat44_mul43(tm, tree.getNodeWtmRel(pnode_idx), origNode_tm);
    v_mat44_inverse43(itm, tm);
  }
  else
  {
    v_mat44_ident(tm);
    v_mat44_ident(itm);
  }

  aim.updateDstAngles(v_mat44_mul_vec3v(itm, dir));

  Point2 prevLocalAngles(aim.yawDst, aim.pitchDst);
  const bool hasStabilizer = hasStabId >= 0 && st.getParam(hasStabId) > 0.5f && prevYawId >= 0 && prevPitchId >= 0;
  const bool isYawStabilized = hasStabilizer && stabYawId >= 0 && st.getParam(stabYawId) > 0.5f;
  const bool isPitchStabilized = hasStabilizer && stabPitchId >= 0 && st.getParam(stabPitchId) > 0.5f;
  if (isYawStabilized || isPitchStabilized)
  {
    const float prevYaw = st.getParam(prevYawId);
    const float prevPitch = st.getParam(prevPitchId);
    vec3f prevDir = v_angles_to_dir(v_deg_to_rad(v_make_vec4f(-prevYaw, prevPitch, 0, 0)));
    v_stu_half(&prevLocalAngles.x, v_rad_to_deg(v_dir_to_angles(v_mat44_mul_vec3v(itm, prevDir))));
  }

  aim.clampAngles(aimDesc, aim.yawDst, aim.pitchDst);
  aim.updateRotation(aimDesc, stabErrorId >= 0 ? st.getParam(stabErrorId) : 0.f, isYawStabilized, isPitchStabilized,
    stabYawMultId >= 0 ? st.getParam(stabYawMultId) : 1.0f, stabPitchMultId >= 0 ? st.getParam(stabPitchMultId) : 1.0f,
    prevLocalAngles.x, prevLocalAngles.y);

  if (curYawId >= 0)
    st.setParam(curYawId, aim.yaw);
  if (curPitchId >= 0)
    st.setParam(curPitchId, aim.pitch);
  if (curWorldYawId >= 0 && curWorldPitchId >= 0)
  {
    vec3f curDirLocal = v_angles_to_dir(v_deg_to_rad(v_make_vec4f(-aim.yaw, aim.pitch, 0, 0)));
    curDirLocal = v_perm_xycd(curDirLocal, v_neg(curDirLocal)); // curDirLocal.z = -curDirLocal.z;
    Point2 curDir;
    v_stu_half(&curDir.x, v_rad_to_deg(v_dir_to_angles(v_mat44_mul_vec3v(tm, curDirLocal))));
    st.setParam(curWorldYawId, curDir.x);
    st.setParam(curWorldPitchId, curDir.y);
  }

  if (hasStabilizer)
  {
    vec3f curDirLocal = v_angles_to_dir(v_deg_to_rad(v_make_vec4f(-prevLocalAngles.x, prevLocalAngles.y, 0, 0)));
    curDirLocal = v_perm_xycd(curDirLocal, v_neg(curDirLocal)); // curDirLocal.z = -curDirLocal.z;
    Point2 curDir;
    v_stu_half(&curDir.x, v_rad_to_deg(v_dir_to_angles(v_mat44_mul_vec3v(tm, curDirLocal))));
    curDir.x = -curDir.x;

    st.setParam(prevYawId, curDir.x);
    st.setParam(prevPitchId, curDir.y);
  }
}

void AnimPostBlendAimCtrl::createNode(AnimationGraph &graph, const DataBlock &blk)
{
  const char *name = blk.getStr("name", NULL);

  if (!name)
  {
    DAG_FATAL("not found 'name' param");
    return;
  }

  AnimPostBlendAimCtrl *node = new AnimPostBlendAimCtrl(graph);

  node->aimDesc.name = blk.getStr("node");
  node->aimDesc.defaultYaw = blk.getReal("defaultYaw", node->aimDesc.defaultYaw);
  node->aimDesc.defaultPitch = blk.getReal("defaultPitch", node->aimDesc.defaultPitch);
  node->aimDesc.yawSpeed = blk.getReal("yawSpeed", 0.f);
  node->aimDesc.pitchSpeed = blk.getReal("pitchSpeed", 0.f);
  node->aimDesc.yawAccel = blk.getReal("yawAccel", -1.f);
  node->aimDesc.pitchAccel = blk.getReal("pitchAccel", -1.f);

  node->aimDesc.limits.load(blk.getPoint4("limit"));
  const DataBlock *limitsTableBlk = blk.getBlockByName("limitsTable");
  if (limitsTableBlk)
  {
    node->aimDesc.limitsTable.resize(limitsTableBlk->paramCount());
    for (int i = 0; i < limitsTableBlk->paramCount(); ++i)
      node->aimDesc.limitsTable[i].load(limitsTableBlk->getPoint4(i));
  }
  SimpleString paramName(blk.getStr("param", name));

  if (blk.paramExists("paramYawSpeedMul"))
    node->yawSpeedMulId = graph.addParamId(blk.getStr("paramYawSpeedMul"), IPureAnimStateHolder::PT_ScalarParam);
  if (blk.paramExists("paramPitchSpeedMul"))
    node->pitchSpeedMulId = graph.addParamId(blk.getStr("paramPitchSpeedMul"), IPureAnimStateHolder::PT_ScalarParam);
  if (blk.paramExists("paramYawSpeed"))
    node->yawSpeedId = graph.addParamId(blk.getStr("paramYawSpeed"), IPureAnimStateHolder::PT_ScalarParam);
  if (blk.paramExists("paramPitchSpeed"))
    node->pitchSpeedId = graph.addParamId(blk.getStr("paramPitchSpeed"), IPureAnimStateHolder::PT_ScalarParam);
  if (blk.paramExists("paramYawAccel"))
    node->yawAccelId = graph.addParamId(blk.getStr("paramYawAccel"), IPureAnimStateHolder::PT_ScalarParam);
  if (blk.paramExists("paramPitchAccel"))
    node->pitchAccelId = graph.addParamId(blk.getStr("paramPitchAccel"), IPureAnimStateHolder::PT_ScalarParam);

  if (blk.paramExists("paramMinYawAngle"))
    node->aimDesc.minYawAngleId = graph.addParamId(blk.getStr("paramMinYawAngle"), IPureAnimStateHolder::PT_ScalarParam);
  if (blk.paramExists("paramMaxYawAngle"))
    node->aimDesc.maxYawAngleId = graph.addParamId(blk.getStr("paramMaxYawAngle"), IPureAnimStateHolder::PT_ScalarParam);
  if (blk.paramExists("paramMinPitchAngle"))
    node->aimDesc.minPitchAngleId = graph.addParamId(blk.getStr("paramMinPitchAngle"), IPureAnimStateHolder::PT_ScalarParam);
  if (blk.paramExists("paramMaxPitchAngle"))
    node->aimDesc.maxPitchAngleId = graph.addParamId(blk.getStr("paramMaxPitchAngle"), IPureAnimStateHolder::PT_ScalarParam);


  {
    node->targetYawId =
      graph.addParamId(blk.getStr("paramTargetYaw", String(0, "%s:targetYaw", paramName)), IPureAnimStateHolder::PT_ScalarParam);
    node->curYawId = graph.addParamId(blk.getStr("paramYaw", String(0, "%s:yaw", paramName)), IPureAnimStateHolder::PT_ScalarParam);
    if (blk.paramExists("paramWorldYaw"))
      node->curWorldYawId = graph.addParamId(blk.getStr("paramWorldYaw"), IPureAnimStateHolder::PT_ScalarParam);
    node->prevYawId =
      graph.addParamId(blk.getStr("paramPrevYaw", String(0, "%s:prevYaw", paramName)), IPureAnimStateHolder::PT_ScalarParam);
  }

  {
    node->targetPitchId =
      graph.addParamId(blk.getStr("paramTargetPitch", String(0, "%s:targetPitch", paramName)), IPureAnimStateHolder::PT_ScalarParam);
    node->curPitchId =
      graph.addParamId(blk.getStr("paramPitch", String(0, "%s:pitch", paramName)), IPureAnimStateHolder::PT_ScalarParam);
    if (blk.paramExists("paramWorldPitch"))
      node->curWorldPitchId = graph.addParamId(blk.getStr("paramWorldPitch"), IPureAnimStateHolder::PT_ScalarParam);
    node->prevPitchId =
      graph.addParamId(blk.getStr("paramPrevPitch", String(0, "%s:prevPitch", paramName)), IPureAnimStateHolder::PT_ScalarParam);
  }

  {
    node->hasStabId =
      graph.addParamId(blk.getStr("paramHasStab", String(0, "%s:hasStab", paramName)), IPureAnimStateHolder::PT_ScalarParam);
    node->stabYawId =
      graph.addParamId(blk.getStr("paramStabYaw", String(0, "%s:stabYaw", paramName)), IPureAnimStateHolder::PT_ScalarParam);
    node->stabYawMultId =
      graph.addParamId(blk.getStr("paramStabYawMult", String(0, "%s:stabYawMult", paramName)), IPureAnimStateHolder::PT_ScalarParam);

    node->stabPitchId =
      graph.addParamId(blk.getStr("paramStabPitch", String(0, "%s:stabPitch", paramName)), IPureAnimStateHolder::PT_ScalarParam);
    node->stabPitchMultId = graph.addParamId(blk.getStr("paramStabPitchMult", String(0, "%s:stabPitchMult", paramName)),
      IPureAnimStateHolder::PT_ScalarParam);

    node->stabErrorId =
      graph.addParamId(blk.getStr("paramStabError", String(0, "%s:stabError", paramName)), IPureAnimStateHolder::PT_ScalarParam);
  }

  // allocate state variable
  node->varId = graph.addInlinePtrParamId(String(0, "var%s", blk.getStr("param", paramName)), sizeof(AimState),
    IPureAnimStateHolder::PT_InlinePtr);

  graph.registerBlendNode(node, name);
}

//
// GeomNode attach Controller
//
void AttachGeomNodeCtrl::reset(IPureAnimStateHolder &st)
{
  if (perAnimStateDataVarId >= 0)
    memset(st.getInlinePtr(perAnimStateDataVarId), 0xFF, sizeof(int16_t) * nodeNames.size());
}
void AttachGeomNodeCtrl::init(IPureAnimStateHolder &st, const GeomNodeTree &tree)
{
  G_ASSERT(perAnimStateDataVarId >= 0);
  dag::Span<dag::Index16> nodeIds((dag::Index16 *)st.getInlinePtr(perAnimStateDataVarId), nodeNames.size());

  mem_set_ff(nodeIds);
  for (int i = 0; i < nodeNames.size(); i++)
  {
    nodeIds[i] = tree.findNodeIndex(nodeNames[i]);
    AttachDesc &att = AttachGeomNodeCtrl::getAttachDesc(st, destVarId[i].nodeWtm);
    att.wtm.identity();
    att.w = 0;
  }
}
void AttachGeomNodeCtrl::process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx)
{
  if (wt < 1e-4f || tree.empty())
    return;

  dag::Span<dag::Index16> nodeIds((dag::Index16 *)st.getInlinePtr(perAnimStateDataVarId), nodeNames.size());
  G_ASSERT_AND_DO(nodeIds.size(), return);

  for (int i = 0; i < nodeIds.size(); i++)
  {
    if (!nodeIds[i])
      continue;
    AttachDesc &att = AttachGeomNodeCtrl::getAttachDesc(st, destVarId[i].nodeWtm);
    float w = att.w * wt * getWeightMul(st, destVarId[i].wScale, destVarId[i].wScaleInverted);
    if (w < 1e-4f)
      continue;

    mat44f &n_wtm = tree.getNodeWtmRel(nodeIds[i]);
    tree.partialCalcWtm(nodeIds[i]);
    mat44f attWtm;
    v_mat44_make_from_43cu(attWtm, att.wtm.array);
    if (ctx.acScale)
    {
      vec3f s = v_max(v_min(*ctx.acScale, maxNodeScale), minNodeScale);
      attWtm.col0 = v_mul(attWtm.col0, v_splat_x(s));
      attWtm.col1 = v_mul(attWtm.col1, v_splat_y(s));
      attWtm.col2 = v_mul(attWtm.col2, v_splat_z(s));
    }
    if (w >= 0.9999f)
    {
      n_wtm = attWtm;
    }
    else
    {
      mat33f m, om;
      v_mat33_from_mat44(m, attWtm);
      v_mat33_from_mat44(om, n_wtm);
      BLEND_FINAL_WTM(m, om, w, attach);

      n_wtm.set33(m);
      n_wtm.col3 = v_perm_xyzd(v_lerp_vec4f(v_splats(w), n_wtm.col3, attWtm.col3), attWtm.col3);
    }
    tree.recalcTm(nodeIds[i]);
    tree.invalidateWtm(nodeIds[i]);
  }
}
void AttachGeomNodeCtrl::advance(IPureAnimStateHolder & /*st*/, real /*dt*/) {}
void AttachGeomNodeCtrl::createNode(AnimationGraph &graph, const DataBlock &blk)
{
  const char *name = blk.getStr("name", NULL);
  G_ASSERTF_AND_DO(name != NULL, return, "not found '%s' param", "name");

  AttachGeomNodeCtrl *node = new AttachGeomNodeCtrl(graph);
  for (int i = 0, nid = blk.getNameId("attach"); i < blk.blockCount(); i++)
  {
    const DataBlock &b = *blk.getBlock(i);
    if (b.getBlockNameId() != nid)
      continue;
    const char *var_nm = b.getStr("var", NULL);
    const char *node_nm = b.getStr("node", NULL);
    if ((!var_nm || !*var_nm) || (!node_nm || !*node_nm))
    {
      DAG_FATAL("AttachGeomNodeCtrl<%s> bad params: var=%s node=%s", name, var_nm, node_nm);
      continue;
    }
    node->nodeNames.push_back() = node_nm;
    VarId &dv = node->destVarId.push_back();
    dv.wScale = graph.addParamIdEx(b.getStr("wtModulate", blk.getStr("wtModulate", NULL)), IPureAnimStateHolder::PT_ScalarParam);
    dv.wScaleInverted = b.getBool("wtModulateInverse", blk.getBool("wtModulateInverse", false));
    dv.nodeWtm = graph.addInlinePtrParamId(var_nm, sizeof(AttachDesc));
  }
  Point3_vec4 minScale{}, maxScale{};                        //-V519
  minScale = blk.getPoint3("minNodeScale", Point3(1, 1, 1)); //-V519
  maxScale = blk.getPoint3("maxNodeScale", Point3(1, 1, 1)); //-V519
  node->minNodeScale = v_ld(&minScale.x);
  node->maxNodeScale = v_ld(&maxScale.x);
  node->perAnimStateDataVarId =
    graph.addInlinePtrParamId(String(0, "%s$treeCtx", name), sizeof(dag::Index16) * node->nodeNames.size());
  graph.registerBlendNode(node, name);
}
//
// Node dir controller
//
AnimPostBlendNodeLookatCtrl::AnimPostBlendNodeLookatCtrl(AnimationGraph &g, const char *param_name) : AnimPostBlendCtrl(g)
{
  varId = -1;
  v_mat33_ident(tmRot);
  v_mat33_ident(itmRot);
  String pname;
  pname.printf(0, "%s.x", param_name);
  paramXId = graph.addParamId(pname.str(), IPureAnimStateHolder::PT_ScalarParam);
  pname.printf(0, "%s.y", param_name);
  paramYId = graph.addParamId(pname.str(), IPureAnimStateHolder::PT_ScalarParam);
  pname.printf(0, "%s.z", param_name);
  paramZId = graph.addParamId(pname.str(), IPureAnimStateHolder::PT_ScalarParam);
}

void AnimPostBlendNodeLookatCtrl::init(IPureAnimStateHolder &st, const GeomNodeTree &tree)
{
  NodeId *nodeId = (NodeId *)st.getInlinePtr(varId);

  init_target_nodes(tree, targetNodes, nodeId);
}
void AnimPostBlendNodeLookatCtrl::process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &)
{
  if (wt <= 0.001f || targetNodes.size() < 1)
    return;

  NodeId *nodeId = (NodeId *)st.getInlinePtr(varId);
  Point3_vec4 pos(st.getParam(paramXId), st.getParam(paramYId), st.getParam(paramZId));
  vec3f v_pos = v_ld(&pos.x);

  for (int i = 0; i < targetNodes.size(); i++)
  {
    auto n_idx = nodeId[i].target;
    if (!n_idx)
      continue;
    if (v_test_all_bits_zeros(tree.getNodeTm(n_idx).col0))
      continue;

    mat44f &n_wtm = tree.getNodeWtmRel(n_idx);
    mat33f nodeWtm;
    nodeWtm.col0 = n_wtm.col0;
    nodeWtm.col1 = n_wtm.col1;
    nodeWtm.col2 = n_wtm.col2;

    mat33f wtm;
    v_mat33_mul(wtm, nodeWtm, tmRot);

    vec3f dir = v_norm3(v_sub(v_pos, n_wtm.col3));
    wtm.col2 = dir;
    wtm.col1 = v_norm3(v_cross3(wtm.col2, wtm.col0));
    wtm.col0 = v_norm3(v_cross3(wtm.col1, wtm.col2));
    if (swapYZ)
    {
      vec4f t = wtm.col1;
      wtm.col1 = wtm.col2;
      wtm.col2 = t;
    }
    if (!leftTm)
      wtm.col2 = v_neg(wtm.col2);

    v_mat33_mul(nodeWtm, wtm, itmRot);

    n_wtm.col0 = nodeWtm.col0;
    n_wtm.col1 = nodeWtm.col1;
    n_wtm.col2 = nodeWtm.col2;
    tree.recalcTm(n_idx, false);
    tree.invalidateWtm(n_idx);
  }
}

void AnimPostBlendNodeLookatCtrl::createNode(AnimationGraph &graph, const DataBlock &blk)
{
  const char *name = blk.getStr("name", NULL);
  const char *pname = blk.getStr("param", NULL);

  if (!name)
  {
    DAG_FATAL("not found 'name' param in lookat controller");
    return;
  }
  if (!pname)
  {
    DAG_FATAL("not found 'param' param in '%s'", name);
    return;
  }

  String var_name = String("var") + name;

  AnimPostBlendNodeLookatCtrl *node = new AnimPostBlendNodeLookatCtrl(graph, pname);

  Point3 euler = blk.getPoint3("rot_euler", Point3(0, 0, 0)) * PI / 180;
  v_mat33_make_rot_cw_zyx(node->tmRot, v_neg(v_ldu(&euler.x)));
  v_mat33_inverse(node->itmRot, node->tmRot);

  node->leftTm = blk.getBool("leftTm", true);
  node->swapYZ = blk.getBool("swapYZ", false);

  // read node list
  int nid = blk.getNameId("targetNode");
  for (int j = 0; j < blk.paramCount(); ++j)
    if (blk.getParamType(j) == DataBlock::TYPE_STRING && blk.getParamNameId(j) == nid)
      node->targetNodes.push_back().name = blk.getStr(j);

  if (node->targetNodes.empty())
    DAG_FATAL("no target nodes in '%s'", name);

  // allocate state variable
  node->varId = graph.addInlinePtrParamId(var_name, sizeof(NodeId) * node->targetNodes.size(), IPureAnimStateHolder::PT_InlinePtr);

  graph.registerBlendNode(node, name);
}

//
// Node dir controller with lookat and up nodes
//
void AnimPostBlendNodeLookatNodeCtrl::init(IPureAnimStateHolder &st, const GeomNodeTree &tree)
{
  LocalData &ldata = *(LocalData *)st.getInlinePtr(varId);
  ldata.targetNodeId = resolve_node_by_name(tree, targetNode);

  ldata.lookatNodeId = tree.findINodeIndex(lookatNodeName);
  if (!ldata.lookatNodeId)
  {
    DAG_FATAL("lookat node '%s' does not exist", lookatNodeName);
    valid = false;
  }
  if (upNodeName)
  {
    ldata.upNodeId = tree.findINodeIndex(upNodeName);
    if (!ldata.upNodeId)
    {
      DAG_FATAL("up node '%s' does not exist", upNodeName);
      valid = false;
    }
  }
}

void AnimPostBlendNodeLookatNodeCtrl::process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &)
{
  if (wt <= 0.001f || !valid)
    return;

  LocalData &ldata = *(LocalData *)st.getInlinePtr(varId);
  const vec3f v_pos = tree.getNodeWposRel(ldata.lookatNodeId);

  auto n_idx = ldata.targetNodeId;
  if (!n_idx)
    return;
  if (v_test_all_bits_zeros(tree.getNodeTm(n_idx).col0))
    return;

  mat44f &n_wtm = tree.getNodeWtmRel(n_idx);
  const vec4f lookatDir = v_norm3(v_sub(v_pos, n_wtm.col3));
  vec4f dirUp;
  if (!upNodeName.empty())
  {
    const vec4f upNodePos = tree.getNodeWposRel(ldata.upNodeId);
    dirUp = v_norm3(v_sub(upNodePos, n_wtm.col3));
  }
  else
  {
    dirUp = v_ldu_p3(&upVector.x);
  }

  const vec4f sideDir = v_norm3(v_cross3(lookatDir, dirUp));
  const vec4f upDir = v_norm3(v_cross3(sideDir, lookatDir));

  (&n_wtm.col0)[lookatAxis] = lookatDir;
  (&n_wtm.col0)[upAxis] = upDir;
  (&n_wtm.col0)[3 - lookatAxis - upAxis] = sideDir;

  for (int i = 0; i < 3; ++i)
  {
    if (negAxes[i])
      (&n_wtm.col0)[i] = v_neg((&n_wtm.col0)[i]);
  }

  tree.recalcTm(n_idx, false);
  tree.invalidateWtm(n_idx);
}

void AnimPostBlendNodeLookatNodeCtrl::createNode(AnimationGraph &graph, const DataBlock &blk)
{
  const char *name = blk.getStr("name", NULL);

  if (!name)
  {
    DAG_FATAL("not found 'name' param in lookat controller");
    return;
  }

  const char *targetNodeName = blk.getStr("targetNode", "");
  const char *lookatNodeName = blk.getStr("lookatNode", "");
  const char *upNodeName = blk.getStr("upNode", NULL);

  // Necessary paramaters: name, targetNode, lookatNode, upNode. Optional parameters: lookatAxis, upAxis, negX, negY, negZ.
  // The target node will 'look' at lookatNode with lookatAxis and then turn around lookatAxis in such a way
  // that the angle between upAxis and the vector from the target node to upNode is minimized.
  // Afterwards, the xyz axes can be reversed if negX, negY or negZ are true.

  const bool valid = (*targetNodeName && *lookatNodeName);

  if (!valid)
  {
    DAG_FATAL("lookat node controller should either have 'targetNode', and 'lookatNode' specified");
    return;
  }

  AnimPostBlendNodeLookatNodeCtrl *node = new AnimPostBlendNodeLookatNodeCtrl(graph);
  node->valid = valid;
  node->targetNode = targetNodeName;
  node->lookatNodeName = lookatNodeName;
  node->upNodeName = upNodeName;

  node->lookatAxis = blk.getInt("lookatAxis", 2);
  node->upAxis = blk.getInt("upAxis", 1);
  node->lookatAxis = clamp(node->lookatAxis, 0, 2);
  node->upAxis = clamp(node->upAxis, 0, 2);
  if (node->lookatAxis == node->upAxis)
    node->upAxis = (node->upAxis + 1) % 3;

  if (upNodeName == NULL)
  {
    node->upVector = blk.getPoint3("upVector", Point3(0, 1, 0));
  }

  node->negAxes[0] = blk.getBool("negX", false);
  node->negAxes[1] = blk.getBool("negY", false);
  node->negAxes[2] = blk.getBool("negZ", false);

  // allocate state variable
  node->varId = graph.addInlinePtrParamId(String("var") + name, sizeof(LocalData), IPureAnimStateHolder::PT_InlinePtr);

  graph.registerBlendNode(node, name);
}

AnimPostBlendEffFromAttachement::AnimPostBlendEffFromAttachement(AnimationGraph &g) :
  AnimPostBlendCtrl(g), nodes(midmem), destVarId(midmem), namedSlotId(-1), varSlotId(-1), varId(-1)
{}
void AnimPostBlendEffFromAttachement::init(IPureAnimStateHolder &st, const GeomNodeTree &tree)
{
  LocalData &ldata = *(LocalData *)st.getInlinePtr(varId);
  ldata.lastRefUid = INVALID_ATTACHMENT_UID;
  for (int i = 0; i < nodes.size(); i++)
  {
    ldata.node[i].src = dag::Index16();
    ldata.node[i].dest = nodes[i].dest.empty() ? dag::Index16() : tree.findINodeIndex(nodes[i].dest);
  }
}
void AnimPostBlendEffFromAttachement::process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx)
{
  if (wt <= 0.001f || !nodes.size())
    return;

  LocalData &ldata = *(LocalData *)st.getInlinePtr(varId);
  const int slotId = varSlotId >= 0 ? (int)st.getParam(varSlotId) : namedSlotId;
  GeomNodeTree *att_tree = slotId < 0 ? &tree : ctx.ac->getAttachedSkeleton(slotId);

  if (!att_tree)
  {
    for (int i = 0; i < nodes.size(); i++)
    {
      // detach effector
      st.paramEffector(destVarId[i].eff).setLooseEnd();
      if (destVarId[i].wtm >= 0)
        AnimV20::AttachGeomNodeCtrl::getAttachDesc(st, destVarId[i].wtm).w = 0;
    }
    ldata.lastRefUid = INVALID_ATTACHMENT_UID;
    return;
  }

  // prepare attachment skeleton
  unsigned att_uid = slotId < 0 ? 0xFFFFFFFF : ctx.ac->getAttachmentUid(slotId);
  dag::Index16 n_idx;
  if (slotId >= 0)
  {
    n_idx = ctx.ac->getSlotNodeIdx(slotId);
    tree.partialCalcWtm(n_idx);
    mat44f wtm;
    if (const mat44f *att_tm = ctx.ac->getAttachmentTm(slotId))
      v_mat44_mul(wtm, tree.getNodeWtmRel(n_idx), *att_tm);
    else
      wtm = tree.getNodeWtmRel(n_idx);
    if (auto att = ctx.ac->getAttachedChar(slotId))
    {
      wtm.col3 = v_add(wtm.col3, ctx.worldTranslate);
      att->setTmWithOfs(wtm, ctx.ac->getWtmOfs());
      att->doRecalcAnimAndWtm();
    }
    else
    {
      att_tree->invalidateWtm(dag::Index16(0));
      att_tree->setRootTm(wtm, ctx.ac->getWtmOfs());
      att_tree->calcWtm();
      att_tree->translate(ctx.worldTranslate);
    }
  }

  for (int i = 0; i < nodes.size(); i++)
  {
    if (ldata.lastRefUid != att_uid)
      ldata.node[i].src = att_tree->findINodeIndex(nodes[i].src);
    float w = wt * getWeightMul(st, destVarId[i].wScale, destVarId[i].wScaleInverted);

    if (ldata.node[i].src && w > 1e-4f)
    {
      if (slotId < 0)
        att_tree->partialCalcWtm(ldata.node[i].src);
      mat44f &m = att_tree->getNodeWtmRel(ldata.node[i].src);
      vec3f p = m.col3;
      if (slotId >= 0)
        p = v_sub(p, ctx.worldTranslate);
      bool should_blend = (ldata.node[i].dest && w < 0.9999f);

      // setup effector
      if (should_blend)
      {
        mat44f &dm = tree.getNodeWtmRel(ldata.node[i].dest);
        tree.partialCalcWtm(ldata.node[i].dest);
        p = v_lerp_vec4f(v_splats(w), dm.col3, p);
      }

      st.paramEffector(destVarId[i].eff).set(v_extract_x(p), v_extract_y(p), v_extract_z(p), false);
      if (destVarId[i].wtm >= 0)
      {
        mat33f m3;
        m3.col0 = m.col0;
        m3.col1 = m.col1;
        m3.col2 = m.col2;
        if (should_blend)
        {
          mat44f &dm = tree.getNodeWtmRel(ldata.node[i].dest);
          mat33f om;
          om.col0 = dm.col0;
          om.col1 = dm.col1;
          om.col2 = dm.col2;
          BLEND_FINAL_WTM(m3, om, w, effFromAttach);
        }

        AnimV20::AttachGeomNodeCtrl::AttachDesc &ad = AnimV20::AttachGeomNodeCtrl::getAttachDesc(st, destVarId[i].wtm);
        v_stu(&ad.wtm.m[0][0], m3.col0);
        v_stu(&ad.wtm.m[1][0], m3.col1);
        v_stu(&ad.wtm.m[2][0], m3.col2);
        v_stu(&ad.wtm.m[3][0], p);
        ad.w = should_blend ? 1 : w;
      }
    }
    else if (!ignoreZeroWt)
    {
      // detach effector
      st.paramEffector(destVarId[i].eff).setLooseEnd();
      if (destVarId[i].wtm >= 0)
        AnimV20::AttachGeomNodeCtrl::getAttachDesc(st, destVarId[i].wtm).w = 0;
    }
  }
  ldata.lastRefUid = att_uid;
}
void AnimPostBlendEffFromAttachement::createNode(AnimationGraph &graph, const DataBlock &blk)
{
  const char *name = blk.getStr("name", NULL);
  G_ASSERTF_RETURN(name, , "no name for %s", __FUNCTION__);

  AnimPostBlendEffFromAttachement *node = new AnimPostBlendEffFromAttachement(graph);

  if (!blk.getBool("localNode", false))
  {
    if (const char *val = blk.getStr("slot", NULL))
      node->namedSlotId = AnimCharV20::addSlotId(val);
    else if (const char *val = blk.getStr("varSlot", NULL))
      node->varSlotId = graph.addParamId(val, IPureAnimStateHolder::PT_ScalarParam);
  }
  node->ignoreZeroWt = blk.getBool("ignoreZeroWt", false);
  for (int j = 0, nid = blk.getNameId("node"); j < blk.blockCount(); j++)
    if (blk.getBlock(j)->getBlockNameId() == nid)
    {
      const DataBlock &b2 = *blk.getBlock(j);
      NodeDesc &nd = node->nodes.push_back();
      nd.src = b2.getStr("node");
      nd.dest = b2.getStr("destNode", NULL);

      VarId &dv = node->destVarId.push_back();
      dv.eff = graph.addEffectorParamId(b2.getStr("effector"));
      dv.effWt = graph.addPbcWtParamId(b2.getStr("effector"));
      if (b2.getBool("writeMatrix", true))
      {
        dv.wtm = graph.getParamId(String(0, "%s.m", b2.getStr("effector")), AnimV20::IAnimStateHolder::PT_InlinePtr);
        dv.wtmWt = graph.addPbcWtParamId(String(0, "%s.m", b2.getStr("effector")));
      }
      else
        dv.wtm = dv.wtmWt = -1;
      dv.wScale = graph.addParamIdEx(b2.getStr("wtModulate", blk.getStr("wtModulate", NULL)), IPureAnimStateHolder::PT_ScalarParam);
      dv.wScaleInverted = b2.getBool("wtModulateInverse", blk.getBool("wtModulateInverse", false));
    }

  // allocate state variable
  node->varId = graph.addInlinePtrParamId(String("var") + name, sizeof(LocalData) + sizeof(int) * ((int)node->nodes.size() - 2),
    IPureAnimStateHolder::PT_InlinePtr);
  graph.registerBlendNode(node, name);
}


AnimPostBlendNodesFromAttachement::AnimPostBlendNodesFromAttachement(AnimationGraph &g) :
  AnimPostBlendCtrl(g), nodes(midmem), varId(-1), namedSlotId(-1), varSlotId(-1), copyWtm(false)
{}
void AnimPostBlendNodesFromAttachement::reset(IPureAnimStateHolder &st)
{
  LocalData &ldata = *(LocalData *)st.getInlinePtr(varId);
  clear_and_shrink(ldata.nodePairs);
  ldata.lastRefUid = INVALID_ATTACHMENT_UID;
}
static void add_subnodes_recursively(StaticTab<dag::Index16, 64> &pairs, GeomNodeTree &src_tree, GeomNodeTree &dest_tree,
  dag::Index16 dest_n)
{
  for (unsigned i = 0, ie = dest_tree.getChildCount(dest_n); i < ie; i++)
  {
    auto cnode = dest_tree.getChildNodeIdx(dest_n, i);
    if (auto src_n = src_tree.findINodeIndex(dest_tree.getNodeName(cnode)))
    {
      pairs.push_back(src_n);
      pairs.push_back(cnode);
      add_subnodes_recursively(pairs, src_tree, dest_tree, cnode);
    }
  }
}
void AnimPostBlendNodesFromAttachement::process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx)
{
  if (wt <= 0.001f || !nodes.size())
    return;

  LocalData &ldata = *(LocalData *)st.getInlinePtr(varId);
  const int slotId = varSlotId >= 0 ? (int)st.getParam(varSlotId) : namedSlotId;
  GeomNodeTree *att_tree = ctx.ac->getAttachedSkeleton(slotId);
  unsigned att_uid = ctx.ac->getAttachmentUid(slotId);

  if (ldata.lastRefUid != att_uid)
  {
    if (!att_tree)
      clear_and_shrink(ldata.nodePairs);
    else
    {
      StaticTab<dag::Index16, 64> pairs;
      for (int i = 0; i < nodes.size(); i++)
      {
        auto src_nidx = att_tree->findINodeIndex(nodes[i].srcNode);
        auto dest_nidx = tree.findINodeIndex(nodes[i].destNode);
        if (!src_nidx || !dest_nidx)
          continue;
        if (nodes[i].includingRoot)
        {
          pairs.push_back(src_nidx);
          pairs.push_back(dest_nidx);
        }
        if (nodes[i].recursive)
          add_subnodes_recursively(pairs, *att_tree, tree, dest_nidx);
      }
      ldata.nodePairs = pairs;
    }
    ldata.lastRefUid = att_uid;
  }

  if (!att_tree)
    return;
  for (int i = 0; i < ldata.nodePairs.size(); i += 2)
  {
    float w = wt * getWeightMul(st, wScaleVarId, wScaleInverted);
    mat44f m4 = att_tree->getNodeTm(ldata.nodePairs[i + 0]);
    m4.col3 = tree.getNodeTm(ldata.nodePairs[i + 1]).col3;

    if (w > 0.999f)
      tree.getNodeTm(ldata.nodePairs[i + 1]) = m4;
    else
    {
      blend_mat44f(m4, tree.getNodeTm(ldata.nodePairs[i + 1]), w);
      tree.getNodeTm(ldata.nodePairs[i + 1]) = m4;
    }
    tree.invalidateWtm(ldata.nodePairs[i + 1]);
  }
}
void AnimPostBlendNodesFromAttachement::createNode(AnimationGraph &graph, const DataBlock &blk)
{
  const char *name = blk.getStr("name", NULL);
  G_ASSERTF_RETURN(name, , "no name for %s", __FUNCTION__);

  AnimPostBlendNodesFromAttachement *node = new AnimPostBlendNodesFromAttachement(graph);

  if (const char *val = blk.getStr("slot", NULL))
    node->namedSlotId = AnimCharV20::addSlotId(val);
  else if (const char *val = blk.getStr("varSlot", NULL))
    node->varSlotId = graph.addParamId(val, IPureAnimStateHolder::PT_ScalarParam);
  node->copyWtm = blk.getBool("copyWtm", false);
  G_ASSERTF(!node->copyWtm, "copyWtm:b=yes not implemented, PBC %s", name);
  for (int j = 0, nid = blk.getNameId("node"); j < blk.blockCount(); j++)
    if (blk.getBlock(j)->getBlockNameId() == nid)
    {
      const DataBlock &b2 = *blk.getBlock(j);
      NodeDesc &nd = node->nodes.push_back();
      nd.srcNode = b2.getStr("srcNode");
      nd.destNode = b2.getStr("destNode");
      nd.recursive = b2.getBool("recursive", false);
      nd.includingRoot = b2.getBool("includingRoot", true);
    }
  node->wScaleVarId = graph.addParamIdEx(blk.getStr("wtModulate", NULL), IPureAnimStateHolder::PT_ScalarParam);
  node->wScaleInverted = blk.getBool("wtModulateInverse", false);

  // allocate state variable
  node->varId = graph.addInlinePtrParamId(String("var") + name, sizeof(LocalData), IPureAnimStateHolder::PT_InlinePtr);
  graph.registerBlendNode(node, name);
}


void AnimPostBlendParamFromNode::process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx)
{
  float &blend_wt = ctx.blendWt[destVarWtId];
  LocalData &ldata = *(LocalData *)st.getInlinePtr(varId);
  GeomNodeTree *att_tree = slotId < 0 ? &tree : ctx.ac->getAttachedSkeleton(slotId);
  unsigned att_uid = slotId < 0 ? 0xFFFFFFFF : ctx.ac->getAttachmentUid(slotId);

  if (ldata.lastRefUid != att_uid)
  {
    ldata.nodeId = att_tree ? att_tree->findINodeIndex(nodeName) : dag::Index16();
    ldata.lastRefUid = att_uid;
  }

  float val = defVal;
  if (att_tree && ldata.nodeId)
  {
    val = att_tree->getNodeLposScalar(ldata.nodeId).length();
    if (invertVal)
      val = 1.0f - val;
  }
  if (blend_wt > 0)
    val = (st.getParam(destVarId) * blend_wt + val * wt) / (blend_wt + wt);
  if (wt > 0)
    st.setParam(destVarId, val);
  blend_wt += wt;
}
void AnimPostBlendParamFromNode::createNode(AnimationGraph &graph, const DataBlock &blk)
{
  const char *name = blk.getStr("name", NULL);
  G_ASSERTF_RETURN(name, , "no name for %s", __FUNCTION__);

  AnimPostBlendParamFromNode *node = new AnimPostBlendParamFromNode(graph);

  if (const char *slot = blk.getStr("slot", NULL))
    node->slotId = AnimCharV20::addSlotId(slot);
  node->nodeName = blk.getStr("node");
  node->destVarId = graph.addParamId(blk.getStr("destVar"), IPureAnimStateHolder::PT_ScalarParam);
  node->destVarWtId = graph.addPbcWtParamId(blk.getStr("destVar"));
  node->defVal = blk.getReal("defVal", 0);
  node->invertVal = blk.getBool("invertVal", false);
  node->varId = graph.addInlinePtrParamId(String("var") + name, sizeof(LocalData), IPureAnimStateHolder::PT_InlinePtr);
  graph.registerBlendNode(node, name);
}

void AnimPostBlendNodeEffectorFromChildIK::createNode(AnimationGraph &graph, const DataBlock &blk)
{
  const char *name = blk.getStr("name", NULL);
  G_ASSERTF_RETURN(name, , "no name for %s", __FUNCTION__);

  AnimPostBlendNodeEffectorFromChildIK *node = new AnimPostBlendNodeEffectorFromChildIK(graph);

  node->parentNodeName = blk.getStr("parentNode", nullptr);
  node->childNodeName = blk.getStr("childNode", nullptr);
  node->srcVarName = blk.getStr("srcVar", nullptr);
  node->destVarName = blk.getStr("destVar", nullptr);
  node->localDataVarId = graph.addInlinePtrParamId(String("var") + name, sizeof(LocalData), IPureAnimStateHolder::PT_InlinePtr);
  node->resetEffByValId = graph.addParamIdEx(blk.getStr("resetEffByVal", nullptr), IPureAnimStateHolder::PT_ScalarParam);
  node->resetEffInvVal = blk.getBool("resetEffInvVal", false);
  G_ASSERTF(node->resetEffByValId >= 0, "resetEffByVal is absent");

  if (const char *val = blk.getStr("varSlot", nullptr))
    node->varSlotId = graph.addParamId(val, IPureAnimStateHolder::PT_ScalarParam);

  graph.registerBlendNode(node, name);
}

void AnimPostBlendNodeEffectorFromChildIK::init(IPureAnimStateHolder &st, const GeomNodeTree &tree)
{
  LocalData &ldata = *(LocalData *)st.getInlinePtr(localDataVarId);
  ldata.parentNodeId = tree.findINodeIndex(parentNodeName);
  ldata.childNodeId = tree.findINodeIndex(childNodeName);
  ldata.srcVarId = graph.getParamId(String(0, "%s.m", srcVarName), AnimV20::IAnimStateHolder::PT_InlinePtr);
  ldata.destEffId = graph.getParamId(destVarName, IPureAnimStateHolder::PT_Effector);
  ldata.destVarId = graph.getParamId(String(0, "%s.m", destVarName), IPureAnimStateHolder::PT_InlinePtr);

  G_ASSERTF(ldata.parentNodeId, "parentNode %s not found", parentNodeName);
  G_ASSERTF(ldata.childNodeId || varSlotId >= 0, "childNode %s not found", childNodeName);
  G_ASSERTF(ldata.srcVarId >= 0, "srcVar %s.m not found", srcVarName);
  G_ASSERTF(ldata.destEffId >= 0, "destVar %s not found", destVarName);
  G_ASSERTF(ldata.destVarId >= 0, "destVar %s.m not found", destVarName);
}

void AnimPostBlendNodeEffectorFromChildIK::process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree,
  AnimPostBlendCtrl::Context &ctx)
{
  LocalData &ldata = *(LocalData *)st.getInlinePtr(localDataVarId);

  bool exit = (wt < 1e-4) || (ldata.srcVarId < 0) || (ldata.destVarId < 0) || !ldata.parentNodeId || !ldata.childNodeId;
  if (exit)
    return;

  float scaleVal = st.getParam(resetEffByValId);
  int inactiveEff = static_cast<int>(clamp(resetEffInvVal ? (1.0f - scaleVal) : scaleVal, 0.0f, 1.0f));
  if (inactiveEff == 1)
  {
    st.paramEffector(ldata.destEffId).setLooseEnd();
    return;
  }

  const mat44f &parent_wtm = tree.getNodeWtmRel(ldata.parentNodeId);
  dag::Index16 childWtmNodeIdx = ldata.childNodeId;
  if (varSlotId >= 0)
  {
    const dag::Index16 idx = ctx.ac->getSlotNodeIdx(st.getParam(varSlotId));
    if (idx.valid())
      childWtmNodeIdx = idx;
    else if (!childWtmNodeIdx.valid())
      return;
  }
  const mat44f &child_wtm = tree.getNodeWtmRel(childWtmNodeIdx);
  AttachGeomNodeCtrl::AttachDesc &srcAd = AttachGeomNodeCtrl::getAttachDesc(st, ldata.srcVarId);
  AttachGeomNodeCtrl::AttachDesc &destAd = AttachGeomNodeCtrl::getAttachDesc(st, ldata.destVarId);
  if (srcAd.w < 1e-3)
  {
    st.paramEffector(ldata.destEffId).setLooseEnd();
    return;
  }

  tree.calcWtm();
  mat44f srcAdWtm;
  v_mat44_make_from_43cu_unsafe(srcAdWtm, &srcAd.wtm.m[0][0]);

  mat44f invTm;
  v_mat44_inverse43(invTm, child_wtm);
  mat44f parLoc;
  v_mat44_mul43(parLoc, invTm, parent_wtm);
  mat44f newWtm;
  v_mat44_mul43(newWtm, srcAdWtm, parLoc);

  v_mat_43cu_from_mat44(&destAd.wtm.m[0][0], newWtm);

  st.paramEffector(ldata.destEffId).set(v_extract_x(newWtm.col3), v_extract_y(newWtm.col3), v_extract_z(newWtm.col3), false);
}

void AnimPostBlendMatVarFromNode::init(IPureAnimStateHolder &st, const GeomNodeTree &tree)
{
  LocalData &ldata = *(LocalData *)st.getInlinePtr(varId);
  ldata.lastRefDestUid = ldata.lastRefSrcUid = INVALID_ATTACHMENT_UID;
  ldata.destVarId = destSlotId < 0 ? graph.getParamId(destVarName, IPureAnimStateHolder::PT_InlinePtr) : -1;
  ldata.srcNodeId = srcSlotId < 0 ? tree.findINodeIndex(srcNodeName) : dag::Index16();
  if (destSlotId < 0 && ldata.destVarId < 0)
    logerr("anim.matFromNode: var \"%s\" not resolved, slot %d", destVarName, destSlotId);
  if (srcSlotId < 0 && !ldata.srcNodeId)
    logerr("anim.matFromNode: node \"%s\" not resolved, slot %d", srcNodeName, srcSlotId);
}
void AnimPostBlendMatVarFromNode::process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &ctx)
{
  LocalData &ldata = *(LocalData *)st.getInlinePtr(varId);
  auto d_ac = destSlotId < 0 ? ctx.ac : ctx.ac->getAttachedChar(destSlotId);
  GeomNodeTree *s_tree = srcSlotId < 0 ? &tree : ctx.ac->getAttachedSkeleton(srcSlotId);
  unsigned dst_att_uid = destSlotId < 0 ? 0xFFFFFFFF : ctx.ac->getAttachmentUid(destSlotId);
  unsigned src_att_uid = srcSlotId < 0 ? 0xFFFFFFFF : ctx.ac->getAttachmentUid(srcSlotId);

  if (destSlotId >= 0 && dst_att_uid != ldata.lastRefDestUid)
  {
    ldata.destVarId = -1;
    if (d_ac && d_ac->getAnimGraph())
      ldata.destVarId = d_ac->getAnimGraph()->getParamId(destVarName, IPureAnimStateHolder::PT_InlinePtr);
    ldata.lastRefDestUid = dst_att_uid;
    if (ldata.destVarId < 0 && d_ac && d_ac->getAnimGraph())
      logerr("%s: anim.matFromNode: var \"%s\" not resolved, slot %d (in %s)", ctx.ac->getCreateInfo()->resName, destVarName,
        destSlotId, d_ac->getCreateInfo()->resName);
  }
  if (srcSlotId >= 0 && src_att_uid != ldata.lastRefSrcUid)
  {
    ldata.srcNodeId = s_tree ? s_tree->findINodeIndex(srcNodeName) : dag::Index16();
    ldata.lastRefSrcUid = src_att_uid;
    if (!ldata.srcNodeId && s_tree)
      logerr("%s: anim.matFromNode: node \"%s\" not resolved, slot %d (in %s), tree=%p", ctx.ac->getCreateInfo()->resName, srcNodeName,
        srcSlotId, s_tree, d_ac ? d_ac->getCreateInfo()->resName.str() : "?");
  }

  if (ldata.destVarId < 0)
    return;

  AttachGeomNodeCtrl::AttachDesc &ad = AttachGeomNodeCtrl::getAttachDesc(*d_ac->getAnimState(), ldata.destVarId);
  float blend_wt0 = 0, &blend_wt = destVarWtId >= 0 ? ctx.blendWt[destVarWtId] : blend_wt0;
  if (!s_tree || !ldata.srcNodeId || wt < 1e-4)
  {
    if (blend_wt <= 0)
      ad.w = 0.0f;
    return;
  }

  mat44f &m = s_tree->getNodeWtmRel(ldata.srcNodeId);
  vec3f pos = m.col3;
  if (srcSlotId >= 0)
    pos = v_sub(pos, ctx.worldTranslate);
  if (destSlotId >= 0)
    pos = v_sub(pos, v_sub(d_ac->getNodeTree().getNodeWposRel(dag::Index16(0)), ctx.worldTranslate));
  if (blend_wt > 0)
  {
    float w = wt / (blend_wt + wt);

    mat33f m3;
    m3.col0 = m.col0;
    m3.col1 = m.col1;
    m3.col2 = m.col2;

    mat33f om3;
    om3.col0 = v_ldu(&ad.wtm.m[0][0]);
    om3.col1 = v_ldu(&ad.wtm.m[1][0]);
    om3.col2 = v_ldu(&ad.wtm.m[2][0]);
    vec3f op = v_ldu(&ad.wtm.m[3][0]);
    BLEND_FINAL_WTM(m3, om3, w, matFromNode);

    v_stu(&ad.wtm.m[0][0], m3.col0);
    v_stu(&ad.wtm.m[1][0], m3.col1);
    v_stu(&ad.wtm.m[2][0], m3.col2);
    v_stu(&ad.wtm.m[3][0], v_lerp_vec4f(v_splats(w), op, pos));
  }
  else
  {
    v_stu(&ad.wtm.m[0][0], m.col0);
    v_stu(&ad.wtm.m[1][0], m.col1);
    v_stu(&ad.wtm.m[2][0], m.col2);
    v_stu(&ad.wtm.m[3][0], pos);
  }
  blend_wt += wt;
  ad.w = 1;
}
void AnimPostBlendMatVarFromNode::createNode(AnimationGraph &graph, const DataBlock &blk)
{
  const char *name = blk.getStr("name", NULL);
  G_ASSERTF_RETURN(name, , "no name for %s", __FUNCTION__);

  AnimPostBlendMatVarFromNode *node = new AnimPostBlendMatVarFromNode(graph);

  if (const char *slot = blk.getStr("destSlot", NULL))
    node->destSlotId = AnimCharV20::addSlotId(slot);
  if (const char *slot = blk.getStr("srcSlot", NULL))
    node->srcSlotId = AnimCharV20::addSlotId(slot);
  node->destVarName = blk.getStr("destVar");
  node->srcNodeName = blk.getStr("srcNode");

  if (node->destSlotId < 0)
  {
    graph.addParamId(node->destVarName, IPureAnimStateHolder::PT_ScalarParam);
    node->destVarWtId = graph.addPbcWtParamId(node->destVarName);
  }

  node->varId = graph.addInlinePtrParamId(String("var") + name, sizeof(LocalData), IPureAnimStateHolder::PT_InlinePtr);
  graph.registerBlendNode(node, name);
}

AnimPostBlendCompoundRotateShift::AnimPostBlendCompoundRotateShift(AnimationGraph &g) : AnimPostBlendCtrl(g)
{
  v_mat33_ident(tmRot[0]);
  v_mat33_ident(tmRot[1]);
}
void AnimPostBlendCompoundRotateShift::init(IPureAnimStateHolder &st, const GeomNodeTree &tree)
{
  LocalData &ldata = *(LocalData *)st.getInlinePtr(localVarId);
  ldata.targetNode = resolve_node_by_name(tree, targetNode);
  ldata.alignAsNode = resolve_node_by_name(tree, alignAsNode);
  ldata.moveAlongNode = resolve_node_by_name(tree, moveAlongNode);
}
void AnimPostBlendCompoundRotateShift::process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &)
{
  if (wt < 1e-4)
    return;
  LocalData &ldata = *(LocalData *)st.getInlinePtr(localVarId);
  if (!ldata.targetNode)
    return;

  mat33f m, m2;
  mat44f &dn_wtm = tree.getNodeWtmRel(ldata.targetNode);

  tree.partialCalcWtm(ldata.targetNode);
  mat44f old_wtm = dn_wtm;

  if (ldata.alignAsNode)
  {
    mat44f &sn_wtm = tree.getNodeWtmRel(ldata.alignAsNode);

    tree.partialCalcWtm(ldata.alignAsNode);
    m2.col0 = sn_wtm.col0;
    m2.col1 = sn_wtm.col1;
    m2.col2 = sn_wtm.col2;
    v_mat33_mul(m, m2, tmRot[0]);
  }
  else
  {
    m.col0 = old_wtm.col0;
    m.col1 = old_wtm.col1;
    m.col2 = old_wtm.col2;
  }

  if (varId.yaw >= 0 || varId.pitch >= 0)
  {
    mat33f m3;
    if (varId.pitch >= 0 && varId.yaw >= 0)
    {
      mat33f rotz, roty;
      v_mat33_make_rot_cw_z(rotz, v_splats(scale.pitch * st.getParam(varId.pitch) * DEG_TO_RAD));
      v_mat33_make_rot_cw_y(roty, v_splats(scale.yaw * st.getParam(varId.yaw) * DEG_TO_RAD));
      v_mat33_mul(m3, roty, rotz);
    }
    else if (varId.pitch >= 0)
      v_mat33_make_rot_cw_z(m3, v_splats(scale.pitch * st.getParam(varId.pitch) * DEG_TO_RAD));
    else
      v_mat33_make_rot_cw_y(m3, v_splats(scale.yaw * st.getParam(varId.yaw) * DEG_TO_RAD));

    v_mat33_mul(m2, m, m3);
  }
  else
    m2 = m;

  if (varId.lean >= 0)
  {
    mat33f m3;
    v_mat33_make_rot_cw_x(m3, v_splats(scale.lean * st.getParam(varId.lean) * DEG_TO_RAD));
    v_mat33_mul(m, m2, m3);
    m2 = m;
  }

  v_mat33_mul(m, m2, tmRot[1]);

  vec3f pos = dn_wtm.col3;

  if (ldata.moveAlongNode)
  {
    mat44f &moveAlong_wtm = tree.getNodeWtmRel(ldata.moveAlongNode);
    tree.partialCalcWtm(ldata.alignAsNode);

    m2.col0 = moveAlong_wtm.col0;
    m2.col1 = moveAlong_wtm.col1;
    m2.col2 = moveAlong_wtm.col2;
  }

  mat33f &moveAlong = ldata.moveAlongNode ? m2 : m;

  if (varId.ofsX >= 0)
    pos = v_madd(moveAlong.col0, v_splats(scale.ofsX * st.getParam(varId.ofsX)), pos);
  if (varId.ofsY >= 0)
    pos = v_madd(moveAlong.col1, v_splats(scale.ofsY * st.getParam(varId.ofsY)), pos);
  if (varId.ofsZ >= 0)
    pos = v_madd(moveAlong.col2, v_splats(scale.ofsZ * st.getParam(varId.ofsZ)), pos);

  dn_wtm.col0 = m.col0;
  dn_wtm.col1 = m.col1;
  dn_wtm.col2 = m.col2;
  dn_wtm.col3 = pos;

  blend_mat44f(dn_wtm, old_wtm, wt);
  tree.invalidateWtm(ldata.targetNode);

  mat44f iwtm;
  v_mat44_inverse43(iwtm, tree.getNodeWtmRel(tree.getParentNodeIdx(ldata.targetNode)));
  v_mat44_mul43(tree.getNodeTm(ldata.targetNode), iwtm, dn_wtm);
}
void AnimPostBlendCompoundRotateShift::createNode(AnimationGraph &graph, const DataBlock &blk)
{
  const char *name = blk.getStr("name", NULL);
  G_ASSERTF_RETURN(name, , "no name for %s", __FUNCTION__);

  AnimPostBlendCompoundRotateShift *node = new AnimPostBlendCompoundRotateShift(graph);

  node->targetNode = blk.getStr("targetNode", NULL);
  node->alignAsNode = blk.getStr("alignAsNode", NULL);
  node->moveAlongNode = blk.getStr("moveAlongNode", NULL);
#define SETUP_PARAM(NM)                          \
  if (const char *pname = blk.getStr(#NM, NULL)) \
  node->varId.NM = graph.addParamId(pname, IPureAnimStateHolder::PT_ScalarParam), node->scale.NM = blk.getReal(#NM "_mul", 1.0f)
  SETUP_PARAM(yaw);
  SETUP_PARAM(pitch);
  SETUP_PARAM(lean);
  SETUP_PARAM(ofsX);
  SETUP_PARAM(ofsY);
  SETUP_PARAM(ofsZ);
#undef SETUP_PARAM

  Point3 euler = blk.getPoint3("preRotEuler", Point3(0, 0, 0)) * PI / 180;
  Point3 scale = blk.getPoint3("preScale", Point3(1, 1, 1));
  v_mat33_make_rot_cw_zyx(node->tmRot[0], v_neg(v_ldu(&euler.x)));
  node->tmRot[0].col0 = v_mul(node->tmRot[0].col0, v_splats(scale.x));
  node->tmRot[0].col1 = v_mul(node->tmRot[0].col1, v_splats(scale.y));
  node->tmRot[0].col2 = v_mul(node->tmRot[0].col2, v_splats(scale.z));

  euler = blk.getPoint3("postRotEuler", Point3(0, 0, 0)) * PI / 180;
  scale = blk.getPoint3("postScale", Point3(1, 1, 1));
  v_mat33_make_rot_cw_zyx(node->tmRot[1], v_neg(v_ldu(&euler.x)));
  node->tmRot[1].col0 = v_mul(node->tmRot[1].col0, v_splats(scale.x));
  node->tmRot[1].col1 = v_mul(node->tmRot[1].col1, v_splats(scale.y));
  node->tmRot[1].col2 = v_mul(node->tmRot[1].col2, v_splats(scale.z));

  node->localVarId = graph.addInlinePtrParamId(String("var") + name, sizeof(LocalData), IPureAnimStateHolder::PT_InlinePtr);
  graph.registerBlendNode(node, name);
}

void AnimPostBlendSetParam::process(IPureAnimStateHolder &st, real wt, GeomNodeTree &, AnimPostBlendCtrl::Context &ctx)
{
  if (wt < minWeight)
    return;
  if (destSlotId < 0)
    st.setParam(destVarId, srcVarId < 0 ? val : st.getParam(srcVarId));
  else if (auto ac = ctx.ac->getAttachedChar(destSlotId))
  {
    if (!ac->getAnimGraph() || !ac->getAnimState())
      return;
    int id = ac->getAnimGraph()->getParamId(destVarName);
    if (id >= 0)
      ac->getAnimState()->setParam(id, srcVarId < 0 ? val : st.getParam(srcVarId));
  }
}
void AnimPostBlendSetParam::createNode(AnimationGraph &graph, const DataBlock &blk)
{
  const char *name = blk.getStr("name", NULL);
  G_ASSERTF_RETURN(name, , "no name for %s", __FUNCTION__);

  AnimPostBlendSetParam *node = new AnimPostBlendSetParam(graph);

  node->minWeight = blk.getReal("minWeight", node->minWeight);
  if (const char *slot = blk.getStr("destSlot", NULL))
    node->destSlotId = AnimCharV20::addSlotId(slot);
  node->destVarName = blk.getStr("destVar");
  if (const char *var = blk.getStr("srcVar", NULL))
  {
    node->srcVarId = graph.getParamId(var);
    if (node->srcVarId < 0)
      node->srcVarId = graph.addParamId(var, IPureAnimStateHolder::PT_ScalarParam);
    else if (graph.getParamType(node->srcVarId) != AnimV20::IPureAnimStateHolder::PT_ScalarParam &&
             graph.getParamType(node->srcVarId) != AnimV20::IPureAnimStateHolder::PT_TimeParam)
    {
      node->srcVarId = -1;
      ANIM_ERR("existing param <%s> must be scalar ot time param", var);
    }
  }
  else if (const char *val = blk.getStr("namedValue", NULL))
    node->val = AnimV20::getEnumValueByName(val);
  else if (const char *val = blk.getStr("slotIdValue", NULL))
    node->val = AnimCharV20::addSlotId(val);
  else
    node->val = blk.getReal("value", 0);

  if (node->destSlotId < 0)
    node->destVarId = graph.addParamId(node->destVarName, IPureAnimStateHolder::PT_ScalarParam);

  graph.registerBlendNode(node, name);
}


void AnimPostBlendTwistCtrl::init(IPureAnimStateHolder &st, const GeomNodeTree &tree)
{
  auto *nodeIds = (dag::Index16 *)st.getInlinePtr(varId);
  if (!twistNodes.size())
  {
    nodeIds[0] = dag::Index16();
    return;
  }
  nodeIds[0] = tree.findINodeIndex(node0);
  nodeIds[1] = tree.findINodeIndex(node1);
  if (!nodeIds[0] || !nodeIds[1])
  {
    logerr("%s: failed to resolve (%s, %s) -> (%d, %d)", __FUNCTION__, node0, node1, (int)nodeIds[0], (int)nodeIds[1]);
    nodeIds[0] = dag::Index16(-2);
    return;
  }
  for (int i = 0; i < twistNodes.size(); i++)
  {
    nodeIds[i + 2] = tree.findINodeIndex(twistNodes[i]);
    if (!nodeIds[i + 2])
    {
      if (!optional)
        logerr("%s: failed to resolve twist[%d] %s", __FUNCTION__, i, twistNodes[i]);
      nodeIds[0] = dag::Index16();
    }
  }
}
void AnimPostBlendTwistCtrl::process(IPureAnimStateHolder &st, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &)
{
  if (wt <= 0)
    return;
  auto *nodeIds = (dag::Index16 *)st.getInlinePtr(varId);
  if (!nodeIds[0])
    return;
  apply_twist_ctrl(tree, nodeIds[0], nodeIds[1], make_span(&nodeIds[2], twistNodes.size()), angDiff);
}
void AnimPostBlendTwistCtrl::createNode(AnimationGraph &graph, const DataBlock &blk)
{
  const char *name = blk.getStr("name", NULL);
  G_ASSERTF_RETURN(name, , "no name for %s", __FUNCTION__);

  AnimPostBlendTwistCtrl *node = new AnimPostBlendTwistCtrl(graph);
  node->optional = blk.getBool("optional", false);
  node->node0 = blk.getStr("node0", NULL);
  node->node1 = blk.getStr("node1", NULL);
  node->angDiff = blk.getReal("angDiff", 0) * DEG_TO_RAD;
  for (int i = 0, nid = blk.getNameId("twistNode"); i < blk.paramCount(); i++)
    if (blk.getParamNameId(i) == nid && blk.getParamType(i) == blk.TYPE_STRING)
      node->twistNodes.push_back() = blk.getStr(i);
  G_ASSERTF(node->optional || node->twistNodes.size(), "twistNodes.count=%d", node->twistNodes.size());

  node->varId = graph.addInlinePtrParamId(String("var") + name, sizeof(int16_t) * (2 + node->twistNodes.size()),
    IPureAnimStateHolder::PT_InlinePtr);
  graph.registerBlendNode(node, name);
}

void AnimPostBlendEyeCtrl::init(IPureAnimStateHolder &, const GeomNodeTree &tree)
{
  eyeDirectionNodeId = tree.findINodeIndex(eyeDirectionNodeName);
  eyelidStartTopNodeId = tree.findINodeIndex(eyelidStartTopNodeName);
  eyelidStartBottomNodeId = tree.findINodeIndex(eyelidStartBottomNodeName);
  eyelidHorizontalReactionNodeId = tree.findINodeIndex(eyelidHorizontalReactionNodeName);
  eyelidVerticalTopReactionNodeId = tree.findINodeIndex(eyelidVerticalTopReactionNodeName);
  eyelidVerticalBottomReactionNodeId = tree.findINodeIndex(eyelidVerticalBottomReactionNodeName);
  eyelidBlinkSourceNodeId = tree.findINodeIndex(eyelidBlinkSourceNodeName);
  eyelidBlinkTopNodeId = tree.findINodeIndex(eyelidBlinkTopNodeName);
  eyelidBlinkBottomNodeId = tree.findINodeIndex(eyelidBlinkBottomNodeName);

  G_ASSERTF(eyeDirectionNodeId, "Node %s not found", eyeDirectionNodeName);
  G_ASSERTF(eyelidStartTopNodeId, "Node %s not found", eyelidStartTopNodeName);
  G_ASSERTF(eyelidStartBottomNodeId, "Node %s not found", eyelidStartBottomNodeName);
  G_ASSERTF(eyelidHorizontalReactionNodeId, "Node %s not found", eyelidHorizontalReactionNodeName);
  G_ASSERTF(eyelidVerticalTopReactionNodeId, "Node %s not found", eyelidVerticalTopReactionNodeName);
  G_ASSERTF(eyelidVerticalBottomReactionNodeId, "Node %s not found", eyelidVerticalBottomReactionNodeName);
  G_ASSERTF(eyelidBlinkSourceNodeId, "Node %s not found", eyelidBlinkSourceNodeName);
  G_ASSERTF(eyelidBlinkTopNodeId, "Node %s not found", eyelidBlinkTopNodeName);
  G_ASSERTF(eyelidBlinkBottomNodeId, "Node %s not found", eyelidBlinkBottomNodeName);
}

void AnimPostBlendEyeCtrl::process(IPureAnimStateHolder &, real wt, GeomNodeTree &tree, AnimPostBlendCtrl::Context &)
{
  if (wt <= 0.001f)
    return;

  tree.partialCalcWtm(eyeDirectionNodeId);
  mat44f &eyeDirWtm = tree.getNodeWtmRel(eyeDirectionNodeId);
  quat4f eyeQuaternion = v_quat_from_mat(eyeDirWtm.col0, eyeDirWtm.col2, eyeDirWtm.col1);
  vec3f eyeRotation = v_euler_from_quat(eyeQuaternion);

  // Eyelids reaction on horizontal motion of the eye
  mat33f m33;
  v_mat33_make_rot_cw_y(m33, v_mul(v_splat_x(eyeRotation), v_splats(horizontalReactionFactor)));

  tree.partialCalcWtm(eyelidHorizontalReactionNodeId);
  mat44f &horizontalWtm = tree.getNodeWtmRel(eyelidHorizontalReactionNodeId);
  horizontalWtm.col0 = m33.col0;
  horizontalWtm.col1 = m33.col2;
  horizontalWtm.col2 = m33.col1;
  tree.recalcTm(eyelidHorizontalReactionNodeId);
  tree.invalidateWtm(eyelidHorizontalReactionNodeId);

  // Top eyelid eaction on vertical motion of the eye
  tree.partialCalcWtm(eyelidStartTopNodeId);
  tree.partialCalcWtm(eyelidVerticalTopReactionNodeId);
  mat44f &startTopWtm = tree.getNodeWtmRel(eyelidStartTopNodeId);
  mat44f &verticalTopWtm = tree.getNodeWtmRel(eyelidVerticalTopReactionNodeId);
  verticalTopWtm.col3 =
    v_perm_xbzw(startTopWtm.col3, v_sub(startTopWtm.col3, v_mul(v_splat_z(eyeRotation), v_splats(-verticalReactionFactor.x))));
  tree.recalcTm(eyelidVerticalTopReactionNodeId);
  tree.invalidateWtm(eyelidVerticalTopReactionNodeId);

  // Bottom eyelid reaction on vertical motion of the eye
  tree.partialCalcWtm(eyelidStartBottomNodeId);
  tree.partialCalcWtm(eyelidVerticalBottomReactionNodeId);
  mat44f &startBottomWtm = tree.getNodeWtmRel(eyelidStartBottomNodeId);
  mat44f &verticalBottomWtm = tree.getNodeWtmRel(eyelidVerticalBottomReactionNodeId);
  verticalBottomWtm.col3 =
    v_perm_xbzw(startBottomWtm.col3, v_sub(startBottomWtm.col3, v_mul(v_splat_z(eyeRotation), v_splats(-verticalReactionFactor.y))));
  tree.recalcTm(eyelidVerticalBottomReactionNodeId);
  tree.invalidateWtm(eyelidVerticalBottomReactionNodeId);

  // Blinking
  float eyelidsDistance = v_extract_y(v_sub(verticalTopWtm.col3, verticalBottomWtm.col3));
  float eyelidsHalfDistance = eyelidsDistance * 0.5f;

  tree.partialCalcWtm(eyelidBlinkSourceNodeId);
  vec4f blinkSource = tree.getNodeWposRel(eyelidBlinkSourceNodeId);
  float blinkValue = v_extract_y(blinkSource);

  tree.partialCalcWtm(eyelidBlinkTopNodeId);
  mat44f &blinkTopWtm = tree.getNodeWtmRel(eyelidBlinkTopNodeId);

  tree.partialCalcWtm(eyelidBlinkBottomNodeId);
  mat44f &blinkBottomWtm = tree.getNodeWtmRel(eyelidBlinkBottomNodeId);

  if (blinkValue < eyelidsHalfDistance)
  {
    blinkTopWtm.col3 =
      v_perm_xbzw(verticalTopWtm.col3, v_sub(verticalTopWtm.col3, v_mul(blinkSource, v_splats(blinkingReactionFactor))));
    blinkBottomWtm.col3 =
      v_perm_xbzw(verticalBottomWtm.col3, v_add(verticalBottomWtm.col3, v_mul(blinkSource, v_splats(blinkingReactionFactor))));
  }
  else
  {
    blinkTopWtm.col3 =
      v_perm_xbzw(verticalTopWtm.col3, v_sub(verticalTopWtm.col3, v_splats(eyelidsHalfDistance * blinkingReactionFactor)));
    blinkBottomWtm.col3 =
      v_perm_xbzw(verticalBottomWtm.col3, v_add(verticalBottomWtm.col3, v_splats(eyelidsHalfDistance * blinkingReactionFactor)));
  }

  tree.recalcTm(eyelidBlinkTopNodeId);
  tree.invalidateWtm(eyelidBlinkTopNodeId);

  tree.recalcTm(eyelidBlinkBottomNodeId);
  tree.invalidateWtm(eyelidBlinkBottomNodeId);
}

void AnimPostBlendEyeCtrl::createNode(AnimationGraph &graph, const DataBlock &blk)
{
  const char *name = blk.getStr("name", NULL);
  G_ASSERTF_RETURN(name, , "no name for %s", __FUNCTION__);

  AnimPostBlendEyeCtrl *node = new AnimPostBlendEyeCtrl(graph);
  node->horizontalReactionFactor = blk.getReal("horizontal_reaction_factor", 0.f);
  node->blinkingReactionFactor = blk.getReal("blinking_reaction_factor", 0.f);
  node->verticalReactionFactor = blk.getPoint2("vertical_reaction_factor", Point2());

  node->eyeDirectionNodeName = blk.getStr("eye_direction", "");
  node->eyelidStartTopNodeName = blk.getStr("eyelid_start_top", "");
  node->eyelidStartBottomNodeName = blk.getStr("eyelid_start_bottom", "");
  node->eyelidHorizontalReactionNodeName = blk.getStr("eyelids_horizontal", "");
  node->eyelidVerticalTopReactionNodeName = blk.getStr("eyelid_vertical_top", "");
  node->eyelidVerticalBottomReactionNodeName = blk.getStr("eyelid_vertical_bottom", "");
  node->eyelidBlinkSourceNodeName = blk.getStr("blink_source", "");
  node->eyelidBlinkTopNodeName = blk.getStr("eyelid_blink_top", "");
  node->eyelidBlinkBottomNodeName = blk.getStr("eyelid_blink_bottom", "");

  graph.registerBlendNode(node, name);
}
