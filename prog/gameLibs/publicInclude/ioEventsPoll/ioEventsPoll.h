//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_bitFlagsMask.h>
#include <EASTL/shared_ptr.h>

namespace ioevents
{
enum class EventFlag
{
  READ = 1,
  WRITE = 2,
  CHILD_EXIT = 4
};

using EventFlags = BitFlagsMask<EventFlag>;
BITMASK_DECLARE_FLAGS_OPERATORS(EventFlag);

using FileDesc = int;

struct EventData
{};

struct ChildEventData : EventData
{
  int rstatus; // exit code of child process
};

struct IOCallback
{
  virtual ~IOCallback() {}

  virtual void onIOEvents(FileDesc fd, EventFlag event, EventData const &event_data) = 0;
  virtual void onUnsubscribed(FileDesc /*fd*/, EventFlag /*event*/) {}
};

using IOCallbackPtr = eastl::shared_ptr<IOCallback>;

template <typename OnEvents, typename OnUnsubscribed>
IOCallbackPtr make_io_callback(OnEvents &&on_events, OnUnsubscribed &&on_unsubscribed)
{
  struct Callback : IOCallback
  {
    OnEvents onEvents;
    OnUnsubscribed onUnsubscribedCb;

    Callback(OnEvents &&on_events, OnUnsubscribed &&on_unsubscribed) :
      onEvents(eastl::forward<OnEvents>(on_events)), onUnsubscribedCb(eastl::forward<OnUnsubscribed>(on_unsubscribed))
    {}

    void onIOEvents(FileDesc fd, EventFlag event, EventData const &event_data) override { onEvents(fd, event, event_data); }

    void onUnsubscribed(FileDesc fd, EventFlag event) override { onUnsubscribedCb(fd, event); }
  };
  return eastl::make_shared<Callback>(eastl::forward<OnEvents>(on_events), eastl::forward<OnUnsubscribed>(on_unsubscribed));
}

template <typename OnEvents>
IOCallbackPtr make_io_callback(OnEvents &&on_events)
{
  struct Callback : IOCallback
  {
    OnEvents onEvents;

    Callback(OnEvents &&on_events) : onEvents(eastl::forward<OnEvents>(on_events)) {}

    void onIOEvents(FileDesc fd, EventFlag event, EventData const &event_data) override { onEvents(fd, event, event_data); }
  };
  return eastl::make_shared<Callback>(eastl::forward<OnEvents>(on_events));
}

struct ActionCallback
{
  virtual ~ActionCallback() {}
  virtual void perform() = 0;
};

template <class F>
eastl::unique_ptr<ActionCallback> make_delayed_action(F &&func)
{
  class DelayedAction : public ActionCallback
  {
  public:
    DelayedAction(F &&func) : f(eastl::forward<F>(func)) {}
    virtual void perform() { f(); }

  private:
    F f;
  };

  return eastl::make_unique<DelayedAction>(eastl::forward<F>(func));
}

class IOEventsDispatcher
{
public:
  virtual ~IOEventsDispatcher() {}

  virtual void subscribe(FileDesc fd, EventFlags events, IOCallbackPtr cb) = 0;
  virtual void unsubscribe(FileDesc fd, EventFlags events) = 0;

  // close socked after it had been unsubscribed
  virtual void asyncCloseSocket(FileDesc fd) = 0;

  // action will be performed once right after polling
  virtual void addDelayedAction(eastl::unique_ptr<ActionCallback> cb) = 0;

  template <class Func>
  void addDelayedAction(Func &&func)
  {
    addDelayedAction(make_delayed_action(eastl::forward<Func>(func)));
  }

  // invoke actions every tick. non-owning pointer
  virtual void addUpdateAction(ActionCallback *cb) = 0;
  virtual void removeUpdateAction(ActionCallback *cb) = 0;
};

class IOEventsPoll : public IOEventsDispatcher
{
public:
  // non-blocking polling
  virtual void poll() = 0;
};

class IOEventsPollLoop : public IOEventsDispatcher
{
public:
  // block until stop called
  virtual void run() = 0;
  virtual void stop(eastl::unique_ptr<ActionCallback> on_stopped) = 0;

  // stop and wait until thread stopped
  virtual void stopSync() = 0;
};

eastl::unique_ptr<IOEventsPollLoop> create_loop(int tick_ms);
eastl::unique_ptr<IOEventsPoll> create_poll();

// libev default_loop is the only loop that can track process childs
// there is only one instance of this kind of poll allowed
eastl::unique_ptr<IOEventsPollLoop> get_ev_default_loop(int ticks_ms);
eastl::unique_ptr<IOEventsPoll> get_ev_default_poll();
} // namespace ioevents
