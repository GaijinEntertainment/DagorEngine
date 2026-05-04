#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t animchar_visbits_get_type();
static ecs::LTComponentList animchar_visbits_component(ECS_HASH("animchar_visbits"), animchar_visbits_get_type(), "prog/daNetGameLibs/hero_cockpit_render/render/renderHeroCockpitES.cpp.inl", "filter_hero_cockpit_es", 0);
// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "renderHeroCockpitES.cpp.inl"
ECS_DEF_PULL_VAR(renderHeroCockpit);
#include <daECS/core/internal/performQuery.h>
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
  "prog/daNetGameLibs/hero_cockpit_render/render/renderHeroCockpitES.cpp.inl",
  ecs::EntitySystemOps(nullptr, filter_hero_cockpit_es_all_events),
  make_span(filter_hero_cockpit_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,nullptr,"animchar_before_render_es");
static constexpr ecs::ComponentDesc mark_hero_cockpit_textures_important_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("hero_cockpit_entities"), ecs::ComponentTypeInfo<ecs::EidList>()}
};
static void mark_hero_cockpit_textures_important_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    mark_hero_cockpit_textures_important_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , components.manager()
    , ECS_RO_COMP(mark_hero_cockpit_textures_important_es_comps, "hero_cockpit_entities", ecs::EidList)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc mark_hero_cockpit_textures_important_es_es_desc
(
  "mark_hero_cockpit_textures_important_es",
  "prog/daNetGameLibs/hero_cockpit_render/render/renderHeroCockpitES.cpp.inl",
  ecs::EntitySystemOps(nullptr, mark_hero_cockpit_textures_important_es_all_events),
  empty_span(),
  make_span(mark_hero_cockpit_textures_important_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,nullptr,"filter_hero_cockpit_es");
static constexpr ecs::ComponentDesc set_hero_cockpit_params_es_comps[] =
{
//start of 7 ro components at [0]
  {ECS_HASH("hero_cockpit_vec"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("hero_cockpit_vec_value"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("hero_cockpit_camera_to_point"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("hero_cockpit_camera_to_point_value"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("hero_cockpit_bbox_min_value"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("hero_cockpit_bbox_max_value"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("hero_cockpit_camera_enabled_value"), ecs::ComponentTypeInfo<int>()}
};
static void set_hero_cockpit_params_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    set_hero_cockpit_params_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RO_COMP(set_hero_cockpit_params_es_comps, "hero_cockpit_vec", ShaderVar)
    , ECS_RO_COMP(set_hero_cockpit_params_es_comps, "hero_cockpit_vec_value", Point4)
    , ECS_RO_COMP(set_hero_cockpit_params_es_comps, "hero_cockpit_camera_to_point", ShaderVar)
    , ECS_RO_COMP(set_hero_cockpit_params_es_comps, "hero_cockpit_camera_to_point_value", Point4)
    , ECS_RO_COMP(set_hero_cockpit_params_es_comps, "hero_cockpit_bbox_min_value", Point4)
    , ECS_RO_COMP(set_hero_cockpit_params_es_comps, "hero_cockpit_bbox_max_value", Point4)
    , ECS_RO_COMP(set_hero_cockpit_params_es_comps, "hero_cockpit_camera_enabled_value", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc set_hero_cockpit_params_es_es_desc
(
  "set_hero_cockpit_params_es",
  "prog/daNetGameLibs/hero_cockpit_render/render/renderHeroCockpitES.cpp.inl",
  ecs::EntitySystemOps(nullptr, set_hero_cockpit_params_es_all_events),
  empty_span(),
  make_span(set_hero_cockpit_params_es_comps+0, 7)/*ro*/,
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
  "prog/daNetGameLibs/hero_cockpit_render/render/renderHeroCockpitES.cpp.inl",
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
inline void get_animchar_draw_info_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable function)
{
  perform_query(&manager, eid, get_animchar_draw_info_ecs_query_desc.getHandle(),
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
inline void get_hero_cockpit_entities_const_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, get_hero_cockpit_entities_const_ecs_query_desc.getHandle(),
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
inline void set_visbits_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable function)
{
  perform_query(&manager, eid, set_visbits_ecs_query_desc.getHandle(),
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
inline void render_depth_prepass_hero_cockpit_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, render_depth_prepass_hero_cockpit_ecs_query_desc.getHandle(),
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
static constexpr ecs::ComponentDesc fill_hero_cockpit_textures_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()}
};
static ecs::CompileTimeQueryDesc fill_hero_cockpit_textures_ecs_query_desc
(
  "fill_hero_cockpit_textures_ecs_query",
  make_span(fill_hero_cockpit_textures_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void fill_hero_cockpit_textures_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable function)
{
  perform_query(&manager, eid, fill_hero_cockpit_textures_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(fill_hero_cockpit_textures_ecs_query_comps, "animchar_render", AnimV20::AnimcharRendComponent)
            );

        }
    }
  );
}
static constexpr ecs::component_t animchar_visbits_get_type(){return ecs::ComponentTypeInfo<unsigned int>::type; }
