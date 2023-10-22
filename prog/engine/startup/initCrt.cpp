#include <supp/_platform.h>
#include <memory/dag_mem.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/setProgGlobals.h>
#include <startup/dag_globalSettings.h>
#include <perfMon/dag_perfTimer.h>
#include <math/random/dag_random.h>
#include <dag_noise/dag_uint_noise.h>
#include <util/engineInternals.h>
#include <time.h>
#include <supp/dag_define_COREIMP.h>

extern void check_cpuid();
extern void init_math();
#if !_TARGET_XBOX
extern KRNLIMP void init_main_thread_id();
#endif

void default_crt_init_kernel_lib()
{
  init_profile_timer();
  init_quit_crit_sec();
  check_cpuid();
  dd_init_local_conv();
#if !_TARGET_XBOX
  init_main_thread_id();
#endif
}

void default_crt_init_core_lib()
{
  // we still add ticks to make unlikely even two clients started in same time to be with same seed
  // we also hash it once, so results are less predictable
  set_rnd_seed(uint32_hash(time(NULL) + int(ref_time_ticks())));
  init_math();
}

void apply_hinstance(void *hInstance, void * /*hPrevInstance*/) { win32_set_instance(hInstance); }

#if !_TARGET_STATIC_LIB
extern "C" const char *dagor_exe_build_date;
extern "C" const char *dagor_exe_build_time;

extern "C" const char *dagor_get_build_stamp_str(char *buf, size_t bufsz, const char *suffix)
{
  return dagor_get_build_stamp_str_ex(buf, bufsz, suffix, dagor_exe_build_date, dagor_exe_build_time);
}

#endif
