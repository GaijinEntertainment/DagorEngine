// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "openXr.h"

bool report_xr_result(XrInstance oxr_instance, XrResult result, const char *call, const char *module, bool *is_runtime_failure);
bool report_xr_result_with_message(XrInstance oxr_instance, XrResult result, const char *call, const char *module,
  bool *is_runtime_failure, const char *message, ...);
bool runtime_check(XrResult result, const char *function, const char *module);

#define XR_REPORT(e) report_xr_result(OXR_INSTANCE, e, #e, OXR_MODULE, nullptr)
#define XR_REPORT_RETURN(e, r)                                       \
  do                                                                 \
  {                                                                  \
    if (!report_xr_result(OXR_INSTANCE, e, #e, OXR_MODULE, nullptr)) \
      return r;                                                      \
  } while (false)
#define XR_REPORTF(e, m, ...) report_xr_result_with_message(OXR_INSTANCE, e, #e, OXR_MODULE, nullptr, m, __VA_ARGS__)
#define XR_REPORTF_RETURN(e, r, m, ...)                                                           \
  do                                                                                              \
  {                                                                                               \
    if (!report_xr_result_with_message(OXR_INSTANCE, e, #e, OXR_MODULE, nullptr, m, __VA_ARGS__)) \
      return r;                                                                                   \
  } while (false)

#define XR_REPORT_RTF(e) report_xr_result(OXR_INSTANCE, e, #e, OXR_MODULE, OXR_RUNTIME_FAILURE)
#define XR_REPORT_RETURN_RTF(e, r)                                               \
  do                                                                             \
  {                                                                              \
    if (!report_xr_result(OXR_INSTANCE, e, #e, OXR_MODULE, OXR_RUNTIME_FAILURE)) \
      return r;                                                                  \
  } while (false)

#define RUNTIME_CHECK(e) runtime_check(e, #e, OXR_MODULE)
