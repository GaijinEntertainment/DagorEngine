// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "vk_to_string.h"

String drv3d_vulkan::formatImageUsageFlags(VkImageUsageFlags flags)
{
  String ret;
  if (flags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
    ret.append("transfer_src ");
  if (flags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
    ret.append("transfer_dst ");
  if (flags & VK_IMAGE_USAGE_SAMPLED_BIT)
    ret.append("sampled ");
  if (flags & VK_IMAGE_USAGE_STORAGE_BIT)
    ret.append("storage ");
  if (flags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
    ret.append("color ");
  if (flags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
    ret.append("depth ");
  if (flags & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
    ret.append("transient ");
  if (flags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
    ret.append("input ");
#if VK_NV_shading_rate_image
  if (flags & VK_IMAGE_USAGE_SHADING_RATE_IMAGE_BIT_NV)
    ret.append("shading_rate ");
#endif
#if VK_EXT_fragment_density_map
  if (flags & VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT)
    ret.append("frag_density ");
#endif
  return ret;
}

String drv3d_vulkan::formatBufferUsageFlags(VkBufferUsageFlags flags)
{
  String ret;
  if (flags & VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
    ret.append("src ");
  if (flags & VK_BUFFER_USAGE_TRANSFER_DST_BIT)
    ret.append("dst ");
  if (flags & VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT)
    ret.append("utb ");
  if (flags & VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT)
    ret.append("stb ");
  if (flags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
    ret.append("ubo ");
  if (flags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
    ret.append("sbo ");
  if (flags & VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
    ret.append("index ");
  if (flags & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
    ret.append("vertex ");
  if (flags & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT)
    ret.append("indirect ");
  if (flags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
    ret.append("addr ");
  if (flags & VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT)
    ret.append("tfb ");
  if (flags & VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT)
    ret.append("tfc ");
  if (flags & VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT)
    ret.append("cond ");
  if (flags & VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR)
    ret.append("acbuild ");
  if (flags & VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR)
    ret.append("acstore ");
  if (flags & VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR)
    ret.append("binding ");
#ifdef VK_ENABLE_BETA_EXTENSIONS
  if (flags & VK_BUFFER_USAGE_VIDEO_ENCODE_DST_BIT_KHR = 0x00008000,
    ret.append("vedst ");
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
  if (flags & VK_BUFFER_USAGE_VIDEO_ENCODE_SRC_BIT_KHR = 0x00010000,
    ret.append("vesrc ");
#endif

  // switch uses some old headers
#if !_TARGET_C3
  if (flags & VK_BUFFER_USAGE_VIDEO_DECODE_SRC_BIT_KHR)
    ret.append("vdsrc ");
  if (flags & VK_BUFFER_USAGE_VIDEO_DECODE_DST_BIT_KHR)
    ret.append("vddst ");
  if (flags & VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT)
    ret.append("sdesc ");
  if (flags & VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT)
    ret.append("rdesc ");
  if (flags & VK_BUFFER_USAGE_PUSH_DESCRIPTORS_DESCRIPTOR_BUFFER_BIT_EXT)
    ret.append("pddesc ");
  if (flags & VK_BUFFER_USAGE_MICROMAP_BUILD_INPUT_READ_ONLY_BIT_EXT)
    ret.append("mmbuild ");
  if (flags & VK_BUFFER_USAGE_MICROMAP_STORAGE_BIT_EXT)
    ret.append("mmstore ");
#endif

  return ret;
}

String drv3d_vulkan::formatPipelineStageFlags(VkPipelineStageFlags flags)
{
  String ret;
  bool nonEmpty = false;
#define APPEND_FLAG(x)   \
  if (flags & x)         \
  {                      \
    if (nonEmpty)        \
      ret.append(" | "); \
    ret.append(#x);      \
    nonEmpty = true;     \
  }

  APPEND_FLAG(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
  APPEND_FLAG(VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);
  APPEND_FLAG(VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);
  APPEND_FLAG(VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
  APPEND_FLAG(VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT);
  APPEND_FLAG(VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT);
  APPEND_FLAG(VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT);
  APPEND_FLAG(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
  APPEND_FLAG(VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);
  APPEND_FLAG(VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
  APPEND_FLAG(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
  APPEND_FLAG(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
  APPEND_FLAG(VK_PIPELINE_STAGE_TRANSFER_BIT);
  APPEND_FLAG(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
  APPEND_FLAG(VK_PIPELINE_STAGE_HOST_BIT);
  APPEND_FLAG(VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
  APPEND_FLAG(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
#if (VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query)
  APPEND_FLAG(VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR);
#endif

  ret.append(String(32, "(%lu)", flags));

  return ret;

#undef APPEND_FLAG
}

String drv3d_vulkan::formatMemoryAccessFlags(VkAccessFlags flags)
{
  String ret;
  bool nonEmpty = false;
#define APPEND_FLAG(x)   \
  if (flags & x)         \
  {                      \
    if (nonEmpty)        \
      ret.append(" | "); \
    ret.append(#x);      \
    nonEmpty = true;     \
  }

  APPEND_FLAG(VK_ACCESS_INDIRECT_COMMAND_READ_BIT);
  APPEND_FLAG(VK_ACCESS_INDEX_READ_BIT);
  APPEND_FLAG(VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
  APPEND_FLAG(VK_ACCESS_UNIFORM_READ_BIT);
  APPEND_FLAG(VK_ACCESS_INPUT_ATTACHMENT_READ_BIT);
  APPEND_FLAG(VK_ACCESS_SHADER_READ_BIT);
  APPEND_FLAG(VK_ACCESS_SHADER_WRITE_BIT);
  APPEND_FLAG(VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);
  APPEND_FLAG(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
  APPEND_FLAG(VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT);
  APPEND_FLAG(VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
  APPEND_FLAG(VK_ACCESS_TRANSFER_READ_BIT);
  APPEND_FLAG(VK_ACCESS_TRANSFER_WRITE_BIT);
  APPEND_FLAG(VK_ACCESS_HOST_READ_BIT);
  APPEND_FLAG(VK_ACCESS_HOST_WRITE_BIT);
  APPEND_FLAG(VK_ACCESS_MEMORY_READ_BIT);
  APPEND_FLAG(VK_ACCESS_MEMORY_WRITE_BIT);

  ret.append(String(32, "(%lu)", flags));

  return ret;
#undef APPEND_FLAG
}

const char *drv3d_vulkan::formatImageLayout(VkImageLayout layout)
{

#define CASE_ELEMENT(x) \
  case x: return #x

  switch (layout)
  {
    CASE_ELEMENT(VK_IMAGE_LAYOUT_UNDEFINED);
    CASE_ELEMENT(VK_IMAGE_LAYOUT_GENERAL);
    CASE_ELEMENT(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    CASE_ELEMENT(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    CASE_ELEMENT(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
    CASE_ELEMENT(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    CASE_ELEMENT(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    CASE_ELEMENT(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    CASE_ELEMENT(VK_IMAGE_LAYOUT_PREINITIALIZED);
    default: break;
  }

  return "unknown_layout";

#undef CASE_ELEMENT
}

String drv3d_vulkan::formatMemoryTypeFlags(VkMemoryPropertyFlags propertyFlags)
{
  String flags;
  VkMemoryPropertyFlags knownFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT |
                                     VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT | VK_MEMORY_PROPERTY_PROTECTED_BIT;

  if (propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    flags += "GPU|";
  if (propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    flags += "CPU|";
  if (propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    flags += "CO|";
  if (propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
    flags += "CA|";
  if (propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
    flags += "FB|";
  if (propertyFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT)
    flags += "P|";
  if (propertyFlags & (~knownFlags))
    flags += "U|";

  if (!flags.empty())
    flags.chop(1);
  else
    flags = "None";

  return flags;
}

#define ENUM_TO_NAME(Name) \
  case Name: enumName = #Name; break

const char *drv3d_vulkan::formatPrimitiveTopology(VkPrimitiveTopology top)
{
  const char *enumName = "<unknown>";
  switch (top)
  {
    ENUM_TO_NAME(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
    ENUM_TO_NAME(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
    ENUM_TO_NAME(VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);
    ENUM_TO_NAME(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    ENUM_TO_NAME(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
    ENUM_TO_NAME(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN);
    ENUM_TO_NAME(VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY);
    ENUM_TO_NAME(VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY);
    ENUM_TO_NAME(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY);
    ENUM_TO_NAME(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY);
    ENUM_TO_NAME(VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
    default: break; // silence stupid -Wswitch
  }
  return enumName;
}


const char *drv3d_vulkan::formatAttachmentLoadOp(const VkAttachmentLoadOp load_op)
{
  switch (load_op)
  {
    case VK_ATTACHMENT_LOAD_OP_LOAD: return "load";
    case VK_ATTACHMENT_LOAD_OP_CLEAR: return "clear";
    case VK_ATTACHMENT_LOAD_OP_DONT_CARE: return "dont_care";
    default: return "unknown";
  }
  return "unknown";
}

const char *drv3d_vulkan::formatAttachmentStoreOp(const VkAttachmentStoreOp store_op)
{
  switch (store_op)
  {
    case VK_ATTACHMENT_STORE_OP_STORE: return "store";
    case VK_ATTACHMENT_STORE_OP_DONT_CARE: return "dont_care";
    case VK_ATTACHMENT_STORE_OP_NONE: return "none";
    default: return "unknown";
  }
  return "unknown";
}

const char *drv3d_vulkan::formatObjectType(VkObjectType obj_type)
{
  const char *enumName = "<unknown>";
  switch (obj_type)
  {
    ENUM_TO_NAME(VK_OBJECT_TYPE_UNKNOWN);
    ENUM_TO_NAME(VK_OBJECT_TYPE_INSTANCE);
    ENUM_TO_NAME(VK_OBJECT_TYPE_PHYSICAL_DEVICE);
    ENUM_TO_NAME(VK_OBJECT_TYPE_DEVICE);
    ENUM_TO_NAME(VK_OBJECT_TYPE_QUEUE);
    ENUM_TO_NAME(VK_OBJECT_TYPE_SEMAPHORE);
    ENUM_TO_NAME(VK_OBJECT_TYPE_COMMAND_BUFFER);
    ENUM_TO_NAME(VK_OBJECT_TYPE_FENCE);
    ENUM_TO_NAME(VK_OBJECT_TYPE_DEVICE_MEMORY);
    ENUM_TO_NAME(VK_OBJECT_TYPE_BUFFER);
    ENUM_TO_NAME(VK_OBJECT_TYPE_IMAGE);
    ENUM_TO_NAME(VK_OBJECT_TYPE_EVENT);
    ENUM_TO_NAME(VK_OBJECT_TYPE_QUERY_POOL);
    ENUM_TO_NAME(VK_OBJECT_TYPE_BUFFER_VIEW);
    ENUM_TO_NAME(VK_OBJECT_TYPE_IMAGE_VIEW);
    ENUM_TO_NAME(VK_OBJECT_TYPE_SHADER_MODULE);
    ENUM_TO_NAME(VK_OBJECT_TYPE_PIPELINE_CACHE);
    ENUM_TO_NAME(VK_OBJECT_TYPE_PIPELINE_LAYOUT);
    ENUM_TO_NAME(VK_OBJECT_TYPE_RENDER_PASS);
    ENUM_TO_NAME(VK_OBJECT_TYPE_PIPELINE);
    ENUM_TO_NAME(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT);
    ENUM_TO_NAME(VK_OBJECT_TYPE_SAMPLER);
    ENUM_TO_NAME(VK_OBJECT_TYPE_DESCRIPTOR_POOL);
    ENUM_TO_NAME(VK_OBJECT_TYPE_DESCRIPTOR_SET);
    ENUM_TO_NAME(VK_OBJECT_TYPE_FRAMEBUFFER);
    ENUM_TO_NAME(VK_OBJECT_TYPE_COMMAND_POOL);
    ENUM_TO_NAME(VK_OBJECT_TYPE_SURFACE_KHR);
    ENUM_TO_NAME(VK_OBJECT_TYPE_SWAPCHAIN_KHR);
    ENUM_TO_NAME(VK_OBJECT_TYPE_DISPLAY_KHR);
    ENUM_TO_NAME(VK_OBJECT_TYPE_DISPLAY_MODE_KHR);
    ENUM_TO_NAME(VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT);
    ENUM_TO_NAME(VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT);
    ENUM_TO_NAME(VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV);
    ENUM_TO_NAME(VK_OBJECT_TYPE_VALIDATION_CACHE_EXT);
    ENUM_TO_NAME(VK_OBJECT_TYPE_PERFORMANCE_CONFIGURATION_INTEL);
    ENUM_TO_NAME(VK_OBJECT_TYPE_DEFERRED_OPERATION_KHR);
    ENUM_TO_NAME(VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NV);
    ENUM_TO_NAME(VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_KHR);
    ENUM_TO_NAME(VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION_KHR);
    ENUM_TO_NAME(VK_OBJECT_TYPE_PRIVATE_DATA_SLOT_EXT);
#if VK_NVX_binary_import
    ENUM_TO_NAME(VK_OBJECT_TYPE_CU_MODULE_NVX);
    ENUM_TO_NAME(VK_OBJECT_TYPE_CU_FUNCTION_NVX);
#endif
    // switch uses some old headers
#if !_TARGET_C3
    ENUM_TO_NAME(VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR);
    ENUM_TO_NAME(VK_OBJECT_TYPE_BUFFER_COLLECTION_FUCHSIA);
#endif
    default: break;
  }
  return enumName;
}
