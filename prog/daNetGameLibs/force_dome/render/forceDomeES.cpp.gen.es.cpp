#include "forceDomeES.cpp.inl"
ECS_DEF_PULL_VAR(forceDome);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc force_dome_resources_created_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("force_dome_resources"), ecs::ComponentTypeInfo<ForceDomeResources>()},
//start of 5 ro components at [1]
  {ECS_HASH("force_dome__color"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("force_dome__texture_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("force_dome__brightness"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("force_dome__edge_thinness"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("force_dome__edge_intensity"), ecs::ComponentTypeInfo<float>()}
};
static void force_dome_resources_created_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    force_dome_resources_created_es(evt
        , ECS_RW_COMP(force_dome_resources_created_es_comps, "force_dome_resources", ForceDomeResources)
    , ECS_RO_COMP(force_dome_resources_created_es_comps, "force_dome__color", Point4)
    , ECS_RO_COMP(force_dome_resources_created_es_comps, "force_dome__texture_scale", float)
    , ECS_RO_COMP(force_dome_resources_created_es_comps, "force_dome__brightness", float)
    , ECS_RO_COMP(force_dome_resources_created_es_comps, "force_dome__edge_thinness", float)
    , ECS_RO_COMP(force_dome_resources_created_es_comps, "force_dome__edge_intensity", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc force_dome_resources_created_es_es_desc
(
  "force_dome_resources_created_es",
  "prog/daNetGameLibs/force_dome/render/forceDomeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, force_dome_resources_created_es_all_events),
  make_span(force_dome_resources_created_es_comps+0, 1)/*rw*/,
  make_span(force_dome_resources_created_es_comps+1, 5)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc force_dome_resources_updated_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("force_dome_resources"), ecs::ComponentTypeInfo<ForceDomeResources>()},
//start of 5 ro components at [1]
  {ECS_HASH("force_dome__color"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("force_dome__texture_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("force_dome__brightness"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("force_dome__edge_thinness"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("force_dome__edge_intensity"), ecs::ComponentTypeInfo<float>()}
};
static void force_dome_resources_updated_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    force_dome_resources_updated_es(evt
        , ECS_RW_COMP(force_dome_resources_updated_es_comps, "force_dome_resources", ForceDomeResources)
    , ECS_RO_COMP(force_dome_resources_updated_es_comps, "force_dome__color", Point4)
    , ECS_RO_COMP(force_dome_resources_updated_es_comps, "force_dome__texture_scale", float)
    , ECS_RO_COMP(force_dome_resources_updated_es_comps, "force_dome__brightness", float)
    , ECS_RO_COMP(force_dome_resources_updated_es_comps, "force_dome__edge_thinness", float)
    , ECS_RO_COMP(force_dome_resources_updated_es_comps, "force_dome__edge_intensity", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc force_dome_resources_updated_es_es_desc
(
  "force_dome_resources_updated_es",
  "prog/daNetGameLibs/force_dome/render/forceDomeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, force_dome_resources_updated_es_all_events),
  make_span(force_dome_resources_updated_es_comps+0, 1)/*rw*/,
  make_span(force_dome_resources_updated_es_comps+1, 5)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,"render","*");
static constexpr ecs::ComponentDesc force_dome_field_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("force_dome_resources"), ecs::ComponentTypeInfo<ForceDomeResources>()}
};
static void force_dome_field_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoRenderTrans>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    force_dome_field_es(static_cast<const UpdateStageInfoRenderTrans&>(evt)
        , ECS_RW_COMP(force_dome_field_es_comps, "force_dome_resources", ForceDomeResources)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc force_dome_field_es_es_desc
(
  "force_dome_field_es",
  "prog/daNetGameLibs/force_dome/render/forceDomeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, force_dome_field_es_all_events),
  make_span(force_dome_field_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoRenderTrans>::build(),
  0
,nullptr,nullptr,"animchar_render_trans_es");
static constexpr ecs::ComponentDesc force_dome_created_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 2 ro components at [1]
  {ECS_HASH("force_dome__radius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("force_dome__position"), ecs::ComponentTypeInfo<Point3>()},
//start of 1 no components at [3]
  {ECS_HASH("force_dome__disableChangeTransform"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void force_dome_created_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    force_dome_created_es(evt
        , ECS_RO_COMP(force_dome_created_es_comps, "force_dome__radius", float)
    , ECS_RO_COMP(force_dome_created_es_comps, "force_dome__position", Point3)
    , ECS_RW_COMP(force_dome_created_es_comps, "transform", TMatrix)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc force_dome_created_es_es_desc
(
  "force_dome_created_es",
  "prog/daNetGameLibs/force_dome/render/forceDomeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, force_dome_created_es_all_events),
  make_span(force_dome_created_es_comps+0, 1)/*rw*/,
  make_span(force_dome_created_es_comps+1, 2)/*ro*/,
  empty_span(),
  make_span(force_dome_created_es_comps+3, 1)/*no*/,
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc force_dome_after_reset_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("force_dome_resources"), ecs::ComponentTypeInfo<ForceDomeResources>()}
};
static void force_dome_after_reset_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    force_dome_after_reset_es(evt
        , ECS_RW_COMP(force_dome_after_reset_es_comps, "force_dome_resources", ForceDomeResources)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc force_dome_after_reset_es_es_desc
(
  "force_dome_after_reset_es",
  "prog/daNetGameLibs/force_dome/render/forceDomeES.cpp.inl",
  ecs::EntitySystemOps(nullptr, force_dome_after_reset_es_all_events),
  make_span(force_dome_after_reset_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<AfterDeviceReset>::build(),
  0
);
static constexpr ecs::ComponentDesc force_dome_render_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("force_dome__position"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("force_dome__radius"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc force_dome_render_ecs_query_desc
(
  "force_dome_render_ecs_query",
  empty_span(),
  make_span(force_dome_render_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void force_dome_render_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, force_dome_render_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(force_dome_render_ecs_query_comps, "force_dome__position", Point3)
            , ECS_RO_COMP(force_dome_render_ecs_query_comps, "force_dome__radius", float)
            );

        }while (++comp != compE);
    }
  );
}
