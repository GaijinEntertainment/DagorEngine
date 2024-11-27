#include "connectivityES.cpp.inl"
ECS_DEF_PULL_VAR(connectivity);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc connectivity_state_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("ui_state__connectivity"), ecs::ComponentTypeInfo<int>()}
};
static void connectivity_state_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    connectivity_state_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(connectivity_state_es_comps, "ui_state__connectivity", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc connectivity_state_es_es_desc
(
  "connectivity_state_es",
  "prog/daNetGame/ui/connectivityES.cpp.inl",
  ecs::EntitySystemOps(nullptr, connectivity_state_es_all_events),
  make_span(connectivity_state_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"netClient,render,ui",nullptr,nullptr,"animchar_before_render_es");
