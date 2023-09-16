#include <debug/dag_debug.h>

#if !_TARGET_ANDROID
#error androidOpenxrLoader.cpp should be included only on android platform
#endif
#define XR_NO_PROTOTYPES
#include <dlfcn.h>
#include "openXrAndroidLoader.h"

#define LOADER_ITER(a) PFN_##a a
LOADER_ITER(xrGetInstanceProcAddr);
#include "openXrPreInitFuncList.inc.h"
#include "openXrPostinitFuncList.inc.h"
#undef LOADER_ITER
bool LoadPreInitOpenxrFuncs()
{
  void *handle = dlopen("libopenxr_loader.so", RTLD_NOW);

  if (handle)
  {
    xrGetInstanceProcAddr = reinterpret_cast<PFN_xrGetInstanceProcAddr>(dlsym(handle, "xrGetInstanceProcAddr"));

#define LOADER_ITER(a)                                                     \
  {                                                                        \
    logdbg("Loading OpenXR function: %s", #a);                             \
    xrGetInstanceProcAddr(XR_NULL_HANDLE, #a, (PFN_xrVoidFunction *)(&a)); \
    uintptr_t addr = (uintptr_t)(void *)a;                                 \
    logdbg("%s -> 0x%llx", #a, addr);                                      \
  }
#include "openxr_preinit_func_list.inc.h"
#undef LOADER_ITER

    return true;
  }
  else
  {
    logerr("Android openXR lib loading failed\n%s", dlerror());
    return false;
  }
}

bool LoadPostInitOpenxrFuncs(XrInstance instance)
{
#define LOADER_ITER(a)                                               \
  {                                                                  \
    logdbg("Loading OpenXR function: %s", #a);                       \
    xrGetInstanceProcAddr(instance, #a, (PFN_xrVoidFunction *)(&a)); \
    uintptr_t addr = (uintptr_t)(void *)a;                           \
    logdbg("%s -> 0x%llx", #a, addr);                                \
  }
#include "openXrPostInitFuncList.inc.h"
#undef LOADER_ITER

  return true;
}
