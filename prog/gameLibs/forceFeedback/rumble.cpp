#include <osApiWrappers/dag_events.h>
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_wndProcComponent.h>
#include <osApiWrappers/dag_wndProcCompMsg.h>
#include <osApiWrappers/dag_atomic.h>
#include <perfMon/dag_cpuFreq.h>

#include <forceFeedback/forceFeedback.h>
#include <startup/dag_globalSettings.h>
#include <startup/dag_inpDevClsDrv.h>
#include <humanInput/dag_hiJoystick.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <generic/dag_tab.h>

namespace force_feedback
{

namespace rumble
{

static HumanInput::IGenJoystick *get_joystick()
{
  if (::global_cls_drv_joy)
    return ::global_cls_drv_joy->getDefaultJoystick();
  return nullptr;
}

static WinCritSec rumble_cs;
static os_event_t rumble_wakeup_event;
static bool is_inited = false;

enum ControlCommand : uint8_t
{
  CMD_NONE,
  CMD_EVENT,
  CMD_RESET,
  CMD_APP_ACTIVATE,
  CMD_APP_DEACTIVATE
};

struct EventState
{
  ControlCommand cmd;
  EventParams params;
};

static Tab<EventState> incoming_events, processing_events;

static void set_rumble(float lo_freq, float hi_freq)
{
  WinAutoLock joyGuard(global_cls_drv_update_cs);
  HumanInput::IGenJoystick *joy = get_joystick();
  if (joy)
    joy->doRumble(lo_freq, hi_freq);
}


class RumbleThread final : public DaThread, public IWndProcComponent
{
public:
  RumbleThread() : DaThread("RumbleThread") {}

  void execute() override
  {
    bool rumbleIsActive = false;

    while (!interlocked_acquire_load(terminating))
    {
      os_event_wait(&rumble_wakeup_event, rumbleIsActive ? 5 : 1000);

      int t = ::get_time_msec();
      {
        WinAutoLock lock(rumble_cs);
        if (incoming_events.empty())
          ;
        else if (processing_events.empty())
          processing_events.swap(incoming_events);
        else // append
          processing_events.insert(processing_events.end(), incoming_events.begin(), incoming_events.end());
        incoming_events.clear();
      }

      if (interlocked_acquire_load(terminating))
        break;

      bool clearProcessing = false;
      for (auto &evt : processing_events) // First pass - process non event commands
      {
        switch (evt.cmd)
        {
          case CMD_NONE: continue; // already processed
          case CMD_EVENT:
          {
            if (evt.params.startTimeMsec == 0) // Not inited
              evt.params.startTimeMsec = t;
            continue;
          }
          break;
          case CMD_RESET: clearProcessing = true; break;
          case CMD_APP_ACTIVATE: isAppActive = true; break;
          case CMD_APP_DEACTIVATE:
            isAppActive = false;
            clearProcessing = true;
            break;
        }
        if (!clearProcessing)
          evt.cmd = CMD_NONE;
      }

      // Second pass - process only event commands
      float loFreq = 0, hiFreq = 0;
      if (clearProcessing)
        processing_events.clear();
      else if (isAppActive)
      {
        for (int iEvt = processing_events.size() - 1; iEvt >= 0; --iEvt)
        {
          bool eraseIt = true;
          EventState &evt = processing_events[iEvt];
          if (evt.cmd == CMD_EVENT)
          {
            if (evt.params.band == EventParams::LO)
              loFreq = ::max(loFreq, evt.params.freq);
            else
              hiFreq = ::max(hiFreq, evt.params.freq);

            if (t < evt.params.startTimeMsec + evt.params.durationMsec)
              eraseIt = false;
          }
          if (eraseIt)
            erase_items(processing_events, iEvt, 1);
        }
      }
      set_rumble(loFreq, hiFreq);
      rumbleIsActive = (loFreq > 0 || hiFreq > 0);
    }

    set_rumble(0, 0);
  }

  virtual RetCode process(void * /*hwnd*/, unsigned msg, uintptr_t wParam, intptr_t /*lParam*/, intptr_t & /*result*/) override
  {
    if (msg == GPCM_Activate)
    {
      // stop vibration early
      if (wParam == GPCMP1_Inactivate)
        set_rumble(0, 0);

      // notify thread
      {
        WinAutoLock cs(rumble_cs);
        incoming_events.push_back({wParam != GPCMP1_Inactivate ? CMD_APP_ACTIVATE : CMD_APP_DEACTIVATE, {}});
      }
      os_event_set(&rumble_wakeup_event);
    }
    return PROCEED_OTHER_COMPONENTS;
  }

public:
  bool isAppActive = true;
};

void add_event(const EventParams &evtp)
{
  {
    WinAutoLock cs(rumble_cs);
    incoming_events.push_back({CMD_EVENT, evtp});
  }

  if (EASTL_UNLIKELY(!is_inited)) // init on first usage
    init();

  os_event_set(&rumble_wakeup_event);
}

static RumbleThread rumble_thread;


void init()
{
  G_ASSERT_RETURN(!is_inited, );

  rumble_thread.isAppActive = ::dgs_app_active;
  add_wnd_proc_component(&rumble_thread);
  os_event_create(&rumble_wakeup_event);
  incoming_events.reserve(64);
  processing_events.reserve(64);
  rumble_thread.start();
  rumble_thread.setAffinity(WORKER_THREADS_AFFINITY_MASK);
  is_inited = true;
}


void shutdown()
{
  if (!is_inited)
    return;

  reset();

  rumble_thread.terminate(true, 1000, &rumble_wakeup_event);
  del_wnd_proc_component(&rumble_thread);
  os_event_destroy(&rumble_wakeup_event);

  is_inited = false;
}

void reset()
{
  if (!is_inited)
    return;

  {
    WinAutoLock cs(rumble_cs);
    incoming_events.push_back({CMD_RESET, {}});
  }

  os_event_set(&rumble_wakeup_event);
}


} // namespace rumble

} // end namespace force_feedback
