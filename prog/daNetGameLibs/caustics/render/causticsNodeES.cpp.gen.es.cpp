#include "causticsNodeES.cpp.inl"
ECS_DEF_PULL_VAR(causticsNode);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc caustics_water_quality_changed_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("render_settings__waterQuality"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void caustics_water_quality_changed_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    caustics_water_quality_changed_es(evt
        , ECS_RO_COMP(caustics_water_quality_changed_es_comps, "render_settings__waterQuality", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc caustics_water_quality_changed_es_es_desc
(
  "caustics_water_quality_changed_es",
  "prog/daNetGameLibs/caustics/render/causticsNodeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, caustics_water_quality_changed_es_all_events),
  empty_span(),
  make_span(caustics_water_quality_changed_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnRenderSettingsReady>::build(),
  0
,"render","render_settings__waterQuality");
static constexpr ecs::ComponentDesc caustics_render_features_changed_es_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("caustics__node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("caustics__indoor_probe_mask"), ecs::ComponentTypeInfo<UniqueTexHolder>()},
  {ECS_HASH("needs_water_heightmap"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("combined_shadows__use_additional_textures"), ecs::ComponentTypeInfo<bool>()}
};
static void caustics_render_features_changed_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    caustics_render_features_changed_es(evt
        , ECS_RW_COMP(caustics_render_features_changed_es_comps, "caustics__node", dabfg::NodeHandle)
    , ECS_RW_COMP(caustics_render_features_changed_es_comps, "caustics__indoor_probe_mask", UniqueTexHolder)
    , ECS_RW_COMP(caustics_render_features_changed_es_comps, "needs_water_heightmap", bool)
    , ECS_RW_COMP(caustics_render_features_changed_es_comps, "combined_shadows__use_additional_textures", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc caustics_render_features_changed_es_es_desc
(
  "caustics_render_features_changed_es",
  "prog/daNetGameLibs/caustics/render/causticsNodeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, caustics_render_features_changed_es_all_events),
  make_span(caustics_render_features_changed_es_comps+0, 4)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ChangeRenderFeatures>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc caustics_before_render_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("caustics__node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("caustics__indoor_probe_mask"), ecs::ComponentTypeInfo<UniqueTexHolder>()},
//start of 1 ro components at [2]
  {ECS_HASH("caustics__indoor_probe_shader"), ecs::ComponentTypeInfo<ShadersECS>()}
};
static void caustics_before_render_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    caustics_before_render_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(caustics_before_render_es_comps, "caustics__node", dabfg::NodeHandle)
    , ECS_RW_COMP(caustics_before_render_es_comps, "caustics__indoor_probe_mask", UniqueTexHolder)
    , ECS_RO_COMP(caustics_before_render_es_comps, "caustics__indoor_probe_shader", ShadersECS)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc caustics_before_render_es_es_desc
(
  "caustics_before_render_es",
  "prog/daNetGameLibs/caustics/render/causticsNodeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, caustics_before_render_es_all_events),
  make_span(caustics_before_render_es_comps+0, 2)/*rw*/,
  make_span(caustics_before_render_es_comps+2, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,nullptr,"animchar_before_render_es");
static constexpr ecs::ComponentDesc water_caustics_node_exists_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("caustics__node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()}
};
static ecs::CompileTimeQueryDesc water_caustics_node_exists_ecs_query_desc
(
  "water_caustics_node_exists_ecs_query",
  empty_span(),
  make_span(water_caustics_node_exists_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void water_caustics_node_exists_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, water_caustics_node_exists_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(water_caustics_node_exists_ecs_query_comps, "caustics__node", dabfg::NodeHandle)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc water_quality_medium_or_high_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("render_settings__waterQuality"), ecs::ComponentTypeInfo<ecs::string>()}
};
static ecs::CompileTimeQueryDesc water_quality_medium_or_high_ecs_query_desc
(
  "water_quality_medium_or_high_ecs_query",
  empty_span(),
  make_span(water_quality_medium_or_high_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void water_quality_medium_or_high_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, water_quality_medium_or_high_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(water_quality_medium_or_high_ecs_query_comps, "render_settings__waterQuality", ecs::string)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc create_caustics_node_ecs_query_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("caustics__node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("caustics__indoor_probe_mask"), ecs::ComponentTypeInfo<UniqueTexHolder>()},
  {ECS_HASH("needs_water_heightmap"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("combined_shadows__use_additional_textures"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc create_caustics_node_ecs_query_desc
(
  "create_caustics_node_ecs_query",
  make_span(create_caustics_node_ecs_query_comps+0, 4)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void create_caustics_node_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, create_caustics_node_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(create_caustics_node_ecs_query_comps, "caustics__node", dabfg::NodeHandle)
            , ECS_RW_COMP(create_caustics_node_ecs_query_comps, "caustics__indoor_probe_mask", UniqueTexHolder)
            , ECS_RW_COMP(create_caustics_node_ecs_query_comps, "needs_water_heightmap", bool)
            , ECS_RW_COMP(create_caustics_node_ecs_query_comps, "combined_shadows__use_additional_textures", bool)
            );

        }while (++comp != compE);
    }
  );
}
