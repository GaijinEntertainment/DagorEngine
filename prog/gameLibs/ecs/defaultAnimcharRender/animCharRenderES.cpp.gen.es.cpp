// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "animCharRenderES.cpp.inl"
ECS_DEF_PULL_VAR(animCharRender);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc animchar_before_render_es_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("animchar_bsph"), ecs::ComponentTypeInfo<vec4f>()},
  {ECS_HASH("animchar_bbox"), ecs::ComponentTypeInfo<bbox3f>()},
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<uint8_t>()},
//start of 2 ro components at [4]
  {ECS_HASH("animchar_node_wtm"), ecs::ComponentTypeInfo<AnimcharNodesMat44>()},
  {ECS_HASH("animchar_render__enabled"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL}
};
static void animchar_before_render_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    animchar_before_render_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(animchar_before_render_es_comps, "animchar_render", AnimV20::AnimcharRendComponent)
    , ECS_RO_COMP(animchar_before_render_es_comps, "animchar_node_wtm", AnimcharNodesMat44)
    , ECS_RW_COMP(animchar_before_render_es_comps, "animchar_bsph", vec4f)
    , ECS_RW_COMP(animchar_before_render_es_comps, "animchar_bbox", bbox3f)
    , ECS_RW_COMP(animchar_before_render_es_comps, "animchar_visbits", uint8_t)
    , ECS_RO_COMP_OR(animchar_before_render_es_comps, "animchar_render__enabled", bool(true))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc animchar_before_render_es_es_desc
(
  "animchar_before_render_es",
  "prog/gameLibs/ecs/defaultAnimcharRender/animCharRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar_before_render_es_all_events),
  make_span(animchar_before_render_es_comps+0, 4)/*rw*/,
  make_span(animchar_before_render_es_comps+4, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,nullptr,"after_camera_sync");
static constexpr ecs::ComponentDesc animchar_render_opaque_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<uint8_t>()},
//start of 1 ro components at [2]
  {ECS_HASH("animchar_bbox"), ecs::ComponentTypeInfo<bbox3f>()}
};
static void animchar_render_opaque_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    animchar_render_opaque_es(static_cast<const UpdateStageInfoRender&>(evt)
        , ECS_RW_COMP(animchar_render_opaque_es_comps, "animchar_render", AnimV20::AnimcharRendComponent)
    , ECS_RO_COMP(animchar_render_opaque_es_comps, "animchar_bbox", bbox3f)
    , ECS_RW_COMP(animchar_render_opaque_es_comps, "animchar_visbits", uint8_t)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc animchar_render_opaque_es_es_desc
(
  "animchar_render_opaque_es",
  "prog/gameLibs/ecs/defaultAnimcharRender/animCharRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar_render_opaque_es_all_events),
  make_span(animchar_render_opaque_es_comps+0, 2)/*rw*/,
  make_span(animchar_render_opaque_es_comps+2, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoRender>::build(),
  0
,"render",nullptr,"*");
static constexpr ecs::ComponentDesc animchar_render_trans_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
//start of 1 ro components at [1]
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<uint8_t>()}
};
static void animchar_render_trans_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoRenderTrans>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    animchar_render_trans_es(static_cast<const UpdateStageInfoRenderTrans&>(evt)
        , ECS_RW_COMP(animchar_render_trans_es_comps, "animchar_render", AnimV20::AnimcharRendComponent)
    , ECS_RO_COMP(animchar_render_trans_es_comps, "animchar_visbits", uint8_t)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc animchar_render_trans_es_es_desc
(
  "animchar_render_trans_es",
  "prog/gameLibs/ecs/defaultAnimcharRender/animCharRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar_render_trans_es_all_events),
  make_span(animchar_render_trans_es_comps+0, 1)/*rw*/,
  make_span(animchar_render_trans_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoRenderTrans>::build(),
  0
,"render",nullptr,"*");
