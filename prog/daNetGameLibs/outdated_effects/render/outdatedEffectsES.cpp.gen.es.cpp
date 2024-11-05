#include "outdatedEffectsES.cpp.inl"
ECS_DEF_PULL_VAR(outdatedEffects);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc register_outdated_effects_set_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("outdated_effects"), ecs::ComponentTypeInfo<ecs::StringList>()}
};
static void register_outdated_effects_set_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    register_outdated_effects_set_es_event_handler(evt
        , ECS_RW_COMP(register_outdated_effects_set_es_event_handler_comps, "outdated_effects", ecs::StringList)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc register_outdated_effects_set_es_event_handler_es_desc
(
  "register_outdated_effects_set_es",
  "prog/daNetGameLibs/outdated_effects/render/outdatedEffectsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, register_outdated_effects_set_es_event_handler_all_events),
  make_span(register_outdated_effects_set_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc register_outdated_effects_params_es_event_handler_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("outdated_effects__check_once"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("outdated_effects__check_all_effects"), ecs::ComponentTypeInfo<bool>()}
};
static void register_outdated_effects_params_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    register_outdated_effects_params_es_event_handler(evt
        , ECS_RO_COMP(register_outdated_effects_params_es_event_handler_comps, "outdated_effects__check_once", bool)
    , ECS_RO_COMP(register_outdated_effects_params_es_event_handler_comps, "outdated_effects__check_all_effects", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc register_outdated_effects_params_es_event_handler_es_desc
(
  "register_outdated_effects_params_es",
  "prog/daNetGameLibs/outdated_effects/render/outdatedEffectsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, register_outdated_effects_params_es_event_handler_all_events),
  empty_span(),
  make_span(register_outdated_effects_params_es_event_handler_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","outdated_effects__check_all_effects,outdated_effects__check_once");
static constexpr ecs::ComponentDesc validate_outdated_one_effect_es_event_handler_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("effect__name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>(), ecs::CDF_OPTIONAL}
};
static void validate_outdated_one_effect_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    validate_outdated_one_effect_es_event_handler(evt
        , ECS_RO_COMP(validate_outdated_one_effect_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(validate_outdated_one_effect_es_event_handler_comps, "effect__name", ecs::string)
    , ECS_RO_COMP_OR(validate_outdated_one_effect_es_event_handler_comps, "transform", TMatrix(TMatrix::IDENT))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc validate_outdated_one_effect_es_event_handler_es_desc
(
  "validate_outdated_one_effect_es",
  "prog/daNetGameLibs/outdated_effects/render/outdatedEffectsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, validate_outdated_one_effect_es_event_handler_all_events),
  empty_span(),
  make_span(validate_outdated_one_effect_es_event_handler_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc validate_outdated_many_effects_es_event_handler_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("effects"), ecs::ComponentTypeInfo<ecs::Object>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>(), ecs::CDF_OPTIONAL}
};
static void validate_outdated_many_effects_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    validate_outdated_many_effects_es_event_handler(evt
        , ECS_RO_COMP(validate_outdated_many_effects_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(validate_outdated_many_effects_es_event_handler_comps, "effects", ecs::Object)
    , ECS_RO_COMP_OR(validate_outdated_many_effects_es_event_handler_comps, "transform", TMatrix(TMatrix::IDENT))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc validate_outdated_many_effects_es_event_handler_es_desc
(
  "validate_outdated_many_effects_es",
  "prog/daNetGameLibs/outdated_effects/render/outdatedEffectsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, validate_outdated_many_effects_es_event_handler_all_events),
  empty_span(),
  make_span(validate_outdated_many_effects_es_event_handler_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc validate_outdated_ground_effects_es_event_handler_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("ground_effect__fx_name"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void validate_outdated_ground_effects_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    validate_outdated_ground_effects_es_event_handler(evt
        , ECS_RO_COMP(validate_outdated_ground_effects_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(validate_outdated_ground_effects_es_event_handler_comps, "ground_effect__fx_name", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc validate_outdated_ground_effects_es_event_handler_es_desc
(
  "validate_outdated_ground_effects_es",
  "prog/daNetGameLibs/outdated_effects/render/outdatedEffectsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, validate_outdated_ground_effects_es_event_handler_all_events),
  empty_span(),
  make_span(validate_outdated_ground_effects_es_event_handler_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc validate_outdated_effect_area_es_event_handler_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("effect_area__effectName"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>(), ecs::CDF_OPTIONAL}
};
static void validate_outdated_effect_area_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    validate_outdated_effect_area_es_event_handler(evt
        , ECS_RO_COMP(validate_outdated_effect_area_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(validate_outdated_effect_area_es_event_handler_comps, "effect_area__effectName", ecs::string)
    , ECS_RO_COMP_OR(validate_outdated_effect_area_es_event_handler_comps, "transform", TMatrix(TMatrix::IDENT))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc validate_outdated_effect_area_es_event_handler_es_desc
(
  "validate_outdated_effect_area_es",
  "prog/daNetGameLibs/outdated_effects/render/outdatedEffectsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, validate_outdated_effect_area_es_event_handler_all_events),
  empty_span(),
  make_span(validate_outdated_effect_area_es_event_handler_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
//static constexpr ecs::ComponentDesc validate_outdated_start_effect_es_event_handler_comps[] ={};
static void validate_outdated_start_effect_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<acesfx::StartEffectEvent>());
  validate_outdated_start_effect_es_event_handler(static_cast<const acesfx::StartEffectEvent&>(evt)
        );
}
static ecs::EntitySystemDesc validate_outdated_start_effect_es_event_handler_es_desc
(
  "validate_outdated_start_effect_es",
  "prog/daNetGameLibs/outdated_effects/render/outdatedEffectsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, validate_outdated_start_effect_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<acesfx::StartEffectEvent>::build(),
  0
,"dev,render");
