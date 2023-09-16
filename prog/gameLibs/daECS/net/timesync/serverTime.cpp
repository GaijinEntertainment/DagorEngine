#include <daECS/net/netbase.h>
#include <daECS/net/time.h>
#include <daECS/net/msgDecl.h>
#include <daECS/net/msgSink.h>
#include <daNet/getTime.h>
#include <math.h>
#include <perfMon/dag_cpuFreq.h>
#include <debug/dag_assert.h>

class ServerTime final : public ITimeManager
{
public:
  int64_t getUsec() const { return ref_time_delta_to_usec(ref_time_ticks() - refTicks); }

  // ITimeManager
  double advance(float, float &) override
  {
    curTimeUs = getUsec();
    return getSeconds();
  }
  double getSeconds() const override { return curTimeUs / 1000000.; }
  int getMillis() const override { return (int)(curTimeUs / 1000. + 0.5); }
  int getAsyncMillis() const override { return getUsec() / 1000; }

  int getType4CC() const override { return _MAKE4C('SRVT'); }

  static void onReceiveTimeSyncRequestMsg(const net::IMessage *imsg)
  {
    auto msg = imsg->cast<TimeSyncRequest>();
    G_ASSERT(msg);
    auto &self = static_cast<const ServerTime &>(get_time_mgr());
    int localQueueCorrection = danet::GetTime() - msg->rcvTime;
    int clientTime = msg->get<0>() + localQueueCorrection;
    TimeSyncResponse respMsg(clientTime, self.getMillis());
    respMsg.connection = imsg->connection;
    send_net_msg(net::get_msg_sink(), eastl::move(respMsg));
  }
  // ~ITimeManager

private:
  int64_t curTimeUs = 0;
  int64_t refTicks = ref_time_ticks();
};

ECS_NET_IMPL_MSG(TimeSyncRequest, net::ROUTING_CLIENT_TO_SERVER, ECS_NET_NO_RCPTF, UNRELIABLE, TIMESYNC_NET_CHANNEL, net::MF_URGENT,
  ECS_NET_NO_DUP, &ServerTime::onReceiveTimeSyncRequestMsg);

ITimeManager *create_server_time() { return new ServerTime(); }
