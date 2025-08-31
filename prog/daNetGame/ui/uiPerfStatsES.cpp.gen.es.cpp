// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "uiPerfStatsES.cpp.inl"
ECS_DEF_PULL_VAR(uiPerfStats);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc ui_perf_stats_es_comps[] =
{
//start of 7 rw components at [0]
  {ECS_HASH("ui_perf_stats"), ecs::ComponentTypeInfo<UiPerfStats>()},
  {ECS_HASH("ui_perf_stats__server_tick_warn"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("ui_perf_stats__low_fps_warn"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("ui_perf_stats__latency_warn"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("ui_perf_stats__latency_variation_warn"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("ui_perf_stats__packet_loss_warn"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("ui_perf_stats__low_tickrate_warn"), ecs::ComponentTypeInfo<int>()},
//start of 6 ro components at [7]
  {ECS_HASH("ui_perf_stats__server_tick_thr"), ecs::ComponentTypeInfo<Point2>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("ui_perf_stats__low_fps_thr"), ecs::ComponentTypeInfo<Point2>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("ui_perf_stats__latency_thr"), ecs::ComponentTypeInfo<Point2>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("ui_perf_stats__packet_loss_thr"), ecs::ComponentTypeInfo<Point2>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("ui_perf_stats__low_tickrate_thr"), ecs::ComponentTypeInfo<Point2>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("ui_perf_stats__fps_drop_thr"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL}
};
static void ui_perf_stats_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<RenderEventUI>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    ui_perf_stats_es(static_cast<const RenderEventUI&>(evt)
        , ECS_RW_COMP(ui_perf_stats_es_comps, "ui_perf_stats", UiPerfStats)
    , ECS_RW_COMP(ui_perf_stats_es_comps, "ui_perf_stats__server_tick_warn", int)
    , ECS_RW_COMP(ui_perf_stats_es_comps, "ui_perf_stats__low_fps_warn", int)
    , ECS_RW_COMP(ui_perf_stats_es_comps, "ui_perf_stats__latency_warn", int)
    , ECS_RW_COMP(ui_perf_stats_es_comps, "ui_perf_stats__latency_variation_warn", int)
    , ECS_RW_COMP(ui_perf_stats_es_comps, "ui_perf_stats__packet_loss_warn", int)
    , ECS_RW_COMP(ui_perf_stats_es_comps, "ui_perf_stats__low_tickrate_warn", int)
    , ECS_RO_COMP_OR(ui_perf_stats_es_comps, "ui_perf_stats__server_tick_thr", Point2(Point2(5, 10)))
    , ECS_RO_COMP_OR(ui_perf_stats_es_comps, "ui_perf_stats__low_fps_thr", Point2(Point2(20, 10)))
    , ECS_RO_COMP_OR(ui_perf_stats_es_comps, "ui_perf_stats__latency_thr", Point2(Point2(150, 200)))
    , ECS_RO_COMP_OR(ui_perf_stats_es_comps, "ui_perf_stats__packet_loss_thr", Point2(Point2(5, 10)))
    , ECS_RO_COMP_OR(ui_perf_stats_es_comps, "ui_perf_stats__low_tickrate_thr", Point2(Point2(20, PHYS_DEFAULT_TICKRATE)))
    , ECS_RO_COMP_OR(ui_perf_stats_es_comps, "ui_perf_stats__fps_drop_thr", float(0.5f))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc ui_perf_stats_es_es_desc
(
  "ui_perf_stats_es",
  "prog/daNetGame/ui/uiPerfStatsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, ui_perf_stats_es_all_events),
  make_span(ui_perf_stats_es_comps+0, 7)/*rw*/,
  make_span(ui_perf_stats_es_comps+7, 6)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<RenderEventUI>::build(),
  0
);
