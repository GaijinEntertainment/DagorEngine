#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t animchar_visbits_get_type();
static ecs::LTComponentList animchar_visbits_component(ECS_HASH("animchar_visbits"), animchar_visbits_get_type(), "prog/daNetGameLibs/hero_cockpit/render/renderHeroCockpitES.cpp.inl", "filter_hero_cockpit_es", 0);
// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "renderHeroCockpitES.cpp.inl"
ECS_DEF_PULL_VAR(renderHeroCockpit);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc prepare_hero_cockpit_es_comps[] =
{
//start of 4 ro components at [0]
  {ECS_HASH("aim_data__gunEid"), ecs::ComponentTypeInfo<ecs::EntityId>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("aim_data__lensNodeId"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("aim_data__lensRenderEnabled"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("camera__active"), ecs::ComponentTypeInfo<bool>()}
};
static void prepare_hero_cockpit_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(ECS_RO_COMP(prepare_hero_cockpit_es_comps, "camera__active", bool)) )
      continue;
    prepare_hero_cockpit_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
          , ECS_RO_COMP_OR(prepare_hero_cockpit_es_comps, "aim_data__gunEid", ecs::EntityId(ecs::INVALID_ENTITY_ID))
      , ECS_RO_COMP_OR(prepare_hero_cockpit_es_comps, "aim_data__lensNodeId", int(-1))
      , ECS_RO_COMP_OR(prepare_hero_cockpit_es_comps, "aim_data__lensRenderEnabled", bool(false))
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc prepare_hero_cockpit_es_es_desc
(
  "prepare_hero_cockpit_es",
  "prog/daNetGameLibs/hero_cockpit/render/renderHeroCockpitES.cpp.inl",
  ecs::EntitySystemOps(nullptr, prepare_hero_cockpit_es_all_events),
  empty_span(),
  make_span(prepare_hero_cockpit_es_comps+0, 4)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,"prepare_aim_occlusion_es","reset_occlusion_data_es");
static constexpr ecs::ComponentDesc filter_hero_cockpit_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("hero_cockpit_entities"), ecs::ComponentTypeInfo<ecs::EidList>()}
};
static void filter_hero_cockpit_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    filter_hero_cockpit_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(filter_hero_cockpit_es_comps, "hero_cockpit_entities", ecs::EidList)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc filter_hero_cockpit_es_es_desc
(
  "filter_hero_cockpit_es",
  "prog/daNetGameLibs/hero_cockpit/render/renderHeroCockpitES.cpp.inl",
  ecs::EntitySystemOps(nullptr, filter_hero_cockpit_es_all_events),
  make_span(filter_hero_cockpit_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,nullptr,"animchar_before_render_es");
static constexpr ecs::ComponentDesc init_hero_cockpit_nodes_es_event_handler_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("hero_cockpit_early_prepass_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("hero_cockpit_late_prepass_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("hero_cockpit_colorpass_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("prepare_hero_cockpit_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()}
};
static void init_hero_cockpit_nodes_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_hero_cockpit_nodes_es_event_handler(evt
        , ECS_RW_COMP(init_hero_cockpit_nodes_es_event_handler_comps, "hero_cockpit_early_prepass_node", dafg::NodeHandle)
    , ECS_RW_COMP(init_hero_cockpit_nodes_es_event_handler_comps, "hero_cockpit_late_prepass_node", dafg::NodeHandle)
    , ECS_RW_COMP(init_hero_cockpit_nodes_es_event_handler_comps, "hero_cockpit_colorpass_node", dafg::NodeHandle)
    , ECS_RW_COMP(init_hero_cockpit_nodes_es_event_handler_comps, "prepare_hero_cockpit_node", dafg::NodeHandle)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_hero_cockpit_nodes_es_event_handler_es_desc
(
  "init_hero_cockpit_nodes_es",
  "prog/daNetGameLibs/hero_cockpit/render/renderHeroCockpitES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_hero_cockpit_nodes_es_event_handler_all_events),
  make_span(init_hero_cockpit_nodes_es_event_handler_comps+0, 4)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<BeforeLoadLevel>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc get_animchar_draw_info_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<animchar_visbits_t>()},
//start of 1 ro components at [2]
  {ECS_HASH("additional_data"), ecs::ComponentTypeInfo<ecs::Point4List>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc get_animchar_draw_info_ecs_query_desc
(
  "get_animchar_draw_info_ecs_query",
  make_span(get_animchar_draw_info_ecs_query_comps+0, 2)/*rw*/,
  make_span(get_animchar_draw_info_ecs_query_comps+2, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_animchar_draw_info_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, get_animchar_draw_info_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(get_animchar_draw_info_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            , ECS_RW_COMP(get_animchar_draw_info_ecs_query_comps, "animchar_visbits", animchar_visbits_t)
            , ECS_RO_COMP_PTR(get_animchar_draw_info_ecs_query_comps, "additional_data", ecs::Point4List)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc get_hero_cockpit_entities_const_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("hero_cockpit_entities"), ecs::ComponentTypeInfo<ecs::EidList>()}
};
static ecs::CompileTimeQueryDesc get_hero_cockpit_entities_const_ecs_query_desc
(
  "get_hero_cockpit_entities_const_ecs_query",
  empty_span(),
  make_span(get_hero_cockpit_entities_const_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_hero_cockpit_entities_const_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_hero_cockpit_entities_const_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_hero_cockpit_entities_const_ecs_query_comps, "hero_cockpit_entities", ecs::EidList)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc set_visbits_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<animchar_visbits_t>()}
};
static ecs::CompileTimeQueryDesc set_visbits_ecs_query_desc
(
  "set_visbits_ecs_query",
  make_span(set_visbits_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void set_visbits_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, set_visbits_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(set_visbits_ecs_query_comps, "animchar_visbits", animchar_visbits_t)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc in_vehicle_cockpit_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("cockpit__isHeroInCockpit"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [1]
  {ECS_HASH("vehicleWithWatched"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc in_vehicle_cockpit_ecs_query_desc
(
  "in_vehicle_cockpit_ecs_query",
  empty_span(),
  make_span(in_vehicle_cockpit_ecs_query_comps+0, 1)/*ro*/,
  make_span(in_vehicle_cockpit_ecs_query_comps+1, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void in_vehicle_cockpit_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, in_vehicle_cockpit_ecs_query_desc.getHandle(),
    ecs::stoppable_query_cb_t([&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if (function(
              ECS_RO_COMP(in_vehicle_cockpit_ecs_query_comps, "cockpit__isHeroInCockpit", bool)
            ) == ecs::QueryCbResult::Stop)
            return ecs::QueryCbResult::Stop;
        }while (++comp != compE);
          return ecs::QueryCbResult::Continue;
    })
  );
}
static constexpr ecs::ComponentDesc is_hero_draw_above_geometry_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("hero__isDrawAboveGeometry"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc is_hero_draw_above_geometry_ecs_query_desc
(
  "is_hero_draw_above_geometry_ecs_query",
  empty_span(),
  make_span(is_hero_draw_above_geometry_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void is_hero_draw_above_geometry_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, is_hero_draw_above_geometry_ecs_query_desc.getHandle(),
    ecs::stoppable_query_cb_t([&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if (function(
              ECS_RO_COMP(is_hero_draw_above_geometry_ecs_query_comps, "hero__isDrawAboveGeometry", bool)
            ) == ecs::QueryCbResult::Stop)
            return ecs::QueryCbResult::Stop;
        }while (++comp != compE);
          return ecs::QueryCbResult::Continue;
    })
  );
}
static constexpr ecs::ComponentDesc is_gun_intersect_static_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("hero_cockpit__intersected"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc is_gun_intersect_static_ecs_query_desc
(
  "is_gun_intersect_static_ecs_query",
  empty_span(),
  make_span(is_gun_intersect_static_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void is_gun_intersect_static_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, is_gun_intersect_static_ecs_query_desc.getHandle(),
    ecs::stoppable_query_cb_t([&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if (function(
              ECS_RO_COMP(is_gun_intersect_static_ecs_query_comps, "hero_cockpit__intersected", bool)
            ) == ecs::QueryCbResult::Stop)
            return ecs::QueryCbResult::Stop;
        }while (++comp != compE);
          return ecs::QueryCbResult::Continue;
    })
  );
}
static constexpr ecs::ComponentDesc cockpit_camera_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("isHeroCockpitCam"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("camera__active"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc cockpit_camera_ecs_query_desc
(
  "cockpit_camera_ecs_query",
  empty_span(),
  make_span(cockpit_camera_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline ecs::QueryCbResult cockpit_camera_ecs_query(Callable function)
{
  return perform_query(g_entity_mgr, cockpit_camera_ecs_query_desc.getHandle(),
    ecs::stoppable_query_cb_t([&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(ECS_RO_COMP(cockpit_camera_ecs_query_comps, "camera__active", bool)) )
            continue;
          if (function(
              ECS_RO_COMP(cockpit_camera_ecs_query_comps, "isHeroCockpitCam", bool)
            ) == ecs::QueryCbResult::Stop)
            return ecs::QueryCbResult::Stop;
        }while (++comp != compE);
        return ecs::QueryCbResult::Continue;
    })
  );
}
static constexpr ecs::ComponentDesc fill_cockpit_entities_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("hero_cockpit_entities"), ecs::ComponentTypeInfo<ecs::EidList>()},
  {ECS_HASH("render_hero_cockpit_into_early_prepass"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc fill_cockpit_entities_ecs_query_desc
(
  "fill_cockpit_entities_ecs_query",
  make_span(fill_cockpit_entities_ecs_query_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void fill_cockpit_entities_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, fill_cockpit_entities_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(fill_cockpit_entities_ecs_query_comps, "hero_cockpit_entities", ecs::EidList)
            , ECS_RW_COMP(fill_cockpit_entities_ecs_query_comps, "render_hero_cockpit_into_early_prepass", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc gather_cockpit_entities_ecs_query_comps[] =
{
//start of 4 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("isInVehicle"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("attaches_list"), ecs::ComponentTypeInfo<ecs::EidList>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("human_weap__currentGunModEids"), ecs::ComponentTypeInfo<ecs::EidList>(), ecs::CDF_OPTIONAL},
//start of 2 rq components at [4]
  {ECS_HASH("watchedByPlr"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("cockpitEntity"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc gather_cockpit_entities_ecs_query_desc
(
  "gather_cockpit_entities_ecs_query",
  empty_span(),
  make_span(gather_cockpit_entities_ecs_query_comps+0, 4)/*ro*/,
  make_span(gather_cockpit_entities_ecs_query_comps+4, 2)/*rq*/,
  empty_span());
template<typename Callable>
inline void gather_cockpit_entities_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, gather_cockpit_entities_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(gather_cockpit_entities_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP_OR(gather_cockpit_entities_ecs_query_comps, "isInVehicle", bool(false))
            , ECS_RO_COMP_OR(gather_cockpit_entities_ecs_query_comps, "attaches_list", ecs::EidList(ecs::EidList{}))
            , ECS_RO_COMP_OR(gather_cockpit_entities_ecs_query_comps, "human_weap__currentGunModEids", ecs::EidList(ecs::EidList{}))
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc check_gun_type_for_prepass_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("gun__reloadable"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("notRenderIntoPrepass"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("attaches_list"), ecs::ComponentTypeInfo<ecs::EidList>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc check_gun_type_for_prepass_ecs_query_desc
(
  "check_gun_type_for_prepass_ecs_query",
  empty_span(),
  make_span(check_gun_type_for_prepass_ecs_query_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void check_gun_type_for_prepass_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, check_gun_type_for_prepass_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP_OR(check_gun_type_for_prepass_ecs_query_comps, "gun__reloadable", bool(false))
            , ECS_RO_COMP_OR(check_gun_type_for_prepass_ecs_query_comps, "notRenderIntoPrepass", bool(false))
            , ECS_RO_COMP_OR(check_gun_type_for_prepass_ecs_query_comps, "attaches_list", ecs::EidList(ecs::EidList{}))
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc render_depth_prepass_hero_cockpit_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("hero_cockpit_entities"), ecs::ComponentTypeInfo<ecs::EidList>()},
//start of 1 ro components at [1]
  {ECS_HASH("render_hero_cockpit_into_early_prepass"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc render_depth_prepass_hero_cockpit_ecs_query_desc
(
  "render_depth_prepass_hero_cockpit_ecs_query",
  make_span(render_depth_prepass_hero_cockpit_ecs_query_comps+0, 1)/*rw*/,
  make_span(render_depth_prepass_hero_cockpit_ecs_query_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void render_depth_prepass_hero_cockpit_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, render_depth_prepass_hero_cockpit_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(render_depth_prepass_hero_cockpit_ecs_query_comps, "hero_cockpit_entities", ecs::EidList)
            , ECS_RO_COMP(render_depth_prepass_hero_cockpit_ecs_query_comps, "render_hero_cockpit_into_early_prepass", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::component_t animchar_visbits_get_type(){return ecs::ComponentTypeInfo<unsigned short>::type; }
