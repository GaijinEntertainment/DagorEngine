#include "localToneMappingSubscriptionES.cpp.inl"
ECS_DEF_PULL_VAR(localToneMappingSubscription);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc register_local_tone_mapping_for_postfx_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("local_tone_mapping__filter_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()}
};
static void register_local_tone_mapping_for_postfx_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<RegisterPostfxResources>());
  register_local_tone_mapping_for_postfx_es(static_cast<const RegisterPostfxResources&>(evt)
        );
}
static ecs::EntitySystemDesc register_local_tone_mapping_for_postfx_es_es_desc
(
  "register_local_tone_mapping_for_postfx_es",
  "prog/daNetGameLibs/local_tone_mapping/render/localToneMappingSubscriptionES.cpp.inl",
  ecs::EntitySystemOps(nullptr, register_local_tone_mapping_for_postfx_es_all_events),
  empty_span(),
  empty_span(),
  make_span(register_local_tone_mapping_for_postfx_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<RegisterPostfxResources>::build(),
  0
,"render");
