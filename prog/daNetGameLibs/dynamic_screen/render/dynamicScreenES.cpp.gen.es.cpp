// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "dynamicScreenES.cpp.inl"
ECS_DEF_PULL_VAR(dynamicScreen);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc dynamic_screen_on_appear_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("dynamic_screen__texture"), ecs::ComponentTypeInfo<SharedTex>()},
  {ECS_HASH("animchar_render"), ecs::ComponentTypeInfo<AnimV20::AnimcharRendComponent>()},
//start of 2 ro components at [2]
  {ECS_HASH("dynamic_screen__texture_size"), ecs::ComponentTypeInfo<IPoint2>()},
  {ECS_HASH("dynamic_screen__texture_mips"), ecs::ComponentTypeInfo<int>()}
};
static void dynamic_screen_on_appear_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    dynamic_screen_on_appear_es(evt
        , ECS_RW_COMP(dynamic_screen_on_appear_es_comps, "dynamic_screen__texture", SharedTex)
    , ECS_RO_COMP(dynamic_screen_on_appear_es_comps, "dynamic_screen__texture_size", IPoint2)
    , ECS_RO_COMP(dynamic_screen_on_appear_es_comps, "dynamic_screen__texture_mips", int)
    , ECS_RW_COMP(dynamic_screen_on_appear_es_comps, "animchar_render", AnimV20::AnimcharRendComponent)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc dynamic_screen_on_appear_es_es_desc
(
  "dynamic_screen_on_appear_es",
  "prog/daNetGameLibs/dynamic_screen/render/dynamicScreenES.cpp.inl",
  ecs::EntitySystemOps(nullptr, dynamic_screen_on_appear_es_all_events),
  make_span(dynamic_screen_on_appear_es_comps+0, 2)/*rw*/,
  make_span(dynamic_screen_on_appear_es_comps+2, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
