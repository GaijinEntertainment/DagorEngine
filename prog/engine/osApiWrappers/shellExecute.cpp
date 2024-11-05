// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_shellExecute.h>
#include <osApiWrappers/dag_progGlobals.h>

#if _TARGET_PC_WIN
#include <windows.h>
#include <shellapi.h>
#elif !_TARGET_XBOX
#include <unistd.h>
#endif

#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_threads.h>
#include <debug/dag_debug.h>
#include <string.h>
#include <stdlib.h>

#if _TARGET_ANDROID
#include <supp/dag_android_native_app_glue.h>
#endif

#if _TARGET_PC_WIN

template <typename T>
class ShellExecuteJob : public cpujobs::IJob
{
  static const int maxTextLength = 1024;

  char op[maxTextLength + 2];
  char file[maxTextLength + 2];
  char params[maxTextLength + 2];
  char dir[maxTextLength + 2];
  int openMode;

  void checkForErrors(HINSTANCE hi)
  {
    // 32 is figure from MSDN:
    // "If the function succeeds, it returns a value greater than 32.
    // If the function fails, it returns an error value that indicates the cause of the failure."
    if (intptr_t(hi) <= 32)
      debug("ShellExecute failed with code %d", intptr_t(hi));
  }
  void initOneString(char *destination, const T *source)
  {
    if (source)
      memcpy(destination, source, min(strLenWithZero(source), maxTextLength));
    else
    {
      destination[0] = 0;
      destination[1] = 0;
    }
  }

public:
  ShellExecuteJob(const T *op_, const T *file_, const T *params_, const T *dir_, OpenConsoleMode open_mode)
  {
    initOneString(op, op_);
    initOneString(file, file_);
    initOneString(params, params_);
    initOneString(dir, dir_);
    openMode = open_mode == OpenConsoleMode::Show ? SW_SHOW : SW_HIDE;

    char *dirIt = dir;
    while ((dirIt = strchr(dirIt, '/')) != NULL)
      *dirIt = '\\';
  }

  virtual void doJob();

  virtual int strLenWithZero(const T *str);

  virtual void releaseJob() { delete this; }
};

template <>
void ShellExecuteJob<char>::doJob()
{
  checkForErrors(ShellExecute(NULL, op, file, params, dir, openMode));
}

template <>
void ShellExecuteJob<wchar_t>::doJob()
{
  checkForErrors(
    ShellExecuteW(NULL, (const wchar_t *)op, (const wchar_t *)file, (const wchar_t *)params, (const wchar_t *)dir, openMode));
}

template <>
int ShellExecuteJob<char>::strLenWithZero(const char *str)
{
  return (int)strlen(str) + 1;
}
template <>
int ShellExecuteJob<wchar_t>::strLenWithZero(const wchar_t *str)
{
  return int((wcslen(str) + 1) * sizeof(wchar_t));
}

static int win_execute_mgr_id = -1;
int get_win_execute_mgr_id()
{
  if (win_execute_mgr_id < 0)
  {
    G_ASSERT(cpujobs::is_inited());
    win_execute_mgr_id = cpujobs::create_virtual_job_manager(128 << 10, WORKER_THREADS_AFFINITY_MASK, "WinShellExecute");
  }
  return win_execute_mgr_id;
}

void os_shell_execute(const char *op, const char *file, const char *params, const char *dir, bool force_sync,
  OpenConsoleMode open_mode)
{
  ShellExecuteJob<char> *job = new ShellExecuteJob<char>(op, file, params, dir, open_mode);
  if (cpujobs::is_inited() && !force_sync)
    cpujobs::add_job(get_win_execute_mgr_id(), job);
  else
  {
    job->doJob();
    job->releaseJob();
  }
}

void os_shell_execute_w(const wchar_t *op, const wchar_t *file, const wchar_t *params, const wchar_t *dir, bool force_sync,
  OpenConsoleMode open_mode)
{
  ShellExecuteJob<wchar_t> *job = new ShellExecuteJob<wchar_t>(op, file, params, dir, open_mode);
  if (cpujobs::is_inited() && !force_sync)
    cpujobs::add_job(get_win_execute_mgr_id(), job);
  else
  {
    job->doJob();
    job->releaseJob();
  }
}

#elif _TARGET_PC_LINUX

void os_shell_execute(const char *op, const char *file, const char *params, const char *dir, bool, OpenConsoleMode)
{
  if (params && *params == 0)
    params = NULL;
  bool reset_envi = true;
  if (!op || strcmp(op, "open") == 0)
    op = "/usr/bin/xdg-open";
  else if (strcmp(op, "explore") == 0)
  {
    op = "/usr/bin/xdg-open";
    file = dir;
    params = NULL;
    dir = NULL;
  }
  else if (strcmp(op, "start") == 0)
  {
    reset_envi = false;
    op = file;
    file = params;
    params = NULL;
  }
  else
  {
    logerr("unsupported: shell_execute(%s, %s, %s, %s)", op, file, params, dir);
    return;
  }

  pid_t pid = fork();
  if (!pid)
  {
    if (dir)
      int r = chdir(dir);
    if (reset_envi)
    {
      unsetenv("LD_PRELOAD");
      unsetenv("LD_LIBRARY_PATH");
    }

    execl(op, op, file, params, (char *)NULL);
    _exit(0);
  }
}

void os_shell_execute_w(const wchar_t *, const wchar_t *, const wchar_t *, const wchar_t *, bool, OpenConsoleMode) { G_ASSERT(0); }

#elif _TARGET_ANDROID
void os_shell_execute(const char *op, const char *file, const char *params, const char *dir, bool, OpenConsoleMode)
{
  android_app *app = (android_app *)win32_get_instance();
  if (!app || !app->activity)
  {
    logerr("shell_execute: bad app instance");
    return;
  }
  JavaVM *javaVM = app->activity->vm;
  JNIEnv *jniEnv = app->activity->env;
  debug("open URL: %s", file);
  jint result = javaVM->AttachCurrentThread(&jniEnv, NULL);
  if (result == JNI_ERR)
  {
    logerr("failed to attach current thread");
    return;
  }
  jclass jcls_Activity = jniEnv->FindClass("android/app/Activity");
  jclass jcls_Intent = jniEnv->FindClass("android/content/Intent");
  jclass jcls_Uri = jniEnv->FindClass("android/net/Uri");
  jstring jstr = jniEnv->NewStringUTF(file);
  jniEnv->ExceptionClear();
  jobject uriObj = jniEnv->CallStaticObjectMethod(jcls_Uri,
    jniEnv->GetStaticMethodID(jcls_Uri, "parse", "(Ljava/lang/String;)Landroid/net/Uri;"), jstr);
  jniEnv->ExceptionClear();
  jobject intentObj = jniEnv->NewObject(jcls_Intent, jniEnv->GetMethodID(jcls_Intent, "<init>", "(Ljava/lang/String;)V"),
    jniEnv->NewStringUTF("android.intent.action.VIEW"));
  jniEnv->ExceptionClear();
  jniEnv->CallObjectMethod(intentObj, jniEnv->GetMethodID(jcls_Intent, "setData", "(Landroid/net/Uri;)Landroid/content/Intent;"),
    uriObj);
  jniEnv->ExceptionClear();
  jniEnv->CallVoidMethod(android::get_activity_class(app->activity),
    jniEnv->GetMethodID(jcls_Activity, "startActivity", "(Landroid/content/Intent;)V"), intentObj);
  jniEnv->ExceptionClear();
}

void os_shell_execute_w(const wchar_t *, const wchar_t *, const wchar_t *, const wchar_t *, bool, OpenConsoleMode) { G_ASSERT(0); }

#elif _TARGET_C1 | _TARGET_C2 | _TARGET_XBOX | _TARGET_C3

void os_shell_execute(const char *, const char *, const char *, const char *, bool, OpenConsoleMode) {}
void os_shell_execute_w(const wchar_t *, const wchar_t *, const wchar_t *, const wchar_t *, bool, OpenConsoleMode) {}

#endif

#if _TARGET_PC_WIN | _TARGET_PC_LINUX
#define EXPORT_PULL dll_pull_osapiwrappers_shellExecute
#include <supp/exportPull.h>
#endif
