/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#define SIGMA

// Switches ( default 1 )
#define SIGMA_USE_EARLY_OUT_IN_TS                       1 // improves performance in regions with hard shadow
#define SIGMA_USE_CATROM                                1 // sharper reprojection
#define SIGMA_5X5_TEMPORAL_KERNEL                       1 // provides variance estimation in a wider radius
#define SIGMA_5X5_BLUR_RADIUS_ESTIMATION_KERNEL         1 // helps to improve stability, but adds 10% of overhead
#define SIGMA_ADJUST_HISTORY_LENGTH_BY_ANTILAG          1 // TODO: is it needed?
#define SIGMA_USE_SPARSE_BLUR                           1 // can be disabled for debugging purposes
#define SIGMA_USE_SCREEN_SPACE_SAMPLING                 1 // almost matches world-space sampling but simpler code ( TODO: doesn't elongate shadow )

// Show
#define SIGMA_SHOW_TILES                                1
#define SIGMA_SHOW_HISTORY_WEIGHT                       2
#define SIGMA_SHOW_HISTORY_LENGTH                       3
#define SIGMA_SHOW_PENUMBRA_SIZE                        4

#define SIGMA_SHOW                                      0 // 0 or SIGMA_SHOW_X

// Settings
#define SIGMA_ROTATOR_MODE                              NRD_FRAME
#define SIGMA_POISSON_SAMPLE_NUM                        8
#define SIGMA_POISSON_SAMPLES                           g_Special8
#define SIGMA_MAX_PIXEL_RADIUS                          32.0
#define SIGMA_TS_SIGMA_SCALE                            3.0
#define SIGMA_MAX_ACCUM_FRAME_NUM                       7

// Data type
#ifdef SIGMA_TRANSLUCENT
    #define SIGMA_TYPE                                  float4
#else
    #define SIGMA_TYPE                                  float
#endif

// Shared constants
#define SIGMA_SHARED_CONSTANTS \
    NRD_CONSTANT( float4x4, gWorldToView ) \
    NRD_CONSTANT( float4x4, gViewToClip ) \
    NRD_CONSTANT( float4x4, gWorldToClipPrev ) \
    NRD_CONSTANT( float4x4, gWorldToViewPrev ) \
    NRD_CONSTANT( float4, gRotator ) \
    NRD_CONSTANT( float4, gRotatorPost ) \
    NRD_CONSTANT( float4, gViewVectorWorld ) \
    NRD_CONSTANT( float4, gLightDirectionView ) \
    NRD_CONSTANT( float4, gFrustum ) \
    NRD_CONSTANT( float4, gFrustumPrev ) \
    NRD_CONSTANT( float4, gCameraDelta ) \
    NRD_CONSTANT( float4, gMvScale ) \
    NRD_CONSTANT( float2, gResourceSizeInv ) \
    NRD_CONSTANT( float2, gResourceSizeInvPrev ) \
    NRD_CONSTANT( float2, gRectSize ) \
    NRD_CONSTANT( float2, gRectSizeInv ) \
    NRD_CONSTANT( float2, gRectSizePrev ) \
    NRD_CONSTANT( float2, gResolutionScale ) \
    NRD_CONSTANT( float2, gRectOffset ) \
    NRD_CONSTANT( uint2, gPrintfAt ) \
    NRD_CONSTANT( uint2, gRectOrigin ) \
    NRD_CONSTANT( int2, gRectSizeMinusOne ) \
    NRD_CONSTANT( int2, gTilesSizeMinusOne ) \
    NRD_CONSTANT( float, gOrthoMode ) \
    NRD_CONSTANT( float, gUnproject ) \
    NRD_CONSTANT( float, gDenoisingRange ) \
    NRD_CONSTANT( float, gPlaneDistSensitivity ) \
    NRD_CONSTANT( float, gStabilizationStrength ) \
    NRD_CONSTANT( float, gDebug ) \
    NRD_CONSTANT( float, gSplitScreen ) \
    NRD_CONSTANT( float, gViewZScale ) \
    NRD_CONSTANT( float, gMinRectDimMulUnproject ) \
    NRD_CONSTANT( uint, gFrameIndex ) \
    NRD_CONSTANT( uint, gIsRectChanged )
