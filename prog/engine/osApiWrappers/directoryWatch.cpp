#include <osApiWrappers/dag_watchDir.h>
#if _TARGET_PC_WIN
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_vromfs.h>
#include <windows.h>
#include <process.h>

struct WatchedFolderMonitorData
{
  volatile HANDLE hFolder = NULL;
  volatile HANDLE hThread = NULL;
  static constexpr int BUF_SZ = (32 << 10) - 20;
  char buf[BUF_SZ];
  int sleepInterval = 30;
  volatile int changedGen = 0, lastChecked = 0;
  volatile int stop = 0;
  WatchedFolderMonitorData() { buf[0] = 0; }
};

static unsigned __stdcall monitor_folder_thread(void *p)
{
  WatchedFolderMonitorData &fd = *(WatchedFolderMonitorData *)p;

  while (!interlocked_acquire_load(fd.stop))
  {
    DWORD bytesret = 0;

    if (!ReadDirectoryChangesW(fd.hFolder, fd.buf, fd.BUF_SZ, TRUE,
          FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_FILE_NAME,
          &bytesret, NULL, NULL))
    {
      if (interlocked_acquire_load(fd.stop))
        break;
      sleep_msec(fd.sleepInterval);
      continue;
    }
    interlocked_increment(fd.changedGen); // return resulted incremented value for val
  }
  ::CloseHandle(fd.hFolder);
  fd.hFolder = NULL;
  _endthreadex(0);
  return 0;
}

#include <supp/_platform.h>

bool could_be_changed_folder(WatchedFolderMonitorData *d)
{
  if (!d)
    return true;
  WatchedFolderMonitorData &fd = *d;
  if (!fd.hFolder)
    return true;
  const bool ret = interlocked_acquire_load(fd.lastChecked) != interlocked_acquire_load(fd.changedGen);
  interlocked_release_store(fd.lastChecked, fd.changedGen);
  return ret;
}

WatchedFolderMonitorData *add_folder_monitor(const char *foldername, int sleep_interval)
{
  HANDLE hDir = CreateFile(foldername, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE | FILE_LIST_DIRECTORY,
    NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

  if (!hDir)
    return nullptr;

  WatchedFolderMonitorData *d = new WatchedFolderMonitorData;
  d->hFolder = hDir;
  d->sleepInterval = sleep_interval;

  uintptr_t handle = _beginthreadex(NULL, 4096, &monitor_folder_thread, d, CREATE_SUSPENDED, NULL);
  if (handle == -1)
  {
    CloseHandle(d->hFolder);
    delete d;
    return nullptr;
  }
  d->hThread = (HANDLE)handle;
  ResumeThread((HANDLE)handle);
  return d;
}

bool destroy_folder_monitor(WatchedFolderMonitorData *d, int attempts)
{
  if (!d)
    return true;
  WatchedFolderMonitorData &fd = *d;
  interlocked_release_store(fd.stop, 1);

  HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
  if (kernel32)
  {
    typedef BOOL(WINAPI * PCancelIoEx)(HANDLE hFile, LPOVERLAPPED lpOverlapped);
    PCancelIoEx pCancelIoEx = (PCancelIoEx)(void *)GetProcAddress(kernel32, "CancelIoEx"); // CancelIoEx is supported starting with
                                                                                           // Vista.
    if (pCancelIoEx)
      pCancelIoEx(fd.hFolder, NULL);
  }

  if (WaitForSingleObject(fd.hThread, attempts <= 0 ? INFINITE : fd.sleepInterval * attempts + 1) == WAIT_OBJECT_0)
  {
    CloseHandle(fd.hThread);
    delete d;
    return true;
  }
  return false;
}
#else

bool could_be_changed_folder(WatchedFolderMonitorData *) { return true; }
WatchedFolderMonitorData *add_folder_monitor(const char *, int) { return nullptr; }
bool destroy_folder_monitor(WatchedFolderMonitorData *, int) { return true; }
#endif

#define EXPORT_PULL dll_pull_osapiwrappers_directory_watch
#include <supp/exportPull.h>
