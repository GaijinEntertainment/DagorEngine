// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if _TARGET_PC
#include <float.h>
#if _TARGET_PC_LINUX
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <fenv.h>
#endif
#endif

#include <supp/dag_cpuControl.h>

static bool g_should_throw_float_exceptions = false;

#if _TARGET_PC_LINUX
static int feenablefpexcept(int excepts)
{
#if defined(__e2k__)
  (void)excepts;
  return 0;
#else
  unsigned short int new_exc;
  unsigned short int old_exc;
  __asm__("fstcw %0" : "=m"(*&new_exc));
  excepts &= FE_ALL_EXCEPT;
  old_exc = (~new_exc) & FE_ALL_EXCEPT;
  new_exc &= ~excepts;
  __asm__("fldcw %0" : : "m"(*&new_exc));
  return old_exc;
#endif
}
#endif

void update_float_exceptions_this_thread(bool enable)
{
#if _TARGET_PC_WIN && !_TARGET_64BIT
  if (enable)
  {
    _clear87();
    unsigned dummy;
    __control87_2(_EM_INEXACT | _EM_UNDERFLOW | _EM_DENORMAL, _MCW_EM, &dummy, NULL);
  }
  else
  {
    _clear87();
    _control87(_MCW_EM, _MCW_EM);
  }
#elif _TARGET_PC_LINUX && !defined(__e2k__)
  if (enable)
  {
    fesetenv(FE_DFL_ENV);
    feenablefpexcept(FE_ALL_EXCEPT & ~(__FE_DENORM | FE_UNDERFLOW | FE_INEXACT));
  }
  else
    fedisableexcept(FE_ALL_EXCEPT);
#else
  (void)enable;
#endif
}

void update_float_exceptions() { update_float_exceptions_this_thread(g_should_throw_float_exceptions); }

void enable_float_exceptions(bool enable)
{
  g_should_throw_float_exceptions = enable;
  update_float_exceptions_this_thread(enable);
}

bool is_float_exceptions_enabled() { return g_should_throw_float_exceptions; }

#if _TARGET_PC
#define EXPORT_PULL dll_pull_kernel_cpu_control
#include <supp/exportPull.h>
#endif
