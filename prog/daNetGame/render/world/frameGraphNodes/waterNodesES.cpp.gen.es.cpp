// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "waterNodesES.cpp.inl"
ECS_DEF_PULL_VAR(waterNodes);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc create_water_nodes_es_comps[] ={};
static void create_water_nodes_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<OnCameraNodeConstruction>());
  create_water_nodes_es(static_cast<const OnCameraNodeConstruction&>(evt)
        );
}
static ecs::EntitySystemDesc create_water_nodes_es_es_desc
(
  "create_water_nodes_es",
  "prog/daNetGame/render/world/frameGraphNodes/waterNodesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_water_nodes_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnCameraNodeConstruction>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc create_water_refraction_stub_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("water"), ecs::ComponentTypeInfo<FFTWater>()}
};
static void create_water_refraction_stub_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  create_water_refraction_stub_es(evt
        , components.manager()
    );
}
static ecs::EntitySystemDesc create_water_refraction_stub_es_es_desc
(
  "create_water_refraction_stub_es",
  "prog/daNetGame/render/world/frameGraphNodes/waterNodesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_water_refraction_stub_es_all_events),
  empty_span(),
  empty_span(),
  make_span(create_water_refraction_stub_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc destroy_water_refraction_stub_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("water"), ecs::ComponentTypeInfo<FFTWater>()}
};
static void destroy_water_refraction_stub_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  destroy_water_refraction_stub_es(evt
        , components.manager()
    );
}
static ecs::EntitySystemDesc destroy_water_refraction_stub_es_es_desc
(
  "destroy_water_refraction_stub_es",
  "prog/daNetGame/render/world/frameGraphNodes/waterNodesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, destroy_water_refraction_stub_es_all_events),
  empty_span(),
  empty_span(),
  make_span(destroy_water_refraction_stub_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc recreate_water_refraction_stub_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("water_refraction_stub"), ecs::ComponentTypeInfo<UniqueTexWithShaderVar>()}
};
static void recreate_water_refraction_stub_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    recreate_water_refraction_stub_es(evt
        , ECS_RW_COMP(recreate_water_refraction_stub_es_comps, "water_refraction_stub", UniqueTexWithShaderVar)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc recreate_water_refraction_stub_es_es_desc
(
  "recreate_water_refraction_stub_es",
  "prog/daNetGame/render/world/frameGraphNodes/waterNodesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, recreate_water_refraction_stub_es_all_events),
  make_span(recreate_water_refraction_stub_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<AfterDeviceReset>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc water_refraction_stub_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("water_refraction_stub"), ecs::ComponentTypeInfo<UniqueTexWithShaderVar>()}
};
static ecs::CompileTimeQueryDesc water_refraction_stub_ecs_query_desc
(
  "water_refraction_stub_ecs_query",
  make_span(water_refraction_stub_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void water_refraction_stub_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, water_refraction_stub_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(water_refraction_stub_ecs_query_comps, "water_refraction_stub", UniqueTexWithShaderVar)
            );

        }while (++comp != compE);
    }
  );
}
