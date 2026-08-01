// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "splineGenGeometryRepositoryES.cpp.inl"
ECS_DEF_PULL_VAR(splineGenGeometryRepository);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc create_transparent_spline_triangle_debug_es_comps[] ={};
static void create_transparent_spline_triangle_debug_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<CreateTriangleDebugNodes>());
  create_transparent_spline_triangle_debug_es(static_cast<const CreateTriangleDebugNodes&>(evt)
        );
}
static ecs::EntitySystemDesc create_transparent_spline_triangle_debug_es_es_desc
(
  "create_transparent_spline_triangle_debug_es",
  "prog/daNetGameLibs/spline_geometry/render/splineGenGeometryRepositoryES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_transparent_spline_triangle_debug_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<CreateTriangleDebugNodes>::build(),
  0
,"dev,render");
static constexpr ecs::ComponentDesc spline_gen_view_nodes_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("spline_gen_repository"), ecs::ComponentTypeInfo<SplineGenGeometryRepository>()}
};
static void spline_gen_view_nodes_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<OnCameraPerViewNodeConstruction>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    spline_gen_view_nodes_es(static_cast<const OnCameraPerViewNodeConstruction&>(evt)
        , ECS_RO_COMP(spline_gen_view_nodes_es_comps, "spline_gen_repository", SplineGenGeometryRepository)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc spline_gen_view_nodes_es_es_desc
(
  "spline_gen_view_nodes_es",
  "prog/daNetGameLibs/spline_geometry/render/splineGenGeometryRepositoryES.cpp.inl",
  ecs::EntitySystemOps(nullptr, spline_gen_view_nodes_es_all_events),
  empty_span(),
  make_span(spline_gen_view_nodes_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnCameraPerViewNodeConstruction>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc spline_gen_request_render_node_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("dafg_camera_registrator__name"), ecs::ComponentTypeInfo<ecs::string>()}
};
static ecs::CompileTimeQueryDesc spline_gen_request_render_node_ecs_query_desc
(
  "spline_gen_request_render_node_ecs_query",
  empty_span(),
  make_span(spline_gen_request_render_node_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void spline_gen_request_render_node_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable function)
{
  perform_query(&manager, eid, spline_gen_request_render_node_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(spline_gen_request_render_node_ecs_query_comps, "dafg_camera_registrator__name", ecs::string)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc load_spline_gen_template_params_ecs_query_comps[] =
{
//start of 10 ro components at [0]
  {ECS_HASH("spline_gen_template__template_name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("spline_gen_template__slices"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("spline_gen_template__stripes"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("spline_gen_template__diffuse_name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("spline_gen_template__normal_name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("spline_gen_template__asset_name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("spline_gen_template__shader_type"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("spline_gen_template__emissive_mask_name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("spline_gen_template__skin_ao_tex_name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("spline_gen_template__asset_lod"), ecs::ComponentTypeInfo<int>()}
};
static ecs::CompileTimeQueryDesc load_spline_gen_template_params_ecs_query_desc
(
  "load_spline_gen_template_params_ecs_query",
  empty_span(),
  make_span(load_spline_gen_template_params_ecs_query_comps+0, 10)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void load_spline_gen_template_params_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, load_spline_gen_template_params_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(load_spline_gen_template_params_ecs_query_comps, "spline_gen_template__template_name", ecs::string)
            , ECS_RO_COMP(load_spline_gen_template_params_ecs_query_comps, "spline_gen_template__slices", int)
            , ECS_RO_COMP(load_spline_gen_template_params_ecs_query_comps, "spline_gen_template__stripes", int)
            , ECS_RO_COMP(load_spline_gen_template_params_ecs_query_comps, "spline_gen_template__diffuse_name", ecs::string)
            , ECS_RO_COMP(load_spline_gen_template_params_ecs_query_comps, "spline_gen_template__normal_name", ecs::string)
            , ECS_RO_COMP(load_spline_gen_template_params_ecs_query_comps, "spline_gen_template__asset_name", ecs::string)
            , ECS_RO_COMP(load_spline_gen_template_params_ecs_query_comps, "spline_gen_template__shader_type", ecs::string)
            , ECS_RO_COMP(load_spline_gen_template_params_ecs_query_comps, "spline_gen_template__emissive_mask_name", ecs::string)
            , ECS_RO_COMP(load_spline_gen_template_params_ecs_query_comps, "spline_gen_template__skin_ao_tex_name", ecs::string)
            , ECS_RO_COMP(load_spline_gen_template_params_ecs_query_comps, "spline_gen_template__asset_lod", int)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc load_spline_gen_shapes_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("spline_gen_shape__points"), ecs::ComponentTypeInfo<ecs::Point4List>()},
//start of 1 ro components at [1]
  {ECS_HASH("spline_gen_shape__shape_name"), ecs::ComponentTypeInfo<ecs::string>()}
};
static ecs::CompileTimeQueryDesc load_spline_gen_shapes_ecs_query_desc
(
  "load_spline_gen_shapes_ecs_query",
  make_span(load_spline_gen_shapes_ecs_query_comps+0, 1)/*rw*/,
  make_span(load_spline_gen_shapes_ecs_query_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void load_spline_gen_shapes_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, load_spline_gen_shapes_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(load_spline_gen_shapes_ecs_query_comps, "spline_gen_shape__points", ecs::Point4List)
            , ECS_RO_COMP(load_spline_gen_shapes_ecs_query_comps, "spline_gen_shape__shape_name", ecs::string)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_spline_gen_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("spline_gen_repository"), ecs::ComponentTypeInfo<SplineGenGeometryRepository>()}
};
static ecs::CompileTimeQueryDesc get_spline_gen_ecs_query_desc
(
  "get_spline_gen_ecs_query",
  make_span(get_spline_gen_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_spline_gen_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, get_spline_gen_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(get_spline_gen_ecs_query_comps, "spline_gen_repository", SplineGenGeometryRepository)
            );

        }while (++comp != compE);
    }
  );
}
