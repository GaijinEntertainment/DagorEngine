// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "physmapPatchES.cpp.inl"
ECS_DEF_PULL_VAR(physmapPatch);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc exec_physmap_patch_data_creation_request_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 rq components at [1]
  {ECS_HASH("create_physmap_patch_data"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void exec_physmap_patch_data_creation_request_es_event_handler_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    exec_physmap_patch_data_creation_request_es_event_handler(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RO_COMP(exec_physmap_patch_data_creation_request_es_event_handler_comps, "eid", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc exec_physmap_patch_data_creation_request_es_event_handler_es_desc
(
  "exec_physmap_patch_data_creation_request_es",
  "prog/daNetGame/render/world/physmapPatchES.cpp.inl",
  ecs::EntitySystemOps(exec_physmap_patch_data_creation_request_es_event_handler_all),
  empty_span(),
  make_span(exec_physmap_patch_data_creation_request_es_event_handler_comps+0, 1)/*ro*/,
  make_span(exec_physmap_patch_data_creation_request_es_event_handler_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"render",nullptr,"*");
static constexpr ecs::ComponentDesc init_physmap_patch_es_event_handler_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("physmap_patch_tex"), ecs::ComponentTypeInfo<UniqueTexHolder>()},
  {ECS_HASH("physmap_patch_invscale"), ecs::ComponentTypeInfo<float>()},
//start of 1 ro components at [2]
  {ECS_HASH("physmap_patch_tex_size"), ecs::ComponentTypeInfo<int>()}
};
static void init_physmap_patch_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_physmap_patch_es_event_handler(evt
        , ECS_RW_COMP(init_physmap_patch_es_event_handler_comps, "physmap_patch_tex", UniqueTexHolder)
    , ECS_RO_COMP(init_physmap_patch_es_event_handler_comps, "physmap_patch_tex_size", int)
    , ECS_RW_COMP(init_physmap_patch_es_event_handler_comps, "physmap_patch_invscale", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_physmap_patch_es_event_handler_es_desc
(
  "init_physmap_patch_es",
  "prog/daNetGame/render/world/physmapPatchES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_physmap_patch_es_event_handler_all_events),
  make_span(init_physmap_patch_es_event_handler_comps+0, 2)/*rw*/,
  make_span(init_physmap_patch_es_event_handler_comps+2, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc gather_physmap_patch_updated_regions_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("physmap_patch_last_update_pos"), ecs::ComponentTypeInfo<Point2>()},
//start of 2 ro components at [1]
  {ECS_HASH("physmap_patch_tex_size"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("physmap_patch_update_distance_squared"), ecs::ComponentTypeInfo<float>()}
};
static void gather_physmap_patch_updated_regions_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ParallelUpdateFrameDelayed>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    gather_physmap_patch_updated_regions_es(static_cast<const ParallelUpdateFrameDelayed&>(evt)
        , ECS_RO_COMP(gather_physmap_patch_updated_regions_es_comps, "physmap_patch_tex_size", int)
    , ECS_RO_COMP(gather_physmap_patch_updated_regions_es_comps, "physmap_patch_update_distance_squared", float)
    , ECS_RW_COMP(gather_physmap_patch_updated_regions_es_comps, "physmap_patch_last_update_pos", Point2)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc gather_physmap_patch_updated_regions_es_es_desc
(
  "gather_physmap_patch_updated_regions_es",
  "prog/daNetGame/render/world/physmapPatchES.cpp.inl",
  ecs::EntitySystemOps(nullptr, gather_physmap_patch_updated_regions_es_all_events),
  make_span(gather_physmap_patch_updated_regions_es_comps+0, 1)/*rw*/,
  make_span(gather_physmap_patch_updated_regions_es_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ParallelUpdateFrameDelayed>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc update_physmap_patch_tex_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("physmap_patch_tex"), ecs::ComponentTypeInfo<UniqueTexHolder>()},
//start of 2 ro components at [1]
  {ECS_HASH("physmap_patch_tex_size"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("physmap_patch_invscale"), ecs::ComponentTypeInfo<float>()}
};
static void update_physmap_patch_tex_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    update_physmap_patch_tex_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(update_physmap_patch_tex_es_comps, "physmap_patch_tex", UniqueTexHolder)
    , ECS_RO_COMP(update_physmap_patch_tex_es_comps, "physmap_patch_tex_size", int)
    , ECS_RO_COMP(update_physmap_patch_tex_es_comps, "physmap_patch_invscale", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_physmap_patch_tex_es_es_desc
(
  "update_physmap_patch_tex_es",
  "prog/daNetGame/render/world/physmapPatchES.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_physmap_patch_tex_es_all_events),
  make_span(update_physmap_patch_tex_es_comps+0, 1)/*rw*/,
  make_span(update_physmap_patch_tex_es_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,nullptr,"animchar_before_render_es");
