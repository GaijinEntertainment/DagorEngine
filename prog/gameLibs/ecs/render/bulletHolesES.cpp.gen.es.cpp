#include "bulletHolesES.cpp.inl"
ECS_DEF_PULL_VAR(bulletHoles);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc bullet_holes_updater_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("collres"), ecs::ComponentTypeInfo<CollisionResource>()},
//start of 3 ro components at [1]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("bullet_holes"), ecs::ComponentTypeInfo<BulletHoles>()},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
//start of 1 no components at [4]
  {ECS_HASH("disableUpdate"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void bullet_holes_updater_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    bullet_holes_updater_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RO_COMP(bullet_holes_updater_es_comps, "transform", TMatrix)
    , ECS_RO_COMP(bullet_holes_updater_es_comps, "bullet_holes", BulletHoles)
    , ECS_RO_COMP(bullet_holes_updater_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RW_COMP(bullet_holes_updater_es_comps, "collres", CollisionResource)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc bullet_holes_updater_es_es_desc
(
  "bullet_holes_updater_es",
  "prog/gameLibs/ecs/render/bulletHolesES.cpp.inl",
  ecs::EntitySystemOps(bullet_holes_updater_es_all),
  make_span(bullet_holes_updater_es_comps+0, 1)/*rw*/,
  make_span(bullet_holes_updater_es_comps+1, 3)/*ro*/,
  empty_span(),
  make_span(bullet_holes_updater_es_comps+4, 1)/*no*/,
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,nullptr,nullptr,"*");
static constexpr ecs::ComponentDesc bullet_holes_es_event_handler_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("bullet_holes"), ecs::ComponentTypeInfo<BulletHoles>()},
  {ECS_HASH("collres"), ecs::ComponentTypeInfo<CollisionResource>()},
//start of 2 ro components at [2]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
//start of 1 no components at [4]
  {ECS_HASH("disableUpdate"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void bullet_holes_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventOnProjectileHit>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    bullet_holes_es_event_handler(static_cast<const EventOnProjectileHit&>(evt)
        , ECS_RO_COMP(bullet_holes_es_event_handler_comps, "transform", TMatrix)
    , ECS_RW_COMP(bullet_holes_es_event_handler_comps, "bullet_holes", BulletHoles)
    , ECS_RO_COMP(bullet_holes_es_event_handler_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RW_COMP(bullet_holes_es_event_handler_comps, "collres", CollisionResource)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc bullet_holes_es_event_handler_es_desc
(
  "bullet_holes_es",
  "prog/gameLibs/ecs/render/bulletHolesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, bullet_holes_es_event_handler_all_events),
  make_span(bullet_holes_es_event_handler_comps+0, 2)/*rw*/,
  make_span(bullet_holes_es_event_handler_comps+2, 2)/*ro*/,
  empty_span(),
  make_span(bullet_holes_es_event_handler_comps+4, 1)/*no*/,
  ecs::EventSetBuilder<EventOnProjectileHit>::build(),
  0
);
