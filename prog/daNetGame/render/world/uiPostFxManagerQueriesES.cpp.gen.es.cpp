#include "uiPostFxManagerQueriesES.cpp.inl"
ECS_DEF_PULL_VAR(uiPostFxManagerQueries);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc get_ui_blur_texid_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("blurred_ui__texid"), ecs::ComponentTypeInfo<TEXTUREID>()}
};
static ecs::CompileTimeQueryDesc get_ui_blur_texid_ecs_query_desc
(
  "get_ui_blur_texid_ecs_query",
  make_span(get_ui_blur_texid_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_ui_blur_texid_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_ui_blur_texid_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(get_ui_blur_texid_ecs_query_comps, "blurred_ui__texid", TEXTUREID)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_ui_blur_sampler_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("blurred_ui__smp"), ecs::ComponentTypeInfo<d3d::SamplerHandle>()}
};
static ecs::CompileTimeQueryDesc get_ui_blur_sampler_ecs_query_desc
(
  "get_ui_blur_sampler_ecs_query",
  empty_span(),
  make_span(get_ui_blur_sampler_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_ui_blur_sampler_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_ui_blur_sampler_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_ui_blur_sampler_ecs_query_comps, "blurred_ui__smp", d3d::SamplerHandle)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_ui_blur_sdr_texid_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("blurred_ui_sdr__texid"), ecs::ComponentTypeInfo<TEXTUREID>()}
};
static ecs::CompileTimeQueryDesc get_ui_blur_sdr_texid_ecs_query_desc
(
  "get_ui_blur_sdr_texid_ecs_query",
  make_span(get_ui_blur_sdr_texid_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_ui_blur_sdr_texid_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_ui_blur_sdr_texid_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(get_ui_blur_sdr_texid_ecs_query_comps, "blurred_ui_sdr__texid", TEXTUREID)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_ui_blur_sdr_sampler_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("blurred_ui_sdr__smp"), ecs::ComponentTypeInfo<d3d::SamplerHandle>()}
};
static ecs::CompileTimeQueryDesc get_ui_blur_sdr_sampler_ecs_query_desc
(
  "get_ui_blur_sdr_sampler_ecs_query",
  empty_span(),
  make_span(get_ui_blur_sdr_sampler_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_ui_blur_sdr_sampler_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_ui_blur_sdr_sampler_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_ui_blur_sdr_sampler_ecs_query_comps, "blurred_ui_sdr__smp", d3d::SamplerHandle)
            );

        }while (++comp != compE);
    }
  );
}
