#include "soundGroupES.cpp.inl"
ECS_DEF_PULL_VAR(soundGroup);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc update_sound_group_using_animchar_es_comps[] ={};
static void update_sound_group_using_animchar_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<ParallelUpdateFrameDelayed>());
  update_sound_group_using_animchar_es(static_cast<const ParallelUpdateFrameDelayed&>(evt)
        );
}
static ecs::EntitySystemDesc update_sound_group_using_animchar_es_es_desc
(
  "update_sound_group_using_animchar_es",
  "prog/gameLibs/ecs/sound/soundGroupES.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_sound_group_using_animchar_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ParallelUpdateFrameDelayed>::build(),
  0
,"sound");
//static constexpr ecs::ComponentDesc update_sound_group_using_transform_es_comps[] ={};
static void update_sound_group_using_transform_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<ParallelUpdateFrameDelayed>());
  update_sound_group_using_transform_es(static_cast<const ParallelUpdateFrameDelayed&>(evt)
        );
}
static ecs::EntitySystemDesc update_sound_group_using_transform_es_es_desc
(
  "update_sound_group_using_transform_es",
  "prog/gameLibs/ecs/sound/soundGroupES.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_sound_group_using_transform_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ParallelUpdateFrameDelayed>::build(),
  0
,"sound");
static constexpr ecs::ComponentDesc destroy_sound_group_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("sound_event_group"), ecs::ComponentTypeInfo<SoundEventGroup>()}
};
static void destroy_sound_group_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    destroy_sound_group_es_event_handler(evt
        , ECS_RW_COMP(destroy_sound_group_es_event_handler_comps, "sound_event_group", SoundEventGroup)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc destroy_sound_group_es_event_handler_es_desc
(
  "destroy_sound_group_es",
  "prog/gameLibs/ecs/sound/soundGroupES.cpp.inl",
  ecs::EntitySystemOps(nullptr, destroy_sound_group_es_event_handler_all_events),
  make_span(destroy_sound_group_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"sound");
static constexpr ecs::ComponentDesc sound_group_with_animchar_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("sound_event_group"), ecs::ComponentTypeInfo<SoundEventGroup>()},
//start of 1 ro components at [1]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
//start of 1 rq components at [2]
  {ECS_HASH("sound_group_with_animchar"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc sound_group_with_animchar_ecs_query_desc
(
  "sound_group_with_animchar_ecs_query",
  make_span(sound_group_with_animchar_ecs_query_comps+0, 1)/*rw*/,
  make_span(sound_group_with_animchar_ecs_query_comps+1, 1)/*ro*/,
  make_span(sound_group_with_animchar_ecs_query_comps+2, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void sound_group_with_animchar_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, sound_group_with_animchar_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(sound_group_with_animchar_ecs_query_comps, "sound_event_group", SoundEventGroup)
            , ECS_RO_COMP(sound_group_with_animchar_ecs_query_comps, "animchar", AnimV20::AnimcharBaseComponent)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc sound_group_with_transform_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("sound_event_group"), ecs::ComponentTypeInfo<SoundEventGroup>()},
//start of 1 ro components at [1]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 1 no components at [2]
  {ECS_HASH("sound_group_with_animchar"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc sound_group_with_transform_ecs_query_desc
(
  "sound_group_with_transform_ecs_query",
  make_span(sound_group_with_transform_ecs_query_comps+0, 1)/*rw*/,
  make_span(sound_group_with_transform_ecs_query_comps+1, 1)/*ro*/,
  empty_span(),
  make_span(sound_group_with_transform_ecs_query_comps+2, 1)/*no*/);
template<typename Callable>
inline void sound_group_with_transform_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, sound_group_with_transform_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(sound_group_with_transform_ecs_query_comps, "sound_event_group", SoundEventGroup)
            , ECS_RO_COMP(sound_group_with_transform_ecs_query_comps, "transform", TMatrix)
            );

        }while (++comp != compE);
    }
  );
}
