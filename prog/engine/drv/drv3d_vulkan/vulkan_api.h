// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#if _TARGET_PC_WIN
#define VULKAN_LIB_NAME_PREFIX       "vulkan"
#define VK_USE_PLATFORM_WIN32_KHR    1
#define VULKAN_LIB_VERSION_SEPERATOR "-"
#define VULKAN_LIB_NAME_EXT          ".dll"
#elif _TARGET_PC_LINUX
#define VULKAN_LIB_NAME_PREFIX   "libvulkan"
// #define VK_USE_PLATFORM_MIR_KHR 1
// #define VK_USE_PLATFORM_WAYLAND_KHR 1
// #define VK_USE_PLATFORM_XCB_KHR 1
#define VK_USE_PLATFORM_XLIB_KHR 1
#include <ioSys/dag_dataBlock.h> // to be loaded BEFORE #define Bool ...
#define VULKAN_LIB_VERSION_SEPERATOR "."
#define VULKAN_LIB_NAME_EXT          ".so"
#elif _TARGET_ANDROID
#define VULKAN_LIB_NAME_PREFIX       "libvulkan"
#define VK_USE_PLATFORM_ANDROID_KHR  1
#define VULKAN_LIB_VERSION_SEPERATOR "."
#define VULKAN_LIB_NAME_EXT          ".so"
#elif _TARGET_C3



#else
#error "not supported target!"
#endif

#include "vulkan/vulkan.h"

// turn off things the switch does not has
#if _TARGET_C3






#endif

// new added formats break enum linearity, this place must be fixed to support them
#define VK_FORMAT_BEGIN_RANGE VK_FORMAT_UNDEFINED
#define VK_FORMAT_END_RANGE   VK_FORMAT_ASTC_12x12_SRGB_BLOCK
#define VK_FORMAT_RANGE_SIZE  (VK_FORMAT_ASTC_12x12_SRGB_BLOCK - VK_FORMAT_UNDEFINED + 1)

// use only KHR version by default
#undef VK_NV_dedicated_allocation

#define VULKAN_FUNCTION_TYPE(name) PFN_##name

#define VULKAN_ABI_VERSION         1
#define VULKAN_API_VERSION_MAJOR   1
#define VULKAN_API_VERSION_MINOR   1
#define VULKAN_API_VERSION_PATCH   0
#define VULKAN_API_VERSION_RELEASE 0

#define VULKAN_ABI_VERSION_S         "1"
#define VULKAN_API_VERSION_MAJOR_S   "1"
#define VULKAN_API_VERSION_MINOR_S   "1"
#define VULKAN_API_VERSION_PATCH_S   "0"
#define VULKAN_API_VERSION_RELEASE_S "0"

#if VULKAN_USE_EXPLICIT_VERSION
#define VULKAN_LIB_VERSION     \
  VULKAN_ABI_VERSION_S         \
  VULKAN_LIB_VERSION_SEPERATOR \
  VULKAN_API_VERSION_MAJOR_S   \
  VULKAN_LIB_VERSION_SEPERATOR \
  VULKAN_API_VERSION_MINOR_S   \
  VULKAN_LIB_VERSION_SEPERATOR \
  VULKAN_API_VERSION_PATCH_S   \
  VULKAN_LIB_VERSION_SEPERATOR \
  VULKAN_API_VERSION_RELEASE_S
#else
#define VULKAN_LIB_VERSION VULKAN_ABI_VERSION_S
#endif


#if _TARGET_PC_WIN
#define VULKAN_LIB_NAME        \
  VULKAN_LIB_NAME_PREFIX       \
  VULKAN_LIB_VERSION_SEPERATOR \
  VULKAN_LIB_VERSION
#elif _TARGET_PC_LINUX || _TARGET_ANDROID || _TARGET_C3
#define VULKAN_LIB_NAME        \
  VULKAN_LIB_NAME_PREFIX       \
  VULKAN_LIB_NAME_EXT          \
  VULKAN_LIB_VERSION_SEPERATOR \
  VULKAN_ABI_VERSION_S
#endif

#define VULKAN_USED_VERSION VK_MAKE_VERSION(VULKAN_API_VERSION_MAJOR, VULKAN_API_VERSION_MINOR, VULKAN_API_VERSION_PATCH)
