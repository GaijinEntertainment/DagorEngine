//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <osApiWrappers/dag_globalMutex.h>
#include <util/dag_string.h>
#include <stdio.h>
#if _TARGET_PC_WIN
#include <direct.h>
#elif _TARGET_PC_LINUX | _TARGET_APPLE | _TARGET_C3
#include <unistd.h>
#endif

#define ATOMIC_PRINTF(...)                  \
  do                                        \
  {                                         \
    AtomicPrintfMutex::inst.lock();         \
    printf(__VA_ARGS__);                    \
    AtomicPrintfMutex::inst.unlock(stdout); \
  } while (0)

#define ATOMIC_FPRINTF(FP, ...)         \
  do                                    \
  {                                     \
    AtomicPrintfMutex::inst.lock();     \
    fprintf(FP, __VA_ARGS__);           \
    AtomicPrintfMutex::inst.unlock(FP); \
  } while (0)

// support for inter-process atomic printf
class AtomicPrintfMutex
{
  void *mutex;
  bool locked;
  String name;

public:
  static void init(const char *exe_name, const char *config_path)
  {
    G_ASSERT(!inst.mutex);
    char buf[DAGOR_MAX_PATH];

    inst.name.printf(0, "%s_%s@%s", exe_name, config_path, getcwd(buf, sizeof(buf)));
    inst.name.replaceAll("\\", "_");
    inst.name.replaceAll("/", "_");
    inst.mutex = global_mutex_create(inst.name);
    // ATOMIC_PRINTF("using mutex %s=%p\n", inst.name.str(), inst.mutex);
    atexit(&term);
  }
  static void term()
  {
    if (!inst.mutex)
      return;
    if (inst.locked)
      global_mutex_leave(inst.mutex);
    global_mutex_destroy(inst.mutex, inst.name);
    inst.mutex = nullptr;
    clear_and_shrink(inst.name);
  }

  inline void lock()
  {
    if (mutex)
    {
      global_mutex_enter(mutex);
      locked = true;
    }
  }
  inline void unlock(FILE *fp, bool flush_always = false)
  {
    if (mutex)
    {
      fflush(fp);
      locked = false;
      global_mutex_leave(mutex);
    }
    else if (flush_always)
      fflush(fp);
  }

  static AtomicPrintfMutex inst; // to be defined in final module explicitly: AtomicPrintfMutex AtomicPrintfMutex::inst;
};
