#pragma once
#if _TARGET_PC_WIN
#define XR_USE_PLATFORM_WIN32
#endif // _TARGET_PC_WIN

#if _TARGET_ANDROID
#define XR_NO_PROTOTYPES
#define XR_USE_PLATFORM_ANDROID
#endif //_TARGET_ANDROID

#include <openxr/openxr.h>

#if _TARGET_ANDROID
#include <3d/dag_drv3d.h>
#include "androidOpenxrLoader.h"

#define LOADER_ITER(a) extern PFN_##a a
LOADER_ITER(xrGetInstanceProcAddr);
#include "openxr_postinit_func_list.inc.h"
#include "openxr_preinit_func_list.inc.h"
#undef LOADER_ITER

#endif //_TARGET_ANDROID
