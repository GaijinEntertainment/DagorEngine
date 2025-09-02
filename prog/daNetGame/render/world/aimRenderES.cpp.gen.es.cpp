// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "aimRenderES.cpp.inl"
ECS_DEF_PULL_VAR(aimRender);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc query_aim_data_ecs_query_comps[] =
{
//start of 7 ro components at [0]
  {ECS_HASH("aim_data__lensNodeId"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("aim_data__lensCollisionNodeId"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("aim_data__lensBoundingSphereRadius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("aim_data__farDofEnabled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("aim_data__lensRenderEnabled"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("aim_data__entityWithScopeLensEid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("camera__active"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc query_aim_data_ecs_query_desc
(
  "query_aim_data_ecs_query",
  empty_span(),
  make_span(query_aim_data_ecs_query_comps+0, 7)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void query_aim_data_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, query_aim_data_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(ECS_RO_COMP(query_aim_data_ecs_query_comps, "camera__active", bool)) )
            continue;
          function(
              ECS_RO_COMP(query_aim_data_ecs_query_comps, "aim_data__lensNodeId", int)
            , ECS_RO_COMP(query_aim_data_ecs_query_comps, "aim_data__lensCollisionNodeId", int)
            , ECS_RO_COMP(query_aim_data_ecs_query_comps, "aim_data__lensBoundingSphereRadius", float)
            , ECS_RO_COMP(query_aim_data_ecs_query_comps, "aim_data__farDofEnabled", bool)
            , ECS_RO_COMP(query_aim_data_ecs_query_comps, "aim_data__lensRenderEnabled", bool)
            , ECS_RO_COMP(query_aim_data_ecs_query_comps, "aim_data__entityWithScopeLensEid", ecs::EntityId)
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc check_if_lens_renderer_enabled_globally_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("lens_renderer_enabled_globally"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc check_if_lens_renderer_enabled_globally_ecs_query_desc
(
  "check_if_lens_renderer_enabled_globally_ecs_query",
  empty_span(),
  make_span(check_if_lens_renderer_enabled_globally_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void check_if_lens_renderer_enabled_globally_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, check_if_lens_renderer_enabled_globally_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(check_if_lens_renderer_enabled_globally_ecs_query_comps, "lens_renderer_enabled_globally", bool)
            );

        }while (++comp != compE);
    }
  );
}
