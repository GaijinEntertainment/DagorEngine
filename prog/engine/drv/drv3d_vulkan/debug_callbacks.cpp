// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "debug_callbacks.h"
#include "globals.h"
#include "driver_config.h"
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>

using namespace drv3d_vulkan;

#if VK_EXT_debug_report
VkBool32 VKAPI_PTR DebugCallbacks::msgCallbackReport(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
  uint64_t object, size_t location, int32_t messageCode, const char *pLayerPrefix, const char *pMessage, void *pUserData)
{
  (void)objectType;
  (void)object;
  (void)location;
  (void)pLayerPrefix;
  (void)pMessage;
  (void)pUserData;
  if (find_value_idx(Globals::Dbg::callbacks.suppressedMessage, messageCode) != -1)
    return VK_FALSE;
  if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
  {
    if ((objectType <= VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT) || !Globals::cfg.bits.allowAssertOnValidationFail)
    {
      D3D_ERROR("vulkan: Error: (objectType=%u, object=0x%X, location=%u, errorCode=%u) %s: %s", objectType, object, location,
        messageCode, pLayerPrefix, pMessage);
    }
    else
    {
      G_ASSERTF(0, "vulkan: Error: (objectType=%u, object=0x%X, location=%u, errorCode=%u) %s: %s", objectType, object, location,
        messageCode, pLayerPrefix, pMessage);
    }
  }
  else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
  {
    debug("vulkan: Perf Warning: (objectType=%u, object=0x%X, location=%u, errorCode=%u) %s: %s", objectType, object, location,
      messageCode, pLayerPrefix, pMessage);
  }
  else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
  {
    debug("vulkan: Warning: (objectType=%u, object=0x%X, location=%u, errorCode=%u) %s: %s", objectType, object, location, messageCode,
      pLayerPrefix, pMessage);
  }
  else if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
  {
    debug("vulkan: Debug: (objectType=%u, object=0x%X, location=%u, errorCode=%u) %s: %s", objectType, object, location, messageCode,
      pLayerPrefix, pMessage);
  }
  else
  {
    debug("vulkan: Info: (objectType=%u, object=0x%X, location=%u, errorCode=%u) %s: %s", objectType, object, location, messageCode,
      pLayerPrefix, pMessage);
  }
  return VK_FALSE;
}
#endif

#if VK_EXT_debug_utils
VkBool32 VKAPI_PTR DebugCallbacks::msgCallbackUtils(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
{
  G_UNUSED(messageSeverity);
  G_UNUSED(messageType);
  G_UNUSED(pCallbackData);
  G_UNUSED(pUserData);
  if (!pCallbackData)
  {
    D3D_ERROR("VK_EXT_debug_utils error: pCallbackData was nullptr!");
    return VK_FALSE;
  }

  if (find_value_idx(Globals::Dbg::callbacks.suppressedMessage, pCallbackData->messageIdNumber) != -1)
    return VK_FALSE;

  enum class ValidationAction
  {
    DO_ASSERT,
    DO_LOG_ERROR,
    DO_LOG_DEBUG,
  };

  ValidationAction action = ValidationAction::DO_LOG_DEBUG;
  const char *serverityName = "Error";
  const char *typeName = "Validation";

  if (VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT == messageSeverity)
  {
    serverityName = "Error";
    if (VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT == messageType)
    {
      action = ValidationAction::DO_ASSERT;
    }
    else if (VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT == messageType)
    {
      action = ValidationAction::DO_LOG_ERROR;
    }
    else if (VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT == messageType)
    {
      action = ValidationAction::DO_LOG_DEBUG; // -V1048
    }
  }
  else if (VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT == messageSeverity)
  {
    serverityName = "Warning";
    action = ValidationAction::DO_LOG_DEBUG; // -V1048
  }
  else if (VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT == messageSeverity)
  {
    serverityName = "Info";
    action = ValidationAction::DO_LOG_DEBUG; // -V1048
  }
  else if (VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT == messageSeverity)
  {
    serverityName = "Verbose";
    action = ValidationAction::DO_LOG_DEBUG; // -V1048
  }
  if (VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT == messageType)
  {
    typeName = "Validation";
  }
  else if (VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT == messageType)
  {
    typeName = "Performance";
  }
  else if (VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT == messageType)
  {
    typeName = "General";
  }

#if VULKAN_VALIDATION_COLLECT_CALLER > 0
  if (ExecutionContext::dbg_get_active_instance())
  {
    debug("vulkan: triggered in execution context by %s", ExecutionContext::dbg_get_active_instance()->getCurrentCmdCaller());
  }
#endif

  String message(2048, "vulkan: %s: %s: %s (%u): %s", typeName, serverityName, pCallbackData->pMessageIdName,
    pCallbackData->messageIdNumber, pCallbackData->pMessage);
  // TODO: use the extended data of pCallbackData to provide more information

  switch (action)
  {
    case ValidationAction::DO_ASSERT:
    {
      if (Globals::cfg.bits.allowAssertOnValidationFail)
      {
        drv3d_vulkan::generateFaultReport();
        G_ASSERTF(!"VULKAN VALIDATION FAILED", "%s", message);
      }
      else
        D3D_ERROR("%s", message);
    }
    break;
    case ValidationAction::DO_LOG_ERROR:
    {
      D3D_ERROR("%s", message);
    }
    break;
    case ValidationAction::DO_LOG_DEBUG:
    {
      debug("%s", message);
    }
    break;
  }
  return VK_FALSE;
}
#endif

#if VK_EXT_debug_report
VulkanDebugReportCallbackEXTHandle create_debug_sink_report(int debug_level, PFN_vkDebugReportCallbackEXT clb)
{
  VkDebugReportCallbackCreateInfoEXT drcci;
  drcci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
  drcci.pNext = nullptr;
  drcci.pfnCallback = clb;
  drcci.pUserData = nullptr;
  drcci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT;

  if (debug_level > 1)
    drcci.flags |= VK_DEBUG_REPORT_WARNING_BIT_EXT;
  if (debug_level > 2)
    drcci.flags |= VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
  if (debug_level > 3)
    drcci.flags |= VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
  if (debug_level > 4)
    drcci.flags |= VK_DEBUG_REPORT_DEBUG_BIT_EXT;

  VulkanDebugReportCallbackEXTHandle result;
  VULKAN_LOG_CALL(Globals::VK::inst.vkCreateDebugReportCallbackEXT(Globals::VK::inst.get(), &drcci, nullptr, ptr(result)));
  return result;
}
#endif

#if VK_EXT_debug_utils
VulkanDebugUtilsMessengerEXTHandle create_debug_sink_utils(int debug_level, PFN_vkDebugUtilsMessengerCallbackEXT clb)
{
  VkDebugUtilsMessengerCreateInfoEXT dumci{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
  dumci.flags = 0;
  dumci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  dumci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
  dumci.pfnUserCallback = clb;
  dumci.pUserData = nullptr;
  if (debug_level > 1)
  {
    dumci.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
  }
  if (debug_level > 2)
  {
    dumci.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  }
  if (debug_level > 3)
  {
    dumci.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
  }
  if (debug_level > 4)
  {
    dumci.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
    dumci.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
  }
  VulkanDebugUtilsMessengerEXTHandle result;
  VULKAN_LOG_CALL(Globals::VK::inst.vkCreateDebugUtilsMessengerEXT(Globals::VK::inst.get(), &dumci, nullptr, ptr(result)));
  return result;
}
#endif

#if VK_EXT_debug_report || VK_EXT_debug_utils
// compiles 2 ways to define a message code supression mechanism
// ignoreMessageCodes:t=<comma seperated list>
// ignoreMessageCode:i=<id>
// ignoreMessageCode can have multiple entries (currently they are merged away by other code
// so the work around with ignoreMessageCodes
Tab<int32_t> compile_supressed_message_set(const DataBlock *config)
{
  Tab<int32_t> result;
  const char *list = config->getStr("ignoreMessageCodes", "");
  if (list[0])
  {
    for (;;)
    {
      int32_t i = atoi(list);
      result.push_back(i);
      list = strstr(list, ",");
      if (!list)
        break;
      ++list; // skip over ,
      // leading spaces for atoi is ok
    }
  }

  int nameId = config->getNameId("ignoreMessageCode");
  int paramId = config->findParam(nameId);
  while (paramId != -1)
  {
    result.push_back(config->getInt(paramId));
    paramId = config->findParam(nameId, paramId);
  }
  return result;
}
#endif

void DebugCallbacks::init()
{
  if (Globals::cfg.debugLevel <= 0)
    return;

#if VK_EXT_debug_report || VK_EXT_debug_utils
  const DataBlock *vkCfg = ::dgs_get_settings()->getBlockByNameEx("vulkan");
  suppressedMessage = compile_supressed_message_set(vkCfg);
#endif

#if VK_EXT_debug_report
  if (Globals::VK::inst.hasExtension<DebugReport>())
    debugHandleReport = create_debug_sink_report(Globals::cfg.debugLevel, msgCallbackReport);
#endif
#if VK_EXT_debug_utils
  if (Globals::VK::inst.hasExtension<DebugUtilsEXT>())
    debugHandleUtils = create_debug_sink_utils(Globals::cfg.debugLevel, msgCallbackUtils);
#endif
}

void DebugCallbacks::shutdown()
{
#if VK_EXT_debug_report
  if (!is_null(debugHandleReport))
  {
    VULKAN_LOG_CALL(Globals::VK::inst.vkDestroyDebugReportCallbackEXT(Globals::VK::inst.get(), debugHandleReport, nullptr));
    debugHandleReport = VulkanNullHandle();
  }
#endif
#if VK_EXT_debug_utils
  if (!is_null(debugHandleUtils))
  {
    VULKAN_LOG_CALL(Globals::VK::inst.vkDestroyDebugUtilsMessengerEXT(Globals::VK::inst.get(), debugHandleUtils, nullptr));
    debugHandleUtils = VulkanNullHandle();
  }
#endif
}
