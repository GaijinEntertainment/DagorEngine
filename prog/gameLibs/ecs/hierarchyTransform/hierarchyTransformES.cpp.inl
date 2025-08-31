// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/componentType.h>
#include <daECS/scene/scene.h>
#include <ecs/core/utility/ecsRecreate.h>

#include <vecmath/dag_vecMath.h>

// requires three templates:
// 1) "entities_hierarchy" (with one component ecs::EidList &hierarchy_transformation)
// 2) "hierarchial_entity" (with 4 components hierarchy_eid:eid, hierarchy_parent,hierarchy_last_parent:eid, hierarchy_transform:m)
//    To make save/load work it must also have these two components: hierarchy_unresolved_id:i, hierarchy_unresolved_parent_id:i. Both
//    must be set to 0 by default.
// 3) "hierarchial_free_transform" with one component hierarchy_parent_last_transform:m

// todo: check cycles on add

ECS_DEF_PULL_VAR(hierarchy_transform);

ECS_UNICAST_EVENT_TYPE(EventAttachToHierarchy);
ECS_REGISTER_EVENT(EventAttachToHierarchy);

template <typename Callable>
inline bool set_hierarchy_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable c);

static void add_entity_hierarchy(ecs::EntityManager &manager, ecs::EntityId eid, ecs::EntityId parent, ecs::EntityId hierarchyEid,
  const TMatrix &localHierarchyTransform)
{
  if (set_hierarchy_ecs_query(manager, eid,
        [&](ecs::EntityId &hierarchy_eid, ecs::EntityId &hierarchy_parent, ecs::EntityId &hierarchy_last_parent,
          TMatrix &hierarchy_transform) {
          hierarchy_eid = hierarchyEid;
          hierarchy_transform = localHierarchyTransform;
          hierarchy_parent = parent;
          hierarchy_last_parent = parent;
        }))
    return;
  ecs::ComponentsInitializer attrs;
  ECS_INIT(attrs, hierarchy_eid, hierarchyEid);
  ECS_INIT(attrs, hierarchy_transform, localHierarchyTransform);
  ECS_INIT(attrs, hierarchy_parent, parent);
  ECS_INIT(attrs, hierarchy_last_parent, parent);
  add_sub_template_async(eid, "hierarchial_entity", eastl::move(attrs));
}

ECS_ON_EVENT(on_appear, EventAttachToHierarchy)
ECS_TRACK(hierarchy_parent)
static void attach_entity_to_hierarchy_es(const ecs::Event &, ecs::EntityManager &manager, ecs::EntityId eid, const TMatrix &transform,
  ecs::EntityId hierarchy_parent, ecs::EntityId &hierarchy_eid, TMatrix &hierarchy_transform, ecs::EntityId &hierarchy_last_parent,
  const ecs::Tag *hierarchy_transform__set = nullptr)
{
  if (hierarchy_parent == eid) // this is not a good idea to add entity as self parent
    return;
  if (!hierarchy_parent) // nothing to attach to
    return;
  // this function only works if we add one entity to same parent (or recently added child) another once per frame
  // if you want to add few children simultaneously, we can either create another function (adding full hierarchy at once)
  // or start to use reCreateSync
  const ecs::EntityId *pHierarchy = ECS_GET_NULLABLE_MGR(manager, ecs::EntityId, hierarchy_parent, hierarchy_eid);
  ecs::EidList *pList = pHierarchy ? ECS_GET_NULLABLE_RW_MGR(manager, ecs::EidList, *pHierarchy, hierarchy_transformation) : nullptr;
  ecs::EntityId hierarchyEid;
  if (!pList)
  {
    // parent doesn't have (valid) hierarchy entity ref
    // child doesn't have valid hierarchy entity.
    // create full new hierarchy entity.
    hierarchyEid = manager.createEntitySync("entities_hierarchy");
    pList = ECS_GET_NULLABLE_RW_MGR(manager, ecs::EidList, hierarchyEid, hierarchy_transformation);
    G_ASSERTF_RETURN(pList, , "entities_hierarchy template doesn't have hierarchy_transformation component");
    pList->push_back(eid);
    add_entity_hierarchy(manager, hierarchy_parent, ecs::INVALID_ENTITY_ID, hierarchyEid, TMatrix::IDENT);
  }
  else
  {
    // add to existing hierarchy, pHierarchy is not NULL
    hierarchyEid = *pHierarchy; // -V1004
    ecs::EidList &hierarchy_transformation = *pList;
    {
      // check if we re-attaching to same hierarchy (when tracking change)
      auto it = eastl::find(hierarchy_transformation.begin(), hierarchy_transformation.end(), eid);
      if (it != hierarchy_transformation.end())
        hierarchy_transformation.erase(it);
    }

    // todo: check cycles
    // attach to hierarchy
    int i = hierarchy_transformation.size() - 1;
    for (; i >= 0; --i)
    {
      if (hierarchy_transformation[i] == hierarchy_parent)
      {
        hierarchy_transformation.insert(hierarchy_transformation.begin() + i + 1, eid);
        break;
      }
    }
    if (i < 0)
      hierarchy_transformation.push_back(eid);
  }
  hierarchy_eid = hierarchyEid;
  TMatrix parentTm = ECS_GET_OR_MGR(manager, hierarchy_parent, transform, TMatrix::IDENT);
  if (!hierarchy_transform__set)
    hierarchy_transform = inverse(parentTm) * transform;
  hierarchy_last_parent = hierarchy_parent;
}


ECS_REQUIRE(ecs::EntityId hierarchy_eid, TMatrix hierarchy_transform, TMatrix transform)
ECS_ON_EVENT(on_appear)
static void hierarchy_entity_became_free_transform_es(const ecs::Event &, ecs::EntityManager &manager, ecs::EntityId hierarchy_parent,
  TMatrix &hierarchy_parent_last_transform, ecs::EntityId &hierarchy_last_parent)
{
  hierarchy_parent_last_transform = ECS_GET_OR_MGR(manager, hierarchy_parent, transform, TMatrix::IDENT);
  hierarchy_last_parent = hierarchy_parent;
}

ECS_AFTER(animchar_non_updatable_es)
ECS_AFTER(start_async_phys_sim_es)
ECS_REQUIRE(ecs::EntityId hierarchy_eid)
static void hierarchy_attached_entity_free_transform_es(const ecs::UpdateStageInfoAct &, ecs::EntityManager &manager,
  ecs::EntityId hierarchy_parent, const TMatrix &transform, TMatrix &hierarchy_transform, TMatrix &hierarchy_parent_last_transform,
  ecs::EntityId &hierarchy_last_parent)
{
  if (!hierarchy_parent)
    return;

  const TMatrix parentTm = ECS_GET_OR_MGR(manager, hierarchy_parent, transform, TMatrix::IDENT);
  if (hierarchy_last_parent != hierarchy_parent)
    hierarchy_parent_last_transform = parentTm;
  hierarchy_transform = inverse(hierarchy_parent_last_transform) * transform;
  hierarchy_parent_last_transform = parentTm;
  hierarchy_last_parent = hierarchy_parent;
}

template <typename Callable>
inline bool perform_hierarchy_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable c);

ECS_AFTER(hierarchy_attached_entity_free_transform_es)
static void hierarchy_attached_entity_transform_es(const ecs::UpdateStageInfoAct &, ecs::EntityManager &manager, ecs::EntityId eid,
  ecs::EidList &hierarchy_transformation)
{
  ecs::EntityId lastParentEid, lastChildEid;
  TMatrix parentTm = TMatrix::IDENT, lastChildTm = TMatrix::IDENT;
  bool hasWorkingLinks = false;
  for (auto i = hierarchy_transformation.begin(), e = hierarchy_transformation.end(); i != e; ++i)
  {
    auto id = *i;
    if (!perform_hierarchy_ecs_query(manager, id,
          [&](ecs::EntityId hierarchy_eid, ecs::EntityId hierarchy_parent, TMatrix &hierarchy_transform, TMatrix &transform,
            ecs::EntityId &hierarchy_last_parent, TMatrix *hierarchy_parent_last_transform) {
            if (hierarchy_eid != eid || !manager.doesEntityExist(hierarchy_parent))
            {
              hierarchy_transformation.erase(i);
              --e;
              --i;
              return;
            }

            if (hierarchy_parent && (hierarchy_parent == lastParentEid || hierarchy_parent == lastChildEid))
            {
              // optimizes chains and multiple connections to same parent
              if (hierarchy_parent == lastChildEid)
              {
                parentTm = lastChildTm;
                lastParentEid = lastChildEid;
              }
            }

            if (hierarchy_parent != lastParentEid)
            {
              // this is small optimization for most likely cases, saves one get
              parentTm = ECS_GET_OR_MGR(manager, hierarchy_parent, transform, TMatrix::IDENT);
              lastParentEid = hierarchy_parent;
            }
            if (hierarchy_last_parent != hierarchy_parent)
              hierarchy_transform = inverse(hierarchy_parent_last_transform ? *hierarchy_parent_last_transform : parentTm) * transform;
            lastChildTm = parentTm * hierarchy_transform;
            transform = lastChildTm;
            if (hierarchy_parent_last_transform)
              *hierarchy_parent_last_transform = parentTm;
            lastChildEid = id;
            hasWorkingLinks = true;
          }))
    {
      hierarchy_transformation.erase(i);
      --e;
      --i;
    }
  }
  if (!hasWorkingLinks)
    manager.destroyEntity(eid);
}

template <typename Callable>
static inline void collect_to_resolve_hierarchy_ecs_query(Callable);

template <typename Callable>
static inline void resolve_hierarchy_ecs_query(Callable);

template <typename Callable>
static inline void reresolve_hierarchy_ecs_query(Callable);

static inline void hierarchy_resolve_hierarchical_entities_es(const ecs::EventOnLocalSceneEntitiesCreated &,
  ecs::EntityManager &manager)
{
  ska::flat_hash_map<int, ecs::EntityId, eastl::hash<int>, eastl::equal_to<int>, framemem_allocator> hiearchy_unresolved_map;

  collect_to_resolve_hierarchy_ecs_query([&](ecs::EntityId eid, int &hierarchy_unresolved_id) {
    if (hierarchy_unresolved_id != 0)
    {
      hiearchy_unresolved_map.insert({hierarchy_unresolved_id, eid});
      hierarchy_unresolved_id = 0;
    }
  });

  // Resolve the parent IDs.
  dag::Vector<ecs::EntityId, framemem_allocator> eidsToReAttach;
  resolve_hierarchy_ecs_query([&](ecs::EntityId eid, int &hierarchy_unresolved_parent_id, ecs::EntityId &hierarchy_parent) {
    if (hierarchy_unresolved_parent_id != 0)
    {
      auto it = hiearchy_unresolved_map.find(hierarchy_unresolved_parent_id);
      if (it != hiearchy_unresolved_map.end())
      {
        hierarchy_parent = it->second;
        eidsToReAttach.push_back(eid);
      }
    }
  });

  // Ensure that all attach_entity_to_hierarchy_es() ran.
  for (auto eid : eidsToReAttach)
    manager.sendEventImmediate(eid, EventAttachToHierarchy());

  // Reresolve the parent IDs because add_entity_hierarchy() could have reset it.
  reresolve_hierarchy_ecs_query([&](int &hierarchy_unresolved_parent_id, ecs::EntityId &hierarchy_parent) {
    if (hierarchy_unresolved_parent_id != 0)
    {
      auto it = hiearchy_unresolved_map.find(hierarchy_unresolved_parent_id);
      if (it != hiearchy_unresolved_map.end())
        hierarchy_parent = it->second;

      hierarchy_unresolved_parent_id = 0;
    }
  });

  // Set hierarchial_free_transform for those entities that had it.
  // See the comment in EntityObjEditor::saveObjectsInternal() why manual template addding is needed.
  for (auto it = hiearchy_unresolved_map.begin(); it != hiearchy_unresolved_map.end(); ++it)
    if ((it->first & 1) == 1) // Odd unresolved ID means that this entity had free transform.
      add_sub_template_async(it->second, "hierarchial_free_transform");
}
