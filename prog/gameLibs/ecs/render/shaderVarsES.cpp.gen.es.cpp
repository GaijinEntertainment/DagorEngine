#include "shaderVarsES.cpp.inl"
ECS_DEF_PULL_VAR(shaderVars);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc shader_vars_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("shader_vars__original_vars"), ecs::ComponentTypeInfo<ecs::Object>(), ecs::CDF_OPTIONAL},
//start of 1 ro components at [1]
  {ECS_HASH("shader_vars__vars"), ecs::ComponentTypeInfo<ecs::Object>()}
};
static void shader_vars_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    shader_vars_es_event_handler(evt
        , ECS_RO_COMP(shader_vars_es_event_handler_comps, "shader_vars__vars", ecs::Object)
    , ECS_RW_COMP_PTR(shader_vars_es_event_handler_comps, "shader_vars__original_vars", ecs::Object)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc shader_vars_es_event_handler_es_desc
(
  "shader_vars_es",
  "prog/gameLibs/ecs/render/shaderVarsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, shader_vars_es_event_handler_all_events),
  make_span(shader_vars_es_event_handler_comps+0, 1)/*rw*/,
  make_span(shader_vars_es_event_handler_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render",nullptr,"*");
static constexpr ecs::ComponentDesc shader_vars_track_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("shader_vars__vars"), ecs::ComponentTypeInfo<ecs::Object>()}
};
static void shader_vars_track_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    shader_vars_track_es_event_handler(evt
        , ECS_RO_COMP(shader_vars_track_es_event_handler_comps, "shader_vars__vars", ecs::Object)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc shader_vars_track_es_event_handler_es_desc
(
  "shader_vars_track_es",
  "prog/gameLibs/ecs/render/shaderVarsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, shader_vars_track_es_event_handler_all_events),
  empty_span(),
  make_span(shader_vars_track_es_event_handler_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,"render","shader_vars__vars","*");
static constexpr ecs::ComponentDesc shader_vars_destroyed_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("shader_vars__original_vars"), ecs::ComponentTypeInfo<ecs::Object>()}
};
static void shader_vars_destroyed_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    shader_vars_destroyed_es_event_handler(evt
        , ECS_RO_COMP(shader_vars_destroyed_es_event_handler_comps, "shader_vars__original_vars", ecs::Object)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc shader_vars_destroyed_es_event_handler_es_desc
(
  "shader_vars_destroyed_es",
  "prog/gameLibs/ecs/render/shaderVarsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, shader_vars_destroyed_es_event_handler_all_events),
  empty_span(),
  make_span(shader_vars_destroyed_es_event_handler_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,nullptr,nullptr,"*");
