// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "websocket_io.h"

#include <debug/dag_assert.h>
#include <debug/dag_debug.h>
#include <debug/dag_fatal.h>
#include <memory/dag_memBase.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_events.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_sockets.h>
#include <osApiWrappers/dag_threads.h>

#include <EASTL/algorithm.h>

#include <limits.h>

// curl_multi_poll and curl_multi_wakeup, 7.68.0
#define WEBSOCKET_CURL_MULTI_POLL (LIBCURL_VERSION_NUM >= 0x074400)

namespace websocket
{

namespace
{

class IoManager;

class ClientImpl final : public WebSocketClient
{
public:
  ClientImpl(ClientConfig &&config, IoConnectionFactory io_factory);
  ~ClientImpl() override;

  void connect(const eastl::string &uri, const eastl::string &connect_ip_hint, ConnectCb &&connect_cb,
    const Headers &additional_headers) override;
  void send(PayloadView message, bool is_text) override;
  void close(CloseStatus status) override;
  void poll() override;

  // Worker thread only
  void deliver(IoEvent &&event);
  void onDetached() { os_event_set_strict(&detachedEvent); }

  const IoSettings settings;
  const IoConnectionFactory factory;

private:
  IoManager *manager = nullptr;
  CloseCb closeCb;
  MessageCb messageCb;
  MessageCb messageAsyncCb;

  // constraint: only the worker thread pushes, so events keep the I/O order
  WinCritSec appQueueMutex;
  eastl::vector<IoEvent> appQueue;

  os_event_t detachedEvent;
  bool *pollAlive = nullptr;

public:
  // constraint: only the manager lock guards these
  ClientImpl *livePrev = nullptr;
  ClientImpl *liveNext = nullptr;

  volatile int64_t pollThreadId = 0;
};

struct IoCommand
{ //-V802
  enum class Type : uint8_t
  {
    Attach,
    Connect,
    ConnectAccepted,
    Send,
    Close,
    Detach
  };

  ClientImpl *client = nullptr;
  Payload payload;
  eastl::string uri;
  eastl::string connectIpHint;
  Headers headers;
  ConnectCbPtr connectCb;
  CloseStatus closeStatus = CloseStatus::Normal;
  Type type = Type::Attach;
  bool isText = false;
};

struct WorkerItem
{
  ClientImpl *client = nullptr;
  IoEvent event;
};

static constexpr int fallback_wait_ms = 10;
// constraint: a wakeup can fail to send, so even a wait that a wakeup would end lasts at most this long
static constexpr int max_wait_ms = 1000;

static volatile int io_wakeups = 0;
static bool force_wake_socket = false;
static volatile int drop_wakeups = 0;

// constraint: never destroyed and not on the heap, so a client destroyed after shutdown() still finds it
static WinCritSec &get_manager_mutex()
{
  alignas(WinCritSec) static char storage[sizeof(WinCritSec)];
  static WinCritSec *mutex = new (storage, _NEW_INPLACE) WinCritSec;
  return *mutex;
}

class IoManager
{
public:
  static IoManager *acquire(ClientImpl *client);
  static void unlink(ClientImpl *client);
  static void release();
  static IoStats getStats();

  void post(IoCommand &&command);
  void queueEvent(ClientImpl *client, IoEvent &&event) { pendingEvents.push_back({client, eastl::move(event)}); }

private:
  class IoThread final : public DaThread
  {
  public:
    explicit IoThread(IoManager &io_manager) : DaThread("WebSocketIO", 256 << 10), manager(io_manager) {}
    void execute() override { manager.ioLoop(); }

  private:
    IoManager &manager;
  };

  class WorkerThread final : public DaThread
  {
  public:
    explicit WorkerThread(IoManager &io_manager) : DaThread("WebSocketWorker", 256 << 10), manager(io_manager) {}
    void execute() override { manager.workerLoop(); }

  private:
    IoManager &manager;
  };

  IoManager();
  ~IoManager();

  void wakeIo();
  void ioLoop();
  void applyCommand(IoCommand &command);
  void connect(IoSlot &slot, IoCommand &command);
  void readTransferResults();
  int performConnections(bool perform_all);
  void waitForIo(int timeout_ms, int &ready_count);
  void openWakeSocket();
  void flushEvents();
  void workerLoop();
  bool isRunning() const { return ioThread.isThreadRunnning() && workerThread.isThreadRunnning(); }

  // constraint: created before and cleaned after both threads, so curl_multi_wakeup always has a valid handle
  CURLM *multi = nullptr;
  volatile int stopIo = 0;
  // constraint: without a working curl_multi_wakeup (older than 7.68, or built with CURL_DISABLE_SOCKETPAIR as for Sony),
  // wakeIo() sends a datagram to this self-connected socket, which every wait watches
  os_socket_t wakeSocket = OS_SOCKET_INVALID;
  bool curlWakeup = false;

  WinCritSec commandMutex;
  eastl::vector<IoCommand> commands;

  WinCritSec workerMutex;
  eastl::vector<WorkerItem> workerQueue;
  os_event_t workerEvent;

  // I/O thread only
  eastl::vector<eastl::unique_ptr<IoSlot>> slots;
  eastl::vector<WorkerItem> pendingEvents;
  eastl::vector<curl_waitfd> waitFds;
  eastl::vector<IoConnection *> waitConns;
  int runningTransfers = 0;
  CURLMcode lastWaitError = CURLM_OK;

  IoThread ioThread;
  WorkerThread workerThread;
};

static IoManager *manager_instance = nullptr;
static unsigned manager_clients = 0;
// constraint: intrusive, so the list leaves nothing on the heap for the exit leak check
static ClientImpl *live_clients = nullptr;

} // namespace

// I/O-side state of one client, owned by the I/O thread between Attach and Detach
class IoSlot
{
public:
  IoSlot(IoManager &io_manager, ClientImpl *owner) : manager(io_manager), client(owner) {}

  void emit(IoEvent &&event)
  {
    if (event.type == IoEvent::Type::Connect)
    {
      event.connectCb = eastl::move(connectCb);
      if (event.connectStatus != ConnectStatus::Ok)
        event.ip = connectIpHint;
    }
    route(eastl::move(event));
  }

  void acceptConnect()
  {
    holdEvents = false;
    eastl::vector<IoEvent> events;
    events.swap(heldEvents);
    for (IoEvent &event : events)
      route(eastl::move(event));
  }

  IoManager &manager;
  ClientImpl *const client;
  eastl::unique_ptr<IoConnection> conn;
  ConnectCbPtr connectCb;
  eastl::string connectIpHint;

private:
  void route(IoEvent &&event)
  {
    if (holdEvents)
    {
      heldEvents.push_back(eastl::move(event));
      return;
    }
    // constraint: the rest waits until poll() ran the connect callback, so onMessageAsync never runs before it
    if (event.type == IoEvent::Type::Connect && event.connectStatus == ConnectStatus::Ok)
      holdEvents = true;
    manager.queueEvent(client, eastl::move(event));
  }

  eastl::vector<IoEvent> heldEvents;
  bool holdEvents = false;
};

void IoConnection::emitConnected(const char *peer_ip)
{
  IoEvent event;
  event.type = IoEvent::Type::Connect;
  event.ip = peer_ip;
  slot.emit(eastl::move(event));
}

void IoConnection::emitConnectFailed(Error &&error)
{
  IoEvent event;
  event.type = IoEvent::Type::Connect;
  event.connectStatus = ConnectStatus::HttpConnectFailed;
  event.error = eastl::move(error);
  slot.emit(eastl::move(event));
  finished = true;
}

void IoConnection::emitMessage(Payload &&payload, bool is_text)
{
  IoEvent event;
  event.isText = is_text;
  event.payload = eastl::move(payload);
  slot.emit(eastl::move(event));
}

void IoConnection::emitClosed(CloseStatusInt status, Error &&error)
{
  IoEvent event;
  event.type = IoEvent::Type::Close;
  event.closeStatus = status;
  event.error = eastl::move(error);
  slot.emit(eastl::move(event));
  finished = true;
}

namespace
{

IoManager *IoManager::acquire(ClientImpl *client)
{
  WinAutoLock lock(get_manager_mutex());
  if (!manager_instance)
    manager_instance = new IoManager;
  ++manager_clients;
  client->liveNext = live_clients;
  if (live_clients)
    live_clients->livePrev = client;
  live_clients = client;
  return manager_instance;
}

void IoManager::unlink(ClientImpl *client)
{
  WinAutoLock lock(get_manager_mutex());
  (client->livePrev ? client->livePrev->liveNext : live_clients) = client->liveNext;
  if (client->liveNext)
    client->liveNext->livePrev = client->livePrev;
}

void IoManager::release()
{
  WinAutoLock lock(get_manager_mutex());
  G_ASSERT_RETURN(manager_clients > 0, );
  if (--manager_clients > 0)
    return;
  delete manager_instance;
  manager_instance = nullptr;
}

IoStats IoManager::getStats()
{
  IoStats stats;
  stats.ioWakeups = (unsigned)interlocked_acquire_load(io_wakeups);
  WinAutoLock lock(get_manager_mutex());
  stats.ioClients = manager_clients;
  stats.ioThreadRunning = manager_instance && manager_instance->isRunning();
  stats.ioWakeSocket = manager_instance && manager_instance->wakeSocket != OS_SOCKET_INVALID;
  return stats;
}

IoManager::IoManager() : ioThread(*this), workerThread(*this)
{
  multi = curl_multi_init();
  if (!multi)
    DAG_FATAL("failed to allocate curl multi handle");
  os_event_create(&workerEvent);
#if WEBSOCKET_CURL_MULTI_POLL
  // a probe that succeeds leaves one wakeup pending, so the first wait returns at once
  curlWakeup = !force_wake_socket && curl_multi_wakeup(multi) == CURLM_OK;
#endif
  if (!curlWakeup)
    openWakeSocket();
  if (!ioThread.start() || !workerThread.start())
    DAG_FATAL("failed to start the WebSocket threads");
}

IoManager::~IoManager()
{
  interlocked_release_store(stopIo, 1);
  wakeIo();
  ioThread.terminate(true);
  workerThread.terminate(true, -1, &workerEvent);
  G_ASSERT(slots.empty() && workerQueue.empty());
  curl_multi_cleanup(multi);
  os_event_destroy(&workerEvent);
  if (wakeSocket != OS_SOCKET_INVALID)
    os_socket_close(wakeSocket);
}

void IoManager::wakeIo()
{
  if (interlocked_acquire_load(drop_wakeups))
    return;
#if WEBSOCKET_CURL_MULTI_POLL
  if (curlWakeup)
  {
    curl_multi_wakeup(multi);
    return;
  }
#endif
  const char wake = 0;
  if (wakeSocket != OS_SOCKET_INVALID)
    os_socket_send(wakeSocket, &wake, sizeof(wake));
}

void IoManager::openWakeSocket()
{
  os_socket_addr addr;
  const os_socket_t s = os_socket_create(OSAF_IPV4, OST_UDP);
  bool ok = s != OS_SOCKET_INVALID && os_socket_addr_from_string(OSAF_IPV4, "127.0.0.1", &addr, sizeof(addr)) == 0 &&
            os_socket_bind(s, &addr, sizeof(addr)) == 0;
  curl_socklen_t addrLen = sizeof(addr);
  ok = ok && getsockname((curl_socket_t)s, (struct sockaddr *)&addr, &addrLen) == 0; //-V1027 sockaddr storage of os_socket_*
  ok = ok && os_socket_connect(s, &addr, (int)addrLen) == 0 && os_socket_set_option(s, OSO_NONBLOCK, 1) == 0;
  if (ok)
  {
    debug("[websocket] the I/O thread wakes through a wake socket, not curl_multi_wakeup");
    wakeSocket = s;
    return;
  }
  logwarn("[websocket] no curl_multi_wakeup and no wake socket (error %d), the I/O thread waits at most %d ms", os_socket_last_error(),
    fallback_wait_ms);
  if (s != OS_SOCKET_INVALID)
    os_socket_close(s);
}

void IoManager::post(IoCommand &&command)
{
  bool wake = false;
  {
    WinAutoLock lock(commandMutex);
    // constraint: a non-empty queue already has a wakeup pending, the I/O thread swaps it before its next wait
    wake = commands.empty();
    commands.push_back(eastl::move(command));
  }
  if (wake)
    wakeIo();
}

void IoManager::ioLoop()
{
  eastl::vector<IoCommand> batch;
  bool performAll = false;
  while (!interlocked_acquire_load(stopIo))
  {
    {
      WinAutoLock lock(commandMutex);
      batch.swap(commands);
    }
    for (IoCommand &command : batch)
      applyCommand(command);
    batch.clear();

    curl_multi_perform(multi, &runningTransfers);
    readTransferResults();

    const int timeoutMs = performConnections(performAll);
    flushEvents();

    int readyCount = 0;
    waitForIo(timeoutMs, readyCount);
    interlocked_increment(io_wakeups);

    bool anyReady = false;
    for (size_t i = 0; i < waitFds.size(); ++i)
    {
      waitConns[i]->readyEvents = waitFds[i].revents;
      anyReady |= waitFds[i].revents != 0;
    }
    // constraint: curl maps no POLLHUP or POLLERR into revents, so a wake without a known socket checks them all
    performAll = readyCount > 0 && !anyReady && !waitFds.empty();
  }
}

void IoManager::applyCommand(IoCommand &command)
{
  if (command.type == IoCommand::Type::Attach)
  {
    slots.push_back(eastl::make_unique<IoSlot>(*this, command.client));
    return;
  }

  auto it = eastl::find_if(slots.begin(), slots.end(),
    [client = command.client](const eastl::unique_ptr<IoSlot> &slot) { return slot->client == client; });
  // constraint: onMessageAsync can post for its client after Detach, such commands find no slot
  if (it == slots.end())
    return;
  IoSlot &slot = **it;
  if (slot.conn && slot.conn->isFinished())
    slot.conn.reset();

  switch (command.type)
  {
    case IoCommand::Type::Connect: connect(slot, command); break;

    case IoCommand::Type::ConnectAccepted: slot.acceptConnect(); break;

    case IoCommand::Type::Send:
      if (!slot.conn)
      {
        logwarn("[websocket] send failed. connection is not active");
        break;
      }
      slot.conn->send(eastl::move(command.payload), command.isText);
      slot.conn->performRequested = true;
      break;

    case IoCommand::Type::Close:
      if (slot.conn)
      {
        slot.conn->close(command.closeStatus);
        slot.conn->performRequested = true;
      }
      break;

    case IoCommand::Type::Detach:
    {
      slot.conn.reset();
      IoEvent event;
      event.type = IoEvent::Type::Detached;
      queueEvent(slot.client, eastl::move(event));
      slots.erase(it);
      break;
    }

    case IoCommand::Type::Attach: break;
  }
}

void IoManager::connect(IoSlot &slot, IoCommand &command)
{
  if (slot.conn)
  {
    IoEvent event;
    event.type = IoEvent::Type::Connect;
    event.connectStatus = ConnectStatus::ConnectIsActive;
    event.ip = slot.connectIpHint;
    event.connectCb = eastl::move(command.connectCb);
    queueEvent(slot.client, eastl::move(event));
    return;
  }

  ConnectRequest request;
  request.ipResolveEntry = create_ip_resolve_entry(command.uri, command.connectIpHint);
  request.uri = eastl::move(command.uri);
  request.headers = eastl::move(command.headers);
  slot.connectIpHint = !request.ipResolveEntry.empty() ? command.connectIpHint : "";
  slot.connectCb = eastl::move(command.connectCb);
  slot.conn = slot.client->factory(slot.client->settings, slot);
  slot.conn->start(multi, request);
}

void IoManager::readTransferResults()
{
  int queued = 0;
  while (CURLMsg *msg = curl_multi_info_read(multi, &queued))
  {
    char *priv = nullptr;
    if (msg->msg != CURLMSG_DONE || curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &priv) != CURLE_OK || !priv)
      continue;
    IoConnection *conn = reinterpret_cast<IoConnection *>(priv);
    conn->onTransferDone(msg->data.result);
    conn->performRequested = true;
  }
}

int IoManager::performConnections(bool perform_all)
{
  waitFds.clear();
  waitConns.clear();
  int timeoutMs = INT_MAX;
  for (const eastl::unique_ptr<IoSlot> &slot : slots)
  {
    IoConnection *conn = slot->conn.get();
    if (!conn)
      continue;

    int deadlineMs = 0;
    const bool deadlinePassed = conn->getDeadline(deadlineMs) && ms_since(deadlineMs) >= 0;
    const bool due = perform_all || conn->performRequested || conn->readyEvents || deadlinePassed || conn->hasPendingWork();
    if (due && !conn->isFinished())
    {
      conn->performRequested = false;
      conn->readyEvents = 0;
      conn->perform();
    }
    if (conn->isFinished())
    {
      slot->conn.reset();
      continue;
    }

    short events = 0;
    const curl_socket_t sock = conn->getWaitSocket(events);
    if (sock != CURL_SOCKET_BAD)
    {
      waitFds.push_back({sock, events, 0});
      waitConns.push_back(conn);
    }
    if (conn->hasPendingWork())
      timeoutMs = 0;
    else if (conn->getDeadline(deadlineMs))
      timeoutMs = eastl::min(timeoutMs, eastl::max(0, -ms_since(deadlineMs)));
  }
  return timeoutMs;
}

void IoManager::waitForIo(int timeout_ms, int &ready_count)
{
  // constraint: the wake socket sits in waitFds only during the wait, waitFds stays index-matched to waitConns after it
  const bool hasWakeSocket = wakeSocket != OS_SOCKET_INVALID;
  if (hasWakeSocket)
    waitFds.push_back({(curl_socket_t)wakeSocket, CURL_WAIT_POLLIN, 0});
  curl_waitfd *fds = waitFds.empty() ? nullptr : waitFds.data();
  const int waitMs = eastl::min(timeout_ms, curlWakeup || hasWakeSocket ? max_wait_ms : fallback_wait_ms);
#if WEBSOCKET_CURL_MULTI_POLL
  const CURLMcode mc = curl_multi_poll(multi, fds, (unsigned)waitFds.size(), waitMs, &ready_count);
#else
  const CURLMcode mc = curl_multi_wait(multi, fds, (unsigned)waitFds.size(), waitMs, &ready_count);
  // constraint: curl_multi_wait returns at once when it has nothing to wait on
  if (mc == CURLM_OK && runningTransfers == 0 && waitFds.empty())
    sleep_msec(waitMs);
#endif
  if (hasWakeSocket)
  {
    if (mc == CURLM_OK && waitFds.back().revents)
    {
      char drain[16];
      while (os_socket_recvfrom(wakeSocket, drain, sizeof(drain)) > 0) {}
      --ready_count;
    }
    waitFds.pop_back();
  }
  if (mc != CURLM_OK)
  {
    if (mc != lastWaitError)
      logwarn("[websocket] waiting for socket events failed, code %d '%s'", mc, curl_multi_strerror(mc));
    sleep_msec(fallback_wait_ms);
  }
  lastWaitError = mc;
}

void IoManager::flushEvents()
{
  if (pendingEvents.empty())
    return;
  {
    WinAutoLock lock(workerMutex);
    for (WorkerItem &item : pendingEvents)
      workerQueue.push_back(eastl::move(item));
  }
  pendingEvents.clear();
  os_event_set(&workerEvent);
}

void IoManager::workerLoop()
{
  eastl::vector<WorkerItem> batch;
  while (!workerThread.isThreadTerminating())
  {
    os_event_wait(&workerEvent, OS_WAIT_INFINITE);
    for (;;)
    {
      {
        WinAutoLock lock(workerMutex);
        batch.swap(workerQueue);
      }
      if (batch.empty())
        break;
      for (WorkerItem &item : batch)
      {
        if (item.event.type == IoEvent::Type::Detached)
          item.client->onDetached();
        else
          item.client->deliver(eastl::move(item.event));
      }
      batch.clear();
    }
  }
}

ClientImpl::ClientImpl(ClientConfig &&config, IoConnectionFactory io_factory) :
  settings{config.maxMessageSize, config.verifyCert, config.verifyHost, config.connectTimeoutMs, config.closeTimeoutMs},
  factory(io_factory)
{
  // constraint: assigned, not constructed: the MoveOnlyFunction move constructor reads a field an empty source leaves unset
  closeCb = eastl::move(config.onClose);
  messageCb = eastl::move(config.onMessage);
  messageAsyncCb = eastl::move(config.onMessageAsync);
  os_event_create(&detachedEvent);
  manager = IoManager::acquire(this);
  IoCommand command;
  command.client = this;
  manager->post(eastl::move(command));
}

ClientImpl::~ClientImpl()
{
  // constraint: leaves the list first, so shutdown() closing clients under the lock never touches it from now on
  IoManager::unlink(this);
  if (pollAlive)
    *pollAlive = false;
  IoCommand command;
  command.type = IoCommand::Type::Detach;
  command.client = this;
  manager->post(eastl::move(command));
  // constraint: the worker sets it after the last event of this client, so no callback runs once it is set
  os_event_wait(&detachedEvent, OS_WAIT_INFINITE);
  os_event_destroy(&detachedEvent);
  IoManager::release();
}

void ClientImpl::connect(const eastl::string &uri, const eastl::string &connect_ip_hint, ConnectCb &&connect_cb,
  const Headers &additional_headers)
{
  IoCommand command;
  command.type = IoCommand::Type::Connect;
  command.client = this;
  command.uri = uri;
  command.connectIpHint = connect_ip_hint;
  command.headers = additional_headers;
  if (connect_cb)
    command.connectCb = eastl::make_unique<ConnectCb>(eastl::move(connect_cb));
  manager->post(eastl::move(command));
}

void ClientImpl::send(PayloadView message, bool is_text)
{
  IoCommand command;
  command.type = IoCommand::Type::Send;
  command.client = this;
  command.isText = is_text;
  command.payload.assign(message.begin(), message.end());
  manager->post(eastl::move(command));
}

void ClientImpl::close(CloseStatus status)
{
  IoCommand command;
  command.type = IoCommand::Type::Close;
  command.client = this;
  command.closeStatus = status;
  manager->post(eastl::move(command));
}

void ClientImpl::poll()
{
  interlocked_release_store(pollThreadId, get_current_thread_id());
  eastl::vector<IoEvent> events;
  {
    WinAutoLock lock(appQueueMutex);
    if (appQueue.empty())
      return;
    events.swap(appQueue);
  }

  // constraint: a callback can destroy this client
  bool alive = true;
  pollAlive = &alive;
  for (IoEvent &event : events)
  {
    switch (event.type)
    {
      case IoEvent::Type::Connect:
        if (event.connectCb && *event.connectCb)
          (*event.connectCb)(this, event.connectStatus, event.ip, eastl::move(event.error));
        if (alive && event.connectStatus == ConnectStatus::Ok)
        {
          IoCommand command;
          command.type = IoCommand::Type::ConnectAccepted;
          command.client = this;
          manager->post(eastl::move(command));
        }
        break;
      case IoEvent::Type::Message:
        if (messageCb)
          messageCb(this, event.payload, event.isText);
        break;
      case IoEvent::Type::Close:
        if (closeCb)
          closeCb(this, event.closeStatus, eastl::move(event.error));
        break;
      case IoEvent::Type::Detached: break;
    }
    if (!alive)
      return;
  }
  pollAlive = nullptr;
}

void ClientImpl::deliver(IoEvent &&event)
{
  if (event.type == IoEvent::Type::Message && messageAsyncCb)
  {
    messageAsyncCb(this, event.payload, event.isText);
    return;
  }
  WinAutoLock lock(appQueueMutex);
  appQueue.push_back(eastl::move(event));
}

} // namespace

eastl::unique_ptr<WebSocketClient> create_io_client(ClientConfig &&config, IoConnectionFactory factory)
{
  return eastl::make_unique<ClientImpl>(eastl::move(config), factory);
}

IoStats get_io_stats() { return IoManager::getStats(); }

static unsigned live_client_count()
{
  WinAutoLock lock(get_manager_mutex());
  unsigned count = 0;
  for (ClientImpl *c = live_clients; c; c = c->liveNext)
    ++count;
  return count;
}

// constraint: only clients this thread polls, whose destructor also runs on it, so a client still in the list right
// before its poll() stays valid during it, unless a callback of that poll() destroys it
static void poll_own_live_clients(int64_t thread_id)
{
  eastl::vector<ClientImpl *> clients;
  {
    WinAutoLock lock(get_manager_mutex());
    for (ClientImpl *c = live_clients; c; c = c->liveNext)
      if (interlocked_acquire_load(c->pollThreadId) == thread_id)
        clients.push_back(c);
  }
  for (ClientImpl *client : clients)
  {
    bool live = false;
    {
      WinAutoLock lock(get_manager_mutex());
      for (ClientImpl *c = live_clients; c && !live; c = c->liveNext)
        live = c == client;
    }
    if (live)
      client->poll();
  }
}

unsigned shutdown(int close_wait_ms)
{
  {
    WinAutoLock lock(get_manager_mutex());
    for (ClientImpl *c = live_clients; c; c = c->liveNext)
      c->close(CloseStatus::GoingAway);
  }
  const int64_t threadId = get_current_thread_id();
  const int startMs = get_time_msec();
  while (live_client_count() > 0 && ms_since(startMs) < close_wait_ms)
  {
    poll_own_live_clients(threadId);
    sleep_msec(1);
  }
  const unsigned alive = live_client_count();
  if (alive > 0)
    logerr("[websocket] %u client(s) not released within %d ms of the shutdown close", alive, close_wait_ms);
  return alive;
}

void force_wake_socket_for_tests(bool force) { force_wake_socket = force; }

void drop_wakeups_for_tests(bool drop) { interlocked_release_store(drop_wakeups, drop ? 1 : 0); }

} // namespace websocket
