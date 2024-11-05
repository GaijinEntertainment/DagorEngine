// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/componentTypes.h>
#include <daECS/core/ecsQuery.h>
#include <ecs/anim/anim.h>
#include <math/dag_TMatrix.h>

template <typename Callable>
static void get_entity_node_transform_ecs_query(ecs::EntityId, Callable);

template <typename Callable>
static void get_entity_transform_ecs_query(ecs::EntityId, Callable);

// FIXME: geomTree.findINodeIndex(node_name) is not fast. We should cache node index and use that instead.
TMatrix get_entitiy_node_transform(uint32_t eid, const char *node_name = nullptr)
{
  TMatrix tm = TMatrix::ZERO;
  if (node_name && node_name[0])
    get_entity_node_transform_ecs_query(ecs::EntityId(eid), [&](const AnimV20::AnimcharBaseComponent &animchar) {
      const GeomNodeTree &geomTree = animchar.getNodeTree();
      if (GeomNodeTree::Index16 nodeIndex = geomTree.findINodeIndex(node_name))
        geomTree.getNodeWtmScalar(nodeIndex, tm);
    });
  else
    get_entity_transform_ecs_query(ecs::EntityId(eid), [&](const TMatrix &transform) { tm = transform; });

  return tm;
}
