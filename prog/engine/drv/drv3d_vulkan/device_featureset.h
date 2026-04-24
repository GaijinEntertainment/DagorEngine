// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "vulkan_api.h"

#define VULKAN_FEATURESET_OPTIONAL_LIST                            \
  VULKAN_FEATURE_OPTIONAL(geometryShader)                          \
  VULKAN_FEATURE_OPTIONAL(textureCompressionBC)                    \
  VULKAN_FEATURE_OPTIONAL(textureCompressionASTC_LDR)              \
  VULKAN_FEATURE_OPTIONAL(fragmentStoresAndAtomics)                \
  VULKAN_FEATURE_OPTIONAL(fillModeNonSolid)                        \
  VULKAN_FEATURE_OPTIONAL(depthBounds)                             \
  VULKAN_FEATURE_OPTIONAL(samplerAnisotropy)                       \
  VULKAN_FEATURE_OPTIONAL(depthClamp)                              \
  VULKAN_FEATURE_OPTIONAL(imageCubeArray)                          \
  VULKAN_FEATURE_OPTIONAL(multiDrawIndirect)                       \
  VULKAN_FEATURE_OPTIONAL(tessellationShader)                      \
  VULKAN_FEATURE_OPTIONAL(shaderInt16)                             \
  VULKAN_FEATURE_OPTIONAL(dualSrcBlend)                            \
  VULKAN_FEATURE_OPTIONAL(drawIndirectFirstInstance)               \
  VULKAN_FEATURE_OPTIONAL(shaderResourceResidency)                 \
  VULKAN_FEATURE_OPTIONAL(sparseBinding)                           \
  VULKAN_FEATURE_OPTIONAL(sparseResidencyBuffer)                   \
  VULKAN_FEATURE_OPTIONAL(sparseResidencyImage2D)                  \
  VULKAN_FEATURE_OPTIONAL(sparseResidencyImage3D)                  \
  VULKAN_FEATURE_OPTIONAL(sparseResidency2Samples)                 \
  VULKAN_FEATURE_OPTIONAL(sparseResidency4Samples)                 \
  VULKAN_FEATURE_OPTIONAL(sparseResidency8Samples)                 \
  VULKAN_FEATURE_OPTIONAL(sparseResidency16Samples)                \
  VULKAN_FEATURE_OPTIONAL(sparseResidencyAliased)                  \
  VULKAN_FEATURE_OPTIONAL(textureCompressionETC2)                  \
  VULKAN_FEATURE_OPTIONAL(pipelineStatisticsQuery)                 \
  VULKAN_FEATURE_OPTIONAL(sampleRateShading)                       \
  VULKAN_FEATURE_OPTIONAL(logicOp)                                 \
  VULKAN_FEATURE_OPTIONAL(wideLines)                               \
  VULKAN_FEATURE_OPTIONAL(largePoints)                             \
  VULKAN_FEATURE_OPTIONAL(alphaToOne)                              \
  VULKAN_FEATURE_OPTIONAL(multiViewport)                           \
  VULKAN_FEATURE_OPTIONAL(shaderTessellationAndGeometryPointSize)  \
  VULKAN_FEATURE_OPTIONAL(shaderStorageImageExtendedFormats)       \
  VULKAN_FEATURE_OPTIONAL(shaderStorageImageMultisample)           \
  VULKAN_FEATURE_OPTIONAL(shaderStorageImageReadWithoutFormat)     \
  VULKAN_FEATURE_OPTIONAL(shaderSampledImageArrayDynamicIndexing)  \
  VULKAN_FEATURE_OPTIONAL(shaderStorageBufferArrayDynamicIndexing) \
  VULKAN_FEATURE_OPTIONAL(shaderStorageImageArrayDynamicIndexing)  \
  VULKAN_FEATURE_OPTIONAL(shaderFloat64)                           \
  VULKAN_FEATURE_OPTIONAL(shaderInt64)                             \
  VULKAN_FEATURE_OPTIONAL(shaderResourceMinLod)                    \
  VULKAN_FEATURE_OPTIONAL(variableMultisampleRate)                 \
  VULKAN_FEATURE_OPTIONAL(inheritedQueries)                        \
  VULKAN_FEATURE_OPTIONAL(depthBiasClamp)                          \
  VULKAN_FEATURE_OPTIONAL(occlusionQueryPrecise)                   \
  VULKAN_FEATURE_OPTIONAL(fullDrawIndexUint32)                     \
  VULKAN_FEATURE_OPTIONAL(shaderUniformBufferArrayDynamicIndexing) \
  VULKAN_FEATURE_OPTIONAL(shaderCullDistance)                      \
  VULKAN_FEATURE_OPTIONAL(shaderClipDistance)                      \
  VULKAN_FEATURE_OPTIONAL(shaderStorageImageWriteWithoutFormat)

// independentBlend:
// - currently adreno 4xx chips do not support this
// - Required to have different color write mask per render target.
#define VULKAN_FEATURESET_REQUIRED_LIST VULKAN_FEATURE_REQUIRE(independentBlend)

namespace drv3d_vulkan
{

bool check_and_update_device_features(VkPhysicalDeviceFeatures &set);

}
