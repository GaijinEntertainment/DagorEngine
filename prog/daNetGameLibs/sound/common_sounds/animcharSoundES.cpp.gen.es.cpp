#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t animchar_get_type();
static ecs::LTComponentList animchar_component(ECS_HASH("animchar"), animchar_get_type(), "prog/daNetGameLibs/sound/common_sounds/./animcharSoundES.cpp.inl", "", 0);
static constexpr ecs::component_t gun__owner_get_type();
static ecs::LTComponentList gun__owner_component(ECS_HASH("gun__owner"), gun__owner_get_type(), "prog/daNetGameLibs/sound/common_sounds/./animcharSoundES.cpp.inl", "", 0);
static constexpr ecs::component_t is_watched_sound_get_type();
static ecs::LTComponentList is_watched_sound_component(ECS_HASH("is_watched_sound"), is_watched_sound_get_type(), "prog/daNetGameLibs/sound/common_sounds/./animcharSoundES.cpp.inl", "", 0);
#include "animcharSoundES.cpp.inl"
ECS_DEF_PULL_VAR(animcharSound);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc animchar_sound_on_appear_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("animchar_sound"), ecs::ComponentTypeInfo<AnimcharSound>()},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
//start of 1 ro components at [2]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void animchar_sound_on_appear_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    animchar_sound_on_appear_es(evt
        , ECS_RO_COMP(animchar_sound_on_appear_es_comps, "eid", ecs::EntityId)
    , ECS_RW_COMP(animchar_sound_on_appear_es_comps, "animchar_sound", AnimcharSound)
    , ECS_RW_COMP(animchar_sound_on_appear_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc animchar_sound_on_appear_es_es_desc
(
  "animchar_sound_on_appear_es",
  "prog/daNetGameLibs/sound/common_sounds/./animcharSoundES.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar_sound_on_appear_es_all_events),
  make_span(animchar_sound_on_appear_es_comps+0, 2)/*rw*/,
  make_span(animchar_sound_on_appear_es_comps+2, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"sound");
static constexpr ecs::ComponentDesc human_animchar_sound_on_appear_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("human_animchar_sound"), ecs::ComponentTypeInfo<HumanAnimcharSound>()},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
//start of 1 ro components at [2]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void human_animchar_sound_on_appear_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    human_animchar_sound_on_appear_es(evt
        , ECS_RO_COMP(human_animchar_sound_on_appear_es_comps, "eid", ecs::EntityId)
    , ECS_RW_COMP(human_animchar_sound_on_appear_es_comps, "human_animchar_sound", HumanAnimcharSound)
    , ECS_RW_COMP(human_animchar_sound_on_appear_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc human_animchar_sound_on_appear_es_es_desc
(
  "human_animchar_sound_on_appear_es",
  "prog/daNetGameLibs/sound/common_sounds/./animcharSoundES.cpp.inl",
  ecs::EntitySystemOps(nullptr, human_animchar_sound_on_appear_es_all_events),
  make_span(human_animchar_sound_on_appear_es_comps+0, 2)/*rw*/,
  make_span(human_animchar_sound_on_appear_es_comps+2, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"sound");
static constexpr ecs::ComponentDesc animchar_sound_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("sound_irqs"), ecs::ComponentTypeInfo<ecs::SharedComponent<ecs::Object>>()}
};
static ecs::CompileTimeQueryDesc animchar_sound_ecs_query_desc
(
  "animchar_sound_ecs_query",
  empty_span(),
  make_span(animchar_sound_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void animchar_sound_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, animchar_sound_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(animchar_sound_ecs_query_comps, "sound_irqs", ecs::SharedComponent<ecs::Object>)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc human_animchar_sound_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("human_voice_sound__irqs"), ecs::ComponentTypeInfo<ecs::SharedComponent<ecs::Object>>()},
  {ECS_HASH("human_steps_sound__irqs"), ecs::ComponentTypeInfo<ecs::SharedComponent<ecs::Array>>()}
};
static ecs::CompileTimeQueryDesc human_animchar_sound_ecs_query_desc
(
  "human_animchar_sound_ecs_query",
  empty_span(),
  make_span(human_animchar_sound_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void human_animchar_sound_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, human_animchar_sound_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(human_animchar_sound_ecs_query_comps, "human_voice_sound__irqs", ecs::SharedComponent<ecs::Object>)
            , ECS_RO_COMP(human_animchar_sound_ecs_query_comps, "human_steps_sound__irqs", ecs::SharedComponent<ecs::Array>)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc human_melee_sound_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("human_melee_sound__irqs"), ecs::ComponentTypeInfo<ecs::SharedComponent<ecs::Object>>()}
};
static ecs::CompileTimeQueryDesc human_melee_sound_ecs_query_desc
(
  "human_melee_sound_ecs_query",
  empty_span(),
  make_span(human_melee_sound_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void human_melee_sound_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, human_melee_sound_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(human_melee_sound_ecs_query_comps, "human_melee_sound__irqs", ecs::SharedComponent<ecs::Object>)
            );

        }
    }
  );
}
static constexpr ecs::component_t animchar_get_type(){return ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>::type; }
static constexpr ecs::component_t gun__owner_get_type(){return ecs::ComponentTypeInfo<ecs::EntityId>::type; }
static constexpr ecs::component_t is_watched_sound_get_type(){return ecs::ComponentTypeInfo<bool>::type; }
