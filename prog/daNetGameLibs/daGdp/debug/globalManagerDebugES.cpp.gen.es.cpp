// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "globalManagerDebugES.cpp.inl"
ECS_DEF_PULL_VAR(globalManagerDebug);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc global_manager_debug_imgui_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("dagdp__global_manager"), ecs::ComponentTypeInfo<dagdp::GlobalManager>()}
};
static ecs::CompileTimeQueryDesc global_manager_debug_imgui_ecs_query_desc
(
  "dagdp::global_manager_debug_imgui_ecs_query",
  make_span(global_manager_debug_imgui_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void dagdp::global_manager_debug_imgui_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, global_manager_debug_imgui_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(global_manager_debug_imgui_ecs_query_comps, "dagdp__global_manager", dagdp::GlobalManager)
            );

        }while (++comp != compE);
    }
  );
}
