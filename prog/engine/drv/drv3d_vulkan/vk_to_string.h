// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_string.h>

#include "vulkan_api.h"

namespace drv3d_vulkan
{

String formatImageUsageFlags(VkImageUsageFlags flags);
String formatBufferUsageFlags(VkBufferUsageFlags flags);
String formatPipelineStageFlags(VkPipelineStageFlags flags);
String formatMemoryAccessFlags(VkAccessFlags flags);
const char *formatImageLayout(VkImageLayout layout);
String formatMemoryTypeFlags(VkMemoryPropertyFlags propertyFlags);
const char *formatPrimitiveTopology(VkPrimitiveTopology top);
const char *formatObjectType(VkObjectType obj_type);
const char *formatAttachmentLoadOp(const VkAttachmentLoadOp load_op);
const char *formatAttachmentStoreOp(const VkAttachmentStoreOp store_op);

} // namespace drv3d_vulkan
