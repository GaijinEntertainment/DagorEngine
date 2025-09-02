// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "particlePhysSys.cpp"
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc particle_phys_debug_es_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("particle_phys"), ecs::ComponentTypeInfo<daphys::ParticlePhysSystem>()}
};
static void particle_phys_debug_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    particle_phys_debug_es(*info.cast<ecs::UpdateStageInfoRenderDebug>()
    , ECS_RO_COMP(particle_phys_debug_es_comps, "transform", TMatrix)
    , ECS_RO_COMP(particle_phys_debug_es_comps, "particle_phys", daphys::ParticlePhysSystem)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc particle_phys_debug_es_es_desc
(
  "particle_phys_debug_es",
  "prog/gameLibs/ecs/phys/particlePhysSys.cpp",
  ecs::EntitySystemOps(particle_phys_debug_es_all),
  empty_span(),
  make_span(particle_phys_debug_es_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoRenderDebug::STAGE)
,"dev,render",nullptr,"*");
static constexpr ecs::ComponentDesc particle_phys_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("particle_phys"), ecs::ComponentTypeInfo<daphys::ParticlePhysSystem>()},
//start of 2 ro components at [1]
  {ECS_HASH("particle_phys__blk"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()}
};
static void particle_phys_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ecs::EventEntityCreated>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    particle_phys_es_event_handler(static_cast<const ecs::EventEntityCreated&>(evt)
        , ECS_RO_COMP(particle_phys_es_event_handler_comps, "particle_phys__blk", ecs::string)
    , ECS_RO_COMP(particle_phys_es_event_handler_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RW_COMP(particle_phys_es_event_handler_comps, "particle_phys", daphys::ParticlePhysSystem)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc particle_phys_es_event_handler_es_desc
(
  "particle_phys_es",
  "prog/gameLibs/ecs/phys/particlePhysSys.cpp",
  ecs::EntitySystemOps(nullptr, particle_phys_es_event_handler_all_events),
  make_span(particle_phys_es_event_handler_comps+0, 1)/*rw*/,
  make_span(particle_phys_es_event_handler_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated>::build(),
  0
,"render");
