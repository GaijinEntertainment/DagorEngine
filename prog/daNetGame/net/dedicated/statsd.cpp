// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "net/dedicated.h"
#include "netded.h"
#include <osApiWrappers/dag_sockets.h>
#include <statsd/statsd.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_direct.h>
#include <memory/dag_framemem.h>
#include "game/player.h"
#include "main/main.h"
#include "main/gameProjConfig.h"

namespace dedicated
{
static constexpr int STATSD_PLAYER_BUCKET_SIZE = 8;

bool init_statsd(const char *circuit_name)
{
  using namespace statsd;
  if (dgs_get_argv("nostatsd"))
    return true;
  char statsd_hostname[64];
  os_gethostname(statsd_hostname, sizeof(statsd_hostname), "unknown");
  if (char *pDot = strchr(statsd_hostname, '.')) // cut domain name
    *pDot = '\0';
  const char *keys[] = {circuit_name, "dedicated", statsd_hostname, nullptr};

  const char *metricsFormat = ::dgs_get_argv("statsd", MetricFormat::influx);

  static const char *statsdProject = ::dgs_get_settings()->getStr("statsdGameName", gameproj::game_telemetry_name());

  if (!strcmp(metricsFormat, MetricFormat::influx))
  {
    statsd::init(statsd::get_dagor_logger(), keys, "127.0.0.1", 8155,
      Env{Env::production /*Env::test if need metrics autoremove after some days*/}, Circuit{circuit_name},
      Application{Application::dedicated}, Platform{""}, Project{statsdProject}, Host{statsd_hostname});
  }
  else if (!strcmp(metricsFormat, MetricFormat::statsd))
  {
    statsd::init(statsd::get_dagor_logger(), keys, "127.0.0.1", 8125);
  }
  return true;
}

void statsd_act_time(dag::ConstSpan<float> frame_hist, float mean_ms, const char *scene_fn, int num_players)
{
  G_UNUSED(frame_hist);
  G_ASSERT(!frame_hist.empty());
  G_ASSERT(num_players > 0);

  int meanUs = mean_ms * 1000.f;

  statsd::profile("act_time_usec", (long)meanUs);

  char tmp[DAGOR_MAX_PATH];
  const char *scene = dd_get_fname_without_path_and_ext(tmp, (int)sizeof(tmp), scene_fn);
  eastl::string plrBucketStr = eastl::to_string((num_players - 1) / STATSD_PLAYER_BUCKET_SIZE);
  statsd::profile("act_time_usec.player_bucket", (long)meanUs, {{"player_bucket", plrBucketStr.c_str()}, {"scene", scene}});
}

void statsd_report_ctrl_ploss(ControlsPlossCalc &ploss_calc, uint16_t &last_reported_ctrl_seq, uint16_t new_ctrl_seq)
{
  ploss_calc.pushSequence(new_ctrl_seq);
  if (net::seq_diff(new_ctrl_seq, last_reported_ctrl_seq) <= ploss_calc.HISTORY_SIZE)
    return;
  last_reported_ctrl_seq = new_ctrl_seq;
  // Note for casual reader:
  // yes, naming is *intentionally* different from WT as it's calculated quite differently:
  // we send controls 2x tickrate (which is also different) & calculate *network* packet loss, not effective one
  if (int nplayers = game::get_num_connected_players())
  {
    int plrBucket = (nplayers - 1) / STATSD_PLAYER_BUCKET_SIZE;
    float ploss = ploss_calc.calcPacketLoss() * 100.f; // 1 lost packet from 9*30 is 0.37%
    statsd::profile("controls_rx_packet_loss_percent", ploss, {"player_bucket", eastl::to_string(plrBucket).c_str()});
  }
}

} // namespace dedicated
