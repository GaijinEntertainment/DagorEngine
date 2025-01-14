#include "shadowsManagerES.cpp.inl"
ECS_DEF_PULL_VAR(shadowsManager);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc shadows_settings_tracking_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("render_settings__ssssQuality"), ecs::ComponentTypeInfo<ecs::string>()},
//start of 3 rq components at [1]
  {ECS_HASH("render_settings__combinedShadows"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("render_settings__forwardRendering"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("render_settings__waterQuality"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void shadows_settings_tracking_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    shadows_settings_tracking_es(evt
        , ECS_RO_COMP(shadows_settings_tracking_es_comps, "render_settings__ssssQuality", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc shadows_settings_tracking_es_es_desc
(
  "shadows_settings_tracking_es",
  "prog/daNetGame/render/world/shadowsManagerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, shadows_settings_tracking_es_all_events),
  empty_span(),
  make_span(shadows_settings_tracking_es_comps+0, 1)/*ro*/,
  make_span(shadows_settings_tracking_es_comps+1, 3)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<OnRenderSettingsReady>::build(),
  0
,nullptr,"render_settings__combinedShadows,render_settings__forwardRendering,render_settings__ssssQuality,render_settings__waterQuality",nullptr,"ssss_settings_tracking_es");
static constexpr ecs::ComponentDesc update_world_bbox_es_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("ri_extra__bboxMin"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("ri_extra__bboxMax"), ecs::ComponentTypeInfo<Point3>()}
};
static void update_world_bbox_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    update_world_bbox_es(evt
        , ECS_RO_COMP(update_world_bbox_es_comps, "transform", TMatrix)
    , ECS_RO_COMP(update_world_bbox_es_comps, "ri_extra__bboxMin", Point3)
    , ECS_RO_COMP(update_world_bbox_es_comps, "ri_extra__bboxMax", Point3)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_world_bbox_es_es_desc
(
  "update_world_bbox_es",
  "prog/daNetGame/render/world/shadowsManagerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_world_bbox_es_all_events),
  empty_span(),
  make_span(update_world_bbox_es_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventRendinstsLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,nullptr,nullptr,nullptr,"rendinst_move_es,rendinst_with_handle_move_es");
static constexpr ecs::ComponentDesc init_shadows_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("render_settings__shadowsQuality"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void init_shadows_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  init_shadows_es(evt
        );
}
static ecs::EntitySystemDesc init_shadows_es_es_desc
(
  "init_shadows_es",
  "prog/daNetGame/render/world/shadowsManagerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_shadows_es_all_events),
  empty_span(),
  empty_span(),
  make_span(init_shadows_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<OnRenderSettingsReady>::build(),
  0
,"render","render_settings__shadowsQuality");
static constexpr ecs::ComponentDesc use_rgba_fmt_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("combined_shadows__use_additional_textures"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc use_rgba_fmt_ecs_query_desc
(
  "use_rgba_fmt_ecs_query",
  empty_span(),
  make_span(use_rgba_fmt_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void use_rgba_fmt_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, use_rgba_fmt_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(use_rgba_fmt_ecs_query_comps, "combined_shadows__use_additional_textures", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc bind_additional_textures_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("combined_shadows__use_additional_textures"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("combined_shadows__additional_textures"), ecs::ComponentTypeInfo<ecs::StringList>()},
  {ECS_HASH("combined_shadows__additional_samplers"), ecs::ComponentTypeInfo<ecs::StringList>()}
};
static ecs::CompileTimeQueryDesc bind_additional_textures_ecs_query_desc
(
  "bind_additional_textures_ecs_query",
  empty_span(),
  make_span(bind_additional_textures_ecs_query_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void bind_additional_textures_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, bind_additional_textures_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(bind_additional_textures_ecs_query_comps, "combined_shadows__use_additional_textures", bool)
            , ECS_RO_COMP(bind_additional_textures_ecs_query_comps, "combined_shadows__additional_textures", ecs::StringList)
            , ECS_RO_COMP(bind_additional_textures_ecs_query_comps, "combined_shadows__additional_samplers", ecs::StringList)
            );

        }while (++comp != compE);
    }
  );
}
