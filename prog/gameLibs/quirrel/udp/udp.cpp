#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_events.h>
#include <osApiWrappers/dag_sockets.h>
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_atomic.h>
#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_delayedAction.h>
#include <EASTL/bonus/fixed_ring_buffer.h>
#include <EASTL/unordered_map.h>
#include <ioEventsPoll/ioEventsPoll.h>
#include <quirrel/sqEventBus/sqEventBus.h>
#include <quirrel/sqModules/sqModules.h>
#include <quirrel_json/quirrel_json.h>
#include <sqstdblob.h>

namespace bindquirrel::udp
{


static constexpr int MAX_MTU = 1200;
static constexpr int INPUT_BUFFER_SIZE = 16;

struct SocketInfo
{
  os_socket_t s;
  eastl::string id;
};

struct UDPPacket
{
  eastl::shared_ptr<const SocketInfo> sinfo;
  os_socket_addr from;
  int recvTime;
  eastl::fixed_vector<char, MAX_MTU, false> data;
};

using InputBuffer = eastl::fixed_ring_buffer<UDPPacket, INPUT_BUFFER_SIZE>;

static WinCritSec input_buffer_mutex;
static eastl::unique_ptr<InputBuffer> input_buffer;
static UDPPacket current_packet;

static eastl::unordered_map<eastl::string, SocketInfo> sockets;
static volatile int sockets_num = 0;

class SocketsPollThread final : public DaThread
{
public:
  os_event_t wakeupEvent;
  eastl::unique_ptr<ioevents::IOEventsPoll> iopoll;
  WinCritSec iopollMutex;

  SocketsPollThread() : DaThread("UdpPoll"), iopoll(ioevents::create_poll()) { os_event_create(&wakeupEvent); }

  ~SocketsPollThread()
  {
    os_event_set(&wakeupEvent);
    os_event_destroy(&wakeupEvent);
  }

  void execute() override
  {
    while (!terminating)
    {
      {
        WinAutoLock lock(iopollMutex);
        iopoll->poll();
      }
      int waitMs = sockets_num > 0 ? 1 : 10;
      os_event_wait(&wakeupEvent, waitMs);
    }
  }

  void onSocketReadReady(const eastl::shared_ptr<const SocketInfo> &sinfo)
  {
    os_socket_addr from;
    int recvts = get_time_msec();
    while (true)
    {
      UDPPacket &p = current_packet;
      p.recvTime = recvts;
      p.sinfo = sinfo;
      int fromLen = sizeof(os_socket_addr);
      p.data.resize(p.data.max_size());
      int nread = os_socket_recvfrom(sinfo->s, p.data.data(), p.data.max_size(), 0, &p.from, &fromLen);
      if (nread <= 0)
        break;
      p.data.resize(nread);
      WinAutoLock lock(input_buffer_mutex);
      if (!input_buffer->full())
        input_buffer->push_back(p);
    }
  }

  void registerSocket(const SocketInfo &sinfo)
  {
    WinAutoLock lock(iopollMutex);
    iopoll->subscribe(os_socket_sys_handle(sinfo.s), ioevents::EventFlag::READ,
      ioevents::make_io_callback([sinfo = eastl::make_shared<const SocketInfo>(sinfo), this](ioevents::FileDesc, ioevents::EventFlag,
                                   ioevents::EventData const &) { this->onSocketReadReady(sinfo); }));
    os_event_set(&wakeupEvent);
  }

  void unregisterSocket(os_socket_t s)
  {
    WinAutoLock lock(iopollMutex);
    iopoll->unsubscribe(os_socket_sys_handle(s), ioevents::EventFlag::READ);
  }
};

static eastl::unique_ptr<SocketsPollThread> poll_thread;

static void update(void *)
{
  TIME_PROFILE(udp_update);

  WinAutoLock lock(input_buffer_mutex);
  for (UDPPacket &p : *input_buffer)
  {
    if (sockets.find(p.sinfo->id) == sockets.end())
      continue; // packet from unregistered socket
                //
    char addr[20] = {0};
    int port = os_socket_addr_get_port(&p.from, sizeof(p.from));
    os_socket_addr_to_string(&p.from, sizeof(p.from), addr, sizeof(addr));

    if (HSQUIRRELVM vm = sqeventbus::get_vm())
    {
      Sqrat::Table evt(vm);
      evt.SetValue("recvTime", p.recvTime);
      evt.SetValue("host", addr);
      evt.SetValue("port", port);
      evt.SetValue("socketId", p.sinfo->id);
      SQUserPointer destPtr = sqstd_createblob(vm, p.data.size());
      memcpy(destPtr, p.data.data(), p.data.size());
      evt.SetValue("data", Sqrat::Var<Sqrat::Object>(vm, -1).value);
      sq_pop(vm, 1);
      sqeventbus::send_event("udp.on_packet", evt);
    }
  }
  input_buffer->clear();
}

void init()
{
  os_sockets_init();
  input_buffer = eastl::make_unique<InputBuffer>(INPUT_BUFFER_SIZE);
  input_buffer->clear();
  poll_thread = eastl::make_unique<SocketsPollThread>();
  poll_thread->start();
  poll_thread->setAffinity(WORKER_THREADS_AFFINITY_MASK);
  register_regular_action_to_idle_cycle(update, nullptr);
}

void shutdown()
{
  unregister_regular_action_to_idle_cycle(update, nullptr);
  if (poll_thread)
    poll_thread->terminate(true);
  poll_thread.reset();
  input_buffer.reset();
}

static os_socket_t get_socket(eastl::string_view socket_id)
{
  G_ASSERT_RETURN(!!poll_thread, OS_SOCKET_INVALID);
  if (auto it = sockets.find_as(socket_id); it != sockets.end())
    return it->second.s;

  os_socket_t s = os_socket_create(OsSocketAddressFamily::OSAF_IPV4, OsSocketType::OST_UDP);
  if (s == OS_SOCKET_INVALID)
  {
    char errbuf[32] = {0};
    logwarn("[UDP] failed to create socket: %s", os_socket_error_str(os_socket_last_error(), errbuf, sizeof(errbuf)));
    return s;
  }
  os_socket_set_option(s, OsSocketOption::OSO_NONBLOCK, 1);
  eastl::string idstr(socket_id);
  auto it = sockets.insert(eastl::make_pair(idstr, SocketInfo{s, idstr})).first;
  poll_thread->registerSocket(it->second);
  interlocked_increment(sockets_num);
  return s;
}

static void close_socket(eastl::string_view socket_id)
{
  G_ASSERT_RETURN(!!poll_thread, );
  auto it = sockets.find_as(socket_id);
  if (it == sockets.end())
    return;
  os_socket_t s = it->second.s;
  poll_thread->unregisterSocket(s);
  interlocked_decrement(sockets_num);
  sockets.erase(it);
  os_socket_close(s);
}


static bool send(eastl::string_view socket_id, eastl::string_view host, int port, Sqrat::Object data)
{
  os_socket_t s = get_socket(socket_id);
  os_socket_addr addr;
  if (os_socket_addr_from_string(OsSocketAddressFamily::OSAF_IPV4, host.data(), &addr, sizeof(addr)) != 0)
  {
    logwarn("[UDP] bad host address %s", host.data());
    return false;
  }
  os_socket_addr_set_port(&addr, sizeof(addr), port);
  if (data.GetType() == OT_STRING)
  {
    Sqrat::Var sv = data.GetVar<eastl::string_view>();
    return os_socket_sendto(s, sv.value.data(), sv.value.size(), 0, &addr, sizeof(addr)) == sv.value.size();
  }
  else if (data.GetType() == OT_INSTANCE)
  {
    HSQUIRRELVM vm = data.GetVM();
    sq_pushobject(vm, data.GetObject());
    SQUserPointer blobPtr;
    if (SQ_SUCCEEDED(sqstd_getblob(vm, -1, &blobPtr)))
    {
      SQInteger blobSize = sqstd_getblobsize(vm, -1);
      sq_pop(vm, 1);
      return os_socket_sendto(s, static_cast<char *>(blobPtr), blobSize, 0, &addr, sizeof(addr)) == blobSize;
    }
    sq_pop(vm, 1);
  }
  G_ASSERTF(false, "data parameter must be either blob or string");
  return false;
}


static bool socket_bind(eastl::string_view socket_id, eastl::string_view host, int port)
{
  os_socket_t s = get_socket(socket_id);
  if (s == OS_SOCKET_INVALID)
    return false;

  os_socket_addr addr;
  if (os_socket_addr_from_string(OsSocketAddressFamily::OSAF_IPV4, host.data(), &addr, sizeof(addr)) != 0)
  {
    logwarn("[UDP] failed to bind socket: bad host address %s", host.data());
    return false;
  }
  os_socket_addr_set_port(&addr, sizeof(addr), port);

  int err = os_socket_bind(s, &addr, sizeof(addr));
  if (err != 0)
  {
    char errbuf[32] = {0};
    logwarn("[UDP] failed to bind socket: %s", os_socket_error_str(os_socket_last_error(), errbuf, sizeof(errbuf)));
    return false;
  }
  return true;
}

static eastl::string last_error()
{
  char errbuf[32] = {0};
  return os_socket_error_str(os_socket_last_error(), errbuf, sizeof(errbuf));
}

static eastl::string socket_error(eastl::string_view socket_id)
{
  char errbuf[32] = {0};
  return os_socket_error_str(os_socket_error(get_socket(socket_id)), errbuf, sizeof(errbuf));
}

void bind(SqModules *module_mgr)
{
  Sqrat::Table apiNs(module_mgr->getVM());
  apiNs.Func("send", send)
    .Func("bind", socket_bind)
    .Func("close_socket", close_socket)
    .Func("socket_error", socket_error)
    .Func("last_error", last_error);


  module_mgr->addNativeModule("dagor.udp", apiNs);
}

} // namespace bindquirrel::udp
