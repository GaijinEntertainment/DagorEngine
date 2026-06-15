// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_singleProcessChecker.h>
#include <debug/dag_debug.h>

#if _TARGET_PC_WIN && DAGOR_DBGLEVEL > 0
#include <osApiWrappers/dag_messageBox.h>
#include <osApiWrappers/dag_miscApi.h>
#endif

struct MutexWrapper
{
  void *handle;
  MutexWrapper();
  void close();
  ~MutexWrapper();
};

#if _TARGET_PC

#if _TARGET_PC_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

MutexWrapper::MutexWrapper() : handle(NULL) {}
MutexWrapper::~MutexWrapper()
{
  if (handle)
    CloseHandle(handle);
}
void MutexWrapper::close()
{
  if (handle)
  {
    CloseHandle(handle);
    handle = NULL;
  }
}
#else
MutexWrapper::MutexWrapper() : handle(NULL) {}
MutexWrapper::~MutexWrapper() {}
void MutexWrapper::close() {}
#endif

MutexWrapper mutex_wrapper;

#endif

////////////////////////////////////////////////////////////////////////////////////
bool init_single_process_checker(const char *mutex_name)
{
#if _TARGET_PC_WIN
  mutex_wrapper.handle = OpenMutex(MUTEX_MODIFY_STATE, TRUE, mutex_name);
  if (mutex_wrapper.handle)
  {
    debug("Another instance of game is already started");
#if DAGOR_DBGLEVEL < 1
    return false;
#endif
  }
  else
  {
    mutex_wrapper.handle = CreateMutex(NULL, FALSE, mutex_name);
    if (!mutex_wrapper.handle)
      debug("Failed to create mutex\n");
  }
#endif
  G_UNUSED(mutex_name);
  return true;
}
void close_single_process_checker()
{
#if _TARGET_PC
  mutex_wrapper.close();
#endif
}

ExistingInstanceAction check_or_prompt_existing_instance(const char *mutex_name)
{
#if _TARGET_PC_WIN
  mutex_wrapper.handle = OpenMutex(MUTEX_MODIFY_STATE, TRUE, mutex_name);
  if (!mutex_wrapper.handle)
  {
    mutex_wrapper.handle = CreateMutex(NULL, FALSE, mutex_name);
    if (!mutex_wrapper.handle)
      debug("Failed to create mutex\n");
    return ExistingInstanceAction::Continue;
  }

  debug("Another instance of game is already started");

#if DAGOR_DBGLEVEL > 0
  // Drop our probe handle so it doesn't keep the named mutex alive while
  // we wait. The other instance still holds it; we will re-probe below.
  CloseHandle(mutex_wrapper.handle);
  mutex_wrapper.handle = NULL;

  const int btn = os_message_box("Another instance of this executable is already running.\n\n"
                                 "Yes    -- close this instance.\n"
                                 "No     -- wait for the previous instance to exit, then continue.\n"
                                 "Cancel -- run both (may cause slow DX12 startup due to PSO cache contention).",
    "Another instance is running",
    GUI_MB_YES_NO_CANCEL | GUI_MB_ICON_WARNING | GUI_MB_NATIVE_DLG | GUI_MB_TOPMOST | GUI_MB_DEF_BUTTON_2);

  if (btn == GUI_MB_BUTTON_1)
  {
    debug("single-instance: user chose 'close this instance'");
    return ExistingInstanceAction::ExitProcess;
  }
  if (btn == GUI_MB_BUTTON_2)
  {
    debug("single-instance: user chose 'wait for previous instance'");
    for (int waited_ms = 0;; waited_ms += 500)
    {
      HANDLE probe = OpenMutex(MUTEX_MODIFY_STATE, TRUE, mutex_name);
      if (!probe)
        break;
      CloseHandle(probe);
      if (waited_ms > 0 && (waited_ms % 10000) == 0)
        debug("single-instance: still waiting for previous instance (%d s)", waited_ms / 1000);
      sleep_msec(500);
    }
    debug("single-instance: previous instance exited, continuing");
    mutex_wrapper.handle = CreateMutex(NULL, FALSE, mutex_name);
    if (!mutex_wrapper.handle)
      debug("Failed to create mutex\n");
    return ExistingInstanceAction::Continue;
  }
  // BUTTON_3 / CLOSE / FAIL -- run both
  debug("single-instance: user chose 'run both'");
  return ExistingInstanceAction::Continue;
#else
  return ExistingInstanceAction::ExitProcess;
#endif

#else
  G_UNUSED(mutex_name);
  return ExistingInstanceAction::Continue;
#endif
}
