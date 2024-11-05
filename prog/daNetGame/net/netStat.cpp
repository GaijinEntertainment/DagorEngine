// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "netStat.h"
#include <generic/dag_staticTab.h>
#include <daECS/net/network.h>
#include <daNet/daNetStatistics.h>
#include <daNet/daNetPeerInterface.h>
#include <generic/dag_carray.h>
#include <math/dag_mathUtils.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/numeric_limits.h>
#include "netPrivate.h"

#define BUCKET_TIME_MS 1000
#define HISTORY_COUNT  10u

void timesync_push_rtt(uint32_t rtt) { netstat::set(netstat::CT_RTT, rtt); } // called by timesync's ClientTime impl

namespace netstat
{

static const DaNetStatistics *get_danet_stat(DaNetPeerInterface **pdaif = nullptr)
{
  net::CNetwork *net = get_net_internal();
  auto daif = static_cast<DaNetPeerInterface *>(net ? net->getDriver()->getControlIface() : NULL);
  if (pdaif)
    *pdaif = daif;
  return daif ? daif->GetStatistics(UNASSIGNED_SYSTEM_INDEX) : NULL;
}

static constexpr cnt_t INVALID_GAUGE_VALUE = eastl::numeric_limits<cnt_t>::max();
static constexpr bool gauge_mask[NUM_COUNTER_TYPES] = {
#define _NS(_1, _2, val, ...) val,
  DEF_NS_COUNTERS
#undef _NS
};
static Sample counters;

struct Ctx
{
  uint32_t nextUpdateTime = 0;
  uint32_t numRecords = 0;
  carray<Sample, HISTORY_COUNT> buckets;
  carray<Sample, NUM_AG_TYPES> aggregated;
  DaNetStatistics daSample;

  Ctx()
  {
    for (int i = 0; i < NUM_COUNTER_TYPES; ++i)
      counters.counters[i] = (gauge_mask[i] ? INVALID_GAUGE_VALUE : 0);
  }

  void aggregate()
  {
    for (int i = 0; i < NUM_COUNTER_TYPES; ++i)
    {
      aggregated[AG_CNT].counters[i] = 0;
      aggregated[AG_SUM].counters[i] = 0;
      aggregated[AG_AVG].counters[i] = 0;
      aggregated[AG_VAR].counters[i] = 0;
      aggregated[AG_MIN].counters[i] = eastl::numeric_limits<cnt_t>::max();
      aggregated[AG_MAX].counters[i] = eastl::numeric_limits<cnt_t>::min();
      aggregated[AG_LAST].counters[i] = 0;
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
        aggregated[AG_CNT].counters[counter]++;
        aggregated[AG_SUM].counters[counter] += val;
        aggregated[AG_MIN].counters[counter] = eastl::min(aggregated[AG_MIN].counters[counter], val);
        aggregated[AG_MAX].counters[counter] = eastl::max(aggregated[AG_MAX].counters[counter], val);
        aggregated[AG_LAST].counters[counter] = val;
      }
    }
    for (int counter = 0; counter < NUM_COUNTER_TYPES; ++counter)
    {
      if (aggregated[AG_CNT].counters[counter] > 0)
        aggregated[AG_AVG].counters[counter] = aggregated[AG_SUM].counters[counter] / aggregated[AG_CNT].counters[counter];
      else
        aggregated[AG_MIN].counters[counter] = 0;
    }
    for (int record = 0; record < cnt; ++record)
      for (int counter = 0; counter < NUM_COUNTER_TYPES; ++counter)
      {
        cnt_t val = buckets[record].counters[counter];
        if (gauge_mask[counter] && val == INVALID_GAUGE_VALUE)
          continue;
        // Variance metric is Mean Absolute Deviation
        cnt_t avg = aggregated[AG_AVG].counters[counter];
        auto diff = eastl::max(avg, val) - eastl::min(avg, val);
        aggregated[AG_VAR].counters[counter] += diff;
      }
    for (int counter = 0; counter < NUM_COUNTER_TYPES; ++counter)
      aggregated[AG_VAR].counters[counter] /= eastl::max(aggregated[AG_CNT].counters[counter], 1u);
  }

  void updateDaNetCounters(float time_err_k)
  {
    DaNetPeerInterface *daif;
    const DaNetStatistics *daStat = get_danet_stat(&daif);
    if (!daStat)
      return;
    inc(CT_RX_BYTES, (daStat->bytesReceived - daSample.bytesReceived) / time_err_k);
    inc(CT_TX_BYTES, (daStat->bytesSent - daSample.bytesSent) / time_err_k);
    inc(CT_RX_PACKETS, (daStat->packetsReceived - daSample.packetsReceived) / time_err_k);
    inc(CT_TX_PACKETS, (daStat->packetsSent - daSample.packetsSent) / time_err_k);
    daSample = *daStat;

    danet::PeerQoSStat qstat = daif->GetPeerQoSStat(0);
    if (qstat.lowestPing)
    {
      set(CT_ENET_PLOSS, (uint32_t)(qstat.packetLoss * 1000.f));
      set(CT_ENET_RTT, (uint32_t)(qstat.lowestPing));
    }
  }

  void update(uint32_t ct_ms)
  {
    if (ct_ms < nextUpdateTime)
      return;
    if (nextUpdateTime)
    {
      float timeErrK = (ct_ms - nextUpdateTime + BUCKET_TIME_MS) / float(BUCKET_TIME_MS);
      updateDaNetCounters(timeErrK);
      buckets[numRecords++ % HISTORY_COUNT] = counters;
      aggregate();
      for (int i = 0; i < NUM_COUNTER_TYPES; ++i)
        counters.counters[i] = gauge_mask[i] ? INVALID_GAUGE_VALUE : 0;
    }
    else if (const DaNetStatistics *daStat = get_danet_stat())
      daSample = *daStat;
    else
      memset(&daSample, 0, sizeof(daSample));
    nextUpdateTime = ct_ms + BUCKET_TIME_MS;
  }
};
static eastl::unique_ptr<Ctx> ctx;

void init() { ctx.reset(new Ctx); }
void term() { ctx.reset(); }

void update(uint32_t ct_ms)
{
  if (ctx)
    ctx->update(ct_ms);
}

void inc(CounterType type, cnt_t val) { counters.counters[type] += val; }

void set(CounterType type, cnt_t val) { counters.counters[type] = val; }

void max(CounterType type, cnt_t val) { counters.counters[type] = eastl::max(counters.counters[type], val); }

dag::ConstSpan<Sample> get_aggregations() { return ctx ? ctx->aggregated : dag::ConstSpan<Sample>(); }

} // namespace netstat
