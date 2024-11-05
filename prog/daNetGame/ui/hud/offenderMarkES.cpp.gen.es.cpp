#include "offenderMarkES.cpp.inl"
ECS_DEF_PULL_VAR(offenderMark);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc offender_mark_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("offender_marks"), ecs::ComponentTypeInfo<ecs::Array>()},
//start of 1 ro components at [1]
  {ECS_HASH("possessedByPlr"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void offender_mark_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<RenderEventUI>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    offender_mark_es(static_cast<const RenderEventUI&>(evt)
        , ECS_RO_COMP(offender_mark_es_comps, "possessedByPlr", ecs::EntityId)
    , ECS_RW_COMP(offender_mark_es_comps, "offender_marks", ecs::Array)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc offender_mark_es_es_desc
(
  "offender_mark_es",
  "prog/daNetGame/ui/hud/offenderMarkES.cpp.inl",
  ecs::EntitySystemOps(nullptr, offender_mark_es_all_events),
  make_span(offender_mark_es_comps+0, 1)/*rw*/,
  make_span(offender_mark_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<RenderEventUI>::build(),
  0
,"render,ui",nullptr,"*");
