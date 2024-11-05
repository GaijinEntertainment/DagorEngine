// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/dag_debug.h>

#include "vulkan_api.h"


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

namespace drv3d_vulkan
{

inline const char *vulkan_error_string(VkResult result)
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
void set_last_error(VkResult error);
VkResult get_last_error();

inline VkResult vulkan_check_result(VkResult result, const char *line, const char *file, int lno)
{
  set_last_error(result);
  switch (result)
  {
    default:
      debug("%s returned unknown return code %u, %s %u", line, result, file, lno);
      break;
// report success codes
#define VULKAN_OK_CODE(name) \
  case name: break;
// report error codes
#define VULKAN_ERROR_CODE(name) \
  case name: /*D3D_ERROR*/ debug("%s returned %s, %s %u", line, #name, file, lno); break;
      VULKAN_RESULT_CODE_LIST
#undef VULKAN_OK_CODE
#undef VULKAN_ERROR_CODE
  }
  // G_ASSERT(result >= 0);
  return result;
}

void generateFaultReport();

#if 0
  //DAGOR_DBGLEVEL > 1
template <typename T> inline T log_and_return_internal(T c, const char *cstr)
{
  debug("%u %s %s:%u", GetCurrentThreadId(), cstr, __FILE__, __LINE__);
  return c;
}
#define VULKAN_LOG_CALL(c)                                              \
  do                                                                    \
  {                                                                     \
    debug("%u %s %s:%u", GetCurrentThreadId(), #c, __FILE__, __LINE__); \
    c;                                                                  \
  } while (0);
#define VULKAN_LOG_CALL_R(c)   log_and_return_internal(c, #c)
#define VULKAN_CHECK_RESULT(r) drv3d_vulkan::vulkan_check_result(VULKAN_LOG_CALL_R(r), #r, __FILE__, __LINE__)
#else
#define VULKAN_LOG_CALL(c)     c
#define VULKAN_LOG_CALL_R(c)   c
#define VULKAN_CHECK_RESULT(r) drv3d_vulkan::vulkan_check_result(VULKAN_LOG_CALL_R(r), #r, __FILE__, __LINE__)
#endif

#define VULKAN_OK(r)         ((r) >= 0)
#define VULKAN_FAIL(r)       ((r) < 0)
#define VULKAN_CHECK_OK(r)   VULKAN_OK(VULKAN_CHECK_RESULT(r))
#define VULKAN_CHECK_FAIL(r) VULKAN_FAIL(VULKAN_CHECK_RESULT(r))
#define VULKAN_EXIT_ON_FAIL(r) \
  if (VULKAN_CHECK_FAIL(r))    \
  {                            \
    generateFaultReport();     \
    DAG_FATAL(#r " failed");   \
  }

} // namespace drv3d_vulkan

#undef VULKAN_RESULT_CODE_LIST
