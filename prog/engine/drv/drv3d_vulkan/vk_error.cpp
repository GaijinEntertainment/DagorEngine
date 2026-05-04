// Copyright (C) Gaijin Games KFT.  All rights reserved.

// TLS like wrapper for "last error" style error reporting
#include "vk_error.h"
#include "backend.h"
#include "backend_interop.h"
#include "globals.h"
#include "driver_config.h"

namespace
{
thread_local VkResult tlsLastErrorCode = VK_SUCCESS;
} // anonymous namespace

void drv3d_vulkan::set_last_error(VkResult error) { tlsLastErrorCode = error; }
VkResult drv3d_vulkan::get_last_error() { return tlsLastErrorCode; }

#define VULKAN_RESULT_CODE_LIST                        \
  VULKAN_OK_CODE(VK_SUCCESS)                           \
  VULKAN_OK_CODE(VK_NOT_READY)                         \
  VULKAN_OK_CODE(VK_TIMEOUT)                           \
  VULKAN_OK_CODE(VK_EVENT_SET)                         \
  VULKAN_OK_CODE(VK_EVENT_RESET)                       \
  VULKAN_OK_CODE(VK_INCOMPLETE)                        \
  VULKAN_OK_CODE(VK_PIPELINE_COMPILE_REQUIRED)         \
  VULKAN_ERROR_CODE(VK_ERROR_OUT_OF_HOST_MEMORY)       \
  VULKAN_ERROR_CODE(VK_ERROR_OUT_OF_DEVICE_MEMORY)     \
  VULKAN_ERROR_CODE(VK_ERROR_INITIALIZATION_FAILED)    \
  VULKAN_ERROR_CODE(VK_ERROR_DEVICE_LOST)              \
  VULKAN_ERROR_CODE(VK_ERROR_MEMORY_MAP_FAILED)        \
  VULKAN_ERROR_CODE(VK_ERROR_LAYER_NOT_PRESENT)        \
  VULKAN_ERROR_CODE(VK_ERROR_EXTENSION_NOT_PRESENT)    \
  VULKAN_ERROR_CODE(VK_ERROR_FEATURE_NOT_PRESENT)      \
  VULKAN_ERROR_CODE(VK_ERROR_INCOMPATIBLE_DRIVER)      \
  VULKAN_ERROR_CODE(VK_ERROR_TOO_MANY_OBJECTS)         \
  VULKAN_ERROR_CODE(VK_ERROR_FORMAT_NOT_SUPPORTED)     \
  VULKAN_ERROR_CODE(VK_ERROR_SURFACE_LOST_KHR)         \
  VULKAN_ERROR_CODE(VK_ERROR_OUT_OF_DATE_KHR)          \
  VULKAN_ERROR_CODE(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR) \
  VULKAN_ERROR_CODE(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR) \
  VULKAN_ERROR_CODE(VK_ERROR_VALIDATION_FAILED_EXT)    \
  VULKAN_ERROR_CODE(VK_ERROR_FRAGMENTED_POOL)          \
  VULKAN_ERROR_CODE(VK_ERROR_OUT_OF_POOL_MEMORY_KHR)

const char *drv3d_vulkan::vulkan_error_string(VkResult result)
{
  switch (result)
  {
    default:
      return result > 0 ? "unknown vulkan success code" : "unknown vulkan error code";
// report success codes
#define VULKAN_OK_CODE(name) \
  case name: return #name;
// report error codes
#define VULKAN_ERROR_CODE(name) \
  case name: return #name;
      VULKAN_RESULT_CODE_LIST
#undef VULKAN_OK_CODE
#undef VULKAN_ERROR_CODE
  }
}

VkResult drv3d_vulkan::vulkan_check_non_success_result(VkResult result, const char *line, const char *file, int lno)
{
  switch (result)
  {
    default:
      debug("%s returned unknown return code %u, %s %u", line, result, file, lno);
      break;
// report success codes
#define VULKAN_OK_CODE(name) \
  case name: return result; break;
// report error codes
#define VULKAN_ERROR_CODE(name) \
  case name: /*D3D_ERROR*/ debug("%s returned %s, %s %u", line, #name, file, lno); break;
      VULKAN_RESULT_CODE_LIST
#undef VULKAN_OK_CODE
#undef VULKAN_ERROR_CODE
  }
  // G_ASSERT(result >= 0);
  if (VK_ERROR_DEVICE_LOST == result)
  {
    deviceLostIfFailed(result);
    // ignore device lost failures on pending operations that can't be dropped
    // "bypass" most of methods, yet process only at needed points (submits, fence waits)
    return Globals::cfg.bits.resetOnDeviceLost ? VK_SUCCESS : result;
  }
  set_last_error(result);
  return result;
}

#undef VULKAN_RESULT_CODE_LIST

void drv3d_vulkan::deviceLostIfFailed(VkResult error)
{
  if (VULKAN_OK(error))
    return;
  set_last_error(error);
  if (!Backend::interop.deviceLost.load())
  {
    Backend::interop.deviceLost = true;
    generateFaultReport();
    if (Globals::cfg.bits.resetOnDeviceLost)
      D3D_ERROR("vulkan: GPU crash detected (code: %s:%08lX)", vulkan_error_string(error), error);
    else
      DAG_FATAL("vulkan: GPU crash detected (code: %s:%08lX)", vulkan_error_string(error), error);
  }
}
