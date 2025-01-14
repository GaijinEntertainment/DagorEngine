// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.

#pragma once

#include <debug/dag_hwExcept.h>
#include <memory/dag_mem.h>
#include <perfMon/dag_cpuFreq.h>
#include <util/dag_string.h>
#include <util/dag_simpleString.h>
#include <util/dag_globDef.h>
#include <util/dag_delayedAction.h>
#include <osApiWrappers/dag_dbgStr.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/dag_threads.h>
#include <debug/dag_debug.h>
#include <unistd.h>
#include "dag_addBasePathDef.h"
#include "dag_loadSettings.h"
#include <workCycle/dag_workCycle.h>
#include <supp/dag_android_native_app_glue.h>
#include <osApiWrappers/dag_wndProcCompMsg.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_texMgr.h>
#include <startup/dag_globalSettings.h>
#include <drv/hid/dag_hiCreate.h>
#include <ioSys/dag_dataBlock.h>
#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_statDrv.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <statsd/statsd.h>
#include <EASTL/bitset.h>
#include <EASTL/fixed_vector.h>
#include <atomic>
#include <mutex>

const size_t ANDROID_ROTATED_LOGS = 3;

#define AWINDOW_FLAG_KEEP_SCREEN_ON 0x00000080

void DagorWinMainInit(int nCmdShow, bool debugmode);
int DagorWinMain(int nCmdShow, bool debugmode);

JavaVM *g_jvm = nullptr;

extern void default_crt_init_kernel_lib();
extern void default_crt_init_core_lib();
extern void apply_hinstance(void *hInstance, void *hPrevInstance);
extern void win32_set_instance(void *hinst);
extern void win32_set_main_wnd(void *hwnd);
extern void (*android_exit_app_ptr)(int code);
extern "C" void android_native_app_destroy(android_app *app);
extern "C" const char *dagor_exe_build_date;
extern "C" const char *dagor_exe_build_time;

extern void messagebox_report_fatal_error(const char *title, const char *msg, const char *call_stack);
extern void android_d3d_reinit(void *w);
extern void android_update_window_size(void *w);
extern void android_init_soft_input(android_app *state);

namespace workcycle_internal
{
extern intptr_t main_window_proc(void *, unsigned, uintptr_t, intptr_t);
} // namespace workcycle_internal

const char *dagor_android_internal_path = NULL;
const char *dagor_android_external_path = NULL;
const DataBlock *dagor_android_preload_settings_blk = &DataBlock::emptyBlock;

SimpleString dagor_android_self_pkg_name, dagor_android_self_pkg_ver_name;
int dagor_android_self_pkg_ver_code = 0;

float dagor_android_scr_xdpi = 1;
float dagor_android_scr_ydpi = 1;
int dagor_android_scr_xres = 0;
int dagor_android_scr_yres = 0;

void (*dagor_android_user_message_loop_handler)(struct android_app *app, int32_t cmd) = nullptr;
SimpleString (*dagor_android_app_get_anr_execution_context)() = nullptr;

DataBlock dagor_anr_handler_settings;
void (*dagor_anr_handler)(int32_t cmd) = nullptr;
bool (*dagor_should_ignore_android_cmd)(int32_t cmd) = nullptr;
bool (*dagor_allow_to_delay_android_cmd)(int32_t cmd) = nullptr;

bool dagor_fast_shutdown = false;

ALooper *dagor_command_thread_looper = nullptr;

namespace native_android
{
extern int32_t window_width;
extern int32_t window_height;
extern int32_t buffer_width;
extern int32_t buffer_height;
} // namespace native_android

static int get_unicode_char(struct android_app *app, int eventType, int keyCode, int metaState)
{
  JavaVM *javaVM = app->activity->vm;
  JNIEnv *jniEnv = app->activity->env;

  JavaVMAttachArgs attachArgs;
  attachArgs.version = JNI_VERSION_1_6;
  attachArgs.name = "NativeThread";
  attachArgs.group = NULL;

  jint result = javaVM->AttachCurrentThread(&jniEnv, &attachArgs);
  if (result == JNI_ERR)
    return 0;

  jclass class_key_event = jniEnv->FindClass("android/view/KeyEvent");
  int unicodeKey;

  if (metaState == 0)
  {
    jmethodID method_get_unicode_char = jniEnv->GetMethodID(class_key_event, "getUnicodeChar", "()I");
    jmethodID eventConstructor = jniEnv->GetMethodID(class_key_event, "<init>", "(II)V");
    jobject eventObj = jniEnv->NewObject(class_key_event, eventConstructor, eventType, keyCode);
    unicodeKey = jniEnv->CallIntMethod(eventObj, method_get_unicode_char);
  }
  else
  {
    jmethodID method_get_unicode_char = jniEnv->GetMethodID(class_key_event, "getUnicodeChar", "(I)I");
    jmethodID eventConstructor = jniEnv->GetMethodID(class_key_event, "<init>", "(II)V");
    jobject eventObj = jniEnv->NewObject(class_key_event, eventConstructor, eventType, keyCode);

    unicodeKey = jniEnv->CallIntMethod(eventObj, method_get_unicode_char, metaState);
  }

  javaVM->DetachCurrentThread();
  return unicodeKey;
}

static int32_t get_pointer_id(void *event, int pidx)
{
  // Dagor engine expect touch id start from 1, AMotionEvent_getPointerId for first touch return 0
  return android::motion_event::get_pointer_id(event, pidx) + 1;
}

static int get_pointer(void *event, int pidx)
{
  const int x = android::motion_event::get_x(event, pidx);
  const int y = android::motion_event::get_y(event, pidx);
  return x | (y << 16);
}


struct MainWindowProcCall
{
  uint32_t message;
  uintptr_t wParam;
  uintptr_t lParam;
};


struct AndroidProcessResult
{
  // In order to avoid allocations but still be able to handle more
  // use max touch points count.
  static const int MAX_TOUCH_POINTS = 16;

  eastl::fixed_vector<MainWindowProcCall, MAX_TOUCH_POINTS, true> wndCalls;
  int status = 0;
};

static std::mutex g_android_done_command_mutex;
static std::condition_variable g_android_done_command_condition;

static std::atomic<int32_t> g_android_pending_command(-1);

// Normally we don't have so many input events.
// It could only happen if case our DagorMainThread is very laggy
// In case of big lags do not buffer more than 128 events

static std::mutex g_android_input_mutex;
static int g_android_next_input_call_id = 0;
using InputCallsArray = eastl::array<MainWindowProcCall, 128>;
static InputCallsArray g_android_input_calls;

static std::mutex g_android_joystick_events_mutex;
static eastl::bitset<128> g_android_joystick_events_free;
static eastl::array<android::JoystickEvent, 128> g_android_joystick_events_storage;

static android::JoystickEvent *allocate_android_joystick_event(void *event)
{
  int eventId = -1;
  {
    std::unique_lock<std::mutex> lock(g_android_joystick_events_mutex);
    eventId = g_android_joystick_events_free.find_first();
    if (eventId < g_android_joystick_events_free.size())
      g_android_joystick_events_free.set(eventId, false);
    else
      return nullptr;
  }

  android::JoystickEvent *joystickEvent = &g_android_joystick_events_storage[eventId];

  joystickEvent->deviceId = android::motion_event::get_device_id(event);
  for (int i = 0, sz = joystickEvent->axisValues.size(); i < sz; ++i)
    joystickEvent->axisValues[i] = android::motion_event::get_axis_value(event, i, 0);

  return joystickEvent;
}


static void free_android_joystick_events(const InputCallsArray &input_calls, int input_calls_size)
{
  if (input_calls_size <= 0)
    return;

  std::unique_lock<std::mutex> lock(g_android_joystick_events_mutex);
  for (int i = 0; i < input_calls_size; ++i)
  {
    const MainWindowProcCall &call = input_calls[i];
    if (call.message == GPCM_JoystickMovement)
    {
      android::JoystickEvent *joystickEvent = (android::JoystickEvent *)call.wParam;
      const int eventId = eastl::distance(g_android_joystick_events_storage.begin(), joystickEvent);
      g_android_joystick_events_free.set(eventId, true);
    }
  }
}


static AndroidProcessResult process_android_key_event(struct android_app *app, void *event)
{
  AndroidProcessResult result;

  int key = android::key_event::get_key_code(event);
  int c = get_unicode_char(app, android::key_event::get_action(event) & AMOTION_EVENT_ACTION_MASK, key,
    android::key_event::get_meta_state(event));
  switch (android::key_event::get_action(event) & AMOTION_EVENT_ACTION_MASK)
  {
    case AKEY_EVENT_ACTION_DOWN:
      if (!c && key == AKEYCODE_DEL)
        c = 0x0008;
      if (key == AKEYCODE_ENTER)
        c = 0; // skip unicode 'Line Feed' character (we handle return ourselves)
      if (key == AKEYCODE_VOLUME_DOWN || key == AKEYCODE_VOLUME_UP)
        return result;

      if (key)
        result.wndCalls.push_back(MainWindowProcCall{GPCM_KeyPress, (uintptr_t)key, (uintptr_t)c});
      else if (c)
        result.wndCalls.push_back(MainWindowProcCall{GPCM_Char, (uintptr_t)c, 0});
      break;
    case AKEY_EVENT_ACTION_UP: result.wndCalls.push_back(MainWindowProcCall{GPCM_KeyRelease, (uintptr_t)key, 0}); break;
  }

  result.status = 1;

  return result;
}


static AndroidProcessResult process_android_motion_event(struct android_app *app, int event_source, void *event)
{
  AndroidProcessResult result;
  result.status = 1;

  static constexpr int AINPUT_SOURCE_JOYSTICK = 0x01000010;

  bool srcTouch = (event_source & AINPUT_SOURCE_TOUCHSCREEN) == AINPUT_SOURCE_TOUCHSCREEN;
  bool srcMouse = (event_source & AINPUT_SOURCE_MOUSE) == AINPUT_SOURCE_MOUSE;
  if (srcTouch || srcMouse)
  {
    int pidx =
      (android::motion_event::get_action(event) & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
    switch (android::motion_event::get_action(event) & AMOTION_EVENT_ACTION_MASK)
    {
      case AMOTION_EVENT_ACTION_MOVE:
        for (int i = 0, ie = android::motion_event::get_pointer_count(event); i < ie; i++)
          result.wndCalls.push_back(
            MainWindowProcCall{GPCM_TouchMoved, (uintptr_t)get_pointer_id(event, i), (uintptr_t)get_pointer(event, i)});
        break;
      case AMOTION_EVENT_ACTION_DOWN:
      case AMOTION_EVENT_ACTION_POINTER_DOWN:
        result.wndCalls.push_back(
          MainWindowProcCall{GPCM_TouchBegan, (uintptr_t)get_pointer_id(event, pidx), (uintptr_t)get_pointer(event, pidx)});
        break;
      case AMOTION_EVENT_ACTION_UP:
      case AMOTION_EVENT_ACTION_POINTER_UP:
        result.wndCalls.push_back(
          MainWindowProcCall{GPCM_TouchEnded, (uintptr_t)get_pointer_id(event, pidx), (uintptr_t)get_pointer(event, pidx)});
        break;
      case AMOTION_EVENT_ACTION_CANCEL:
        for (int i = 0, ie = android::motion_event::get_pointer_count(event); i < ie; i++)
          result.wndCalls.push_back(
            MainWindowProcCall{GPCM_TouchEnded, (uintptr_t)get_pointer_id(event, i), (uintptr_t)get_pointer(event, i)});
        break;
    }
  }
  else if (event_source == AINPUT_SOURCE_JOYSTICK)
  {
    if (android::JoystickEvent *joystickEvent = allocate_android_joystick_event(event))
      result.wndCalls.push_back(MainWindowProcCall{GPCM_JoystickMovement, uintptr_t(joystickEvent), 0});
  }

  return result;
}


static int32_t dagor_android_handle_input_thread(struct android_app *app, AInputEvent *event);


namespace
{

struct DagorMainThread final : public DaThread
{
  DagorMainThread() : DaThread("DagorMainThread", 4 << 20) {}

  void execute() override
  {
    DagorHwException::setHandler("main");

    default_crt_init_kernel_lib();

    if (dagor_android_external_path)
    {
      char path[DAGOR_MAX_PATH];
      strcpy(path, dagor_android_external_path);
      strcat(path, "/");
      dd_add_base_path(path);
      debug("addpath: %s", path);
    }

    static DagorSettingsBlkHolder stgBlkHolder;
    ::DagorWinMainInit(0, 0);

    default_crt_init_core_lib();
    while (!win32_get_main_wnd())
    {
      dagor_idle_cycle(false);
      debug("waiting for window to be inited...");
      sleep_msec(100);
    }
    ::DagorWinMain(0, 0);
  }
};


struct DagorInputThread final : public DaThread
{
  ALooper *looper = nullptr;
  AInputQueue *inputQueue = nullptr;


  DagorInputThread() : DaThread("DagorInputThread", 1 << 20) {}


  void attachLooper()
  {
    looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);

    if (inputQueue)
    {
      debug("CMD: Attach input queue to looper");
      AInputQueue_attachLooper(inputQueue, looper, LOOPER_ID_INPUT, nullptr, nullptr);
    }
  }


  void detachLooper()
  {
    if (inputQueue)
    {
      debug("CMD: Detach input queue from looper");
      AInputQueue_detachLooper(inputQueue);

      inputQueue = nullptr;
    }
  }


  void execute() override
  {
    attachLooper();

    while (!isThreadTerminating())
    {
      struct android_poll_source *source;
      int ident, events;
      while ((ident = ALooper_pollAll(500 /* timeout */, nullptr, &events, (void **)&source)) >= 0)
      {
        android_app *app = (android_app *)win32_get_instance();

        AInputEvent *event = NULL;
        while (AInputQueue_getEvent(inputQueue, &event) >= 0)
        {
          if (AInputQueue_preDispatchEvent(inputQueue, event))
            continue;

          int32_t handled = 0;
          if (app)
            handled = dagor_android_handle_input_thread(app, event);

          AInputQueue_finishEvent(inputQueue, event, handled);
        }
      }
    }

    detachLooper();
  }
};

} // namespace

static DagorMainThread g_dagor_main_thread;
static DagorInputThread g_dagor_input_thread;


static int push_android_input(const AndroidProcessResult &result)
{
  if (result.wndCalls.empty())
    return result.status;

  std::unique_lock<std::mutex> lock(g_android_input_mutex);
  if (g_android_next_input_call_id + result.wndCalls.size() < g_android_input_calls.size())
  {
    eastl::copy(result.wndCalls.begin(), result.wndCalls.end(), g_android_input_calls.begin() + g_android_next_input_call_id);
    g_android_next_input_call_id += result.wndCalls.size();
  }
  return result.status;
}


static void push_android_command_and_wait(int32_t cmd)
{
  const char *cmdName = android::app_command_to_string(cmd);

  int waitTimeMs = get_time_msec();

  g_android_pending_command.store(cmd, std::memory_order_release);

  {
    // After that time Android may generate ANR (Application Not Responsive)
    static const std::chrono::seconds ANR_TIMEOUT_SEC = std::chrono::seconds(5);

    std::unique_lock<std::mutex> lock(g_android_done_command_mutex);
    if (!g_android_done_command_condition.wait_for(lock, ANR_TIMEOUT_SEC,
          [] { return g_android_pending_command.load(std::memory_order_acquire) < 0; }))
    {
      logerr("android: ANR: Handling of %s is taking more than %d sec. Context: %s", cmdName, ANR_TIMEOUT_SEC.count(),
        dagor_android_app_get_anr_execution_context ? dagor_android_app_get_anr_execution_context() : "<unknown>");

      if (dagor_anr_handler)
        dagor_anr_handler(cmd);

      g_android_done_command_condition.wait(lock, [] { return g_android_pending_command.load(std::memory_order_acquire) < 0; });
    }
  }

  const int curTime = get_time_msec();
  waitTimeMs = curTime - waitTimeMs;

  debug("CMD: Done: %s (%d ms)", cmdName, waitTimeMs);
}


static void run_android_command_as_delayed_action(int32_t cmd);


static void dagor_android_handle_cmd_thread(struct android_app *app, int32_t cmd)
{
  const char *cmdName = android::app_command_to_string(cmd);
  debug("CMD: Received: %s", cmdName);

  if (dagor_allow_to_delay_android_cmd && dagor_allow_to_delay_android_cmd(cmd))
  {
    run_android_command_as_delayed_action(cmd);
    return;
  }

  switch (cmd)
  {
    case APP_CMD_INPUT_CHANGED:
    case APP_CMD_GAINED_FOCUS:
    case APP_CMD_LOST_FOCUS:
    case APP_CMD_CONTENT_RECT_CHANGED:
    case APP_CMD_INIT_WINDOW:
    case APP_CMD_WINDOW_RESIZED:
    case APP_CMD_WINDOW_REDRAW_NEEDED:
    case APP_CMD_TERM_WINDOW: push_android_command_and_wait(cmd); break;
    default: debug("CMD: Ignored: %s", cmdName); break;
  }
}


static int32_t dagor_android_handle_input_thread(struct android_app *app, AInputEvent *event)
{
  for (int i = 0; i < android::MAX_INPUT_CALLBACK_NUM; i++)
    if (android::input_callbacks[i])
      (*android::input_callbacks[i])(event);

  int src = AInputEvent_getSource(event);

  int ime_status = HumanInput::getScreenKeyboardStatus_android();
  if (ime_status == 1)
    return 0;
  switch (AInputEvent_getType(event))
  {
    case AINPUT_EVENT_TYPE_MOTION: return ime_status == 2 ? 0 : push_android_input(process_android_motion_event(app, src, event));
    case AINPUT_EVENT_TYPE_KEY: return push_android_input(process_android_key_event(app, event));
  }
  return 0;
}


// Fatal has happend. The game is going to exit anyway.
// We can't process Android OS commands properly in that state.
// For example, the render thread has caused a fatal and we've received APP_CMD_TERM_WINDOW
// In that case the game can't process the command and might cause ANR.
// We can't just ignore the commands - the render might work and crash
// in case we ignore window related commands.
// So, the only way - just exit on receving such commands.
std::atomic<bool> dagor_android_in_fatal_state = false;


bool android_should_ignore_cmd(int8_t cmd) { return dagor_should_ignore_android_cmd ? dagor_should_ignore_android_cmd(cmd) : false; }


static void dagor_android_handle_cmd(struct android_app *app, int32_t cmd)
{
  if (dagor_android_in_fatal_state)
  {
    switch (cmd)
    {
      case APP_CMD_INIT_WINDOW:
      case APP_CMD_WINDOW_RESIZED:
      case APP_CMD_TERM_WINDOW:
      case APP_CMD_DESTROY:
        debug("CMD: Commands can't be process during Fatal. Exit.");
        quit_game(-1);
        break;
    };
  }

  static uint32_t lastReinitFrameNo = 0;

  switch (cmd)
  {
    case APP_CMD_INPUT_CHANGED:
      if (app->externalInputProcessing)
      {
        if (g_dagor_input_thread.isThreadStarted())
          g_dagor_input_thread.terminate(true);
        g_dagor_input_thread.inputQueue = app->inputQueue;

        if (g_dagor_input_thread.inputQueue)
          g_dagor_input_thread.start();
      }
      break;
    case APP_CMD_INIT_WINDOW:
      // The window is being shown, get it ready.
      if (app->window != NULL)
      {
        debug("CMD: init window %p", app->window);
        win32_set_main_wnd(app->window);

        lastReinitFrameNo = dagor_frame_no();
        android_d3d_reinit(app->window);
      }
      break;
    case APP_CMD_WINDOW_RESIZED:
    case APP_CMD_WINDOW_REDRAW_NEEDED:
      if (app->window != NULL)
      {
        debug("CMD: window resized %p", app->window);
        const uint32_t curFrameNo = dagor_frame_no();

        int32_t new_width = ANativeWindow_getWidth(app->window);
        int32_t new_height = ANativeWindow_getHeight(app->window);
        if ((curFrameNo != lastReinitFrameNo) || (new_width != native_android::window_width) ||
            (new_height != native_android::window_height))
        {
          lastReinitFrameNo = curFrameNo;
          android_d3d_reinit(app->window);
        }
        else
          debug("CMD: window resized: ignore reinit for the same frame or same window size.");
      }
      break;
    case APP_CMD_TERM_WINDOW:
      // The window is being hidden or closed, clean it up.
      debug("CMD: term window");
      discard_unused_managed_textures();
      debug("CMD: unload unused tex done");
      d3d::window_destroyed(app->window);
      win32_set_main_wnd(NULL);
      break;
    case APP_CMD_GAINED_FOCUS:
      // When our app gains focus, we start monitoring the accelerometer.
      debug("CMD: gained focus");
      dgs_app_active = true;
      break;
    case APP_CMD_LOST_FOCUS:
      debug("CMD: lost focus");
      dgs_app_active = false;
      break;
    case APP_CMD_DESTROY:
      debug("CMD: destroy: %s", dagor_fast_shutdown ? "fast" : "normal");

      if (app->externalInputProcessing)
        g_dagor_input_thread.terminate(true);

      if (!dagor_fast_shutdown && d3d::is_inited())
      {
        debug("d3d::release_driver");
        d3d::release_driver();
        debug("d3d::is_inited()=%d", d3d::is_inited());
      }
      win32_set_instance(NULL);
      break;
  }
  if (dagor_android_user_message_loop_handler)
    dagor_android_user_message_loop_handler(app, cmd);
}


static void run_android_command_as_delayed_action(int32_t cmd)
{
  const char *cmdName = android::app_command_to_string(cmd);

  auto handleCmd = [cmd]() { dagor_android_handle_cmd((android_app *)win32_get_instance(), cmd); };

  if (is_main_thread())
  {
    handleCmd();
    debug("CMD: Done: %s", cmdName);
  }
  else
  {
    delayed_call(handleCmd);
    debug("CMD: Done: %s (delayed)", cmdName);
  }
}


static void android_exit_app(int code)
{
  android_exit_app_ptr = NULL;
  android_app *state = (android_app *)win32_get_instance();
  debug("android_exit_app(%p) code = %i", state, code);
  if (code < 0)
  {
    debug("fast exit due to exit code is < 0");
    win32_set_instance(NULL);
    android_native_app_destroy(state);
    debug("--- application destroyed ---");
    _exit(0);
    logerr("unreachable code after exit!");
    return;
  }
  if (state)
  {
    android::activity_finish(state->activity);
    if (is_main_thread())
      for (;;)
      {
        dagor_idle_cycle(false);
        sleep_msec(10);
        if (!dgs_app_active)
        {
          for (int i = 0; i < 50; i++)
          {
            dagor_idle_cycle(false);
            sleep_msec(10);
          }
          debug("immediately destroy");
          win32_set_instance(NULL);
          android_native_app_destroy(state);
          debug("--- application destroyed ---");
          _exit(0);
        }
      }
  }
  if (!is_main_thread())
    for (;;)
      sleep_msec(1000);
}

SimpleString get_library_name(JavaVM *jvm, jobject activity)
{
#define JNI_ASSERT(jni, cond)         \
  if (jni->ExceptionCheck())          \
  {                                   \
    jni->ExceptionDescribe();         \
    G_ASSERT(!jni->ExceptionCheck()); \
  }                                   \
  G_ASSERT(cond);
  JNIEnv *jni;
  jvm->AttachCurrentThread(&jni, NULL);

  jclass ourActivity = jni->GetObjectClass(activity);
  JNI_ASSERT(jni, ourActivity);

  jmethodID getLibraryName = jni->GetMethodID(ourActivity, "getLibraryName", "()Ljava/lang/String;");
  if (jni->ExceptionCheck()) // No method in class is normal case
  {
    jni->ExceptionClear();
    jvm->DetachCurrentThread();
    return SimpleString("");
  }

  jstring libraryName = (jstring)jni->CallObjectMethod(activity, getLibraryName);
  JNI_ASSERT(jni, libraryName);

  const char *libraryNameC = jni->GetStringUTFChars(libraryName, NULL);
  if (libraryNameC == NULL)
  {
    jvm->DetachCurrentThread();
    return SimpleString("");
  }

  SimpleString ret(libraryNameC);

  jni->ReleaseStringUTFChars(libraryName, libraryNameC);

  jvm->DetachCurrentThread();
  return ret;
#undef JNI_ASSERT
}

SimpleString get_logs_folder(JavaVM *jvm, jobject activity)
{
#define JNI_ASSERT(jni, cond)         \
  if (jni->ExceptionCheck())          \
  {                                   \
    jni->ExceptionDescribe();         \
    G_ASSERT(!jni->ExceptionCheck()); \
  }                                   \
  G_ASSERT(cond);
  JNIEnv *jni;
  jvm->AttachCurrentThread(&jni, NULL);

  jclass ourActivity = jni->GetObjectClass(activity);
  JNI_ASSERT(jni, ourActivity);

  jmethodID getExternalMediaDirs = jni->GetMethodID(ourActivity, "getExternalMediaDirs", "()[Ljava/io/File;");
  if (jni->ExceptionCheck()) // No method in class is normal case
  {
    jni->ExceptionClear();
    jvm->DetachCurrentThread();
    return SimpleString("");
  }

  jobjectArray mediaDirs = (jobjectArray)jni->CallObjectMethod(activity, getExternalMediaDirs);
  JNI_ASSERT(jni, mediaDirs);

  jclass fileClass = jni->FindClass("java/io/File");
  JNI_ASSERT(jni, fileClass);

  jmethodID getAbsolutePath = jni->GetMethodID(fileClass, "getAbsolutePath", "()Ljava/lang/String;");
  JNI_ASSERT(jni, getAbsolutePath);

  for (int i = 0, sz = jni->GetArrayLength(mediaDirs); i < sz; ++i)
  {
    jstring path = (jstring)jni->CallObjectMethod(jni->GetObjectArrayElement(mediaDirs, i), getAbsolutePath);
    JNI_ASSERT(jni, fileClass);

    if (path)
    {
      const char *pathUTF = jni->GetStringUTFChars(path, NULL);
      SimpleString ret(pathUTF);
      jni->ReleaseStringUTFChars(path, pathUTF);
      jvm->DetachCurrentThread();
      return ret;
    }
  }

  jvm->DetachCurrentThread();
  return SimpleString("");
#undef JNI_ASSERT
}

SimpleString get_intent_arguments(JavaVM *jvm, jobject activity)
{
#define JNI_ASSERT(jni, cond)         \
  if (jni->ExceptionCheck())          \
  {                                   \
    jni->ExceptionDescribe();         \
    G_ASSERT(!jni->ExceptionCheck()); \
  }                                   \
  G_ASSERT(cond);

  JNIEnv *jni;
  jvm->AttachCurrentThread(&jni, NULL);

  jclass ourActivity = jni->GetObjectClass(activity);
  JNI_ASSERT(jni, ourActivity);

  jmethodID getIntentArguments = jni->GetMethodID(ourActivity, "getIntentArguments", "()Ljava/lang/String;");
  if (jni->ExceptionCheck()) // No method in class is normal case
  {
    jni->ExceptionClear();
    jvm->DetachCurrentThread();
    return SimpleString("");
  }

  jstring intentArguments = (jstring)jni->CallObjectMethod(activity, getIntentArguments);
  JNI_ASSERT(jni, intentArguments);

  const char *intentArgumentsC = jni->GetStringUTFChars(intentArguments, NULL);
  if (intentArgumentsC == NULL)
  {
    jvm->DetachCurrentThread();
    return SimpleString("");
  }

  SimpleString ret(intentArgumentsC);

  jni->ReleaseStringUTFChars(intentArguments, intentArgumentsC);

  jvm->DetachCurrentThread();
  return ret;
#undef JNI_ASSERT
}

extern void setup_debug_system(const char *exe_fname, const char *prefix, bool datetime_name, const size_t rotatedCount);

static constexpr int MAX_ARGS_COUNT = 64;
#if defined(DEBUG_ANDROID_CMDLINE_FILE)
#include <folders/folders.h>
#include <osApiWrappers/dag_files.h>

void read_argv_from_file(const char *file, int &argc, char **argv)
{
  String fileName = String(0, "%s/%s", folders::get_game_dir(), file);
  file_ptr_t cmdFile = df_open(fileName, DF_READ | DF_IGNORE_MISSING);
  if (cmdFile)
  {
    debug("Android command line file found %s", fileName);
    int lSize = df_length(cmdFile);
    char *cmdBuffer = (char *)malloc(sizeof(char) * (lSize + 1));
    cmdBuffer[lSize] = 0;
    if (df_read(cmdFile, cmdBuffer, lSize) == lSize)
    {
      const char *delimeter = " \n";
      char *token = strtok(cmdBuffer, delimeter);
      while ((argc < MAX_ARGS_COUNT) && (token != NULL))
      {
        int l = strlen(token);
        argv[argc] = new char[l + 1];
        strcpy(argv[argc], token);
        argv[argc][l] = 0;
        token = strtok(NULL, delimeter);
        argc++;
      }
      debug("Found %d args in android command line file", argc);
    }
    else
    {
      debug("Can't read android command line file");
    }
    free(cmdBuffer);
    df_close(cmdFile);
  }
}
#endif


static void android_looper_poll_all_blocking()
{
  android_app *app = (android_app *)win32_get_instance();
  if (!app)
    return;

  struct android_poll_source *source;
  int ident, events;
  while ((ident = ALooper_pollAll(-1 /* wait until an event appears */, nullptr, &events, (void **)&source)) >= 0)
  {
    if (source != NULL)
      source->process(source->app, source);

    // Check if we are exiting.
    if (app->destroyRequested != 0)
    {
      push_android_command_and_wait(APP_CMD_DESTROY);
      android_native_app_destroy(app);
      debug("--- application destroyed ---");
      _exit(0);
      return;
    }
  }
}


void android_main(struct android_app *state)
{
  JavaVM *jvm = android::get_java_vm(state->activity);
  jobject activityObj = android::get_activity_class(state->activity);
  SimpleString logsFolder = get_logs_folder(jvm, activityObj);

  SimpleString libraryName = get_library_name(jvm, activityObj);
  setup_debug_system((libraryName == "") ? "run" : libraryName,
    logsFolder.empty() ? state->activity->externalDataPath : logsFolder.c_str(), DAGOR_DBGLEVEL > 0, ANDROID_ROTATED_LOGS);

  g_android_joystick_events_free.set();

  state->onAppCmd = dagor_android_handle_cmd_thread;
  state->externalInputProcessing = 1;

  apply_hinstance(state, NULL);

  android_exit_app_ptr = &android_exit_app;

  measure_cpu_freq();

  dagor_android_internal_path = state->activity->internalDataPath;
  dagor_android_external_path = state->activity->externalDataPath;

  SimpleString defaultLibName{"android-native-app"};

  char *argv[MAX_ARGS_COUNT];
  int argc = 0;

  auto copyArg = [&](const char *arg, size_t len) {
    G_ASSERT(argc < MAX_ARGS_COUNT);

    argv[argc] = new char[len + 1];
    argv[argc][len] = '\0';
    ::strncpy(argv[argc], arg, len);
    ++argc;
  };

  const char *libname = !libraryName.empty() ? libraryName.c_str() : defaultLibName.c_str();
  copyArg(libname, ::strlen(libname));

#if DAGOR_DBGLEVEL > 0
  SimpleString intentArguments = get_intent_arguments(jvm, activityObj);
  debug("intentArguments: %s", intentArguments.c_str());

  int start = 0;
  for (int cursor = 0; cursor < intentArguments.length(); ++cursor)
  {
    char c = intentArguments[cursor];
    if (c == ' ')
    {
      if (cursor > start)
        copyArg(intentArguments.c_str() + start, cursor - start);
      start = cursor + 1;
    }
  }

  if (intentArguments.length() > start)
    copyArg(intentArguments.c_str() + start, intentArguments.length() - start);
#endif

#if defined(DEBUG_ANDROID_CMDLINE_FILE)
  read_argv_from_file(DEBUG_ANDROID_CMDLINE_FILE, argc, argv);
#endif

  dgs_init_argv(argc, argv);

  dgs_report_fatal_error = messagebox_report_fatal_error;

#if DAGOR_DBGLEVEL != 0
  debug("BUILD TIMESTAMP:   %s %s\n\n", dagor_exe_build_date, dagor_exe_build_time);
#endif

  g_jvm = state->activity->vm;

#define JNI_ASSERT(jni, cond) G_ASSERT((cond) && !jni->ExceptionCheck())
  JNIEnv *jni;
  state->activity->vm->AttachCurrentThread(&jni, NULL);

  jclass activityClass = jni->FindClass("android/app/NativeActivity");
  JNI_ASSERT(jni, activityClass);

  jmethodID getWindowManager = jni->GetMethodID(activityClass, "getWindowManager", "()Landroid/view/WindowManager;");
  JNI_ASSERT(jni, getWindowManager);

  jobject wm = jni->CallObjectMethod(android::get_activity_class(state->activity), getWindowManager);
  JNI_ASSERT(jni, wm);

  jclass windowManagerClass = jni->FindClass("android/view/WindowManager");
  JNI_ASSERT(jni, windowManagerClass);

  jmethodID getDefaultDisplay = jni->GetMethodID(windowManagerClass, "getDefaultDisplay", "()Landroid/view/Display;");
  JNI_ASSERT(jni, getDefaultDisplay);

  jobject display = jni->CallObjectMethod(wm, getDefaultDisplay);
  JNI_ASSERT(jni, display);

  jclass displayClass = jni->FindClass("android/view/Display");
  JNI_ASSERT(jni, displayClass);

  {
    jmethodID getWidth = jni->GetMethodID(displayClass, "getWidth", "()I");
    JNI_ASSERT(jni, getWidth);
    jmethodID getHeight = jni->GetMethodID(displayClass, "getHeight", "()I");
    JNI_ASSERT(jni, getHeight);

    dagor_android_scr_xres = jni->CallIntMethod(display, getWidth);
    JNI_ASSERT(jni, true);
    dagor_android_scr_yres = jni->CallIntMethod(display, getHeight);
    JNI_ASSERT(jni, true);
  }

  jclass displayMetricsClass = jni->FindClass("android/util/DisplayMetrics");
  JNI_ASSERT(jni, displayMetricsClass);

  jmethodID displayMetricsConstructor = jni->GetMethodID(displayMetricsClass, "<init>", "()V");
  JNI_ASSERT(jni, displayMetricsConstructor);

  jobject displayMetrics = jni->NewObject(displayMetricsClass, displayMetricsConstructor);
  JNI_ASSERT(jni, displayMetrics);

  jmethodID getMetrics = jni->GetMethodID(displayClass, "getMetrics", "(Landroid/util/DisplayMetrics;)V");
  JNI_ASSERT(jni, getMetrics);

  jni->CallVoidMethod(display, getMetrics, displayMetrics);
  JNI_ASSERT(jni, true);

  {
    jfieldID xdpi_id = jni->GetFieldID(displayMetricsClass, "xdpi", "F");
    JNI_ASSERT(jni, xdpi_id);
    jfieldID ydpi_id = jni->GetFieldID(displayMetricsClass, "ydpi", "F");
    JNI_ASSERT(jni, ydpi_id);

    dagor_android_scr_xdpi = jni->GetFloatField(displayMetrics, xdpi_id);
    JNI_ASSERT(jni, true);
    dagor_android_scr_ydpi = jni->GetFloatField(displayMetrics, ydpi_id);
    JNI_ASSERT(jni, true);
  }

  jclass jcls_PackageInfo = jni->FindClass("android/content/pm/PackageInfo");
  jclass jcls_PackageManager = jni->FindClass("android/content/pm/PackageManager");

  jobject pkgMgr = jni->CallObjectMethod(android::get_activity_class(state->activity),
    jni->GetMethodID(activityClass, "getPackageManager", "()Landroid/content/pm/PackageManager;"));
  jstring pkgName = (jstring)jni->CallObjectMethod(android::get_activity_class(state->activity),
    jni->GetMethodID(activityClass, "getPackageName", "()Ljava/lang/String;"));
  jobject pkgInfo = jni->CallObjectMethod(pkgMgr,
    jni->GetMethodID(jcls_PackageManager, "getPackageInfo", "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;"), pkgName, 0);
  jni->ExceptionClear();

  jstring ver_name = (jstring)jni->GetObjectField(pkgInfo, jni->GetFieldID(jcls_PackageInfo, "versionName", "Ljava/lang/String;"));

  if (const char *utf8 = jni->GetStringUTFChars(pkgName, NULL))
  {
    dagor_android_self_pkg_name = utf8;
    jni->ReleaseStringUTFChars(pkgName, utf8);
  }
  if (const char *utf8 = jni->GetStringUTFChars(ver_name, NULL))
  {
    dagor_android_self_pkg_ver_name = utf8;
    jni->ReleaseStringUTFChars(ver_name, utf8);
  }
  dagor_android_self_pkg_ver_code = jni->GetIntField(pkgInfo, jni->GetFieldID(jcls_PackageInfo, "versionCode", "I"));
  jni->ExceptionClear();

  android_init_soft_input(state);

  state->activity->vm->DetachCurrentThread();
#undef JNI_ASSERT
  debug("Android: screenRes=%dx%d, DPI=%.1f,%.1f, scrSz=%.1fx%.1f cm", dagor_android_scr_xres, dagor_android_scr_yres,
    dagor_android_scr_xdpi, dagor_android_scr_ydpi, dagor_android_scr_xres / dagor_android_scr_xdpi * 2.54,
    dagor_android_scr_yres / dagor_android_scr_ydpi * 2.54);

  dagor_command_thread_looper = ALooper_forThread();
  debug("Androd: dagor_command_thread_looper: %p", dagor_command_thread_looper);

  g_dagor_main_thread.start();

  while (g_dagor_main_thread.isThreadRunnning())
    android_looper_poll_all_blocking();

  if (g_dagor_main_thread.isThreadStarted())
    g_dagor_main_thread.terminate(true);

#if defined(DEBUG_ANDROID_CMDLINE_FILE)
  for (int i = 0; i < argc; i++)
    delete[] argv[i];
#endif

  win32_set_instance(NULL);
}


void dagor_process_sys_messages(bool /*input_only*/)
{
  android_app *app = (android_app *)win32_get_instance();
  if (!app)
    return;

  const int32_t pendingCommand = g_android_pending_command.load(std::memory_order_acquire);
  if (pendingCommand >= 0)
  {
    dagor_android_handle_cmd(app, pendingCommand);

    g_android_pending_command.store(-1, std::memory_order_release);
    g_android_done_command_condition.notify_all();
  }

  int inputCallsSize = 0;
  InputCallsArray inputCalls;

  {
    std::unique_lock<std::mutex> lock(g_android_input_mutex);
    if (g_android_next_input_call_id > 0)
    {
      inputCallsSize = g_android_next_input_call_id;

      auto from = g_android_input_calls.begin();
      auto to = from + g_android_next_input_call_id;
      eastl::copy(from, to, inputCalls.begin());
      g_android_next_input_call_id = 0;
    }
  }

  for (int i = 0; i < inputCallsSize; ++i)
  {
    const MainWindowProcCall &call = inputCalls[i];
    workcycle_internal::main_window_proc(app, call.message, call.wParam, call.lParam);
  }

  free_android_joystick_events(inputCalls, inputCallsSize);
}

void dagor_idle_cycle(bool input_only)
{
  TIME_PROFILE(dagor_idle_cycle);
  perform_regular_actions_for_idle_cycle();
  dagor_process_sys_messages(input_only);
}

const char *os_get_default_lang()
{
  android_app *state = (android_app *)win32_get_instance();
  if (!state)
    return "en";

  static char buf[4] = {0, 0, 0, 0};
  AConfiguration_getLanguage(state->config, buf);
  buf[2] = 0;
  return buf;
}

const char *os_get_default_country()
{
  android_app *state = (android_app *)win32_get_instance();
  if (!state)
    return "";

  static char buf[4] = {0, 0, 0, 0};
  AConfiguration_getCountry(state->config, buf);
  buf[2] = 0;
  return buf;
}

void android_set_keep_screen_on(bool on)
{
  android_app *state = (android_app *)win32_get_instance();
  if (!state)
  {
    G_ASSERT(!"Android: activity instance is empty");
    return;
  }
  android::activity_setWindowFlags(state->activity, on ? AWINDOW_FLAG_KEEP_SCREEN_ON : 0, on ? 0 : AWINDOW_FLAG_KEEP_SCREEN_ON);
}
