/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#define RELAX

// Settings
// IMPORTANT: if == 1, then for 0-roughness "GetEncodingAwareNormalWeight" can return values < 1 even for same normals due to data re-packing
#define RELAX_NORMAL_ULP                                    ( 1.5 / 255.0 )

#define RELAX_MAX_ACCUM_FRAME_NUM                           255
#define RELAX_ANTILAG_ACCELERATION_AMOUNT_SCALE             10.0 // Multiplier used to put RelaxAntilagSettings::accelerationAmount to convenient [0; 1] range

// Shared constants
#define RELAX_SHARED_CONSTANTS \
    NRD_CONSTANT( float4x4, gWorldToClip ) \
    NRD_CONSTANT( float4x4, gWorldToClipPrev ) \
    NRD_CONSTANT( float4x4, gWorldToViewPrev ) \
    NRD_CONSTANT( float4x4, gWorldPrevToWorld ) \
    NRD_CONSTANT( float4, gRotatorPre ) \
    NRD_CONSTANT( float4, gFrustumRight ) \
    NRD_CONSTANT( float4, gFrustumUp ) \
    NRD_CONSTANT( float4, gFrustumForward ) \
    NRD_CONSTANT( float4, gPrevFrustumRight ) \
    NRD_CONSTANT( float4, gPrevFrustumUp ) \
    NRD_CONSTANT( float4, gPrevFrustumForward ) \
    NRD_CONSTANT( float4, gCameraDelta ) \
    NRD_CONSTANT( float4, gMvScale ) \
    NRD_CONSTANT( float2, gJitter ) \
    NRD_CONSTANT( float2, gResolutionScale ) \
    NRD_CONSTANT( float2, gRectOffset ) \
    NRD_CONSTANT( float2, gResourceSizeInv ) \
    NRD_CONSTANT( float2, gResourceSize ) \
    NRD_CONSTANT( float2, gRectSizeInv ) \
    NRD_CONSTANT( float2, gRectSizePrev ) \
    NRD_CONSTANT( float2, gResourceSizeInvPrev ) \
    NRD_CONSTANT( uint2, gPrintfAt ) \
    NRD_CONSTANT( uint2, gRectOrigin ) \
    NRD_CONSTANT( int2, gRectSize ) \
    NRD_CONSTANT( float, gSpecMaxAccumulatedFrameNum ) \
    NRD_CONSTANT( float, gSpecMaxFastAccumulatedFrameNum ) \
    NRD_CONSTANT( float, gDiffMaxAccumulatedFrameNum ) \
    NRD_CONSTANT( float, gDiffMaxFastAccumulatedFrameNum ) \
    NRD_CONSTANT( float, gDisocclusionThreshold ) \
    NRD_CONSTANT( float, gDisocclusionThresholdAlternate ) \
    NRD_CONSTANT( float, gCameraAttachedReflectionMaterialID ) \
    NRD_CONSTANT( float, gStrandMaterialID ) \
    NRD_CONSTANT( float, gStrandThickness ) \
    NRD_CONSTANT( float, gRoughnessFraction ) \
    NRD_CONSTANT( float, gSpecVarianceBoost ) \
    NRD_CONSTANT( float, gSplitScreen ) \
    NRD_CONSTANT( float, gDiffBlurRadius ) \
    NRD_CONSTANT( float, gSpecBlurRadius ) \
    NRD_CONSTANT( float, gDepthThreshold ) \
    NRD_CONSTANT( float, gLobeAngleFraction ) \
    NRD_CONSTANT( float, gSpecLobeAngleSlack ) \
    NRD_CONSTANT( float, gHistoryFixEdgeStoppingNormalPower ) \
    NRD_CONSTANT( float, gRoughnessEdgeStoppingRelaxation ) \
    NRD_CONSTANT( float, gNormalEdgeStoppingRelaxation ) \
    NRD_CONSTANT( float, gColorBoxSigmaScale ) \
    NRD_CONSTANT( float, gHistoryAccelerationAmount ) \
    NRD_CONSTANT( float, gHistoryResetTemporalSigmaScale ) \
    NRD_CONSTANT( float, gHistoryResetSpatialSigmaScale ) \
    NRD_CONSTANT( float, gHistoryResetAmount ) \
    NRD_CONSTANT( float, gDenoisingRange ) \
    NRD_CONSTANT( float, gSpecPhiLuminance ) \
    NRD_CONSTANT( float, gDiffPhiLuminance ) \
    NRD_CONSTANT( float, gDiffMaxLuminanceRelativeDifference ) \
    NRD_CONSTANT( float, gSpecMaxLuminanceRelativeDifference ) \
    NRD_CONSTANT( float, gLuminanceEdgeStoppingRelaxation ) \
    NRD_CONSTANT( float, gConfidenceDrivenRelaxationMultiplier ) \
    NRD_CONSTANT( float, gConfidenceDrivenLuminanceEdgeStoppingRelaxation ) \
    NRD_CONSTANT( float, gConfidenceDrivenNormalEdgeStoppingRelaxation ) \
    NRD_CONSTANT( float, gDebug ) \
    NRD_CONSTANT( float, gOrthoMode ) \
    NRD_CONSTANT( float, gUnproject ) \
    NRD_CONSTANT( float, gFramerateScale ) \
    NRD_CONSTANT( float, gCheckerboardResolveAccumSpeed ) \
    NRD_CONSTANT( float, gJitterDelta ) \
    NRD_CONSTANT( float, gHistoryFixFrameNum ) \
    NRD_CONSTANT( float, gHistoryFixBasePixelStride ) \
    NRD_CONSTANT( float, gHistoryThreshold ) \
    NRD_CONSTANT( float, gViewZScale ) \
    NRD_CONSTANT( float, gMinHitDistanceWeight ) \
    NRD_CONSTANT( float, gDiffMinMaterial ) \
    NRD_CONSTANT( float, gSpecMinMaterial ) \
    NRD_CONSTANT( uint, gRoughnessEdgeStoppingEnabled ) \
    NRD_CONSTANT( uint, gFrameIndex ) \
    NRD_CONSTANT( uint, gDiffCheckerboard ) \
    NRD_CONSTANT( uint, gSpecCheckerboard ) \
    NRD_CONSTANT( uint, gHasHistoryConfidence ) \
    NRD_CONSTANT( uint, gHasDisocclusionThresholdMix ) \
    NRD_CONSTANT( uint, gResetHistory )

#define gResolutionScalePrev ( gRectSizePrev * gResourceSizeInvPrev )

#if( !defined RELAX_DIFFUSE && !defined RELAX_SPECULAR )
    #define RELAX_DIFFUSE
    #define RELAX_SPECULAR
#endif