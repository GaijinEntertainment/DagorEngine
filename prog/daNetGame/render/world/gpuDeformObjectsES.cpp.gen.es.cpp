#include "gpuDeformObjectsES.cpp.inl"
ECS_DEF_PULL_VAR(gpuDeformObjects);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc process_deformables_ecs_query_comps[] =
{
//start of 4 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("deform_bbox__bmin"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("deform_bbox__bmax"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("transform_lastFrame"), ecs::ComponentTypeInfo<TMatrix>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc process_deformables_ecs_query_desc
(
  "process_deformables_ecs_query",
  empty_span(),
  make_span(process_deformables_ecs_query_comps+0, 4)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void process_deformables_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, process_deformables_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(process_deformables_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(process_deformables_ecs_query_comps, "deform_bbox__bmin", Point3)
            , ECS_RO_COMP(process_deformables_ecs_query_comps, "deform_bbox__bmax", Point3)
            , ECS_RO_COMP_PTR(process_deformables_ecs_query_comps, "transform_lastFrame", TMatrix)
            );

        }while (++comp != compE);
    }
  );
}
