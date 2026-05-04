// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "riexMotionVectorsES.cpp.inl"
ECS_DEF_PULL_VAR(riexMotionVectors);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc riex_set_prev_tm_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void riex_set_prev_tm_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventOnRendinstMovement>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    riex_set_prev_tm_es(static_cast<const EventOnRendinstMovement&>(evt)
        , ECS_RO_COMP(riex_set_prev_tm_es_comps, "eid", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc riex_set_prev_tm_es_es_desc
(
  "riex_set_prev_tm_es",
  "prog/daNetGameLibs/riex_motion_vectors/render/riexMotionVectorsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, riex_set_prev_tm_es_all_events),
  empty_span(),
  make_span(riex_set_prev_tm_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventOnRendinstMovement>::build(),
  0
,"render");
