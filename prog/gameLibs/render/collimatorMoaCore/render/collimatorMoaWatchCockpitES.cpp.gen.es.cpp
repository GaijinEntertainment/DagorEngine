#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t collimator_moa_render__relem_update_required_get_type();
static ecs::LTComponentList collimator_moa_render__relem_update_required_component(ECS_HASH("collimator_moa_render__relem_update_required"), collimator_moa_render__relem_update_required_get_type(), "prog/gameLibs/render/collimatorMoaCore/render/collimatorMoaWatchCockpitES.cpp.inl", "collimator_moa_toggle_update_es_event_handler", 0);
// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "collimatorMoaWatchCockpitES.cpp.inl"
ECS_DEF_PULL_VAR(collimatorMoaWatchCockpit);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc collimator_moa_toggle_update_es_event_handler_comps[] =
{
//start of 3 rq components at [0]
  {ECS_HASH("human_weap__currentGunModEids"), ecs::ComponentTypeInfo<ecs::EidList>()},
  {ECS_HASH("watchedByPlr"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("cockpitEntity"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void collimator_moa_toggle_update_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  collimator_moa_toggle_update_es_event_handler(evt
        );
}
static ecs::EntitySystemDesc collimator_moa_toggle_update_es_event_handler_es_desc
(
  "collimator_moa_toggle_update_es",
  "prog/gameLibs/render/collimatorMoaCore/render/collimatorMoaWatchCockpitES.cpp.inl",
  ecs::EntitySystemOps(nullptr, collimator_moa_toggle_update_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  make_span(collimator_moa_toggle_update_es_event_handler_comps+0, 3)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","human_weap__currentGunModEids");
static constexpr ecs::ComponentDesc collimator_moa_update_lens_eids_es_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("collimator_moa_render__gun_mod_eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("collimator_moa_render__rigid_id"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("collimator_moa_render__relem_id"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("collimator_moa_render__relem_update_required"), ecs::ComponentTypeInfo<bool>()}
};
static void collimator_moa_update_lens_eids_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    collimator_moa_update_lens_eids_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(collimator_moa_update_lens_eids_es_comps, "collimator_moa_render__gun_mod_eid", ecs::EntityId)
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
  make_span(collimator_moa_update_lens_eids_es_comps+0, 4)/*rw*/,
  empty_span(),
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
inline void gather_collimator_moa_lens_info_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, gather_collimator_moa_lens_info_ecs_query_desc.getHandle(),
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
static constexpr ecs::ComponentDesc gather_weapon_template_name_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("human_weap__currentGunSlot"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("human_weap__weapTemplates"), ecs::ComponentTypeInfo<ecs::Object>()}
};
static ecs::CompileTimeQueryDesc gather_weapon_template_name_ecs_query_desc
(
  "gather_weapon_template_name_ecs_query",
  empty_span(),
  make_span(gather_weapon_template_name_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void gather_weapon_template_name_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, gather_weapon_template_name_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(gather_weapon_template_name_ecs_query_comps, "human_weap__currentGunSlot", int)
            , ECS_RO_COMP(gather_weapon_template_name_ecs_query_comps, "human_weap__weapTemplates", ecs::Object)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc count_cockpit_entities_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 3 rq components at [1]
  {ECS_HASH("human_weap__currentGunModEids"), ecs::ComponentTypeInfo<ecs::EidList>()},
  {ECS_HASH("watchedByPlr"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("cockpitEntity"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc count_cockpit_entities_ecs_query_desc
(
  "count_cockpit_entities_ecs_query",
  empty_span(),
  make_span(count_cockpit_entities_ecs_query_comps+0, 1)/*ro*/,
  make_span(count_cockpit_entities_ecs_query_comps+1, 3)/*rq*/,
  empty_span());
template<typename Callable>
inline void count_cockpit_entities_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, count_cockpit_entities_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(count_cockpit_entities_ecs_query_comps, "eid", ecs::EntityId)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc gather_gun_mod_list_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("human_weap__currentGunModEids"), ecs::ComponentTypeInfo<ecs::EidList>()},
//start of 2 rq components at [2]
  {ECS_HASH("watchedByPlr"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("cockpitEntity"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc gather_gun_mod_list_ecs_query_desc
(
  "gather_gun_mod_list_ecs_query",
  empty_span(),
  make_span(gather_gun_mod_list_ecs_query_comps+0, 2)/*ro*/,
  make_span(gather_gun_mod_list_ecs_query_comps+2, 2)/*rq*/,
  empty_span());
template<typename Callable>
inline void gather_gun_mod_list_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, gather_gun_mod_list_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(gather_gun_mod_list_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(gather_gun_mod_list_ecs_query_comps, "human_weap__currentGunModEids", ecs::EidList)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::component_t collimator_moa_render__relem_update_required_get_type(){return ecs::ComponentTypeInfo<bool>::type; }
