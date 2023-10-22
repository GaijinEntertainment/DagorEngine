#include <osApiWrappers/dag_singleProcessChecker.h>
#include <debug/dag_debug.h>

struct MutexWrapper
{
  void *handle;
  bool single;
  MutexWrapper();
  void close();
  ~MutexWrapper();
};

#if _TARGET_PC

#if _TARGET_PC_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

MutexWrapper::MutexWrapper() : handle(NULL), single(true) {}
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
MutexWrapper::MutexWrapper() : handle(NULL), single(true) {}
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
    mutex_wrapper.single = false;
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
bool is_process_single()
{
#if _TARGET_PC
  return mutex_wrapper.single;
#else
  return true;
#endif
}
