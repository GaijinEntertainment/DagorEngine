#include "riexES.cpp.inl"
ECS_DEF_PULL_VAR(riex);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc riex_object_group_process_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__riex_manager"), ecs::ComponentTypeInfo<dagdp::RiexManager>()}
};
static void riex_object_group_process_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<dagdp::EventObjectGroupProcess>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dagdp::riex_object_group_process_es(static_cast<const dagdp::EventObjectGroupProcess&>(evt)
        , ECS_RW_COMP(riex_object_group_process_es_comps, "dagdp__riex_manager", dagdp::RiexManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc riex_object_group_process_es_es_desc
(
  "riex_object_group_process_es",
  "prog/daNetGameLibs/daGdp/render/objects/riexES.cpp.inl",
  ecs::EntitySystemOps(nullptr, riex_object_group_process_es_all_events),
  make_span(riex_object_group_process_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<dagdp::EventObjectGroupProcess>::build(),
  0
,nullptr,nullptr,"*");
static constexpr ecs::ComponentDesc riex_view_finalize_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__riex_manager"), ecs::ComponentTypeInfo<dagdp::RiexManager>()}
};
static void riex_view_finalize_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<dagdp::EventViewFinalize>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dagdp::riex_view_finalize_es(static_cast<const dagdp::EventViewFinalize&>(evt)
        , ECS_RW_COMP(riex_view_finalize_es_comps, "dagdp__riex_manager", dagdp::RiexManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc riex_view_finalize_es_es_desc
(
  "riex_view_finalize_es",
  "prog/daNetGameLibs/daGdp/render/objects/riexES.cpp.inl",
  ecs::EntitySystemOps(nullptr, riex_view_finalize_es_all_events),
  make_span(riex_view_finalize_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<dagdp::EventViewFinalize>::build(),
  0
,nullptr,nullptr,"*");
static constexpr ecs::ComponentDesc riex_finalize_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__riex_manager"), ecs::ComponentTypeInfo<dagdp::RiexManager>()}
};
static void riex_finalize_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<dagdp::EventFinalize>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dagdp::riex_finalize_es(static_cast<const dagdp::EventFinalize&>(evt)
        , ECS_RW_COMP(riex_finalize_es_comps, "dagdp__riex_manager", dagdp::RiexManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc riex_finalize_es_es_desc
(
  "riex_finalize_es",
  "prog/daNetGameLibs/daGdp/render/objects/riexES.cpp.inl",
  ecs::EntitySystemOps(nullptr, riex_finalize_es_all_events),
  make_span(riex_finalize_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<dagdp::EventFinalize>::build(),
  0
,nullptr,nullptr,"*");
static constexpr ecs::ComponentDesc riex_invalidate_views_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__riex_manager"), ecs::ComponentTypeInfo<dagdp::RiexManager>()}
};
static void riex_invalidate_views_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<dagdp::EventInvalidateViews>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dagdp::riex_invalidate_views_es(static_cast<const dagdp::EventInvalidateViews&>(evt)
        , ECS_RW_COMP(riex_invalidate_views_es_comps, "dagdp__riex_manager", dagdp::RiexManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc riex_invalidate_views_es_es_desc
(
  "riex_invalidate_views_es",
  "prog/daNetGameLibs/daGdp/render/objects/riexES.cpp.inl",
  ecs::EntitySystemOps(nullptr, riex_invalidate_views_es_all_events),
  make_span(riex_invalidate_views_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<dagdp::EventInvalidateViews>::build(),
  0
,nullptr,nullptr,"*");
static constexpr ecs::ComponentDesc dagdp_object_group_riex_changed_es_comps[] =
{
//start of 4 rq components at [0]
  {ECS_HASH("dagdp__assets"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("dagdp__params"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("dagdp__name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("dagdp_object_group_riex"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void dagdp_object_group_riex_changed_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  dagdp::dagdp_object_group_riex_changed_es(evt
        );
}
static ecs::EntitySystemDesc dagdp_object_group_riex_changed_es_es_desc
(
  "dagdp_object_group_riex_changed_es",
  "prog/daNetGameLibs/daGdp/render/objects/riexES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dagdp_object_group_riex_changed_es_all_events),
  empty_span(),
  empty_span(),
  make_span(dagdp_object_group_riex_changed_es_comps+0, 4)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear,
                       ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render","dagdp__assets,dagdp__name,dagdp__params");
static constexpr ecs::ComponentDesc riex_object_group_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("dagdp__assets"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("dagdp__params"), ecs::ComponentTypeInfo<ecs::Object>()},
//start of 1 rq components at [3]
  {ECS_HASH("dagdp_object_group_riex"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc riex_object_group_ecs_query_desc
(
  "dagdp::riex_object_group_ecs_query",
  empty_span(),
  make_span(riex_object_group_ecs_query_comps+0, 3)/*ro*/,
  make_span(riex_object_group_ecs_query_comps+3, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void dagdp::riex_object_group_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, riex_object_group_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(riex_object_group_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(riex_object_group_ecs_query_comps, "dagdp__assets", ecs::Object)
            , ECS_RO_COMP(riex_object_group_ecs_query_comps, "dagdp__params", ecs::Object)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc manager_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__global_manager"), ecs::ComponentTypeInfo<dagdp::GlobalManager>()}
};
static ecs::CompileTimeQueryDesc manager_ecs_query_desc
(
  "dagdp::manager_ecs_query",
  make_span(manager_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void dagdp::manager_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, manager_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(manager_ecs_query_comps, "dagdp__global_manager", dagdp::GlobalManager)
            );

        }while (++comp != compE);
    }
  );
}
