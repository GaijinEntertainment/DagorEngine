// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "vulkan_api.h"


#define VULKAN_FEATURESET_DISABLED_LIST                           \
  VULKAN_FEATURE_DISABLE(sampleRateShading)                       \
  VULKAN_FEATURE_DISABLE(dualSrcBlend)                            \
  VULKAN_FEATURE_DISABLE(logicOp)                                 \
  VULKAN_FEATURE_DISABLE(wideLines)                               \
  VULKAN_FEATURE_DISABLE(largePoints)                             \
  VULKAN_FEATURE_DISABLE(alphaToOne)                              \
  VULKAN_FEATURE_DISABLE(multiViewport)                           \
  VULKAN_FEATURE_DISABLE(textureCompressionETC2)                  \
  VULKAN_FEATURE_DISABLE(pipelineStatisticsQuery)                 \
  VULKAN_FEATURE_DISABLE(vertexPipelineStoresAndAtomics)          \
  VULKAN_FEATURE_DISABLE(shaderTessellationAndGeometryPointSize)  \
  VULKAN_FEATURE_DISABLE(shaderImageGatherExtended)               \
  VULKAN_FEATURE_DISABLE(shaderStorageImageExtendedFormats)       \
  VULKAN_FEATURE_DISABLE(shaderStorageImageMultisample)           \
  VULKAN_FEATURE_DISABLE(shaderStorageImageReadWithoutFormat)     \
  VULKAN_FEATURE_DISABLE(shaderSampledImageArrayDynamicIndexing)  \
  VULKAN_FEATURE_DISABLE(shaderStorageBufferArrayDynamicIndexing) \
  VULKAN_FEATURE_DISABLE(shaderStorageImageArrayDynamicIndexing)  \
  VULKAN_FEATURE_DISABLE(shaderFloat64)                           \
  VULKAN_FEATURE_DISABLE(shaderInt64)                             \
  VULKAN_FEATURE_DISABLE(shaderInt16)                             \
  VULKAN_FEATURE_DISABLE(shaderResourceResidency)                 \
  VULKAN_FEATURE_DISABLE(shaderResourceMinLod)                    \
  VULKAN_FEATURE_DISABLE(sparseBinding)                           \
  VULKAN_FEATURE_DISABLE(sparseResidencyBuffer)                   \
  VULKAN_FEATURE_DISABLE(sparseResidencyImage2D)                  \
  VULKAN_FEATURE_DISABLE(sparseResidencyImage3D)                  \
  VULKAN_FEATURE_DISABLE(sparseResidency2Samples)                 \
  VULKAN_FEATURE_DISABLE(sparseResidency4Samples)                 \
  VULKAN_FEATURE_DISABLE(sparseResidency8Samples)                 \
  VULKAN_FEATURE_DISABLE(sparseResidency16Samples)                \
  VULKAN_FEATURE_DISABLE(sparseResidencyAliased)                  \
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
  VULKAN_FEATURE_OPTIONAL(drawIndirectFirstInstance)

// independentBlend:
// - currently adreno 4xx chips do not support this
// - Required to have different color write mask per render target.
#define VULKAN_FEATURESET_REQUIRED_LIST VULKAN_FEATURE_REQUIRE(independentBlend)

namespace drv3d_vulkan
{

bool check_and_update_device_features(VkPhysicalDeviceFeatures &set);

}
