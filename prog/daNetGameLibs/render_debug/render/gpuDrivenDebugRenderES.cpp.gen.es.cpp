// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "gpuDrivenDebugRenderES.cpp.inl"
ECS_DEF_PULL_VAR(gpuDrivenDebugRender);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc query_gpu_debug_render_for_prepare_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("gpu_driven_debug_render"), ecs::ComponentTypeInfo<GpuDrivenDebugRender>()}
};
static ecs::CompileTimeQueryDesc query_gpu_debug_render_for_prepare_ecs_query_desc
(
  "query_gpu_debug_render_for_prepare_ecs_query",
  make_span(query_gpu_debug_render_for_prepare_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void query_gpu_debug_render_for_prepare_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, query_gpu_debug_render_for_prepare_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(query_gpu_debug_render_for_prepare_ecs_query_comps, "gpu_driven_debug_render", GpuDrivenDebugRender)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc query_gpu_debug_render_for_render_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("gpu_driven_debug_render"), ecs::ComponentTypeInfo<GpuDrivenDebugRender>()}
};
static ecs::CompileTimeQueryDesc query_gpu_debug_render_for_render_ecs_query_desc
(
  "query_gpu_debug_render_for_render_ecs_query",
  make_span(query_gpu_debug_render_for_render_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void query_gpu_debug_render_for_render_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, query_gpu_debug_render_for_render_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(query_gpu_debug_render_for_render_ecs_query_comps, "gpu_driven_debug_render", GpuDrivenDebugRender)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc query_gpu_debug_render_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("gpu_driven_debug_render"), ecs::ComponentTypeInfo<GpuDrivenDebugRender>()}
};
static ecs::CompileTimeQueryDesc query_gpu_debug_render_ecs_query_desc
(
  "query_gpu_debug_render_ecs_query",
  make_span(query_gpu_debug_render_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void query_gpu_debug_render_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, query_gpu_debug_render_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(query_gpu_debug_render_ecs_query_comps, "gpu_driven_debug_render", GpuDrivenDebugRender)
            );

        }while (++comp != compE);
    }
  );
}
