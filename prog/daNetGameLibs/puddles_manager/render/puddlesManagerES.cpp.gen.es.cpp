// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "puddlesManagerES.cpp.inl"
ECS_DEF_PULL_VAR(puddlesManager);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc init_puddles_manager_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("puddles__manager"), ecs::ComponentTypeInfo<PuddlesManager>()},
//start of 1 ro components at [1]
  {ECS_HASH("puddles__bare_minimum_dist"), ecs::ComponentTypeInfo<float>()}
};
static void init_puddles_manager_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<OnLevelLoaded>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_puddles_manager_es(static_cast<const OnLevelLoaded&>(evt)
        , ECS_RW_COMP(init_puddles_manager_es_comps, "puddles__manager", PuddlesManager)
    , ECS_RO_COMP(init_puddles_manager_es_comps, "puddles__bare_minimum_dist", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_puddles_manager_es_es_desc
(
  "init_puddles_manager_es",
  "prog/daNetGameLibs/puddles_manager/render/puddlesManagerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_puddles_manager_es_all_events),
  make_span(init_puddles_manager_es_comps+0, 1)/*rw*/,
  make_span(init_puddles_manager_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnLevelLoaded>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc reinit_puddles_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("puddles__manager"), ecs::ComponentTypeInfo<PuddlesManager>()},
//start of 1 ro components at [1]
  {ECS_HASH("puddles__bare_minimum_dist"), ecs::ComponentTypeInfo<float>()}
};
static void reinit_puddles_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ChangeRenderFeatures>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    reinit_puddles_es(static_cast<const ChangeRenderFeatures&>(evt)
        , ECS_RW_COMP(reinit_puddles_es_comps, "puddles__manager", PuddlesManager)
    , ECS_RO_COMP(reinit_puddles_es_comps, "puddles__bare_minimum_dist", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc reinit_puddles_es_es_desc
(
  "reinit_puddles_es",
  "prog/daNetGameLibs/puddles_manager/render/puddlesManagerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, reinit_puddles_es_all_events),
  make_span(reinit_puddles_es_comps+0, 1)/*rw*/,
  make_span(reinit_puddles_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ChangeRenderFeatures>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc prepare_puddles_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("puddles__manager"), ecs::ComponentTypeInfo<PuddlesManager>()}
};
static void prepare_puddles_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<BeforeDraw>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    prepare_puddles_es(static_cast<const BeforeDraw&>(evt)
        , ECS_RW_COMP(prepare_puddles_es_comps, "puddles__manager", PuddlesManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc prepare_puddles_es_es_desc
(
  "prepare_puddles_es",
  "prog/daNetGameLibs/puddles_manager/render/puddlesManagerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, prepare_puddles_es_all_events),
  make_span(prepare_puddles_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<BeforeDraw>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc after_device_reset_puddles_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("puddles__manager"), ecs::ComponentTypeInfo<PuddlesManager>()}
};
static void after_device_reset_puddles_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<AfterDeviceReset>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    after_device_reset_puddles_es(static_cast<const AfterDeviceReset&>(evt)
        , ECS_RW_COMP(after_device_reset_puddles_es_comps, "puddles__manager", PuddlesManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc after_device_reset_puddles_es_es_desc
(
  "after_device_reset_puddles_es",
  "prog/daNetGameLibs/puddles_manager/render/puddlesManagerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, after_device_reset_puddles_es_all_events),
  make_span(after_device_reset_puddles_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<AfterDeviceReset>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc remove_puddles_in_crater_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("puddles__manager"), ecs::ComponentTypeInfo<PuddlesManager>()}
};
static void remove_puddles_in_crater_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<RemovePuddlesInRadius>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    remove_puddles_in_crater_es(static_cast<const RemovePuddlesInRadius&>(evt)
        , ECS_RW_COMP(remove_puddles_in_crater_es_comps, "puddles__manager", PuddlesManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc remove_puddles_in_crater_es_es_desc
(
  "remove_puddles_in_crater_es",
  "prog/daNetGameLibs/puddles_manager/render/puddlesManagerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, remove_puddles_in_crater_es_all_events),
  make_span(remove_puddles_in_crater_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<RemovePuddlesInRadius>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc invalidate_puddles_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("puddles__manager"), ecs::ComponentTypeInfo<PuddlesManager>()}
};
static void invalidate_puddles_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<AfterHeightmapChange>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    invalidate_puddles_es(static_cast<const AfterHeightmapChange&>(evt)
        , ECS_RW_COMP(invalidate_puddles_es_comps, "puddles__manager", PuddlesManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc invalidate_puddles_es_es_desc
(
  "invalidate_puddles_es",
  "prog/daNetGameLibs/puddles_manager/render/puddlesManagerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, invalidate_puddles_es_all_events),
  make_span(invalidate_puddles_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<AfterHeightmapChange>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc unload_puddles_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("puddles__manager"), ecs::ComponentTypeInfo<PuddlesManager>()}
};
static void unload_puddles_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UnloadLevel>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    unload_puddles_es(static_cast<const UnloadLevel&>(evt)
        , ECS_RW_COMP(unload_puddles_es_comps, "puddles__manager", PuddlesManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc unload_puddles_es_es_desc
(
  "unload_puddles_es",
  "prog/daNetGameLibs/puddles_manager/render/puddlesManagerES.cpp.inl",
  ecs::EntitySystemOps(nullptr, unload_puddles_es_all_events),
  make_span(unload_puddles_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UnloadLevel>::build(),
  0
,"render");
