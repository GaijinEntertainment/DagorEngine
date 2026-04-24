// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/dag_debug.h>

#include "vulkan_api.h"

namespace drv3d_vulkan
{

const char *vulkan_error_string(VkResult result);
void set_last_error(VkResult error);
VkResult get_last_error();

void generateFaultReport();
void deviceLostIfFailed(VkResult error);

VkResult vulkan_check_non_success_result(VkResult result, const char *line, const char *file, int lno);

inline VkResult vulkan_check_result(VkResult result, const char *line, const char *file, int lno)
{
  if (DAGOR_LIKELY(result == VK_SUCCESS))
    return result;
  else
    return vulkan_check_non_success_result(result, line, file, lno);
}

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
