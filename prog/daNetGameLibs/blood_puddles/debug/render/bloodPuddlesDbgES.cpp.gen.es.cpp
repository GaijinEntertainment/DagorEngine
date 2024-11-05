#include "bloodPuddlesDbgES.cpp.inl"
ECS_DEF_PULL_VAR(bloodPuddlesDbg);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc init_blood_puddles_node_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("blood_puddle_dbg__render_node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()}
};
static void init_blood_puddles_node_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_blood_puddles_node_es(evt
        , ECS_RW_COMP(init_blood_puddles_node_es_comps, "blood_puddle_dbg__render_node", dabfg::NodeHandle)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_blood_puddles_node_es_es_desc
(
  "init_blood_puddles_node_es",
  "prog/daNetGameLibs/blood_puddles/debug/render/bloodPuddlesDbgES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_blood_puddles_node_es_all_events),
  make_span(init_blood_puddles_node_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
