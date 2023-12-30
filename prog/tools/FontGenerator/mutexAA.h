#include <osApiWrappers/dag_globalMutex.h>
#include <generic/dag_tab.h>
#include <util/dag_simpleString.h>
#include <debug/dag_debug.h>
#if _TARGET_PC_WIN
#include <windows.h>
#endif
#include <ctype.h>

struct MutexAutoAcquire
{
  MutexAutoAcquire(const char *_fn)
  {
    char fn[4096];
#if _TARGET_PC_WIN
    if (!GetFullPathName(_fn, 4096, fn, NULL))
#else
    if (!realpath(_fn, fn))
#endif
      strncpy(fn, _fn, 4096);
    for (char *p = fn; *p; p++)
      *p = (*p == ':' || *p == '/' || *p == '\\') ? '_' : tolower(*p);

    void *m = global_mutex_create(fn);
    global_mutex_enter(m);
    debug("+++mutex [%s] acquired", fn);

    g_mutexes.push_back(m);
    g_mutex_names.push_back() = fn;
  }
  ~MutexAutoAcquire()
  {
    if (!g_mutexes.size())
      return;

    void *m = g_mutexes.back();
    SimpleString nm(g_mutex_names.back());
    g_mutexes.pop_back();
    g_mutex_names.pop_back();

    global_mutex_leave(m);
    global_mutex_destroy(m, nm);
    debug("---mutex [%s] released", nm);
  }

public:
  static Tab<void *> g_mutexes;
  static Tab<SimpleString> g_mutex_names;

  static void __cdecl release_mutexes()
  {
    // debug("atexit: release_g_mutexes(%d)", g_mutexes.size());
    while (g_mutexes.size())
    {
      void *m = g_mutexes.back();
      SimpleString nm(g_mutex_names.back());
      g_mutexes.pop_back();
      g_mutex_names.pop_back();

      debug("! ---mutex %p [%s] released", m, nm);
      global_mutex_leave(m);
      global_mutex_destroy(m, nm);
    }
  }
};
