// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "net/netPrivate.h"
#include <EASTL/algorithm.h>
#include <EASTL/numeric.h>
#include <EASTL/tuple.h>
#include <EASTL/bonus/fixed_ring_buffer.h>
#include <daECS/net/network.h>
#include "phys/physUtils.h"
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_miscApi.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <daNet/getTime.h>
#include <util/dag_convar.h>
#include <rapidJsonUtils/rapidJsonUtils.h>
#if _TARGET_PC_LINUX
#include <time.h>
#include <unistd.h>
#include <errno.h>
#define SLEEP_OVERHEAD_LINUX_US \
  130 // 70..130us according to manual testing (see also http://www.martani.net/2011/07/nanosleep-usleep-and-sleep-precision.html )
#endif
#define SLEEP_OVERHEAD_WINDOWS_US \
  1000 // On Windows you can't sleep less then 1 ms (mean sleep overhead is ~0.5 ms, but stddev is ~0.28 ms)
#include "netded.h"
#include <statsd/statsd.h>
#include "net/dedicated.h"
#include "main/appProfile.h"
#include "main/level.h"
#include "game/player.h"
#include "phys/physConsts.h"

ConVarT<int, false> dbg_force_server_tick_rate("net.force_server_tick_rate", -1, "Force server tick rate, -1 to disable");

namespace workcycle_internal
{
extern volatile int64_t last_wc_time;
} // namespace workcycle_internal
static int last_server_tick_us = 0;
size_t dedicated::number_of_tickrate_changes = 0;

template <typename C, typename F = float>
static inline eastl::pair<F, F> calc_stddev(const C &cont) // stddev, mean
{
  size_t size = cont.size();
  G_FAST_ASSERT(size > 0);
  F mean = eastl::accumulate(cont.begin(), cont.end(), 0.) / size;
  double accum = 0;
  eastl::for_each(cont.begin(), cont.end(), [&accum, &mean](F d) { accum += sqr(d - mean); });
  return eastl::make_pair(std::sqrt(accum / size), mean); // population stddev (no bessel correction)
}

#if _TARGET_PC_LINUX
static bool is_linux_distro_wsl()
{
  FILE *fp = fopen("/proc/sys/kernel/osrelease", "rb");
  if (!fp)
    return false;
  char tmp[128];
  size_t readed = fread(tmp, 1, sizeof(tmp) - 1, fp);
  fclose(fp);
  tmp[readed] = '\0';
  return strstr(tmp, "Microsoft") || strstr(tmp, "WSL"); // https://github.com/microsoft/WSL/issues/423#issuecomment-221627364
}
static int sleepOverheadUs = 0;
#endif
static void usleep_precise(int sleepUs)
{
  if (sleepUs <= 0)
    return;
#if _TARGET_PC_LINUX
  if (!sleepOverheadUs)
    sleepOverheadUs = is_linux_distro_wsl() ? SLEEP_OVERHEAD_WINDOWS_US : SLEEP_OVERHEAD_LINUX_US;
  timespec prevtm;
  clock_gettime(CLOCK_MONOTONIC, &prevtm);
  if (sleepUs >= sleepOverheadUs)
  {
    timespec waituntil = prevtm;
    waituntil.tv_nsec += (sleepUs - sleepOverheadUs) * 1000;
    long secDiff = 1000000000L - waituntil.tv_nsec;
    if (secDiff <= 0)
    {
      waituntil.tv_nsec = -secDiff;
      waituntil.tv_sec++;
    }
    while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &waituntil, NULL) == EINTR)
      ;
  }
  int64_t sleepNs = int64_t(sleepUs) * 1000;
  do
  {
    timespec curtm;
    clock_gettime(CLOCK_MONOTONIC, &curtm);
    if ((curtm.tv_nsec + (curtm.tv_sec - prevtm.tv_sec) * 1000000000L) - prevtm.tv_nsec >= sleepNs)
      break;
    cpu_yield();
  } while (1);
#else
  int64_t reft = ref_time_ticks();
  int sleepMs = (sleepUs - SLEEP_OVERHEAD_WINDOWS_US) / 1000;
  if (sleepMs > 0)
    sleep_msec(sleepMs);
  while (get_time_usec(reft) < sleepUs)
    cpu_yield();
#endif
}

static bool is_dynamic_tickrate_enabled_by_matching(bool fallback = false)
{
  auto *extraParams = jsonutils::get_ptr<rapidjson::Value::ConstObject>(get_matching_invite_data(), "extraParams");
  if (!extraParams)
    if (auto modeInfo = jsonutils::get_ptr<rapidjson::Value::ConstObject>(get_matching_invite_data(), "mode_info"))
      extraParams = jsonutils::get_ptr<rapidjson::Value::ConstObject>(*modeInfo, "extraParams");
  bool result = extraParams ? jsonutils::get_or(*extraParams, "enableDynamicTickrate", fallback) : fallback;
  debug("Dynamic tickrate enabled %d", result);
  return result;
}

bool dedicated::is_dynamic_tickrate_enabled()
{
  static bool enabledByMatching = is_dynamic_tickrate_enabled_by_matching();
  return enabledByMatching || dbg_force_server_tick_rate.get() > 0;
}

static bool try_change_tickrate(float meanFrameTime)
{
  if (dbg_force_server_tick_rate.get() > 0)
    return phys_set_tickrate(dbg_force_server_tick_rate.get());

  int currentTickRate = phys_get_tickrate();
  float currentTimeStep = 1000 / currentTickRate;

  // Very simple heuristic. Change tickrate, if lags are notably bad and don't rush to change it back.
  const DataBlock *config = dgs_get_settings()->getBlockByNameEx("tickrate");

  float enableLowThreshold = config->getReal("enableLowThreshold", 1.0);
  float enableNormalThreshold = config->getReal("enableNormalThreshold", 0.25);
#if defined(__SANITIZE_ADDRESS__)
  enableLowThreshold = config->getReal("enableLowThresholdAsan", enableLowThreshold * 4.0);
  enableNormalThreshold = config->getReal("enableNormalThresholdAsan", enableNormalThreshold * 2.0);
#endif

  float lowTickrateMultiplier = config->getReal("lowTickrateMultiplier", 0.5);
  float highTickrateMultiplier = config->getReal("highTickrateMultiplier", 1.0);

  if (meanFrameTime > (currentTimeStep * enableLowThreshold))
    return phys_set_tickrate(PHYS_DEFAULT_TICKRATE * lowTickrateMultiplier);
  else if (meanFrameTime < (currentTimeStep * enableNormalThreshold) && currentTickRate != PHYS_DEFAULT_TICKRATE)
    return phys_set_tickrate(PHYS_DEFAULT_TICKRATE * highTickrateMultiplier);

  return false;
}

namespace workcycle_internal
{
void set_title(const char *, bool) {}
void set_title_tooltip(const char *, const char *, bool) {}
void idle_loop()
{
  if (!is_level_loaded())
    sleep_msec(50);
  else if (int numPlrs = game::get_num_connected_players())
  {
    int frameTimeUs = get_time_usec(workcycle_internal::last_wc_time);
    static eastl::ring_buffer<float, eastl::fixed_vector<float, PHYS_DEFAULT_TICKRATE + 1, /*overflow*/ true>, EASTLAllocatorType>
      frameHist; // in millis
    int timeStepUs = 1000000 / phys_get_tickrate();

    last_server_tick_us = frameTimeUs;

    // try to make real dt as close to timeStep as possible in order to achieve monotonically increasing phys ticks (w/out dups&gaps)
    usleep_precise(timeStepUs - frameTimeUs);

    if (!frameHist.capacity())
    {
      frameHist.resize(phys_get_tickrate());
      frameHist.clear();
    }

    if (frameTimeUs > timeStepUs)
    {
      eastl::string tickrate = eastl::to_string(phys_get_tickrate());
      statsd::profile("lagged_frame_ms", (long)(frameTimeUs / 1000), {"tickrate", tickrate.c_str()});
      int _2timeStepUs = timeStepUs * 2;
      if (frameTimeUs > _2timeStepUs)
      {
        statsd::counter("lagged_frame_2ticks", 1, {"tickrate", tickrate.c_str()});
        if (frameTimeUs > (_2timeStepUs + timeStepUs))
          statsd::counter("lagged_frame_3ticks", 1, {"tickrate", tickrate.c_str()});
      }
    }

    float frameTimeMs = frameTimeUs / 1000.f; // in ms to avoid rounding errors
    frameHist.push_back(frameTimeMs);
    if (!frameHist.full())
      return;

    net::CNetwork *net = get_net_internal();
    const eastl::vector<eastl::unique_ptr<net::Connection>> *conns = net ? &net->getClientConnections() : nullptr;
    if (conns && !conns->empty()) // this checks shall not be needed but be paranoid
    {
      static uint32_t peerId = 0;
      if (net::IConnection *conn = (peerId < conns->size()) ? (*conns)[peerId].get() : nullptr)
      {
        danet::PeerQoSStat qstat = conn->getPeerQoSStat();
        if (qstat.lowestPing)
        {
          if ((danet::GetTime() - qstat.connectionStartTime) > 10000 /*ms*/) // 2xENET_PEER_PACKET_THROTTLE_INTERVAL
          {
            statsd::profile("enet_ping", (long)qstat.lowestPing);
            statsd::profile("enet_ping_variance", (long)qstat.highestPingVariance);
          }
          statsd::profile("enet_tx_packet_loss_percent", qstat.packetLoss * 100.f);
        }
      }
      if (++peerId >= max(32, (int)conns->size())) // wrap send period manually to avoid biasing small sessions over big ones
        peerId = 0;
    }

    statsd::counter("num_clients", numPlrs);

    float stddev, mean;
    eastl::tie(stddev, mean) = calc_stddev(frameHist);

    // Assume that this callback is called when settings already loaded and it's constant during whole life of dedicated, so it's safe
    // to cache pointer to it
    static const char *cachedSceneFn = dgs_get_settings()->getStr("scene", "unknown");

    dedicated::statsd_act_time(make_span_const(frameHist.c.begin(), frameHist.size()), mean, cachedSceneFn, numPlrs);
    frameHist.clear();

    if (dedicated::is_dynamic_tickrate_enabled() && try_change_tickrate(mean))
    {
      ++dedicated::number_of_tickrate_changes;
      debug("Changing tickrate %d time to %d", dedicated::number_of_tickrate_changes, phys_get_tickrate());
    }

    // 1.5 is 0.87 % (https://en.wikipedia.org/wiki/68%E2%80%9395%E2%80%9399.7_rule#Table_of_numerical_values )
    if (frameTimeMs > 2.f * (mean + 1.5f * stddev))
      logwarn("frame took %.1fms (which is > 200%% of %.1f+1.5*%.1f ms)", frameTimeMs, mean, stddev);
  }
  else // assume idle mode if there are no connections to send data to
    sleep_msec(100);
}
} // namespace workcycle_internal

namespace dedicated
{
int get_server_tick_time_us() { return last_server_tick_us; }
} // namespace dedicated
