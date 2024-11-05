/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#define REBLUR

// Switches ( default 1 )
#define REBLUR_USE_CATROM_FOR_SURFACE_MOTION_IN_TA              1
#define REBLUR_USE_CATROM_FOR_VIRTUAL_MOTION_IN_TA              1
#define REBLUR_USE_CATROM_FOR_SURFACE_MOTION_IN_TS              1
#define REBLUR_USE_CATROM_FOR_VIRTUAL_MOTION_IN_TS              1
#define REBLUR_USE_HISTORY_FIX                                  1
#define REBLUR_USE_YCOCG                                        1
#define REBLUR_USE_MORE_STRICT_PARALLAX_BASED_CHECK             1
#define REBLUR_USE_ANTIFIREFLY                                  1
#define REBLUR_USE_CONFIDENCE_NON_LINEARLY                      1

// Switches ( default 0 )
#define REBLUR_USE_SCREEN_SPACE_SAMPLING                        0
#define REBLUR_USE_ANTILAG_NOT_INVOKING_HISTORY_FIX             0
#define REBLUR_USE_DECOMPRESSED_HIT_DIST_IN_RECONSTRUCTION      0 // compression helps to preserve "lobe important" values

#ifdef REBLUR_OCCLUSION
    #undef REBLUR_USE_ANTIFIREFLY
    #define REBLUR_USE_ANTIFIREFLY                              0 // not needed in occlusion mode
#endif

// Switches ( default 2 )
#define REBLUR_VIRTUAL_HISTORY_AMOUNT                           2 // 0 - debug surface motion, 1 - debug virtual motion

// Show
#define REBLUR_SHOW_FAST_HISTORY                                1 // requires "blurRadius" = 0
#define REBLUR_SHOW_CURVATURE                                   2
#define REBLUR_SHOW_CURVATURE_SIGN                              3
#define REBLUR_SHOW_SURFACE_HISTORY_CONFIDENCE                  4
#define REBLUR_SHOW_VIRTUAL_HISTORY_CONFIDENCE                  5
#define REBLUR_SHOW_VIRTUAL_HISTORY_NORMAL_CONFIDENCE           6
#define REBLUR_SHOW_VIRTUAL_HISTORY_ROUGHNESS_CONFIDENCE        7
#define REBLUR_SHOW_VIRTUAL_HISTORY_PARALLAX_CONFIDENCE         8
#define REBLUR_SHOW_HIT_DIST_FOR_TRACKING                       9

#define REBLUR_SHOW                                             0 // 0 or "REBLUR_SHOW_X"

// Settings
#define REBLUR_PRE_BLUR_POISSON_SAMPLE_NUM                      8
#define REBLUR_PRE_BLUR_POISSON_SAMPLES( i )                    g_Special8[ i ]

#define REBLUR_POISSON_SAMPLE_NUM                               8
#define REBLUR_POISSON_SAMPLES( i )                             g_Special8[ i ]

#define REBLUR_PRE_BLUR_ROTATOR_MODE                            NRD_FRAME // TODO: others are expensive, but work better
#define REBLUR_PRE_BLUR_FRACTION_SCALE                          2.0
#define REBLUR_PRE_BLUR_NON_LINEAR_ACCUM_SPEED                  ( 1.0 / ( 1.0 + 10.0 ) )

#define REBLUR_BLUR_ROTATOR_MODE                                NRD_FRAME
#define REBLUR_BLUR_FRACTION_SCALE                              1.0

#define REBLUR_POST_BLUR_ROTATOR_MODE                           NRD_FRAME
#define REBLUR_POST_BLUR_FRACTION_SCALE                         0.5
#define REBLUR_POST_BLUR_RADIUS_SCALE                           2.0

#define REBLUR_HIT_DIST_MIN_WEIGHT( smc )                       ( 0.1 * smc ) // was 0.1

#define REBLUR_VIRTUAL_MOTION_PREV_PREV_WEIGHT_ITERATION_NUM    2
#define REBLUR_COLOR_CLAMPING_SIGMA_SCALE                       2.0 // using smaller values leads to bias if camera rotates slowly due to reprojection instabilities
#define REBLUR_FIREFLY_SUPPRESSOR_MAX_RELATIVE_INTENSITY        float2( 10.0, 1.1 )
#define REBLUR_FIREFLY_SUPPRESSOR_RADIUS_SCALE                  0.1
#define REBLUR_ANTI_FIREFLY_FILTER_RADIUS                       4 // pixels
#define REBLUR_ANTI_FIREFLY_SIGMA_SCALE                         2.0
#define REBLUR_SAMPLES_PER_FRAME                                1.0 // TODO: expose in settings, it will become useful with very clean signals, when max number of accumulated frames is low

// Data types
#ifdef REBLUR_OCCLUSION
    #define REBLUR_TYPE                                         float
#else
    #define REBLUR_TYPE                                         float4
#endif

#define REBLUR_SH_TYPE                                          float4
#define REBLUR_FAST_TYPE                                        float

// Shared constants
#define REBLUR_SHARED_CONSTANTS \
    NRD_CONSTANT( float4x4, gViewToClip ) \
    NRD_CONSTANT( float4x4, gViewToWorld ) \
    NRD_CONSTANT( float4x4, gWorldToViewPrev ) \
    NRD_CONSTANT( float4x4, gWorldToClipPrev ) \
    NRD_CONSTANT( float4x4, gWorldPrevToWorld ) \
    NRD_CONSTANT( float4, gFrustum ) \
    NRD_CONSTANT( float4, gFrustumPrev ) \
    NRD_CONSTANT( float4, gCameraDelta ) \
    NRD_CONSTANT( float4, gAntilagParams ) \
    NRD_CONSTANT( float4, gHitDistParams ) \
    NRD_CONSTANT( float4, gViewVectorWorld ) \
    NRD_CONSTANT( float4, gViewVectorWorldPrev ) \
    NRD_CONSTANT( float4, gMvScale ) \
    NRD_CONSTANT( float2, gResourceSize ) \
    NRD_CONSTANT( float2, gResourceSizeInv ) \
    NRD_CONSTANT( float2, gResourceSizeInvPrev ) \
    NRD_CONSTANT( float2, gRectSize ) \
    NRD_CONSTANT( float2, gRectSizeInv ) \
    NRD_CONSTANT( float2, gRectSizePrev ) \
    NRD_CONSTANT( float2, gResolutionScale ) \
    NRD_CONSTANT( float2, gResolutionScalePrev ) \
    NRD_CONSTANT( float2, gRectOffset ) \
    NRD_CONSTANT( float2, gSpecProbabilityThresholdsForMvModification ) \
    NRD_CONSTANT( float2, gJitter ) \
    NRD_CONSTANT( uint2, gPrintfAt ) \
    NRD_CONSTANT( uint2, gRectOrigin ) \
    NRD_CONSTANT( int2, gRectSizeMinusOne ) \
    NRD_CONSTANT( float, gDisocclusionThreshold ) \
    NRD_CONSTANT( float, gDisocclusionThresholdAlternate ) \
    NRD_CONSTANT( float, gStabilizationStrength ) \
    NRD_CONSTANT( float, gDebug ) \
    NRD_CONSTANT( float, gOrthoMode ) \
    NRD_CONSTANT( float, gUnproject ) \
    NRD_CONSTANT( float, gDenoisingRange ) \
    NRD_CONSTANT( float, gPlaneDistSensitivity ) \
    NRD_CONSTANT( float, gFramerateScale ) \
    NRD_CONSTANT( float, gMinBlurRadius ) \
    NRD_CONSTANT( float, gMaxBlurRadius ) \
    NRD_CONSTANT( float, gDiffPrepassBlurRadius ) \
    NRD_CONSTANT( float, gSpecPrepassBlurRadius ) \
    NRD_CONSTANT( float, gMaxAccumulatedFrameNum ) \
    NRD_CONSTANT( float, gMaxFastAccumulatedFrameNum ) \
    NRD_CONSTANT( float, gAntiFirefly ) \
    NRD_CONSTANT( float, gLobeAngleFraction ) \
    NRD_CONSTANT( float, gRoughnessFraction ) \
    NRD_CONSTANT( float, gResponsiveAccumulationRoughnessThreshold ) \
    NRD_CONSTANT( float, gHistoryFixFrameNum ) \
    NRD_CONSTANT( float, gMinRectDimMulUnproject ) \
    NRD_CONSTANT( float, gUsePrepassNotOnlyForSpecularMotionEstimation ) \
    NRD_CONSTANT( float, gSplitScreen ) \
    NRD_CONSTANT( float, gCheckerboardResolveAccumSpeed ) \
    NRD_CONSTANT( uint, gHasHistoryConfidence ) \
    NRD_CONSTANT( uint, gHasDisocclusionThresholdMix ) \
    NRD_CONSTANT( uint, gDiffCheckerboard ) \
    NRD_CONSTANT( uint, gSpecCheckerboard ) \
    NRD_CONSTANT( uint, gFrameIndex ) \
    NRD_CONSTANT( uint, gDiffMaterialMask ) \
    NRD_CONSTANT( uint, gSpecMaterialMask ) \
    NRD_CONSTANT( uint, gIsRectChanged ) \
    NRD_CONSTANT( uint, gResetHistory )

#ifdef REBLUR_DIRECTIONAL_OCCLUSION
    #undef REBLUR_USE_CATROM_FOR_SURFACE_MOTION_IN_TA
    #define REBLUR_USE_CATROM_FOR_SURFACE_MOTION_IN_TA          0

    #undef REBLUR_USE_CATROM_FOR_VIRTUAL_MOTION_IN_TA
    #define REBLUR_USE_CATROM_FOR_VIRTUAL_MOTION_IN_TA          0
#endif

// PERFORMANCE MODE: x1.25 perf boost by sacrificing IQ ( DIFFUSE_SPECULAR on RTX 3090 at 1440p 2.05 vs 2.55 ms )
#ifdef REBLUR_PERFORMANCE_MODE
    #undef REBLUR_USE_CATROM_FOR_SURFACE_MOTION_IN_TA
    #define REBLUR_USE_CATROM_FOR_SURFACE_MOTION_IN_TA          0

    #undef REBLUR_USE_CATROM_FOR_VIRTUAL_MOTION_IN_TA
    #define REBLUR_USE_CATROM_FOR_VIRTUAL_MOTION_IN_TA          0

    #undef REBLUR_USE_CATROM_FOR_SURFACE_MOTION_IN_TS
    #define REBLUR_USE_CATROM_FOR_SURFACE_MOTION_IN_TS          0

    #undef REBLUR_USE_CATROM_FOR_VIRTUAL_MOTION_IN_TS
    #define REBLUR_USE_CATROM_FOR_VIRTUAL_MOTION_IN_TS          0

    #undef REBLUR_USE_SCREEN_SPACE_SAMPLING
    #define REBLUR_USE_SCREEN_SPACE_SAMPLING                    1

    #undef REBLUR_POISSON_SAMPLE_NUM
    #define REBLUR_POISSON_SAMPLE_NUM                           6

    #undef REBLUR_POISSON_SAMPLES
    #define REBLUR_POISSON_SAMPLES( i )                         g_Special6[ i ]

    #undef REBLUR_PRE_BLUR_POISSON_SAMPLE_NUM
    #define REBLUR_PRE_BLUR_POISSON_SAMPLE_NUM                  6

    #undef REBLUR_PRE_BLUR_POISSON_SAMPLES
    #define REBLUR_PRE_BLUR_POISSON_SAMPLES( i )                g_Special6[ i ]

    #undef REBLUR_PRE_BLUR_ROTATOR_MODE
    #define REBLUR_PRE_BLUR_ROTATOR_MODE                        NRD_FRAME

    #undef REBLUR_BLUR_ROTATOR_MODE
    #define REBLUR_BLUR_ROTATOR_MODE                            NRD_FRAME

    #undef REBLUR_POST_BLUR_ROTATOR_MODE
    #define REBLUR_POST_BLUR_ROTATOR_MODE                       NRD_FRAME

    #undef REBLUR_ANTI_FIREFLY_FILTER_RADIUS
    #define REBLUR_ANTI_FIREFLY_FILTER_RADIUS                   3
#endif
