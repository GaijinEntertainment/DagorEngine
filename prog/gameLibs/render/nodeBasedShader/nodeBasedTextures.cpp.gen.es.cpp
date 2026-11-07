// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "nodeBasedTextures.cpp.inl"
ECS_DEF_PULL_VAR(nodeBasedTextures);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc add_nbs_texture_es_comps[] =
{
//start of 4 ro components at [0]
  {ECS_HASH("nbs_texture__system"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("nbs_texture__name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("nbs_texture__sampler"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("nbs_texture__slot"), ecs::ComponentTypeInfo<int>()}
};
static void add_nbs_texture_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    add_nbs_texture_es(evt
        , ECS_RO_COMP(add_nbs_texture_es_comps, "nbs_texture__system", ecs::string)
    , ECS_RO_COMP(add_nbs_texture_es_comps, "nbs_texture__name", ecs::string)
    , ECS_RO_COMP(add_nbs_texture_es_comps, "nbs_texture__sampler", ecs::string)
    , ECS_RO_COMP(add_nbs_texture_es_comps, "nbs_texture__slot", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc add_nbs_texture_es_es_desc
(
  "add_nbs_texture_es",
  "prog/gameLibs/render/nodeBasedShader/nodeBasedTextures.cpp.inl",
  ecs::EntitySystemOps(nullptr, add_nbs_texture_es_all_events),
  empty_span(),
  make_span(add_nbs_texture_es_comps+0, 4)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc remove_nbs_texture_es_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("nbs_texture__system"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("nbs_texture__slot"), ecs::ComponentTypeInfo<int>()}
};
static void remove_nbs_texture_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    remove_nbs_texture_es(evt
        , ECS_RO_COMP(remove_nbs_texture_es_comps, "nbs_texture__system", ecs::string)
    , ECS_RO_COMP(remove_nbs_texture_es_comps, "nbs_texture__slot", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc remove_nbs_texture_es_es_desc
(
  "remove_nbs_texture_es",
  "prog/gameLibs/render/nodeBasedShader/nodeBasedTextures.cpp.inl",
  ecs::EntitySystemOps(nullptr, remove_nbs_texture_es_all_events),
  empty_span(),
  make_span(remove_nbs_texture_es_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
