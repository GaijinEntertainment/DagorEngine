#pragma once
#include "vulkan_device.h"
#include "vulkan_instance.h"
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>

namespace drv3d_vulkan
{
struct PhysicalDeviceSet
{
  VulkanPhysicalDeviceHandle device;
  VkPhysicalDeviceFeatures features{};
  VkPhysicalDeviceProperties properties{};
  eastl::vector<VkQueueFamilyProperties> queueFamilyProperties;
  carray<VkFormatProperties, VK_FORMAT_RANGE_SIZE> formatProperties{}; //-V730_NOINIT
  VkPhysicalDeviceMemoryProperties memoryProperties{};
  VkPhysicalDeviceSubgroupProperties subgroupProperties{};
  eastl::vector<VkExtensionProperties> extensions;

#if VK_KHR_imageless_framebuffer
  VkPhysicalDeviceImagelessFramebufferFeaturesKHR imagelessFramebufferFeature = //
    {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGELESS_FRAMEBUFFER_FEATURES_KHR, nullptr, false};
#endif

#if VK_EXT_conditional_rendering
  VkPhysicalDeviceConditionalRenderingFeaturesEXT conditionalRenderingFeature = //
    {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONDITIONAL_RENDERING_FEATURES_EXT, nullptr, false, false};
#endif

#if VK_EXT_memory_budget
  VkPhysicalDeviceMemoryBudgetPropertiesEXT memoryBudgetInfo = //
    {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT, nullptr};
#endif

  bool memoryBudgetInfoAvailable = false;
  uint32_t deviceLocalHeapSizeKb = 0;
  uint32_t deviceLocalMemoryBudget = 0;

#if VK_EXT_device_memory_report
  VkPhysicalDeviceDeviceMemoryReportFeaturesEXT deviceMemoryReportFeature = //
    {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_MEMORY_REPORT_FEATURES_EXT, nullptr, false};
#endif

#if VK_KHR_buffer_device_address
  VkPhysicalDeviceBufferDeviceAddressFeaturesKHR deviceBufferDeviceAddressFeature = //
    {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR, nullptr, true};
#endif

#if VK_KHR_ray_tracing_pipeline
  VkPhysicalDeviceRayTracingPipelineFeaturesKHR deviceRayTracingPipelineFeature = //
    {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR, nullptr, true, false, false, true};
#endif

#if VK_KHR_ray_query
  VkPhysicalDeviceRayQueryFeaturesKHR deviceRayQueryFeature = //
    {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR, nullptr, true};
#endif

#if VK_KHR_acceleration_structure
  VkPhysicalDeviceAccelerationStructureFeaturesKHR deviceAccelerationStructureFeature = //
    {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR, nullptr, true};
#endif

#if VK_EXT_device_fault
  VkPhysicalDeviceFaultFeaturesEXT deviceFaultFeature = //
    {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FAULT_FEATURES_EXT, nullptr, false, false};
#endif

#if VK_EXT_descriptor_indexing
  VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures{
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT, nullptr};
#endif

#if VK_KHR_shader_float16_int8
  VkPhysicalDeviceShaderFloat16Int8Features shaderFloat16Int8Features = //
    {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES, nullptr, false, false};
#endif

  bool hasDevProps2 = false;

  bool hasConditionalRender = false;
  bool hasImagelessFramebuffer = false;
  bool hasDeviceMemoryReport = false;
  bool hasDeviceBufferDeviceAddress = false;
  bool hasRayTracingPipeline = false;
  bool hasRayQuery = false;
  bool hasAccelerationStructure = false;
  bool hasDeviceFaultFeature = false;
  bool hasDeviceFaultVendorInfo = false;
  bool hasDepthStencilResolve = false;
  bool hasBindless = false;
  bool hasLazyMemory = false;
  bool hasShaderFloat16 = false;
  uint32_t maxBindlessTextures = 0;
  uint32_t maxBindlessSamplers = 0;
#if D3D_HAS_RAY_TRACING
  uint32_t raytraceShaderHeaderSize = 0;
  uint32_t raytraceMaxRecursionDepth = 0;
  uint32_t raytraceTopAccelerationInstanceElementSize = 0;
#endif
#if VK_KHR_driver_properties
  VkPhysicalDeviceDriverPropertiesKHR driverProps = // init it to something safe
    {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES_KHR, nullptr, VK_DRIVER_ID_MAX_ENUM, "<unknown>", "<unknown>", {0, 0, 0, 0}};
#endif
#if VK_KHR_depth_stencil_resolve
  VkPhysicalDeviceDepthStencilResolveProperties depthStencilResolveProps = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_STENCIL_RESOLVE_PROPERTIES, nullptr,
    VK_RESOLVE_MODE_SAMPLE_ZERO_BIT, // supportedDepthResolveModes
    VK_RESOLVE_MODE_SAMPLE_ZERO_BIT, // supportedStencilResolveModes
    VK_FALSE,                        // independentResolveNone
    VK_FALSE};                       // independentResolve
#endif
#if VK_EXT_descriptor_indexing
  VkPhysicalDeviceDescriptorIndexingProperties descriptorIndexingProps = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES, nullptr};
#endif
  uint32_t vendorId = D3D_VENDOR_NONE;
  char vendorName[16] = {};
  uint32_t warpSize = 32;
  uint32_t score = 0;
  uint32_t driverVersionDecoded[4] = {};

  uint32_t deviceTypeScoreMultiplier()
  {
    switch (properties.deviceType)
    {
      case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return 6;
      case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: return 5;
      case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return 4;
      case VK_PHYSICAL_DEVICE_TYPE_OTHER: return 3;
      case VK_PHYSICAL_DEVICE_TYPE_CPU: return 2;
      default: return 1;
    }
  }

  inline void calculateScore()
  {
    score = 0;
#define VULKAN_FEATURE_REQUIRE(name) \
  if (features.name == VK_FALSE)     \
    score = 0;
#define VULKAN_FEATURE_DISABLE(name)
#define VULKAN_FEATURE_OPTIONAL(name) \
  if (features.name == VK_TRUE)       \
    ++score;
    VULKAN_FEATURESET_DISABLED_LIST
    VULKAN_FEATURESET_OPTIONAL_LIST
    VULKAN_FEATURESET_REQUIRED_LIST
#undef VULKAN_FEATURE_REQUIRE
#undef VULKAN_FEATURE_DISABLE
#undef VULKAN_FEATURE_OPTIONAL
    // score <<= 24;
    // TODO: add max tex size, queue stuff
    score *= deviceTypeScoreMultiplier();
  }

  void detectDeviceVendor()
  {
    vendorId = d3d_get_vendor(properties.vendorID);
    strcpy(vendorName, d3d_get_vendor_name(vendorId));
  }

  void guessWarpSize()
  {
    if (vendorId == D3D_VENDOR_ATI)
      warpSize = 64;
  }

  template <typename T>
  bool hasExtension() const
  {
    return end(extensions) != eastl::find_if(begin(extensions), end(extensions),
                                [=](const VkExtensionProperties &ep) { return 0 == strcmp(extension_name<T>(), ep.extensionName); });
  }

  template <typename T>
  void disableExtension()
  {
    extensions.erase(eastl::find_if(begin(extensions), end(extensions),
      [=](const VkExtensionProperties &ep) { return 0 == strcmp(extension_name<T>(), ep.extensionName); }));
  }

  void removeInstanceIncompatibleExtensions(VulkanInstance &instance)
  {
    if (hasExtension<DebugMarkerEXT>() && !instance.hasExtension<DebugReport>())
      disableExtension<DebugMarkerEXT>();
  }

  bool queryExtensionList(VulkanInstance &instance)
  {
    uint32_t numExtensions = 0;
    VkResult rc = VK_INCOMPLETE;
    while (VK_INCOMPLETE == rc)
    {
      VULKAN_LOG_CALL(instance.vkEnumerateDeviceExtensionProperties(device, nullptr, &numExtensions, nullptr));
      extensions.resize(numExtensions);
      rc = VULKAN_LOG_CALL_R(instance.vkEnumerateDeviceExtensionProperties(device, nullptr, &numExtensions, extensions.data()));
      extensions.resize(numExtensions);
    }

    bool ret = VULKAN_OK(rc);

    if (ret)
      removeInstanceIncompatibleExtensions(instance);

    return ret;
  }

  template <typename T>
  void chainExtendedFeaturesInfoStructs(T &target)
  {
#if VK_KHR_imageless_framebuffer
    if (hasExtension<ImagelessFramebufferKHR>())
    {
      chain_structs(target, imagelessFramebufferFeature);
    }
#endif

#if VK_EXT_conditional_rendering
    if (hasExtension<ConditionalRenderingEXT>())
    {
      chain_structs(target, conditionalRenderingFeature);
    }
#endif

#if VK_EXT_device_memory_report
    if (hasExtension<DeviceMemoryReportEXT>())
    {
      chain_structs(target, deviceMemoryReportFeature);
    }
#endif

#if VK_KHR_buffer_device_address
    if (hasExtension<BufferDeviceAddressKHR>())
    {
      chain_structs(target, deviceBufferDeviceAddressFeature);
    }
#endif

#if VK_KHR_ray_tracing_pipeline
    if (hasExtension<RaytracingPipelineKHR>())
    {
      chain_structs(target, deviceRayTracingPipelineFeature);
    }
#endif

#if VK_KHR_ray_query
    if (hasExtension<RayQueryKHR>())
    {
      chain_structs(target, deviceRayQueryFeature);
    }
#endif

#if VK_KHR_acceleration_structure
    if (hasExtension<AccelerationStructureKHR>())
    {
      chain_structs(target, deviceAccelerationStructureFeature);
    }
#endif

#if VK_EXT_device_fault
    if (hasExtension<DeviceFaultEXT>())
    {
      chain_structs(target, deviceFaultFeature);
    }
#endif

#if VK_EXT_descriptor_indexing
    if (hasExtension<DescriptorIndexingEXT>())
    {
      chain_structs(target, indexingFeatures);
    }
#endif

#if VK_KHR_shader_float16_int8
    if (hasExtension<ShaderFloat16Int8KHR>())
    {
      chain_structs(target, shaderFloat16Int8Features);
    }
#endif
  }

  uint32_t getAvailableVideoMemoryKb() const { return deviceLocalHeapSizeKb; }

  uint32_t calculateTotalAvailableDeviceLocalMemoryKb()
  {
    uint32_t ret = 0;
    for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; ++i)
    {
      if (memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
      {
        uint32_t availableKb = 0;
#if VK_EXT_memory_budget
        if (memoryBudgetInfoAvailable)
        {
          // sanity check as spec says "heap's usage is an estimate"
          VkDeviceSize budget = memoryBudgetInfo.heapBudget[i];

#if _TARGET_PC_LINUX
          // report less memory as vulkan drivers can't swap memory
          // between processes and having any light load on system (any browser)
          // will generate OOM for us
          // TODO: this should be removed when residency will be in place
          budget = (budget * 80) / 100;
#endif
          // apply soft limit from config
          int64_t softLimitMb =
            ::dgs_get_settings()->getBlockByNameEx("vulkan")->getBlockByNameEx(String(64, "heap%u", i))->getInt64("softLimitMb", -1);
          if (softLimitMb != -1)
            budget = softLimitMb << 20;

          if (budget < memoryBudgetInfo.heapUsage[i])
            availableKb = 0;
          else
            availableKb = (budget - memoryBudgetInfo.heapUsage[i]) >> 10;
        }
        else
#endif
        {
          availableKb = memoryProperties.memoryHeaps[i].size >> 10;
        }
        // usually we have heap 0 and heap 2 device local heaps
        // and they are interleaved (i.e. allocation in 2 will count in 0 too)
        // so just find out biggest heap and use it as available memory
        ret = max<uint32_t>(ret, availableKb);
      }
    }

    return ret;
  }

  uint32_t getCurrentAvailableMemoryKb(VulkanInstance &instance)
  {
#if VK_EXT_memory_budget
    if (!memoryBudgetInfoAvailable)
      return 0;

    VkPhysicalDeviceMemoryProperties2KHR pdmp = //
      {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2_KHR, nullptr};

    memoryBudgetInfo.pNext = nullptr;
    chain_structs(pdmp, memoryBudgetInfo);

    VULKAN_LOG_CALL(instance.vkGetPhysicalDeviceMemoryProperties2KHR(device, &pdmp));

    return calculateTotalAvailableDeviceLocalMemoryKb();
#else
    return 0;
#endif
  }

  void enableExtendedFeatures(VkDeviceCreateInfo &create_info, VkPhysicalDeviceFeatures2 &features2)
  {
    if (!hasDevProps2)
      return;

    features2.features = *create_info.pEnabledFeatures;
    create_info.pEnabledFeatures = nullptr;
    chainExtendedFeaturesInfoStructs(create_info);
    chain_structs(create_info, features2);
  }

  void processExtendedFeaturesQueryResult()
  {
#if VK_EXT_conditional_rendering
    if (hasExtension<ConditionalRenderingEXT>())
    {
      hasConditionalRender = conditionalRenderingFeature.conditionalRendering != VK_FALSE;
      conditionalRenderingFeature.pNext = nullptr;
    }
#endif

#if VK_KHR_imageless_framebuffer
    if (hasExtension<ImagelessFramebufferKHR>() && hasExtension<ImageFormatListKHR>())
    {
      hasImagelessFramebuffer = imagelessFramebufferFeature.imagelessFramebuffer == VK_TRUE;
      imagelessFramebufferFeature.pNext = nullptr;
    }
#endif

#if VK_EXT_device_memory_report
    if (hasExtension<DeviceMemoryReportEXT>())
    {
      hasDeviceMemoryReport = deviceMemoryReportFeature.deviceMemoryReport == VK_TRUE;
      deviceMemoryReportFeature.pNext = nullptr;
    }
#endif

#if VK_KHR_buffer_device_address
    if (hasExtension<BufferDeviceAddressKHR>())
    {
      hasDeviceBufferDeviceAddress = deviceBufferDeviceAddressFeature.bufferDeviceAddress == VK_TRUE;
      deviceBufferDeviceAddressFeature.pNext = nullptr;
    }
#endif

#if VK_KHR_ray_tracing_pipeline
    if (hasExtension<RaytracingPipelineKHR>())
    {
      hasRayTracingPipeline = deviceRayTracingPipelineFeature.rayTracingPipeline == VK_TRUE &&
                              deviceRayTracingPipelineFeature.rayTracingPipelineTraceRaysIndirect == VK_TRUE;
      deviceRayTracingPipelineFeature.pNext = nullptr;
    }
#endif

#if VK_KHR_ray_query
    if (hasExtension<RayQueryKHR>())
    {
      hasRayQuery = deviceRayQueryFeature.rayQuery == VK_TRUE;
      deviceRayQueryFeature.pNext = nullptr;
    }
#endif

#if VK_KHR_acceleration_structure
    if (hasExtension<AccelerationStructureKHR>())
    {
      hasAccelerationStructure = deviceAccelerationStructureFeature.accelerationStructure == VK_TRUE;
      deviceAccelerationStructureFeature.pNext = nullptr;
    }
#endif

#if VK_EXT_device_fault
    if (hasExtension<DeviceFaultEXT>())
    {
      hasDeviceFaultFeature = deviceFaultFeature.deviceFault;
      hasDeviceFaultVendorInfo = deviceFaultFeature.deviceFaultVendorBinary;
      deviceFaultFeature.pNext = nullptr;
    }
#endif

#if VK_EXT_descriptor_indexing
    if (hasExtension<DescriptorIndexingEXT>())
    {
      hasBindless = indexingFeatures.descriptorBindingPartiallyBound && indexingFeatures.runtimeDescriptorArray;
      indexingFeatures.pNext = nullptr;
    }
#endif

#if VK_KHR_shader_float16_int8
    if (hasExtension<ShaderFloat16Int8KHR>())
    {
      hasShaderFloat16 = shaderFloat16Int8Features.shaderFloat16;
      shaderFloat16Int8Features.pNext = nullptr;
    }
#endif
  }

  template <class TargetExt, class DepExt>
  bool checkExtensionDepencency()
  {
    bool hasExt = hasExtension<DepExt>();
    if (!hasExt)
    {
      debug("vulkan: extension %s cannot be used: depencency %s is not available", extension_name<TargetExt>(),
        extension_name<DepExt>());
    }
    return hasExt;
  }

  bool initExt(VulkanInstance &instance)
  {
    if (!instance.hasExtension<GetPhysicalDeviceProperties2KHR>())
      return false;
    hasDevProps2 = true;
    VkPhysicalDeviceFeatures2KHR pdf = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR, nullptr};

    chainExtendedFeaturesInfoStructs(pdf);

    VULKAN_LOG_CALL(instance.vkGetPhysicalDeviceFeatures2KHR(device, &pdf));
    features = pdf.features;

    processExtendedFeaturesQueryResult();

    VkPhysicalDeviceProperties2KHR pdp = //
      {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR, nullptr};

#if VK_KHR_driver_properties
    if (hasExtension<DriverPropertiesKHR>())
      chain_structs(pdp, driverProps);
#endif

#if VK_KHR_depth_stencil_resolve
    {
      bool depthStencilResolvePresent = checkExtensionDepencency<DepthStencilResolveKHR, DepthStencilResolveKHR>();
      hasDepthStencilResolve = depthStencilResolvePresent;
      hasDepthStencilResolve = hasDepthStencilResolve && checkExtensionDepencency<DepthStencilResolveKHR, MultiviewKHR>();
      hasDepthStencilResolve = hasDepthStencilResolve && checkExtensionDepencency<DepthStencilResolveKHR, CreateRenderPass2KHR>();
      hasDepthStencilResolve = hasDepthStencilResolve && checkExtensionDepencency<DepthStencilResolveKHR, Maintenance2KHR>();
      if (hasDepthStencilResolve)
      {
        debug("vulkan: extension %s is available", extension_name<DepthStencilResolveKHR>());
        chain_structs(pdp, depthStencilResolveProps);
      }
      else if (depthStencilResolvePresent)
      {
        debug("vulkan: disabling extension %s", extension_name<DepthStencilResolveKHR>());
        disableExtension<DepthStencilResolveKHR>();
      }
    }
#endif

    VkPhysicalDeviceSubgroupProperties sgdp{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES};
    // query subgroup props only when API allows it
    if (instance.getAPIVersion() >= VK_API_VERSION_1_1)
      chain_structs(pdp, sgdp);

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR pdrtpp = //
      {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
#if VK_KHR_ray_tracing_pipeline
    if (hasExtension<RaytracingPipelineKHR>())
      chain_structs(pdp, pdrtpp);
#endif

#if VK_EXT_descriptor_indexing
    if (hasExtension<DescriptorIndexingEXT>())
      chain_structs(pdp, descriptorIndexingProps);
#endif

    VULKAN_LOG_CALL(instance.vkGetPhysicalDeviceProperties2KHR(device, &pdp));
    properties = pdp.properties;

#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
    if (hasExtension<RaytracingPipelineKHR>() || hasExtension<RayQueryKHR>())
    {
      raytraceMaxRecursionDepth = pdrtpp.maxRayRecursionDepth;
      raytraceTopAccelerationInstanceElementSize = sizeof(VkAccelerationStructureInstanceKHR);
    }
#endif

#if VK_EXT_descriptor_indexing
    if (hasExtension<DescriptorIndexingEXT>())
    {
      maxBindlessTextures = descriptorIndexingProps.maxDescriptorSetUpdateAfterBindSampledImages;
      maxBindlessSamplers = descriptorIndexingProps.maxDescriptorSetUpdateAfterBindSamplers;
    }
#endif

    subgroupProperties = sgdp;

    {
      uint32_t qfpc = 0;
      VULKAN_LOG_CALL(instance.vkGetPhysicalDeviceQueueFamilyProperties2KHR(device, &qfpc, nullptr));
      eastl::vector<VkQueueFamilyProperties2KHR> qf;
      qf.resize(qfpc);
      for (auto &&info : qf)
      {
        info.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2_KHR;
        info.pNext = nullptr;
      }
      VULKAN_LOG_CALL(instance.vkGetPhysicalDeviceQueueFamilyProperties2KHR(device, &qfpc, qf.data()));
      qf.resize(qfpc);
      queueFamilyProperties.reserve(qfpc);
      for (auto &&info : qf)
        queueFamilyProperties.push_back(info.queueFamilyProperties);
    }

    VkPhysicalDeviceMemoryProperties2KHR pdmp = //
      {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2_KHR, nullptr};

#if VK_EXT_memory_budget
    if (hasExtension<MemoryBudgetEXT>())
    {
      chain_structs(pdmp, memoryBudgetInfo);
      memoryBudgetInfoAvailable = true;
    }
#endif

    VULKAN_LOG_CALL(instance.vkGetPhysicalDeviceMemoryProperties2KHR(device, &pdmp));
    memoryProperties = pdmp.memoryProperties;

    VkFormatProperties2KHR fp = {VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2_KHR, nullptr};
    for (uint32_t i = 0; i < formatProperties.size(); ++i)
    {
      VULKAN_LOG_CALL(instance.vkGetPhysicalDeviceFormatProperties2KHR(device, (VkFormat)i, &fp));
      formatProperties[i] = fp.formatProperties;
    }

    deviceLocalHeapSizeKb = calculateTotalAvailableDeviceLocalMemoryKb();

    return true;
  }

  void initUnextended(VulkanInstance &instance)
  {
    VULKAN_LOG_CALL(instance.vkGetPhysicalDeviceFeatures(device, &features));
    VULKAN_LOG_CALL(instance.vkGetPhysicalDeviceProperties(device, &properties));
    uint32_t qfpc = 0;
    VULKAN_LOG_CALL(instance.vkGetPhysicalDeviceQueueFamilyProperties(device, &qfpc, nullptr));
    queueFamilyProperties.resize(qfpc);
    VULKAN_LOG_CALL(instance.vkGetPhysicalDeviceQueueFamilyProperties(device, &qfpc, queueFamilyProperties.data()));
    queueFamilyProperties.resize(qfpc);
    VULKAN_LOG_CALL(instance.vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties));
    for (uint32_t i = 0; i < formatProperties.size(); ++i)
      VULKAN_LOG_CALL(instance.vkGetPhysicalDeviceFormatProperties(device, (VkFormat)i, &formatProperties[i]));
  }

  void decodeDriverVersion()
  {
    // https://www.reddit.com/r/vulkan/comments/fmift4/how_to_decode_driverversion_field_of
    switch (vendorId)
    {
      case D3D_VENDOR_NVIDIA:
        driverVersionDecoded[0] = (properties.driverVersion >> 22) & 0x3ff;
        driverVersionDecoded[1] = (properties.driverVersion >> 14) & 0x0ff;
        driverVersionDecoded[2] = (properties.driverVersion >> 6) & 0x0ff;
        driverVersionDecoded[3] = properties.driverVersion & 0x003f;
        break;

      case D3D_VENDOR_AMD:
        driverVersionDecoded[0] = (properties.driverVersion >> 22) & 0x3ff;
        driverVersionDecoded[1] = (properties.driverVersion >> 12) & 0x3ff;
        driverVersionDecoded[2] = properties.driverVersion & 0x0fff;
        driverVersionDecoded[3] = 0;
        break;

      case D3D_VENDOR_INTEL:
        driverVersionDecoded[0] = (properties.driverVersion >> 14) & 0x0ff;
        driverVersionDecoded[1] = properties.driverVersion & 0x3fff;
        driverVersionDecoded[2] = 0;
        driverVersionDecoded[3] = 0;
        break;

      default:
        driverVersionDecoded[0] = VK_VERSION_MAJOR(properties.driverVersion);
        driverVersionDecoded[1] = VK_VERSION_MINOR(properties.driverVersion);
        driverVersionDecoded[2] = VK_VERSION_PATCH(properties.driverVersion);
        driverVersionDecoded[3] = 0;
        break;
    }
  }

  void detectLazyMemorySupport()
  {
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
    {
      VkMemoryType &iter = memoryProperties.memoryTypes[i];
      VkMemoryPropertyFlags lazyFlag = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
      // check for device resident lazily memory (just for sanity)
      if ((iter.propertyFlags & lazyFlag) == lazyFlag)
      {
        hasLazyMemory = true;
        return;
      }
    }
  }

  void init(VulkanInstance &instance, VulkanPhysicalDeviceHandle pd)
  {
    device = pd;
    queryExtensionList(instance);
    if (!initExt(instance))
      initUnextended(instance);
    detectLazyMemorySupport();
    detectDeviceVendor();
    decodeDriverVersion();
    guessWarpSize();
    calculateScore();
  }

  static inline const char *bool32ToStr(VkBool32 b) { return b ? "yes" : "no"; }

  static inline void print(const VkPhysicalDeviceFeatures &DAGOR_HAS_LOGS(set))
  {
    debug("robustBufferAccess: %s", bool32ToStr(set.robustBufferAccess));
    debug("fullDrawIndexUint32: %s", bool32ToStr(set.fullDrawIndexUint32));
    debug("imageCubeArray: %s", bool32ToStr(set.imageCubeArray));
    debug("independentBlend: %s", bool32ToStr(set.independentBlend));
    debug("geometryShader: %s", bool32ToStr(set.geometryShader));
    debug("tessellationShader: %s", bool32ToStr(set.tessellationShader));
    debug("sampleRateShading: %s", bool32ToStr(set.sampleRateShading));
    debug("dualSrcBlend: %s", bool32ToStr(set.dualSrcBlend));
    debug("logicOp: %s", bool32ToStr(set.logicOp));
    debug("multiDrawIndirect: %s", bool32ToStr(set.multiDrawIndirect));
    debug("drawIndirectFirstInstance: %s", bool32ToStr(set.drawIndirectFirstInstance));
    debug("depthClamp: %s", bool32ToStr(set.depthClamp));
    debug("depthBiasClamp: %s", bool32ToStr(set.depthBiasClamp));
    debug("fillModeNonSolid: %s", bool32ToStr(set.fillModeNonSolid));
    debug("depthBounds: %s", bool32ToStr(set.depthBounds));
    debug("wideLines: %s", bool32ToStr(set.wideLines));
    debug("largePoints: %s", bool32ToStr(set.largePoints));
    debug("alphaToOne: %s", bool32ToStr(set.alphaToOne));
    debug("multiViewport: %s", bool32ToStr(set.multiViewport));
    debug("samplerAnisotropy: %s", bool32ToStr(set.samplerAnisotropy));
    debug("textureCompressionETC2: %s", bool32ToStr(set.textureCompressionETC2));
    debug("textureCompressionASTC_LDR: %s", bool32ToStr(set.textureCompressionASTC_LDR));
    debug("textureCompressionBC: %s", bool32ToStr(set.textureCompressionBC));
    debug("occlusionQueryPrecise: %s", bool32ToStr(set.occlusionQueryPrecise));
    debug("pipelineStatisticsQuery: %s", bool32ToStr(set.pipelineStatisticsQuery));
    debug("vertexPipelineStoresAndAtomics: %s", bool32ToStr(set.vertexPipelineStoresAndAtomics));
    debug("fragmentStoresAndAtomics: %s", bool32ToStr(set.fragmentStoresAndAtomics));
    debug("shaderTessellationAndGeometryPointSize: %s", bool32ToStr(set.shaderTessellationAndGeometryPointSize));
    debug("shaderImageGatherExtended: %s", bool32ToStr(set.shaderImageGatherExtended));
    debug("shaderStorageImageExtendedFormats: %s", bool32ToStr(set.shaderStorageImageExtendedFormats));
    debug("shaderStorageImageMultisample: %s", bool32ToStr(set.shaderStorageImageMultisample));
    debug("shaderStorageImageReadWithoutFormat: %s", bool32ToStr(set.shaderStorageImageReadWithoutFormat));
    debug("shaderStorageImageWriteWithoutFormat: %s", bool32ToStr(set.shaderStorageImageWriteWithoutFormat));
    debug("shaderUniformBufferArrayDynamicIndexing: %s", bool32ToStr(set.shaderUniformBufferArrayDynamicIndexing));
    debug("shaderSampledImageArrayDynamicIndexing: %s", bool32ToStr(set.shaderSampledImageArrayDynamicIndexing));
    debug("shaderStorageBufferArrayDynamicIndexing: %s", bool32ToStr(set.shaderStorageBufferArrayDynamicIndexing));
    debug("shaderStorageImageArrayDynamicIndexing: %s", bool32ToStr(set.shaderStorageImageArrayDynamicIndexing));
    debug("shaderClipDistance: %s", bool32ToStr(set.shaderClipDistance));
    debug("shaderCullDistance: %s", bool32ToStr(set.shaderCullDistance));
    debug("shaderFloat64: %s", bool32ToStr(set.shaderFloat64));
    debug("shaderInt64: %s", bool32ToStr(set.shaderInt64));
    debug("shaderInt16: %s", bool32ToStr(set.shaderInt16));
    debug("shaderResourceResidency: %s", bool32ToStr(set.shaderResourceResidency));
    debug("shaderResourceMinLod: %s", bool32ToStr(set.shaderResourceMinLod));
    debug("sparseBinding: %s", bool32ToStr(set.sparseBinding));
    debug("sparseResidencyBuffer: %s", bool32ToStr(set.sparseResidencyBuffer));
    debug("sparseResidencyImage2D: %s", bool32ToStr(set.sparseResidencyImage2D));
    debug("sparseResidencyImage3D: %s", bool32ToStr(set.sparseResidencyImage3D));
    debug("sparseResidency2Samples: %s", bool32ToStr(set.sparseResidency2Samples));
    debug("sparseResidency4Samples: %s", bool32ToStr(set.sparseResidency4Samples));
    debug("sparseResidency8Samples: %s", bool32ToStr(set.sparseResidency8Samples));
    debug("sparseResidency16Samples: %s", bool32ToStr(set.sparseResidency16Samples));
    debug("sparseResidencyAliased: %s", bool32ToStr(set.sparseResidencyAliased));
    debug("variableMultisampleRate: %s", bool32ToStr(set.variableMultisampleRate));
    debug("inheritedQueries: %s", bool32ToStr(set.inheritedQueries));
  }

  static inline const char *nameOf(VkPhysicalDeviceType type)
  {
    switch (type)
    {
      default:
      case VK_PHYSICAL_DEVICE_TYPE_OTHER: return "Other";
      case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return "Integrated GPU";
      case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return "Discrete GPU";
      case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: return "Virtual GPU";
      case VK_PHYSICAL_DEVICE_TYPE_CPU: return "CPU";
    }
  }

  static inline void print(const VkPhysicalDeviceLimits &DAGOR_HAS_LOGS(limits))
  {
    debug("maxImageDimension1D: %u", limits.maxImageDimension1D);
    debug("maxImageDimension2D: %u", limits.maxImageDimension2D);
    debug("maxImageDimension3D: %u", limits.maxImageDimension3D);
    debug("maxImageDimensionCube: %u", limits.maxImageDimensionCube);
    debug("maxImageArrayLayers: %u", limits.maxImageArrayLayers);
    debug("maxTexelBufferElements: %u", limits.maxTexelBufferElements);
    debug("maxUniformBufferRange: %u", limits.maxUniformBufferRange);
    debug("maxStorageBufferRange: %u", limits.maxStorageBufferRange);
    debug("maxPushConstantsSize: %u", limits.maxPushConstantsSize);
    debug("maxMemoryAllocationCount: %u", limits.maxMemoryAllocationCount);
    debug("maxSamplerAllocationCount: %u", limits.maxSamplerAllocationCount);
    debug("bufferImageGranularity: %u", limits.bufferImageGranularity);
    debug("sparseAddressSpaceSize: %u", limits.sparseAddressSpaceSize);
    debug("maxBoundDescriptorSets: %u", limits.maxBoundDescriptorSets);
    debug("maxPerStageDescriptorSamplers: %u", limits.maxPerStageDescriptorSamplers);
    debug("maxPerStageDescriptorUniformBuffers: %u", limits.maxPerStageDescriptorUniformBuffers);
    debug("maxPerStageDescriptorStorageBuffers: %u", limits.maxPerStageDescriptorStorageBuffers);
    debug("maxPerStageDescriptorSampledImages: %u", limits.maxPerStageDescriptorSampledImages);
    debug("maxPerStageDescriptorStorageImages: %u", limits.maxPerStageDescriptorStorageImages);
    debug("maxPerStageDescriptorInputAttachments: %u", limits.maxPerStageDescriptorInputAttachments);
    debug("maxPerStageResources: %u", limits.maxPerStageResources);
    debug("maxDescriptorSetSamplers: %u", limits.maxDescriptorSetSamplers);
    debug("maxDescriptorSetUniformBuffers: %u", limits.maxDescriptorSetUniformBuffers);
    debug("maxDescriptorSetUniformBuffersDynamic: %u", limits.maxDescriptorSetUniformBuffersDynamic);
    debug("maxDescriptorSetStorageBuffers: %u", limits.maxDescriptorSetStorageBuffers);
    debug("maxDescriptorSetStorageBuffersDynamic: %u", limits.maxDescriptorSetStorageBuffersDynamic);
    debug("maxDescriptorSetSampledImages: %u", limits.maxDescriptorSetSampledImages);
    debug("maxDescriptorSetStorageImages: %u", limits.maxDescriptorSetStorageImages);
    debug("maxDescriptorSetInputAttachments: %u", limits.maxDescriptorSetInputAttachments);
    debug("maxVertexInputAttributes: %u", limits.maxVertexInputAttributes);
    debug("maxVertexInputBindings: %u", limits.maxVertexInputBindings);
    debug("maxVertexInputAttributeOffset: %u", limits.maxVertexInputAttributeOffset);
    debug("maxVertexInputBindingStride: %u", limits.maxVertexInputBindingStride);
    debug("maxVertexOutputComponents: %u", limits.maxVertexOutputComponents);
    debug("maxTessellationGenerationLevel: %u", limits.maxTessellationGenerationLevel);
    debug("maxTessellationPatchSize: %u", limits.maxTessellationPatchSize);
    debug("maxTessellationControlPerVertexInputComponents: %u", limits.maxTessellationControlPerVertexInputComponents);
    debug("maxTessellationControlPerVertexOutputComponents: %u", limits.maxTessellationControlPerVertexOutputComponents);
    debug("maxTessellationControlPerPatchOutputComponents: %u", limits.maxTessellationControlPerPatchOutputComponents);
    debug("maxTessellationControlTotalOutputComponents: %u", limits.maxTessellationControlTotalOutputComponents);
    debug("maxTessellationEvaluationInputComponents: %u", limits.maxTessellationEvaluationInputComponents);
    debug("maxTessellationEvaluationOutputComponents: %u", limits.maxTessellationEvaluationOutputComponents);
    debug("maxGeometryShaderInvocations: %u", limits.maxGeometryShaderInvocations);
    debug("maxGeometryInputComponents: %u", limits.maxGeometryInputComponents);
    debug("maxGeometryOutputComponents: %u", limits.maxGeometryOutputComponents);
    debug("maxGeometryOutputVertices: %u", limits.maxGeometryOutputVertices);
    debug("maxGeometryTotalOutputComponents: %u", limits.maxGeometryTotalOutputComponents);
    debug("maxFragmentInputComponents: %u", limits.maxFragmentInputComponents);
    debug("maxFragmentOutputAttachments: %u", limits.maxFragmentOutputAttachments);
    debug("maxFragmentDualSrcAttachments: %u", limits.maxFragmentDualSrcAttachments);
    debug("maxFragmentCombinedOutputResources: %u", limits.maxFragmentCombinedOutputResources);
    debug("maxComputeSharedMemorySize: %u", limits.maxComputeSharedMemorySize);
    debug("maxComputeWorkGroupCount[0]: %u", limits.maxComputeWorkGroupCount[0]);
    debug("maxComputeWorkGroupCount[1]: %u", limits.maxComputeWorkGroupCount[1]);
    debug("maxComputeWorkGroupCount[2]: %u", limits.maxComputeWorkGroupCount[2]);
    debug("maxComputeWorkGroupInvocations: %u", limits.maxComputeWorkGroupInvocations);
    debug("maxComputeWorkGroupSize[0]: %u", limits.maxComputeWorkGroupSize[0]);
    debug("maxComputeWorkGroupSize[1]: %u", limits.maxComputeWorkGroupSize[1]);
    debug("maxComputeWorkGroupSize[2]: %u", limits.maxComputeWorkGroupSize[2]);
    debug("subPixelPrecisionBits: %u", limits.subPixelPrecisionBits);
    debug("subTexelPrecisionBits: %u", limits.subTexelPrecisionBits);
    debug("mipmapPrecisionBits: %u", limits.mipmapPrecisionBits);
    debug("maxDrawIndexedIndexValue: %u", limits.maxDrawIndexedIndexValue);
    debug("maxDrawIndirectCount: %u", limits.maxDrawIndirectCount);
    debug("maxSamplerLodBias: %f", limits.maxSamplerLodBias);
    debug("maxSamplerAnisotropy: %f", limits.maxSamplerAnisotropy);
    debug("maxViewports: %u", limits.maxViewports);
    debug("maxViewportDimensions[0]: %u", limits.maxViewportDimensions[0]);
    debug("maxViewportDimensions[1]: %u", limits.maxViewportDimensions[1]);
    debug("viewportBoundsRange[0]: %f", limits.viewportBoundsRange[0]);
    debug("viewportBoundsRange[1]: %f", limits.viewportBoundsRange[1]);
    debug("viewportSubPixelBits: %u", limits.viewportSubPixelBits);
    debug("minMemoryMapAlignment: %u", limits.minMemoryMapAlignment);
    debug("minTexelBufferOffsetAlignment: %u", limits.minTexelBufferOffsetAlignment);
    debug("minUniformBufferOffsetAlignment: %u", limits.minUniformBufferOffsetAlignment);
    debug("minStorageBufferOffsetAlignment: %u", limits.minStorageBufferOffsetAlignment);
    debug("minTexelOffset: %u", limits.minTexelOffset);
    debug("maxTexelOffset: %u", limits.maxTexelOffset);
    debug("minTexelGatherOffset: %u", limits.minTexelGatherOffset);
    debug("maxTexelGatherOffset: %u", limits.maxTexelGatherOffset);
    debug("minInterpolationOffset: %f", limits.minInterpolationOffset);
    debug("maxInterpolationOffset: %f", limits.maxInterpolationOffset);
    debug("subPixelInterpolationOffsetBits: %u", limits.subPixelInterpolationOffsetBits);
    debug("maxFramebufferWidth: %u", limits.maxFramebufferWidth);
    debug("maxFramebufferHeight: %u", limits.maxFramebufferHeight);
    debug("maxFramebufferLayers: %u", limits.maxFramebufferLayers);
    debug("framebufferColorSampleCounts: %u", limits.framebufferColorSampleCounts);
    debug("framebufferDepthSampleCounts: %u", limits.framebufferDepthSampleCounts);
    debug("framebufferStencilSampleCounts: %u", limits.framebufferStencilSampleCounts);
    debug("framebufferNoAttachmentsSampleCounts: %u", limits.framebufferNoAttachmentsSampleCounts);
    debug("maxColorAttachments: %u", limits.maxColorAttachments);
    debug("sampledImageColorSampleCounts: %u", limits.sampledImageColorSampleCounts);
    debug("sampledImageIntegerSampleCounts: %u", limits.sampledImageIntegerSampleCounts);
    debug("sampledImageDepthSampleCounts: %u", limits.sampledImageDepthSampleCounts);
    debug("sampledImageStencilSampleCounts: %u", limits.sampledImageStencilSampleCounts);
    debug("storageImageSampleCounts: %u", limits.storageImageSampleCounts);
    debug("maxSampleMaskWords: %u", limits.maxSampleMaskWords);
    debug("timestampComputeAndGraphics: %u", limits.timestampComputeAndGraphics);
    debug("timestampPeriod: %f", limits.timestampPeriod);
    debug("maxClipDistances: %u", limits.maxClipDistances);
    debug("maxCullDistances: %u", limits.maxCullDistances);
    debug("maxCombinedClipAndCullDistances: %u", limits.maxCombinedClipAndCullDistances);
    debug("discreteQueuePriorities: %u", limits.discreteQueuePriorities);
    debug("pointSizeRange[0]: %f", limits.pointSizeRange[0]);
    debug("pointSizeRange[1]: %f", limits.pointSizeRange[1]);
    debug("lineWidthRange[0]: %f", limits.lineWidthRange[0]);
    debug("lineWidthRange[1]: %f", limits.lineWidthRange[1]);
    debug("pointSizeGranularity: %f", limits.pointSizeGranularity);
    debug("lineWidthGranularity: %f", limits.lineWidthGranularity);
    debug("strictLines: %s", bool32ToStr(limits.strictLines));
    debug("standardSampleLocations: %s", bool32ToStr(limits.standardSampleLocations));
    debug("optimalBufferCopyOffsetAlignment: %u", limits.optimalBufferCopyOffsetAlignment);
    debug("optimalBufferCopyRowPitchAlignment: %u", limits.optimalBufferCopyRowPitchAlignment);
    debug("nonCoherentAtomSize: %u", limits.nonCoherentAtomSize);
  }

  static inline void print(const VkPhysicalDeviceSparseProperties &DAGOR_HAS_LOGS(props))
  {
    debug("residencyStandard2DBlockShape: %s", bool32ToStr(props.residencyStandard2DBlockShape));
    debug("residencyStandard2DMultisampleBlockShape: %s", bool32ToStr(props.residencyStandard2DMultisampleBlockShape));
    debug("residencyStandard3DBlockShape: %s", bool32ToStr(props.residencyStandard3DBlockShape));
    debug("residencyAlignedMipSize: %s", bool32ToStr(props.residencyAlignedMipSize));
    debug("residencyNonResidentStrict: %s", bool32ToStr(props.residencyNonResidentStrict));
  }

  static inline void print(const VkPhysicalDeviceProperties &prop)
  {
    debug("Device: %s", prop.deviceName);
    debug("API version: %u.%u.%u", VK_VERSION_MAJOR(prop.apiVersion), VK_VERSION_MINOR(prop.apiVersion),
      VK_VERSION_PATCH(prop.apiVersion));
    debug("Driver version RAW: %lu", prop.driverVersion);
    debug("VendorID: 0x%X", prop.vendorID);
    debug("DeviceID: 0x%X", prop.deviceID);
    debug("Device Type: %s", nameOf(prop.deviceType));
    String uuidStr("0x");
    for (int i = 0; i < VK_UUID_SIZE; ++i)
      uuidStr.aprintf(32, "%X", prop.pipelineCacheUUID[i]);
    debug("Device UUID: %s", uuidStr);
    print(prop.limits);
    print(prop.sparseProperties);
  }

  static inline void print(const eastl::vector<VkQueueFamilyProperties> &qs)
  {
    String flags;
    for (auto &&q : qs)
    {
      flags.clear();
      if (q.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        flags += "Graphics | ";
      if (q.queueFlags & VK_QUEUE_COMPUTE_BIT)
        flags += "Compute | ";
      if (q.queueFlags & VK_QUEUE_TRANSFER_BIT)
        flags += "Transfer | ";
      if (q.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
        flags += "Sparse Binding | ";
      if (!flags.empty())
        flags.chop(3);
      else
        flags = "None";
      debug("Queue %u:", &q - qs.data());
      debug("Flags: %s", flags);
      debug("Count: %u", q.queueCount);
      debug("Timestamp Valid Bits: %u", q.timestampValidBits);
      debug("Min Image Transfer Granularity: %u, %u, %u", q.minImageTransferGranularity.width, q.minImageTransferGranularity.height,
        q.minImageTransferGranularity.depth);
    }
  }

  static inline const char *formatName(VkFormat fmt)
  {
    switch (fmt)
    {
      default: return "Unknown";
#define E_TO_N(e) \
  case VK_FORMAT_##e: return #e
        E_TO_N(UNDEFINED);
        E_TO_N(R4G4_UNORM_PACK8);
        E_TO_N(R4G4B4A4_UNORM_PACK16);
        E_TO_N(B4G4R4A4_UNORM_PACK16);
        E_TO_N(R5G6B5_UNORM_PACK16);
        E_TO_N(B5G6R5_UNORM_PACK16);
        E_TO_N(R5G5B5A1_UNORM_PACK16);
        E_TO_N(B5G5R5A1_UNORM_PACK16);
        E_TO_N(A1R5G5B5_UNORM_PACK16);
        E_TO_N(R8_UNORM);
        E_TO_N(R8_SNORM);
        E_TO_N(R8_USCALED);
        E_TO_N(R8_SSCALED);
        E_TO_N(R8_UINT);
        E_TO_N(R8_SINT);
        E_TO_N(R8_SRGB);
        E_TO_N(R8G8_UNORM);
        E_TO_N(R8G8_SNORM);
        E_TO_N(R8G8_USCALED);
        E_TO_N(R8G8_SSCALED);
        E_TO_N(R8G8_UINT);
        E_TO_N(R8G8_SINT);
        E_TO_N(R8G8_SRGB);
        E_TO_N(R8G8B8_UNORM);
        E_TO_N(R8G8B8_SNORM);
        E_TO_N(R8G8B8_USCALED);
        E_TO_N(R8G8B8_SSCALED);
        E_TO_N(R8G8B8_UINT);
        E_TO_N(R8G8B8_SINT);
        E_TO_N(R8G8B8_SRGB);
        E_TO_N(B8G8R8_UNORM);
        E_TO_N(B8G8R8_SNORM);
        E_TO_N(B8G8R8_USCALED);
        E_TO_N(B8G8R8_SSCALED);
        E_TO_N(B8G8R8_UINT);
        E_TO_N(B8G8R8_SINT);
        E_TO_N(B8G8R8_SRGB);
        E_TO_N(R8G8B8A8_UNORM);
        E_TO_N(R8G8B8A8_SNORM);
        E_TO_N(R8G8B8A8_USCALED);
        E_TO_N(R8G8B8A8_SSCALED);
        E_TO_N(R8G8B8A8_UINT);
        E_TO_N(R8G8B8A8_SINT);
        E_TO_N(R8G8B8A8_SRGB);
        E_TO_N(B8G8R8A8_UNORM);
        E_TO_N(B8G8R8A8_SNORM);
        E_TO_N(B8G8R8A8_USCALED);
        E_TO_N(B8G8R8A8_SSCALED);
        E_TO_N(B8G8R8A8_UINT);
        E_TO_N(B8G8R8A8_SINT);
        E_TO_N(B8G8R8A8_SRGB);
        E_TO_N(A8B8G8R8_UNORM_PACK32);
        E_TO_N(A8B8G8R8_SNORM_PACK32);
        E_TO_N(A8B8G8R8_USCALED_PACK32);
        E_TO_N(A8B8G8R8_SSCALED_PACK32);
        E_TO_N(A8B8G8R8_UINT_PACK32);
        E_TO_N(A8B8G8R8_SINT_PACK32);
        E_TO_N(A8B8G8R8_SRGB_PACK32);
        E_TO_N(A2R10G10B10_UNORM_PACK32);
        E_TO_N(A2R10G10B10_SNORM_PACK32);
        E_TO_N(A2R10G10B10_USCALED_PACK32);
        E_TO_N(A2R10G10B10_SSCALED_PACK32);
        E_TO_N(A2R10G10B10_UINT_PACK32);
        E_TO_N(A2R10G10B10_SINT_PACK32);
        E_TO_N(A2B10G10R10_UNORM_PACK32);
        E_TO_N(A2B10G10R10_SNORM_PACK32);
        E_TO_N(A2B10G10R10_USCALED_PACK32);
        E_TO_N(A2B10G10R10_SSCALED_PACK32);
        E_TO_N(A2B10G10R10_UINT_PACK32);
        E_TO_N(A2B10G10R10_SINT_PACK32);
        E_TO_N(R16_UNORM);
        E_TO_N(R16_SNORM);
        E_TO_N(R16_USCALED);
        E_TO_N(R16_SSCALED);
        E_TO_N(R16_UINT);
        E_TO_N(R16_SINT);
        E_TO_N(R16_SFLOAT);
        E_TO_N(R16G16_UNORM);
        E_TO_N(R16G16_SNORM);
        E_TO_N(R16G16_USCALED);
        E_TO_N(R16G16_SSCALED);
        E_TO_N(R16G16_UINT);
        E_TO_N(R16G16_SINT);
        E_TO_N(R16G16_SFLOAT);
        E_TO_N(R16G16B16_UNORM);
        E_TO_N(R16G16B16_SNORM);
        E_TO_N(R16G16B16_USCALED);
        E_TO_N(R16G16B16_SSCALED);
        E_TO_N(R16G16B16_UINT);
        E_TO_N(R16G16B16_SINT);
        E_TO_N(R16G16B16_SFLOAT);
        E_TO_N(R16G16B16A16_UNORM);
        E_TO_N(R16G16B16A16_SNORM);
        E_TO_N(R16G16B16A16_USCALED);
        E_TO_N(R16G16B16A16_SSCALED);
        E_TO_N(R16G16B16A16_UINT);
        E_TO_N(R16G16B16A16_SINT);
        E_TO_N(R16G16B16A16_SFLOAT);
        E_TO_N(R32_UINT);
        E_TO_N(R32_SINT);
        E_TO_N(R32_SFLOAT);
        E_TO_N(R32G32_UINT);
        E_TO_N(R32G32_SINT);
        E_TO_N(R32G32_SFLOAT);
        E_TO_N(R32G32B32_UINT);
        E_TO_N(R32G32B32_SINT);
        E_TO_N(R32G32B32_SFLOAT);
        E_TO_N(R32G32B32A32_UINT);
        E_TO_N(R32G32B32A32_SINT);
        E_TO_N(R32G32B32A32_SFLOAT);
        E_TO_N(R64_UINT);
        E_TO_N(R64_SINT);
        E_TO_N(R64_SFLOAT);
        E_TO_N(R64G64_UINT);
        E_TO_N(R64G64_SINT);
        E_TO_N(R64G64_SFLOAT);
        E_TO_N(R64G64B64_UINT);
        E_TO_N(R64G64B64_SINT);
        E_TO_N(R64G64B64_SFLOAT);
        E_TO_N(R64G64B64A64_UINT);
        E_TO_N(R64G64B64A64_SINT);
        E_TO_N(R64G64B64A64_SFLOAT);
        E_TO_N(B10G11R11_UFLOAT_PACK32);
        E_TO_N(E5B9G9R9_UFLOAT_PACK32);
        E_TO_N(D16_UNORM);
        E_TO_N(X8_D24_UNORM_PACK32);
        E_TO_N(D32_SFLOAT);
        E_TO_N(S8_UINT);
        E_TO_N(D16_UNORM_S8_UINT);
        E_TO_N(D24_UNORM_S8_UINT);
        E_TO_N(D32_SFLOAT_S8_UINT);
        E_TO_N(BC1_RGB_UNORM_BLOCK);
        E_TO_N(BC1_RGB_SRGB_BLOCK);
        E_TO_N(BC1_RGBA_UNORM_BLOCK);
        E_TO_N(BC1_RGBA_SRGB_BLOCK);
        E_TO_N(BC2_UNORM_BLOCK);
        E_TO_N(BC2_SRGB_BLOCK);
        E_TO_N(BC3_UNORM_BLOCK);
        E_TO_N(BC3_SRGB_BLOCK);
        E_TO_N(BC4_UNORM_BLOCK);
        E_TO_N(BC4_SNORM_BLOCK);
        E_TO_N(BC5_UNORM_BLOCK);
        E_TO_N(BC5_SNORM_BLOCK);
        E_TO_N(BC6H_UFLOAT_BLOCK);
        E_TO_N(BC6H_SFLOAT_BLOCK);
        E_TO_N(BC7_UNORM_BLOCK);
        E_TO_N(BC7_SRGB_BLOCK);
        E_TO_N(ETC2_R8G8B8_UNORM_BLOCK);
        E_TO_N(ETC2_R8G8B8_SRGB_BLOCK);
        E_TO_N(ETC2_R8G8B8A1_UNORM_BLOCK);
        E_TO_N(ETC2_R8G8B8A1_SRGB_BLOCK);
        E_TO_N(ETC2_R8G8B8A8_UNORM_BLOCK);
        E_TO_N(ETC2_R8G8B8A8_SRGB_BLOCK);
        E_TO_N(EAC_R11_UNORM_BLOCK);
        E_TO_N(EAC_R11_SNORM_BLOCK);
        E_TO_N(EAC_R11G11_UNORM_BLOCK);
        E_TO_N(EAC_R11G11_SNORM_BLOCK);
        E_TO_N(ASTC_4x4_UNORM_BLOCK);
        E_TO_N(ASTC_4x4_SRGB_BLOCK);
        E_TO_N(ASTC_5x4_UNORM_BLOCK);
        E_TO_N(ASTC_5x4_SRGB_BLOCK);
        E_TO_N(ASTC_5x5_UNORM_BLOCK);
        E_TO_N(ASTC_5x5_SRGB_BLOCK);
        E_TO_N(ASTC_6x5_UNORM_BLOCK);
        E_TO_N(ASTC_6x5_SRGB_BLOCK);
        E_TO_N(ASTC_6x6_UNORM_BLOCK);
        E_TO_N(ASTC_6x6_SRGB_BLOCK);
        E_TO_N(ASTC_8x5_UNORM_BLOCK);
        E_TO_N(ASTC_8x5_SRGB_BLOCK);
        E_TO_N(ASTC_8x6_UNORM_BLOCK);
        E_TO_N(ASTC_8x6_SRGB_BLOCK);
        E_TO_N(ASTC_8x8_UNORM_BLOCK);
        E_TO_N(ASTC_8x8_SRGB_BLOCK);
        E_TO_N(ASTC_10x5_UNORM_BLOCK);
        E_TO_N(ASTC_10x5_SRGB_BLOCK);
        E_TO_N(ASTC_10x6_UNORM_BLOCK);
        E_TO_N(ASTC_10x6_SRGB_BLOCK);
        E_TO_N(ASTC_10x8_UNORM_BLOCK);
        E_TO_N(ASTC_10x8_SRGB_BLOCK);
        E_TO_N(ASTC_10x10_UNORM_BLOCK);
        E_TO_N(ASTC_10x10_SRGB_BLOCK);
        E_TO_N(ASTC_12x10_UNORM_BLOCK);
        E_TO_N(ASTC_12x10_SRGB_BLOCK);
        E_TO_N(ASTC_12x12_UNORM_BLOCK);
        E_TO_N(ASTC_12x12_SRGB_BLOCK);
#undef E_TO_N
    }
  }

  static inline void formatFlagsToString(String &str, VkFormatFeatureFlags flags)
  {
    str.clear();
    if (flags & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
      str += "Samplerd Image | ";
    if (flags & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT)
      str += "Storage Image | ";
    if (flags & VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT)
      str += "Storage Image Atomic | ";
    if (flags & VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT)
      str += "Uniform Texel Buffer | ";
    if (flags & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT)
      str += "Storage Texel Buffer | ";
    if (flags & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT)
      str += "Storage Texel Buffer Atomic | ";
    if (flags & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT)
      str += "Vertex Buffer | ";
    if (flags & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)
      str += "Color Attachment | ";
    if (flags & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT)
      str += "Color Attachment Blend | ";
    if (flags & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
      str += "Depth Stencil Attachment | ";
    if (flags & VK_FORMAT_FEATURE_BLIT_SRC_BIT)
      str += "Blit Source | ";
    if (flags & VK_FORMAT_FEATURE_BLIT_DST_BIT)
      str += "Blit Destination | ";
    if (flags & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)
      str += "Sampled Image Filter Linear | ";
    if (flags & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_CUBIC_BIT_IMG)
      str += "Sampled Image Filter Cubic | ";
    if (flags & VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR)
      str += "Transfer Source | ";
    if (flags & VK_FORMAT_FEATURE_TRANSFER_DST_BIT_KHR)
      str += "Transfer Destination | ";
    if (str.empty())
      str = "None";
    else
      str.chop(3);
  }

  static inline void print(dag::ConstSpan<VkFormatProperties> fmt)
  {
    String flags;
    for (int i = 0; i < fmt.size(); ++i)
    {
      debug("%s:", formatName((VkFormat)i));
      formatFlagsToString(flags, fmt[i].linearTilingFeatures);
      debug("Linear Layout: %s", flags);
      formatFlagsToString(flags, fmt[i].optimalTilingFeatures);
      debug("Optimal Layout: %s", flags);
      formatFlagsToString(flags, fmt[i].bufferFeatures);
      debug("Buffer Features: %s", flags);
    }
  }

  static inline void print(uint32_t DAGOR_HAS_LOGS(i), VkMemoryType type)
  {
    debug("Memory type %u: %s(0x%08lX), heap %u", i, formatMemoryTypeFlags(type.propertyFlags), type.propertyFlags, type.heapIndex);
  }

  static inline void print(uint32_t DAGOR_HAS_LOGS(i), VkMemoryHeap type)
  {
    String flags;
    if (type.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
      flags += "Device Local | ";

    if (!flags.empty())
      flags.chop(3);
    else
      flags = "None";
    debug("Heap %i: %s, %u Gib, %u Mib, %u Kib, %u b", i, flags, type.size / 1024 / 1024 / 1024, type.size / 1024 / 1024,
      type.size / 1024, type.size);
  }

  static inline void print(const VkPhysicalDeviceMemoryProperties &mem)
  {
    for (uint32_t i = 0; i < mem.memoryTypeCount; ++i)
      print(i, mem.memoryTypes[i]);
    for (uint32_t i = 0; i < mem.memoryHeapCount; ++i)
      print(i, mem.memoryHeaps[i]);
  }

#if VK_EXT_memory_budget
  static inline void print(const VkPhysicalDeviceMemoryProperties &mem, const VkPhysicalDeviceMemoryBudgetPropertiesEXT &budget)
  {
    for (int i = 0; i != mem.memoryHeapCount; ++i)
      debug("Heap %u budget: %u Mb Used: %u Mb", i, budget.heapBudget[i] >> 20, budget.heapUsage[i] >> 20);
  }
#endif


#if VK_KHR_driver_properties
  static inline void print(const VkPhysicalDeviceDriverPropertiesKHR &drv_props)
  {
    debug("vulkan: driver: name = %s", drv_props.driverName);
    debug("vulkan: driver: info = %s", drv_props.driverInfo);

    String drvTypeStr(32, "<unknown id %u>", (uint32_t)drv_props.driverID);

#if VK_VERSION_1_2
#define DRV_ID_STR(id, val) \
  case id:                  \
    drvTypeStr = val;       \
    break
#else
#define DRV_ID_STR(id, val) \
  case id##_KHR:            \
    drvTypeStr = val;       \
    break
#endif

    switch (drv_props.driverID)
    {
      DRV_ID_STR(VK_DRIVER_ID_AMD_PROPRIETARY, "AMD proprietary");
      DRV_ID_STR(VK_DRIVER_ID_AMD_OPEN_SOURCE, "AMD open source");
      DRV_ID_STR(VK_DRIVER_ID_MESA_RADV, "MESA RADV");
      DRV_ID_STR(VK_DRIVER_ID_NVIDIA_PROPRIETARY, "NV proprietary");
      DRV_ID_STR(VK_DRIVER_ID_INTEL_PROPRIETARY_WINDOWS, "Intel proprietary");
      DRV_ID_STR(VK_DRIVER_ID_INTEL_OPEN_SOURCE_MESA, "Intel MESA");
      DRV_ID_STR(VK_DRIVER_ID_IMAGINATION_PROPRIETARY, "Imgtec proprietary");
      DRV_ID_STR(VK_DRIVER_ID_QUALCOMM_PROPRIETARY, "Qualcomm proprietary");
      DRV_ID_STR(VK_DRIVER_ID_ARM_PROPRIETARY, "ARM proprietary");
      DRV_ID_STR(VK_DRIVER_ID_GOOGLE_SWIFTSHADER, "Google swiftshader");
      DRV_ID_STR(VK_DRIVER_ID_GGP_PROPRIETARY, "GGP proprietary");
      DRV_ID_STR(VK_DRIVER_ID_BROADCOM_PROPRIETARY, "Broadcom proprietary");
#if VK_VERSION_1_2
      DRV_ID_STR(VK_DRIVER_ID_MESA_LLVMPIPE, "MESA llvmpipe");
      DRV_ID_STR(VK_DRIVER_ID_MOLTENVK, "Molten VK");
#endif
#if VK_VERSION_1_3
      DRV_ID_STR(VK_DRIVER_ID_COREAVI_PROPRIETARY, "CoreAVI proprietary");
      DRV_ID_STR(VK_DRIVER_ID_JUICE_PROPRIETARY, "Juice proprietary");
      DRV_ID_STR(VK_DRIVER_ID_VERISILICON_PROPRIETARY, "Verisilicon proprietary");
      DRV_ID_STR(VK_DRIVER_ID_MESA_TURNIP, "MESA Turnip");
      DRV_ID_STR(VK_DRIVER_ID_MESA_V3DV, "MESA v3dv");
      DRV_ID_STR(VK_DRIVER_ID_MESA_PANVK, "MESA PanVK");
      DRV_ID_STR(VK_DRIVER_ID_SAMSUNG_PROPRIETARY, "Samsung proprietary");
      DRV_ID_STR(VK_DRIVER_ID_MESA_VENUS, "MESA Venus");
#endif
      default: break;
    }

#undef DRV_ID_STR

    debug("vulkan: driver: type id = %s", drvTypeStr);
    debug("vulkan: driver: conformant against vulkan %u.%u.%u.%u", drv_props.conformanceVersion.major,
      drv_props.conformanceVersion.minor, drv_props.conformanceVersion.subminor, drv_props.conformanceVersion.patch);
  }
#endif

  inline void printExt() const
  {
    debug("VK_EXT_conditional_rendering:");
    debug("conditionalRendering: %s", hasConditionalRender ? "yes" : "no");
    debug("VK_KHR_imageless_framebuffer:");
    debug("imagelessFramebuffer: %s", hasImagelessFramebuffer ? "yes" : "no");
    debug("memoryBudget: %s", memoryBudgetInfoAvailable ? "yes" : "no");
    debug("deviceMemoryReport: %s", hasDeviceMemoryReport ? "yes" : "no");
    debug("bufferDeviceAddress: %s", hasDeviceBufferDeviceAddress ? "yes" : "no");
    debug("rayTracingPipeline: %s", hasRayTracingPipeline ? "yes" : "no");
    debug("rayQuery: %s", hasRayQuery ? "yes" : "no");
    debug("accelerationStructure: %s", hasAccelerationStructure ? "yes" : "no");
    debug("deviceFaultFeature: %s", hasDeviceFaultFeature ? "yes" : "no");
    debug("deviceFaultVendorInfo: %s", hasDeviceFaultVendorInfo ? "yes" : "no");
    debug("depthStencilResolve: %s", hasDepthStencilResolve ? "yes" : "no");
#if VK_EXT_memory_budget
    if (memoryBudgetInfoAvailable)
      print(memoryProperties, memoryBudgetInfo);
#endif
#if D3D_HAS_RAY_TRACING
    debug("Raytracing:");
    debug("shaderHeaderSize: %u", raytraceShaderHeaderSize);
    debug("maxRecursionDepth: %u", raytraceMaxRecursionDepth);
    debug("topAccelerationInstanceElementSize: %u", raytraceTopAccelerationInstanceElementSize);
#endif
#if VK_EXT_descriptor_indexing
    debug("Bindless (VK_EXT_descriptor_indexing): %s", hasBindless ? "yes" : "no");
    debug("maxBindlessTextures (maxDescriptorSetUpdateAfterBindSampledImages): %u", maxBindlessTextures);
    debug("maxBindlessSamplers (maxDescriptorSetUpdateAfterBindSamplers): %u", maxBindlessSamplers);
#endif
    debug("WarpSize: %u", warpSize);
    debug("Driver version: %u.%u.%u.%u", driverVersionDecoded[0], driverVersionDecoded[1], driverVersionDecoded[2],
      driverVersionDecoded[3]);
    debug("hasLazyMemory: %s", hasLazyMemory ? "yes" : "no");
#if VK_KHR_shader_float16_int8
    debug("hasShaderFloat16: %s", hasShaderFloat16 ? "yes" : "no");
#endif
  }

  inline void print() const
  {
    print(properties);
#if VK_KHR_driver_properties
    if (hasExtension<DriverPropertiesKHR>())
      print(driverProps);
#endif
    print(memoryProperties);
    print(features);
    print(queueFamilyProperties);
    print(formatProperties);
    printExt();
  }
};
} // namespace drv3d_vulkan