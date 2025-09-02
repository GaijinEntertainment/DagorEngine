// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "dngSoundES.cpp.inl"
ECS_DEF_PULL_VAR(dngSound);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc dng_sound_debug_draw_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("msg_sink"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void dng_sound_debug_draw_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  G_UNUSED(components);
    dng_sound_debug_draw_es(*info.cast<ecs::UpdateStageInfoRenderDebug>());
}
static ecs::EntitySystemDesc dng_sound_debug_draw_es_es_desc
(
  "dng_sound_debug_draw_es",
  "prog/daNetGame/sound/dngSoundES.cpp.inl",
  ecs::EntitySystemOps(dng_sound_debug_draw_es_all),
  empty_span(),
  empty_span(),
  make_span(dng_sound_debug_draw_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoRenderDebug::STAGE)
,"dev,render,sound",nullptr,"*");
static constexpr ecs::ComponentDesc dng_sound_set_cur_scene_name_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("msg_sink"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void dng_sound_set_cur_scene_name_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  dng_sound_set_cur_scene_name_es(evt
        );
}
static ecs::EntitySystemDesc dng_sound_set_cur_scene_name_es_es_desc
(
  "dng_sound_set_cur_scene_name_es",
  "prog/daNetGame/sound/dngSoundES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dng_sound_set_cur_scene_name_es_all_events),
  empty_span(),
  empty_span(),
  make_span(dng_sound_set_cur_scene_name_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<EventLevelLoaded>::build(),
  0
,"dev,sound");
static constexpr ecs::ComponentDesc sound_begin_update_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("msg_sink"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void sound_begin_update_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<ParallelUpdateFrameDelayed>());
  sound_begin_update_es(static_cast<const ParallelUpdateFrameDelayed&>(evt)
        );
}
static ecs::EntitySystemDesc sound_begin_update_es_es_desc
(
  "sound_begin_update_es",
  "prog/daNetGame/sound/dngSoundES.cpp.inl",
  ecs::EntitySystemOps(nullptr, sound_begin_update_es_all_events),
  empty_span(),
  empty_span(),
  make_span(sound_begin_update_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ParallelUpdateFrameDelayed>::build(),
  0
,"sound",nullptr,nullptr,"after_net_phys_sync");
static constexpr ecs::ComponentDesc sound_end_update_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("msg_sink"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void sound_end_update_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<ParallelUpdateFrameDelayed>());
  sound_end_update_es(static_cast<const ParallelUpdateFrameDelayed&>(evt)
        );
}
static ecs::EntitySystemDesc sound_end_update_es_es_desc
(
  "sound_end_update_es",
  "prog/daNetGame/sound/dngSoundES.cpp.inl",
  ecs::EntitySystemOps(nullptr, sound_end_update_es_all_events),
  empty_span(),
  empty_span(),
  make_span(sound_end_update_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ParallelUpdateFrameDelayed>::build(),
  0
,"sound",nullptr,nullptr,"sound_begin_update_es");
static constexpr ecs::ComponentDesc dng_sound_listener_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("dng_sound_listener_enabled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 1 rq components at [2]
  {ECS_HASH("dngSoundListener"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc dng_sound_listener_ecs_query_desc
(
  "dng_sound_listener_ecs_query",
  empty_span(),
  make_span(dng_sound_listener_ecs_query_comps+0, 2)/*ro*/,
  make_span(dng_sound_listener_ecs_query_comps+2, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void dng_sound_listener_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, dng_sound_listener_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(dng_sound_listener_ecs_query_comps, "dng_sound_listener_enabled", bool)
            , ECS_RO_COMP(dng_sound_listener_ecs_query_comps, "transform", TMatrix)
            );

        }while (++comp != compE);
    }
  );
}
