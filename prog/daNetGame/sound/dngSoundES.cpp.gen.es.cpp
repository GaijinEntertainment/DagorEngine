#include "dngSoundES.cpp.inl"
ECS_DEF_PULL_VAR(dngSound);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc sound_draw_debug_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("msg_sink"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void sound_draw_debug_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  G_UNUSED(components);
    sound_draw_debug_es(*info.cast<ecs::UpdateStageInfoRenderDebug>());
}
static ecs::EntitySystemDesc sound_draw_debug_es_es_desc
(
  "sound_draw_debug_es",
  "prog/daNetGame/sound/dngSoundES.cpp.inl",
  ecs::EntitySystemOps(sound_draw_debug_es_all),
  empty_span(),
  empty_span(),
  make_span(sound_draw_debug_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoRenderDebug::STAGE)
,"dev,render,sound",nullptr,"*");
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
