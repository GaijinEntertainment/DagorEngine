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
static constexpr ecs::ComponentDesc init_local_tone_mapping_samplers_node_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("local_tone_mapping__samplers_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()}
};
static void init_local_tone_mapping_samplers_node_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_local_tone_mapping_samplers_node_es(evt
        , ECS_RW_COMP(init_local_tone_mapping_samplers_node_es_comps, "local_tone_mapping__samplers_node", dabfg::NodeHandle)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_local_tone_mapping_samplers_node_es_es_desc
(
  "init_local_tone_mapping_samplers_node_es",
  "prog/daNetGameLibs/local_tone_mapping/render/localToneMappingSubscriptionES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_local_tone_mapping_samplers_node_es_all_events),
  make_span(init_local_tone_mapping_samplers_node_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
