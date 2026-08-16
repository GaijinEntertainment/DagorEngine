// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "groundHolesRenderES.cpp.inl"
ECS_DEF_PULL_VAR(groundHolesRender);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc underground_zones_draw_debug_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("should_render_ground_holes"), ecs::ComponentTypeInfo<bool>()}
};
static void underground_zones_draw_debug_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    underground_zones_draw_debug_es(*info.cast<ecs::UpdateStageInfoRenderDebug>()
    , components.manager()
    , ECS_RO_COMP(underground_zones_draw_debug_es_comps, "should_render_ground_holes", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc underground_zones_draw_debug_es_es_desc
(
  "underground_zones_draw_debug_es",
  "prog/daNetGameLibs/ground_holes/render/groundHolesRenderES.cpp.inl",
  ecs::EntitySystemOps(underground_zones_draw_debug_es_all),
  empty_span(),
  make_span(underground_zones_draw_debug_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoRenderDebug::STAGE)
,"dev,render",nullptr,"*");
static constexpr ecs::ComponentDesc ground_holes_initialize_es_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("hmap_holes_scale_step_offset_varId"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("hmap_holes_temp_ofs_size_varId"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("should_render_ground_holes"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("holes"), ecs::ComponentTypeInfo<ecs::Point4List>()}
};
static void ground_holes_initialize_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    ground_holes_initialize_es(evt
        , ECS_RW_COMP(ground_holes_initialize_es_comps, "hmap_holes_scale_step_offset_varId", int)
    , ECS_RW_COMP(ground_holes_initialize_es_comps, "hmap_holes_temp_ofs_size_varId", int)
    , ECS_RW_COMP(ground_holes_initialize_es_comps, "should_render_ground_holes", bool)
    , ECS_RW_COMP(ground_holes_initialize_es_comps, "holes", ecs::Point4List)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc ground_holes_initialize_es_es_desc
(
  "ground_holes_initialize_es",
  "prog/daNetGameLibs/ground_holes/render/groundHolesRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, ground_holes_initialize_es_all_events),
  make_span(ground_holes_initialize_es_comps+0, 4)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc ground_holes_on_disappear_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("hmapHolesTex"), ecs::ComponentTypeInfo<UniqueTexWithShaderVar>()},
//start of 1 ro components at [1]
  {ECS_HASH("hmap_holes_scale_step_offset_varId"), ecs::ComponentTypeInfo<int>()}
};
static void ground_holes_on_disappear_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    ground_holes_on_disappear_es(evt
        , ECS_RO_COMP(ground_holes_on_disappear_es_comps, "hmap_holes_scale_step_offset_varId", int)
    , ECS_RW_COMP(ground_holes_on_disappear_es_comps, "hmapHolesTex", UniqueTexWithShaderVar)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc ground_holes_on_disappear_es_es_desc
(
  "ground_holes_on_disappear_es",
  "prog/daNetGameLibs/ground_holes/render/groundHolesRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, ground_holes_on_disappear_es_all_events),
  make_span(ground_holes_on_disappear_es_comps+0, 1)/*rw*/,
  make_span(ground_holes_on_disappear_es_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc ground_hole_render_es_comps[] =
{
//start of 9 rw components at [0]
  {ECS_HASH("hmapHolesTex"), ecs::ComponentTypeInfo<UniqueTexWithShaderVar>()},
  {ECS_HASH("hmapHolesTmpTex"), ecs::ComponentTypeInfo<UniqueTexWithShaderVar>()},
  {ECS_HASH("hmapHolesBuf"), ecs::ComponentTypeInfo<UniqueBufWithShaderVar>()},
  {ECS_HASH("hmapHolesProcessRenderer"), ecs::ComponentTypeInfo<PostFxRenderer>()},
  {ECS_HASH("hmapHolesMipmapRenderer"), ecs::ComponentTypeInfo<PostFxRenderer>()},
  {ECS_HASH("hmapHolesPrepareRenderer"), ecs::ComponentTypeInfo<ShadersECS>()},
  {ECS_HASH("should_render_ground_holes"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("holes"), ecs::ComponentTypeInfo<ecs::Point4List>()},
  {ECS_HASH("invalidate_bboxes"), ecs::ComponentTypeInfo<ecs::Point3List>()},
//start of 3 ro components at [9]
  {ECS_HASH("hmap_holes_scale_step_offset_varId"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("hmap_holes_temp_ofs_size_varId"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("heightmap_holes_process_cs"), ecs::ComponentTypeInfo<ComputeShader>()}
};
static void ground_hole_render_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    ground_hole_render_es(static_cast<const UpdateStageInfoRender&>(evt)
        , ECS_RW_COMP(ground_hole_render_es_comps, "hmapHolesTex", UniqueTexWithShaderVar)
    , ECS_RW_COMP(ground_hole_render_es_comps, "hmapHolesTmpTex", UniqueTexWithShaderVar)
    , ECS_RW_COMP(ground_hole_render_es_comps, "hmapHolesBuf", UniqueBufWithShaderVar)
    , ECS_RW_COMP(ground_hole_render_es_comps, "hmapHolesProcessRenderer", PostFxRenderer)
    , ECS_RW_COMP(ground_hole_render_es_comps, "hmapHolesMipmapRenderer", PostFxRenderer)
    , ECS_RW_COMP(ground_hole_render_es_comps, "hmapHolesPrepareRenderer", ShadersECS)
    , ECS_RW_COMP(ground_hole_render_es_comps, "should_render_ground_holes", bool)
    , ECS_RO_COMP(ground_hole_render_es_comps, "hmap_holes_scale_step_offset_varId", int)
    , ECS_RO_COMP(ground_hole_render_es_comps, "hmap_holes_temp_ofs_size_varId", int)
    , ECS_RW_COMP(ground_hole_render_es_comps, "holes", ecs::Point4List)
    , ECS_RW_COMP(ground_hole_render_es_comps, "invalidate_bboxes", ecs::Point3List)
    , ECS_RO_COMP(ground_hole_render_es_comps, "heightmap_holes_process_cs", ComputeShader)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc ground_hole_render_es_es_desc
(
  "ground_hole_render_es",
  "prog/daNetGameLibs/ground_holes/render/groundHolesRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, ground_hole_render_es_all_events),
  make_span(ground_hole_render_es_comps+0, 9)/*rw*/,
  make_span(ground_hole_render_es_comps+9, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoRender>::build(),
  0
,"render",nullptr,"*");
static constexpr ecs::ComponentDesc ground_holes_before_render_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("holes"), ecs::ComponentTypeInfo<ecs::Point4List>()},
  {ECS_HASH("invalidate_bboxes"), ecs::ComponentTypeInfo<ecs::Point3List>()},
//start of 1 ro components at [2]
  {ECS_HASH("should_render_ground_holes"), ecs::ComponentTypeInfo<bool>()}
};
static void ground_holes_before_render_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    ground_holes_before_render_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , components.manager()
    , ECS_RW_COMP(ground_holes_before_render_es_comps, "holes", ecs::Point4List)
    , ECS_RW_COMP(ground_holes_before_render_es_comps, "invalidate_bboxes", ecs::Point3List)
    , ECS_RO_COMP(ground_holes_before_render_es_comps, "should_render_ground_holes", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc ground_holes_before_render_es_es_desc
(
  "ground_holes_before_render_es",
  "prog/daNetGameLibs/ground_holes/render/groundHolesRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, ground_holes_before_render_es_all_events),
  make_span(ground_holes_before_render_es_comps+0, 2)/*rw*/,
  make_span(ground_holes_before_render_es_comps+2, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render",nullptr,nullptr,"animchar_before_render_es");
static constexpr ecs::ComponentDesc ground_holes_convar_helper_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("should_render_ground_holes"), ecs::ComponentTypeInfo<bool>()}
};
static void ground_holes_convar_helper_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    ground_holes_convar_helper_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , ECS_RW_COMP(ground_holes_convar_helper_es_comps, "should_render_ground_holes", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc ground_holes_convar_helper_es_es_desc
(
  "ground_holes_convar_helper_es",
  "prog/daNetGameLibs/ground_holes/render/groundHolesRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, ground_holes_convar_helper_es_all_events),
  make_span(ground_holes_convar_helper_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"dev,render",nullptr,"ground_holes_before_render_es","animchar_before_render_es");
static constexpr ecs::ComponentDesc ground_holes_render_when_event_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("should_render_ground_holes"), ecs::ComponentTypeInfo<bool>()}
};
static void ground_holes_render_when_event_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    ground_holes_render_when_event_es(evt
        , ECS_RW_COMP(ground_holes_render_when_event_es_comps, "should_render_ground_holes", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc ground_holes_render_when_event_es_es_desc
(
  "ground_holes_render_when_event_es",
  "prog/daNetGameLibs/ground_holes/render/groundHolesRenderES.cpp.inl",
  ecs::EntitySystemOps(nullptr, ground_holes_render_when_event_es_all_events),
  make_span(ground_holes_render_when_event_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<AfterDeviceReset,
                       EventLevelLoaded>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc spawn_hole_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("ground_hole_sphere_shape"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("ground_hole_shape_intersection"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc spawn_hole_ecs_query_desc
(
  "spawn_hole_ecs_query",
  empty_span(),
  make_span(spawn_hole_ecs_query_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void spawn_hole_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, spawn_hole_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(spawn_hole_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(spawn_hole_ecs_query_comps, "ground_hole_sphere_shape", bool)
            , ECS_RO_COMP(spawn_hole_ecs_query_comps, "ground_hole_shape_intersection", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc underground_zones_draw_debug_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 1 rq components at [1]
  {ECS_HASH("underground_zone"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc underground_zones_draw_debug_ecs_query_desc
(
  "underground_zones_draw_debug_ecs_query",
  empty_span(),
  make_span(underground_zones_draw_debug_ecs_query_comps+0, 1)/*ro*/,
  make_span(underground_zones_draw_debug_ecs_query_comps+1, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void underground_zones_draw_debug_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, underground_zones_draw_debug_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(underground_zones_draw_debug_ecs_query_comps, "transform", TMatrix)
            );

        }while (++comp != compE);
    }
  );
}
