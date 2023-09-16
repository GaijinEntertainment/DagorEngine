// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)

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
#include <3d/dag_drv3d.h>
#include <3d/dag_texMgr.h>
#include <startup/dag_globalSettings.h>
#include <humanInput/dag_hiCreate.h>
#include <ioSys/dag_dataBlock.h>
#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_statDrv.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <atomic>

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
extern bool application_active;
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

bool dagor_android_fast_shutdown = false;

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

int32_t dagor_android_handle_motion_event(struct android_app *app, int event_source, void *event)
{
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
          workcycle_internal::main_window_proc(app, GPCM_TouchMoved, get_pointer_id(event, i), get_pointer(event, i));
        break;
      case AMOTION_EVENT_ACTION_DOWN:
      case AMOTION_EVENT_ACTION_POINTER_DOWN:
        workcycle_internal::main_window_proc(app, GPCM_TouchBegan, get_pointer_id(event, pidx), get_pointer(event, pidx));
        break;
      case AMOTION_EVENT_ACTION_UP:
      case AMOTION_EVENT_ACTION_POINTER_UP:
        workcycle_internal::main_window_proc(app, GPCM_TouchEnded, get_pointer_id(event, pidx), get_pointer(event, pidx));
        break;
      case AMOTION_EVENT_ACTION_CANCEL:
        for (int i = 0, ie = android::motion_event::get_pointer_count(event); i < ie; i++)
          workcycle_internal::main_window_proc(app, GPCM_TouchEnded, get_pointer_id(event, i), get_pointer(event, i));
        break;
    }
  }
  else if (event_source == AINPUT_SOURCE_JOYSTICK)
    workcycle_internal::main_window_proc(app, GPCM_JoystickMovement, intptr_t(event), 0);
  return 1;
}

int32_t dagor_android_handle_key_event(struct android_app *app, void *event)
{
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
      {
        return 0;
      }
      if (key)
        workcycle_internal::main_window_proc(app, GPCM_KeyPress, key, c);
      else if (c)
        workcycle_internal::main_window_proc(app, GPCM_Char, c, 0);
      break;
    case AKEY_EVENT_ACTION_UP: workcycle_internal::main_window_proc(app, GPCM_KeyRelease, key, 0); break;
  }
  return 1;
}

static void dagor_android_handle_cmd(struct android_app *app, int32_t cmd)
{
  switch (cmd)
  {
    case APP_CMD_SAVE_STATE:
      // The system has asked us to save our current state.  Do so.
      debug("CMD: save state");
      break;
    case APP_CMD_INIT_WINDOW:
      // The window is being shown, get it ready.
      if (app->window != NULL)
      {
        debug("CMD: init window %p", app->window);
        win32_set_main_wnd(app->window);
        android_d3d_reinit(app->window);
        workcycle_internal::application_active = dgs_app_active = true;
      }
      break;
    case APP_CMD_WINDOW_RESIZED:
      debug("CMD: window resized");
      if (app->window != NULL)
        android_d3d_reinit(app->window);
      break;
    case APP_CMD_TERM_WINDOW:
      // The window is being hidden or closed, clean it up.
      debug("CMD: term window");
      discard_unused_managed_textures();
      debug("CMD: unload unused tex done");
      workcycle_internal::application_active = dgs_app_active = false;
      d3d::window_destroyed(app->window);
      win32_set_main_wnd(NULL);
      break;
    case APP_CMD_GAINED_FOCUS:
      // When our app gains focus, we start monitoring the accelerometer.
      debug("CMD: gained focus");
      workcycle_internal::application_active = dgs_app_active = true;
      break;
    case APP_CMD_LOST_FOCUS:
      debug("CMD: lost focus");
      workcycle_internal::application_active = dgs_app_active = false;
      break;
  }
  if (dagor_android_user_message_loop_handler)
    dagor_android_user_message_loop_handler(app, cmd);
}


namespace
{

struct ANRWatcher
{
  int64_t lastPoll = 0;
  const int64_t TRIGGER_THRESHOLD_US = 1000 * 1000; // 1s
  // system will show ANR window when server trhreshold is reached, such errors must be fixed with high priority!
  const int64_t SEVERE_TRIGGER_THRESHOLD_US = 5 * 1000 * 1000; // 5s

  void check()
  {
    uint64_t now = ref_time_ticks();
    uint64_t dlt = ref_time_delta_to_usec(now - lastPoll);

    const int level = dlt > SEVERE_TRIGGER_THRESHOLD_US ? LOGLEVEL_ERR : LOGLEVEL_WARN;

    if (dlt > TRIGGER_THRESHOLD_US && lastPoll)
      logmessage(level, "android: %sANR: last system poll was %u ms ago. context: %s",
        dlt > SEVERE_TRIGGER_THRESHOLD_US ? "severe " : "", dlt / 1000,
        dagor_android_app_get_anr_execution_context ? dagor_android_app_get_anr_execution_context() : "<unknown>");
    lastPoll = now;
  }
};

struct ANRWatcherThread final : public DaThread
{
  std::atomic<int64_t> lastPoll = 0;
  std::atomic<bool> shouldReport = true;
  std::atomic<bool> shouldReportSevere = true;

  const int64_t TRIGGER_THRESHOLD_US = 1000 * 1000; // 1s
  // system will show ANR window when server trhreshold is reached, such errors must be fixed with high priority!
  const int64_t SEVERE_TRIGGER_THRESHOLD_US = 5 * 1000 * 1000; // 5s

  ANRWatcherThread() : DaThread("ANRWatcher") {}

  void execute() override
  {
    while (!terminating)
    {
      check();
      sleep_msec(100);
    }
  }

  void check()
  {
    const int64_t lastPollValue = lastPoll.load(std::memory_order_acquire);

    uint64_t now = ref_time_ticks();
    uint64_t dlt = ref_time_delta_to_usec(now - lastPollValue);

    const bool isSevere = dlt > SEVERE_TRIGGER_THRESHOLD_US;

#if DAGOR_DBGLEVEL > 0
    const int level = LOGLEVEL_ERR;
#else
    const int level = isSevere ? LOGLEVEL_ERR : LOGLEVEL_WARN;
#endif

    if (dlt > TRIGGER_THRESHOLD_US && lastPollValue)
    {
      if (!isSevere && !shouldReport.load(std::memory_order_acquire))
        return;

      if (isSevere && !shouldReportSevere.load(std::memory_order_acquire))
        return;

      if (isSevere)
        shouldReportSevere.store(false, std::memory_order_release);
      else
        shouldReport.store(false, std::memory_order_release);

      if (level == LOGLEVEL_ERR)
        dump_thread_callstack(get_main_thread_id());

      logmessage(level, "android: %sANR Thread: last system poll was %u ms ago. context: %s", isSevere ? "severe " : "", dlt / 1000,
        dagor_android_app_get_anr_execution_context ? dagor_android_app_get_anr_execution_context() : "<unknown>");
    }
  }

  void poll()
  {
    lastPoll.store(ref_time_ticks(), std::memory_order_release);
    shouldReport.store(true, std::memory_order_release);
    shouldReportSevere.store(true, std::memory_order_release);
  }
};

eastl::unique_ptr<ANRWatcher> g_anr_watcher;
eastl::unique_ptr<ANRWatcherThread> g_anr_watcher_thread;

} // namespace


static void android_exit_app(int code)
{
  if (g_anr_watcher_thread)
  {
    g_anr_watcher_thread->terminate(true /* wait */);
    g_anr_watcher_thread.reset();
  }

  g_anr_watcher.reset();

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
      const char *delimeter = " ";
      int i = 0;
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

void android_main(struct android_app *state)
{
  SimpleString libraryName = get_library_name(android::get_java_vm(state->activity), android::get_activity_class(state->activity));
  setup_debug_system((libraryName == "") ? "run" : libraryName, state->activity->externalDataPath, DAGOR_DBGLEVEL > 0,
    ANDROID_ROTATED_LOGS);

  state->onAppCmd = dagor_android_handle_cmd;

  android::init_input_handler(state);

  apply_hinstance(state, NULL);

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
  SimpleString intentArguments = get_intent_arguments(state->activity->vm, android::get_activity_class(state->activity));
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

  android_exit_app_ptr = &android_exit_app;
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

#if defined(DEBUG_ANDROID_CMDLINE_FILE)
  for (int i = 0; i < argc; i++)
    delete[] argv[i];
#endif

  win32_set_instance(NULL);
}

enum class ANRWatcherMode
{
  MainThread,
  SeparateThread,
};

void init_anr_watcher(ANRWatcherMode mode)
{
  if (g_anr_watcher_thread || g_anr_watcher)
    return;

  if (mode == ANRWatcherMode::SeparateThread)
  {
    debug("android: Use ANRWatcherThread");
    g_anr_watcher_thread.reset(new ANRWatcherThread());
    g_anr_watcher_thread->start();
  }
  else
  {
    debug("android: Use ANRWatcher");
    g_anr_watcher.reset(new ANRWatcher());
  }
}

void dagor_process_sys_messages(bool /*input_only*/)
{
  android_app *state = (android_app *)win32_get_instance();
  if (!state)
    return;

  if (g_anr_watcher)
    g_anr_watcher->check();
  else if (g_anr_watcher_thread)
    g_anr_watcher_thread->poll();

  struct android_poll_source *source;
  int ident, events;
  while ((ident = ALooper_pollAll(0, nullptr, &events, (void **)&source)) >= 0)
  {
    // Process this event.
    if (source != NULL)
      source->process(source->app, source);

    // Check if we are exiting.
    if (state->destroyRequested != 0)
    {
      debug("destroyRequested: %s", dagor_android_fast_shutdown ? "fast" : "normal");
      if (!dagor_android_fast_shutdown && d3d::is_inited())
      {
        debug("d3d::release_driver");
        d3d::release_driver();
        debug("d3d::is_inited()=%d", d3d::is_inited());
      }
      win32_set_instance(NULL);
      android_native_app_destroy(state);
      debug("--- application destroyed ---");
      _exit(0);
      return;
    }
  }

  android::process_system_messages(state);
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
