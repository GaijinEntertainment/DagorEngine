//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/anim/animComponent.h>
#include <daECS/core/entityId.h>
#include <daECS/core/event.h>
#include <anim/dag_animPureInterface.h>
#include <math/dag_mathUtils.h>
#include <math/dag_TMatrix.h>


// we subtract difference between accurate double camera position and camera position in floats (used in render)

// vec3f ofs = v_sub(animchar_node_wtm.wofs, rounded_cam_pos);
// ofs = v_sub(ofs, remainder_cam_pos);
// should be accurate enough.
// However, with fast-math enabled, clang replaces two ordered sub_ps with unordered add, basically calculating
// (rounded_cam_pos+remainder_cam_pos) first that is not only slower (more instructions), but also leads to inaccurate calculations
// unless we switch off fast-math, which isn't easy to do, we should workaround it
//-ffast-math (or fp:fast) in clang _driver_ enables following optimization in clang frontend:
//   -fassociative-math -freciprocal-math -fno-math-errno -ffinite-math-only -fno-trapping-math -fno-signed-zeros -ffp-contract=fast
//   -funsafe-math-optimizations
// and, additionally -ffast-math (in clang frontend), which is different optimization!
// unfortunately there is no way to switch it off. If we enable all the optimization above, clang driver discover it and
// automatically adds -ffast-math the only way (as it seems) is to get clang frontend command line (-### flag) and directly invoke
// clang frontend with that command-line - but without "-ffast-math" or, alternatively disable one of the optimizations (i.e.
// -fno-signed-zeros) as for now we work-around this using NEGATIVES. clang has no point to 'optimize' anything with that vec3f ofs =
// v_add(animchar_node_wtm.wofs, negative_rounded_cam_pos);
VECTORCALL DAGOR_NOINLINE inline vec4f animchar_reconstruct_offset(vec4f wofs, vec4f negative_rounded_cam_pos,
  vec4f negative_remainder_cam_pos)
{
  vec4f ab = v_add(wofs, negative_rounded_cam_pos);
  return v_add(ab, negative_remainder_cam_pos);
}

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
  bool animchar__turnDir, ecs::EntityId eid, const TMatrix &transform, AnimcharNodesMat44 *animchar_node_wtm,
  vec4f *animchar_render__root_pos);
}; // namespace anim
