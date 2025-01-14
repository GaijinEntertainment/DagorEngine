#include "ragdollES.cpp.inl"
ECS_DEF_PULL_VAR(ragdoll);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc ragdoll_start_es_event_handler_comps[] =
{
//start of 5 rw components at [0]
  {ECS_HASH("projectile_impulse"), ecs::ComponentTypeInfo<ProjectileImpulse>()},
  {ECS_HASH("ragdoll"), ecs::ComponentTypeInfo<ragdoll_t>()},
  {ECS_HASH("collres"), ecs::ComponentTypeInfo<CollisionResource>()},
  {ECS_HASH("ragdoll__massMatrix"), ecs::ComponentTypeInfo<Point4>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("ragdoll__applyParams"), ecs::ComponentTypeInfo<bool>()},
//start of 8 ro components at [5]
  {ECS_HASH("ragdoll__isSingleBody"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("projectile_impulse__impulseSaveDeltaTime"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("projectile_impulse__cinematicArtistryMult"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("projectile_impulse__cinematicArtistrySpeedMult"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("projectile_impulse__maxSingleImpulse"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("projectile_impulse__maxVelocity"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("projectile_impulse__maxOmega"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("projectile_impulse__maxCount"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL}
};
static void ragdoll_start_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ParallelUpdateFrameDelayed>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    ragdoll_start_es_event_handler(static_cast<const ParallelUpdateFrameDelayed&>(evt)
        , ECS_RW_COMP(ragdoll_start_es_event_handler_comps, "projectile_impulse", ProjectileImpulse)
    , ECS_RW_COMP(ragdoll_start_es_event_handler_comps, "ragdoll", ragdoll_t)
    , ECS_RW_COMP(ragdoll_start_es_event_handler_comps, "collres", CollisionResource)
    , ECS_RW_COMP_PTR(ragdoll_start_es_event_handler_comps, "ragdoll__massMatrix", Point4)
    , ECS_RW_COMP(ragdoll_start_es_event_handler_comps, "ragdoll__applyParams", bool)
    , ECS_RO_COMP_OR(ragdoll_start_es_event_handler_comps, "ragdoll__isSingleBody", bool(false))
    , ECS_RO_COMP_OR(ragdoll_start_es_event_handler_comps, "projectile_impulse__impulseSaveDeltaTime", float(0.5f))
    , ECS_RO_COMP_OR(ragdoll_start_es_event_handler_comps, "projectile_impulse__cinematicArtistryMult", float(0.01f))
    , ECS_RO_COMP_OR(ragdoll_start_es_event_handler_comps, "projectile_impulse__cinematicArtistrySpeedMult", float(0.1f))
    , ECS_RO_COMP_OR(ragdoll_start_es_event_handler_comps, "projectile_impulse__maxSingleImpulse", float(0.01f))
    , ECS_RO_COMP_OR(ragdoll_start_es_event_handler_comps, "projectile_impulse__maxVelocity", float(-1.0f))
    , ECS_RO_COMP_OR(ragdoll_start_es_event_handler_comps, "projectile_impulse__maxOmega", float(-1.0f))
    , ECS_RO_COMP_OR(ragdoll_start_es_event_handler_comps, "projectile_impulse__maxCount", int(-1))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc ragdoll_start_es_event_handler_es_desc
(
  "ragdoll_start_es",
  "prog/gameLibs/ecs/phys/ragdollES.cpp.inl",
  ecs::EntitySystemOps(nullptr, ragdoll_start_es_event_handler_all_events),
  make_span(ragdoll_start_es_event_handler_comps+0, 5)/*rw*/,
  make_span(ragdoll_start_es_event_handler_comps+5, 8)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ParallelUpdateFrameDelayed>::build(),
  0
,nullptr,nullptr,"start_async_phys_sim_es");
static constexpr ecs::ComponentDesc ragdoll_alive_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("projectile_impulse"), ecs::ComponentTypeInfo<ProjectileImpulse>()},
//start of 2 ro components at [1]
  {ECS_HASH("projectile_impulse__impulseSaveDeltaTime"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("isAlive"), ecs::ComponentTypeInfo<bool>()}
};
static void ragdoll_alive_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventOnPhysImpulse>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(ECS_RO_COMP(ragdoll_alive_es_event_handler_comps, "isAlive", bool)) )
      continue;
    ragdoll_alive_es_event_handler(static_cast<const EventOnPhysImpulse&>(evt)
          , ECS_RW_COMP(ragdoll_alive_es_event_handler_comps, "projectile_impulse", ProjectileImpulse)
      , ECS_RO_COMP_OR(ragdoll_alive_es_event_handler_comps, "projectile_impulse__impulseSaveDeltaTime", float(1.f))
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc ragdoll_alive_es_event_handler_es_desc
(
  "ragdoll_alive_es",
  "prog/gameLibs/ecs/phys/ragdollES.cpp.inl",
  ecs::EntitySystemOps(nullptr, ragdoll_alive_es_event_handler_all_events),
  make_span(ragdoll_alive_es_event_handler_comps+0, 1)/*rw*/,
  make_span(ragdoll_alive_es_event_handler_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventOnPhysImpulse>::build(),
  0
);
static constexpr ecs::ComponentDesc ragdoll_dead_es_event_handler_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("ragdoll"), ecs::ComponentTypeInfo<ragdoll_t>()},
  {ECS_HASH("projectile_impulse"), ecs::ComponentTypeInfo<ProjectileImpulse>()},
//start of 3 ro components at [2]
  {ECS_HASH("projectile_impulse__impulseSaveDeltaTime"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("projectile_impulse__cinematicArtistryMultDead"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("isAlive"), ecs::ComponentTypeInfo<bool>()}
};
static void ragdoll_dead_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventOnPhysImpulse>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(!ECS_RO_COMP(ragdoll_dead_es_event_handler_comps, "isAlive", bool)) )
      continue;
    ragdoll_dead_es_event_handler(static_cast<const EventOnPhysImpulse&>(evt)
          , ECS_RW_COMP(ragdoll_dead_es_event_handler_comps, "ragdoll", ragdoll_t)
      , ECS_RW_COMP(ragdoll_dead_es_event_handler_comps, "projectile_impulse", ProjectileImpulse)
      , ECS_RO_COMP_OR(ragdoll_dead_es_event_handler_comps, "projectile_impulse__impulseSaveDeltaTime", float(1.f))
      , ECS_RO_COMP_OR(ragdoll_dead_es_event_handler_comps, "projectile_impulse__cinematicArtistryMultDead", float(5.f))
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc ragdoll_dead_es_event_handler_es_desc
(
  "ragdoll_dead_es",
  "prog/gameLibs/ecs/phys/ragdollES.cpp.inl",
  ecs::EntitySystemOps(nullptr, ragdoll_dead_es_event_handler_all_events),
  make_span(ragdoll_dead_es_event_handler_comps+0, 2)/*rw*/,
  make_span(ragdoll_dead_es_event_handler_comps+2, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventOnPhysImpulse>::build(),
  0
);
