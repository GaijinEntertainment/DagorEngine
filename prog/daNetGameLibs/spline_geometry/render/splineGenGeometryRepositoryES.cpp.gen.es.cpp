#include "splineGenGeometryRepositoryES.cpp.inl"
ECS_DEF_PULL_VAR(splineGenGeometryRepository);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc load_spline_gen_template_params_ecs_query_comps[] =
{
//start of 7 ro components at [0]
  {ECS_HASH("spline_gen_template__template_name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("spline_gen_template__slices"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("spline_gen_template__stripes"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("spline_gen_template__diffuse_name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("spline_gen_template__normal_name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("spline_gen_template__asset_name"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("spline_gen_template__asset_lod"), ecs::ComponentTypeInfo<int>()}
};
static ecs::CompileTimeQueryDesc load_spline_gen_template_params_ecs_query_desc
(
  "load_spline_gen_template_params_ecs_query",
  empty_span(),
  make_span(load_spline_gen_template_params_ecs_query_comps+0, 7)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void load_spline_gen_template_params_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, load_spline_gen_template_params_ecs_query_desc.getHandle(),
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
            , ECS_RO_COMP(load_spline_gen_template_params_ecs_query_comps, "spline_gen_template__asset_lod", int)
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
inline void get_spline_gen_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_spline_gen_ecs_query_desc.getHandle(),
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
