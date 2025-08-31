// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "vulkan_api.h"


#define VULKAN_FEATURESET_DISABLED_LIST                           \
  VULKAN_FEATURE_DISABLE(sampleRateShading)                       \
  VULKAN_FEATURE_DISABLE(logicOp)                                 \
  VULKAN_FEATURE_DISABLE(wideLines)                               \
  VULKAN_FEATURE_DISABLE(largePoints)                             \
  VULKAN_FEATURE_DISABLE(alphaToOne)                              \
  VULKAN_FEATURE_DISABLE(multiViewport)                           \
  VULKAN_FEATURE_DISABLE(shaderTessellationAndGeometryPointSize)  \
  VULKAN_FEATURE_DISABLE(shaderStorageImageExtendedFormats)       \
  VULKAN_FEATURE_DISABLE(shaderStorageImageMultisample)           \
  VULKAN_FEATURE_DISABLE(shaderStorageImageReadWithoutFormat)     \
  VULKAN_FEATURE_DISABLE(shaderSampledImageArrayDynamicIndexing)  \
  VULKAN_FEATURE_DISABLE(shaderStorageBufferArrayDynamicIndexing) \
  VULKAN_FEATURE_DISABLE(shaderStorageImageArrayDynamicIndexing)  \
  VULKAN_FEATURE_DISABLE(shaderFloat64)                           \
  VULKAN_FEATURE_DISABLE(shaderInt64)                             \
  VULKAN_FEATURE_DISABLE(shaderResourceMinLod)                    \
  VULKAN_FEATURE_DISABLE(variableMultisampleRate)                 \
  VULKAN_FEATURE_DISABLE(inheritedQueries)                        \
  VULKAN_FEATURE_DISABLE(depthBiasClamp)                          \
  VULKAN_FEATURE_DISABLE(occlusionQueryPrecise)                   \
  VULKAN_FEATURE_DISABLE(fullDrawIndexUint32)                     \
  VULKAN_FEATURE_DISABLE(shaderUniformBufferArrayDynamicIndexing) \
  VULKAN_FEATURE_DISABLE(shaderCullDistance)                      \
  VULKAN_FEATURE_DISABLE(shaderClipDistance)                      \
  VULKAN_FEATURE_DISABLE(shaderStorageImageWriteWithoutFormat)

#define VULKAN_FEATURESET_OPTIONAL_LIST               \
  VULKAN_FEATURE_OPTIONAL(geometryShader)             \
  VULKAN_FEATURE_OPTIONAL(textureCompressionBC)       \
  VULKAN_FEATURE_OPTIONAL(textureCompressionASTC_LDR) \
  VULKAN_FEATURE_OPTIONAL(fragmentStoresAndAtomics)   \
  VULKAN_FEATURE_OPTIONAL(fillModeNonSolid)           \
  VULKAN_FEATURE_OPTIONAL(depthBounds)                \
  VULKAN_FEATURE_OPTIONAL(samplerAnisotropy)          \
  VULKAN_FEATURE_OPTIONAL(depthClamp)                 \
  VULKAN_FEATURE_OPTIONAL(imageCubeArray)             \
  VULKAN_FEATURE_OPTIONAL(multiDrawIndirect)          \
  VULKAN_FEATURE_OPTIONAL(tessellationShader)         \
  VULKAN_FEATURE_OPTIONAL(shaderInt16)                \
  VULKAN_FEATURE_OPTIONAL(dualSrcBlend)               \
  VULKAN_FEATURE_OPTIONAL(drawIndirectFirstInstance)  \
  VULKAN_FEATURE_OPTIONAL(shaderResourceResidency)    \
  VULKAN_FEATURE_OPTIONAL(sparseBinding)              \
  VULKAN_FEATURE_OPTIONAL(sparseResidencyBuffer)      \
  VULKAN_FEATURE_OPTIONAL(sparseResidencyImage2D)     \
  VULKAN_FEATURE_OPTIONAL(sparseResidencyImage3D)     \
  VULKAN_FEATURE_OPTIONAL(sparseResidency2Samples)    \
  VULKAN_FEATURE_OPTIONAL(sparseResidency4Samples)    \
  VULKAN_FEATURE_OPTIONAL(sparseResidency8Samples)    \
  VULKAN_FEATURE_OPTIONAL(sparseResidency16Samples)   \
  VULKAN_FEATURE_OPTIONAL(sparseResidencyAliased)     \
  VULKAN_FEATURE_OPTIONAL(textureCompressionETC2)     \
  VULKAN_FEATURE_OPTIONAL(pipelineStatisticsQuery)

// independentBlend:
// - currently adreno 4xx chips do not support this
// - Required to have different color write mask per render target.
#define VULKAN_FEATURESET_REQUIRED_LIST VULKAN_FEATURE_REQUIRE(independentBlend)

namespace drv3d_vulkan
{

bool check_and_update_device_features(VkPhysicalDeviceFeatures &set);

}
