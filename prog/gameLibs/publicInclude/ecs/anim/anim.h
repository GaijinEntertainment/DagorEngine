//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/entityId.h>
#include <daECS/core/entityComponent.h>
#include <daECS/core/event.h>
#include <animChar/dag_animCharacter2.h>
#include <anim/dag_animPureInterface.h>
#include <math/dag_mathUtils.h>
#include <math/dag_TMatrix.h>
struct AnimcharNodesMat44 : public AnimV20::AnimcharFinalMat44
{
  AnimcharNodesMat44() = default;
  bool onLoaded(ecs::EntityManager &mgr, ecs::EntityId eid);
  EA_NON_COPYABLE(AnimcharNodesMat44);
};

inline void animchar_copy_nodes(const AnimV20::AnimcharBaseComponent &animchar, AnimcharNodesMat44 &nodes, vec3f &root)
{
  animchar.copyNodesTo(nodes);
  root = v_add(nodes.nwtm[0].col3, nodes.wofs);
}

ECS_DECLARE_RELOCATABLE_TYPE(AnimV20::AnimcharBaseComponent);
ECS_DECLARE_RELOCATABLE_TYPE(AnimV20::AnimcharRendComponent);
ECS_DECLARE_RELOCATABLE_TYPE(AnimcharNodesMat44);

#if DAECS_EXTENSIVE_CHECKS
#define ANIMCHAR_VERIFY_NODE_POS_S(pos, nidx, animchar)                                                                          \
  G_ASSERTF(!check_nan(pos) && (pos).lengthSq() < 1e10f, "Bad node %d(%s) pos=%@ of animchar '%s' wtmOfs=(%f,%f,%f)", (int)nidx, \
    (animchar).getNodeTree().getNodeName(dag::Index16(nidx)), pos, (animchar).getCreateInfo()->resName,                          \
    v_extract_x((animchar).getWtmOfs()), v_extract_y((animchar).getWtmOfs()), v_extract_z((animchar).getWtmOfs()))
#define ANIMCHAR_VERIFY_NODE_POS(vpos, nidx, animchar)                                                                 \
  G_ASSERTF(!check_nan(as_point3(&vpos)) && v_extract_x(v_length3_sq_x(vpos)) < 1e10f,                                 \
    "Bad node %d(%s) pos=(%f,%f,%f) of animchar '%s' wtmOfs=(%f,%f,%f)", (int)nidx,                                    \
    (animchar).getNodeTree().getNodeName(dag::Index16(nidx)), v_extract_x(vpos), v_extract_y(vpos), v_extract_z(vpos), \
    (animchar).getCreateInfo()->resName, v_extract_x((animchar).getWtmOfs()), v_extract_y((animchar).getWtmOfs()),     \
    v_extract_z((animchar).getWtmOfs()))
#else
#define ANIMCHAR_VERIFY_NODE_POS_S(vpos, nidx, animchar) ((void)0)
#define ANIMCHAR_VERIFY_NODE_POS(vpos, nidx, animchar)   ((void)0)
#endif


ECS_UNICAST_EVENT_TYPE(EventAnimIrq, int /* irqType */)

// Irq handler that unregisters itself on destruction
namespace ecs
{
class AnimIrqHandler : public AnimV20::IGenericIrq
{
protected:
  ecs::EntityId eid;
  AnimIrqHandler(ecs::EntityId ent) : eid(ent) {}
  AnimIrqHandler(AnimIrqHandler &&) = default;
  ~AnimIrqHandler();
};
} // namespace ecs

namespace anim
{
void dump_animchar_state(AnimV20::AnimcharBaseComponent &animchar, const bool json_format = false);
void animchar_act(float dt, float *accum_dt, const float *dt_threshold, AnimV20::AnimcharBaseComponent &animchar,
  bool animchar__turnDir, const TMatrix &transform, AnimcharNodesMat44 *animchar_node_wtm, vec4f *animchar_render__root_pos);
}; // namespace anim
