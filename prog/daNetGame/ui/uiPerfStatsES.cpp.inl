// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <ecs/core/attributeEx.h>
#include "net/netStat.h"
#include "net/plosscalc.h"
#include <frameTimeMetrics/aggregator.h>
#include <perfMon/dag_cpuFreq.h>
#include <startup/dag_globalSettings.h>
#include <workCycle/dag_workCycle.h>
#include "render/renderEvent.h"


extern FixedPlossCalc &phys_get_snapshots_ploss_calc();
extern int phys_get_tickrate();


struct UiPerfStats
{
  UiPerfStats() : frameTimeMetrics(eastl::make_unique<FrameTimeMetricsAggregator>()) {}

  eastl::unique_ptr<FrameTimeMetricsAggregator> frameTimeMetrics;
};


ECS_DECLARE_RELOCATABLE_TYPE(UiPerfStats);
ECS_REGISTER_RELOCATABLE_TYPE(UiPerfStats, nullptr);
ECS_AUTO_REGISTER_COMPONENT(UiPerfStats, "ui_perf_stats", nullptr, 0);


static void set_warn_level_greater(int &warn_level, float val, const Point2 &range)
{
  if (val > range[1])
    warn_level = 2;
  else if (val > range[0])
    warn_level = 1;
  else
    warn_level = 0;
}


static void set_warn_level_lower(int &warn_level, float val, const Point2 &range)
{
  if (val < range[1])
    warn_level = 2;
  else if (val < range[0])
    warn_level = 1;
  else
    warn_level = 0;
}


static void ui_perf_stats_es(const RenderEventUI & /*evt*/,
  UiPerfStats &ui_perf_stats,

  int &ui_perf_stats__server_tick_warn,
  int &ui_perf_stats__low_fps_warn,
  int &ui_perf_stats__latency_warn,
  int &ui_perf_stats__latency_variation_warn,
  int &ui_perf_stats__packet_loss_warn,
  int &ui_perf_stats__low_tickrate_warn,

  Point2 ui_perf_stats__server_tick_thr = Point2(5, 10),
  Point2 ui_perf_stats__low_fps_thr = Point2(20, 10),
  Point2 ui_perf_stats__latency_thr = Point2(150, 200),
  Point2 ui_perf_stats__packet_loss_thr = Point2(5, 10),
  Point2 ui_perf_stats__low_tickrate_thr = Point2(20, PHYS_DEFAULT_TICKRATE),
  float ui_perf_stats__fps_drop_thr = 0.5f)
{
  using namespace netstat;

  dag::ConstSpan<netstat::Sample> ns = netstat::get_aggregations();
  if (ns.empty())
  {
    ui_perf_stats__server_tick_warn = 0;
    ui_perf_stats__latency_warn = 0;
    ui_perf_stats__packet_loss_warn = 0;
    ui_perf_stats__latency_variation_warn = 0;
  }
  else
  {
    float packetLoss = phys_get_snapshots_ploss_calc().calcPacketLoss() * 100.f;
    set_warn_level_greater(ui_perf_stats__server_tick_warn, ns[AG_AVG].counters[CT_SERVER_TICK], ui_perf_stats__server_tick_thr);

    set_warn_level_greater(ui_perf_stats__latency_warn, ns[AG_AVG].counters[CT_RTT], ui_perf_stats__latency_thr);

    set_warn_level_greater(ui_perf_stats__packet_loss_warn, packetLoss, ui_perf_stats__packet_loss_thr);

    ui_perf_stats__latency_variation_warn = 0;
  }

  auto &fps = ui_perf_stats.frameTimeMetrics;
  fps->update(::get_time_msec(), ::dagor_frame_no(), ::dagor_game_act_time, PerfDisplayMode::FPS);
  set_warn_level_lower(ui_perf_stats__low_fps_warn, fps->getLastAverageFps(), ui_perf_stats__low_fps_thr);
  if (!ui_perf_stats__low_fps_warn)
    if (fps->getLastMaxFrameTime() > fps->getLastAverageFrameTime() / ui_perf_stats__fps_drop_thr)
      ui_perf_stats__low_fps_warn = 1;

  set_warn_level_lower(ui_perf_stats__low_tickrate_warn, phys_get_tickrate(), ui_perf_stats__low_tickrate_thr);
}
