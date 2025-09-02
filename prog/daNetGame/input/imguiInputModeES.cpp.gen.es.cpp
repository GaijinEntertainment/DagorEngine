// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "imguiInputModeES.cpp.inl"
ECS_DEF_PULL_VAR(imguiInputMode);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc update_hybrid_imgui_input_es_comps[] ={};
static void update_hybrid_imgui_input_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  G_UNUSED(components);
    update_hybrid_imgui_input_es(*info.cast<ecs::UpdateStageInfoAct>());
}
static ecs::EntitySystemDesc update_hybrid_imgui_input_es_es_desc
(
  "update_hybrid_imgui_input_es",
  "prog/daNetGame/input/imguiInputModeES.cpp.inl",
  ecs::EntitySystemOps(update_hybrid_imgui_input_es_all),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"dev,render",nullptr,"*");
