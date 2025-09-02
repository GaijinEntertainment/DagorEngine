// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "tonemapES.cpp.inl"
ECS_DEF_PULL_VAR(tonemap);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc tonemapper_white_balance_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("white_balance"), ecs::ComponentTypeInfo<ecs::Object>()},
//start of 1 rq components at [1]
  {ECS_HASH("tonemapper"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void tonemapper_white_balance_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    tonemapper_white_balance_es_event_handler(evt
        , ECS_RO_COMP(tonemapper_white_balance_es_event_handler_comps, "white_balance", ecs::Object)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc tonemapper_white_balance_es_event_handler_es_desc
(
  "tonemapper_white_balance_es",
  "prog/gameLibs/ecs/render/tonemapES.cpp.inl",
  ecs::EntitySystemOps(nullptr, tonemapper_white_balance_es_event_handler_all_events),
  empty_span(),
  make_span(tonemapper_white_balance_es_event_handler_comps+0, 1)/*ro*/,
  make_span(tonemapper_white_balance_es_event_handler_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"white_balance");
static constexpr ecs::ComponentDesc tonemapper_color_grading_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("color_grading"), ecs::ComponentTypeInfo<ecs::Object>()},
//start of 1 rq components at [1]
  {ECS_HASH("tonemapper"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void tonemapper_color_grading_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    tonemapper_color_grading_es_event_handler(evt
        , ECS_RO_COMP(tonemapper_color_grading_es_event_handler_comps, "color_grading", ecs::Object)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc tonemapper_color_grading_es_event_handler_es_desc
(
  "tonemapper_color_grading_es",
  "prog/gameLibs/ecs/render/tonemapES.cpp.inl",
  ecs::EntitySystemOps(nullptr, tonemapper_color_grading_es_event_handler_all_events),
  empty_span(),
  make_span(tonemapper_color_grading_es_event_handler_comps+0, 1)/*ro*/,
  make_span(tonemapper_color_grading_es_event_handler_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"color_grading");
static constexpr ecs::ComponentDesc tonemapper_tonemap_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("tonemap"), ecs::ComponentTypeInfo<ecs::Object>()},
//start of 1 rq components at [1]
  {ECS_HASH("tonemapper"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void tonemapper_tonemap_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    tonemapper_tonemap_es_event_handler(evt
        , ECS_RO_COMP(tonemapper_tonemap_es_event_handler_comps, "tonemap", ecs::Object)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc tonemapper_tonemap_es_event_handler_es_desc
(
  "tonemapper_tonemap_es",
  "prog/gameLibs/ecs/render/tonemapES.cpp.inl",
  ecs::EntitySystemOps(nullptr, tonemapper_tonemap_es_event_handler_all_events),
  empty_span(),
  make_span(tonemapper_tonemap_es_event_handler_comps+0, 1)/*ro*/,
  make_span(tonemapper_tonemap_es_event_handler_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"tonemap");
static constexpr ecs::ComponentDesc tonemapper_appear_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("tonemap_save"), ecs::ComponentTypeInfo<ecs::Object>(), ecs::CDF_OPTIONAL},
//start of 3 ro components at [1]
  {ECS_HASH("white_balance"), ecs::ComponentTypeInfo<ecs::Object>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("color_grading"), ecs::ComponentTypeInfo<ecs::Object>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("tonemap"), ecs::ComponentTypeInfo<ecs::Object>(), ecs::CDF_OPTIONAL},
//start of 1 rq components at [4]
  {ECS_HASH("tonemapper"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void tonemapper_appear_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    tonemapper_appear_es_event_handler(evt
        , ECS_RO_COMP_PTR(tonemapper_appear_es_event_handler_comps, "white_balance", ecs::Object)
    , ECS_RO_COMP_PTR(tonemapper_appear_es_event_handler_comps, "color_grading", ecs::Object)
    , ECS_RO_COMP_PTR(tonemapper_appear_es_event_handler_comps, "tonemap", ecs::Object)
    , ECS_RW_COMP_PTR(tonemapper_appear_es_event_handler_comps, "tonemap_save", ecs::Object)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc tonemapper_appear_es_event_handler_es_desc
(
  "tonemapper_appear_es",
  "prog/gameLibs/ecs/render/tonemapES.cpp.inl",
  ecs::EntitySystemOps(nullptr, tonemapper_appear_es_event_handler_all_events),
  make_span(tonemapper_appear_es_event_handler_comps+0, 1)/*rw*/,
  make_span(tonemapper_appear_es_event_handler_comps+1, 3)/*ro*/,
  make_span(tonemapper_appear_es_event_handler_comps+4, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc tonemapper_disapear_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("tonemap_save"), ecs::ComponentTypeInfo<ecs::Object>(), ecs::CDF_OPTIONAL},
//start of 1 rq components at [1]
  {ECS_HASH("tonemapper"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void tonemapper_disapear_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    tonemapper_disapear_es_event_handler(evt
        , ECS_RW_COMP_PTR(tonemapper_disapear_es_event_handler_comps, "tonemap_save", ecs::Object)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc tonemapper_disapear_es_event_handler_es_desc
(
  "tonemapper_disapear_es",
  "prog/gameLibs/ecs/render/tonemapES.cpp.inl",
  ecs::EntitySystemOps(nullptr, tonemapper_disapear_es_event_handler_all_events),
  make_span(tonemapper_disapear_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  make_span(tonemapper_disapear_es_event_handler_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
);
