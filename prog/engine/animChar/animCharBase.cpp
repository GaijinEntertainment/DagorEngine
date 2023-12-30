#include <animChar/dag_animCharacter2.h>
#include <anim/dag_animChannels.h>
#include <anim/dag_animBlend.h>
#include <anim/dag_animStateHolder.h>
#include <anim/dag_animBlendCtrl.h>
#include <anim/dag_animIrq.h>
#include <anim/dag_animKeyInterp.h>
#include <math/dag_quatInterp.h>
#include <perfMon/dag_statDrv.h>
#include <generic/dag_tab.h>
#include <math/dag_geomTree.h>
#include <math/dag_geomTreeMap.h>
#include <phys/dag_fastPhys.h>
#include <phys/dag_physResource.h>
#include <gameRes/dag_stdGameRes.h>
#include <gameRes/dag_gameResSystem.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_roDataBlock.h>
#include "animBranches.h"
#include "animPdlMgr.h"

#include <debug/dag_log.h>
#include <debug/dag_debug.h>
#include <startup/dag_globalSettings.h>
// #include <debug/dag_debug3d.h>

using namespace AnimV20;

#if MEASURE_PERF
#include <perfMon/dag_perfMonStat.h>
#include <perfMon/dag_perfTimer2.h>
#include <startup/dag_globalSettings.h>

static PerformanceTimer2 perf_tm;
static PerformanceTimer2 perf_tm2, perf_tm3;
static int perf_cnt = 0;
static int perf_frameno_start = 0;
namespace perfanimgblend
{
extern PerformanceTimer2 perf_tm;
}
#endif

namespace AnimCharV20
{
bool (*trace_static_ray_down)(const Point3_vec4 &from, float max_t, float &out_t, intptr_t ctx) = NULL;
bool (*trace_static_ray_dir)(const Point3_vec4 &from, Point3_vec4 dir, float max_t, float &out_t, intptr_t ctx) = NULL;
} // namespace AnimCharV20

#define LOGLEVEL_DEBUG _MAKE4C('ANIM')
//
// Animchar base component
//
AnimcharBaseComponent::AnimcharBaseComponent() :
  animGraph(NULL),
  animMap(midmem),
  animState(NULL),
  stateDirector(NULL),
  originalNodeTree(NULL),
  stateDirectorEnabled(true),
  animValid(false),
  forceAnimUpdate(false),
  magic(MAGIC_VALUE),
  originLinVelStorage(NULL),
  originAngVelStorage(NULL),
  centerNodeBsphRad(0),
  centerNodeIdx(0),
  fastPhysSystem(NULL),
  physSys(NULL),
  totalDeltaTime(0),
  postCtrl(NULL),
  motionMatchingController(nullptr),
  animDbgCtx(NULL)
{
  node0.pOfs = v_zero();
  node0.pScale_id = V_C_ONE;
  node0.sScale = V_C_ONE;
  node0.setChanNodeId(-2);
  charDepNodeId = dag::Index16();
  charDepBasePYofs = 0;
}

AnimcharBaseComponent::~AnimcharBaseComponent()
{
  if (animGraph && animState)
    animGraph->resetBlendNodes(*animState);

  for (int i = 0; i < attachment.size(); i++)
    releaseAttachment(i);
  clear_and_shrink(attachment);
  clear_and_shrink(attachmentSlotId);

  destroyDebugBlenderContext();
  del_it(fastPhysSystem);
  setOriginalNodeTreeRes(NULL);
  postCtrl = NULL;
  motionMatchingController = nullptr;

  destroyGraph(stateDirector);

  if (animState)
  {
    delete animState;
    animState = NULL;
  }
  animGraph = NULL;
  centerNodeBsphRad = 0;
  centerNodeIdx = dag::Index16(0);
}

void AnimcharBaseComponent::setFastPhysSystem(FastPhysSystem *system)
{
  if (fastPhysSystem == system)
    return;

  totalDeltaTime = 0;
  fastPhysSystem = system;

  nodeTree.invalidateWtm();
  vec3f world_translate = nodeTree.translateToZero();
  nodeTree.calcWtm();
  nodeTree.translate(world_translate);

  if (fastPhysSystem)
    fastPhysSystem->init(nodeTree);

  nodeTree.invalidateWtm();
  recalcWtm();
}

void AnimcharBaseComponent::setFastPhysSystemGravityDirection(const Point3 &gravity_direction)
{
  if (fastPhysSystem)
    fastPhysSystem->gravityDirection = gravity_direction;
}

bool AnimcharBaseComponent::resetFastPhysSystem()
{
  if (!fastPhysSystem)
    return true;
  if (!originalNodeTree)
    return false;
  if (nodeTree.empty())
    return false;

  FastPhysSystem *fpsys = fastPhysSystem;
  mat44f tm = nodeTree.getRootTm();
  vec3f wofs = nodeTree.getWtmOfs();

  nodeTree = *originalNodeTree;
  nodeTree.getRootTm() = tm;
  nodeTree.setWtmOfs(wofs);
  nodeTree.invalidateWtm();

  fastPhysSystem = NULL;
  invalidateAnim();
  calcAnimWtm(true);

  totalDeltaTime = 0;
  fastPhysSystem = fpsys;
  fastPhysSystem->reset(nodeTree.getWtmOfs());

  return true;
}

void AnimcharBaseComponent::setPostController(IAnimCharPostController *ctrl)
{
  if (postCtrl == ctrl)
    return;

  postCtrl = ctrl;
  recalcWtm();
}

void AnimcharBaseComponent::setTm(const mat44f &tm, bool setup_wofs)
{
  nodeTree.setRootTm(tm, setup_wofs);
  nodeTree.invalidateWtm();
}
void AnimcharBaseComponent::setTm(const TMatrix &tm, bool setup_wofs)
{
  mat44f tm4;
  v_mat44_make_from_43cu(tm4, tm.array);
  setTm(tm4, setup_wofs);
}
void AnimcharBaseComponent::setTm(vec4f v_pos, vec3f v_dir, vec3f v_up)
{
  vec3f v_right = v_norm3(v_cross3(v_up, v_dir));
  v_dir = v_cross3(v_right, v_up);
  mat44f tm4;
  tm4.col0 = v_neg(v_right);
  tm4.col1 = v_up;
  tm4.col2 = v_neg(v_dir);
  tm4.col3 = v_pos;

  setTm(tm4);
}
void AnimcharBaseComponent::setTm(const Point3 &pos, const Point3 &dir, const Point3 &up)
{
  vec4f v3mask = v_cast_vec4f(V_CI_MASK1110);
  setTm(v_or(v_and(v_ldu(&pos.x), v3mask), V_C_UNIT_0001), v_and(v_ldu(&dir.x), v3mask), v_and(v_ldu(&up.x), v3mask));
}
void AnimcharBaseComponent::setPos(const Point3 &pos)
{
  nodeTree.changeRootPos(v_perm_xyzd(v_ldu(&pos.x), V_C_UNIT_0001));
  nodeTree.invalidateWtm();
}
void AnimcharBaseComponent::setDir(float ax, float ay, float az)
{
  if (nodeTree.empty())
    return;
  mat44f &root_tm = nodeTree.getRootTm();
  Matrix3 m = rotxM3(ax) * rotyM3(ay) * rotzM3(az);

  root_tm.col0 = v_and(v_ldu(m.m[0]), v_cast_vec4f(V_CI_MASK1110));
  root_tm.col1 = v_and(v_ldu(m.m[1]), v_cast_vec4f(V_CI_MASK1110));
  root_tm.col2 = v_and(v_ldu(m.m[2]), v_cast_vec4f(V_CI_MASK1110));
  nodeTree.invalidateWtm();
}

void AnimcharBaseComponent::getTm(mat44f &tm) const
{
  if (nodeTree.empty())
    return;
  auto &root_tm = nodeTree.getRootTm();
  tm.col0 = root_tm.col0;
  tm.col1 = root_tm.col1;
  tm.col2 = root_tm.col2;
  tm.col3 = v_add(root_tm.col3, nodeTree.getWtmOfs());
}

void AnimcharBaseComponent::getTm(TMatrix &tm) const
{
  if (nodeTree.empty())
    return;
  mat44f root_tm = nodeTree.getRootTm();
  root_tm.col3 = v_add(root_tm.col3, nodeTree.getWtmOfs());
  v_mat_43cu_from_mat44(tm.array, root_tm);
}

Point3 AnimcharBaseComponent::getPos(bool recalc_anim_if_needed)
{
  if (recalc_anim_if_needed)
    calcAnimWtm(true);

  if (!nodeTree.empty())
  {
    auto &root_tm = nodeTree.getRootTm();
    Point3 ret;
    v_stu_p3(&ret.x, v_add(root_tm.col3, nodeTree.getWtmOfs()));
    return ret;
  }
  return Point3(0, 0, 0);
}

void AnimcharBaseComponent::registerIrqHandler(int irq, IGenericIrq *handler)
{
  if (irq < 0)
    return;
  if (irq >= irqHandlers.size())
    irqHandlers.resize(irq + 1);
  irqHandlers[irq].push_back(handler);
}
void AnimcharBaseComponent::unregisterIrqHandler(int irq, IGenericIrq *handler)
{
  if (irq >= 0 && irq >= irqHandlers.size())
    return;
  for (IrqTab &itab : (irq < 0 ? make_span(irqHandlers) : make_span(&irqHandlers[irq], 1)))
    for (int i = itab.size() - 1; i >= 0; --i)
      if (itab[i] == handler)
        erase_items(itab, i, 1);
}

void AnimcharBaseComponent::reset()
{
  resetAnim();
  totalDeltaTime = 0;

  if (fastPhysSystem)
    fastPhysSystem->reset(nodeTree.getWtmOfs());
}

void AnimcharBaseComponent::resetFastPhysWtmOfs(const vec3f wofs)
{
  if (fastPhysSystem)
    fastPhysSystem->reset(wofs);
}

void AnimcharBaseComponent::recalcWtm()
{
  if (!nodeTree.isWtmValid(false))
    recalcWtm(nodeTree.translateToZero());
  else
    postRecalcWtm();
}

void AnimcharBaseComponent::recalcWtm(vec3f world_translate)
{
  nodeTree.calcWtm();
  nodeTree.translate(world_translate);
  postRecalcWtm();
}

void AnimcharBaseComponent::forcePostRecalcWtm(real dt)
{
  totalDeltaTime = dt;
  postRecalcWtm();
}

void AnimcharBaseComponent::postRecalcWtm()
{
  //== call external node post-processing

  if (postCtrl)
    postCtrl->update(totalDeltaTime, nodeTree, this);

  if (fastPhysSystem && totalDeltaTime > 0)
    fastPhysSystem->update(totalDeltaTime, nodeTree.getWtmOfs());

  if (recalcableAttachments || attachment.size() > sizeof(recalcableAttachments) * CHAR_BIT)
    recalcAttachments();

  totalDeltaTime = 0;
}

void AnimcharBaseComponent::updateFastPhys(const float dt)
{
  if (fastPhysSystem)
    fastPhysSystem->update(dt, nodeTree.getWtmOfs());
}

void AnimcharBaseComponent::act(real dt, bool calc_anim)
{
  // debug_ctx("act (dt=%.4f)", dt);
  totalDeltaTime += dt;

  if (!animState)
  {
    recalcWtm();
    return;
  }
#if MEASURE_PERF
  perf_tm.go();
#endif

  AnimcharIrqHandler ih{&irq, this};
  if (stateDirector)
    stateDirector->setIrqHandler(&ih);

  animState->advance(dt);
  if (stateDirector && stateDirectorEnabled)
    stateDirector->advance();
  animValid = false;

  if (originLinVelStorage || originAngVelStorage)
  {
    AnimBlender::TlsContext &tls = animGraph->selectBlenderCtx(&irq, this);
    animGraph->blendOriginVel(tls, *animState, true);
    if (originLinVelStorage && !nodeTree.empty())
    {
      vec3f v = tls.originLinVel;
      if (stateDirector && stateDirectorEnabled)
      {
        Point3_vec4 add_vel;
        stateDirector->getAddOriginVel(add_vel);
        v = v_add(v, v_ld(&add_vel.x));
      }
      v_stu_p3(&originLinVelStorage->x, v_mat44_mul_vec3v(nodeTree.getRootTm(), v));
    }
    if (originAngVelStorage)
    {
      // TMatrix &wtm = root->tm;
      // Point3 v = animGraph->getBlendedOriginAngVel();
      //*originAngVelStorage = wtm.getcol(0)*v[0] + wtm.getcol(1)*v[1] + wtm.getcol(2)*v[2];
      v_stu_p3(&originAngVelStorage->x, tls.originAngVel);
      originAngVelStorage->y = -originAngVelStorage->y;
    }
  }
  calcAnimWtm(forceAnimUpdate || calc_anim);
  if (stateDirector)
    stateDirector->setIrqHandler(nullptr);
#if MEASURE_PERF
  perf_tm.pause();
  perf_cnt++;
#if MEASURE_PERF_FRAMES
  if (::dagor_frame_no() > perf_frameno_start + MEASURE_PERF_FRAMES)
#else
  if (perf_cnt >= MEASURE_PERF)
#endif
  {
    perfmonstat::animchar_act_cnt = perf_cnt;
    perfmonstat::animchar_act_time_usec = perf_tm.getTotalUsec();
    perfmonstat::animchar_blendAnim_getprs_time_usec = perf_tm2.getTotalUsec();
    perfmonstat::animchar_blendAnim_pbc_time_usec = perf_tm3.getTotalUsec();
    perfmonstat::generation_counter++;

    perf_tm.reset();
    perf_tm2.reset();
    perf_tm3.reset();
    perf_cnt = 0;
    perf_frameno_start = ::dagor_frame_no();
  }
#endif
}

bool AnimcharBaseComponent::loadAnimGraphCpp(const char *res_name)
{
  if (!animState)
    setupAnim();

  stateDirector = makeGraphFromRes(res_name);
  if (!stateDirector)
  {
    DAG_FATAL("can't create state director from graph <%s>", res_name);
    RETURN_X_AFTER_FATAL(false);
  }

  if (!stateDirector->init(animGraph, animState))
  {
    destroyGraph(stateDirector);
    DAG_FATAL("can't init state director from graph <%s>", res_name);
    RETURN_X_AFTER_FATAL(false);
  }

  if (animState)
  {
    animState->reset();
    animGraph->resetBlendNodes(*animState);
  }
  resetAnim();
  return true;
}

void AnimcharBaseComponent::resetAnim()
{
  // reset/init AnimationGraph
  if (animState)
  {
    animState->reset();
    animGraph->resetBlendNodes(*animState);
    animGraph->postBlendInit(*animState, nodeTree);
    if (stateDirector)
      stateDirector->reset(true);
    // debug_ctx("animState.size=%d", animState->getSize());
  }
  if (originalNodeTree && nodeTree.nodeCount() > 1)
  {
    memcpy(&nodeTree.getRootTm() + 1, &originalNodeTree->getRootTm() + 1, sizeof(mat44f) * (nodeTree.nodeCount() - 1));
    memcpy(&nodeTree.getRootWtmRel() + 1, &originalNodeTree->getRootWtmRel() + 1, sizeof(mat44f) * (nodeTree.nodeCount() - 1));
  }
}

bool AnimcharBaseComponent::load(const AnimCharCreationProps &props, int skeletonId, int acId, int physId)
{
  GeomNodeTree *skeleton = (GeomNodeTree *)::get_game_resource(skeletonId);
  if (!skeleton)
    return false;
  AnimCharData *ac = (acId >= 0) ? (AnimCharData *)::get_game_resource(acId) : NULL;
  if (acId >= 0 && !ac)
  {
    ::release_game_resource(skeletonId);
    return false;
  }

  AnimationGraph *ag = ac ? ac->graph.get() : nullptr;
  loadData(props, skeleton, ag);

  if (ac && ac->agResName && ac->agResName[0])
    loadAnimGraphCpp(ac->agResName);

  if (physId >= 0)
  {
    DynamicPhysObjectData *poData = (DynamicPhysObjectData *)::get_game_resource(physId);
    setPhysicsResource(poData ? poData->physRes : NULL);
    ::release_game_resource(physId);
  }

  ::release_game_resource(skeletonId);
  if (acId >= 0)
    ::release_game_resource(acId);

  centerNodeIdx =
    (!props.centerNode || strcmp(props.centerNode, "@root") == 0) ? dag::Index16(0) : nodeTree.findNodeIndex(props.centerNode);
  if (!centerNodeIdx)
  {
    logwarn("center node \"%s\" not found, Root is used instead (animchar: %s)", props.centerNode, creationInfo.resName);
    centerNodeIdx = dag::Index16(0);
  }
  centerNodeBsphRad = props.bsphRad;
  return true;
}

void AnimcharBaseComponent::loadData(const AnimCharCreationProps &props, GeomNodeTree *skeleton, AnimationGraph *ag)
{
  setOriginalNodeTreeRes(skeleton);
  if (skeleton)
    nodeTree = *skeleton;

  if (props.useCharDep && ag)
  {
    float *p = (float *)&node0.pOfs;
    p[0] = p[2] = p[3] = 0;
    p[1] = props.pyOfs;

    p = (float *)&node0.pScale_id;
    p[0] = props.pxScale;
    p[1] = props.pyScale;
    p[2] = props.pzScale;
    p[3] = 0;

    p = (float *)&node0.sScale;
    p[0] = p[1] = p[2] = p[3] = props.sScale;

    node0.setChanNodeId(ag->getNodeId(props.rootNode));
    charDepNodeId = nodeTree.findNodeIndex(props.rootNode);
    charDepBasePYofs = props.pyOfs;
  }

  animGraph = ag;
  if (animGraph)
  {
    setupAnim();
    resetAnim();
  }

  if (const RoDataBlock *b = props.props.get())
  {
    int nid = b->getNameId("attachment");
    decltype(attachmentSlotId) slots;
    slots.reserve(48);

    for (int i = 0; i < b->blockCount(); i++)
      if (b->getBlock(i)->getBlockNameId() == nid)
      {
        const char *slot_nm = b->getBlock(i)->getStr("slot", NULL);
        int slot_id = AnimCharV20::addSlotId(slot_nm);
        if (slot_id < 0)
        {
          logerr("bad slot <%s> in block #%d (animchar %s)", slot_nm, i + 1, creationInfo.resName);
          continue;
        }
        if (find_value_idx(slots, slot_id) < 0)
        {
          G_FAST_ASSERT(slot_id == decltype(slots)::value_type(slot_id)); // No overflow
          slots.push_back(slot_id);
        }
        else
          logerr("duplicate slot <%s> definition in block #%d (animchar %s)", slot_nm, i + 1, creationInfo.resName);
      }

    recalcableAttachments = 0;
    clear_and_resize(attachment, slots.size());
    mem_set_0(attachment);
    slots.shrink_to_fit();
    attachmentSlotId = eastl::move(slots);

    for (int i = 0; i < b->blockCount(); i++)
      if (b->getBlock(i)->getBlockNameId() == nid)
      {
        const char *slot_nm = b->getBlock(i)->getStr("slot", NULL);
        int slot_id = AnimCharV20::addSlotId(slot_nm);
        if (slot_id < 0)
          continue;
        int idx = find_value_idx(attachmentSlotId, slot_id);
        G_ASSERTF(idx >= 0, "slot_id=%d blk #%d", slot_id, i + 1);
        const char *node_nm = b->getBlock(i)->getStr("node", NULL);
        attachment[idx].nodeIdx = node_nm ? nodeTree.findNodeIndex(node_nm) : (nodeTree.empty() ? dag::Index16() : dag::Index16(0));
        if (node_nm && !attachment[idx].nodeIdx)
        {
          attachment[idx].nodeIdx = nodeTree.empty() ? dag::Index16() : dag::Index16(0);

          char buf[4 << 10];
          logerr("slot <%s> refers to bad node <%s>%s", slot_nm, node_nm, dgs_get_fatal_context(buf, sizeof(buf)));
        }
        TMatrix tm = b->getBlock(i)->getTm("tm", TMatrix::IDENT);
        attachment[idx].tm.col0 = v_and(v_ldu(tm.m[0]), v_cast_vec4f(V_CI_MASK1110));
        attachment[idx].tm.col1 = v_and(v_ldu(tm.m[1]), v_cast_vec4f(V_CI_MASK1110));
        attachment[idx].tm.col2 = v_and(v_ldu(tm.m[2]), v_cast_vec4f(V_CI_MASK1110));
        attachment[idx].tm.col3 = v_perm_xyzd(v_ldu(tm.m[3]), V_C_UNIT_0001);
      }
  }
}

void AnimcharBaseComponent::cloneTo(AnimcharBaseComponent *as, bool reset_anim) const
{
  TIME_PROFILE_DEV(animchar_clone);

  as->animGraph = animGraph;
  as->nodeTree = nodeTree;
  as->node0 = node0;
  as->charDepNodeId = charDepNodeId;
  as->charDepBasePYofs = charDepBasePYofs;
  as->physSys = physSys;

  if (originalNodeTree)
    as->setOriginalNodeTreeRes(originalNodeTree);

  if (animGraph)
  {
    as->animMap = animMap;
    as->animMapPRS = animMapPRS;
    as->animState = new AnimCommonStateHolder(*animState);

    as->stateDirector = cloneGraph(stateDirector, as->animState);
    as->stateDirectorEnabled = stateDirectorEnabled;
    if (as->stateDirector)
      as->stateDirector->reset(true);
  }
  as->creationInfo = creationInfo;
  as->centerNodeBsphRad = centerNodeBsphRad;
  as->centerNodeIdx = centerNodeIdx;

  as->recalcableAttachments = recalcableAttachments;
  as->attachmentSlotId = attachmentSlotId;
  as->attachment = attachment;
  for (int i = 0; i < attachment.size(); i++)
  {
    as->attachment[i].animChar = NULL;
    as->attachment[i].nodeIdx = attachment[i].nodeIdx;
    as->attachment[i].tm = attachment[i].tm;
    as->attachment[i].uid = INVALID_ATTACHMENT_UID;
    as->attachment[i].recalcable = false;
  }

  if (reset_anim && animGraph)
    as->resetAnim();
}

int AnimcharBaseComponent::setAttachedChar(int slot_id, attachment_uid_t uid, AnimcharBaseComponent *attChar, bool recalcable)
{
  int idx = slot_id >= 0 ? find_value_idx(attachmentSlotId, slot_id) : -1;
  if (idx < 0)
    return idx;
  releaseAttachment(idx);
  attachment[idx].animChar = attChar;
  attachment[idx].uid = uid;
  attachment[idx].recalcable = recalcable;
  if (attChar && recalcable && idx < sizeof(recalcableAttachments) * CHAR_BIT)
    recalcableAttachments |= decltype(recalcableAttachments)(1) << idx;
  return idx;
}

AnimcharBaseComponent *AnimcharBaseComponent::getAttachedChar(int slot_id) const
{
  int idx = slot_id >= 0 ? find_value_idx(attachmentSlotId, slot_id) : -1;
  if (idx < 0)
    return NULL;
  return attachment[idx].animChar;
}
GeomNodeTree *AnimcharBaseComponent::getAttachedSkeleton(int slot_id) const
{
  int idx = slot_id >= 0 ? find_value_idx(attachmentSlotId, slot_id) : -1;
  if (idx < 0)
    return NULL;
  return attachment[idx].animChar ? &attachment[idx].animChar->getNodeTree() : NULL;
}
const mat44f *AnimcharBaseComponent::getAttachmentTm(int slot_id) const
{
  int idx = slot_id >= 0 ? find_value_idx(attachmentSlotId, slot_id) : -1;
  if (idx < 0)
    return NULL;
  return &attachment[idx].tm;
}
const mat44f *AnimcharBaseComponent::getSlotNodeWtm(int slot_id) const
{
  int idx = slot_id >= 0 ? find_value_idx(attachmentSlotId, slot_id) : -1;
  if (idx < 0)
    return NULL;
  return &nodeTree.getNodeWtmRel(attachment[idx].nodeIdx);
}

bool AnimcharBaseComponent::initAttachmentTmAndNodeWtm(int slot_id, mat44f &attach_tm) const
{
  int idx = slot_id >= 0 ? find_value_idx(attachmentSlotId, slot_id) : -1;
  if (idx < 0)
    return false;
  v_mat44_mul(attach_tm, nodeTree.getNodeWtmRel(attachment[idx].nodeIdx), attachment[idx].tm);
  return true;
}

dag::Index16 AnimcharBaseComponent::getSlotNodeIdx(int slot_id) const
{
  int idx = slot_id >= 0 ? find_value_idx(attachmentSlotId, slot_id) : -1;
  if (idx < 0)
    return {};
  return attachment[idx].nodeIdx;
}

void AnimcharBaseComponent::releaseAttachment(int idx)
{
  if (idx < 0 || idx >= attachment.size())
    return;
  attachment[idx].uid = INVALID_ATTACHMENT_UID;
  attachment[idx].animChar = NULL;
  if (idx < sizeof(recalcableAttachments) * CHAR_BIT)
    recalcableAttachments &= ~(decltype(recalcableAttachments)(1) << idx);
}

void AnimcharBaseComponent::setupAnim()
{
  if (!animGraph)
    return;

  animMap.reserve(nodeTree.nodeCount());
  if (originalNodeTree)
    animMapPRS.reserve(nodeTree.nodeCount() * 3);
  if (node0.chanNodeId() != -2)
  {
    G_ASSERTF(originalNodeTree || node0.chanNodeId() == -2, "originalNodeTree=%p node0.chanNodeId=%d", originalNodeTree,
      node0.chanNodeId());
  }

  for (dag::Index16 i(0), ie(nodeTree.nodeCount()); i != ie; ++i)
  {
    AnimMap am;
    am.geomId = i;
    am.animId = animGraph->getNodeId(nodeTree.getNodeName(i));
    if (am.animId != -1)
    {
      if (am.animId == node0.chanNodeId())
        insert_items(animMap, 0, 1, &am);
      else
        animMap.push_back(am);

      if (originalNodeTree)
      {
        int base = (am.animId == node0.chanNodeId()) ? insert_items(animMapPRS, 0, 3) : append_items(animMapPRS, 3);
        v_mat4_decompose(originalNodeTree->getNodeTm(i), animMapPRS[base + 0], animMapPRS[base + 1], animMapPRS[base + 2]);
      }
    }
  }
  animMap.shrink_to_fit();
  animMapPRS.shrink_to_fit();

  animState = new AnimCommonStateHolder(*animGraph);
  // debug_ctx("setup animGraph: %d/%d nodes", animMap.size(), nmb.totalNodes);
}

void AnimcharBaseComponent::createDebugBlenderContext(bool dump_all_nodes)
{
  destroyDebugBlenderContext();
  animDbgCtx = animGraph ? animGraph->createDebugBlenderContext(&nodeTree, dump_all_nodes) : NULL;
}
void AnimcharBaseComponent::destroyDebugBlenderContext()
{
  if (animDbgCtx)
    animGraph->destroyDebugBlenderContext(animDbgCtx);
  animDbgCtx = NULL;
}
const DataBlock *AnimcharBaseComponent::getDebugBlenderState(bool dump_tm)
{
  if (animGraph && !animDbgCtx)
    createDebugBlenderContext(false);
  return animGraph ? animGraph->getDebugBlenderState(animDbgCtx, *animState, dump_tm) : NULL;
}
const DataBlock *AnimcharBaseComponent::getDebugNodemasks()
{
  if (animGraph && !animDbgCtx)
    createDebugBlenderContext(false);
  return animGraph ? animGraph->getDebugNodemasks(animDbgCtx) : NULL;
}

void AnimcharBaseComponent::calcAnimWtm(bool may_calc_anim)
{
  if (animGraph && !animValid && may_calc_anim)
  {
    AnimBlender::TlsContext *tls = NULL;
    nodeTree.invalidateWtm();
    vec3f world_translate = nodeTree.translateToZero();
    bool haveBlend = false;
    if (!postCtrl || !postCtrl->overridesBlender())
    {
      haveBlend |= animGraph->blend(*(tls = &animGraph->selectBlenderCtx(&irq, this)), *animState, getCharDepModif());
      haveBlend |= motionMatchingController && motionMatchingController->blend(*tls, animMap, animMapPRS);
      // we need both blend, because we have blending from animGraph to motionMatchingController.
    }
    if (haveBlend && tls)
    {
#if MEASURE_PERF
      perfanimgblend::perf_tm.go();
      perf_tm2.go();
#endif

      mat44f *gn_tm_base = &nodeTree.getRootTm();
      if (node0.chanNodeId() != -2)
      {
        const vec3f *p, *s;
        const quat4f *r;

        if (node0.chanNodeId() >= 0)
        {
          G_ASSERT(node0.chanNodeId() == animMap[0].animId);
          animGraph->getNodeTm(*tls, animMap[0].animId, gn_tm_base[animMap[0].geomId.index()], &animMapPRS[0]);
          for (int i = 1, e = animMap.size(); i < e; i++)
          {
            animGraph->getNodePRS(*tls, animMap[i].animId, &p, &r, &s, animMapPRS.data() + i * 3);
            if (r)
              v_mat44_compose(gn_tm_base[animMap[i].geomId.index()], p ? *p : animMapPRS[i * 3 + 0], *r,
                s ? *s : animMapPRS[i * 3 + 2]);
          }
        }
        else
          for (int i = 0, e = animMap.size(); i < e; i++)
          {
            animGraph->getNodePRS(*tls, animMap[i].animId, &p, &r, &s, animMapPRS.data() + i * 3);
            if (r)
              v_mat44_compose(gn_tm_base[animMap[i].geomId.index()], animMapPRS[i * 3 + 0], *r, animMapPRS[i * 3 + 2]);
          }
      }
      else
      {
        for (int i = 0, e = animMap.size(); i < e; i++)
          animGraph->getNodeTm(*tls, animMap[i].animId, gn_tm_base[animMap[i].geomId.index()],
            animMapPRS.data() ? animMapPRS.data() + i * 3 : NULL);
      }
#if MEASURE_PERF
      perf_tm2.pause();
      perfanimgblend::perf_tm.pause();
#endif
    }

    nodeTree.calcWtm();

#if MEASURE_PERF
    perfanimgblend::perf_tm.go();
    perf_tm3.go();
#endif
    // post-blend processing
    if (tls)
    {
      AnimPostBlendCtrl::Context ctx(*originalNodeTree, this, (float *)alloca(animGraph->getPbcWtParamCount() * sizeof(float)),
        animGraph->getPbcWtParamCount(), node0.chanNodeId() < -1 ? NULL : &node0.sScale, &irq, this, world_translate);
      animGraph->postBlendProcess(*tls, *animState, nodeTree, ctx);
    }

    if (postCtrl)
      postCtrl->onPostAnimBlend(nodeTree, world_translate);
#if MEASURE_PERF
    perf_tm3.pause();
    perfanimgblend::perf_tm.pause();
#endif

    recalcWtm(world_translate);

    animValid = true;
    return;
  }

  recalcWtm();
}

void AnimcharBaseComponent::recalcAttachments()
{
  for (int i = 0, e = attachment.size(); i < e; i++)
  {
    G_ASSERTF(nodeTree.isIndexValid(attachment[i].nodeIdx), "nodeIdx=%d nodeTree=%p (%d nodes)", (int)attachment[i].nodeIdx, &nodeTree,
      nodeTree.nodeCount());
    if (attachment[i].animChar && attachment[i].recalcable)
    {
      mat44f tm;
      //== we could use bitflag for identity tm to skip mat44f multiplication (but for now ALL tms are not identical)
      v_mat44_mul(tm, nodeTree.getNodeWtmRel(attachment[i].nodeIdx), attachment[i].tm);
      attachment[i].animChar->setTm(tm);
      attachment[i].animChar->doRecalcAnimAndWtm();
    }
  }
}

void AnimcharBaseComponent::setOriginalNodeTreeRes(GeomNodeTree *n)
{
  if (n)
    ::game_resource_add_ref((GameResource *)n);
  if (originalNodeTree)
    ::release_game_resource((GameResource *)originalNodeTree);
  originalNodeTree = n;
}

intptr_t AnimcharBaseComponent::irq(int type, intptr_t p1, intptr_t p2, intptr_t p3, void *arg)
{
  AnimcharBaseComponent *ac = (AnimcharBaseComponent *)arg;

  // debug_ctx("irq #%d: 0x%p(%s), 0x%p, 0x%p (time=%.4f)", type, p1, animGraph->getBlendNodeName((IAnimBlendNode*)p1), p2, p3,
  // get_time_msec()/1000.0);
  if (type >= GIRQT_FIRST_SERVICE_IRQ)
  {
    // route service irqs to states director
    if (ac->stateDirector && ac->stateDirectorEnabled)
      ac->stateDirector->atIrq(type, (IAnimBlendNode *)p1);
  }
  else if (type == GIRQT_TraceFootStepDown)
  {
    if (!AnimCharV20::trace_static_ray_down)
      return GIRQR_NoResponse;
    Point3_vec4 &pt = *(Point3_vec4 *)(void *)p1;
    float max_trace_dist = *(float *)(void *)p2;
    float *res = (float *)(void *)p3;
    // draw_debug_line_buffered(pt, pt-Point3(0, max_trace_dist, 0), E3DCOLOR_MAKE(255,32,32,255));
    if (AnimCharV20::trace_static_ray_down(pt, max_trace_dist, *res, ac->traceContext))
    {
      // draw_debug_line_buffered(pt, pt-Point3(0, *res, 0), E3DCOLOR_MAKE(2,255,32,255));
      return GIRQR_TraceOK;
    }
  }
  else if (type == GIRQT_TraceFootStepDir)
  {
    if (!AnimCharV20::trace_static_ray_dir)
      return GIRQR_NoResponse;
    Point3_vec4 &pt = *(Point3_vec4 *)(void *)p1;
    const Point3_vec4 &dir = *(Point3_vec4 *)(void *)p2;
    float *res = (float *)(void *)p3;
    float max_trace_dist = *res;
    if (AnimCharV20::trace_static_ray_dir(pt, dir, max_trace_dist, *res, ac->traceContext))
    {
      // draw_debug_line_buffered(pt, pt-Point3(0, *res, 0), E3DCOLOR_MAKE(2,255,32,255));
      return GIRQR_TraceOK;
    }
  }
  else
  {
    // route user irqs to registered handlers
    if (type < ac->irqHandlers.size())
      for (auto *handler : ac->irqHandlers[type])
        handler->irq(type, p1, p2, p3);
  }

  return GIRQR_NoResponse;
}

bool AnimcharBaseComponent::updateCharDepScales(float pyOfs, float pxScale, float pyScale, float pzScale, float sScale)
{
  if (node0.chanNodeId() == -2 || !animGraph)
    return false;
  node0.pOfs = v_make_vec4f(0, pyOfs, 0, 0);
  node0.pScale_id = v_perm_xyzd(v_make_vec4f(pxScale, pyScale, pzScale, 0.f), node0.pScale_id);
  node0.sScale = v_splats(sScale);
  return true;
}
bool AnimcharBaseComponent::getCharDepScales(float &pyOfs, float &pxScale, float &pyScale, float &pzScale, float &sScale) const
{
  if (node0.chanNodeId() == -2 || !animGraph)
    return false;
  pyOfs = v_extract_y(node0.pOfs);
  pxScale = v_extract_x(node0.pScale_id);
  pyScale = v_extract_y(node0.pScale_id);
  pzScale = v_extract_z(node0.pScale_id);
  sScale = v_extract_x(node0.sScale);
  return true;
}
