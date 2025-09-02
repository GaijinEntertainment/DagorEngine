#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t blood_puddle_dbg__mode_get_type();
static ecs::LTComponentList blood_puddle_dbg__mode_component(ECS_HASH("blood_puddle_dbg__mode"), blood_puddle_dbg__mode_get_type(), "prog/daNetGameLibs/blood_puddles/debug/render/bloodPuddlesDbgES.cpp.inl", "", 0);
// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "bloodPuddlesDbgES.cpp.inl"
ECS_DEF_PULL_VAR(bloodPuddlesDbg);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc init_blood_puddles_node_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("blood_puddle_dbg__render_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
//start of 1 ro components at [1]
  {ECS_HASH("blood_puddle_dbg__mode"), ecs::ComponentTypeInfo<int>()}
};
static void init_blood_puddles_node_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_blood_puddles_node_es(evt
        , ECS_RW_COMP(init_blood_puddles_node_es_comps, "blood_puddle_dbg__render_node", dafg::NodeHandle)
    , ECS_RO_COMP(init_blood_puddles_node_es_comps, "blood_puddle_dbg__mode", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_blood_puddles_node_es_es_desc
(
  "init_blood_puddles_node_es",
  "prog/daNetGameLibs/blood_puddles/debug/render/bloodPuddlesDbgES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_blood_puddles_node_es_all_events),
  make_span(init_blood_puddles_node_es_comps+0, 1)/*rw*/,
  make_span(init_blood_puddles_node_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::component_t blood_puddle_dbg__mode_get_type(){return ecs::ComponentTypeInfo<int>::type; }
