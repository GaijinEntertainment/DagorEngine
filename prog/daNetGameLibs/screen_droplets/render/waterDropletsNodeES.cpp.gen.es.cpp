// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "waterDropletsNodeES.cpp.inl"
ECS_DEF_PULL_VAR(waterDropletsNode);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc update_water_droplets_node_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("water_droplets_node"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("screen_droplets__visible"), ecs::ComponentTypeInfo<bool>()}
};
static void update_water_droplets_node_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    update_water_droplets_node_es(evt
        , ECS_RW_COMP(update_water_droplets_node_es_comps, "water_droplets_node", dafg::NodeHandle)
    , ECS_RW_COMP(update_water_droplets_node_es_comps, "screen_droplets__visible", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_water_droplets_node_es_es_desc
(
  "update_water_droplets_node_es",
  "prog/daNetGameLibs/screen_droplets/render/waterDropletsNodeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_water_droplets_node_es_all_events),
  make_span(update_water_droplets_node_es_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc register_screen_droplets_for_postfx_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("screen_droplets__visible"), ecs::ComponentTypeInfo<bool>()}
};
static void register_screen_droplets_for_postfx_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<RegisterPostfxResources>());
  register_screen_droplets_for_postfx_es(static_cast<const RegisterPostfxResources&>(evt)
        );
}
static ecs::EntitySystemDesc register_screen_droplets_for_postfx_es_es_desc
(
  "register_screen_droplets_for_postfx_es",
  "prog/daNetGameLibs/screen_droplets/render/waterDropletsNodeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, register_screen_droplets_for_postfx_es_all_events),
  empty_span(),
  empty_span(),
  make_span(register_screen_droplets_for_postfx_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<RegisterPostfxResources>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc find_water_droplets_needs_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("needsWaterDroplets"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc find_water_droplets_needs_ecs_query_desc
(
  "find_water_droplets_needs_ecs_query",
  empty_span(),
  make_span(find_water_droplets_needs_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline ecs::QueryCbResult find_water_droplets_needs_ecs_query(Callable function)
{
  return perform_query(g_entity_mgr, find_water_droplets_needs_ecs_query_desc.getHandle(),
    ecs::stoppable_query_cb_t([&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if (function(
              ECS_RO_COMP(find_water_droplets_needs_ecs_query_comps, "needsWaterDroplets", ecs::Tag)
            ) == ecs::QueryCbResult::Stop)
            return ecs::QueryCbResult::Stop;
        }while (++comp != compE);
          return ecs::QueryCbResult::Continue;
    })
  );
}
