// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>
#include <daECS/core/coreEvents.h>
#include <ecs/anim/anim.h>
#include <shaders/dag_dynSceneRes.h>
#include "render/world/hideNodesEvent.h"

ECS_ON_EVENT(on_appear)
static void hero_animchar_node_hider_init_es_event_handler(const ecs::Event &,
  const ecs::string &hero_animchar_node_hider__rootNode,
  const ecs::StringList &hero_animchar_node_hider__endNodes,
  const AnimV20::AnimcharBaseComponent &animchar,
  const AnimV20::AnimcharRendComponent &animchar_render,
  ecs::IntList &hero_animchar_node_hider__nodeIndices)
{
  const GeomNodeTree &nodeTree = animchar.getNodeTree();
  const auto rootNodeIdx = nodeTree.findNodeIndex(hero_animchar_node_hider__rootNode.c_str());
  if (!rootNodeIdx)
    return;

  eastl::vector<dag::Index16, framemem_allocator> childNodes{rootNodeIdx};

  const DynamicRenderableSceneInstance *sceneInstance = animchar_render.getSceneInstance();
  for (size_t i = 0; i < childNodes.size(); ++i)
  {
    const auto idx = childNodes[i];
    const char *nodeName = nodeTree.getNodeName(idx);
    int renderNodeId = sceneInstance->getNodeId(nodeName);
    if (renderNodeId >= 0)
      hero_animchar_node_hider__nodeIndices.push_back(renderNodeId);

    bool isEndNode = false;
    for (const ecs::string &endNode : hero_animchar_node_hider__endNodes)
      isEndNode |= endNode == nodeName;
    if (isEndNode)
      continue;

    size_t childCount = nodeTree.getChildCount(idx);
    for (size_t j = 0; j < childCount; ++j)
      childNodes.push_back(nodeTree.getChildNodeIdx(idx, j));
  }
}

ECS_REQUIRE(ecs::Tag hero)
static inline void hero_animchar_node_hider_es_event_handler(const HideNodesEvent &,
  AnimV20::AnimcharRendComponent &animchar_render,
  const bool human_net_phys__isZoomingRenderData,
  const ecs::IntList &hero_animchar_node_hider__nodeIndices)
{
  if (!human_net_phys__isZoomingRenderData)
    return;
  TMatrix zeroTm = ZERO<TMatrix>();
  DynamicRenderableSceneInstance *sceneInstance = animchar_render.getSceneInstance();
  for (const int &nodeIdx : hero_animchar_node_hider__nodeIndices)
  {
    const Point3 &nodePos = *(const Point3 *)sceneInstance->getNodeWtm(nodeIdx).m[3];
    zeroTm.setcol(3, nodePos);
    sceneInstance->setNodeWtm(nodeIdx, zeroTm);
  }
}
