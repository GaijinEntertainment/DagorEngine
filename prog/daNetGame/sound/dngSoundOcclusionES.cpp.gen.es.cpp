#include "dngSoundOcclusionES.cpp.inl"
ECS_DEF_PULL_VAR(dngSoundOcclusion);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc dngsound_occlusion_gameobjects_created_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("msg_sink"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void dngsound_occlusion_gameobjects_created_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  dngsound_occlusion_gameobjects_created_es(evt
        );
}
static ecs::EntitySystemDesc dngsound_occlusion_gameobjects_created_es_es_desc
(
  "dngsound_occlusion_gameobjects_created_es",
  "prog/daNetGame/sound/dngSoundOcclusionES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dngsound_occlusion_gameobjects_created_es_all_events),
  empty_span(),
  empty_span(),
  make_span(dngsound_occlusion_gameobjects_created_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<EventGameObjectsCreated>::build(),
  0
,"sound");
static constexpr ecs::ComponentDesc dngsound_occlusion_enable_es_comps[] =
{
//start of 5 ro components at [0]
  {ECS_HASH("sound_occlusion__indoorMul"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("sound_occlusion__outdoorMul"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("sound_occlusion__sourceIndoorMul"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("sound_occlusion__sourceOutdoorMul"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("sound_occlusion__undergroundDepthDist"), ecs::ComponentTypeInfo<float>()},
//start of 1 rq components at [5]
  {ECS_HASH("soundOcclusion"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void dngsound_occlusion_enable_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dngsound_occlusion_enable_es(evt
        , ECS_RO_COMP(dngsound_occlusion_enable_es_comps, "sound_occlusion__indoorMul", float)
    , ECS_RO_COMP(dngsound_occlusion_enable_es_comps, "sound_occlusion__outdoorMul", float)
    , ECS_RO_COMP(dngsound_occlusion_enable_es_comps, "sound_occlusion__sourceIndoorMul", float)
    , ECS_RO_COMP(dngsound_occlusion_enable_es_comps, "sound_occlusion__sourceOutdoorMul", float)
    , ECS_RO_COMP(dngsound_occlusion_enable_es_comps, "sound_occlusion__undergroundDepthDist", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc dngsound_occlusion_enable_es_es_desc
(
  "dngsound_occlusion_enable_es",
  "prog/daNetGame/sound/dngSoundOcclusionES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dngsound_occlusion_enable_es_all_events),
  empty_span(),
  make_span(dngsound_occlusion_enable_es_comps+0, 5)/*ro*/,
  make_span(dngsound_occlusion_enable_es_comps+5, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<EventLevelLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"sound");
static constexpr ecs::ComponentDesc dngsound_occlusion_disappear_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("soundOcclusion"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void dngsound_occlusion_disappear_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  dngsound_occlusion_disappear_es(evt
        );
}
static ecs::EntitySystemDesc dngsound_occlusion_disappear_es_es_desc
(
  "dngsound_occlusion_disappear_es",
  "prog/daNetGame/sound/dngSoundOcclusionES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dngsound_occlusion_disappear_es_all_events),
  empty_span(),
  empty_span(),
  make_span(dngsound_occlusion_disappear_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"sound");
static constexpr ecs::ComponentDesc game_objects_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("game_objects"), ecs::ComponentTypeInfo<GameObjects>()}
};
static ecs::CompileTimeQueryDesc game_objects_ecs_query_desc
(
  "game_objects_ecs_query",
  empty_span(),
  make_span(game_objects_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void game_objects_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, game_objects_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(game_objects_ecs_query_comps, "game_objects", GameObjects)
            );

        }while (++comp != compE);
    }
  );
}
