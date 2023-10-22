#include "openXrErrorReporting.h"
#include "debug/dag_log.h"
#include "osApiWrappers/dag_messageBox.h"
#include "util/dag_localization.h"
#include <EASTL/string.h>

static bool report_xr_result_base(XrInstance oxr_instance, XrResult result, const char *call, const char *module,
  bool *is_runtime_failure)
{
  if (XR_SUCCEEDED(result))
    return true;

  if (is_runtime_failure && result == XR_ERROR_RUNTIME_FAILURE)
  {
    logdbg("[XR][%s] Call to %s resulted in a runtime error. Recreating OpenXR.", module, call);
    *is_runtime_failure = true;
    return false;
  }

  char resultStr[XR_MAX_RESULT_STRING_SIZE];
  if (oxr_instance && XR_SUCCEEDED(xrResultToString(oxr_instance, result, resultStr)))
    logerr("[XR][%s] Call to %s failed with reason: %s", module, call, resultStr);
  else
  {
#define CHECK_RES(x) \
  case x: logerr("[XR][%s] Call to %s failed with reason: %s", module, call, #x); break;
    switch (result)
    {
      CHECK_RES(XR_ERROR_OUT_OF_MEMORY)
      CHECK_RES(XR_ERROR_API_LAYER_NOT_PRESENT)
      CHECK_RES(XR_ERROR_INSTANCE_LOST)
      CHECK_RES(XR_ERROR_RUNTIME_FAILURE)
      CHECK_RES(XR_ERROR_VALIDATION_FAILURE)
      CHECK_RES(XR_ERROR_SIZE_INSUFFICIENT)
      default: logerr("[XR][%s] Call to %s failed with reason: %d", module, call, result);
    }
#undef CHECK_RES
  }

  return false;
}

bool report_xr_result(XrInstance oxr_instance, XrResult result, const char *call, const char *module, bool *is_runtime_failure)
{
  bool xrResult = report_xr_result_base(oxr_instance, result, call, module, is_runtime_failure);
  bool hasRuntimeFailure = is_runtime_failure && *is_runtime_failure;
  G_ASSERTF(xrResult || hasRuntimeFailure, "[XR][%s] %s failed", module, call);
  return xrResult;
}

bool report_xr_result_with_message(XrInstance oxr_instance, XrResult result, const char *call, const char *module,
  bool *is_runtime_failure, const char *message, ...)
{
  if (!report_xr_result_base(oxr_instance, result, call, module, is_runtime_failure) && message)
  {
    if (is_runtime_failure && *is_runtime_failure)
      return false;

    va_list arguments;
    va_start(arguments, message);

    eastl::string resolvedMessage;
    resolvedMessage.sprintf_va_list(message, arguments);

    va_end(arguments);

    logerr("[XR][%s] %s", module, resolvedMessage.c_str());

    G_ASSERTF(false, "[XR][%s] %s", module, resolvedMessage.c_str());

    return false;
  }

  return true;
}

bool runtime_check(XrResult result, const char *function, const char *module)
{
  if (XR_SUCCEEDED(result))
    return true;

  report_xr_result(0, result, function, module, nullptr);

  switch (result)
  {
    case XR_ERROR_RUNTIME_FAILURE:
      ::os_message_box(::get_localized_text("xr/runtime_failure", "The OpenXR runtime has failed to initialize properly."
                                                                  "\nThe game will launch on flatscreen now."),
        ::get_localized_text("msgbox/error_header"));
      break;
    case XR_ERROR_LIMIT_REACHED:
      ::os_message_box(::get_localized_text("xr/runtime_limit", "The OpenXR runtime can't start a new VR application. "
                                                                "Please close other VR applications before trying again."
                                                                "\nThe game will launch on flatscreen now."),
        ::get_localized_text("msgbox/error_header"));
      break;
    case XR_ERROR_RUNTIME_UNAVAILABLE:
      ::os_message_box(::get_localized_text("xr/runtime_unavailable", "No OpenXR runtime is detected. "
                                                                      "Please make sure that your system has a working OpenXR runtime."
                                                                      "\nThe game will launch on flatscreen now."),
        ::get_localized_text("msgbox/error_header"));
      break;
    default: break;
  }

  return false;
}
