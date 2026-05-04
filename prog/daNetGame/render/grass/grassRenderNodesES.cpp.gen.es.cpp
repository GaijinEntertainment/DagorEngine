// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "grassRenderNodesES.cpp.inl"
ECS_DEF_PULL_VAR(grassRenderNodes);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc init_grass_render_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("grass_per_camera_res_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("grass_generation_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("grass_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
//start of 1 rq components at [3]
  {ECS_HASH("grass_render"), ecs::ComponentTypeInfo<GrassRenderer>()}
};
static void init_grass_render_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_grass_render_es(evt
        , components.manager()
    , ECS_RW_COMP(init_grass_render_es_comps, "grass_per_camera_res_node", dafg::NodeHandle)
    , ECS_RW_COMP(init_grass_render_es_comps, "grass_generation_node", dafg::NodeHandle)
    , ECS_RW_COMP(init_grass_render_es_comps, "grass_node", dafg::NodeHandle)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_grass_render_es_es_desc
(
  "init_grass_render_es",
  "prog/daNetGame/render/grass/grassRenderNodesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_grass_render_es_all_events),
  make_span(init_grass_render_es_comps+0, 3)/*rw*/,
  empty_span(),
  make_span(init_grass_render_es_comps+3, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ChangeRenderFeatures,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
//static constexpr ecs::ComponentDesc create_grass_prepass_nodes_es_comps[] ={};
static void create_grass_prepass_nodes_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<OnCameraNodeConstruction>());
  create_grass_prepass_nodes_es(static_cast<const OnCameraNodeConstruction&>(evt)
        );
}
static ecs::EntitySystemDesc create_grass_prepass_nodes_es_es_desc
(
  "create_grass_prepass_nodes_es",
  "prog/daNetGame/render/grass/grassRenderNodesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_grass_prepass_nodes_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnCameraNodeConstruction>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc get_grass_render_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("grass_render"), ecs::ComponentTypeInfo<GrassRenderer>()}
};
static ecs::CompileTimeQueryDesc get_grass_render_ecs_query_desc
(
  "get_grass_render_ecs_query",
  make_span(get_grass_render_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_grass_render_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, get_grass_render_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(get_grass_render_ecs_query_comps, "grass_render", GrassRenderer)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc init_grass_render_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("grass_render"), ecs::ComponentTypeInfo<GrassRenderer>()}
};
static ecs::CompileTimeQueryDesc init_grass_render_ecs_query_desc
(
  "init_grass_render_ecs_query",
  make_span(init_grass_render_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void init_grass_render_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, init_grass_render_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(init_grass_render_ecs_query_comps, "grass_render", GrassRenderer)
            );

        }while (++comp != compE);
    }
  );
}
