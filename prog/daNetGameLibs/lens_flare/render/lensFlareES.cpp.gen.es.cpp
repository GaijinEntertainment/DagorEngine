// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "lensFlareES.cpp.inl"
ECS_DEF_PULL_VAR(lensFlare);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc lens_flare_renderer_on_reinit_es_comps[] =
{
//start of 6 rw components at [0]
  {ECS_HASH("lens_flare_renderer"), ecs::ComponentTypeInfo<LensFlareRenderer>()},
  {ECS_HASH("lens_flare_renderer__per_camera_res_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("lens_flare_renderer__render_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("lens_flare_renderer__prepare_lights_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("lens_flare_renderer__camcam_lens_prepare_lights_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("lens_flare_renderer__camcam_lens_render_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()}
};
static void lens_flare_renderer_on_reinit_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    lens_flare_renderer_on_reinit_es(evt
        , components.manager()
    , ECS_RW_COMP(lens_flare_renderer_on_reinit_es_comps, "lens_flare_renderer", LensFlareRenderer)
    , ECS_RW_COMP(lens_flare_renderer_on_reinit_es_comps, "lens_flare_renderer__per_camera_res_node", dafg::NodeHandle)
    , ECS_RW_COMP(lens_flare_renderer_on_reinit_es_comps, "lens_flare_renderer__render_node", dafg::NodeHandle)
    , ECS_RW_COMP(lens_flare_renderer_on_reinit_es_comps, "lens_flare_renderer__prepare_lights_node", dafg::NodeHandle)
    , ECS_RW_COMP(lens_flare_renderer_on_reinit_es_comps, "lens_flare_renderer__camcam_lens_prepare_lights_node", dafg::NodeHandle)
    , ECS_RW_COMP(lens_flare_renderer_on_reinit_es_comps, "lens_flare_renderer__camcam_lens_render_node", dafg::NodeHandle)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc lens_flare_renderer_on_reinit_es_es_desc
(
  "lens_flare_renderer_on_reinit_es",
  "prog/daNetGameLibs/lens_flare/render/lensFlareES.cpp.inl",
  ecs::EntitySystemOps(nullptr, lens_flare_renderer_on_reinit_es_all_events),
  make_span(lens_flare_renderer_on_reinit_es_comps+0, 6)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ChangeRenderFeatures,
                       OnRenderSettingsUpdated,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc register_lens_flare_for_postfx_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("lens_flare_renderer"), ecs::ComponentTypeInfo<LensFlareRenderer>()},
//start of 1 no components at [1]
  {ECS_HASH("renderInDistortion"), ecs::ComponentTypeInfo<ecs::Tag>()}
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
  make_span(register_lens_flare_for_postfx_es_comps+1, 1)/*no*/,
  ecs::EventSetBuilder<RegisterPostfxResources>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc register_lens_flare_for_distortion_es_comps[] =
{
//start of 2 rq components at [0]
  {ECS_HASH("lens_flare_renderer"), ecs::ComponentTypeInfo<LensFlareRenderer>()},
  {ECS_HASH("renderInDistortion"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void register_lens_flare_for_distortion_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<RegisterDistortionModifiersResources>());
  register_lens_flare_for_distortion_es(static_cast<const RegisterDistortionModifiersResources&>(evt)
        );
}
static ecs::EntitySystemDesc register_lens_flare_for_distortion_es_es_desc
(
  "register_lens_flare_for_distortion_es",
  "prog/daNetGameLibs/lens_flare/render/lensFlareES.cpp.inl",
  ecs::EntitySystemOps(nullptr, register_lens_flare_for_distortion_es_all_events),
  empty_span(),
  empty_span(),
  make_span(register_lens_flare_for_distortion_es_comps+0, 2)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<RegisterDistortionModifiersResources>::build(),
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
inline void lens_glow_enabled_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable function)
{
  perform_query(&manager, eid, lens_glow_enabled_ecs_query_desc.getHandle(),
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
