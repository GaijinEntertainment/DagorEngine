// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <drv/3d/dag_decl.h>

#include "vulkan_device.h"
#include "vulkan_instance.h"
#include "vk_to_string.h"
#include "device_featureset.h"

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
    {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR, nullptr, false};
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
  VkPhysicalDeviceAccelerationStructurePropertiesKHR deviceAccelerationStructureProps = //
    {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR, nullptr};
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

#if VK_EXT_pageable_device_local_memory
  VkPhysicalDevicePageableDeviceLocalMemoryFeaturesEXT pageableDeviceLocalMemoryFeatures = //
    {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PAGEABLE_DEVICE_LOCAL_MEMORY_FEATURES_EXT, nullptr, false};
#endif

#if VK_EXT_memory_priority
  VkPhysicalDeviceMemoryPriorityFeaturesEXT memoryPriorityFeatures = //
    {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT, nullptr, false};
#endif

#if VK_EXT_pipeline_creation_cache_control
  VkPhysicalDevicePipelineCreationCacheControlFeaturesEXT pipelineCreationCacheControlFeatures = //
    {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_CREATION_CACHE_CONTROL_FEATURES_EXT, nullptr, false};
#endif

#if VK_KHR_16bit_storage
  VkPhysicalDevice16BitStorageFeaturesKHR sixteenBitStorageFeatures = //
    {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES, nullptr, false, false, false, false};
#endif

#if VK_KHR_display
  Tab<VkDisplayPropertiesKHR> displays;
#endif

#if VK_KHR_synchronization2
  VkPhysicalDeviceSynchronization2Features synchronization2Features = //
    {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES, nullptr, false};
#endif

#if VK_KHR_global_priority
  VkPhysicalDeviceGlobalPriorityQueryFeaturesKHR globalPriorityFeatures = //
    {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GLOBAL_PRIORITY_QUERY_FEATURES_KHR, nullptr, false};
  eastl::vector<VkQueueFamilyGlobalPriorityPropertiesKHR> queuesGlobalPriorityProperties;
#endif

  bool hasDevProps2 = false;

  bool hasConditionalRender = false;
  bool hasImagelessFramebuffer = false;
  bool hasDeviceMemoryReport = false;
  bool hasDeviceBufferDeviceAddress = false;
  bool hasRayTracingPipeline = false;
  bool hasRayQuery = false;
  bool hasAccelerationStructure = false;
  bool hasIndirectRayTrace = false;
  bool hasRayTracePrimitiveCulling = false;
  bool hasDeviceFaultFeature = false;
  bool hasDeviceFaultVendorInfo = false;
  bool hasDepthStencilResolve = false;
  bool hasBindless = false;
  bool hasLazyMemory = false;
  bool hasShaderFloat16 = false;
  bool hasMemoryPriority = false;
  bool hasPageableDeviceLocalMemory = false;
  bool hasPipelineCreationCacheControl = false;
  bool hasSixteenBitStorage = false;
  bool hasSynchronization2 = false;
  bool hasGlobalPriority = false;
  uint32_t maxBindlessTextures = 0;
  uint32_t maxBindlessSamplers = 0;
  uint32_t maxBindlessBuffers = 0;
  uint32_t maxBindlessStorageBuffers = 0;
  uint32_t raytraceShaderHeaderSize = 0;
  uint32_t raytraceMaxRecursionDepth = 0;
  uint32_t raytraceTopAccelerationInstanceElementSize = 0;
  uint32_t raytraceScratchBufferAlignment = 0;
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

  bool hasExtensionByName(const char *name) const
  {
    return end(extensions) != eastl::find_if(begin(extensions), end(extensions),
                                [name](const VkExtensionProperties &ep) { return 0 == strcmp(name, ep.extensionName); });
  }

  void disableExtensionByName(const char *name)
  {
    if (hasExtensionByName(name))
      extensions.erase(eastl::find_if(begin(extensions), end(extensions),
        [name](const VkExtensionProperties &ep) { return 0 == strcmp(name, ep.extensionName); }));
  }

  template <typename T>
  void disableExtension()
  {
    if (hasExtension<T>())
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

#if VK_EXT_memory_priority
    if (hasExtension<MemoryPriorityEXT>())
    {
      chain_structs(target, memoryPriorityFeatures);
    }
#endif

#if VK_EXT_pageable_device_local_memory
    if (hasExtension<PageableDeviceLocalMemoryEXT>())
    {
      chain_structs(target, pageableDeviceLocalMemoryFeatures);
    }
#endif

#if VK_EXT_pipeline_creation_cache_control
    if (hasExtension<PipelineCreationCacheControlEXT>())
    {
      chain_structs(target, pipelineCreationCacheControlFeatures);
    }
#endif

#if VK_KHR_16bit_storage
    if (hasExtension<SixteenBitStorageKHR>())
    {
      chain_structs(target, sixteenBitStorageFeatures);
    }
#endif

#if VK_KHR_synchronization2
    if (hasExtension<Synchronization2KHR>())
    {
      chain_structs(target, synchronization2Features);
    }
#endif

#if VK_KHR_global_priority
    if (hasExtension<GlobalPriorityKHR>())
    {
      chain_structs(target, globalPriorityFeatures);
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
    else
      hasConditionalRender = false;
#endif

#if VK_KHR_imageless_framebuffer
    if (hasExtension<ImagelessFramebufferKHR>() && hasExtension<ImageFormatListKHR>())
    {
      hasImagelessFramebuffer = imagelessFramebufferFeature.imagelessFramebuffer == VK_TRUE;
      imagelessFramebufferFeature.pNext = nullptr;
    }
    else
      hasImagelessFramebuffer = false;
#endif

#if VK_EXT_device_memory_report
    if (hasExtension<DeviceMemoryReportEXT>())
    {
      hasDeviceMemoryReport = deviceMemoryReportFeature.deviceMemoryReport == VK_TRUE;
      deviceMemoryReportFeature.pNext = nullptr;
    }
    else
      hasDeviceMemoryReport = false;
#endif

#if VK_KHR_buffer_device_address
    if (hasExtension<BufferDeviceAddressKHR>())
    {
      hasDeviceBufferDeviceAddress = deviceBufferDeviceAddressFeature.bufferDeviceAddress == VK_TRUE;
      deviceBufferDeviceAddressFeature.pNext = nullptr;
    }
    else
      hasDeviceBufferDeviceAddress = false;
#endif

#if VK_KHR_ray_tracing_pipeline
    if (hasExtension<RaytracingPipelineKHR>())
    {
      hasRayTracingPipeline = deviceRayTracingPipelineFeature.rayTracingPipeline == VK_TRUE;
      hasIndirectRayTrace = deviceRayTracingPipelineFeature.rayTracingPipelineTraceRaysIndirect == VK_TRUE;
      hasRayTracePrimitiveCulling = deviceRayTracingPipelineFeature.rayTraversalPrimitiveCulling == VK_TRUE;
      deviceRayTracingPipelineFeature.pNext = nullptr;
    }
    else
    {
      hasRayTracingPipeline = false;
      hasIndirectRayTrace = false;
      hasRayTracePrimitiveCulling = false;
    }
#endif

#if VK_KHR_ray_query
    if (hasExtension<RayQueryKHR>())
    {
      hasRayQuery = deviceRayQueryFeature.rayQuery == VK_TRUE;
      deviceRayQueryFeature.pNext = nullptr;
    }
    else
      hasRayQuery = false;
#endif

#if VK_KHR_acceleration_structure
    if (hasExtension<AccelerationStructureKHR>())
    {
      hasAccelerationStructure = deviceAccelerationStructureFeature.accelerationStructure == VK_TRUE;
      deviceAccelerationStructureFeature.pNext = nullptr;
    }
    else
      hasAccelerationStructure = false;
#endif

#if VK_EXT_device_fault
    if (hasExtension<DeviceFaultEXT>())
    {
      hasDeviceFaultFeature = deviceFaultFeature.deviceFault;
      hasDeviceFaultVendorInfo = deviceFaultFeature.deviceFaultVendorBinary;
      deviceFaultFeature.pNext = nullptr;
    }
    else
    {
      hasDeviceFaultFeature = false;
      hasDeviceFaultVendorInfo = false;
    }
#endif

#if VK_EXT_descriptor_indexing
    if (hasExtension<DescriptorIndexingEXT>())
    {
      hasBindless = indexingFeatures.descriptorBindingPartiallyBound && indexingFeatures.runtimeDescriptorArray;
      indexingFeatures.pNext = nullptr;
    }
    else
      hasBindless = false;
#endif

#if VK_KHR_shader_float16_int8
    if (hasExtension<ShaderFloat16Int8KHR>())
    {
      hasShaderFloat16 = shaderFloat16Int8Features.shaderFloat16;
      shaderFloat16Int8Features.pNext = nullptr;
    }
    else
      hasShaderFloat16 = false;
#endif

#if VK_EXT_memory_priority
    if (hasExtension<MemoryPriorityEXT>())
    {
      hasMemoryPriority = memoryPriorityFeatures.memoryPriority;
      memoryPriorityFeatures.pNext = nullptr;
    }
    else
      hasMemoryPriority = false;
#endif

#if VK_EXT_pageable_device_local_memory
    if (hasExtension<PageableDeviceLocalMemoryEXT>())
    {
      hasPageableDeviceLocalMemory = pageableDeviceLocalMemoryFeatures.pageableDeviceLocalMemory;
      pageableDeviceLocalMemoryFeatures.pNext = nullptr;
    }
    else
      hasPageableDeviceLocalMemory = false;
#endif

#if VK_EXT_pipeline_creation_cache_control
    if (hasExtension<PipelineCreationCacheControlEXT>())
    {
      hasPipelineCreationCacheControl = pipelineCreationCacheControlFeatures.pipelineCreationCacheControl;
      pipelineCreationCacheControlFeatures.pNext = nullptr;
    }
    else
      hasPipelineCreationCacheControl = false;
#endif

#if VK_KHR_16bit_storage
    if (hasExtension<SixteenBitStorageKHR>())
    {
      hasSixteenBitStorage = sixteenBitStorageFeatures.storageBuffer16BitAccess;
      sixteenBitStorageFeatures.pNext = nullptr;
    }
    else
      hasSixteenBitStorage = false;
#endif

#if VK_KHR_synchronization2
    if (hasExtension<Synchronization2KHR>())
    {
      hasSynchronization2 = synchronization2Features.synchronization2;
      synchronization2Features.pNext = nullptr;
    }
    else
      hasSynchronization2 = false;
#endif

#if VK_KHR_global_priority
    if (hasExtension<GlobalPriorityKHR>())
    {
      hasGlobalPriority = globalPriorityFeatures.globalPriorityQuery;
      globalPriorityFeatures.pNext = nullptr;
    }
    else
      hasGlobalPriority = false;
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
    VkPhysicalDeviceFeatures2KHR pdf = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR, nullptr, {}};

    chainExtendedFeaturesInfoStructs(pdf);

    VULKAN_LOG_CALL(instance.vkGetPhysicalDeviceFeatures2KHR(device, &pdf));
    features = pdf.features;

    processExtendedFeaturesQueryResult();

    VkPhysicalDeviceProperties2KHR pdp = //
      {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR, nullptr};

#if VK_KHR_driver_properties
    if (hasExtension<DriverPropertiesKHR>())
    {
      driverProps.pNext = nullptr;
      chain_structs(pdp, driverProps);
    }
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
        depthStencilResolveProps.pNext = nullptr;
        chain_structs(pdp, depthStencilResolveProps);
      }
      else if (depthStencilResolvePresent)
        disableExtension<DepthStencilResolveKHR>();
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

#if VK_KHR_acceleration_structure
    if (hasExtension<AccelerationStructureKHR>())
      chain_structs(pdp, deviceAccelerationStructureProps);
#endif

    VULKAN_LOG_CALL(instance.vkGetPhysicalDeviceProperties2KHR(device, &pdp));
    properties = pdp.properties;

#if VK_KHR_acceleration_structure
    if (hasExtension<AccelerationStructureKHR>())
    {
      raytraceScratchBufferAlignment = deviceAccelerationStructureProps.minAccelerationStructureScratchOffsetAlignment;
      deviceAccelerationStructureProps.pNext = nullptr;
    }
#endif

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
      maxBindlessBuffers = descriptorIndexingProps.maxDescriptorSetUpdateAfterBindUniformBuffers;
      maxBindlessStorageBuffers = descriptorIndexingProps.maxDescriptorSetUpdateAfterBindStorageBuffers;
      descriptorIndexingProps.pNext = nullptr;
    }
#endif

    subgroupProperties = sgdp;

    {
      uint32_t qfpc = 0;
      VULKAN_LOG_CALL(instance.vkGetPhysicalDeviceQueueFamilyProperties2KHR(device, &qfpc, nullptr));
      eastl::vector<VkQueueFamilyProperties2KHR> qf;
      qf.resize(qfpc);
#if VK_KHR_global_priority
      if (hasExtension<GlobalPriorityKHR>())
      {
        queuesGlobalPriorityProperties.resize(qfpc);
        for (auto &&info : queuesGlobalPriorityProperties)
        {
          info.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_GLOBAL_PRIORITY_PROPERTIES_KHR;
          info.pNext = nullptr;
        }
      }
#endif
      for (auto &&info : qf)
      {
        info.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2_KHR;
#if VK_KHR_global_priority
        if (hasExtension<GlobalPriorityKHR>())
          chain_structs(info, queuesGlobalPriorityProperties[&info - qf.begin()]);
        else
          info.pNext = nullptr;
#else
        info.pNext = nullptr;
#endif
      }
      VULKAN_LOG_CALL(instance.vkGetPhysicalDeviceQueueFamilyProperties2KHR(device, &qfpc, qf.data()));
      qf.resize(qfpc);
      queueFamilyProperties.clear();
      queueFamilyProperties.reserve(qfpc);
      for (auto &&info : qf)
        queueFamilyProperties.push_back(info.queueFamilyProperties);
    }

    VkPhysicalDeviceMemoryProperties2KHR pdmp = //
      {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2_KHR, nullptr};

#if VK_EXT_memory_budget
    if (hasExtension<MemoryBudgetEXT>())
    {
      memoryBudgetInfo.pNext = nullptr;
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

  void queryDisplays(VulkanInstance &instance)
  {
#if VK_KHR_display
    if (!displays.empty())
      return;

    if (instance.hasExtension<DisplayKHR>())
    {
      uint32_t count = 0;
      if (VULKAN_OK(instance.vkGetPhysicalDeviceDisplayPropertiesKHR(device, &count, nullptr)))
      {
        displays.resize(count);
        if (VULKAN_FAIL(instance.vkGetPhysicalDeviceDisplayPropertiesKHR(device, &count, displays.data())))
        {
          clear_and_shrink(displays);
          return;
        }
      }
    }
#endif
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
    queryDisplays(instance);
  }

  static inline const char *bool32ToStr(VkBool32 b) { return b ? "y" : "n"; }
  static inline const char *boolToStr(VkBool32 b) { return b ? "y" : "n"; }

  // DID stands for "device info dump"
#define apd(fmt, ...)                  \
  {                                    \
    out += "vulkan: DID: ";            \
    out.aprintf(64, fmt, __VA_ARGS__); \
    out += "\n";                       \
  }

#define apdnf(str)          \
  {                         \
    out += "vulkan: DID: "; \
    out += str;             \
    out += "\n";            \
  }

  static inline void print(const VkPhysicalDeviceFeatures &set, String &out)
  {
    apd("robustBufferAccess: %s, fullDrawIndexUint32: %s, imageCubeArray: %s, independentBlend: %s",
      bool32ToStr(set.robustBufferAccess), bool32ToStr(set.fullDrawIndexUint32), bool32ToStr(set.imageCubeArray),
      bool32ToStr(set.independentBlend));
    apd("geometryShader: %s, tessellationShader: %s, sampleRateShading: %s, dualSrcBlend: %s", bool32ToStr(set.geometryShader),
      bool32ToStr(set.tessellationShader), bool32ToStr(set.sampleRateShading), bool32ToStr(set.dualSrcBlend));
    apd("logicOp: %s, multiDrawIndirect: %s, drawIndirectFirstInstance: %s, depthClamp: %s", bool32ToStr(set.logicOp),
      bool32ToStr(set.multiDrawIndirect), bool32ToStr(set.drawIndirectFirstInstance), bool32ToStr(set.depthClamp));
    apd("depthBiasClamp: %s, fillModeNonSolid: %s, depthBounds: %s, wideLines: %s", bool32ToStr(set.depthBiasClamp),
      bool32ToStr(set.fillModeNonSolid), bool32ToStr(set.depthBounds), bool32ToStr(set.wideLines));
    apd("largePoints: %s, alphaToOne: %s, multiViewport: %s, samplerAnisotropy: %s", bool32ToStr(set.largePoints),
      bool32ToStr(set.alphaToOne), bool32ToStr(set.multiViewport), bool32ToStr(set.samplerAnisotropy));
    apd("textureCompression[ETC2,ASTC_LDR,BC]: %s,%s,%s", bool32ToStr(set.textureCompressionETC2),
      bool32ToStr(set.textureCompressionASTC_LDR), bool32ToStr(set.textureCompressionBC));
    apd("occlusionQueryPrecise: %s, pipelineStatisticsQuery: %s, vertexPipelineStoresAndAtomics: %s, fragmentStoresAndAtomics: %s",
      bool32ToStr(set.occlusionQueryPrecise), bool32ToStr(set.pipelineStatisticsQuery),
      bool32ToStr(set.vertexPipelineStoresAndAtomics), bool32ToStr(set.fragmentStoresAndAtomics));
    apd("shaderTessellationAndGeometryPointSize: %s, shaderImageGatherExtended: %s, shaderClipDistance: %s, shaderCullDistance: %s",
      bool32ToStr(set.shaderTessellationAndGeometryPointSize), bool32ToStr(set.shaderImageGatherExtended),
      bool32ToStr(set.shaderClipDistance), bool32ToStr(set.shaderCullDistance));
    apd("shaderStorageImage[ExtendedFormats,Multisample,ReadWithoutFormat,WriteWithoutFormat]: %s,%s,%s,%s",
      bool32ToStr(set.shaderStorageImageExtendedFormats), bool32ToStr(set.shaderStorageImageMultisample),
      bool32ToStr(set.shaderStorageImageReadWithoutFormat), bool32ToStr(set.shaderStorageImageWriteWithoutFormat));
    apd("shader[UniformBuffer,SampledImage,StorageBuffer,StorageImage]ArrayDynamicIndexing: %s,%s,%s,%s",
      bool32ToStr(set.shaderUniformBufferArrayDynamicIndexing), bool32ToStr(set.shaderSampledImageArrayDynamicIndexing),
      bool32ToStr(set.shaderStorageBufferArrayDynamicIndexing), bool32ToStr(set.shaderStorageImageArrayDynamicIndexing));
    apd("shader[Float64,Int64,Int16,ResourceResidency,ResourceMinLod]: %s,%s,%s,%s,%s", bool32ToStr(set.shaderFloat64),
      bool32ToStr(set.shaderInt64), bool32ToStr(set.shaderInt16), bool32ToStr(set.shaderResourceResidency),
      bool32ToStr(set.shaderResourceMinLod));
    apd("sparseBinding: %s, sparseResidency[Buffer,Image2D,Image3D,2Samples,4Samples,8Samples,16Samples,Aliased]: "
        "%s,%s,%s,%s,%s,%s,%s,%s",
      bool32ToStr(set.sparseBinding), bool32ToStr(set.sparseResidencyBuffer), bool32ToStr(set.sparseResidencyImage2D),
      bool32ToStr(set.sparseResidencyImage3D), bool32ToStr(set.sparseResidency2Samples), bool32ToStr(set.sparseResidency4Samples),
      bool32ToStr(set.sparseResidency8Samples), bool32ToStr(set.sparseResidency16Samples), bool32ToStr(set.sparseResidencyAliased));
    apd("variableMultisampleRate: %s, inheritedQueries: %s", bool32ToStr(set.variableMultisampleRate),
      bool32ToStr(set.inheritedQueries));
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

  static inline void print(const VkPhysicalDeviceLimits &limits, String &out)
  {
    apd("maxImageDimension1D,2D,3D,Cube: %u, %u, %u, %u", limits.maxImageDimension1D, limits.maxImageDimension2D,
      limits.maxImageDimension3D, limits.maxImageDimensionCube);
    apd("maxImageArrayLayers: %u, maxTexelBufferElements: %u", limits.maxImageArrayLayers, limits.maxTexelBufferElements);
    apd("maxUniformBufferRange: %u, maxStorageBufferRange: %u", limits.maxUniformBufferRange, limits.maxStorageBufferRange);
    apd("maxMemoryAllocationCount: %u, maxSamplerAllocationCount: %u", limits.maxMemoryAllocationCount,
      limits.maxSamplerAllocationCount);
    apd("maxPushConstantsSize: %u, bufferImageGranularity: %u, sparseAddressSpaceSize: %u, maxBoundDescriptorSets: %u",
      limits.maxPushConstantsSize, limits.bufferImageGranularity, limits.sparseAddressSpaceSize, limits.maxBoundDescriptorSets);
    apd(
      "maxPerStageDescriptor[Samplers,UniformBuffers,StorageBuffers,SampledImages,StorageImages,InputAttachments]: %u,%u,%u,%u,%u,%u",
      limits.maxPerStageDescriptorSamplers, limits.maxPerStageDescriptorUniformBuffers, limits.maxPerStageDescriptorStorageBuffers,
      limits.maxPerStageDescriptorSampledImages, limits.maxPerStageDescriptorStorageImages,
      limits.maxPerStageDescriptorInputAttachments);
    apd("maxPerStageResources: %u, maxDescriptorSetSamplers: %u", limits.maxPerStageResources, limits.maxDescriptorSetSamplers);
    apd("maxDescriptorSetUniform[Buffers,BuffersDynamic]: %u,%u", limits.maxDescriptorSetUniformBuffers,
      limits.maxDescriptorSetUniformBuffersDynamic);
    apd("maxDescriptorSetStorage[Buffers,BuffersDynamic]: %u,%u", limits.maxDescriptorSetStorageBuffers,
      limits.maxDescriptorSetStorageBuffersDynamic);
    apd("maxDescriptorSet[Sampled,Storage]Images: %u,%u", limits.maxDescriptorSetSampledImages, limits.maxDescriptorSetStorageImages);
    apd("maxDescriptorSetInputAttachments: %u", limits.maxDescriptorSetInputAttachments);
    apd("maxVertexInput[Attributes,Bindings,AttributeOffset,BindingStride]: %u,%u,%u,%u", limits.maxVertexInputAttributes,
      limits.maxVertexInputBindings, limits.maxVertexInputAttributeOffset, limits.maxVertexInputBindingStride);
    apd("maxVertexOutputComponents: %u, maxTessellationGenerationLevel: %u, maxTessellationPatchSize: %u",
      limits.maxVertexOutputComponents, limits.maxTessellationGenerationLevel, limits.maxTessellationPatchSize);
    apd("maxTessellationControl[PerVertexInput,PerVertexOutput,PerPatchOutput,TotalOutput]Components: %u,%u,%u,%u",
      limits.maxTessellationControlPerVertexInputComponents, limits.maxTessellationControlPerVertexOutputComponents,
      limits.maxTessellationControlPerPatchOutputComponents, limits.maxTessellationControlTotalOutputComponents);
    apd("maxTessellationEvaluation[Input,Output]Components: %u,%u", limits.maxTessellationEvaluationInputComponents,
      limits.maxTessellationEvaluationOutputComponents);
    apd("maxGeometry[ShaderInvocations,InputComponents,OutputComponents,OutputVertices,TotalOutputComponents]: %u,%u,%u,%u,%u",
      limits.maxGeometryShaderInvocations, limits.maxGeometryInputComponents, limits.maxGeometryOutputComponents,
      limits.maxGeometryOutputVertices, limits.maxGeometryTotalOutputComponents);
    apd("maxFragment[InputComponents,OutputAttachments,DualSrcAttachments,CombinedOutputResources]: %u,%u,%u,%u",
      limits.maxFragmentInputComponents, limits.maxFragmentOutputAttachments, limits.maxFragmentDualSrcAttachments,
      limits.maxFragmentCombinedOutputResources);
    apd("maxComputeSharedMemorySize: %u, maxComputeWorkGroupInvocations: %u", limits.maxComputeSharedMemorySize,
      limits.maxComputeWorkGroupInvocations);
    apd("maxComputeWorkGroupCount: [%u,%u,%u], maxComputeWorkGroupSize: [%u,%u,%u]", limits.maxComputeWorkGroupCount[0],
      limits.maxComputeWorkGroupCount[1], limits.maxComputeWorkGroupCount[2], limits.maxComputeWorkGroupSize[0],
      limits.maxComputeWorkGroupSize[1], limits.maxComputeWorkGroupSize[2]);
    apd("sub[Pixel,Texel]PrecisionBits: %u,%u, mipmapPrecisionBits: %u", limits.subPixelPrecisionBits, limits.subTexelPrecisionBits,
      limits.mipmapPrecisionBits);
    apd("maxDraw[IndexedIndexValue,IndirectCount]: %u,%u", limits.maxDrawIndexedIndexValue, limits.maxDrawIndirectCount);
    apd("maxSampler[LodBias,Anisotropy]: %f,%f, maxViewport[s,Dimensions]: %u, [%u,%u]", limits.maxSamplerLodBias,
      limits.maxSamplerAnisotropy, limits.maxViewports, limits.maxViewportDimensions[0], limits.maxViewportDimensions[1]);
    apd("viewportBoundsRange: [%f,%f], viewportSubPixelBits: %u, minMemoryMapAlignment: %u", limits.viewportBoundsRange[0],
      limits.viewportBoundsRange[1], limits.viewportSubPixelBits, limits.minMemoryMapAlignment);
    apd("min[Texel,Uniform,Storage]BufferOffsetAlignment: %u,%u,%u", limits.minTexelBufferOffsetAlignment,
      limits.minUniformBufferOffsetAlignment, limits.minStorageBufferOffsetAlignment);
    apd("[min,max]TexelOffset: %u,%u, [min,max]TexelGatherOffset: %u,%u", limits.minTexelOffset, limits.maxTexelOffset,
      limits.minTexelGatherOffset, limits.maxTexelGatherOffset);
    apd("[min,max]InterpolationOffset: %f,%f, subPixelInterpolationOffsetBits: %u", limits.minInterpolationOffset,
      limits.maxInterpolationOffset, limits.subPixelInterpolationOffsetBits);
    apd("maxFramebuffer[Width,Height,Layers]: %u,%u,%u", limits.maxFramebufferWidth, limits.maxFramebufferHeight,
      limits.maxFramebufferLayers);
    apd("framebuffer[Color,Depth,Stencil,NoAttachments]SampleCounts: %u,%u,%u,%u", limits.framebufferColorSampleCounts,
      limits.framebufferDepthSampleCounts, limits.framebufferStencilSampleCounts, limits.framebufferNoAttachmentsSampleCounts);
    apd("sampledImage[Color,Integer,Depth,Stencil]SampleCounts: %u,%u,%u,%u", limits.sampledImageColorSampleCounts,
      limits.sampledImageIntegerSampleCounts, limits.sampledImageDepthSampleCounts, limits.sampledImageStencilSampleCounts);
    apd("maxColorAttachments: %u, storageImageSampleCounts: %u, maxSampleMaskWords: %u", limits.maxColorAttachments,
      limits.storageImageSampleCounts, limits.maxSampleMaskWords);
    apd("timestampComputeAndGraphics: %u, timestampPeriod: %f", limits.timestampComputeAndGraphics, limits.timestampPeriod);
    apd("max[Clip,Cull]Distances: %u,%u, maxCombinedClipAndCullDistances: %u", limits.maxClipDistances, limits.maxCullDistances,
      limits.maxCombinedClipAndCullDistances);
    apd("pointSizeRange: [%f,%f], lineWidthRange: [%f,%f]", limits.pointSizeRange[0], limits.pointSizeRange[1],
      limits.lineWidthRange[0], limits.lineWidthRange[1]);
    apd("discreteQueuePriorities: %u, pointSizeGranularity: %f, lineWidthGranularity: %f", limits.discreteQueuePriorities,
      limits.pointSizeGranularity, limits.lineWidthGranularity);
    apd("strictLines: %s, standardSampleLocations: %s", bool32ToStr(limits.strictLines), bool32ToStr(limits.standardSampleLocations));
    apd("optimalBufferCopy[Offset,RowPitch]Alignment: %u,%u, nonCoherentAtomSize: %u", limits.optimalBufferCopyOffsetAlignment,
      limits.optimalBufferCopyRowPitchAlignment, limits.nonCoherentAtomSize);
  }

  static inline void print(const VkPhysicalDeviceSparseProperties &props, String &out)
  {
    apd("residencyStandard[2D,2DMultisample,3D]BlockShape: %s,%s,%s, residencyAlignedMipSize: %s, residencyNonResidentStrict: %s",
      bool32ToStr(props.residencyStandard2DBlockShape), bool32ToStr(props.residencyStandard2DMultisampleBlockShape),
      bool32ToStr(props.residencyStandard3DBlockShape), bool32ToStr(props.residencyAlignedMipSize),
      bool32ToStr(props.residencyNonResidentStrict));
  }

  static inline void print(const VkPhysicalDeviceProperties &prop, String &out)
  {
    apd("%s %s(VID%X DID%X) API %u.%u.%u Driver RAW %lu", nameOf(prop.deviceType), prop.deviceName, prop.vendorID, prop.deviceID,
      VK_VERSION_MAJOR(prop.apiVersion), VK_VERSION_MINOR(prop.apiVersion), VK_VERSION_PATCH(prop.apiVersion), prop.driverVersion);
    String uuidStr("0x");
    for (int i = 0; i < VK_UUID_SIZE; ++i)
      uuidStr.aprintf(32, "%X", prop.pipelineCacheUUID[i]);
    apd("UUID: %s", uuidStr);
    print(prop.limits, out);
    print(prop.sparseProperties, out);
  }

#if VK_KHR_global_priority
  static inline void print(const eastl::vector<VkQueueFamilyGlobalPriorityPropertiesKHR> &prop, String &out)
  {
    String prios;
    String dump("Queue priorities: [");
    for (auto &&q : prop)
    {
      prios.clear();
      for (uint32_t i = 0; i < q.priorityCount; ++i)
      {
        switch (q.priorities[i])
        {
          case VK_QUEUE_GLOBAL_PRIORITY_LOW_KHR: prios += "LOW|"; break;
          case VK_QUEUE_GLOBAL_PRIORITY_MEDIUM_KHR: prios += "MED|"; break;
          case VK_QUEUE_GLOBAL_PRIORITY_HIGH_KHR: prios += "HI|"; break;
          case VK_QUEUE_GLOBAL_PRIORITY_REALTIME_KHR: prios += "REALTIME|"; break;
          default: prios += "UNK|"; break;
        }
      }
      if (!prios.empty())
        prios.chop(1);
      else
        prios = "None";
      dump.aprintf(32, "%u:%s", &q - prop.data(), prios);
      dump += ",";
    }
    dump.pop_back();
    dump += "]";
    apdnf(dump);
  }
#endif

  static inline void print(const eastl::vector<VkQueueFamilyProperties> &qs, String &out)
  {
    String flags;
    String dump("Queues: [");
    for (auto &&q : qs)
    {
      flags.clear();
      if (q.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        flags += "G|";
      if (q.queueFlags & VK_QUEUE_COMPUTE_BIT)
        flags += "C|";
      if (q.queueFlags & VK_QUEUE_TRANSFER_BIT)
        flags += "T|";
      if (q.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
        flags += "S|";
      if (!flags.empty())
        flags.chop(1);
      else
        flags = "None";
      dump.aprintf(32, "%u:%s c%ut%u", &q - qs.data(), flags, q.queueCount, q.timestampValidBits);
      if (q.minImageTransferGranularity.width != 1 || q.minImageTransferGranularity.height != 1 ||
          q.minImageTransferGranularity.depth != 1)
        dump.aprintf(16, " %u-%u-%u", q.minImageTransferGranularity.width, q.minImageTransferGranularity.height,
          q.minImageTransferGranularity.depth);
      dump += ",";
    }
    dump.pop_back();
    dump += "]";
    apdnf(dump);
  }

  static inline const char *formatName(VkFormat fmt)
  {
    switch (fmt)
    {
      default: return "Unknown";
#define E_TO_N(e, v) \
  case VK_FORMAT_##e: return v
        E_TO_N(UNDEFINED, "Unk");
        E_TO_N(R4G4_UNORM_PACK8, "RG4UN_16");
        E_TO_N(R4G4B4A4_UNORM_PACK16, "RGBA4UN_16");
        E_TO_N(B4G4R4A4_UNORM_PACK16, "BGRA4UN_16");
        E_TO_N(R5G6B5_UNORM_PACK16, "RGB565UN_16");
        E_TO_N(B5G6R5_UNORM_PACK16, "BGR565UN_16");
        E_TO_N(R5G5B5A1_UNORM_PACK16, "RGB5A1UN_16");
        E_TO_N(B5G5R5A1_UNORM_PACK16, "BGR5A1UN_16");
        E_TO_N(A1R5G5B5_UNORM_PACK16, "A1RGB5UN_16");
        E_TO_N(R8_UNORM, "R8UN");
        E_TO_N(R8_SNORM, "R8SN");
        E_TO_N(R8_USCALED, "R8US");
        E_TO_N(R8_SSCALED, "R8SS");
        E_TO_N(R8_UINT, "R8UI");
        E_TO_N(R8_SINT, "R8SI");
        E_TO_N(R8_SRGB, "R8s");
        E_TO_N(R8G8_UNORM, "RG8UN");
        E_TO_N(R8G8_SNORM, "RG8SN");
        E_TO_N(R8G8_USCALED, "RG8US");
        E_TO_N(R8G8_SSCALED, "RG8SS");
        E_TO_N(R8G8_UINT, "RG8UI");
        E_TO_N(R8G8_SINT, "RG8SI");
        E_TO_N(R8G8_SRGB, "RG8s");
        E_TO_N(R8G8B8_UNORM, "RGB8UN");
        E_TO_N(R8G8B8_SNORM, "RGB8SN");
        E_TO_N(R8G8B8_USCALED, "RGB8US");
        E_TO_N(R8G8B8_SSCALED, "RGB8SS");
        E_TO_N(R8G8B8_UINT, "RGB8UI");
        E_TO_N(R8G8B8_SINT, "RGB8SI");
        E_TO_N(R8G8B8_SRGB, "RGB8s");
        E_TO_N(B8G8R8_UNORM, "BGR8UN");
        E_TO_N(B8G8R8_SNORM, "BGR8SN");
        E_TO_N(B8G8R8_USCALED, "BGR8US");
        E_TO_N(B8G8R8_SSCALED, "BGR8SS");
        E_TO_N(B8G8R8_UINT, "BGR8UI");
        E_TO_N(B8G8R8_SINT, "BGR8SI");
        E_TO_N(B8G8R8_SRGB, "BGR8s");
        E_TO_N(R8G8B8A8_UNORM, "RGBA8UN");
        E_TO_N(R8G8B8A8_SNORM, "RGBA8SN");
        E_TO_N(R8G8B8A8_USCALED, "RGBA8US");
        E_TO_N(R8G8B8A8_SSCALED, "RGBA8SS");
        E_TO_N(R8G8B8A8_UINT, "RGBA8UI");
        E_TO_N(R8G8B8A8_SINT, "RGBA8SI");
        E_TO_N(R8G8B8A8_SRGB, "RGBA8s");
        E_TO_N(B8G8R8A8_UNORM, "BGRA8UN");
        E_TO_N(B8G8R8A8_SNORM, "BGRA8SN");
        E_TO_N(B8G8R8A8_USCALED, "BGRA8US");
        E_TO_N(B8G8R8A8_SSCALED, "BGRA8SS");
        E_TO_N(B8G8R8A8_UINT, "BGRA8UI");
        E_TO_N(B8G8R8A8_SINT, "BGRA8SI");
        E_TO_N(B8G8R8A8_SRGB, "BGRA8s");
        E_TO_N(A8B8G8R8_UNORM_PACK32, "ABGR8UN_32");
        E_TO_N(A8B8G8R8_SNORM_PACK32, "ABGR8SN_32");
        E_TO_N(A8B8G8R8_USCALED_PACK32, "ABGR8US_32");
        E_TO_N(A8B8G8R8_SSCALED_PACK32, "ABGR8SS_32");
        E_TO_N(A8B8G8R8_UINT_PACK32, "ABGR8UI_32");
        E_TO_N(A8B8G8R8_SINT_PACK32, "ABGR8SI_32");
        E_TO_N(A8B8G8R8_SRGB_PACK32, "ABGR8s_32");
        E_TO_N(A2R10G10B10_UNORM_PACK32, "A2RGB10UN_32");
        E_TO_N(A2R10G10B10_SNORM_PACK32, "A2RGB10SN_32");
        E_TO_N(A2R10G10B10_USCALED_PACK32, "A2RGB10US_32");
        E_TO_N(A2R10G10B10_SSCALED_PACK32, "A2RGB10SS_32");
        E_TO_N(A2R10G10B10_UINT_PACK32, "A2RGB10UI_32");
        E_TO_N(A2R10G10B10_SINT_PACK32, "A2RGB10SI_32");
        E_TO_N(A2B10G10R10_UNORM_PACK32, "A2BGR10UN_32");
        E_TO_N(A2B10G10R10_SNORM_PACK32, "A2BGR10SN_32");
        E_TO_N(A2B10G10R10_USCALED_PACK32, "A2BGR10US_32");
        E_TO_N(A2B10G10R10_SSCALED_PACK32, "A2BGR10SS_32");
        E_TO_N(A2B10G10R10_UINT_PACK32, "A2BGR10UI_32");
        E_TO_N(A2B10G10R10_SINT_PACK32, "A2BGR10SI_32");
        E_TO_N(R16_UNORM, "R16UN");
        E_TO_N(R16_SNORM, "R16SN");
        E_TO_N(R16_USCALED, "R16US");
        E_TO_N(R16_SSCALED, "R16SS");
        E_TO_N(R16_UINT, "R16UI");
        E_TO_N(R16_SINT, "R16SI");
        E_TO_N(R16_SFLOAT, "R16SF");
        E_TO_N(R16G16_UNORM, "RG16UN");
        E_TO_N(R16G16_SNORM, "RG16SN");
        E_TO_N(R16G16_USCALED, "RG16US");
        E_TO_N(R16G16_SSCALED, "RG16SS");
        E_TO_N(R16G16_UINT, "RG16UI");
        E_TO_N(R16G16_SINT, "RG16SI");
        E_TO_N(R16G16_SFLOAT, "RG16SF");
        E_TO_N(R16G16B16_UNORM, "RGB16UN");
        E_TO_N(R16G16B16_SNORM, "RGB16SN");
        E_TO_N(R16G16B16_USCALED, "RGB16US");
        E_TO_N(R16G16B16_SSCALED, "RGB16SS");
        E_TO_N(R16G16B16_UINT, "RGB16UI");
        E_TO_N(R16G16B16_SINT, "RGB16SI");
        E_TO_N(R16G16B16_SFLOAT, "RGB16SF");
        E_TO_N(R16G16B16A16_UNORM, "RGBA16UN");
        E_TO_N(R16G16B16A16_SNORM, "RGBA16SN");
        E_TO_N(R16G16B16A16_USCALED, "RGBA16US");
        E_TO_N(R16G16B16A16_SSCALED, "RGBA16SS");
        E_TO_N(R16G16B16A16_UINT, "RGBA16UI");
        E_TO_N(R16G16B16A16_SINT, "RGBA16SI");
        E_TO_N(R16G16B16A16_SFLOAT, "RGBA16SF");
        E_TO_N(R32_UINT, "R32UI");
        E_TO_N(R32_SINT, "R32SI");
        E_TO_N(R32_SFLOAT, "R32SF");
        E_TO_N(R32G32_UINT, "RG32UI");
        E_TO_N(R32G32_SINT, "RG32SI");
        E_TO_N(R32G32_SFLOAT, "RG32SF");
        E_TO_N(R32G32B32_UINT, "RGB32UI");
        E_TO_N(R32G32B32_SINT, "RGB32SI");
        E_TO_N(R32G32B32_SFLOAT, "RGB32SF");
        E_TO_N(R32G32B32A32_UINT, "RGBA32UI");
        E_TO_N(R32G32B32A32_SINT, "RGBA32SI");
        E_TO_N(R32G32B32A32_SFLOAT, "RGBA32SF");
        E_TO_N(R64_UINT, "R64UI");
        E_TO_N(R64_SINT, "R64SI");
        E_TO_N(R64_SFLOAT, "R64SF");
        E_TO_N(R64G64_UINT, "RG64UI");
        E_TO_N(R64G64_SINT, "RG64SI");
        E_TO_N(R64G64_SFLOAT, "RG64SF");
        E_TO_N(R64G64B64_UINT, "RGB64UI");
        E_TO_N(R64G64B64_SINT, "RGB64SI");
        E_TO_N(R64G64B64_SFLOAT, "RGB64SF");
        E_TO_N(R64G64B64A64_UINT, "RGBA64UI");
        E_TO_N(R64G64B64A64_SINT, "RGBA64SI");
        E_TO_N(R64G64B64A64_SFLOAT, "RGBA64SF");
        E_TO_N(B10G11R11_UFLOAT_PACK32, "B10GR11UF_32");
        E_TO_N(E5B9G9R9_UFLOAT_PACK32, "E5BGR9UF_32");
        E_TO_N(D16_UNORM, "D16UN");
        E_TO_N(X8_D24_UNORM_PACK32, "X8D24UN_32");
        E_TO_N(D32_SFLOAT, "D32SF");
        E_TO_N(S8_UINT, "S8UI");
        E_TO_N(D16_UNORM_S8_UINT, "D16UNS8UI");
        E_TO_N(D24_UNORM_S8_UINT, "D32UNS8UI");
        E_TO_N(D32_SFLOAT_S8_UINT, "D32SFS8UI");
        E_TO_N(BC1_RGB_UNORM_BLOCK, "RGBUN_BC1");
        E_TO_N(BC1_RGB_SRGB_BLOCK, "RGBs_BC1");
        E_TO_N(BC1_RGBA_UNORM_BLOCK, "RGBAUN_BC1");
        E_TO_N(BC1_RGBA_SRGB_BLOCK, "RGBAs_BC1");
        E_TO_N(BC2_UNORM_BLOCK, "UN_BC2");
        E_TO_N(BC2_SRGB_BLOCK, "s_BC2");
        E_TO_N(BC3_UNORM_BLOCK, "UN_BC3");
        E_TO_N(BC3_SRGB_BLOCK, "s_BC3");
        E_TO_N(BC4_UNORM_BLOCK, "UN_BC4");
        E_TO_N(BC4_SNORM_BLOCK, "SN_BC4");
        E_TO_N(BC5_UNORM_BLOCK, "UN_BC5");
        E_TO_N(BC5_SNORM_BLOCK, "SN_BC5");
        E_TO_N(BC6H_UFLOAT_BLOCK, "UF_BC6H");
        E_TO_N(BC6H_SFLOAT_BLOCK, "SF_BC6H");
        E_TO_N(BC7_UNORM_BLOCK, "UN_BC7");
        E_TO_N(BC7_SRGB_BLOCK, "s_BC7");
        E_TO_N(ETC2_R8G8B8_UNORM_BLOCK, "RGB8UN_ETC2");
        E_TO_N(ETC2_R8G8B8_SRGB_BLOCK, "RGB8s_ETC2");
        E_TO_N(ETC2_R8G8B8A1_UNORM_BLOCK, "RGB8A1UN_ETC2");
        E_TO_N(ETC2_R8G8B8A1_SRGB_BLOCK, "RGB8A1s_ETC2");
        E_TO_N(ETC2_R8G8B8A8_UNORM_BLOCK, "RGBA8UN_ETC2");
        E_TO_N(ETC2_R8G8B8A8_SRGB_BLOCK, "RGBA8s_ETC2");
        E_TO_N(EAC_R11_UNORM_BLOCK, "R11UN_EAC");
        E_TO_N(EAC_R11_SNORM_BLOCK, "R11SN_EAC");
        E_TO_N(EAC_R11G11_UNORM_BLOCK, "RG11UN_EAC");
        E_TO_N(EAC_R11G11_SNORM_BLOCK, "RG11SN_EAC");
        E_TO_N(ASTC_4x4_UNORM_BLOCK, "UN_ASTC44");
        E_TO_N(ASTC_4x4_SRGB_BLOCK, "s_ASTC44");
        E_TO_N(ASTC_5x4_UNORM_BLOCK, "UN_ASTC54");
        E_TO_N(ASTC_5x4_SRGB_BLOCK, "s_ASTC54");
        E_TO_N(ASTC_5x5_UNORM_BLOCK, "UN_ASTC55");
        E_TO_N(ASTC_5x5_SRGB_BLOCK, "s_ASTC55");
        E_TO_N(ASTC_6x5_UNORM_BLOCK, "UN_ASTC65");
        E_TO_N(ASTC_6x5_SRGB_BLOCK, "s_ASTC65");
        E_TO_N(ASTC_6x6_UNORM_BLOCK, "UN_ASTC66");
        E_TO_N(ASTC_6x6_SRGB_BLOCK, "s_ASTC66");
        E_TO_N(ASTC_8x5_UNORM_BLOCK, "UN_ASTC85");
        E_TO_N(ASTC_8x5_SRGB_BLOCK, "s_ASTC85");
        E_TO_N(ASTC_8x6_UNORM_BLOCK, "UN_ASTC86");
        E_TO_N(ASTC_8x6_SRGB_BLOCK, "s_ASTC86");
        E_TO_N(ASTC_8x8_UNORM_BLOCK, "UN_ASTC88");
        E_TO_N(ASTC_8x8_SRGB_BLOCK, "s_ASTC88");
        E_TO_N(ASTC_10x5_UNORM_BLOCK, "UN_ASTC105");
        E_TO_N(ASTC_10x5_SRGB_BLOCK, "s_ASTC105");
        E_TO_N(ASTC_10x6_UNORM_BLOCK, "UN_ASTC106");
        E_TO_N(ASTC_10x6_SRGB_BLOCK, "s_ASTC106");
        E_TO_N(ASTC_10x8_UNORM_BLOCK, "UN_ASTC108");
        E_TO_N(ASTC_10x8_SRGB_BLOCK, "s_ASTC108");
        E_TO_N(ASTC_10x10_UNORM_BLOCK, "UN_ASTC1010");
        E_TO_N(ASTC_10x10_SRGB_BLOCK, "s_ASTC1010");
        E_TO_N(ASTC_12x10_UNORM_BLOCK, "UN_ASTC1210");
        E_TO_N(ASTC_12x10_SRGB_BLOCK, "s_ASTC1210");
        E_TO_N(ASTC_12x12_UNORM_BLOCK, "UN_ASTC1212");
        E_TO_N(ASTC_12x12_SRGB_BLOCK, "s_ASTC1212");
#undef E_TO_N
    }
  }

  static inline void formatFlagsToString(String &str, VkFormatFeatureFlags flags)
  {
    if (flags & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
      str += "Spl|";
    if (flags & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT)
    {
      if (flags & VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT)
        str += "UAV-A|";
      else
        str += "UAV|";
    }
    if (flags & VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT)
      str += "TBuf|";
    if (flags & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT)
    {
      if (flags & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT)
        str += "UAV-A TBuf|";
      else
        str += "UAV TBuf|";
    }
    if (flags & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT)
      str += "VBuf|";
    if (flags & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)
      str += "RT|";
    if (flags & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT)
      str += "RT-B|";
    if (flags & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
      str += "DS|";
    if ((flags & (VK_FORMAT_FEATURE_BLIT_DST_BIT | VK_FORMAT_FEATURE_BLIT_SRC_BIT)) ==
        (VK_FORMAT_FEATURE_BLIT_DST_BIT | VK_FORMAT_FEATURE_BLIT_SRC_BIT))
      str += "Blit|";
    else
    {
      if (flags & VK_FORMAT_FEATURE_BLIT_SRC_BIT)
        str += "BlitS|";
      if (flags & VK_FORMAT_FEATURE_BLIT_DST_BIT)
        str += "BlitD|";
    }
    if (flags & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)
      str += "SplL|";
    if (flags & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_CUBIC_BIT_IMG)
      str += "SplC|";

    if ((flags & (VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR | VK_FORMAT_FEATURE_TRANSFER_DST_BIT_KHR)) ==
        (VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR | VK_FORMAT_FEATURE_TRANSFER_DST_BIT_KHR))
      str += "Cpy|";
    else
    {
      if (flags & VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR)
        str += "CpyS|";
      if (flags & VK_FORMAT_FEATURE_TRANSFER_DST_BIT_KHR)
        str += "CpyD|";
    }
    if (str.empty())
      str += "None";
    else
      str.chop(1);
  }

  using UniqFeaturesMap = ska::flat_hash_map<VkFormatFeatureFlags, eastl::vector<VkFormat>>;

  static inline void print(const UniqFeaturesMap &masks, String &out, const char *feature_type)
  {
    String dump("");
    const size_t formatsPerLine = 16;
    for (auto i = masks.begin(); i != masks.end(); ++i)
    {
      if (i->first == 0)
        continue;
      size_t cnt = 0;
      for (VkFormat j : i->second)
      {
        if (cnt % formatsPerLine == 0)
        {
          if (cnt > 0)
          {
            dump.pop_back();
            apdnf(dump);
          }
          dump.clear();
          dump += feature_type;
          dump += " ";
          formatFlagsToString(dump, i->first);
          dump += ": ";
        }
        dump.aprintf(16, "%s", formatName(j));
        dump += ",";
        ++cnt;
      }
      if (dump.size())
      {
        dump.pop_back();
        apdnf(dump);
      }
    }
  }

  static inline void print(dag::ConstSpan<VkFormatProperties> fmt, String &out)
  {
    const VkFormatFeatureFlags usefullFlags =
      (VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT | VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT |
        VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT | VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT |
        VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT | VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT |
        VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT |
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT |
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_CUBIC_BIT_IMG |
        VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR | VK_FORMAT_FEATURE_TRANSFER_DST_BIT_KHR);

    UniqFeaturesMap linMasks;
    UniqFeaturesMap optMasks;
    UniqFeaturesMap bufMasks;
    for (int i = 0; i < fmt.size(); ++i)
    {
      linMasks[fmt[i].linearTilingFeatures & usefullFlags].push_back((VkFormat)i);
      optMasks[fmt[i].optimalTilingFeatures & usefullFlags].push_back((VkFormat)i);
      bufMasks[fmt[i].bufferFeatures & usefullFlags].push_back((VkFormat)i);
    }

    print(linMasks, out, "FMT lin");
    print(optMasks, out, "FMT opt");
    print(bufMasks, out, "FMT buf");
  }

  static inline void print(uint32_t i, VkMemoryHeap type, String &out)
  {
    String flags;
    if (type.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
      flags += "GPU";

    if (!flags.empty())
      flags.chop(3);
    else
      flags = "None";
    apd("Heap %i: %s, %u Gib, %u Mib, %u Kib, %u b", i, flags, type.size / 1024 / 1024 / 1024, type.size / 1024 / 1024,
      type.size / 1024, type.size);
  }

  static inline void print(const VkPhysicalDeviceMemoryProperties &mem, String &out)
  {
    String dump("Memory: [");
    for (uint32_t i = 0; i < mem.memoryTypeCount; ++i)
      dump.aprintf(48, "%u:%u %s.%03lX,", i, mem.memoryTypes[i].heapIndex, formatMemoryTypeFlags(mem.memoryTypes[i].propertyFlags),
        mem.memoryTypes[i].propertyFlags);
    dump.pop_back();
    dump += "]";
    apdnf(dump);
    dump = "Heaps: [";
    for (uint32_t i = 0; i < mem.memoryHeapCount; ++i)
      dump.aprintf(48, "%i:%s %.4gGib,", i, mem.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT ? "GPU" : "CPU",
        mem.memoryHeaps[i].size / 1024.0f / 1024.0f / 1024.0f);
    dump.pop_back();
    dump += "]";
    apdnf(dump);
  }

#if VK_EXT_memory_budget
  static inline void print(const VkPhysicalDeviceMemoryProperties &mem, const VkPhysicalDeviceMemoryBudgetPropertiesEXT &budget,
    String &out)
  {
    String dump("Budget: [");
    for (int i = 0; i != mem.memoryHeapCount; ++i)
      dump.aprintf(32, "%u:%uMb used %uMb,", i, budget.heapBudget[i] >> 20, budget.heapUsage[i] >> 20);
    dump.pop_back();
    dump += "]";
    apdnf(dump);
  }
#endif


#if VK_KHR_driver_properties
  static inline void print(const VkPhysicalDeviceDriverPropertiesKHR &drv_props, String &out)
  {
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

    apd("driver: %s %s [%s] API v%u.%u.%u.%u", drv_props.driverName, drv_props.driverInfo, drvTypeStr,
      drv_props.conformanceVersion.major, drv_props.conformanceVersion.minor, drv_props.conformanceVersion.subminor,
      drv_props.conformanceVersion.patch);
  }
#endif

  inline void printDisplays(String &out) const
  {
#if VK_KHR_display
    if (displays.empty())
    {
      apdnf("No displays found");
      return;
    }
    else
    {
      for (const VkDisplayPropertiesKHR &iter : displays)
        apd("Display %u:%s", &iter - displays.begin(), iter.displayName ? iter.displayName : "unknown");
      return;
    }
#else
    apdnf("No display ext");
#endif
  }

  inline void printExt(String &out) const
  {
    apd("conditionalRendering: %s, imagelessFramebuffer: %s, memoryBudget: %s, deviceMemoryReport: %s",
      boolToStr(hasConditionalRender), boolToStr(hasImagelessFramebuffer), boolToStr(memoryBudgetInfoAvailable),
      boolToStr(hasDeviceMemoryReport));
    apd("bufferDeviceAddress: %s, rayTracingPipeline: %s, rayQuery: %s, accelerationStructure: %s",
      boolToStr(hasDeviceBufferDeviceAddress), boolToStr(hasRayTracingPipeline), boolToStr(hasRayQuery),
      boolToStr(hasAccelerationStructure));
    apd("deviceFaultFeature: %s, deviceFaultVendorInfo: %s, depthStencilResolve: %s", boolToStr(hasDeviceFaultFeature),
      boolToStr(hasDeviceFaultVendorInfo), boolToStr(hasDepthStencilResolve));
#if VK_EXT_memory_budget
    if (memoryBudgetInfoAvailable)
      print(memoryProperties, memoryBudgetInfo, out);
#endif
#if D3D_HAS_RAY_TRACING
    apd("raytrace[ShaderHeaderSize, MaxRecursionDepth]: %u,%u", raytraceShaderHeaderSize, raytraceMaxRecursionDepth);
    apd("raytrace[TopAccelerationInstanceElementSize, ScratchBufferAlignment]: %u, %u", raytraceTopAccelerationInstanceElementSize,
      raytraceScratchBufferAlignment);
#endif
#if VK_EXT_descriptor_indexing
    apd("Bindless: %s, maxBindless[Textures,Samplers,Buffers,StorageBuffers]: %u,%u,%u,%u", boolToStr(hasBindless),
      maxBindlessTextures, maxBindlessSamplers, maxBindlessBuffers, maxBindlessStorageBuffers);
#endif
    apd("WarpSize: %u, Driver version: %u.%u.%u.%u", warpSize, driverVersionDecoded[0], driverVersionDecoded[1],
      driverVersionDecoded[2], driverVersionDecoded[3]);
    apd("hasLazyMemory: %s, hasShaderFloat16: %s, hasMemoryPriority: %s, hasPageableDeviceLocalMemory: %s", boolToStr(hasLazyMemory),
      boolToStr(hasShaderFloat16), boolToStr(hasMemoryPriority), boolToStr(hasPageableDeviceLocalMemory));
    apd("hasPipelineCreationCacheControl: %s", boolToStr(hasPipelineCreationCacheControl));
    apd("hasSixteenBitStorage: %s", boolToStr(hasSixteenBitStorage));
    apd("hasSynchronization2: %s, hasGlobalPriority: %s", boolToStr(hasSynchronization2), boolToStr(hasGlobalPriority));
  }

  inline void print(uint32_t device_index)
  {
    String out;
    apd("start %u", device_index);
    print(properties, out);
#if VK_KHR_driver_properties
    if (hasExtension<DriverPropertiesKHR>())
      print(driverProps, out);
#endif
    print(memoryProperties, out);
    print(features, out);
    print(queueFamilyProperties, out);
#if VK_KHR_global_priority
    if (hasGlobalPriority)
      print(queuesGlobalPriorityProperties, out);
#endif
    print(formatProperties, out);
    printExt(out);
    printDisplays(out);

    apd("end %u", device_index);
    out.pop_back();
    debug(out);
  }
#undef apd
#undef apdnf
};
} // namespace drv3d_vulkan