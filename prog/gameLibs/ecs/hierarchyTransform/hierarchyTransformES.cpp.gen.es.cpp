#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t hierarchy_eid_get_type();
static ecs::LTComponentList hierarchy_eid_component(ECS_HASH("hierarchy_eid"), hierarchy_eid_get_type(), "prog/gameLibs/ecs/hierarchyTransform/hierarchyTransformES.cpp.inl", "attach_entity_to_hierarchy_es", 0);
static constexpr ecs::component_t hierarchy_last_parent_get_type();
static ecs::LTComponentList hierarchy_last_parent_component(ECS_HASH("hierarchy_last_parent"), hierarchy_last_parent_get_type(), "prog/gameLibs/ecs/hierarchyTransform/hierarchyTransformES.cpp.inl", "", 0);
static constexpr ecs::component_t hierarchy_parent_get_type();
static ecs::LTComponentList hierarchy_parent_component(ECS_HASH("hierarchy_parent"), hierarchy_parent_get_type(), "prog/gameLibs/ecs/hierarchyTransform/hierarchyTransformES.cpp.inl", "", 0);
static constexpr ecs::component_t hierarchy_transform_get_type();
static ecs::LTComponentList hierarchy_transform_component(ECS_HASH("hierarchy_transform"), hierarchy_transform_get_type(), "prog/gameLibs/ecs/hierarchyTransform/hierarchyTransformES.cpp.inl", "", 0);
static constexpr ecs::component_t hierarchy_transformation_get_type();
static ecs::LTComponentList hierarchy_transformation_component(ECS_HASH("hierarchy_transformation"), hierarchy_transformation_get_type(), "prog/gameLibs/ecs/hierarchyTransform/hierarchyTransformES.cpp.inl", "attach_entity_to_hierarchy_es", 0);
static constexpr ecs::component_t transform_get_type();
static ecs::LTComponentList transform_component(ECS_HASH("transform"), transform_get_type(), "prog/gameLibs/ecs/hierarchyTransform/hierarchyTransformES.cpp.inl", "perform_hierarchy_ecs_query", 0);
// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "hierarchyTransformES.cpp.inl"
ECS_DEF_PULL_VAR(hierarchyTransform);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc hierarchy_attached_entity_free_transform_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("hierarchy_transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("hierarchy_parent_last_transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("hierarchy_last_parent"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 2 ro components at [3]
  {ECS_HASH("hierarchy_parent"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 1 rq components at [5]
  {ECS_HASH("hierarchy_eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void hierarchy_attached_entity_free_transform_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    hierarchy_attached_entity_free_transform_es(*info.cast<ecs::UpdateStageInfoAct>()
    , components.manager()
    , ECS_RO_COMP(hierarchy_attached_entity_free_transform_es_comps, "hierarchy_parent", ecs::EntityId)
    , ECS_RO_COMP(hierarchy_attached_entity_free_transform_es_comps, "transform", TMatrix)
    , ECS_RW_COMP(hierarchy_attached_entity_free_transform_es_comps, "hierarchy_transform", TMatrix)
    , ECS_RW_COMP(hierarchy_attached_entity_free_transform_es_comps, "hierarchy_parent_last_transform", TMatrix)
    , ECS_RW_COMP(hierarchy_attached_entity_free_transform_es_comps, "hierarchy_last_parent", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc hierarchy_attached_entity_free_transform_es_es_desc
(
  "hierarchy_attached_entity_free_transform_es",
  "prog/gameLibs/ecs/hierarchyTransform/hierarchyTransformES.cpp.inl",
  ecs::EntitySystemOps(hierarchy_attached_entity_free_transform_es_all),
  make_span(hierarchy_attached_entity_free_transform_es_comps+0, 3)/*rw*/,
  make_span(hierarchy_attached_entity_free_transform_es_comps+3, 2)/*ro*/,
  make_span(hierarchy_attached_entity_free_transform_es_comps+5, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,nullptr,nullptr,nullptr,"animchar_non_updatable_es,start_async_phys_sim_es");
static constexpr ecs::ComponentDesc hierarchy_attached_entity_transform_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("hierarchy_transformation"), ecs::ComponentTypeInfo<ecs::EidList>()},
//start of 1 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void hierarchy_attached_entity_transform_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    hierarchy_attached_entity_transform_es(*info.cast<ecs::UpdateStageInfoAct>()
    , components.manager()
    , ECS_RO_COMP(hierarchy_attached_entity_transform_es_comps, "eid", ecs::EntityId)
    , ECS_RW_COMP(hierarchy_attached_entity_transform_es_comps, "hierarchy_transformation", ecs::EidList)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc hierarchy_attached_entity_transform_es_es_desc
(
  "hierarchy_attached_entity_transform_es",
  "prog/gameLibs/ecs/hierarchyTransform/hierarchyTransformES.cpp.inl",
  ecs::EntitySystemOps(hierarchy_attached_entity_transform_es_all),
  make_span(hierarchy_attached_entity_transform_es_comps+0, 1)/*rw*/,
  make_span(hierarchy_attached_entity_transform_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,nullptr,nullptr,nullptr,"hierarchy_attached_entity_free_transform_es");
static constexpr ecs::ComponentDesc attach_entity_to_hierarchy_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("hierarchy_eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("hierarchy_transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("hierarchy_last_parent"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 4 ro components at [3]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("hierarchy_parent"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("hierarchy_transform__set"), ecs::ComponentTypeInfo<ecs::Tag>(), ecs::CDF_OPTIONAL}
};
static void attach_entity_to_hierarchy_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    attach_entity_to_hierarchy_es(evt
        , components.manager()
    , ECS_RO_COMP(attach_entity_to_hierarchy_es_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(attach_entity_to_hierarchy_es_comps, "transform", TMatrix)
    , ECS_RO_COMP(attach_entity_to_hierarchy_es_comps, "hierarchy_parent", ecs::EntityId)
    , ECS_RW_COMP(attach_entity_to_hierarchy_es_comps, "hierarchy_eid", ecs::EntityId)
    , ECS_RW_COMP(attach_entity_to_hierarchy_es_comps, "hierarchy_transform", TMatrix)
    , ECS_RW_COMP(attach_entity_to_hierarchy_es_comps, "hierarchy_last_parent", ecs::EntityId)
    , ECS_RO_COMP_PTR(attach_entity_to_hierarchy_es_comps, "hierarchy_transform__set", ecs::Tag)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc attach_entity_to_hierarchy_es_es_desc
(
  "attach_entity_to_hierarchy_es",
  "prog/gameLibs/ecs/hierarchyTransform/hierarchyTransformES.cpp.inl",
  ecs::EntitySystemOps(nullptr, attach_entity_to_hierarchy_es_all_events),
  make_span(attach_entity_to_hierarchy_es_comps+0, 3)/*rw*/,
  make_span(attach_entity_to_hierarchy_es_comps+3, 4)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventAttachToHierarchy,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,"hierarchy_parent");
static constexpr ecs::ComponentDesc hierarchy_entity_became_free_transform_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("hierarchy_parent_last_transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("hierarchy_last_parent"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 ro components at [2]
  {ECS_HASH("hierarchy_parent"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 3 rq components at [3]
  {ECS_HASH("hierarchy_transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("hierarchy_eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void hierarchy_entity_became_free_transform_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    hierarchy_entity_became_free_transform_es(evt
        , components.manager()
    , ECS_RO_COMP(hierarchy_entity_became_free_transform_es_comps, "hierarchy_parent", ecs::EntityId)
    , ECS_RW_COMP(hierarchy_entity_became_free_transform_es_comps, "hierarchy_parent_last_transform", TMatrix)
    , ECS_RW_COMP(hierarchy_entity_became_free_transform_es_comps, "hierarchy_last_parent", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc hierarchy_entity_became_free_transform_es_es_desc
(
  "hierarchy_entity_became_free_transform_es",
  "prog/gameLibs/ecs/hierarchyTransform/hierarchyTransformES.cpp.inl",
  ecs::EntitySystemOps(nullptr, hierarchy_entity_became_free_transform_es_all_events),
  make_span(hierarchy_entity_became_free_transform_es_comps+0, 2)/*rw*/,
  make_span(hierarchy_entity_became_free_transform_es_comps+2, 1)/*ro*/,
  make_span(hierarchy_entity_became_free_transform_es_comps+3, 3)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
//static constexpr ecs::ComponentDesc hierarchy_resolve_hierarchical_entities_es_comps[] ={};
static void hierarchy_resolve_hierarchical_entities_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ecs::EventOnLocalSceneEntitiesCreated>());
  hierarchy_resolve_hierarchical_entities_es(static_cast<const ecs::EventOnLocalSceneEntitiesCreated&>(evt)
        , components.manager()
    );
}
static ecs::EntitySystemDesc hierarchy_resolve_hierarchical_entities_es_es_desc
(
  "hierarchy_resolve_hierarchical_entities_es",
  "prog/gameLibs/ecs/hierarchyTransform/hierarchyTransformES.cpp.inl",
  ecs::EntitySystemOps(nullptr, hierarchy_resolve_hierarchical_entities_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventOnLocalSceneEntitiesCreated>::build(),
  0
);
static constexpr ecs::ComponentDesc set_hierarchy_ecs_query_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("hierarchy_eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("hierarchy_parent"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("hierarchy_last_parent"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("hierarchy_transform"), ecs::ComponentTypeInfo<TMatrix>()}
};
static ecs::CompileTimeQueryDesc set_hierarchy_ecs_query_desc
(
  "set_hierarchy_ecs_query",
  make_span(set_hierarchy_ecs_query_comps+0, 4)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline bool set_hierarchy_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable function)
{
  return perform_query(&manager, eid, set_hierarchy_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(set_hierarchy_ecs_query_comps, "hierarchy_eid", ecs::EntityId)
            , ECS_RW_COMP(set_hierarchy_ecs_query_comps, "hierarchy_parent", ecs::EntityId)
            , ECS_RW_COMP(set_hierarchy_ecs_query_comps, "hierarchy_last_parent", ecs::EntityId)
            , ECS_RW_COMP(set_hierarchy_ecs_query_comps, "hierarchy_transform", TMatrix)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc perform_hierarchy_ecs_query_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("hierarchy_transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("hierarchy_last_parent"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("hierarchy_parent_last_transform"), ecs::ComponentTypeInfo<TMatrix>(), ecs::CDF_OPTIONAL},
//start of 2 ro components at [4]
  {ECS_HASH("hierarchy_eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("hierarchy_parent"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static ecs::CompileTimeQueryDesc perform_hierarchy_ecs_query_desc
(
  "perform_hierarchy_ecs_query",
  make_span(perform_hierarchy_ecs_query_comps+0, 4)/*rw*/,
  make_span(perform_hierarchy_ecs_query_comps+4, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline bool perform_hierarchy_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable function)
{
  return perform_query(&manager, eid, perform_hierarchy_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(perform_hierarchy_ecs_query_comps, "hierarchy_eid", ecs::EntityId)
            , ECS_RO_COMP(perform_hierarchy_ecs_query_comps, "hierarchy_parent", ecs::EntityId)
            , ECS_RW_COMP(perform_hierarchy_ecs_query_comps, "hierarchy_transform", TMatrix)
            , ECS_RW_COMP(perform_hierarchy_ecs_query_comps, "transform", TMatrix)
            , ECS_RW_COMP(perform_hierarchy_ecs_query_comps, "hierarchy_last_parent", ecs::EntityId)
            , ECS_RW_COMP_PTR(perform_hierarchy_ecs_query_comps, "hierarchy_parent_last_transform", TMatrix)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc collect_to_resolve_hierarchy_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("hierarchy_unresolved_id"), ecs::ComponentTypeInfo<int>()},
//start of 1 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static ecs::CompileTimeQueryDesc collect_to_resolve_hierarchy_ecs_query_desc
(
  "collect_to_resolve_hierarchy_ecs_query",
  make_span(collect_to_resolve_hierarchy_ecs_query_comps+0, 1)/*rw*/,
  make_span(collect_to_resolve_hierarchy_ecs_query_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void collect_to_resolve_hierarchy_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, collect_to_resolve_hierarchy_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(collect_to_resolve_hierarchy_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RW_COMP(collect_to_resolve_hierarchy_ecs_query_comps, "hierarchy_unresolved_id", int)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc resolve_hierarchy_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("hierarchy_unresolved_parent_id"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("hierarchy_parent"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 ro components at [2]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static ecs::CompileTimeQueryDesc resolve_hierarchy_ecs_query_desc
(
  "resolve_hierarchy_ecs_query",
  make_span(resolve_hierarchy_ecs_query_comps+0, 2)/*rw*/,
  make_span(resolve_hierarchy_ecs_query_comps+2, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void resolve_hierarchy_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, resolve_hierarchy_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(resolve_hierarchy_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RW_COMP(resolve_hierarchy_ecs_query_comps, "hierarchy_unresolved_parent_id", int)
            , ECS_RW_COMP(resolve_hierarchy_ecs_query_comps, "hierarchy_parent", ecs::EntityId)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc reresolve_hierarchy_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("hierarchy_unresolved_parent_id"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("hierarchy_parent"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static ecs::CompileTimeQueryDesc reresolve_hierarchy_ecs_query_desc
(
  "reresolve_hierarchy_ecs_query",
  make_span(reresolve_hierarchy_ecs_query_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void reresolve_hierarchy_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, reresolve_hierarchy_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(reresolve_hierarchy_ecs_query_comps, "hierarchy_unresolved_parent_id", int)
            , ECS_RW_COMP(reresolve_hierarchy_ecs_query_comps, "hierarchy_parent", ecs::EntityId)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::component_t hierarchy_eid_get_type(){return ecs::ComponentTypeInfo<ecs::EntityId>::type; }
static constexpr ecs::component_t hierarchy_last_parent_get_type(){return ecs::ComponentTypeInfo<ecs::EntityId>::type; }
static constexpr ecs::component_t hierarchy_parent_get_type(){return ecs::ComponentTypeInfo<ecs::EntityId>::type; }
static constexpr ecs::component_t hierarchy_transform_get_type(){return ecs::ComponentTypeInfo<TMatrix>::type; }
static constexpr ecs::component_t hierarchy_transformation_get_type(){return ecs::ComponentTypeInfo<ecs::EidList>::type; }
static constexpr ecs::component_t transform_get_type(){return ecs::ComponentTypeInfo<TMatrix>::type; }
