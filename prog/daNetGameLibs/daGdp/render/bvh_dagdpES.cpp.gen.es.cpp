// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "bvh_dagdpES.cpp.inl"
ECS_DEF_PULL_VAR(bvh_dagdp);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc bvh_recreate_views_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__bvh_manager"), ecs::ComponentTypeInfo<dagdp::BvhManager>()}
};
static void bvh_recreate_views_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<dagdp::EventRecreateViews>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dagdp::bvh_recreate_views_es(static_cast<const dagdp::EventRecreateViews&>(evt)
        , ECS_RW_COMP(bvh_recreate_views_es_comps, "dagdp__bvh_manager", dagdp::BvhManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc bvh_recreate_views_es_es_desc
(
  "bvh_recreate_views_es",
  "prog/daNetGameLibs/daGdp/render/bvh_dagdpES.cpp.inl",
  ecs::EntitySystemOps(nullptr, bvh_recreate_views_es_all_events),
  make_span(bvh_recreate_views_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<dagdp::EventRecreateViews>::build(),
  0
,nullptr,nullptr,"*");
static constexpr ecs::ComponentDesc bvh_changed_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__global_manager"), ecs::ComponentTypeInfo<dagdp::GlobalManager>()},
//start of 1 rq components at [1]
  {ECS_HASH("dagdp__bvh_manager"), ecs::ComponentTypeInfo<dagdp::BvhManager>()}
};
static void bvh_changed_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<BVHDagdpChanged>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dagdp::bvh_changed_es(static_cast<const BVHDagdpChanged&>(evt)
        , ECS_RW_COMP(bvh_changed_es_comps, "dagdp__global_manager", dagdp::GlobalManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc bvh_changed_es_es_desc
(
  "bvh_changed_es",
  "prog/daNetGameLibs/daGdp/render/bvh_dagdpES.cpp.inl",
  ecs::EntitySystemOps(nullptr, bvh_changed_es_all_events),
  make_span(bvh_changed_es_comps+0, 1)/*rw*/,
  empty_span(),
  make_span(bvh_changed_es_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<BVHDagdpChanged>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc dagdp_bvh_range_update_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__global_manager"), ecs::ComponentTypeInfo<dagdp::GlobalManager>()}
};
static void dagdp_bvh_range_update_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dagdp::dagdp_bvh_range_update_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(dagdp_bvh_range_update_es_comps, "dagdp__global_manager", dagdp::GlobalManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc dagdp_bvh_range_update_es_es_desc
(
  "dagdp_bvh_range_update_es",
  "prog/daNetGameLibs/daGdp/render/bvh_dagdpES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dagdp_bvh_range_update_es_all_events),
  make_span(dagdp_bvh_range_update_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,"dagdp_update_es");
static constexpr ecs::ComponentDesc bvh_manager_appear_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__bvh_manager"), ecs::ComponentTypeInfo<dagdp::BvhManager>()}
};
static void bvh_manager_appear_es_all_events(ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dagdp::bvh_manager_appear_es(evt
        , ECS_RW_COMP(bvh_manager_appear_es_comps, "dagdp__bvh_manager", dagdp::BvhManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc bvh_manager_appear_es_es_desc
(
  "bvh_manager_appear_es",
  "prog/daNetGameLibs/daGdp/render/bvh_dagdpES.cpp.inl",
  ecs::EntitySystemOps(nullptr, bvh_manager_appear_es_all_events),
  make_span(bvh_manager_appear_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc get_bvh_manager_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__bvh_manager"), ecs::ComponentTypeInfo<dagdp::BvhManager>()}
};
static ecs::CompileTimeQueryDesc get_bvh_manager_ecs_query_desc
(
  "dagdp::get_bvh_manager_ecs_query",
  make_span(get_bvh_manager_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void dagdp::get_bvh_manager_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, get_bvh_manager_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(get_bvh_manager_ecs_query_comps, "dagdp__bvh_manager", dagdp::BvhManager)
            );

        }while (++comp != compE);
    }
  );
}
