#include "toolAnimcharES.cpp.inl"
ECS_DEF_PULL_VAR(toolAnimchar);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc animchar_act_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("animchar_node_wtm"), ecs::ComponentTypeInfo<AnimcharNodesMat44>()},
  {ECS_HASH("animchar_render__root_pos"), ecs::ComponentTypeInfo<vec3f>()},
//start of 1 ro components at [3]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()}
};
static void animchar_act_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    animchar_act_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RW_COMP(animchar_act_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    , ECS_RO_COMP(animchar_act_es_comps, "transform", TMatrix)
    , ECS_RW_COMP(animchar_act_es_comps, "animchar_node_wtm", AnimcharNodesMat44)
    , ECS_RW_COMP(animchar_act_es_comps, "animchar_render__root_pos", vec3f)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc animchar_act_es_es_desc
(
  "animchar_act_es",
  "prog/tools/sceneTools/daEditorX/services/animCharMgr/toolAnimcharES.cpp.inl",
  ecs::EntitySystemOps(animchar_act_es_all),
  make_span(animchar_act_es_comps+0, 3)/*rw*/,
  make_span(animchar_act_es_comps+3, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,nullptr,nullptr,"*");
static constexpr ecs::ComponentDesc animchar_before_render_es_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
  {ECS_HASH("animchar_bsph"), ecs::ComponentTypeInfo<vec4f>()},
  {ECS_HASH("animchar_bbox"), ecs::ComponentTypeInfo<bbox3f>()},
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<uint8_t>()},
//start of 2 ro components at [4]
  {ECS_HASH("animchar_node_wtm"), ecs::ComponentTypeInfo<AnimcharNodesMat44>()},
  {ECS_HASH("animchar_render__enabled"), ecs::ComponentTypeInfo<bool>()}
};
static void animchar_before_render_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<BeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    animchar_before_render_es(static_cast<const BeforeRender&>(evt)
        , ECS_RW_COMP(animchar_before_render_es_comps, "animchar_render", AnimV20::AnimcharRendComponent)
    , ECS_RO_COMP(animchar_before_render_es_comps, "animchar_node_wtm", AnimcharNodesMat44)
    , ECS_RW_COMP(animchar_before_render_es_comps, "animchar_bsph", vec4f)
    , ECS_RW_COMP(animchar_before_render_es_comps, "animchar_bbox", bbox3f)
    , ECS_RW_COMP(animchar_before_render_es_comps, "animchar_visbits", uint8_t)
    , ECS_RO_COMP(animchar_before_render_es_comps, "animchar_render__enabled", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc animchar_before_render_es_es_desc
(
  "animchar_before_render_es",
  "prog/tools/sceneTools/daEditorX/services/animCharMgr/toolAnimcharES.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar_before_render_es_all_events),
  make_span(animchar_before_render_es_comps+0, 4)/*rw*/,
  make_span(animchar_before_render_es_comps+4, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<BeforeRender>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc animchar_render_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
//start of 2 ro components at [1]
  {ECS_HASH("animchar_bbox"), ecs::ComponentTypeInfo<bbox3f>()},
  {ECS_HASH("animchar_visbits"), ecs::ComponentTypeInfo<uint8_t>()}
};
static void animchar_render_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<RenderStage>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    animchar_render_es(static_cast<const RenderStage&>(evt)
        , ECS_RW_COMP(animchar_render_es_comps, "animchar_render", AnimV20::AnimcharRendComponent)
    , ECS_RO_COMP(animchar_render_es_comps, "animchar_bbox", bbox3f)
    , ECS_RO_COMP(animchar_render_es_comps, "animchar_visbits", uint8_t)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc animchar_render_es_es_desc
(
  "animchar_render_es",
  "prog/tools/sceneTools/daEditorX/services/animCharMgr/toolAnimcharES.cpp.inl",
  ecs::EntitySystemOps(nullptr, animchar_render_es_all_events),
  make_span(animchar_render_es_comps+0, 1)/*rw*/,
  make_span(animchar_render_es_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<RenderStage>::build(),
  0
,"render",nullptr,"*");
