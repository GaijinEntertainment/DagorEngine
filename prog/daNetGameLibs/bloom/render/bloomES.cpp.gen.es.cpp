#include "bloomES.cpp.inl"
ECS_DEF_PULL_VAR(bloom);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc reset_bloom_es_comps[] ={};
static void reset_bloom_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ChangeRenderFeatures>());
  reset_bloom_es(static_cast<const ChangeRenderFeatures&>(evt)
        , components.manager()
    );
}
static ecs::EntitySystemDesc reset_bloom_es_es_desc
(
  "reset_bloom_es",
  "prog/daNetGameLibs/bloom/render/bloomES.cpp.inl",
  ecs::EntitySystemOps(nullptr, reset_bloom_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ChangeRenderFeatures>::build(),
  0
,"render");
//static constexpr ecs::ComponentDesc resize_bloom_es_comps[] ={};
static void resize_bloom_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<SetResolutionEvent>());
  resize_bloom_es(static_cast<const SetResolutionEvent&>(evt)
        , components.manager()
    );
}
static ecs::EntitySystemDesc resize_bloom_es_es_desc
(
  "resize_bloom_es",
  "prog/daNetGameLibs/bloom/render/bloomES.cpp.inl",
  ecs::EntitySystemOps(nullptr, resize_bloom_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<SetResolutionEvent>::build(),
  0
,"render");
//static constexpr ecs::ComponentDesc init_bloom_es_comps[] ={};
static void init_bloom_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  init_bloom_es(evt
        , components.manager()
    );
}
static ecs::EntitySystemDesc init_bloom_es_es_desc
(
  "init_bloom_es",
  "prog/daNetGameLibs/bloom/render/bloomES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_bloom_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<OnLevelLoaded>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc change_bloom_params_with_fg_change_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("bloom__upsample_chain"), ecs::ComponentTypeInfo<dag::Vector<dabfg::NodeHandle>>()},
//start of 5 ro components at [1]
  {ECS_HASH("bloom__upSample"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("bloom__halation_color"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("bloom__halation_brightness"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("bloom__halation_mip_factor"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("bloom__halation_end_mip"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL}
};
static void change_bloom_params_with_fg_change_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    change_bloom_params_with_fg_change_es(evt
        , ECS_RW_COMP(change_bloom_params_with_fg_change_es_comps, "bloom__upsample_chain", dag::Vector<dabfg::NodeHandle>)
    , ECS_RO_COMP(change_bloom_params_with_fg_change_es_comps, "bloom__upSample", float)
    , ECS_RO_COMP_OR(change_bloom_params_with_fg_change_es_comps, "bloom__halation_color", E3DCOLOR(E3DCOLOR(255, 0, 0, 0)))
    , ECS_RO_COMP_OR(change_bloom_params_with_fg_change_es_comps, "bloom__halation_brightness", float(2))
    , ECS_RO_COMP_OR(change_bloom_params_with_fg_change_es_comps, "bloom__halation_mip_factor", float(2))
    , ECS_RO_COMP_OR(change_bloom_params_with_fg_change_es_comps, "bloom__halation_end_mip", int(1))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc change_bloom_params_with_fg_change_es_es_desc
(
  "change_bloom_params_with_fg_change_es",
  "prog/daNetGameLibs/bloom/render/bloomES.cpp.inl",
  ecs::EntitySystemOps(nullptr, change_bloom_params_with_fg_change_es_all_events),
  make_span(change_bloom_params_with_fg_change_es_comps+0, 1)/*rw*/,
  make_span(change_bloom_params_with_fg_change_es_comps+1, 5)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,"render","bloom__halation_brightness,bloom__halation_color,bloom__halation_end_mip,bloom__halation_mip_factor,bloom__upSample");
static constexpr ecs::ComponentDesc change_bloom_shader_vars_es_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("bloom__radius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("bloom__threshold"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("bloom__mul"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL}
};
static void change_bloom_shader_vars_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    change_bloom_shader_vars_es(evt
        , ECS_RO_COMP(change_bloom_shader_vars_es_comps, "bloom__radius", float)
    , ECS_RO_COMP_OR(change_bloom_shader_vars_es_comps, "bloom__threshold", float(1))
    , ECS_RO_COMP_OR(change_bloom_shader_vars_es_comps, "bloom__mul", float(0.1))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc change_bloom_shader_vars_es_es_desc
(
  "change_bloom_shader_vars_es",
  "prog/daNetGameLibs/bloom/render/bloomES.cpp.inl",
  ecs::EntitySystemOps(nullptr, change_bloom_shader_vars_es_all_events),
  empty_span(),
  make_span(change_bloom_shader_vars_es_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render","bloom__mul,bloom__radius,bloom__threshold");
static constexpr ecs::ComponentDesc init_bloom_nodes_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("bloom__downsample_node"), ecs::ComponentTypeInfo<resource_slot::NodeHandleWithSlotsAccess>()}
};
static void init_bloom_nodes_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_bloom_nodes_es(evt
        , ECS_RW_COMP(init_bloom_nodes_es_comps, "bloom__downsample_node", resource_slot::NodeHandleWithSlotsAccess)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_bloom_nodes_es_es_desc
(
  "init_bloom_nodes_es",
  "prog/daNetGameLibs/bloom/render/bloomES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_bloom_nodes_es_all_events),
  make_span(init_bloom_nodes_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc disable_bloom_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("bloom__downsample_chain"), ecs::ComponentTypeInfo<dag::Vector<dabfg::NodeHandle>>()},
  {ECS_HASH("bloom__upsample_chain"), ecs::ComponentTypeInfo<dag::Vector<dabfg::NodeHandle>>()}
};
static ecs::CompileTimeQueryDesc disable_bloom_ecs_query_desc
(
  "disable_bloom_ecs_query",
  make_span(disable_bloom_ecs_query_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void disable_bloom_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, disable_bloom_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(disable_bloom_ecs_query_comps, "bloom__downsample_chain", dag::Vector<dabfg::NodeHandle>)
            , ECS_RW_COMP(disable_bloom_ecs_query_comps, "bloom__upsample_chain", dag::Vector<dabfg::NodeHandle>)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc init_bloom_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("bloom__downsample_chain"), ecs::ComponentTypeInfo<dag::Vector<dabfg::NodeHandle>>()},
  {ECS_HASH("bloom__upsample_chain"), ecs::ComponentTypeInfo<dag::Vector<dabfg::NodeHandle>>()},
//start of 5 ro components at [2]
  {ECS_HASH("bloom__upSample"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("bloom__halation_color"), ecs::ComponentTypeInfo<E3DCOLOR>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("bloom__halation_brightness"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("bloom__halation_mip_factor"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("bloom__halation_end_mip"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc init_bloom_ecs_query_desc
(
  "init_bloom_ecs_query",
  make_span(init_bloom_ecs_query_comps+0, 2)/*rw*/,
  make_span(init_bloom_ecs_query_comps+2, 5)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void init_bloom_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, init_bloom_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(init_bloom_ecs_query_comps, "bloom__downsample_chain", dag::Vector<dabfg::NodeHandle>)
            , ECS_RW_COMP(init_bloom_ecs_query_comps, "bloom__upsample_chain", dag::Vector<dabfg::NodeHandle>)
            , ECS_RO_COMP(init_bloom_ecs_query_comps, "bloom__upSample", float)
            , ECS_RO_COMP_OR(init_bloom_ecs_query_comps, "bloom__halation_color", E3DCOLOR(E3DCOLOR(255, 0, 0, 0)))
            , ECS_RO_COMP_OR(init_bloom_ecs_query_comps, "bloom__halation_brightness", float(2))
            , ECS_RO_COMP_OR(init_bloom_ecs_query_comps, "bloom__halation_mip_factor", float(2))
            , ECS_RO_COMP_OR(init_bloom_ecs_query_comps, "bloom__halation_end_mip", int(1))
            );

        }
    }
  );
}
