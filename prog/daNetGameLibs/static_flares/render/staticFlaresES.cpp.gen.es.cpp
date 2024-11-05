#include "staticFlaresES.cpp.inl"
ECS_DEF_PULL_VAR(staticFlares);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc init_static_flares_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("static_flares__instancesBuf"), ecs::ComponentTypeInfo<UniqueBufHolder>()},
  {ECS_HASH("static_flares__instances"), ecs::ComponentTypeInfo<StaticFlareInstances>()},
  {ECS_HASH("static_flares__node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
//start of 1 ro components at [3]
  {ECS_HASH("static_flares__maxCount"), ecs::ComponentTypeInfo<int>()}
};
static void init_static_flares_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_static_flares_es(evt
        , ECS_RO_COMP(init_static_flares_es_comps, "static_flares__maxCount", int)
    , ECS_RW_COMP(init_static_flares_es_comps, "static_flares__instancesBuf", UniqueBufHolder)
    , ECS_RW_COMP(init_static_flares_es_comps, "static_flares__instances", StaticFlareInstances)
    , ECS_RW_COMP(init_static_flares_es_comps, "static_flares__node", dabfg::NodeHandle)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_static_flares_es_es_desc
(
  "init_static_flares_es",
  "prog/daNetGameLibs/static_flares/render/staticFlaresES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_static_flares_es_all_events),
  make_span(init_static_flares_es_comps+0, 3)/*rw*/,
  make_span(init_static_flares_es_comps+3, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc static_flare_before_render_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("static_flares__copiedCount"), ecs::ComponentTypeInfo<int>()},
//start of 4 ro components at [1]
  {ECS_HASH("static_flares__instancesBuf"), ecs::ComponentTypeInfo<UniqueBufHolder>()},
  {ECS_HASH("static_flares__instances"), ecs::ComponentTypeInfo<StaticFlareInstances>()},
  {ECS_HASH("static_flares__count"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("static_flares__visible"), ecs::ComponentTypeInfo<bool>()}
};
static void static_flare_before_render_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(ECS_RO_COMP(static_flare_before_render_es_comps, "static_flares__visible", bool)) )
      continue;
    static_flare_before_render_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
          , ECS_RO_COMP(static_flare_before_render_es_comps, "static_flares__instancesBuf", UniqueBufHolder)
      , ECS_RO_COMP(static_flare_before_render_es_comps, "static_flares__instances", StaticFlareInstances)
      , ECS_RO_COMP(static_flare_before_render_es_comps, "static_flares__count", int)
      , ECS_RW_COMP(static_flare_before_render_es_comps, "static_flares__copiedCount", int)
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc static_flare_before_render_es_es_desc
(
  "static_flare_before_render_es",
  "prog/daNetGameLibs/static_flares/render/staticFlaresES.cpp.inl",
  ecs::EntitySystemOps(nullptr, static_flare_before_render_es_all_events),
  make_span(static_flare_before_render_es_comps+0, 1)/*rw*/,
  make_span(static_flare_before_render_es_comps+1, 4)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
);
static constexpr ecs::ComponentDesc get_static_flares_ecs_query_comps[] =
{
//start of 6 ro components at [0]
  {ECS_HASH("static_flares__shader"), ecs::ComponentTypeInfo<ShadersECS>()},
  {ECS_HASH("static_flares__minSize"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("static_flares__maxSize"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("static_flares_ctg_min_max"), ecs::ComponentTypeInfo<ShaderVar>()},
  {ECS_HASH("static_flares__copiedCount"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("static_flares__visible"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc get_static_flares_ecs_query_desc
(
  "get_static_flares_ecs_query",
  empty_span(),
  make_span(get_static_flares_ecs_query_comps+0, 6)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_static_flares_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_static_flares_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(ECS_RO_COMP(get_static_flares_ecs_query_comps, "static_flares__visible", bool)) )
            continue;
          function(
              ECS_RO_COMP(get_static_flares_ecs_query_comps, "static_flares__shader", ShadersECS)
            , ECS_RO_COMP(get_static_flares_ecs_query_comps, "static_flares__minSize", float)
            , ECS_RO_COMP(get_static_flares_ecs_query_comps, "static_flares__maxSize", float)
            , ECS_RO_COMP(get_static_flares_ecs_query_comps, "static_flares_ctg_min_max", ShaderVar)
            , ECS_RO_COMP(get_static_flares_ecs_query_comps, "static_flares__copiedCount", int)
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc add_static_flare_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("static_flares__instances"), ecs::ComponentTypeInfo<StaticFlareInstances>()},
  {ECS_HASH("static_flares__count"), ecs::ComponentTypeInfo<int>()},
//start of 1 ro components at [2]
  {ECS_HASH("static_flares__maxCount"), ecs::ComponentTypeInfo<int>()}
};
static ecs::CompileTimeQueryDesc add_static_flare_ecs_query_desc
(
  "add_static_flare_ecs_query",
  make_span(add_static_flare_ecs_query_comps+0, 2)/*rw*/,
  make_span(add_static_flare_ecs_query_comps+2, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void add_static_flare_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, add_static_flare_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(add_static_flare_ecs_query_comps, "static_flares__instances", StaticFlareInstances)
            , ECS_RO_COMP(add_static_flare_ecs_query_comps, "static_flares__maxCount", int)
            , ECS_RW_COMP(add_static_flare_ecs_query_comps, "static_flares__count", int)
            );

        }while (++comp != compE);
    }
  );
}
