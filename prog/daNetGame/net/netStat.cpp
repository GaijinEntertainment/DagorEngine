// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "netStat.h"
#include <daNet/daNetPeerInterface.h>
#include <daNet/daNetStatistics.h>
#include <generic/dag_staticTab.h>
#include <generic/dag_carray.h>
#include <math/dag_mathUtils.h>
#include <osApiWrappers/dag_spinlock.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/numeric_limits.h>

#define BUCKET_TIME_MS 1000
#define HISTORY_COUNT  10u

void timesync_push_rtt(uint32_t rtt) { netstat::set(netstat::CT_RTT, rtt); } // called by timesync's ClientTime impl

namespace netstat
{

struct NetIOStats
{
  uint64_t rxBytes = 0;
  uint64_t txBytes = 0;
  uint64_t rxPackets = 0;
  uint64_t txPackets = 0;
  uint32_t rttMs = 0;
  uint32_t plossPermille = 0;
};

static NetIOStats pull_io_stats(DaNetPeerInterface *peer)
{
  NetIOStats io;
  if (!peer)
    return io;
  if (const DaNetStatistics *s = peer->GetStatistics(UNASSIGNED_SYSTEM_INDEX))
  {
    io.rxBytes = s->bytesReceived;
    io.txBytes = s->bytesSent;
    io.rxPackets = s->packetsReceived;
    io.txPackets = s->packetsSent;
  }
  danet::PeerQoSStat q = peer->GetPeerQoSStat(0);
  if (q.lowestPing)
  {
    io.rttMs = (uint32_t)q.lowestPing;
    io.plossPermille = (uint32_t)(q.packetLoss * 1000.f);
  }
  return io;
}

static constexpr cnt_t INVALID_GAUGE_VALUE = eastl::numeric_limits<cnt_t>::max();
static constexpr bool gauge_mask[NUM_COUNTER_TYPES] = {
#define _NS(_1, _2, val, ...) val,
  DEF_NS_COUNTERS
#undef _NS
};
static Sample counters;

static OSSpinlock aggregated_lock;

struct Ctx
{
  uint32_t nextUpdateTime = 0;
  uint32_t numRecords = 0;
  carray<Sample, HISTORY_COUNT> buckets;
  carray<Sample, NUM_AG_TYPES> aggregated;
  NetIOStats prevBucketIO;

  Ctx()
  {
    for (int i = 0; i < NUM_COUNTER_TYPES; ++i)
      counters.counters[i] = (gauge_mask[i] ? INVALID_GAUGE_VALUE : 0);
  }

  void aggregate()
  {
    carray<Sample, NUM_AG_TYPES> result;
    for (int i = 0; i < NUM_COUNTER_TYPES; ++i)
    {
      result[AG_CNT].counters[i] = 0;
      result[AG_SUM].counters[i] = 0;
      result[AG_AVG].counters[i] = 0;
      result[AG_VAR].counters[i] = 0;
      result[AG_MIN].counters[i] = eastl::numeric_limits<cnt_t>::max();
      result[AG_MAX].counters[i] = eastl::numeric_limits<cnt_t>::min();
      result[AG_LAST].counters[i] = 0;
    }
    uint32_t cnt = eastl::max(eastl::min(numRecords, HISTORY_COUNT), 1u);
    uint32_t leastRecent = ((numRecords < HISTORY_COUNT) ? 0u : numRecords);
    for (int i = 0; i < cnt; ++i)
    {
      uint32_t record = (leastRecent + i) % HISTORY_COUNT; // walks buckets in order from the least recent to the most recent
      for (int counter = 0; counter < NUM_COUNTER_TYPES; ++counter)
      {
        cnt_t val = buckets[record].counters[counter];
        if (gauge_mask[counter] && val == INVALID_GAUGE_VALUE)
          continue;
        result[AG_CNT].counters[counter]++;
        result[AG_SUM].counters[counter] += val;
        result[AG_MIN].counters[counter] = eastl::min(result[AG_MIN].counters[counter], val);
        result[AG_MAX].counters[counter] = eastl::max(result[AG_MAX].counters[counter], val);
        result[AG_LAST].counters[counter] = val;
      }
    }
    for (int counter = 0; counter < NUM_COUNTER_TYPES; ++counter)
    {
      if (result[AG_CNT].counters[counter] > 0)
        result[AG_AVG].counters[counter] = result[AG_SUM].counters[counter] / result[AG_CNT].counters[counter];
      else
        result[AG_MIN].counters[counter] = 0;
    }
    for (int record = 0; record < cnt; ++record)
      for (int counter = 0; counter < NUM_COUNTER_TYPES; ++counter)
      {
        cnt_t val = buckets[record].counters[counter];
        if (gauge_mask[counter] && val == INVALID_GAUGE_VALUE)
          continue;
        // Variance metric is Mean Absolute Deviation
        cnt_t avg = result[AG_AVG].counters[counter];
        auto diff = eastl::max(avg, val) - eastl::min(avg, val);
        result[AG_VAR].counters[counter] += diff;
      }
    for (int counter = 0; counter < NUM_COUNTER_TYPES; ++counter)
      result[AG_VAR].counters[counter] /= eastl::max(result[AG_CNT].counters[counter], 1u);

    OSSpinlockScopedLock lock(aggregated_lock);
    aggregated = result;
  }

  void applyIOStats(float time_err_k, const NetIOStats &io)
  {
    inc(CT_RX_BYTES, (cnt_t)((io.rxBytes - prevBucketIO.rxBytes) / time_err_k));
    inc(CT_TX_BYTES, (cnt_t)((io.txBytes - prevBucketIO.txBytes) / time_err_k));
    inc(CT_RX_PACKETS, (cnt_t)((io.rxPackets - prevBucketIO.rxPackets) / time_err_k));
    inc(CT_TX_PACKETS, (cnt_t)((io.txPackets - prevBucketIO.txPackets) / time_err_k));
    prevBucketIO = io;
    if (io.rttMs)
    {
      set(CT_ENET_RTT, io.rttMs);
      set(CT_ENET_PLOSS, io.plossPermille);
    }
  }

  void update(uint32_t ct_ms, const NetIOStats &io)
  {
    if (ct_ms < nextUpdateTime)
      return;
    if (nextUpdateTime)
    {
      float timeErrK = (ct_ms - nextUpdateTime + BUCKET_TIME_MS) / float(BUCKET_TIME_MS);
      applyIOStats(timeErrK, io);
      buckets[numRecords++ % HISTORY_COUNT] = counters;
      aggregate();
      for (int i = 0; i < NUM_COUNTER_TYPES; ++i)
        counters.counters[i] = gauge_mask[i] ? INVALID_GAUGE_VALUE : 0;
    }
    else
      prevBucketIO = io;
    nextUpdateTime = ct_ms + BUCKET_TIME_MS;
  }
};
static eastl::unique_ptr<Ctx> ctx;

void init()
{
  OSSpinlockScopedLock lock(aggregated_lock);
  ctx.reset(new Ctx);
}

void shutdown()
{
  OSSpinlockScopedLock lock(aggregated_lock);
  ctx.reset();
}

void update(uint32_t ct_ms, DaNetPeerInterface *peer)
{
  if (ctx)
    ctx->update(ct_ms, pull_io_stats(peer));
}

void inc(CounterType type, cnt_t val) { counters.counters[type] += val; }
void set(CounterType type, cnt_t val) { counters.counters[type] = val; }
void max(CounterType type, cnt_t val) { counters.counters[type] = eastl::max(counters.counters[type], val); }

StaticTab<Sample, NUM_AG_TYPES> get_aggregations()
{
  StaticTab<Sample, NUM_AG_TYPES> out;
  OSSpinlockScopedLock lock(aggregated_lock);
  if (ctx)
  {
    out.resize(NUM_AG_TYPES);
    eastl::copy(ctx->aggregated.begin(), ctx->aggregated.end(), out.begin());
  }
  return out;
}

} // namespace netstat
