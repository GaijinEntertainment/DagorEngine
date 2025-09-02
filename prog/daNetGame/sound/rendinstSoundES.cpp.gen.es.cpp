// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "rendinstSoundES.cpp.inl"
ECS_DEF_PULL_VAR(rendinstSound);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc rendinst_tree_sound_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("rendinst_tree_sounds__fallingPath"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("rendinst_tree_sounds__falledPath"), ecs::ComponentTypeInfo<ecs::string>()}
};
static ecs::CompileTimeQueryDesc rendinst_tree_sound_ecs_query_desc
(
  "rendinst_tree_sound_ecs_query",
  empty_span(),
  make_span(rendinst_tree_sound_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void rendinst_tree_sound_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, rendinst_tree_sound_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(rendinst_tree_sound_ecs_query_comps, "rendinst_tree_sounds__fallingPath", ecs::string)
            , ECS_RO_COMP(rendinst_tree_sound_ecs_query_comps, "rendinst_tree_sounds__falledPath", ecs::string)
            );

        }while (++comp != compE);
    }
  );
}
