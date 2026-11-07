// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "antiAliasingBenchmarkES.cpp.inl"
ECS_DEF_PULL_VAR(antiAliasingBenchmark);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc init_aa_benchmark_es_comps[] ={};
static void init_aa_benchmark_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<BeforeLoadLevel>());
  init_aa_benchmark_es(static_cast<const BeforeLoadLevel&>(evt)
        , components.manager()
    );
}
static ecs::EntitySystemDesc init_aa_benchmark_es_es_desc
(
  "init_aa_benchmark_es",
  "prog/daNetGame/render/world/antiAliasingBenchmarkES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_aa_benchmark_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<BeforeLoadLevel>::build(),
  0
,"render");
//static constexpr ecs::ComponentDesc aa_benchmark_multiplexing_es_comps[] ={};
static void aa_benchmark_multiplexing_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<QueryMultiplexingExtents>());
  aa_benchmark_multiplexing_es(static_cast<const QueryMultiplexingExtents&>(evt)
        );
}
static ecs::EntitySystemDesc aa_benchmark_multiplexing_es_es_desc
(
  "aa_benchmark_multiplexing_es",
  "prog/daNetGame/render/world/antiAliasingBenchmarkES.cpp.inl",
  ecs::EntitySystemOps(nullptr, aa_benchmark_multiplexing_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<QueryMultiplexingExtents>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc aa_benchmark_params_ecs_query_comps[] =
{
//start of 7 ro components at [0]
  {ECS_HASH("aa_benchmark__subsamples"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("aa_benchmark__filter"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("aa_benchmark__taps"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("aa_benchmark__view"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("aa_benchmark__metric_exp_decay"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("aa_benchmark__heatmap_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("aa_benchmark__heatmap_opacity"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc aa_benchmark_params_ecs_query_desc
(
  "aa_benchmark_params_ecs_query",
  empty_span(),
  make_span(aa_benchmark_params_ecs_query_comps+0, 7)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void aa_benchmark_params_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, aa_benchmark_params_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(aa_benchmark_params_ecs_query_comps, "aa_benchmark__subsamples", int)
            , ECS_RO_COMP(aa_benchmark_params_ecs_query_comps, "aa_benchmark__filter", int)
            , ECS_RO_COMP(aa_benchmark_params_ecs_query_comps, "aa_benchmark__taps", int)
            , ECS_RO_COMP(aa_benchmark_params_ecs_query_comps, "aa_benchmark__view", int)
            , ECS_RO_COMP(aa_benchmark_params_ecs_query_comps, "aa_benchmark__metric_exp_decay", float)
            , ECS_RO_COMP(aa_benchmark_params_ecs_query_comps, "aa_benchmark__heatmap_scale", float)
            , ECS_RO_COMP(aa_benchmark_params_ecs_query_comps, "aa_benchmark__heatmap_opacity", float)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc aa_benchmark_nodes_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("aa_benchmark__nodes"), ecs::ComponentTypeInfo<dag::Vector<dafg::NodeHandle>>()}
};
static ecs::CompileTimeQueryDesc aa_benchmark_nodes_ecs_query_desc
(
  "aa_benchmark_nodes_ecs_query",
  make_span(aa_benchmark_nodes_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void aa_benchmark_nodes_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable function)
{
  perform_query(&manager, eid, aa_benchmark_nodes_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(aa_benchmark_nodes_ecs_query_comps, "aa_benchmark__nodes", dag::Vector<dafg::NodeHandle>)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc aa_benchmark_imgui_ecs_query_comps[] =
{
//start of 8 rw components at [0]
  {ECS_HASH("aa_benchmark__subsamples"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("aa_benchmark__filter"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("aa_benchmark__taps"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("aa_benchmark__view"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("aa_benchmark__heatmap_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("aa_benchmark__heatmap_opacity"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("aa_benchmark__dump_path"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("aa_benchmark__dump_frames"), ecs::ComponentTypeInfo<int>()}
};
static ecs::CompileTimeQueryDesc aa_benchmark_imgui_ecs_query_desc
(
  "aa_benchmark_imgui_ecs_query",
  make_span(aa_benchmark_imgui_ecs_query_comps+0, 8)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void aa_benchmark_imgui_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable function)
{
  perform_query(&manager, eid, aa_benchmark_imgui_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(aa_benchmark_imgui_ecs_query_comps, "aa_benchmark__subsamples", int)
            , ECS_RW_COMP(aa_benchmark_imgui_ecs_query_comps, "aa_benchmark__filter", int)
            , ECS_RW_COMP(aa_benchmark_imgui_ecs_query_comps, "aa_benchmark__taps", int)
            , ECS_RW_COMP(aa_benchmark_imgui_ecs_query_comps, "aa_benchmark__view", int)
            , ECS_RW_COMP(aa_benchmark_imgui_ecs_query_comps, "aa_benchmark__heatmap_scale", float)
            , ECS_RW_COMP(aa_benchmark_imgui_ecs_query_comps, "aa_benchmark__heatmap_opacity", float)
            , ECS_RW_COMP(aa_benchmark_imgui_ecs_query_comps, "aa_benchmark__dump_path", ecs::string)
            , ECS_RW_COMP(aa_benchmark_imgui_ecs_query_comps, "aa_benchmark__dump_frames", int)
            );

        }
    }
  );
}
