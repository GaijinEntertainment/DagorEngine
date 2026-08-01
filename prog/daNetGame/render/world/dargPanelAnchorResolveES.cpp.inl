// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/componentTypes.h>
#include <daECS/core/ecsQuery.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <ecs/anim/anim.h>
#include <math/dag_TMatrix.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <util/dag_hash.h>
#include <osApiWrappers/dag_localConv.h> // dd_stricmp: match findINodeIndex' case-insensitive lookup
#include <string.h>

template <typename Callable>
static void get_entity_node_transform_ecs_query(ecs::EntityManager &manager, ecs::EntityId, Callable);

template <typename Callable>
static void get_entity_transform_ecs_query(ecs::EntityManager &manager, ecs::EntityId, Callable);

// Node name lookup is a linear scan over the skeleton, so cache resolved indices.
// A cached index is verified against the node name before use, so stale entries are harmless.
static ska::flat_hash_map<uint64_t, dag::Index16> resolved_node_index_cache;

static dag::Index16 find_node_index_cached(uint32_t eid, const GeomNodeTree &geom_tree, const char *node_name)
{
  const uint64_t key = (uint64_t(eid) << 32) | str_hash_fnv1(node_name);
  auto it = resolved_node_index_cache.find(key);
  if (it != resolved_node_index_cache.end() && geom_tree.isIndexValid(it->second) &&
      dd_stricmp(geom_tree.getNodeName(it->second), node_name) == 0) // findINodeIndex matches case-insensitively
    return it->second;
  dag::Index16 nodeIndex = geom_tree.findINodeIndex(node_name);
  if (nodeIndex)
  {
    // entries for destroyed entities are never queried again, so bound the map size
    // instead of tracking entity lifetime; a re-resolve after a clear is one linear scan
    if (resolved_node_index_cache.size() >= 1024)
      resolved_node_index_cache.clear();
    resolved_node_index_cache[key] = nodeIndex;
  }
  else if (it != resolved_node_index_cache.end())
    resolved_node_index_cache.erase(it); // node is gone from this skeleton: drop the stale entry
  return nodeIndex;
}

TMatrix get_entitiy_node_transform(uint32_t eid, const char *node_name = nullptr)
{
  TMatrix tm = TMatrix::ZERO;
  if (node_name && node_name[0])
    get_entity_node_transform_ecs_query(*g_entity_mgr, ecs::EntityId(eid), [&](const AnimV20::AnimcharBaseComponent &animchar) {
      const GeomNodeTree &geomTree = animchar.getNodeTree();
      if (GeomNodeTree::Index16 nodeIndex = find_node_index_cached(eid, geomTree, node_name))
        geomTree.getNodeWtmScalar(nodeIndex, tm);
    });
  else
    get_entity_transform_ecs_query(*g_entity_mgr, ecs::EntityId(eid), [&](const TMatrix &transform) { tm = transform; });

  return tm;
}
