//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotPhysDecl.h>
#include <daECS/core/entityId.h>
#include <dasModules/aotEcs.h>
#include <ecs/scripts/dasEcsEntity.h>
#include <ecs/anim/anim.h>
#include <ecs/anim/animState.h>
#include <ecs/anim/animchar_visbits.h>
#include <dasModules/aotGeomNodeTree.h>
#include <animChar/dag_animCharacter2.h>
#include <shaders/dag_dynSceneRes.h>
#include <animChar/dag_animate2ndPass.h>
#include <anim/dag_animBlendCtrl.h>
#include <anim/dag_animPostBlendCtrl.h>
#include <ecs/phys/ragdoll.h>
#include <phys/dag_physDecl.h>
#include <memory/dag_framemem.h>
#include <dasModules/dasManagedTab.h>
#include <generic/dag_patchTab.h>

#include <../engine/anim/animFifo.h>

namespace das
{
struct NameIdPair
{
  int id;
  const char *name;
};
} // namespace das
typedef Ptr<AnimV20::IAnimBlendNode> IAnimBlendNodePtr;
typedef Ptr<AnimV20::AnimBlendNodeLeaf> AnimBlendNodeLeafPtr;
typedef Ptr<AnimV20::AnimPostBlendCtrl> AnimPostBlendCtrlPtr;
MAKE_TYPE_FACTORY(AnimBlendNodeLeafPtr, AnimBlendNodeLeafPtr);
MAKE_TYPE_FACTORY(AnimPostBlendCtrlPtr, AnimPostBlendCtrlPtr);

using RoNameMapExIdPatchableTab = PatchableTab<uint16_t>;
DAS_BIND_SPAN(RoNameMapExIdPatchableTab, RoNameMapExIdPatchableTab, uint16_t);

MAKE_TYPE_FACTORY(NameIdPair, das::NameIdPair);
MAKE_TYPE_FACTORY(AnimcharBaseComponent, AnimV20::AnimcharBaseComponent);
MAKE_TYPE_FACTORY(AnimcharRendComponent, AnimV20::AnimcharRendComponent);
MAKE_TYPE_FACTORY(AnimationGraph, AnimV20::AnimationGraph);
MAKE_TYPE_FACTORY(IAnimStateHolder, AnimV20::IAnimStateHolder);
MAKE_TYPE_FACTORY(IAnimBlendNode, AnimV20::IAnimBlendNode);
MAKE_TYPE_FACTORY(AnimBlendNodeNull, AnimV20::AnimBlendNodeNull);
MAKE_TYPE_FACTORY(AnimBlendNodeContinuousLeaf, AnimV20::AnimBlendNodeContinuousLeaf);
MAKE_TYPE_FACTORY(AnimBlendNodeStillLeaf, AnimV20::AnimBlendNodeStillLeaf);
MAKE_TYPE_FACTORY(AnimBlendNodeParametricLeaf, AnimV20::AnimBlendNodeParametricLeaf);
MAKE_TYPE_FACTORY(AnimBlendNodeSingleLeaf, AnimV20::AnimBlendNodeSingleLeaf);
MAKE_TYPE_FACTORY(AnimBlendCtrl_1axis, AnimV20::AnimBlendCtrl_1axis);
MAKE_TYPE_FACTORY(AnimBlendCtrl_Fifo3, AnimV20::AnimBlendCtrl_Fifo3);
MAKE_TYPE_FACTORY(AnimBlendCtrl_RandomSwitcher, AnimV20::AnimBlendCtrl_RandomSwitcher);
MAKE_TYPE_FACTORY(AnimBlendCtrl_Hub, AnimV20::AnimBlendCtrl_Hub);
MAKE_TYPE_FACTORY(AnimBlendCtrl_Blender, AnimV20::AnimBlendCtrl_Blender);
MAKE_TYPE_FACTORY(AnimBlendCtrl_BinaryIndirectSwitch, AnimV20::AnimBlendCtrl_BinaryIndirectSwitch);
MAKE_TYPE_FACTORY(AnimBlendCtrl_SetMotionMatchingTag, AnimV20::AnimBlendCtrl_SetMotionMatchingTag);
MAKE_TYPE_FACTORY(AnimBlendCtrl_LinearPoly, AnimV20::AnimBlendCtrl_LinearPoly);
MAKE_TYPE_FACTORY(AnimBlendCtrl_ParametricSwitcher, AnimV20::AnimBlendCtrl_ParametricSwitcher);

MAKE_TYPE_FACTORY(AnimBlendNodeLeaf, AnimV20::AnimBlendNodeLeaf);
MAKE_TYPE_FACTORY(AnimPostBlendCtrl, AnimV20::AnimPostBlendCtrl);
MAKE_TYPE_FACTORY(AnimBlendCtrl_1axisAnimSlice, AnimV20::AnimBlendCtrl_1axis::AnimSlice);

MAKE_TYPE_FACTORY(AnimFifo3Queue, AnimV20::AnimFifo3Queue);
MAKE_TYPE_FACTORY(IAnimBlendNodePtr, IAnimBlendNodePtr);

MAKE_TYPE_FACTORY(AnimData, AnimV20::AnimData);
MAKE_TYPE_FACTORY(AnimBlendCtrl_ParametricSwitcherItemAnim, AnimV20::AnimBlendCtrl_ParametricSwitcher::ItemAnim);
MAKE_TYPE_FACTORY(AnimBlendCtrl_RandomSwitcherRandomAnim, AnimV20::AnimBlendCtrl_RandomSwitcher::RandomAnim);
MAKE_TYPE_FACTORY(AnimBlendCtrl_LinearPolyAnimPoint, AnimV20::AnimBlendCtrl_LinearPoly::AnimPoint);
MAKE_TYPE_FACTORY(AnimationGraphStateRec, AnimV20::AnimationGraph::StateRec);

MAKE_TYPE_FACTORY(AnimPostBlendParamFromNodeLocalData, AnimV20::AnimPostBlendParamFromNode::LocalData);
MAKE_TYPE_FACTORY(AnimPostBlendParamFromNode, AnimV20::AnimPostBlendParamFromNode);

MAKE_TYPE_FACTORY(AttachGeomNodeCtrlVarId, AnimV20::AttachGeomNodeCtrl::VarId);
using AttachGeomNodeCtrlVarIdTab = Tab<AnimV20::AttachGeomNodeCtrl::VarId>;
DAS_BIND_VECTOR(AttachGeomNodeCtrlVarIdTab, AttachGeomNodeCtrlVarIdTab, AnimV20::AttachGeomNodeCtrl::VarId,
  " ::Tab<AnimV20::AttachGeomNodeCtrl::VarId>");
MAKE_TYPE_FACTORY(AttachGeomNodeCtrlAttachDesc, AnimV20::AttachGeomNodeCtrl::AttachDesc);
MAKE_TYPE_FACTORY(AttachGeomNodeCtrl, AnimV20::AttachGeomNodeCtrl);

MAKE_TYPE_FACTORY(AnimcharNodesMat44, AnimcharNodesMat44);
MAKE_TYPE_FACTORY(RoNameMapEx, RoNameMapEx);
MAKE_TYPE_FACTORY(DynSceneResNameMapResource, DynSceneResNameMapResource);

// TODO: extract these render related types into separate module
MAKE_TYPE_FACTORY(DynamicRenderableSceneLodsResource, DynamicRenderableSceneLodsResource);
MAKE_TYPE_FACTORY(DynamicRenderableSceneInstance, DynamicRenderableSceneInstance);

MAKE_TYPE_FACTORY(Animate2ndPassCtx, Animate2ndPassCtx);

using BnlPtrTab = PtrTab<AnimV20::AnimBlendNodeLeaf>;
using PbCtrlPtrTab = PtrTab<AnimV20::AnimPostBlendCtrl>;

DAS_BIND_VECTOR(BnlPtrTab, BnlPtrTab, AnimBlendNodeLeafPtr, " ::PtrTab<AnimV20::AnimBlendNodeLeaf>");
DAS_BIND_VECTOR(PbCtrlPtrTab, PbCtrlPtrTab, AnimPostBlendCtrlPtr, " ::PtrTab<AnimV20::AnimPostBlendCtrl>");

MAKE_TYPE_FACTORY(AnimBlender, AnimV20::AnimBlender);

DAS_BIND_ENUM_CAST_98(AnimcharVisbits);

template <>
struct das::isCloneable<AnimV20::AnimationGraph> : false_type
{};

template <>
struct das::isCloneable<AnimV20::AnimBlender> : false_type
{};

namespace bind_dascript
{
inline void animchar_act(::AnimV20::AnimcharBaseComponent &animchar, real dt, bool calc_anim) { animchar.act(dt, calc_anim); }

inline const char *animchar_get_res_name(const ::AnimV20::AnimcharBaseComponent &animchar)
{
  return animchar.getCreateInfo()->resName.c_str();
}

inline int ronamemapex_get_name_id(const RoNameMapEx &names, const char *node_name)
{
  return names.getNameId(node_name ? node_name : "");
}

inline void scene_instance_show_node(DynamicRenderableSceneInstance *scene_instance, int node_idx, bool show)
{
  scene_instance->showNode(node_idx, show);
}

inline bool scene_instance_is_node_visible(const DynamicRenderableSceneInstance *scene_instance, int node_idx)
{
  return !scene_instance->isNodeHidden(node_idx);
}

inline void scene_instance_get_local_bounding_box(const DynamicRenderableSceneInstance *scene_instance, BBox3 &out)
{
  out = scene_instance->getLocalBoundingBox();
}

inline bool calc_world_box(const ::AnimCharV20::AnimcharRendComponent &animchar_render, bbox3f &box,
  const AnimcharNodesMat44 &finalWtm, bool accurate = true)
{
  return animchar_render.calcWorldBox(box, finalWtm, accurate);
}

inline int anim_graph_getStateIdx(const ::AnimV20::AnimationGraph &animGraph, const char *state_name)
{
  return animGraph.getStateIdx(state_name ? state_name : "");
}

inline int anim_graph_getNodeId(::AnimV20::AnimationGraph &animGraph, const char *state_name)
{
  return animGraph.getNodeId(state_name ? state_name : "");
}

inline int anim_graph_getBlendNodeId(const ::AnimV20::AnimationGraph &animGraph, const char *state_name)
{
  return animGraph.getBlendNodeId(state_name ? state_name : "");
}

inline int anim_graph_getParamId(const ::AnimV20::AnimationGraph &animGraph, const char *state_name, int type)
{
  return animGraph.getParamId(state_name ? state_name : "", type);
}

inline ::AnimV20::AnimBlendCtrl_Fifo3 *anim_graph_getFifo3NodePtr(::AnimV20::AnimationGraph &animGraph, int node_idx)
{
  ::AnimV20::IAnimBlendNode *node = animGraph.getBlendNodePtr(node_idx);
  if (node && node->isSubOf(::AnimV20::AnimBlendCtrl_Fifo3CID))
    return static_cast<::AnimV20::AnimBlendCtrl_Fifo3 *>(node);

  return nullptr;
}

inline ::AnimV20::IAnimBlendNode *AnimFifo3Queue_get_node(::AnimV20::AnimFifo3Queue &queue, int idx)
{
  return uint32_t(idx) < 3 ? queue.node[idx] : nullptr;
}

inline void AnimBlendCtrl_1axis_getChildren(const ::AnimV20::AnimBlendCtrl_1axis &ctrl,
  const das::TBlock<void, das::TTemporary<das::TArray<::AnimV20::AnimBlendCtrl_1axis::AnimSlice>>> &block, das::Context *context,
  das::LineInfoArg *at)
{
  auto res = ctrl.getChildren();

  das::Array arr;
  arr.data = (char *)res.data();
  arr.size = uint32_t(res.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

inline void anim_graph_getParamNames(const ::AnimV20::AnimationGraph &animGraph,
  const das::TBlock<void, das::TTemporary<das::TArray<das::NameIdPair>>> &block, das::Context *context, das::LineInfoArg *at)
{
  Tab<das::NameIdPair> pairs(framemem_ptr());
  pairs.reserve(animGraph.getParamNames().nameCount());
  iterate_names(animGraph.getParamNames(), [&pairs](int id, const char *name) { pairs.push_back(das::NameIdPair{id, name}); });

  das::Array arr;
  arr.data = (char *)pairs.data();
  arr.size = uint32_t(pairs.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

inline void anim_graph_getAnimNodeNames(const ::AnimV20::AnimationGraph &animGraph,
  const das::TBlock<void, das::TTemporary<das::TArray<das::NameIdPair>>> &block, das::Context *context, das::LineInfoArg *at)
{
  Tab<das::NameIdPair> pairs(framemem_ptr());
  pairs.reserve(animGraph.getAnimNodeNames().nameCount());
  iterate_names(animGraph.getAnimNodeNames(), [&pairs](int id, const char *name) { pairs.push_back(das::NameIdPair{id, name}); });

  das::Array arr;
  arr.data = (char *)pairs.data();
  arr.size = uint32_t(pairs.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

inline void anim_graph_getStRec(const ::AnimV20::AnimationGraph &animGraph,
  const das::TBlock<void, das::TTemporary<das::TArray<::AnimV20::AnimationGraph::StateRec>>> &block, das::Context *context,
  das::LineInfoArg *at)
{
  dag::ConstSpan<::AnimV20::AnimationGraph::StateRec> names = animGraph.getStRec();

  das::Array arr;
  arr.data = (char *)names.data();
  arr.size = uint32_t(names.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

inline float AnimBlendCtrl_ParametricSwitcherItemAnim_getStart(const ::AnimV20::AnimBlendCtrl_ParametricSwitcher::ItemAnim &item)
{
  return item.range[0];
}
inline float AnimBlendCtrl_ParametricSwitcherItemAnim_getEnd(const ::AnimV20::AnimBlendCtrl_ParametricSwitcher::ItemAnim &item)
{
  return item.range[1];
}

inline void AnimBlendCtrl_ParametricSwitcher_getChildren(::AnimV20::AnimBlendCtrl_ParametricSwitcher &param_switch,
  const das::TBlock<void, das::TTemporary<das::TArray<::AnimV20::AnimBlendCtrl_ParametricSwitcher::ItemAnim>>> &block,
  das::Context *context, das::LineInfoArg *at)
{
  dag::ConstSpan<::AnimV20::AnimBlendCtrl_ParametricSwitcher::ItemAnim> children = param_switch.getChildren();

  das::Array arr;
  arr.data = (char *)children.data();
  arr.size = uint32_t(children.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

inline void AnimBlendCtrl_RandomSwitcher_getChildren(const ::AnimV20::AnimBlendCtrl_RandomSwitcher &param_switch,
  const das::TBlock<void, das::TTemporary<das::TArray<::AnimV20::AnimBlendCtrl_RandomSwitcher::RandomAnim>>> &block,
  das::Context *context, das::LineInfoArg *at)
{
  dag::ConstSpan<::AnimV20::AnimBlendCtrl_RandomSwitcher::RandomAnim> children = param_switch.getChildren();

  das::Array arr;
  arr.data = (char *)children.data();
  arr.size = uint32_t(children.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

inline void AnimBlendCtrl_Hub_getChildren(const ::AnimV20::AnimBlendCtrl_Hub &hub,
  const das::TBlock<void, das::TTemporary<das::TArray<Ptr<::AnimV20::IAnimBlendNode>>>> &block, das::Context *context,
  das::LineInfoArg *at)
{
  dag::ConstSpan<Ptr<::AnimV20::IAnimBlendNode>> children = hub.getChildren();

  das::Array arr;
  arr.data = (char *)children.data();
  arr.size = uint32_t(children.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

inline void AnimBlendCtrl_Hub_getDefNodeWt(const ::AnimV20::AnimBlendCtrl_Hub &hub,
  const das::TBlock<void, das::TTemporary<das::TArray<float>>> &block, das::Context *context, das::LineInfoArg *at)
{
  dag::ConstSpan<float> children = hub.getDefNodeWt();

  das::Array arr;
  arr.data = (char *)children.data();
  arr.size = uint32_t(children.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

inline void AnimBlendCtrl_Blender_getChildren(const ::AnimV20::AnimBlendCtrl_Blender &hub,
  const das::TBlock<void, das::TTemporary<das::TArray<Ptr<::AnimV20::IAnimBlendNode>>>> &block, das::Context *context,
  das::LineInfoArg *at)
{
  dag::ConstSpan<Ptr<::AnimV20::IAnimBlendNode>> children = hub.getChildren();

  das::Array arr;
  arr.data = (char *)children.data();
  arr.size = uint32_t(children.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

inline void AnimBlendCtrl_BinaryIndirectSwitch_getChildren(const ::AnimV20::AnimBlendCtrl_BinaryIndirectSwitch &hub,
  const das::TBlock<void, das::TTemporary<das::TArray<Ptr<::AnimV20::IAnimBlendNode>>>> &block, das::Context *context,
  das::LineInfoArg *at)
{
  dag::ConstSpan<Ptr<::AnimV20::IAnimBlendNode>> children = hub.getChildren();

  das::Array arr;
  arr.data = (char *)children.data();
  arr.size = uint32_t(children.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

inline void AnimBlendCtrl_LinearPoly_getChildren(const ::AnimV20::AnimBlendCtrl_LinearPoly &hub,
  const das::TBlock<void, das::TTemporary<das::TArray<::AnimV20::AnimBlendCtrl_LinearPoly::AnimPoint>>> &block, das::Context *context,
  das::LineInfoArg *at)
{
  dag::ConstSpan<::AnimV20::AnimBlendCtrl_LinearPoly::AnimPoint> children = hub.getChildren();

  das::Array arr;
  arr.data = (char *)children.data();
  arr.size = uint32_t(children.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

inline void AttachGeomNodeCtrl_getNodeNames(const ::AnimV20::AttachGeomNodeCtrl &pbc,
  const das::TBlock<void, das::TTemporary<das::TArray<const char *>>> &block, das::Context *context, das::LineInfoArg *at)
{
  das::Array arr;
  arr.data = (char *)pbc.nodeNames.data();
  arr.size = uint32_t(pbc.nodeNames.size());
  arr.capacity = arr.size;
  arr.lock = 1;
  arr.flags = 0;
  vec4f arg = das::cast<das::Array *>::from(&arr);
  context->invoke(block, &arg, nullptr, at);
}

inline bool anim_blend_node_isSubOf(::AnimV20::IAnimBlendNode &blend_node, uint32_t id)
{
  return blend_node.isSubOf(DClassID(id)); // TODO: isSubOf() const
}

template <typename T>
inline ::AnimV20::AnimData *AnimBlendNodeLeaf_get_anim(T &anim)
{
  return anim.anim.get();
}

inline ::AnimV20::AnimData *AnimData_get_source_anim_data(const ::AnimV20::AnimData &anim_data)
{
  return anim_data.getSourceAnimData();
}

inline ::AnimV20::IAnimBlendNode *IAnimBlendNodePtr_get(IAnimBlendNodePtr &node_ptr) { return node_ptr.get(); }

inline ::AnimV20::AnimBlendNodeLeaf *AnimBlendNodeLeafPtr_get(AnimBlendNodeLeafPtr &node_ptr) { return node_ptr.get(); }

inline ::AnimV20::AnimPostBlendCtrl *AnimPostBlendCtrlPtr_get(AnimPostBlendCtrlPtr &node_ptr) { return node_ptr.get(); }

inline void send_change_anim_state_event(ecs::EntityId eid, const char *name, ecs::hash_str_t name_hash, int state_id)
{
  // Send immediate event here because the lifetime of the name is unknown.
  // It may be a temporary string and the event couldn't be delayed.
  g_entity_mgr->sendEventImmediate(eid, EventChangeAnimState(ecs::HashedConstString{name, name_hash}, state_id));
}

inline void anim_graph_enqueueState(::AnimV20::AnimationGraph &anim_graph, ::AnimV20::AnimCommonStateHolder &state, int state_idx,
  float force_dur, float force_speed)
{
  anim_graph.enqueueState(state, anim_graph.getState(state_idx), force_dur, force_speed);
}

inline void anim_graph_setStateSpeed(::AnimV20::AnimationGraph &anim_graph, ::AnimV20::AnimCommonStateHolder &state, int state_idx,
  float force_speed)
{
  anim_graph.setStateSpeed(state, anim_graph.getState(state_idx), force_speed);
}

inline bool anim_graph_enqueueNode(::AnimV20::AnimationGraph &anim_graph, ::AnimV20::AnimCommonStateHolder &state, const char *anim)
{
  if (anim_graph.getRoot() && anim_graph.getRoot()->isSubOf(AnimV20::AnimBlendCtrl_Fifo3CID))
  {
    AnimV20::IAnimBlendNode *n = anim_graph.getBlendNodePtr(anim);
    if (!n)
      return false;

    AnimV20::AnimBlendCtrl_Fifo3 *fifo = (AnimV20::AnimBlendCtrl_Fifo3 *)anim_graph.getRoot();
    if (!fifo->isEnqueued(state, n))
      n->resume(state, true);
    fifo->enqueueState(state, n, /*overlap_time*/ 0.15f, /*max_lag*/ 0.0f);
    return true;
  }
  return false;
}

inline void animchar_setRagdollPostController(::AnimCharV20::AnimcharBaseComponent &animchar, PhysRagdoll &ragdoll)
{
  animchar.setPostController(&ragdoll);
}
inline void animchar_resetRagdollPostController(::AnimCharV20::AnimcharBaseComponent &animchar)
{
  animchar.setPostController(nullptr);
}

inline void animchar_copy_nodes(const ::AnimCharV20::AnimcharBaseComponent &animchar, AnimcharNodesMat44 &nodes, das::float4 &root)
{
  vec3f animRoot;
  animchar_copy_nodes(animchar, nodes, animRoot);
  v_stu(&root.x, animRoot);
}

inline das::float4 animchar_render_prepareSphere(const ::AnimCharV20::AnimcharRendComponent &animchar_render,
  const AnimcharNodesMat44 &nodes)
{
  return animchar_render.prepareSphere(nodes);
}

inline BBox3 animchar_render_calcWorldBox(const ::AnimCharV20::AnimcharRendComponent &animchar_render,
  const AnimcharNodesMat44 &finalWtm, bool accurate)
{
  bbox3f out_wbb;
  animchar_render.calcWorldBox(out_wbb, finalWtm, accurate);
  BBox3 out;
  v_stu_bbox3(out, out_wbb);
  return out;
}

inline void animchar_getDebugBlenderState(::AnimV20::AnimcharBaseComponent &animchar, bool dump_tm,
  const das::TBlock<void, das::TTemporary<const DataBlock>> &block, das::Context *context, das::LineInfoArg *at)
{
  if (const DataBlock *res = animchar.getDebugBlenderState(dump_tm))
  {
    vec4f arg = das::cast<const DataBlock *>::from(res);
    context->invoke(block, &arg, nullptr, at);
  }
}

inline void animchar_getAnimBlendNodeWeights(::AnimV20::AnimationGraph &graph, ::AnimV20::IPureAnimStateHolder &st,
  const das::TBlock<void, das::TTemporary<das::TArray<float>>, das::TTemporary<das::TArray<float>>,
    das::TTemporary<das::TArray<float>>> &block,
  das::Context *context, das::LineInfoArg *at)
{
  ::AnimV20::AnimBlender::TlsContext &tlsCtx = graph.selectBlenderCtx();

  Tab<float> abnWt(framemem_ptr());
  mem_set_0(tlsCtx.bnlWt);
  mem_set_0(tlsCtx.pbcWt);
  abnWt.resize(graph.getAnimNodeCount());
  mem_set_0(abnWt);
  ::AnimV20::IAnimBlendNode::BlendCtx bctx(st, tlsCtx, true);
  bctx.abnWt = abnWt.data();
  graph.getRoot()->buildBlendingList(bctx, 1.0);

  das::Array abnWtArr;
  abnWtArr.data = (char *)abnWt.data();
  abnWtArr.size = uint32_t(abnWt.size());
  abnWtArr.capacity = abnWtArr.size;
  abnWtArr.lock = 1;
  abnWtArr.flags = 0;

  das::Array bnlWtArr;
  bnlWtArr.data = (char *)tlsCtx.bnlWt.data();
  bnlWtArr.size = uint32_t(tlsCtx.bnlWt.size());
  bnlWtArr.capacity = bnlWtArr.size;
  bnlWtArr.lock = 1;
  bnlWtArr.flags = 0;

  das::Array pbcWtArr;
  pbcWtArr.data = (char *)tlsCtx.pbcWt.data();
  pbcWtArr.size = uint32_t(tlsCtx.pbcWt.size());
  pbcWtArr.capacity = pbcWtArr.size;
  pbcWtArr.lock = 1;
  pbcWtArr.flags = 0;

  vec4f args[3];

  context->invokeEx(
    block, args, nullptr,
    [&](das::SimNode *code) {
      args[0] = das::cast<das::Array *>::from(&abnWtArr);
      args[1] = das::cast<das::Array *>::from(&bnlWtArr);
      args[2] = das::cast<das::Array *>::from(&pbcWtArr);
      code->eval(*context);
    },
    at);
}


} // namespace bind_dascript
