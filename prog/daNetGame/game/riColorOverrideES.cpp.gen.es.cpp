// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "riColorOverrideES.cpp.inl"
ECS_DEF_PULL_VAR(riColorOverride);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc init_ri_color_override_es_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("ri_color_override__name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("ri_color_override__from"), ecs::ComponentTypeInfo<E3DCOLOR>()},
  {ECS_HASH("ri_color_override__to"), ecs::ComponentTypeInfo<E3DCOLOR>()}
};
static void init_ri_color_override_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_ri_color_override_es(evt
        , ECS_RO_COMP(init_ri_color_override_es_comps, "ri_color_override__name", ecs::string)
    , ECS_RO_COMP(init_ri_color_override_es_comps, "ri_color_override__from", E3DCOLOR)
    , ECS_RO_COMP(init_ri_color_override_es_comps, "ri_color_override__to", E3DCOLOR)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_ri_color_override_es_es_desc
(
  "init_ri_color_override_es",
  "prog/daNetGame/game/riColorOverrideES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_ri_color_override_es_all_events),
  empty_span(),
  make_span(init_ri_color_override_es_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnLevelLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc edit_ri_color_override_es_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("ri_color_override__name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("ri_color_override__from"), ecs::ComponentTypeInfo<E3DCOLOR>()},
  {ECS_HASH("ri_color_override__to"), ecs::ComponentTypeInfo<E3DCOLOR>()}
};
static void edit_ri_color_override_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    edit_ri_color_override_es(evt
        , ECS_RO_COMP(edit_ri_color_override_es_comps, "ri_color_override__name", ecs::string)
    , ECS_RO_COMP(edit_ri_color_override_es_comps, "ri_color_override__from", E3DCOLOR)
    , ECS_RO_COMP(edit_ri_color_override_es_comps, "ri_color_override__to", E3DCOLOR)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc edit_ri_color_override_es_es_desc
(
  "edit_ri_color_override_es",
  "prog/daNetGame/game/riColorOverrideES.cpp.inl",
  ecs::EntitySystemOps(nullptr, edit_ri_color_override_es_all_events),
  empty_span(),
  make_span(edit_ri_color_override_es_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,"render","*");
static constexpr ecs::ComponentDesc destr_ri_color_override_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("ri_color_override__name"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void destr_ri_color_override_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    destr_ri_color_override_es(evt
        , ECS_RO_COMP(destr_ri_color_override_es_comps, "ri_color_override__name", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc destr_ri_color_override_es_es_desc
(
  "destr_ri_color_override_es",
  "prog/daNetGame/game/riColorOverrideES.cpp.inl",
  ecs::EntitySystemOps(nullptr, destr_ri_color_override_es_all_events),
  empty_span(),
  make_span(destr_ri_color_override_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
