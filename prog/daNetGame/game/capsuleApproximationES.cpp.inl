// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/anim/anim.h>
#include <ecs/core/entityManager.h>
#include "capsuleApproximation.h"
#include <ecs/phys/collRes.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/componentTypes.h>
#include <ecs/render/updateStageRender.h>

#include <gameRes/dag_collisionResource.h>
#include <math/dag_math3d.h>
#include <util/dag_convar.h>
#include <debug/dag_debug3d.h>

ConVarB capsule_approximation_debug_draw("capsule_approximation.debug_draw", false, nullptr);

ECS_REGISTER_SHARED_TYPE(CapsuleApproximation, nullptr);
ECS_AUTO_REGISTER_COMPONENT_DEPS(
  ecs::SharedComponent<CapsuleApproximation>, "capsule_approximation", nullptr, 0, "collres", "animchar");

template <typename Callable>
static void capsule_approximation_debug_ecs_query(Callable c);

ECS_NO_ORDER
ECS_TAG(render, dev)
static __forceinline void debug_draw_capsule_approximation_es(const UpdateStageInfoRenderDebug &)
{
  if (!capsule_approximation_debug_draw.get())
    return;
  begin_draw_cached_debug_lines();
  capsule_approximation_debug_ecs_query(
    [](ECS_SHARED(CapsuleApproximation) capsule_approximation, const AnimV20::AnimcharBaseComponent &animchar) {
      if (capsule_approximation.capsuleDatas.empty())
        return;
      BBox3 box;
      for (auto &data : capsule_approximation.capsuleDatas)
      {
        G_ASSERT_CONTINUE(data.nodeIndex);
        TMatrix tm;
        animchar.getNodeTree().getNodeWtmScalar(data.nodeIndex, tm);
        Point3 ptA = tm * data.a, ptB = tm * data.b;
        float rad = data.rad * length(tm.getcol(0));
        BBox3 mbox;
        mbox += ptA;
        mbox += ptB;
        mbox[0] -= Point3(rad, rad, rad);
        mbox[1] += Point3(rad, rad, rad);
        box += mbox;
        draw_cached_debug_capsule_w(ptA, ptB, rad, E3DCOLOR_MAKE(255, 64, 255, 255));
      }
      draw_cached_debug_box(box, E3DCOLOR_MAKE(255, 128, 255, 255));
    });
  end_draw_cached_debug_lines();
}

static const char *defaultNodeNames[] = {
  "Bip01 L Thigh", "Bip01 L UpperArm", "Bip01 L Forearm", "Bip01 R Thigh", "Bip01 R UpperArm", "Bip01 R Forearm", "Bip01 Head"};
static const char *defaultComplexNodeNames[3][2] = {
  {"Bip01 Spine", "Bip01 Spine1"}, {"Bip01 L Calf", "Bip01 L Foot"}, {"Bip01 R Calf", "Bip01 R Foot"}};

bool CapsuleApproximation::onLoaded(const ecs::EntityManager &mgr, ecs::EntityId eid)
{
  const ecs::Array *nodeNames = mgr.getNullable<ecs::Array>(eid, ECS_HASH("capsule_approximation__nodeNames"));
  const ecs::Array *complexNodeNames = mgr.getNullable<ecs::Array>(eid, ECS_HASH("capsule_approximation__complexNodeNames"));
  const CollisionResource &collres = mgr.get<CollisionResource>(eid, ECS_HASH("collres"));
  const AnimV20::AnimcharBaseComponent &animchar = mgr.get<AnimV20::AnimcharBaseComponent>(eid, ECS_HASH("animchar"));

  auto checkNode = [eid](const char *nodeName, dag::Index16 nodeIdx, const CollisionNode *collisionNode) {
    G_UNUSED(nodeName);
    if (!nodeIdx)
    {
#if DAGOR_DBGLEVEL > 0
      logerr("Unknown animchar node (%s) on template <%s>", nodeName, g_entity_mgr->getEntityTemplateName(eid));
#endif
      return false;
    }
    if (!collisionNode)
    {
#if DAGOR_DBGLEVEL > 0
      logerr("Unknown collision node (%s) on template <%s>", nodeName, g_entity_mgr->getEntityTemplateName(eid));
#endif
      return false;
    }
    return true;
  };

  auto addNode = [&](const char *nodeName) {
    const auto nodeIdx = animchar.getNodeTree().findINodeIndex(nodeName);
    const CollisionNode *collisionNode = collres.getNodeByName(nodeName);
    if (checkNode(nodeName, nodeIdx, collisionNode))
      addCapsuleData(nodeIdx, collisionNode->nodeIndex, collisionNode->modelBBox);
  };
  if (nodeNames)
    for (const ecs::ChildComponent &node : *nodeNames)
      addNode(node.getStr());
  else
    for (const char *nodeName : defaultNodeNames)
      addNode(nodeName);

  BBox3 box;
  dag::Index16 rootNodeIdx;
  int rootCollNodeId = -1;
  TMatrix tm;
  auto beginComplexNode = [&]() {
    box.setempty();
    rootNodeIdx = dag::Index16();
    rootCollNodeId = -1;
    tm.identity();
  };
  auto addComplexNode = [&](const char *nodeName) {
    const auto nodeIdx = animchar.getNodeTree().findINodeIndex(nodeName);
    const CollisionNode *collisionNode = collres.getNodeByName(nodeName);
    if (!checkNode(nodeName, nodeIdx, collisionNode))
      return;
    if (!rootNodeIdx)
    {
      rootNodeIdx = nodeIdx;
      rootCollNodeId = collisionNode->nodeIndex;
      box = collisionNode->modelBBox;
    }
    else
    {
      TMatrix localTM;
      animchar.getNodeTree().getNodeTmScalar(nodeIdx, localTM);
      tm *= localTM;
      BBox3 addBox = tm * collisionNode->modelBBox;
      const Point3 size = abs(box.center() - addBox.center());
      int maxSide = 0;
      if (size[1] > size[maxSide])
        maxSide = 1;
      if (size[2] > size[maxSide])
        maxSide = 2;
      for (int i = 0; i < 3; ++i)
      {
        if (i == maxSide)
          continue;
        addBox.lim[0][i] = max(box.lim[0][i], addBox.lim[0][i]);
        addBox.lim[1][i] = min(box.lim[1][i], addBox.lim[1][i]);
      }
      box += addBox;
    }
  };
  auto endComplexNode = [&]() {
    if (rootNodeIdx)
      addCapsuleData(rootNodeIdx, rootCollNodeId, box);
  };
  if (complexNodeNames)
    for (const ecs::ChildComponent &nodes : *complexNodeNames)
    {
      beginComplexNode();
      for (const ecs::ChildComponent &nodeName : nodes.get<ecs::Array>())
        addComplexNode(nodeName.getStr());
      endComplexNode();
    }
  else
    for (const auto &nodes : defaultComplexNodeNames)
    {
      beginComplexNode();
      for (const char *const nodeName : nodes)
        addComplexNode(nodeName);
      endComplexNode();
    }
  inited = true;
  return inited;
}

void CapsuleApproximation::addCapsuleData(dag::Index16 node_idx, uint16_t coll_node_id, const BBox3 &box)
{
  Capsule c;
  c.set(box);
  capsuleDatas.push_back(CapsuleData{c.a, c.r, c.b, node_idx, coll_node_id});
}
