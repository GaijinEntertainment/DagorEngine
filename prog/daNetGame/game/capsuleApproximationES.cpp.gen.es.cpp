// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "capsuleApproximationES.cpp.inl"
ECS_DEF_PULL_VAR(capsuleApproximation);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc debug_draw_capsule_approximation_es_comps[] ={};
static void debug_draw_capsule_approximation_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  G_UNUSED(components);
    debug_draw_capsule_approximation_es(*info.cast<UpdateStageInfoRenderDebug>());
}
static ecs::EntitySystemDesc debug_draw_capsule_approximation_es_es_desc
(
  "debug_draw_capsule_approximation_es",
  "prog/daNetGame/game/capsuleApproximationES.cpp.inl",
  ecs::EntitySystemOps(debug_draw_capsule_approximation_es_all),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<UpdateStageInfoRenderDebug::STAGE)
,"dev,render",nullptr,"*");
static constexpr ecs::ComponentDesc capsule_approximation_debug_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("capsule_approximation"), ecs::ComponentTypeInfo<ecs::SharedComponent<CapsuleApproximation>>()},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()}
};
static ecs::CompileTimeQueryDesc capsule_approximation_debug_ecs_query_desc
(
  "capsule_approximation_debug_ecs_query",
  empty_span(),
  make_span(capsule_approximation_debug_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void capsule_approximation_debug_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, capsule_approximation_debug_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(capsule_approximation_debug_ecs_query_comps, "capsule_approximation", ecs::SharedComponent<CapsuleApproximation>)
            , ECS_RO_COMP(capsule_approximation_debug_ecs_query_comps, "animchar", AnimV20::AnimcharBaseComponent)
            );

        }while (++comp != compE);
    }
  );
}
