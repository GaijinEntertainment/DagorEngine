#include <ev.h>
#include <debug/dag_logSys.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_events.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_sockets.h>
#include <perfMon/dag_statDrv.h>

#include <EASTL/algorithm.h>
#include <EASTL/map.h>
#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>
#include <errno.h>

#include <ioEventsPoll/ioEventsPoll.h>

#if _TARGET_PC_WIN | _TARGET_XBOX
extern "C" __declspec(dllimport) int __stdcall WSAGetLastError();
#endif

namespace ioevents
{

template <class Ev>
struct CallbackData
{
  IOCallbackPtr callback;
  Ev watcher;
};

using IOSubsMap = eastl::map<FileDesc, CallbackData<ev_io>>;
using ChildSubsMap = eastl::map<FileDesc, CallbackData<ev_child>>;
using ActionsList = eastl::vector<ActionCallback *>;

typedef struct ev_loop EvLoop;
struct EvLoopDestroyer
{
  void operator()(EvLoop *l) { l ? ev_loop_destroy(l) : (void)0; }
};
static eastl::unique_ptr<EvLoop, EvLoopDestroyer> default_loop;
static inline EvLoop *get_default_loop()
{
  if (!default_loop)
    default_loop.reset(ev_default_loop(0));
  return default_loop.get();
}

static int map_ev_event(EventFlag event)
{
  if (event == EventFlag::READ)
    return EV_READ;
  return EV_WRITE;
}

static void syserr_cb(const char *msg) EV_THROW
{
  // Note: this fatal is important, WT has special handling for it ("(libev) error creating signal/async pipe")
#if _TARGET_PC_WIN | _TARGET_XBOX
  int err = WSAGetLastError();
#else
  int err = errno;
#endif
  DAG_FATAL("%s, err=%d", msg, err);
}
static void dummy_cb(EvLoop *, ev_async *, int) {}

static void io_ev_cb(EvLoop *loop, ev_io *w, int revents)
{
  G_UNREFERENCED(loop);
  G_UNREFERENCED(revents);
  IOCallback *callback = static_cast<IOCallback *>(w->data);
  if (revents & EV_READ)
    callback->onIOEvents(w->fd, EventFlag::READ, EventData());
  if (revents & EV_WRITE)
    callback->onIOEvents(w->fd, EventFlag::WRITE, EventData());
}

static void child_ev_cb(EvLoop *loop, ev_child *w, int revents)
{
  G_UNREFERENCED(loop);
  G_UNREFERENCED(revents);
  IOCallback *callback = static_cast<IOCallback *>(w->data);
  ChildEventData eventData;
  eventData.rstatus = w->rstatus;
  callback->onIOEvents(w->pid, EventFlag::CHILD_EXIT, eventData);
}

#if EV_CHILD_ENABLE
static void subscribe_child(EvLoop *loop, ChildSubsMap &subscribers, FileDesc child_pid, IOCallbackPtr cb)
{
  auto it = subscribers.find(child_pid);
  if (it != subscribers.end())
  {
    CallbackData<ev_child> &cd = it->second;
    cd.callback = cb;
    cd.watcher.data = cb.get();
    return;
  }

  CallbackData<ev_child> &cd = subscribers[child_pid];
  cd.callback = cb;
  ev_child_init(&cd.watcher, child_ev_cb, child_pid, 0);
  cd.watcher.data = cb.get();
  ev_child_start(loop, &cd.watcher);
}

static void unsubscribe_child(EvLoop *loop, ChildSubsMap &subscribers, FileDesc child_pid)
{
  auto it = subscribers.find(child_pid);
  if (it != subscribers.end())
  {
    ev_child &watcher = it->second.watcher;
    IOCallback *callback = static_cast<IOCallback *>(watcher.data);
    callback->onUnsubscribed(child_pid, EventFlag::CHILD_EXIT);
    ev_child_stop(loop, &watcher);
    subscribers.erase(it);
  }
}

static void unsubscribe_child_all(EvLoop *loop, ChildSubsMap &subscribers)
{
  for (auto &kv : subscribers)
  {
    ev_child &watcher = kv.second.watcher;
    IOCallback *callback = static_cast<IOCallback *>(watcher.data);
    callback->onUnsubscribed(kv.first, EventFlag::CHILD_EXIT);
    ev_child_stop(loop, &watcher);
  }
  subscribers.clear();
}
#else
static void subscribe_child(EvLoop *loop, ChildSubsMap &subscribers, FileDesc child_pid, IOCallbackPtr cb)
{
  DAG_FATAL("EV_CHILD events are not supported on current platform");
}

static void unsubscribe_child(EvLoop *loop, ChildSubsMap &subscribers, FileDesc child_pid)
{
  DAG_FATAL("EV_CHILD events are not supported on current platform");
}

static void unsubscribe_child_all(EvLoop *loop, ChildSubsMap &subscribers) {}
#endif

static void subscribe_io(EvLoop *loop, IOSubsMap &subscribers, FileDesc fd, EventFlag event, IOCallbackPtr cb)
{
  auto it = subscribers.find(fd);
  if (it != subscribers.end())
  {
    CallbackData<ev_io> &cd = it->second;
    cd.callback = cb;
    cd.watcher.data = cb.get();
    return;
  }

  CallbackData<ev_io> &cd = subscribers[fd];
  cd.callback = cb;
  ev_io_init(&cd.watcher, io_ev_cb, fd, map_ev_event(event));
  cd.watcher.data = cb.get();
  ev_io_start(loop, &cd.watcher);
}

static void unsubscribe_io(EvLoop *loop, IOSubsMap &subscribers, FileDesc fd, EventFlag event)
{
  auto it = subscribers.find(fd);
  if (it != subscribers.end())
  {
    ev_io &watcher = it->second.watcher;
    IOCallback *callback = static_cast<IOCallback *>(watcher.data);
    callback->onUnsubscribed(fd, event);
    ev_io_stop(loop, &watcher);
    subscribers.erase(it);
  }
}

void unsubscribe_io_all(EvLoop *loop, IOSubsMap &subscribers, EventFlag event)
{
  for (auto &kv : subscribers)
  {
    const FileDesc &fd = kv.first;
    ev_io &watcher = kv.second.watcher;
    IOCallback *callback = static_cast<IOCallback *>(watcher.data);
    ev_io_stop(loop, &watcher);
    callback->onUnsubscribed(fd, event);
  }
  subscribers.clear();
}

class DelayedActions
{
public:
  void add(eastl::unique_ptr<ActionCallback> cb)
  {
    WinAutoLock lock(mutex);
    current.push_back(eastl::move(cb));
  }

  void perform()
  {
    {
      WinAutoLock lock(mutex);
      eastl::swap(next, current);
    }
    for (auto &cb : next)
      cb->perform();
    next.clear();
  }

private:
  eastl::vector<eastl::unique_ptr<ActionCallback>> current, next;

  WinCritSec mutex;
};

class IOEventsPollImpl : public IOEventsPoll
{
public:
  IOEventsPollImpl(bool use_default_loop);
  ~IOEventsPollImpl();

  void poll() override;
  void subscribe(FileDesc fd, EventFlags events, IOCallbackPtr cb) override;
  void unsubscribe(FileDesc fd, EventFlags events) override;

  void asyncCloseSocket(FileDesc fd) override;

  void addDelayedAction(eastl::unique_ptr<ActionCallback> cb) override;

  void addUpdateAction(ActionCallback *cb) override;
  void removeUpdateAction(ActionCallback *cb) override;

private:
  EvLoop *loop;

  IOSubsMap readSubscribers;
  IOSubsMap writeSubscribers;
  ChildSubsMap childProcSubscribers;

  DelayedActions delayedActions;
  ActionsList postPollActions;
  bool updatePerforming = false;
  bool useDefaultLoop = false;
};

IOEventsPollImpl::IOEventsPollImpl(bool use_default_loop)
{
  useDefaultLoop = use_default_loop;
  debug("[ioevents] IOEventsPollImpl::%s%s", __FUNCTION__, useDefaultLoop ? " (default)" : "");
  if (useDefaultLoop)
    loop = get_default_loop();
  else
    loop = ev_loop_new(0);
  ev_set_syserr_cb(syserr_cb);
}

IOEventsPollImpl::~IOEventsPollImpl()
{
  debug("[ioevents] IOEventsPollImpl::%s", __FUNCTION__);
  poll();
  unsubscribe_io_all(loop, readSubscribers, EventFlag::READ);
  unsubscribe_io_all(loop, writeSubscribers, EventFlag::WRITE);
  unsubscribe_child_all(loop, childProcSubscribers);

  if (!useDefaultLoop)
    ev_loop_destroy(loop);
}

void IOEventsPollImpl::addDelayedAction(eastl::unique_ptr<ActionCallback> cb) { delayedActions.add(eastl::move(cb)); }

void IOEventsPollImpl::poll()
{
  TIME_PROFILE_DEV(ioevents_poll);

  ev_run(loop, EVRUN_NOWAIT);
  delayedActions.perform();

  updatePerforming = true;
  for (ActionCallback *cb : postPollActions)
    cb->perform();
  updatePerforming = false;
}

void IOEventsPollImpl::addUpdateAction(ActionCallback *cb)
{
  G_ASSERT(!updatePerforming);

  G_ASSERT(eastl::find(postPollActions.begin(), postPollActions.end(), cb) == postPollActions.end());
  postPollActions.push_back(cb);
}

void IOEventsPollImpl::removeUpdateAction(ActionCallback *cb)
{
  G_ASSERT(!updatePerforming);

  postPollActions.erase(eastl::remove(postPollActions.begin(), postPollActions.end(), cb), postPollActions.end());
}

void IOEventsPollImpl::subscribe(FileDesc fd, EventFlags events, IOCallbackPtr cb)
{
  if (events & EventFlag::READ)
    subscribe_io(loop, readSubscribers, fd, EventFlag::READ, cb);
  if (events & EventFlag::WRITE)
    subscribe_io(loop, writeSubscribers, fd, EventFlag::WRITE, cb);
  if (events & EventFlag::CHILD_EXIT)
  {
    G_ASSERTF(useDefaultLoop, "child events are allowed only in default poll");
    subscribe_child(loop, childProcSubscribers, fd, cb);
  }
}

void IOEventsPollImpl::unsubscribe(FileDesc fd, EventFlags events)
{
  if (events & EventFlag::READ)
    unsubscribe_io(loop, readSubscribers, fd, EventFlag::READ);
  if (events & EventFlag::WRITE)
    unsubscribe_io(loop, writeSubscribers, fd, EventFlag::WRITE);
  if (events & EventFlag::CHILD_EXIT)
    unsubscribe_child(loop, childProcSubscribers, fd);
}

void IOEventsPollImpl::asyncCloseSocket(FileDesc fd)
{
  unsubscribe(fd, EventFlag::READ | EventFlag::WRITE);
  os_socket_close(fd);
}

class IOEventsPollLoopImpl : public IOEventsPollLoop
{
public:
  using IOEventsPollLoop::addDelayedAction;

  IOEventsPollLoopImpl(int tick_ms, bool use_default_loop);
  ~IOEventsPollLoopImpl();

  void run() override;
  void stop(eastl::unique_ptr<ActionCallback> on_stopped) override;
  void stopSync() override;

  void subscribe(FileDesc fd, EventFlags events, IOCallbackPtr cb) override;
  void unsubscribe(FileDesc fd, EventFlags events) override;
  void asyncCloseSocket(FileDesc fd) override;

  void addDelayedAction(eastl::unique_ptr<ActionCallback> cb) override;

  void addUpdateAction(ActionCallback *cb) override;
  void removeUpdateAction(ActionCallback *cb) override;

private:
  static void performDelayedActions(EvLoop *, ev_check *, int);
  static void performTimerActions(EvLoop *, ev_timer *, int);

  void wakeup();

private:
  EvLoop *loop;
  ev_async wakeupEvent;
  ev_check delayedActionsWatcher;
  ev_timer timerWatcher;

  ActionsList timerActions;

  IOSubsMap readSubscribers;
  IOSubsMap writeSubscribers;
  ChildSubsMap childProcSubscribers;

  DelayedActions delayedActions;

  eastl::unique_ptr<ActionCallback> onStoppedCb;
  bool useDefaultLoop = false;
};

IOEventsPollLoopImpl::IOEventsPollLoopImpl(int tick_ms, bool use_default_loop)
{
  useDefaultLoop = use_default_loop;
  if (useDefaultLoop)
    loop = get_default_loop();
  else
    loop = ev_loop_new(0);
  ev_set_userdata(loop, this);
  ev_async_init(&wakeupEvent, dummy_cb); //-V1027
  ev_async_start(loop, &wakeupEvent);

  ev_check_init(&delayedActionsWatcher, &IOEventsPollLoopImpl::performDelayedActions); //-V1027
  ev_check_start(loop, &delayedActionsWatcher);

  double timeIntervalS = double(tick_ms) * 0.001;
  ev_timer_init(&timerWatcher, &IOEventsPollLoopImpl::performTimerActions, 0, double(tick_ms) * 0.001); //-V1027
  ev_timer_start(loop, &timerWatcher);
}

IOEventsPollLoopImpl::~IOEventsPollLoopImpl() { G_ASSERT(loop == nullptr); }

void IOEventsPollLoopImpl::run()
{
  debug("[ioevents] IOEventsPollLoopImpl::%s", __FUNCTION__);
  ev_run(loop, 0);

  // cleanup
  delayedActions.perform();
  ev_async_stop(loop, &wakeupEvent);
  unsubscribe_io_all(loop, readSubscribers, EventFlag::READ);
  unsubscribe_io_all(loop, writeSubscribers, EventFlag::WRITE);
  unsubscribe_child_all(loop, childProcSubscribers);
  ev_check_stop(loop, &delayedActionsWatcher);
  ev_timer_stop(loop, &timerWatcher);

  if (!useDefaultLoop)
    ev_loop_destroy(loop);
  loop = nullptr;
  debug("[ioevents] IOEventsPollLoopImpl::%s finished", __FUNCTION__);

  if (onStoppedCb)
  {
    auto cb = eastl::move(onStoppedCb);
    cb->perform();
  }
}

void IOEventsPollLoopImpl::stop(eastl::unique_ptr<ActionCallback> on_stopped)
{
  debug("[ioevents] IOEventsPollLoopImpl::%s", __FUNCTION__);
  addDelayedAction([this, on_stopped = eastl::move(on_stopped)]() mutable {
    onStoppedCb = eastl::move(on_stopped);
    ev_break(loop, EVBREAK_ALL);
  });
}

void IOEventsPollLoopImpl::stopSync()
{
  debug("[ioevents] IOEventsPollLoopImpl::%s", __FUNCTION__);
  // TODO: assert that current thread is not loop thread
  os_event_t event;
  os_event_create(&event);

  os_event_t *pevent = &event;
  stop(make_delayed_action([pevent]() mutable { os_event_set(pevent); }));

  os_event_wait(&event, OS_WAIT_INFINITE);
  os_event_destroy(&event);
}

void IOEventsPollLoopImpl::wakeup() { ev_async_send(loop, &wakeupEvent); }

void IOEventsPollLoopImpl::performDelayedActions(EvLoop *loop, ev_check *, int)
{
  auto self = static_cast<IOEventsPollLoopImpl *>(ev_userdata(loop));
  self->delayedActions.perform();
}

void IOEventsPollLoopImpl::performTimerActions(EvLoop *loop, ev_timer *, int)
{
  auto self = static_cast<IOEventsPollLoopImpl *>(ev_userdata(loop));
  for (ActionCallback *cb : self->timerActions)
    cb->perform();
}

void IOEventsPollLoopImpl::addDelayedAction(eastl::unique_ptr<ActionCallback> cb)
{
  delayedActions.add(eastl::move(cb));
  wakeup();
}

void IOEventsPollLoopImpl::subscribe(FileDesc fd, EventFlags events, IOCallbackPtr cb)
{
  addDelayedAction([this, fd, events, cb] {
    if (events & EventFlag::READ)
      subscribe_io(loop, readSubscribers, fd, EventFlag::READ, cb);
    if (events & EventFlag::WRITE)
      subscribe_io(loop, writeSubscribers, fd, EventFlag::WRITE, cb);
    if (events & EventFlag::CHILD_EXIT)
    {
      G_ASSERTF(useDefaultLoop, "child events are allowed only in default poll");
      subscribe_child(loop, childProcSubscribers, fd, cb);
    }
  });
}

void IOEventsPollLoopImpl::unsubscribe(FileDesc fd, EventFlags events)
{
  addDelayedAction([this, fd, events] {
    if (events & EventFlag::READ)
      unsubscribe_io(loop, readSubscribers, fd, EventFlag::READ);
    if (events & EventFlag::WRITE)
      unsubscribe_io(loop, writeSubscribers, fd, EventFlag::WRITE);
    if (events & EventFlag::CHILD_EXIT)
      unsubscribe_child(loop, childProcSubscribers, fd);
  });
}

void IOEventsPollLoopImpl::asyncCloseSocket(FileDesc fd)
{
  addDelayedAction([this, fd] {
    unsubscribe_io(loop, readSubscribers, fd, EventFlag::READ);
    unsubscribe_io(loop, writeSubscribers, fd, EventFlag::WRITE);
    os_socket_close(fd);
  });
}

void IOEventsPollLoopImpl::addUpdateAction(ActionCallback *cb)
{
  addDelayedAction([this, cb] {
    G_ASSERT(eastl::find(timerActions.begin(), timerActions.end(), cb) == timerActions.end());
    timerActions.push_back(cb);
  });
}

void IOEventsPollLoopImpl::removeUpdateAction(ActionCallback *cb)
{
  addDelayedAction(
    [this, cb] { timerActions.erase(eastl::remove(timerActions.begin(), timerActions.end(), cb), timerActions.end()); });
}

eastl::unique_ptr<IOEventsPollLoop> create_loop(int tick_ms) { return eastl::make_unique<IOEventsPollLoopImpl>(tick_ms, false); }

eastl::unique_ptr<IOEventsPoll> create_poll() { return eastl::make_unique<IOEventsPollImpl>(false); }

static volatile int default_loop_used = 0;

class IOEventsDefaultPoll : public IOEventsPollImpl
{
public:
  IOEventsDefaultPoll() : IOEventsPollImpl(true) {}

  ~IOEventsDefaultPoll() { default_loop_used = 0; }
};

class IOEventsDefaultPollLoop : public IOEventsPollLoopImpl
{
public:
  IOEventsDefaultPollLoop(int ticks_ms) : IOEventsPollLoopImpl(ticks_ms, true) {}

  ~IOEventsDefaultPollLoop() { default_loop_used = 0; }
};

eastl::unique_ptr<IOEventsPoll> get_ev_default_poll()
{
  if (interlocked_compare_exchange(default_loop_used, 1, 0))
  {
    G_ASSERTF(0, "default poll already used. only one instance allowed");
    return eastl::unique_ptr<IOEventsPoll>();
  }

  return eastl::make_unique<IOEventsDefaultPoll>();
}

eastl::unique_ptr<IOEventsPollLoop> get_ev_default_loop(int ticks_ms)
{
  if (interlocked_compare_exchange(default_loop_used, 1, 0))
  {
    G_ASSERTF(0, "default poll already used. only one instance allowed");
    return eastl::unique_ptr<IOEventsPollLoop>();
  }
  return eastl::make_unique<IOEventsDefaultPollLoop>(ticks_ms);
}

} // namespace ioevents
