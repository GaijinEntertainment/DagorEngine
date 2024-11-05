// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// used for de and encoding into blk's
#define DX12_D3D_CAP_SET                                    \
  DX12_D3D_CAP(hasAnisotropicFilter);                       \
  DX12_D3D_CAP(hasDepthReadOnly);                           \
  DX12_D3D_CAP(hasStructuredBuffers);                       \
  DX12_D3D_CAP(hasNoOverwriteOnShaderResourceBuffers);      \
  DX12_D3D_CAP(hasForcedSamplerCount);                      \
  DX12_D3D_CAP(hasVolMipMap);                               \
  DX12_D3D_CAP(hasAsyncCompute);                            \
  DX12_D3D_CAP(hasOcclusionQuery);                          \
  DX12_D3D_CAP(hasConstBufferOffset);                       \
  DX12_D3D_CAP(hasDepthBoundsTest);                         \
  DX12_D3D_CAP(hasConditionalRender);                       \
  DX12_D3D_CAP(hasResourceCopyConversion);                  \
  DX12_D3D_CAP(hasAsyncCopy);                               \
  DX12_D3D_CAP(hasReadMultisampledDepth);                   \
  DX12_D3D_CAP(hasInstanceID);                              \
  DX12_D3D_CAP(hasConservativeRassterization);              \
  DX12_D3D_CAP(hasQuadTessellation);                        \
  DX12_D3D_CAP(hasGather4);                                 \
  DX12_D3D_CAP(hasAlphaCoverage);                           \
  DX12_D3D_CAP(hasWellSupportedIndirect);                   \
  DX12_D3D_CAP(hasBindless);                                \
  DX12_D3D_CAP(hasNVApi);                                   \
  DX12_D3D_CAP(hasATIApi);                                  \
  DX12_D3D_CAP(hasVariableRateShading);                     \
  DX12_D3D_CAP(hasVariableRateShadingTexture);              \
  DX12_D3D_CAP(hasVariableRateShadingShaderOutput);         \
  DX12_D3D_CAP(hasVariableRateShadingCombiners);            \
  DX12_D3D_CAP(hasVariableRateShadingBy4);                  \
  DX12_D3D_CAP(hasAliasedTextures);                         \
  DX12_D3D_CAP(hasResourceHeaps);                           \
  DX12_D3D_CAP(hasBufferOverlapCopy);                       \
  DX12_D3D_CAP(hasBufferOverlapRegionsCopy);                \
  DX12_D3D_CAP(hasUAVOnlyForcedSampleCount);                \
  DX12_D3D_CAP(hasShader64BitIntegerResources);             \
  DX12_D3D_CAP(hasNativeRenderPassSubPasses);               \
  DX12_D3D_CAP(hasTiled2DResources);                        \
  DX12_D3D_CAP(hasTiled3DResources);                        \
  DX12_D3D_CAP(hasTiledSafeResourcesAccess);                \
  DX12_D3D_CAP(hasTiledMemoryAliasing);                     \
  DX12_D3D_CAP(hasDLSS);                                    \
  DX12_D3D_CAP(hasXESS);                                    \
  DX12_D3D_CAP(hasDrawID);                                  \
  DX12_D3D_CAP(hasMeshShader);                              \
  DX12_D3D_CAP(hasBasicViewInstancing);                     \
  DX12_D3D_CAP(hasOptimizedViewInstancing);                 \
  DX12_D3D_CAP(hasAcceleratedViewInstancing);               \
  DX12_D3D_CAP(hasRenderPassDepthResolve);                  \
  DX12_D3D_CAP(hasStereoExpansion);                         \
  DX12_D3D_CAP(hasTileBasedArchitecture);                   \
  DX12_D3D_CAP(hasLazyMemory);                              \
  DX12_D3D_CAP(hasIndirectSupport);                         \
  DX12_D3D_CAP(hasCompareSampler);                          \
  DX12_D3D_CAP(hasRayAccelerationStructure);                \
  DX12_D3D_CAP(hasRayQuery);                                \
  DX12_D3D_CAP(hasRayDispatch);                             \
  DX12_D3D_CAP(hasIndirectRayDispatch);                     \
  DX12_D3D_CAP(hasGeometryIndexInRayAccelerationStructure); \
  DX12_D3D_CAP(hasSkipPrimitiveTypeInRayTracingShaders);    \
  DX12_D3D_CAP(hasNativeRayTracePipelineExpansion);

#define DX12_D3D_CAP_SET_RELEVANT_FOR_PIPELINES             \
  DX12_D3D_CAP(hasVariableRateShadingShaderOutput);         \
  DX12_D3D_CAP(hasVariableRateShadingBy4);                  \
  DX12_D3D_CAP(hasShader64BitIntegerResources);             \
  DX12_D3D_CAP(hasTiled2DResources);                        \
  DX12_D3D_CAP(hasTiled3DResources);                        \
  DX12_D3D_CAP(hasMeshShader);                              \
  DX12_D3D_CAP(hasBasicViewInstancing);                     \
  DX12_D3D_CAP(hasShaderFloat16Support);                    \
  DX12_D3D_CAP(hasRayQuery);                                \
  DX12_D3D_CAP(hasRayDispatch);                             \
  DX12_D3D_CAP(hasGeometryIndexInRayAccelerationStructure); \
  DX12_D3D_CAP(hasSkipPrimitiveTypeInRayTracingShaders);
