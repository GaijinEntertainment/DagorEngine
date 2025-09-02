// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "nbsVolumesES.cpp.inl"
ECS_DEF_PULL_VAR(nbsVolumes);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc update_nbs_volumes_es_comps[] ={};
static void update_nbs_volumes_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  update_nbs_volumes_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        );
}
static ecs::EntitySystemDesc update_nbs_volumes_es_es_desc
(
  "update_nbs_volumes_es",
  "prog/daNetGameLibs/nbs_volumes/render/nbsVolumesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_nbs_volumes_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,nullptr,"animchar_before_render_es");
static constexpr ecs::ComponentDesc destroy_nbs_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("nbs_spheres_manager"), ecs::ComponentTypeInfo<NBSSpheresManager>()}
};
static void destroy_nbs_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    destroy_nbs_es_event_handler(evt
        , ECS_RW_COMP(destroy_nbs_es_event_handler_comps, "nbs_spheres_manager", NBSSpheresManager)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc destroy_nbs_es_event_handler_es_desc
(
  "destroy_nbs_es",
  "prog/daNetGameLibs/nbs_volumes/render/nbsVolumesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, destroy_nbs_es_event_handler_all_events),
  make_span(destroy_nbs_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
);
static constexpr ecs::ComponentDesc gather_nbs_spheres_ecs_query_comps[] =
{
//start of 5 ro components at [0]
  {ECS_HASH("nbs_sphere__class_name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("nbs_sphere__center_density"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("nbs_sphere__radius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("nbs_sphere__edge_density"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc gather_nbs_spheres_ecs_query_desc
(
  "gather_nbs_spheres_ecs_query",
  empty_span(),
  make_span(gather_nbs_spheres_ecs_query_comps+0, 5)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void gather_nbs_spheres_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, gather_nbs_spheres_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(gather_nbs_spheres_ecs_query_comps, "nbs_sphere__class_name", ecs::string)
            , ECS_RO_COMP(gather_nbs_spheres_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(gather_nbs_spheres_ecs_query_comps, "nbs_sphere__center_density", float)
            , ECS_RO_COMP(gather_nbs_spheres_ecs_query_comps, "nbs_sphere__radius", float)
            , ECS_RO_COMP(gather_nbs_spheres_ecs_query_comps, "nbs_sphere__edge_density", float)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc update_nbs_buffers_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("nbs_spheres_manager"), ecs::ComponentTypeInfo<NBSSpheresManager>()},
//start of 1 ro components at [1]
  {ECS_HASH("nbs_spheres_manager__class_name"), ecs::ComponentTypeInfo<ecs::string>()}
};
static ecs::CompileTimeQueryDesc update_nbs_buffers_ecs_query_desc
(
  "update_nbs_buffers_ecs_query",
  make_span(update_nbs_buffers_ecs_query_comps+0, 1)/*rw*/,
  make_span(update_nbs_buffers_ecs_query_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void update_nbs_buffers_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, update_nbs_buffers_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(update_nbs_buffers_ecs_query_comps, "nbs_spheres_manager", NBSSpheresManager)
            , ECS_RO_COMP(update_nbs_buffers_ecs_query_comps, "nbs_spheres_manager__class_name", ecs::string)
            );

        }while (++comp != compE);
    }
  );
}
