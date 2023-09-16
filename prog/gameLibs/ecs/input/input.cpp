#include <ecs/input/input.h>
#include <ecs/input/message.h>
#include <daECS/core/coreEvents.h>
#include <workCycle/dag_startupModules.h>
#include <startup/dag_inpDevClsDrv.h>
#include <startup/dag_restart.h>
#include <humanInput/dag_hiKeyboard.h>
#include <humanInput/dag_hiPointing.h>
#include <humanInput/dag_hiJoystick.h>
#include <humanInput/dag_hiComposite.h>
#include <humanInput/dag_hiDInput.h>
#include <humanInput/dag_hiCreate.h>
#include <daInput/input_api.h>
#include <perfMon/dag_cpuFreq.h>
#include <ioSys/dag_dataBlock.h>
#include "esHidEventRouter.h"
#include <util/dag_delayedAction.h>
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_atomic.h>


#define DAINPUT_ECS_EVENT ECS_REGISTER_EVENT
DAINPUT_ECS_EVENTS
#undef DAINPUT_ECS_EVENT

#if _TARGET_XBOX
extern void enable_xbox_hw_mouse(bool);
#endif

namespace ecs
{
class DaInputMgr;
}

class ecs::DaInputMgr : public ecs::IGenHidEventHandler
{
public:
  DaInputMgr()
  {
    if (HumanInput::IGenPointing *mouse = global_cls_drv_pnt ? global_cls_drv_pnt->getDevice(0) : nullptr)
      mouse->setRelativeMovementMode(true); //== required for correct handling mouse DZ events
    register_hid_event_handler(this, 100);
  }
  ~DaInputMgr() { unregister_hid_event_handler(this); }

  virtual bool ehIsActive() const override { return true; }
  virtual bool gmehMove(const Context &, float dx, float dy) override
  {
    dainput::mouse_move_event_occurs(ref_time_delta_to_usec(ref_time_ticks()), dx, dy, HumanInput::raw_state_pnt.mouse.x,
      HumanInput::raw_state_pnt.mouse.y);
    return false;
  }
  virtual bool gmehButtonDown(const Context &, int btn_idx) override
  {
    if (g_entity_mgr)
      g_entity_mgr->broadcastEvent(
        EventHidGlobalInputSink(get_time_msec(), dainput::DEV_pointing, true, (dainput::DEV_pointing << 8) | btn_idx));
    dainput::mouse_btn_event_occurs(ref_time_delta_to_usec(ref_time_ticks()), btn_idx, true);
    return false;
  }
  virtual bool gmehButtonUp(const Context &, int btn_idx) override
  {
    if (g_entity_mgr)
      g_entity_mgr->broadcastEvent(
        EventHidGlobalInputSink(get_time_msec(), dainput::DEV_pointing, false, (dainput::DEV_pointing << 8) | btn_idx));
    dainput::mouse_btn_event_occurs(ref_time_delta_to_usec(ref_time_ticks()), btn_idx, false);
    return false;
  }
  virtual bool gmehWheel(const Context &, int scroll) override
  {
    dainput::mouse_wheel_event_occurs(ref_time_delta_to_usec(ref_time_ticks()), scroll);
    return false;
  }
  virtual bool gtehTouchBegan(const Context &, HumanInput::IGenPointing *, int, const HumanInput::PointingRawState::Touch &) override
  {
    dainput::touch_event_occurs(ref_time_delta_to_usec(ref_time_ticks()));
    return false;
  }
  virtual bool gtehTouchMoved(const Context &, HumanInput::IGenPointing *, int, const HumanInput::PointingRawState::Touch &) override
  {
    dainput::touch_event_occurs(ref_time_delta_to_usec(ref_time_ticks()));
    return false;
  }

  virtual bool gtehTouchEnded(const Context &, HumanInput::IGenPointing *, int, const HumanInput::PointingRawState::Touch &) override
  {
    dainput::touch_event_occurs(ref_time_delta_to_usec(ref_time_ticks()));
    return false;
  }
  virtual bool gkehButtonDown(const Context &, int btn_idx, bool repeat, wchar_t) override
  {
    if (g_entity_mgr && btn_idx && !repeat)
      g_entity_mgr->broadcastEvent(
        EventHidGlobalInputSink(get_time_msec(), dainput::DEV_kbd, true, (dainput::DEV_kbd << 8) | btn_idx));
    if (btn_idx && !repeat)
      dainput::kbd_btn_event_occurs(ref_time_delta_to_usec(ref_time_ticks()), btn_idx, true);
    return false;
  }
  virtual bool gkehButtonUp(const Context &, int btn_idx) override
  {
    if (g_entity_mgr && btn_idx)
      g_entity_mgr->broadcastEvent(
        EventHidGlobalInputSink(get_time_msec(), dainput::DEV_kbd, false, (dainput::DEV_kbd << 8) | btn_idx));
    if (btn_idx)
      dainput::kbd_btn_event_occurs(ref_time_delta_to_usec(ref_time_ticks()), btn_idx, false);
    return false;
  }

#if _TARGET_64BIT
  static void bcast_global_input_sink_delayed_cb(void *arg)
  {
    int t0 = int(uintptr_t(arg));
    int i = (uintptr_t(arg) >> 32) & ~(1 << 31);
    bool pressed = (uintptr_t(arg) >> 63) != 0;
    g_entity_mgr->broadcastEvent(EventHidGlobalInputSink(t0, dainput::DEV_gamepad, pressed, (dainput::DEV_gamepad << 8) | i));
  }
#endif

  virtual bool gjehStateChanged(const Context & /*ctx*/, HumanInput::IGenJoystick *joy, int /*joy_ord_id*/) override
  {
    auto bcast_global_input_sink_event = [&](int i, bool pressed) {
      int t0 = get_time_msec();
#if _TARGET_64BIT // Use delayed callback to avoid allocating heap for short-lived delayed action instance
      void *arg = (void *)(uintptr_t(t0) | (uintptr_t(i) << 32) | (pressed ? (1LL << 63) : 0));
      add_delayed_callback(bcast_global_input_sink_delayed_cb, arg);
#else
      ::delayed_call([i, t0, pressed]() {
        g_entity_mgr->broadcastEvent(EventHidGlobalInputSink(t0, dainput::DEV_gamepad, pressed, (dainput::DEV_gamepad << 8) | i));
      });
#endif
    };
    if (g_entity_mgr && global_cls_drv_joy && global_cls_drv_joy->getDefaultJoystick() == joy &&
        (joy->getRawState().buttons.getWord0() | joy->getRawState().buttonsPrev.getWord0()))
    {
      const HumanInput::JoystickRawState &state = joy->getRawState();
      for (int i = 0, inc = 0; i < state.MAX_BUTTONS && i < 32; i += inc)
        if (state.buttons.getIter(i, inc))
          if (!state.buttonsPrev.get(i))
            bcast_global_input_sink_event(i, true);
      for (int i = 0, inc = 0; i < state.MAX_BUTTONS && i < 32; i += inc)
        if (state.buttonsPrev.getIter(i, inc))
          if (!state.buttons.get(i))
            bcast_global_input_sink_event(i, false);
    }

    if (global_cls_drv_joy && global_cls_drv_joy->getDefaultJoystick() == joy)
      dainput::gpad_change_event_occurs(ref_time_delta_to_usec(ref_time_ticks()), joy);
    return false;
  }
};

static InitOnDemand<ecs::DaInputMgr> dainput_mgr;
class DaInputGamepadPollThread final : public DaThread
{
  int stepMsec = 0;
  void execute() override
  {
    while (!interlocked_acquire_load(this->terminating))
    {
      if (dainput::is_safe_to_sample_input())
      {
        global_cls_drv_update_cs.lock();
        if (::global_cls_drv_joy)
          ::global_cls_drv_joy->updateDevices();
        dainput::tick_event_occurs(ref_time_delta_to_usec(ref_time_ticks()));
        global_cls_drv_update_cs.unlock();
      }
      sleep_msec(stepMsec);
    }
  }

public:
  DaInputGamepadPollThread(int step) : DaThread("dainput::poll"), stepMsec(step) {}
};

static eastl::unique_ptr<DaInputGamepadPollThread> poll_thread = nullptr;

static void start_poll_thread(int poll_thread_interval_msec)
{
  if (!poll_thread && poll_thread_interval_msec > 0)
  {
    poll_thread.reset(new DaInputGamepadPollThread(poll_thread_interval_msec));
    poll_thread->start();
    poll_thread->setAffinity(WORKER_THREADS_AFFINITY_MASK);
  }
}

static void stop_poll_thread()
{
  if (poll_thread)
  {
    poll_thread->terminate(true, -1);
    poll_thread.reset();
  }
}

DaInputComponent::DaInputComponent(const ecs::EntityManager &, ecs::EntityId) { dainput_mgr.demandInit(); }

ECS_REGISTER_RELOCATABLE_TYPE(DaInputComponent, nullptr);

ECS_AUTO_REGISTER_COMPONENT(DaInputComponent, "input", nullptr, 0);
ECS_DEF_PULL_VAR(input);

static void on_action_triggered(dainput::action_handle_t action, bool term, int dur_ms)
{
  if (!term)
    g_entity_mgr->broadcastEventImmediate(EventDaInputActionTriggered(action, dur_ms));
  else
    g_entity_mgr->broadcastEventImmediate(EventDaInputActionTerminated(action, dur_ms));
}

void ecs::init_hid_drivers(int poll_thread_interval_msec)
{
#if _TARGET_PC | _TARGET_IOS | _TARGET_ANDROID | _TARGET_C3
  ::dagor_init_mouse_win();
  ::dagor_init_keyboard_win();
#elif _TARGET_XBOX
  ::dagor_init_keyboard_win();
  if (!::global_cls_drv_pnt)
    ::global_cls_drv_pnt = HumanInput::createMouseEmuClassDriver();
  enable_xbox_hw_mouse(true);
#elif _TARGET_C1 | _TARGET_C2




#else
  ::dagor_init_keyboard_null();
  ::dagor_init_mouse_null();
#endif


#if !(_TARGET_PC | _TARGET_IOS | _TARGET_ANDROID | _TARGET_C1 | _TARGET_C2 | _TARGET_XBOX | _TARGET_C3)
  ::dagor_init_joystick();
  start_poll_thread(poll_thread_interval_msec);
  return;
#endif

#if _TARGET_PC_WIN
  HumanInput::startupDInput();
  ::startup_game(RESTART_ALL);
#else
  ::startup_game(RESTART_INPUT);
#endif

#if _TARGET_C1 | _TARGET_C2

#elif !_TARGET_TVOS
  ::global_cls_composite_drv_joy = HumanInput::CompositeJoystickClassDriver::create();
#endif

#if !_TARGET_TVOS
  if (!::global_cls_composite_drv_joy)
    ::global_cls_drv_joy = HumanInput::createJoystickClassDriver(/*exclude_xinput*/ true, /*remap_360*/ false);
  else
  {
#if _TARGET_PC_WIN
    ::global_cls_composite_drv_joy->addClassDrv(::HumanInput::createXinputJoystickClassDriver(), /*is_xinput*/ true);
#endif
#if _TARGET_XBOX
    ::global_cls_composite_drv_joy->addClassDrv(::HumanInput::createXinputJoystickClassDriver(true), /*is_xinput*/ true);
#endif
#if _TARGET_PC_LINUX
    ::global_cls_composite_drv_joy->addClassDrv(::HumanInput::createJoystickClassDriver(/*exclude_xinput*/ false, /*remap_360*/ true),
      /*is_xinput*/ true);
#endif
#if _TARGET_PC | _TARGET_ANDROID | _TARGET_IOS
    ::global_cls_composite_drv_joy->addClassDrv(::HumanInput::createJoystickClassDriver(/*exclude_xinput*/ true, /*remap_360*/ false),
      /*is_xinput*/ false);
#endif
    ::global_cls_drv_joy = ::global_cls_composite_drv_joy;
  }
#endif
  if (::global_cls_drv_joy)
    for (int i = 0; i < ::global_cls_drv_joy->getDeviceCount(); i++)
      if (::global_cls_drv_joy->getDevice(i))
      {
        ::global_cls_drv_joy->getDevice(i)->enableSensorsTiltCorrection(true);
        ::global_cls_drv_joy->getDevice(i)->reportGyroSensorsAngDelta(true);
      }

  start_poll_thread(poll_thread_interval_msec);
}
void ecs::term_hid_drivers()
{
  stop_poll_thread();
  if (::global_cls_composite_drv_joy)
  {
    ::global_cls_composite_drv_joy->destroy();
    if (::global_cls_drv_joy == ::global_cls_composite_drv_joy)
      ::global_cls_drv_joy = NULL;
    ::global_cls_composite_drv_joy = NULL;
  }
  if (::global_cls_drv_joy)
  {
    ::global_cls_drv_joy->destroy();
    ::global_cls_drv_joy = NULL;
  }
}

void ecs::init_input(const char *controls_config_fname) { ecs::init_input(DataBlock(controls_config_fname)); }
void ecs::init_input(const DataBlock &controls_config_blk)
{
  dainput::register_dev1_kbd(global_cls_drv_kbd->getDevice(0));
  dainput::register_dev2_pnt(global_cls_drv_pnt->getDevice(0));
  dainput::register_dev3_gpad(global_cls_drv_joy ? global_cls_drv_joy->getDevice(0) : nullptr);
  dainput::init_actions(controls_config_blk);
  dainput::dump_action_sets();
  dainput::set_event_receiver(&on_action_triggered);
  g_entity_mgr->broadcastEventImmediate(EventDaInputInit(true));
}
void ecs::term_input()
{
  g_entity_mgr->broadcastEventImmediate(EventDaInputInit(false));
  dainput::reset_devices();
  dainput::reset_actions();
}
