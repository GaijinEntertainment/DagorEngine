#include <daECS/net/netbase.h>
#include <daECS/net/time.h>
#include <generic/dag_carray.h>
#include <daECS/net/msgDecl.h>
#include <daECS/net/msgSink.h>
#include <daNet/getTime.h>
#include <perfMon/dag_cpuFreq.h>
#include <math.h>
#include <stdlib.h> // abs
#include <EASTL/algorithm.h>
#include <debug/dag_debug.h>
#include <memory.h>
#include <limits.h>

#define INITIAL_TIME_SYNC_INTERVAL_MS 250
#define NUM_INITIAL_TIME_PROBES       4
#define MAX_TIME_STEP_MS              1000

struct TimeRec
{
  int rtt, timeSkew;
  int score() const { return rtt + abs(timeSkew); }
  bool operator<(const TimeRec &rhs) const { return score() < rhs.score(); }
};

class ClientTime final : public ITimeManager //-V730
{
public:
  int64_t getUsec() const { return ref_time_delta_to_usec(ref_time_ticks() - epochTicks); }

  void updateSrvOfs(int rdt_us)
  {
    if (!numTimeRec || !timeSkewMs)
      return;
    int skewDiff = (int)sqrtf(abs(timeSkewMs));
    if (timeSkewMs < 0)
      skewDiff = -eastl::min(skewDiff, rdt_us / 1000);
    timeSkewMs -= skewDiff;
    srvOfsUs += skewDiff * 1000;
    // debug("%s skew=%d skd=%d offs=%d rdtus=%d", __FUNCTION__, timeSkewMs, skewDiff, (int)srvOfsUs, rdt_us);
  }

  int getType4CC() const override { return _MAKE4C('CLIT'); }

  // ITimeManager
  double advance(float, float &rt_dt) override
  {
    double prevT = getSeconds();
    int64_t prevCurTimeUs = curTimeUs;
    curTimeUs = getUsec();
    updateTimeRequest(getAsyncMillis());
    updateSrvOfs(int(curTimeUs - prevCurTimeUs));
    double curT = getSeconds();
    rt_dt = curT - prevT;
    return curT;
  }
  double getSeconds() const override { return (curTimeUs + srvOfsUs) / 1000000.; }
  int getMillis() const override { return (int)((curTimeUs + srvOfsUs) / 1000. + 0.5); }
  int getAsyncMillis() const override { return getUsec() / 1000; }

  static void onReceiveTimeSyncResponseTrampoline(const net::IMessage *msg)
  {
    auto respMsg = msg->cast<TimeSyncResponse>();
    G_ASSERT(respMsg);
    static_cast<ClientTime &>(get_time_mgr()).onReceiveTimeSyncResponse(respMsg);
  }

  void onReceiveTimeSyncResponse(const TimeSyncResponse *rmsg)
  {
    int64_t absCurTimeUs = getUsec();
    int curTime = (absCurTimeUs + srvOfsUs) / 1000;

    int sentTime = rmsg->get<0>(), receivedServerTime = rmsg->get<1>();
    G_ASSERT(receivedServerTime >= 0);

    int rtt = eastl::max(int(rmsg->rcvTime - sentTime), 1);
    int localQueueCorrection = (int)danet::GetTime() - rmsg->rcvTime;
    int serverTime = receivedServerTime + localQueueCorrection + rtt / 2; // https://en.wikipedia.org/wiki/Cristian%27s_algorithm

    timesync_push_rtt(rtt);

    TimeRec &tc = timeHistory[numTimeRec++ % TIME_HISTORY_DEPTH];
    tc.rtt = rtt;
    tc.timeSkew = serverTime - curTime;
    // debug("received rtt %d, server time %d, local time %d, queue %d, skew %d",
    //       tc.rtt, serverTime, curTime, localQueueCorrection, tc.timeSkew);

    // select record with lowest RTT for TIME_HISTORY_DEPTH*TIME_SYNC_INTERVAL_MS period
    TimeRec minTimeRec{INT_MAX / 2, INT_MAX / 2};
    for (int i = 0, num = eastl::min(numTimeRec, (unsigned)TIME_HISTORY_DEPTH); i < num; ++i)
      if (timeHistory[i] < minTimeRec)
        minTimeRec = timeHistory[i];

    if (memcmp(&bestTimeRec, &minTimeRec, sizeof(bestTimeRec)) != 0)
    {
      if (abs(minTimeRec.timeSkew) < MAX_TIME_STEP_MS)
      {
        bestTimeRec = minTimeRec;
        timeSkewMs = bestTimeRec.timeSkew;
      }
      else
      {
        srvOfsUs = int64_t(serverTime) * 1000 - absCurTimeUs;
        timeSkewMs = 0;
        bestTimeRec = tc;
      }
      debug("applied rtt %d, skew %d ms", bestTimeRec.rtt, bestTimeRec.timeSkew);
    }
    float _1;
    advance(0.f, _1);
  }
  // ~ITimeManager

  void updateTimeRequest(int cur_time)
  {
    if (!net::get_msg_sink() || cur_time <= nextRequestTimeMs)
      return;
    send_net_msg(net::get_msg_sink(), TimeSyncRequest(danet::GetTime()));
    nextRequestTimeMs = cur_time + (numTimeRec < NUM_INITIAL_TIME_PROBES ? INITIAL_TIME_SYNC_INTERVAL_MS : TIME_SYNC_INTERVAL_MS);
  }

private:
  static constexpr int TIME_HISTORY_DEPTH = 8;
  carray<TimeRec, TIME_HISTORY_DEPTH> timeHistory; // cyclic buffer
  TimeRec bestTimeRec = {0, 0};
  unsigned numTimeRec = 0;
  const int64_t epochTicks = ref_time_ticks(); // fixed ref point in time
  int64_t srvOfsUs = 0;
  int64_t curTimeUs = 0;
  int timeSkewMs = 0;
  int nextRequestTimeMs = 0;
};

class ReplayTime final : public ITimeManager // similar to AccumTime but handle TimeSyncResponse
{
  double advance(float dt, float &) override
  {
    syncTime += dt;
    asyncTime += dt;
    return syncTime;
  }
  double getSeconds() const override { return syncTime; }
  int getMillis() const override { return (int)(syncTime * 1000.0 + 0.5); }
  int getAsyncMillis() const override { return (int)(asyncTime * 1000.0 + 0.5); }
  int getType4CC() const override { return _MAKE4C('RPLT'); }

  void onReceiveTimeSyncResponse(const TimeSyncResponse *rmsg)
  {
    if (timeSyncResponseHandled)
      return; // ignore all time sync responses except initial one

    int receivedServerTime = rmsg->get<1>();
    G_ASSERT(receivedServerTime > 0);

    int curTimeMs = int(asyncTime / 1000.);
    if (curTimeMs > receivedServerTime)
      logwarn("%s time is %d ms ahead of server time, time will go \"backwards\"", __FUNCTION__, curTimeMs - receivedServerTime);

    syncTime = receivedServerTime / 1000.;
    timeSyncResponseHandled = true;
  }

  double syncTime = 0, asyncTime = 0;
  bool timeSyncResponseHandled = false;
  friend void on_time_sync_response_received(const net::IMessage *msg);

public:
  ReplayTime(double start_time = 0.0) : syncTime(start_time), asyncTime(start_time) {}
};

void on_time_sync_response_received(const net::IMessage *msg) // non-static due to friendship
{
  auto respMsg = msg->cast<TimeSyncResponse>();
  G_ASSERT(respMsg);
  ITimeManager &tmgr = get_time_mgr();
  int t4cc = tmgr.getType4CC();
  if (t4cc == _MAKE4C('CLIT'))
    static_cast<ClientTime &>(tmgr).onReceiveTimeSyncResponse(respMsg);
  else if (t4cc == _MAKE4C('RPLT'))
    static_cast<ReplayTime &>(tmgr).onReceiveTimeSyncResponse(respMsg);
  else
    G_ASSERTF(0, "Unknown/unsupported time impl %x", t4cc);
}

ECS_NET_IMPL_MSG(TimeSyncResponse, net::ROUTING_SERVER_TO_CLIENT, &net::direct_connection_rcptf, UNRELIABLE, TIMESYNC_NET_CHANNEL,
  net::MF_URGENT, ECS_NET_NO_DUP, &on_time_sync_response_received);

ITimeManager *create_client_time() { return new ClientTime(); }
ITimeManager *create_replay_time(double start_time) { return new ReplayTime(start_time); }
