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

#if _TARGET_SIMD_SSE
#include <xmmintrin.h>
#endif
#if _TARGET_SIMD_NEON && defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#endif
#include <util/dag_stdint.h>

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

void set_default_fp_control_this_thread()
{
#if _TARGET_SIMD_SSE
  // FTZ + DAZ (0x40): denormals are much slower and useless for our math; DAZ also
  // covers garbage loaded from memory (e.g. unused .w of a vertex position)
  _mm_setcsr((_mm_getcsr() & ~_MM_ROUND_MASK) | _MM_FLUSH_ZERO_MASK | _MM_ROUND_NEAREST | 0x0040);
#elif _TARGET_SIMD_NEON && _TARGET_64BIT
#if defined(_MSC_VER) && !defined(__clang__)
  uint64_t fpcr = _ReadStatusReg(ARM64_FPCR);
  fpcr = (fpcr | (1ull << 24)) & ~(3ull << 22); // FZ, RMode=RN
  _WriteStatusReg(ARM64_FPCR, fpcr);
#else
  uint64_t fpcr;
  __asm__ __volatile__("mrs %0, fpcr" : "=r"(fpcr));
  fpcr = (fpcr | (1ull << 24)) & ~(3ull << 22); // FZ, RMode=RN
  __asm__ __volatile__("msr fpcr, %0" ::"r"(fpcr));
#endif
#elif _TARGET_SIMD_NEON
  // AArch32: FZ affects VFP scalar ops only, NEON is always flush-to-zero
  uint32_t fpscr;
  __asm__ __volatile__("vmrs %0, fpscr" : "=r"(fpscr));
  fpscr = (fpscr | (1u << 24)) & ~(3u << 22); // FZ, RMode=RN
  __asm__ __volatile__("vmsr fpscr, %0" ::"r"(fpscr));
#endif
}

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
  // fesetenv(FE_DFL_ENV) above resets MXCSR, so FTZ/DAZ must be re-applied last
  set_default_fp_control_this_thread();
}

void update_float_exceptions() { update_float_exceptions_this_thread(g_should_throw_float_exceptions); }

void enable_float_exceptions(bool enable)
{
  g_should_throw_float_exceptions = enable;
  update_float_exceptions_this_thread(enable);
}

bool is_float_exceptions_enabled() { return g_should_throw_float_exceptions; }

#if _TARGET_PC_WIN | _TARGET_XBOX

// Windows does not inherit FP control state into new threads: each engine thread
// applies it on entry (DaThread/cpujobs call update_float_exceptions). For the main
// thread init_seg(lib) runs before all default (user-seg) static ctors. Loader-phase
// hooks (PE TLS callbacks, DllMain) cannot be used instead: ntdll restores the
// initial thread context, including MXCSR, via NtContinue after they return.
#pragma warning(disable : 4073) // initializers put in library initialization area - intended
#pragma init_seg(lib)
static struct DagorDefaultFpCtrl
{
  DagorDefaultFpCtrl() { set_default_fp_control_this_thread(); }
} dagor_default_fp_ctrl;

#elif defined(__GNUC__) || defined(__clang__)

// POSIX inherits the FP environment on pthread_create, so setting it in the main
// thread before all default-priority static ctors covers descendant threads
__attribute__((constructor(101))) static void dagor_fp_ctrl_early_init() { set_default_fp_control_this_thread(); }

#endif

#if _TARGET_PC
#define EXPORT_PULL dll_pull_kernel_cpu_control
#include <supp/exportPull.h>
#endif
