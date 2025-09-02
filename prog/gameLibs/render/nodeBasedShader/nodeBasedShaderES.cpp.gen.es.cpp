// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "nodeBasedShaderES.cpp.inl"
ECS_DEF_PULL_VAR(nodeBasedShader);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc gather_nbs_sphere_managers_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("nbs_spheres_manager__class_name"), ecs::ComponentTypeInfo<ecs::string>()}
};
static ecs::CompileTimeQueryDesc gather_nbs_sphere_managers_ecs_query_desc
(
  "gather_nbs_sphere_managers_ecs_query",
  empty_span(),
  make_span(gather_nbs_sphere_managers_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void gather_nbs_sphere_managers_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, gather_nbs_sphere_managers_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(gather_nbs_sphere_managers_ecs_query_comps, "nbs_spheres_manager__class_name", ecs::string)
            );

        }while (++comp != compE);
    }
  );
}
