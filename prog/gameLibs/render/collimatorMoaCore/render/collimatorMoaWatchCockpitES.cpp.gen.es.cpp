// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "collimatorMoaWatchCockpitES.cpp.inl"
ECS_DEF_PULL_VAR(collimatorMoaWatchCockpit);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc collimator_moa_update_lens_eids_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("collimator_moa_render__rigid_id"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("collimator_moa_render__relem_id"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("collimator_moa_render__relem_update_required"), ecs::ComponentTypeInfo<bool>()},
//start of 1 ro components at [3]
  {ECS_HASH("collimator_moa_render__gun_mod_eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void collimator_moa_update_lens_eids_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    collimator_moa_update_lens_eids_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , components.manager()
    , ECS_RO_COMP(collimator_moa_update_lens_eids_es_comps, "collimator_moa_render__gun_mod_eid", ecs::EntityId)
    , ECS_RW_COMP(collimator_moa_update_lens_eids_es_comps, "collimator_moa_render__rigid_id", int)
    , ECS_RW_COMP(collimator_moa_update_lens_eids_es_comps, "collimator_moa_render__relem_id", int)
    , ECS_RW_COMP(collimator_moa_update_lens_eids_es_comps, "collimator_moa_render__relem_update_required", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc collimator_moa_update_lens_eids_es_es_desc
(
  "collimator_moa_update_lens_eids_es",
  "prog/gameLibs/render/collimatorMoaCore/render/collimatorMoaWatchCockpitES.cpp.inl",
  ecs::EntitySystemOps(nullptr, collimator_moa_update_lens_eids_es_all_events),
  make_span(collimator_moa_update_lens_eids_es_comps+0, 3)/*rw*/,
  make_span(collimator_moa_update_lens_eids_es_comps+3, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc gather_collimator_moa_lens_info_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("animchar__res"), ecs::ComponentTypeInfo<ecs::string>()},
//start of 1 rq components at [2]
  {ECS_HASH("gunScope"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc gather_collimator_moa_lens_info_ecs_query_desc
(
  "gather_collimator_moa_lens_info_ecs_query",
  empty_span(),
  make_span(gather_collimator_moa_lens_info_ecs_query_comps+0, 2)/*ro*/,
  make_span(gather_collimator_moa_lens_info_ecs_query_comps+2, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void gather_collimator_moa_lens_info_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable function)
{
  perform_query(&manager, eid, gather_collimator_moa_lens_info_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(gather_collimator_moa_lens_info_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RO_COMP(gather_collimator_moa_lens_info_ecs_query_comps, "animchar__res", ecs::string)
            );

        }
    }
  );
}
