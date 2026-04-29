// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "billboardDecalsES.cpp.inl"
ECS_DEF_PULL_VAR(billboardDecals);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc bullet_holes_after_device_reset_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("billboard_decals__mgr"), ecs::ComponentTypeInfo<BillboardDecalsPtr>()}
};
static void bullet_holes_after_device_reset_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    bullet_holes_after_device_reset_es(evt
        , ECS_RW_COMP(bullet_holes_after_device_reset_es_comps, "billboard_decals__mgr", BillboardDecalsPtr)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc bullet_holes_after_device_reset_es_es_desc
(
  "bullet_holes_after_device_reset_es",
  "prog/daNetGameLibs/billboard_decals/render/billboardDecalsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, bullet_holes_after_device_reset_es_all_events),
  make_span(bullet_holes_after_device_reset_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<AfterDeviceReset>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc bullet_holes_before_render_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("billboard_decals__mgr"), ecs::ComponentTypeInfo<BillboardDecalsPtr>()}
};
static void bullet_holes_before_render_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    bullet_holes_before_render_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(bullet_holes_before_render_es_comps, "billboard_decals__mgr", BillboardDecalsPtr)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc bullet_holes_before_render_es_es_desc
(
  "bullet_holes_before_render_es",
  "prog/daNetGameLibs/billboard_decals/render/billboardDecalsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, bullet_holes_before_render_es_all_events),
  make_span(bullet_holes_before_render_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc bullet_holes_render_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("billboard_decals__mgr"), ecs::ComponentTypeInfo<BillboardDecalsPtr>()}
};
static void bullet_holes_render_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    bullet_holes_render_es(evt
        , ECS_RW_COMP(bullet_holes_render_es_comps, "billboard_decals__mgr", BillboardDecalsPtr)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc bullet_holes_render_es_es_desc
(
  "bullet_holes_render_es",
  "prog/daNetGameLibs/billboard_decals/render/billboardDecalsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, bullet_holes_render_es_all_events),
  make_span(bullet_holes_render_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnRenderDecals>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc bullet_holes_on_level_loaded_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("billboard_decals__mgr"), ecs::ComponentTypeInfo<BillboardDecalsPtr>()},
//start of 5 ro components at [1]
  {ECS_HASH("billboard_decals__maxDecals"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("billboard_decals__diffuseTextureArray"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("billboard_decals__normalsTextureArray"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("billboard_decals__depthHardDist"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("billboard_decals__depthSoftnessDist"), ecs::ComponentTypeInfo<float>()}
};
static void bullet_holes_on_level_loaded_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    bullet_holes_on_level_loaded_es(evt
        , ECS_RW_COMP(bullet_holes_on_level_loaded_es_comps, "billboard_decals__mgr", BillboardDecalsPtr)
    , ECS_RO_COMP(bullet_holes_on_level_loaded_es_comps, "billboard_decals__maxDecals", int)
    , ECS_RO_COMP(bullet_holes_on_level_loaded_es_comps, "billboard_decals__diffuseTextureArray", ecs::string)
    , ECS_RO_COMP(bullet_holes_on_level_loaded_es_comps, "billboard_decals__normalsTextureArray", ecs::string)
    , ECS_RO_COMP(bullet_holes_on_level_loaded_es_comps, "billboard_decals__depthHardDist", float)
    , ECS_RO_COMP(bullet_holes_on_level_loaded_es_comps, "billboard_decals__depthSoftnessDist", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc bullet_holes_on_level_loaded_es_es_desc
(
  "bullet_holes_on_level_loaded_es",
  "prog/daNetGameLibs/billboard_decals/render/billboardDecalsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, bullet_holes_on_level_loaded_es_all_events),
  make_span(bullet_holes_on_level_loaded_es_comps+0, 1)/*rw*/,
  make_span(bullet_holes_on_level_loaded_es_comps+1, 5)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnLevelLoaded>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc get_billboard_manager_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("billboard_decals__mgr"), ecs::ComponentTypeInfo<BillboardDecalsPtr>()}
};
static ecs::CompileTimeQueryDesc get_billboard_manager_ecs_query_desc
(
  "get_billboard_manager_ecs_query",
  make_span(get_billboard_manager_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_billboard_manager_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, get_billboard_manager_ecs_query_desc.getHandle(),
    ecs::stoppable_query_cb_t([&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if (function(
              ECS_RW_COMP(get_billboard_manager_ecs_query_comps, "billboard_decals__mgr", BillboardDecalsPtr)
            ) == ecs::QueryCbResult::Stop)
            return ecs::QueryCbResult::Stop;
        }while (++comp != compE);
          return ecs::QueryCbResult::Continue;
    })
  );
}
