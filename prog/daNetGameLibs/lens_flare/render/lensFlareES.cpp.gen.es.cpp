// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "lensFlareES.cpp.inl"
ECS_DEF_PULL_VAR(lensFlare);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc lens_flare_renderer_on_appear_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("lens_flare_renderer"), ecs::ComponentTypeInfo<LensFlareRenderer>()},
  {ECS_HASH("lens_flare_renderer__render_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("lens_flare_renderer__prepare_lights_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()}
};
static void lens_flare_renderer_on_appear_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    lens_flare_renderer_on_appear_es(evt
        , ECS_RW_COMP(lens_flare_renderer_on_appear_es_comps, "lens_flare_renderer", LensFlareRenderer)
    , ECS_RW_COMP(lens_flare_renderer_on_appear_es_comps, "lens_flare_renderer__render_node", dafg::NodeHandle)
    , ECS_RW_COMP(lens_flare_renderer_on_appear_es_comps, "lens_flare_renderer__prepare_lights_node", dafg::NodeHandle)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc lens_flare_renderer_on_appear_es_es_desc
(
  "lens_flare_renderer_on_appear_es",
  "prog/daNetGameLibs/lens_flare/render/lensFlareES.cpp.inl",
  ecs::EntitySystemOps(nullptr, lens_flare_renderer_on_appear_es_all_events),
  make_span(lens_flare_renderer_on_appear_es_comps+0, 3)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc lens_flare_renderer_on_settings_changed_es_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("render_settings__lensFlares"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("render_settings__bare_minimum"), ecs::ComponentTypeInfo<bool>()}
};
static void lens_flare_renderer_on_settings_changed_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    lens_flare_renderer_on_settings_changed_es(evt
        , ECS_RO_COMP(lens_flare_renderer_on_settings_changed_es_comps, "render_settings__lensFlares", bool)
    , ECS_RO_COMP(lens_flare_renderer_on_settings_changed_es_comps, "render_settings__bare_minimum", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc lens_flare_renderer_on_settings_changed_es_es_desc
(
  "lens_flare_renderer_on_settings_changed_es",
  "prog/daNetGameLibs/lens_flare/render/lensFlareES.cpp.inl",
  ecs::EntitySystemOps(nullptr, lens_flare_renderer_on_settings_changed_es_all_events),
  empty_span(),
  make_span(lens_flare_renderer_on_settings_changed_es_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnRenderSettingsReady>::build(),
  0
,"render","render_settings__bare_minimum,render_settings__lensFlares");
static constexpr ecs::ComponentDesc register_lens_flare_for_postfx_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("lens_flare_renderer"), ecs::ComponentTypeInfo<LensFlareRenderer>()}
};
static void register_lens_flare_for_postfx_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<RegisterPostfxResources>());
  register_lens_flare_for_postfx_es(static_cast<const RegisterPostfxResources&>(evt)
        );
}
static ecs::EntitySystemDesc register_lens_flare_for_postfx_es_es_desc
(
  "register_lens_flare_for_postfx_es",
  "prog/daNetGameLibs/lens_flare/render/lensFlareES.cpp.inl",
  ecs::EntitySystemOps(nullptr, register_lens_flare_for_postfx_es_all_events),
  empty_span(),
  empty_span(),
  make_span(register_lens_flare_for_postfx_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<RegisterPostfxResources>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc lens_glow_enabled_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("render_settings__lensFlares"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("render_settings__bare_minimum"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc lens_glow_enabled_ecs_query_desc
(
  "lens_glow_enabled_ecs_query",
  empty_span(),
  make_span(lens_glow_enabled_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void lens_glow_enabled_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, lens_glow_enabled_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(lens_glow_enabled_ecs_query_comps, "render_settings__lensFlares", bool)
            , ECS_RO_COMP(lens_glow_enabled_ecs_query_comps, "render_settings__bare_minimum", bool)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc init_lens_flare_nodes_ecs_query_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("lens_flare_renderer"), ecs::ComponentTypeInfo<LensFlareRenderer>()},
  {ECS_HASH("lens_flare_renderer__render_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("lens_flare_renderer__prepare_lights_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()}
};
static ecs::CompileTimeQueryDesc init_lens_flare_nodes_ecs_query_desc
(
  "init_lens_flare_nodes_ecs_query",
  make_span(init_lens_flare_nodes_ecs_query_comps+0, 3)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void init_lens_flare_nodes_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, init_lens_flare_nodes_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(init_lens_flare_nodes_ecs_query_comps, "lens_flare_renderer", LensFlareRenderer)
            , ECS_RW_COMP(init_lens_flare_nodes_ecs_query_comps, "lens_flare_renderer__render_node", dafg::NodeHandle)
            , ECS_RW_COMP(init_lens_flare_nodes_ecs_query_comps, "lens_flare_renderer__prepare_lights_node", dafg::NodeHandle)
            );

        }while (++comp != compE);
    }
  );
}
