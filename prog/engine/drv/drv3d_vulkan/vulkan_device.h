// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_carray.h>
#include <generic/dag_tab.h>
#include <generic/dag_staticTab.h>
#include <EASTL/bitset.h>
#include <drv/3d/rayTrace/dag_drvRayTrace.h> // for D3D_HAS_RAY_TRACING
#include <drv_log_defs.h>

#include "driver.h"
#include "vulkan_instance.h"
#include "vulkan_loader.h"
#include "vk_entry_points.h"

// No need to have raytracing extension if we can not use it.
#if !D3D_HAS_RAY_TRACING
#undef VK_KHR_ray_tracing_pipeline
#undef VK_KHR_ray_query
#endif

namespace drv3d_vulkan
{
#if VK_KHR_imageless_framebuffer
VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(ImagelessFramebufferKHR);

VULKAN_DECLARE_EXTENSION(ImagelessFramebufferKHR, KHR_IMAGELESS_FRAMEBUFFER);
#endif

#if VK_KHR_shader_draw_parameters
VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(ShaderDrawParametersKHR);

VULKAN_DECLARE_EXTENSION(ShaderDrawParametersKHR, KHR_SHADER_DRAW_PARAMETERS);
#endif

#if VK_AMD_buffer_marker
VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkCmdWriteBufferMarkerAMD)

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkCmdWriteBufferMarkerAMD)
VULKAN_END_EXTENSION_FUCTION_PACK(BufferMarkerAMD);

VULKAN_DECLARE_EXTENSION(BufferMarkerAMD, AMD_BUFFER_MARKER);
#endif

#if VK_NV_device_diagnostic_checkpoints
VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkCmdSetCheckpointNV)
VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkGetQueueCheckpointDataNV)

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkCmdSetCheckpointNV)
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkGetQueueCheckpointDataNV)
VULKAN_END_EXTENSION_FUCTION_PACK(DiagnosticCheckpointsNV);

VULKAN_DECLARE_EXTENSION(DiagnosticCheckpointsNV, NV_DEVICE_DIAGNOSTIC_CHECKPOINTS);
#endif

#if VK_EXT_conservative_rasterization
VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(ConservativeRasterizationEXT);

VULKAN_DECLARE_EXTENSION(ConservativeRasterizationEXT, EXT_CONSERVATIVE_RASTERIZATION);
#endif

#if VK_KHR_acceleration_structure
DEFINE_VULKAN_NON_DISPATCHABLE_HANDLE(VulkanAccelerationStructureKHR, VkAccelerationStructureKHR);

#define VK_FUNC_LIST                      \
  VK_FUNC(vkCmdTraceRaysKHR)              \
  VK_FUNC(vkCmdTraceRaysIndirectKHR)      \
  VK_FUNC(vkCreateRayTracingPipelinesKHR) \
  VK_FUNC(vkGetRayTracingShaderGroupHandlesKHR)

#define VK_FUNC(name) VULKAN_MAKE_EXTENSION_FUNCTION_DEF(name)
VK_FUNC_LIST
#undef VK_FUNC

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
#define VK_FUNC(name) VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(name)
VK_FUNC_LIST
#undef VK_FUNC
VULKAN_END_EXTENSION_FUCTION_PACK(RaytracingPipelineKHR);

VULKAN_DECLARE_EXTENSION(RaytracingPipelineKHR, KHR_RAY_TRACING_PIPELINE);

#undef VK_FUNC_LIST

#endif // VK_KHR_ray_tracing_pipeline

#if VK_KHR_ray_query

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(RayQueryKHR);

VULKAN_DECLARE_EXTENSION(RayQueryKHR, KHR_RAY_QUERY);

#endif // VK_KHR_ray_query

#if VK_KHR_acceleration_structure // required by VK_KHR_ray_query

#define VK_FUNC_LIST                                        \
  VK_FUNC(vkBuildAccelerationStructuresKHR)                 \
  VK_FUNC(vkCmdBuildAccelerationStructuresIndirectKHR)      \
  VK_FUNC(vkCmdBuildAccelerationStructuresKHR)              \
  VK_FUNC(vkCmdCopyAccelerationStructureKHR)                \
  VK_FUNC(vkCmdCopyAccelerationStructureToMemoryKHR)        \
  VK_FUNC(vkCmdCopyMemoryToAccelerationStructureKHR)        \
  VK_FUNC(vkCmdWriteAccelerationStructuresPropertiesKHR)    \
  VK_FUNC(vkCopyAccelerationStructureKHR)                   \
  VK_FUNC(vkCopyAccelerationStructureToMemoryKHR)           \
  VK_FUNC(vkCopyMemoryToAccelerationStructureKHR)           \
  VK_FUNC(vkCreateAccelerationStructureKHR)                 \
  VK_FUNC(vkDestroyAccelerationStructureKHR)                \
  VK_FUNC(vkGetAccelerationStructureBuildSizesKHR)          \
  VK_FUNC(vkGetAccelerationStructureDeviceAddressKHR)       \
  VK_FUNC(vkGetDeviceAccelerationStructureCompatibilityKHR) \
  VK_FUNC(vkWriteAccelerationStructuresPropertiesKHR)

#define VK_FUNC(name) VULKAN_MAKE_EXTENSION_FUNCTION_DEF(name)
VK_FUNC_LIST
#undef VK_FUNC

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
#define VK_FUNC(name) VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(name)
VK_FUNC_LIST
#undef VK_FUNC
VULKAN_END_EXTENSION_FUCTION_PACK(AccelerationStructureKHR);

VULKAN_DECLARE_EXTENSION(AccelerationStructureKHR, KHR_ACCELERATION_STRUCTURE);

#undef VK_FUNC_LIST

#endif // VK_KHR_acceleration_structure

#if VK_EXT_descriptor_indexing // required by VK_KHR_acceleration_structure

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(DescriptorIndexingEXT);

VULKAN_DECLARE_EXTENSION(DescriptorIndexingEXT, EXT_DESCRIPTOR_INDEXING);

#endif // VK_EXT_descriptor_indexing

#if VK_KHR_maintenance3 // required by VK_EXT_descriptor_indexing

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(Maintenance3KHR);

VULKAN_DECLARE_EXTENSION(Maintenance3KHR, KHR_MAINTENANCE3);

#endif // VK_KHR_maintenance3

#if VK_KHR_buffer_device_address // required by VK_KHR_acceleration_structure

#define VK_FUNC_LIST                          \
  VK_FUNC(vkGetBufferDeviceAddressKHR)        \
  VK_FUNC(vkGetBufferOpaqueCaptureAddressKHR) \
  VK_FUNC(vkGetDeviceMemoryOpaqueCaptureAddressKHR)

#define VK_FUNC(name) VULKAN_MAKE_EXTENSION_FUNCTION_DEF(name)
VK_FUNC_LIST
#undef VK_FUNC

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
#define VK_FUNC(name) VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(name)
VK_FUNC_LIST
#undef VK_FUNC
VULKAN_END_EXTENSION_FUCTION_PACK(BufferDeviceAddressKHR);

VULKAN_DECLARE_EXTENSION(BufferDeviceAddressKHR, KHR_BUFFER_DEVICE_ADDRESS);

#endif // VK_KHR_buffer_device_address

#if VK_KHR_deferred_host_operations // required by VK_KHR_acceleration_structure

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(DeferredHostOperationsKHR);

VULKAN_DECLARE_EXTENSION(DeferredHostOperationsKHR, KHR_DEFERRED_HOST_OPERATIONS);

#endif // VK_KHR_deferred_host_operations

#if VK_KHR_shader_float_controls

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(ShaderFloatControlsKHR);

VULKAN_DECLARE_EXTENSION(ShaderFloatControlsKHR, KHR_SHADER_FLOAT_CONTROLS);

#endif // VK_KHR_shader_float_controls

#if VK_KHR_shader_float16_int8

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(ShaderFloat16Int8KHR);

VULKAN_DECLARE_EXTENSION(ShaderFloat16Int8KHR, KHR_SHADER_FLOAT16_INT8);

#endif // VK_KHR_shader_float16_int8

#if VK_KHR_spirv_1_4 // required by VK_KHR_ray_query

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(SPIRV_1_4_KHR);

VULKAN_DECLARE_EXTENSION(SPIRV_1_4_KHR, KHR_SPIRV_1_4);

#endif // VK_KHR_spirv_1_4

#if VK_KHR_device_group_creation // needed for VK_KHR_device_group

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(DeviceGroupCreationKHR);

VULKAN_DECLARE_EXTENSION(DeviceGroupCreationKHR, KHR_DEVICE_GROUP_CREATION);

#endif // VK_KHR_device_group_creation

#if VK_KHR_device_group // needed for VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(DeviceGroupKHR);

VULKAN_DECLARE_EXTENSION(DeviceGroupKHR, KHR_DEVICE_GROUP);

#endif // VK_KHR_device_group

#if VK_EXT_conditional_rendering
VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkCmdBeginConditionalRenderingEXT)
VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkCmdEndConditionalRenderingEXT)

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkCmdBeginConditionalRenderingEXT)
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkCmdEndConditionalRenderingEXT)
VULKAN_END_EXTENSION_FUCTION_PACK(ConditionalRenderingEXT);

VULKAN_DECLARE_EXTENSION(ConditionalRenderingEXT, EXT_CONDITIONAL_RENDERING);
#endif

#if VK_KHR_image_format_list
VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(ImageFormatListKHR);

VULKAN_DECLARE_EXTENSION(ImageFormatListKHR, KHR_IMAGE_FORMAT_LIST);
#endif

#if VK_KHR_descriptor_update_template
DEFINE_VULKAN_NON_DISPATCHABLE_HANDLE(VulkanDescriptorUpdateTemplateKHRHandle, VkDescriptorUpdateTemplateKHR);

VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkCreateDescriptorUpdateTemplateKHR)
VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkDestroyDescriptorUpdateTemplateKHR)
VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkUpdateDescriptorSetWithTemplateKHR)
VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkCmdPushDescriptorSetWithTemplateKHR)

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkCreateDescriptorUpdateTemplateKHR)
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkDestroyDescriptorUpdateTemplateKHR)
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkUpdateDescriptorSetWithTemplateKHR)
// VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkCmdPushDescriptorSetWithTemplateKHR)
VULKAN_END_EXTENSION_FUCTION_PACK(DescriptorUpdateTemplateKHR);

VULKAN_DECLARE_EXTENSION(DescriptorUpdateTemplateKHR, KHR_DESCRIPTOR_UPDATE_TEMPLATE);
#endif

#if VK_KHR_dedicated_allocation
VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(DedicatedAllocationKHR);

VULKAN_DECLARE_EXTENSION(DedicatedAllocationKHR, KHR_DEDICATED_ALLOCATION);
#endif

#if VK_KHR_get_memory_requirements2
VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkGetImageMemoryRequirements2KHR)
VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkGetBufferMemoryRequirements2KHR)
VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkGetImageSparseMemoryRequirements2KHR)

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkGetImageMemoryRequirements2KHR)
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkGetBufferMemoryRequirements2KHR)
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkGetImageSparseMemoryRequirements2KHR)
VULKAN_END_EXTENSION_FUCTION_PACK(GetMemoryRequirements2KHR);

VULKAN_DECLARE_EXTENSION(GetMemoryRequirements2KHR, KHR_GET_MEMORY_REQUIREMENTS_2);
#endif

#if VK_KHR_maintenance1
VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(Maintenance1KHR);

VULKAN_DECLARE_EXTENSION(Maintenance1KHR, KHR_MAINTENANCE1);
#endif

#if VK_KHR_maintenance2
VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(Maintenance2KHR);

VULKAN_DECLARE_EXTENSION(Maintenance2KHR, KHR_MAINTENANCE2);
#endif

#if VK_AMD_negative_viewport_height
VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(NegativeViewportHeightAMD);

VULKAN_DECLARE_EXTENSION(NegativeViewportHeightAMD, AMD_NEGATIVE_VIEWPORT_HEIGHT);
#endif

DEFINE_VULKAN_NON_DISPATCHABLE_HANDLE(VulkanSwapchainKHRHandle, VkSwapchainKHR);

VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkCreateSharedSwapchainsKHR)

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkCreateSharedSwapchainsKHR)
VULKAN_END_EXTENSION_FUCTION_PACK(DisplaySwapchainKHR);

VULKAN_DECLARE_EXTENSION(DisplaySwapchainKHR, KHR_DISPLAY_SWAPCHAIN);


VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkCreateSwapchainKHR)
VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkDestroySwapchainKHR)
VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkGetSwapchainImagesKHR)
VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkAcquireNextImageKHR)
VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkQueuePresentKHR)

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkCreateSwapchainKHR)
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkDestroySwapchainKHR)
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkGetSwapchainImagesKHR)
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkAcquireNextImageKHR)
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkQueuePresentKHR)
VULKAN_END_EXTENSION_FUCTION_PACK(SwapchainKHR);

VULKAN_DECLARE_EXTENSION(SwapchainKHR, KHR_SWAPCHAIN);

VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkCmdPipelineBarrier2KHR)

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkCmdPipelineBarrier2KHR)
VULKAN_END_EXTENSION_FUCTION_PACK(Synchronization2KHR);

VULKAN_DECLARE_EXTENSION(Synchronization2KHR, KHR_SYNCHRONIZATION_2);

#if VK_EXT_debug_marker
VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkDebugMarkerSetObjectTagEXT)
VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkDebugMarkerSetObjectNameEXT)
VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkCmdDebugMarkerBeginEXT)
VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkCmdDebugMarkerEndEXT)
VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkCmdDebugMarkerInsertEXT)

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkDebugMarkerSetObjectTagEXT)
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkDebugMarkerSetObjectNameEXT)
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkCmdDebugMarkerBeginEXT)
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkCmdDebugMarkerEndEXT)
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkCmdDebugMarkerInsertEXT)
VULKAN_END_EXTENSION_FUCTION_PACK(DebugMarkerEXT);

VULKAN_DECLARE_EXTENSION(DebugMarkerEXT, EXT_DEBUG_MARKER);
#endif

#if VK_EXT_memory_budget
VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(MemoryBudgetEXT);

VULKAN_DECLARE_EXTENSION(MemoryBudgetEXT, EXT_MEMORY_BUDGET);
#endif

#if VK_EXT_device_memory_report
VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(DeviceMemoryReportEXT);

VULKAN_DECLARE_EXTENSION(DeviceMemoryReportEXT, EXT_DEVICE_MEMORY_REPORT);
#endif

#if VK_EXT_pipeline_creation_feedback
VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(PipelineCreationFeedbackReportEXT);

VULKAN_DECLARE_EXTENSION(PipelineCreationFeedbackReportEXT, EXT_PIPELINE_CREATION_FEEDBACK);
#endif

#if VK_EXT_calibrated_timestamps
VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkGetCalibratedTimestampsEXT)
VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkGetPhysicalDeviceCalibrateableTimeDomainsEXT)

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkGetCalibratedTimestampsEXT)
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkGetPhysicalDeviceCalibrateableTimeDomainsEXT)
VULKAN_END_EXTENSION_FUCTION_PACK(CalibratedTimestampsEXT);

VULKAN_DECLARE_EXTENSION(CalibratedTimestampsEXT, EXT_CALIBRATED_TIMESTAMPS);
#endif

#if VK_KHR_driver_properties
VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(DriverPropertiesKHR);

VULKAN_DECLARE_EXTENSION(DriverPropertiesKHR, KHR_DRIVER_PROPERTIES);
#endif

#if VK_EXT_load_store_op_none
VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(LoadStoreOpNoneEXT);

VULKAN_DECLARE_EXTENSION(LoadStoreOpNoneEXT, EXT_LOAD_STORE_OP_NONE);
#endif

#if VK_QCOM_render_pass_store_ops
VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(RenderPassStoreOpsQCOM);

VULKAN_DECLARE_EXTENSION(RenderPassStoreOpsQCOM, QCOM_RENDER_PASS_STORE_OPS);
#endif

#if _TARGET_ANDROID && VK_GOOGLE_display_timing
VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(GoogleDisplayTime);

VULKAN_DECLARE_EXTENSION(GoogleDisplayTime, GOOGLE_DISPLAY_TIMING);
#endif

#if VK_EXT_device_fault
VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkGetDeviceFaultInfoEXT)

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkGetDeviceFaultInfoEXT)
VULKAN_END_EXTENSION_FUCTION_PACK(DeviceFaultEXT);

VULKAN_DECLARE_EXTENSION(DeviceFaultEXT, EXT_DEVICE_FAULT);
#endif

#if VK_KHR_depth_stencil_resolve
VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(DepthStencilResolveKHR);
VULKAN_DECLARE_EXTENSION(DepthStencilResolveKHR, KHR_DEPTH_STENCIL_RESOLVE);
#endif

#if VK_KHR_create_renderpass2
VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkCreateRenderPass2KHR)
VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkCreateRenderPass2KHR)
VULKAN_END_EXTENSION_FUCTION_PACK(CreateRenderPass2KHR);
VULKAN_DECLARE_EXTENSION(CreateRenderPass2KHR, KHR_CREATE_RENDERPASS_2);
#endif

#if VK_KHR_multiview
VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(MultiviewKHR);
VULKAN_DECLARE_EXTENSION(MultiviewKHR, KHR_MULTIVIEW);
#endif

#if VK_EXT_memory_priority
VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(MemoryPriorityEXT);
VULKAN_DECLARE_EXTENSION(MemoryPriorityEXT, EXT_MEMORY_PRIORITY);
#endif

#if VK_EXT_pageable_device_local_memory
VULKAN_MAKE_EXTENSION_FUNCTION_DEF(vkSetDeviceMemoryPriorityEXT)

VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_EXTENSION_FUNCTION_PACK_ENTRY(vkSetDeviceMemoryPriorityEXT)
VULKAN_END_EXTENSION_FUCTION_PACK(PageableDeviceLocalMemoryEXT);
VULKAN_DECLARE_EXTENSION(PageableDeviceLocalMemoryEXT, EXT_PAGEABLE_DEVICE_LOCAL_MEMORY);
#endif

#if VK_EXT_pipeline_creation_cache_control
VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(PipelineCreationCacheControlEXT);
VULKAN_DECLARE_EXTENSION(PipelineCreationCacheControlEXT, EXT_PIPELINE_CREATION_CACHE_CONTROL);
#endif

#if VK_KHR_sampler_mirror_clamp_to_edge
VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(SamplerMirrorClampToEdgeKHR);
VULKAN_DECLARE_EXTENSION(SamplerMirrorClampToEdgeKHR, KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE);
#endif

#if VK_KHR_16bit_storage
VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(SixteenBitStorageKHR);
VULKAN_DECLARE_EXTENSION(SixteenBitStorageKHR, KHR_16BIT_STORAGE);
#endif

#if VK_KHR_global_priority
VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(GlobalPriorityKHR);
VULKAN_DECLARE_EXTENSION(GlobalPriorityKHR, KHR_GLOBAL_PRIORITY);
#endif

#if VK_EXT_full_screen_exclusive
VULKAN_BEGIN_EXTENSION_FUNCTION_PACK
VULKAN_END_EXTENSION_FUCTION_PACK(FullScreenExclusiveEXT);
VULKAN_DECLARE_EXTENSION(FullScreenExclusiveEXT, EXT_FULL_SCREEN_EXCLUSIVE);
#endif

template <typename... Extensions>
class VulkanDeviceCore : public Extensions...
{
  typedef TypePack<Extensions...> ExtensionTypePack;
  typedef eastl::bitset<sizeof...(Extensions)> ExtensionBitSetType;
  VulkanInstance *instance = nullptr;
  VulkanDeviceHandle device;
  ExtensionBitSetType extensionEnabled;

public:
  VulkanDeviceCore(const VulkanDeviceCore &) = delete;
  VulkanDeviceCore &operator=(const VulkanDeviceCore &) = delete;

  VulkanDeviceCore() = default;
  ~VulkanDeviceCore()
  {
    if (!is_null(device))
      VULKAN_LOG_CALL(vkDestroyDevice(device, nullptr));
  }

  inline VulkanInstance &getInstance() const { return *instance; }
  inline VulkanDeviceHandle get() const { return device; }
  inline bool isValid() const { return !is_null(device); }
#define VK_DEFINE_ENTRY_POINT(name) PFN_##name name = nullptr;
  VK_DEVICE_ENTRY_POINT_LIST
#undef VK_DEFINE_ENTRY_POINT

private:
  template <typename T, typename T2, typename... L>
  void extensionInit(const VkDeviceCreateInfo &dci)
  {
    // see disableExtension for explanation why explicit this
    extensionInit<T>(dci);
    extensionInit<T2, L...>(dci);
  }
  template <typename T>
  void extensionInit(const VkDeviceCreateInfo &dci)
  {
    auto listStart = dci.ppEnabledExtensionNames;
    auto listEnd = listStart + dci.enabledExtensionCount;
    auto extRef = eastl::find_if(listStart, listEnd, [](const char *e_name) { return 0 == strcmp(e_name, extension_name<T>()); });
    bool loaded = false;
    if (extRef != listEnd)
    {
      loaded = true;
      this->T::enumerateFunctions([this, &loaded](const char *f_name, PFN_vkVoidFunction &function) {
        function = VULKAN_LOG_CALL_R(instance->getLoader().vkGetDeviceProcAddr(device, f_name));

        // try to find function in instance, if it is not device tied
        if (!function)
          function = VULKAN_LOG_CALL_R(instance->getLoader().vkGetInstanceProcAddr(instance->get(), f_name));

        if (!function)
        {
          D3D_ERROR("vulkan: ext %s advertised as supported, but proc %s was not loaded", extension_name<T>(), f_name);
          loaded = false;
        }
      });
    }
    extensionEnabled.set(TypeIndexOf<T, ExtensionTypePack>::value, loaded);
  }
  template <typename T, typename T2, typename... L>
  void extensionShutdown()
  {
    extensionShutdown<T2, L...>();
    extensionShutdown<T>();
  }
  template <typename T>
  void extensionShutdown()
  {
    if (extensionEnabled.test(TypeIndexOf<T, ExtensionTypePack>::value))
    {
      this->T::enumerateFunctions([](const char *, PFN_vkVoidFunction &function) { function = nullptr; });
      extensionEnabled.reset(TypeIndexOf<T, ExtensionTypePack>::value);
    }
  }

  template <typename U, typename T, typename T2, typename... L>
  void extensionNameIterate(U clb)
  {
    if (hasExtension<T>())
      clb(extension_name<T>());
    extensionNameIterate<U, T2, L...>(clb);
  }
  template <typename U, typename T>
  void extensionNameIterate(U clb)
  {
    if (hasExtension<T>())
      clb(extension_name<T>());
  }

public:
  bool init(VulkanInstance *vk_instance, VulkanPhysicalDeviceHandle pd, const VkDeviceCreateInfo &dci)
  {
    instance = vk_instance;

    VkResult rc = vk_instance->vkCreateDevice(pd, &dci, nullptr, ptr(device));
    if (VULKAN_CHECK_FAIL(rc))
    {
      logwarn("vulkan: device create failed with error %08lX:%s", rc, vulkan_error_string(rc));
      return false;
    }

#define VK_DEFINE_ENTRY_POINT(name)                                                                                                \
  *reinterpret_cast<PFN_vkVoidFunction *>(&name) = VULKAN_LOG_CALL_R(vk_instance->getLoader().vkGetDeviceProcAddr(device, #name)); \
  if (!name)                                                                                                                       \
  {                                                                                                                                \
    logwarn("vulkan: no device entrypoint for %s", #name);                                                                         \
    shutdown();                                                                                                                    \
    return false;                                                                                                                  \
  }
    VK_DEVICE_ENTRY_POINT_LIST
#undef VK_DEFINE_ENTRY_POINT

    extensionInit<Extensions...>(dci);

    return true;
  }
  void shutdown()
  {
    extensionShutdown<Extensions...>();

    if (!is_null(device))
    {
      vkDestroyDevice(device, nullptr);
      device = VulkanNullHandle();
    }
#define VK_DEFINE_ENTRY_POINT(name) *reinterpret_cast<PFN_vkVoidFunction *>(&name) = nullptr;
    VK_DEVICE_ENTRY_POINT_LIST
#undef VK_DEFINE_ENTRY_POINT
  }
  template <typename T>
  void disableExtension()
  {
    extensionShutdown<T>();
  }
  template <typename T>
  bool hasExtension() const
  {
    return extensionEnabled.test(TypeIndexOf<T, ExtensionTypePack>::value);
  }
  template <typename T>
  void iterateEnabledExtensionNames(T clb)
  {
    extensionNameIterate<T, Extensions...>(clb);
  }
  static constexpr size_t ExtensionCount = sizeof...(Extensions);
  static const char *ExtensionNames[ExtensionCount];
};

template <typename... Extensions>
const char *VulkanDeviceCore<Extensions...>::ExtensionNames[VulkanDeviceCore<Extensions...>::ExtensionCount] = //
  {extension_name<Extensions>()...};

class VulkanDevice : public VulkanDeviceCore<SwapchainKHR
// no need for device display for now, only works on linux and only for fullscreen, but fullscreen
// works without it
#if 0
    , DisplaySwapchainKHR
#endif
#if VK_EXT_debug_marker
                       ,
                       DebugMarkerEXT
#endif
#if VK_AMD_negative_viewport_height
                       ,
                       NegativeViewportHeightAMD
#endif
#if VK_KHR_maintenance1
                       ,
                       Maintenance1KHR
#endif
#if VK_KHR_maintenance2
                       ,
                       Maintenance2KHR
#endif
#if VK_KHR_get_memory_requirements2
                       ,
                       GetMemoryRequirements2KHR
#endif
#if VK_KHR_dedicated_allocation
                       ,
                       DedicatedAllocationKHR
#endif
#if VK_KHR_descriptor_update_template
                       ,
                       DescriptorUpdateTemplateKHR
#endif
#if VK_KHR_image_format_list
                       ,
                       ImageFormatListKHR
#endif
#if VK_KHR_ray_tracing_pipeline
                       ,
                       RaytracingPipelineKHR
#endif
#if VK_KHR_ray_query
                       ,
                       RayQueryKHR
#endif
#if VK_KHR_acceleration_structure
                       ,
                       AccelerationStructureKHR
#endif
#if VK_EXT_descriptor_indexing
                       ,
                       DescriptorIndexingEXT
#endif
#if VK_KHR_maintenance3
                       ,
                       Maintenance3KHR
#endif
#if VK_KHR_buffer_device_address
                       ,
                       BufferDeviceAddressKHR
#endif
#if VK_KHR_deferred_host_operations
                       ,
                       DeferredHostOperationsKHR
#endif
#if VK_KHR_shader_float_controls
                       ,
                       ShaderFloatControlsKHR
#endif
#if VK_KHR_shader_float16_int8
                       ,
                       ShaderFloat16Int8KHR
#endif
#if VK_KHR_spirv_1_4
                       ,
                       SPIRV_1_4_KHR
#endif
#if VK_KHR_device_group_creation
                       ,
                       DeviceGroupCreationKHR
#endif
#if VK_KHR_device_group
                       ,
                       DeviceGroupKHR
#endif
#if VK_EXT_conditional_rendering
                       ,
                       ConditionalRenderingEXT
#endif
#if VK_NV_device_diagnostic_checkpoints
                       ,
                       DiagnosticCheckpointsNV
#endif
#if VK_AMD_buffer_marker
                       ,
                       BufferMarkerAMD
#endif
#if VK_KHR_imageless_framebuffer
                       ,
                       ImagelessFramebufferKHR
#endif
#if VK_KHR_shader_draw_parameters
                       ,
                       ShaderDrawParametersKHR
#endif
#if VK_EXT_conservative_rasterization
                       ,
                       ConservativeRasterizationEXT
#endif
#if VK_EXT_memory_budget
                       ,
                       MemoryBudgetEXT
#endif
#if VK_EXT_device_memory_report
                       ,
                       DeviceMemoryReportEXT
#endif
#if VK_EXT_pipeline_creation_feedback
                       ,
                       PipelineCreationFeedbackReportEXT
#endif
#if VK_EXT_calibrated_timestamps
                       ,
                       CalibratedTimestampsEXT
#endif
#if VK_KHR_driver_properties
                       ,
                       DriverPropertiesKHR
#endif
#if VK_EXT_load_store_op_none
                       ,
                       LoadStoreOpNoneEXT
#endif
#if VK_QCOM_render_pass_store_ops
                       ,
                       RenderPassStoreOpsQCOM
#endif
#if _TARGET_ANDROID && VK_GOOGLE_display_timing
                       ,
                       GoogleDisplayTime
#endif
#if VK_EXT_device_fault
                       ,
                       DeviceFaultEXT
#endif
#if VK_KHR_depth_stencil_resolve
                       ,
                       DepthStencilResolveKHR
#endif
#if VK_KHR_create_renderpass2
                       ,
                       CreateRenderPass2KHR
#endif
#if VK_KHR_multiview
                       ,
                       MultiviewKHR
#endif
#if VK_EXT_pageable_device_local_memory
                       ,
                       PageableDeviceLocalMemoryEXT
#endif
#if VK_EXT_memory_priority
                       ,
                       MemoryPriorityEXT
#endif
#if VK_EXT_pipeline_creation_cache_control
                       ,
                       PipelineCreationCacheControlEXT
#endif
#if VK_KHR_sampler_mirror_clamp_to_edge
                       ,
                       SamplerMirrorClampToEdgeKHR
#endif
#if VK_KHR_16bit_storage
                       ,
                       SixteenBitStorageKHR
#endif
#if VK_KHR_synchronization2
                       ,
                       Synchronization2KHR
#endif
#if VK_KHR_global_priority
                       ,
                       GlobalPriorityKHR
#endif
#if VK_EXT_full_screen_exclusive
                       ,
                       FullScreenExclusiveEXT
#endif
                       >
{
public:
  VulkanDevice(const VulkanDevice &) = delete;
  VulkanDevice &operator=(const VulkanDevice &) = delete;

  VulkanDevice() = default;
  ~VulkanDevice() {}
};

// checks if all required extensions are in the given list and returns true if they are all
// included.
bool has_all_required_extension(dag::ConstSpan<const char *> ext_list);
// removes mutual exclusive extensions from the list and returns the new list length
int remove_mutual_exclusive_extensions(dag::Span<const char *> ext_list);
bool device_has_all_required_extensions(VulkanDevice &device, void (*clb)(const char *name));
uint32_t fill_device_extension_list(Tab<const char *> &result, const StaticTab<const char *, VulkanDevice::ExtensionCount> &white_list,
  const eastl::vector<VkExtensionProperties> &device_extensions);

struct MemoryRequirementInfo
{
  VkMemoryRequirements requirements;
  bool prefersDedicatedAllocation : 1;
  bool requiresDedicatedAllocation : 1;

  bool needDedicated() const { return prefersDedicatedAllocation || requiresDedicatedAllocation; }
};

MemoryRequirementInfo get_memory_requirements(VulkanDevice &device, VulkanBufferHandle buffer);
MemoryRequirementInfo get_memory_requirements(VulkanDevice &device, VulkanImageHandle image);

} // namespace drv3d_vulkan
