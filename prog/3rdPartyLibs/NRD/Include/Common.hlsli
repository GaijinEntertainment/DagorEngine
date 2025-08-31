/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

//==================================================================================================================
// Naming convention
//==================================================================================================================
// g[In/Prev/History/Out]_[A]_[B]
// gIn_         - an input
// gPrev_       - an input from the previous frame ( looks better than "gInPrev_" )
// gHistory_    - an input from the previous frame, which is a history buffer ( looks better than "gInHistory_" )
// gOut_        - an output
// _            - means "and" ( when used in-between A and B )
// s_           - shared memory
// Fast         - fast history
// Noisy        - noisy input, where an emphasis needed

#include "Poisson.hlsli"

// Constants

#define NRD_NONE                                                0 // bad
#define NRD_FRAME                                               1 // good
#define NRD_PIXEL                                               2 // better, but leads to divergence
#define NRD_RANDOM                                              3 // for experiments only

// FP16

#ifdef __hlsl_dx_compiler
    #define half_float float16_t
    #define half_float2 float16_t2
    #define half_float3 float16_t3
    #define half_float4 float16_t4
#else
    #define half_float float
    #define half_float2 float2
    #define half_float3 float3
    #define half_float4 float4
#endif

//==================================================================================================================
// DEFAULT SETTINGS ( can be modified )
//==================================================================================================================

// Switches ( default 1 )
#define NRD_USE_TILE_CHECK                                      1 // significantly improves performance by skipping computations in "empty" regions
#define NRD_USE_HIGH_PARALLAX_CURVATURE                         1 // flattens surface on high motion
#define NRD_USE_DENANIFICATION                                  1 // needed only if inputs have NAN / INF outside of viewport or denoising range
// Gaijin change:
// We are surely not using these 3, so remove them to claim some registers
#define NRD_USE_HISTORY_CONFIDENCE                              0 // almost no one uses this, but constant folding and dead code elimination work well on modern drivers
#define NRD_USE_DISOCCLUSION_THRESHOLD_MIX                      0 // almost no one uses this, but constant folding and dead code elimination work well on modern drivers
#define NRD_USE_BASECOLOR_METALNESS                             0 // almost no one uses this, but constant folding and dead code elimination work well on modern drivers
#define NRD_USE_SPECULAR_MOTION_V2                              1 // this method offers better IQ on bumpy and wavy surfaces, but it doesn't work on concave mirrors ( no motion acceleration )

// Switches ( default 0 )
#define NRD_USE_QUADRATIC_DISTRIBUTION                          0
#define NRD_USE_EXPONENTIAL_WEIGHTS                             0
#define NRD_USE_HIGH_PARALLAX_CURVATURE_SILHOUETTE_FIX          0 // it fixes silhouettes, but leads to less flattening on bumpy surfaces ( worse for bumpy surfaces ) and shorter arcs on smooth curved surfaces ( worse for low bit normals )
#define NRD_USE_VIEWPORT_OFFSET                                 0 // enable if "CommonSettings::rectOrigin" is used

// Settings
#define NRD_DISOCCLUSION_THRESHOLD                              0.02 // normalized % // TODO: use CommonSettings::disocclusionThreshold?
#define NRD_CATROM_SHARPNESS                                    0.5 // [ 0; 1 ], 0.5 matches Catmull-Rom
#define NRD_RADIANCE_COMPRESSION_MODE                           3 // 0-4, specular color compression for spatial passes
#define NRD_EXP_WEIGHT_DEFAULT_SCALE                            3.0
#define NRD_ROUGHNESS_SENSITIVITY                               0.01 // smaller => more sensitive
#define NRD_CURVATURE_Z_THRESHOLD                               0.1 // normalized %
#define NRD_MAX_ALLOWED_VIRTUAL_MOTION_ACCELERATION             15.0 // keep relatively high to avoid ruining concave mirrors
#define NRD_MAX_PERCENT_OF_LOBE_VOLUME                          0.75 // normalized % // TODO: have a gut feeling that it's too much...

#if( NRD_NORMAL_ENCODING < NRD_NORMAL_ENCODING_R10G10B10A2_UNORM )
    #define NRD_NORMAL_ENCODING_ERROR                           ( 1.50 / 255.0 )
    #define STOCHASTIC_BILINEAR_FILTER                          gLinearClamp
#elif( NRD_NORMAL_ENCODING == NRD_NORMAL_ENCODING_R10G10B10A2_UNORM )
    #define NRD_NORMAL_ENCODING_ERROR                           ( 0.75 / 255.0 )
    #define STOCHASTIC_BILINEAR_FILTER                          gNearestClamp
#else
    #define NRD_NORMAL_ENCODING_ERROR                           ( 0.50 / 255.0 )
    #define STOCHASTIC_BILINEAR_FILTER                          gLinearClamp
#endif

//==================================================================================================================
// CTA & preloading
//==================================================================================================================

// CTA swizzling
#define NRD_CS_MAIN_ARGS                                        int2 threadPos : SV_GroupThreadId, uint2 groupPos : SV_GroupId, int2 _pixelPos : SV_DispatchThreadId, uint threadIndex : SV_GroupIndex

// IMPORTANT: incompatible with "USE_MAX_DIMS", "IGNORE_RS" and dispatches with "downsampleFactor > 1"
#if 1
    // Helps to reuse data already stored in caches
    #define NRD_CTA_ORDER_REVERSED \
        const uint2 numGroups = ( uint2( gRectSize ) + uint2( GROUP_X, GROUP_Y ) - 1 ) / uint2( GROUP_X, GROUP_Y ); \
        const int2 pixelPos = ( numGroups - 1 - groupPos ) * uint2( GROUP_X, GROUP_Y ) + threadPos
#else
    #define NRD_CTA_ORDER_REVERSED \
        const int2 pixelPos = _pixelPos
#endif

#define NRD_CTA_ORDER_DEFAULT \
    const int2 pixelPos = _pixelPos

// Preloading in SMEM
#ifdef NRD_USE_BORDER_2
    #define BORDER                                              2
#else
    #define BORDER                                              1
#endif

#define BUFFER_X                                                ( GROUP_X + BORDER * 2 )
#define BUFFER_Y                                                ( GROUP_Y + BORDER * 2 )

#define PRELOAD_INTO_SMEM_WITH_TILE_CHECK \
    isSky *= NRD_USE_TILE_CHECK; \
    if( isSky == 0.0 ) \
    { \
        int2 groupBase = pixelPos - threadPos - BORDER; \
        uint stageNum = ( BUFFER_X * BUFFER_Y + GROUP_X * GROUP_Y - 1 ) / ( GROUP_X * GROUP_Y ); \
        [unroll] \
        for( uint stage = 0; stage < stageNum; stage++ ) \
        { \
            uint virtualIndex = threadIndex + stage * GROUP_X * GROUP_Y; \
            uint2 newId = uint2( virtualIndex % BUFFER_X, virtualIndex / BUFFER_X ); \
            if( stage == 0 || virtualIndex < BUFFER_X * BUFFER_Y ) \
                Preload( newId, groupBase + newId ); \
        } \
    } \
    GroupMemoryBarrierWithGroupSync( ); \
    /* Not an elegant way to solve loop variables declaration duplication problem */ \
    int i, j

#define PRELOAD_INTO_SMEM \
    int2 groupBase = pixelPos - threadPos - BORDER; \
    uint stageNum = ( BUFFER_X * BUFFER_Y + GROUP_X * GROUP_Y - 1 ) / ( GROUP_X * GROUP_Y ); \
    [unroll] \
    for( uint stage = 0; stage < stageNum; stage++ ) \
    { \
        uint virtualIndex = threadIndex + stage * GROUP_X * GROUP_Y; \
        uint2 newId = uint2( virtualIndex % BUFFER_X, virtualIndex / BUFFER_X ); \
        if( stage == 0 || virtualIndex < BUFFER_X * BUFFER_Y ) \
            Preload( newId, groupBase + newId ); \
    } \
    GroupMemoryBarrierWithGroupSync( ); \
    /* Not an elegant way to solve loop variables declaration duplication problem */ \
    int i, j

// Printf
/*
Usage:
    #ifdef PRINTF_AVAILABLE
        PrintfAt( "a = %f, b = %f, c = %u", a, b, c );
    #endif
*/
#if( defined( __hlsl_dx_compiler ) && defined( __spirv__ ) )
    #define PRINTF_AVAILABLE
    #define PrintfAt(...) \
        if( uint( pixelPos.x ) == gPrintfAt.x && uint( pixelPos.y ) == gPrintfAt.y ) \
            printf(__VA_ARGS__)
#endif

//==================================================================================================================
// KERNELS
//==================================================================================================================

static const float3 g_Special6[ 6 ] =
{
    // https://www.desmos.com/calculator/e5mttzlg6v
    float3( -0.50 * sqrt( 3.0 ) , -0.50             , 1.0 ),
    float3(  0.00               ,  1.00             , 1.0 ),
    float3(  0.50 * sqrt( 3.0 ) , -0.50             , 1.0 ),
    float3(  0.00               , -0.30             , 0.3 ),
    float3(  0.15 * sqrt( 3.0 ) ,  0.15             , 0.3 ),
    float3( -0.15 * sqrt( 3.0 ) ,  0.15             , 0.3 ),
};

static const float3 g_Special8[ 8 ] =
{
    // https://www.desmos.com/calculator/abaqyvswem
    float3( -1.00               ,  0.00               , 1.0 ),
    float3(  0.00               ,  1.00               , 1.0 ),
    float3(  1.00               ,  0.00               , 1.0 ),
    float3(  0.00               , -1.00               , 1.0 ),
    float3( -0.25 * sqrt( 2.0 ) ,  0.25 * sqrt( 2.0 ) , 0.5 ),
    float3(  0.25 * sqrt( 2.0 ) ,  0.25 * sqrt( 2.0 ) , 0.5 ),
    float3(  0.25 * sqrt( 2.0 ) , -0.25 * sqrt( 2.0 ) , 0.5 ),
    float3( -0.25 * sqrt( 2.0 ) , -0.25 * sqrt( 2.0 ) , 0.5 )
};

//==================================================================================================================
// SHARED FUNCTIONS
//==================================================================================================================

// Texture access

#if( NRD_USE_VIEWPORT_OFFSET == 1 )
    #define WithRectOrigin( pos )               ( gRectOrigin + pos )
    #define WithRectOffset( uv )                ( gRectOffset + uv )
#else
    #define WithRectOrigin( pos )               ( pos )
    #define WithRectOffset( uv )                ( uv )
#endif

#if( NRD_USE_DENANIFICATION == 1 )
    // clamp( uv * gRectSize, 0.0, gRectSize - 0.5 ) * gResourceSizeInv
    // = clamp( uv * gRectSize * gResourceSizeInv, 0.0, ( gRectSize - 0.5 ) * gResourceSizeInv )
    // = clamp( uv * gResolutionScale, 0.0, gResolutionScale - 0.5 * gResourceSizeInv )

    #if( NRD_USE_VIEWPORT_OFFSET == 1 )
        #define ClampUvToViewport( uv )         clamp( uv * gResolutionScale, 0.0, gResolutionScale - 0.5 * gResourceSizeInv )
    #else
        #define ClampUvToViewport( uv )         min( uv * gResolutionScale, gResolutionScale - 0.5 * gResourceSizeInv )
    #endif
    #define Denanify( w, x )                    ( ( w ) == 0.0 ? 0.0 : ( x ) )
#else
    #define ClampUvToViewport( uv )             ( uv * gResolutionScale )
    #define Denanify( w, x )                    ( x )
#endif

// Misc

// sigma = standard deviation, variance = sigma ^ 2
#define GetStdDev( m1, m2 )                     sqrt( abs( ( m2 ) - ( m1 ) * ( m1 ) ) ) // sqrt( max( m2 - m1 * m1, 0.0 ) )

#if( NRD_NORMAL_ENCODING == NRD_NORMAL_ENCODING_R10G10B10A2_UNORM )
    #define CompareMaterials( m0, m, minm )     ( max( m0, minm ) == max( m, minm ) )
#else
    #define CompareMaterials( m0, m, minm )     true
#endif

#define UnpackViewZ( z )                        abs( z * gViewZScale )

float PixelRadiusToWorld( float unproject, float orthoMode, float pixelRadius, float viewZ )
{
     return pixelRadius * unproject * lerp( viewZ, 1.0, abs( orthoMode ) );
}

float GetFrustumSize( float minRectDimMulUnproject, float orthoMode, float viewZ )
{
    // TODO: let's assume that all NRD instances are independent, i.e. there is no a need to reach
    // "frustumSize" parity between several instances. For example: 3-monitor setup, side monitors
    // are rotated, i.e. width < height
    return minRectDimMulUnproject * lerp( viewZ, 1.0, abs( orthoMode ) );
}

float GetHitDistFactor( float hitDist, float frustumSize )
{
    return saturate( hitDist / frustumSize );
}

float4 GetBlurKernelRotation( compiletime const uint mode, uint2 pixelPos, float4 baseRotator, uint frameIndex )
{
    if( mode == NRD_NONE )
        return Geometry::GetRotator( 0.0 );
    else if( mode == NRD_PIXEL )
    {
        float angle = Sequence::Bayer4x4( pixelPos, frameIndex );
        float4 rotator = Geometry::GetRotator( angle * Math::Pi( 2.0 ) );

        baseRotator = Geometry::CombineRotators( baseRotator, rotator );
    }
    else if( mode == NRD_RANDOM )
    {
        Rng::Hash::Initialize( pixelPos, frameIndex );

        float2 rnd = Rng::Hash::GetFloat2( );
        float4 rotator = Geometry::GetRotator( rnd.x * Math::Pi( 2.0 ) );
        rotator *= 1.0 + ( rnd.y * 2.0 - 1.0 ) * 0.5;

        baseRotator = Geometry::CombineRotators( baseRotator, rotator );
    }

    return baseRotator;
}

float IsInScreenNearest( float2 uv )
{
    return float( all( uv > 0.0 ) && all( uv < 1.0 ) );
}

// x y
// z w
float4 IsInScreenBilinear( float2 footprintOrigin, float2 rectSize )
{
    float4 p = footprintOrigin.xyxy + float4( 0, 0, 1, 1 );

    float4 r = float4( p >= 0.0 );
    r *= float4( p < rectSize.xyxy );

    return r.xzxz * r.yyww;
}

float2 ApplyCheckerboardShift( float2 pos, uint mode, uint counter, uint frameIndex )
{
    // IMPORTANT: "pos" must be snapped to the pixel center
    float2 posPositive = pos + 16384.0; // apply pattern-neutral bias, to make "pos" non-negative
    uint checkerboard = Sequence::CheckerBoard( uint2( posPositive ), frameIndex );
    float shift = ( ( counter & 0x1 ) == 0 ) ? -1.0 : 1.0;

    pos.x += shift * float( checkerboard != mode && mode != 2 );

    return pos;
}

// Comparison of two methods:
// https://www.desmos.com/calculator/xwq1nrawho
float GetSpecMagicCurve( float roughness, float power = 0.25 )
{
    float f = 1.0 - exp2( -200.0 * roughness * roughness );
    f *= Math::Pow01( roughness, power );

    return f;
}

float ComputeParallaxInPixels( float3 X, float2 uvForZeroParallax, float4x4 mWorldToClip, float2 rectSize )
{
    // Both produce same results, but behavior is different on objects attached to the camera:
    // non-0 parallax on pure translations, 0 parallax on pure rotations
    //      ComputeParallaxInPixels( Xprev + gCameraDelta, gOrthoMode == 0.0 ? smbPixelUv : pixelUv, gWorldToClipPrev, gRectSize );
    // 0 parallax on translations, non-0 parallax on pure rotations
    //      ComputeParallaxInPixels( Xprev - gCameraDelta, gOrthoMode == 0.0 ? pixelUv : smbPixelUv, gWorldToClip, gRectSize );

    float2 uv = Geometry::GetScreenUv( mWorldToClip, X );
    float2 parallaxInUv = uv - uvForZeroParallax;
    float parallaxInPixels = length( parallaxInUv * rectSize );

    return parallaxInPixels;
}

float GetColorCompressionExposureForSpatialPasses( float roughness )
{
    // Prerequsites:
    // - to minimize biasing the results compression for high roughness should be avoided (diffuse signal compression can lead to darker image)
    // - the compression function must be monotonic for full roughness range
    // - returned exposure must be used with colors in the HDR range used in tonemapping, i.e. "color * exposure"

    // Moderate compression
    #if( NRD_RADIANCE_COMPRESSION_MODE == 1 )
        return 0.5 / ( 1.0 + 50.0 * roughness );
    // Less compression for mid-high roughness
    #elif( NRD_RADIANCE_COMPRESSION_MODE == 2 )
        return 0.5 * ( 1.0 - roughness ) / ( 1.0 + 60.0 * roughness );
    // Close to the previous one, but offers more compression for low roughness
    #elif( NRD_RADIANCE_COMPRESSION_MODE == 3 )
        return 0.5 * ( 1.0 - roughness ) / ( 1.0 + 1000.0 * roughness * roughness ) + ( 1.0 - sqrt( saturate( roughness ) ) ) * 0.03;
    // A modification of the preious one ( simpler )
    #elif( NRD_RADIANCE_COMPRESSION_MODE == 4 )
        return 0.6 * ( 1.0 - roughness * roughness ) / ( 1.0 + 400.0 * roughness * roughness );
    // No compression
    #else
        return 0;
    #endif
}

float2 StochasticBilinear( float2 uv, float2 texSize )
{
#if( REBLUR_USE_STF == 1 && NRD_NORMAL_ENCODING == NRD_NORMAL_ENCODING_R10G10B10A2_UNORM )
    // Requires: Rng::Hash::Initialize( pixelPos, gFrameIndex )
    Filtering::Bilinear f = Filtering::GetBilinearFilter( uv, texSize );

    float2 rnd = Rng::Hash::GetFloat2( );
    f.origin += step( rnd, f.weights );

    return ( f.origin + 0.5 ) / texSize;
#else
    return uv;
#endif
}

// Thin lens

/*
https://computergraphics.stackexchange.com/questions/1718/what-is-the-simplest-way-to-compute-principal-curvature-for-a-mesh-triangle
https://www.geeksforgeeks.org/sign-convention-for-spherical-mirrors/

Thin lens equation:
    1 / F = 1 / O + 1 / I
    F = R / 2 - focal distance
    C = 1 / R - curvature

Sign convention:
    convex  : O(-), I(+), C(+)
    concave : O(-), I(+ or -), C(-)

Combine:
    2C = 1 / O + 1 / I
    1 / I = 2C - 1 / O
    1 / I = ( 2CO - 1 ) / O
    I = O / ( 2CO - 1 )
    mag = 1 / ( 2CO - 1 )

O is always negative, while hit distance is always positive:
    I = -O / ( -2CO - 1 )
    I = O / ( 2CO + 1 )

Interactive graph:
    https://www.desmos.com/calculator/dn9spdgwiz
*/

float ApplyThinLensEquation( float O, float curvature )
{
    float I = O / ( 2.0 * curvature * O + 1.0 );

    return I;
}

float3 GetXvirtual( float hitDist, float curvature, float3 X, float3 Xprev, float3 N, float3 V, float roughness )
{
    // GGX dominant direction
    float4 D = ImportanceSampling::GetSpecularDominantDirection( N, V, roughness, ML_SPECULAR_DOMINANT_DIRECTION_G2 );

    // It will be image position in world space relative to the primary hit
    float3 Iw = V;

    #if( NRD_USE_SPECULAR_MOTION_V2 == 0 )
        float hitDistFocused = ApplyThinLensEquation( hitDist, curvature );

        Iw *= hitDistFocused;
    #else
        // Specular ray direction can be used instead of GGX dominant direction
        float3 reflectionRay = D.xyz * hitDist;

        // Construct a basis for the reflector
        float3 principalAxis = N;
        float3x3 reflectorBasis = Geometry::GetBasis( principalAxis );

        // Object position
        float3 O = Geometry::RotateVector( reflectorBasis, reflectionRay );

        // O.z is always negative due to sign convention ( assuming no self intersection rays )
        O.z = -O.z;

        // Image position
        float mag = 1.0 / ( 2.0 * curvature * O.z - 1.0 );

        // Workaround: avoid smearing on silhouettes
        float f = length( X ); // if close to...
        f *= 1.0 - abs( dot( N, V ) ); // a silhouette...
        f *= max( curvature, 0.0 ); // of a convex hull...
        mag *= 1.0 / ( 1.0 + f ); // reduce magnification

        float3 I = O * mag;

        Iw *= length( I );

        // TODO: to fix bahavior on concave mirrors, signed world position is needed. But the code below can't be used "as is"
        // because motion can become inconsistent in some cases ( tests 133, 164, 171 - 176 ) leading to reprojection artefacts
        //Iw = Geometry::RotateVectorInverse( reflectorBasis, I ); // no
        //Iw *= -Math::Sign( mag ); // closer
    #endif

    // Only hit distance is provided, not real motion in the virtual world. If the virtual position is close to the
    // surface due to focusing, better to replace current position with previous position because surface motion is known
    float closenessToSurface = saturate( length( Iw ) / ( hitDist + NRD_EPS ) );
    float3 origin = lerp( Xprev, X, closenessToSurface * D.w );

    return origin - Iw * D.w;
}

// Kernel

float2 GetKernelSampleCoordinates( float4x4 mToClip, float3 offset, float3 X, float3 T, float3 B, float4 rotator = float4( 1, 0, 0, 1 ) )
{
    #if( NRD_USE_QUADRATIC_DISTRIBUTION == 1 )
        offset.xy *= offset.z;
    #endif

    // We can't rotate T and B instead, because they can be skewed
    offset.xy = Geometry::RotateVector( rotator, offset.xy );

    float3 p = X + T * offset.x + B * offset.y;
    float3 clip = Geometry::ProjectiveTransform( mToClip, p ).xyw;
    clip.xy /= clip.z;
    clip.y = -clip.y;

    float2 uv = clip.xy * 0.5 + 0.5;

    return uv;
}

// Weight parameters

float GetNormalWeightParam( float nonLinearAccumSpeed, float lobeAngleFraction, float roughness = 1.0 )
{
    float percentOfVolume = NRD_MAX_PERCENT_OF_LOBE_VOLUME * lerp( lobeAngleFraction, 1.0, nonLinearAccumSpeed );
    float tanHalfAngle = ImportanceSampling::GetSpecularLobeTanHalfAngle( roughness, percentOfVolume );

    // TODO: use gLobeAngleFraction = 0.1 ( non squared! ) and:
    //float tanHalfAngle = ImportanceSampling::GetSpecularLobeTanHalfAngle( roughness, NRD_MAX_PERCENT_OF_LOBE_VOLUME );
    //tanHalfAngle *= lerp( gLobeAngleFraction, 1.0, nonLinearAccumSpeed );

    float angle = atan( tanHalfAngle );
    angle = max( angle, NRD_NORMAL_ENCODING_ERROR );

    return 1.0 / angle;
}

float2 GetGeometryWeightParams( float planeDistSensitivity, float frustumSize, float3 Xv, float3 Nv, float nonLinearAccumSpeed )
{
    float norm = planeDistSensitivity * frustumSize;
    float a = 1.0 / norm;
    float b = dot( Nv, Xv ) * a;

    return float2( a, -b );
}

float2 GetHitDistanceWeightParams( float hitDist, float nonLinearAccumSpeed, float roughness = 1.0 )
{
    // TODO: compare "GetHitDistFactor"?
    // IMPORTANT: since this weight is exponential, 3% can lead to leaks from bright objects in reflections.
    // Even 1% is not enough in some cases, but using a lower value makes things even more fragile
    float smc = GetSpecMagicCurve( roughness );
    float norm = lerp( 0.0005, 1.0, min( nonLinearAccumSpeed, smc ) );
    float a = 1.0 / norm;
    float b = hitDist * a;

    return float2( a, -b );
}

float2 GetRoughnessWeightParams( float roughness, float fraction, float sensitivity = NRD_ROUGHNESS_SENSITIVITY )
{
    float a = 1.0 / lerp( sensitivity, 1.0, saturate( roughness * fraction ) );
    float b = roughness * a;

    return float2( a, -b );
}

float2 GetRelaxedRoughnessWeightParams( float m, float fraction = 1.0, float sensitivity = NRD_ROUGHNESS_SENSITIVITY )
{
    // "m = roughness * roughness" makes test less sensitive to small deltas

    // https://www.desmos.com/calculator/wkvacka5za
    float a = 1.0 / lerp( sensitivity, 1.0, lerp( m * m, m, fraction ) );
    float b = m * a;

    return float2( a, -b );
}

// Weights

// IMPORTANT:
// - works for "negative x" only
// - huge error for x < -2, but still applicable for "weight" calculations
// https://www.desmos.com/calculator/cd3mvg1gfo
#define ExpApprox( x ) \
    rcp( ( x ) * ( x ) - ( x ) + 1.0 )

// Must be used for noisy data
// https://www.desmos.com/calculator/9yoyc3is2g
// scale = 3-5 is needed to match energy in "ComputeNonExponentialWeight" ( especially when used in a recurrent loop )
#define ComputeExponentialWeight( x, px, py ) \
    ExpApprox( -NRD_EXP_WEIGHT_DEFAULT_SCALE * abs( ( x ) * px + py ) )

// A good choice for non noisy data
// Previously "Math::SmoothStep( 0.999, 0.001" was used, but it seems to be not needed anymore
#define ComputeNonExponentialWeight( x, px, py ) \
    Math::SmoothStep( 1.0, 0.0, abs( ( x ) * px + py ) )

#define ComputeNonExponentialWeightWithSigma( x, px, py, sigma ) \
    Math::SmoothStep( 1.0, 0.0, abs( ( x ) * px + py ) - sigma * px )

#if( NRD_USE_EXPONENTIAL_WEIGHTS == 1 )
    #define ComputeWeight( x, px, py )     ComputeExponentialWeight( x, px, py )
#else
    #define ComputeWeight( x, px, py )     ComputeNonExponentialWeight( x, px, py )
#endif

float GetGaussianWeight( float r )
{
    return exp( -0.66 * r * r ); // assuming r is normalized to 1
}

// Encoding precision aware weight functions ( for reprojection )

float GetEncodingAwareNormalWeight( float3 Ncurr, float3 Nprev, float maxAngle, float curvatureAngle, float thresholdAngle, bool remap = false )
{
    float cosa = dot( Ncurr, Nprev );
    float angle = Math::AcosApprox( cosa );
    float w = Math::SmoothStep01( 1.0 - ( angle - curvatureAngle - thresholdAngle ) / maxAngle );

    // Needed to mitigate imprecision issues because prev normals are RGBA8 ( test 3, 43 if roughness is low )
    if( remap ) // TODO: needed only for RELAX
        w = Math::SmoothStep( 0.05, 0.95, w );

    return w;
}

// Only for viewZ comparisons for close to each other pixels ( not sparse filters! )

float GetDisocclusionThreshold( float disocclusionThreshold, float frustumSize, float NoV )
{
    return frustumSize * saturate( disocclusionThreshold / max( 0.01, NoV ) );
}

#define GetDisocclusionWeight( z, z0, disocclusionThreshold ) step( abs( z - z0 ), disocclusionThreshold )

// Upsampling

#define _BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights_Init \
    /* Catmul-Rom with 12 taps ( excluding corners ) */ \
    float2 centerPos = floor( samplePos - 0.5 ) + 0.5; \
    float2 f = saturate( samplePos - centerPos ); \
    float2 w0 = f * ( f * ( -NRD_CATROM_SHARPNESS * f + 2.0 * NRD_CATROM_SHARPNESS ) - NRD_CATROM_SHARPNESS ); \
    float2 w1 = f * ( f * ( ( 2.0 - NRD_CATROM_SHARPNESS ) * f - ( 3.0 - NRD_CATROM_SHARPNESS ) ) ) + 1.0; \
    float2 w2 = f * ( f * ( -( 2.0 - NRD_CATROM_SHARPNESS ) * f + ( 3.0 - 2.0 * NRD_CATROM_SHARPNESS ) ) + NRD_CATROM_SHARPNESS ); \
    float2 w3 = f * ( f * ( NRD_CATROM_SHARPNESS * f - NRD_CATROM_SHARPNESS ) ); \
    float2 w12 = w1 + w2; \
    float2 tc = w2 / w12; \
    float4 w; \
    w.x = w12.x * w0.y; \
    w.y = w0.x * w12.y; \
    w.z = w12.x * w12.y; \
    w.w = w3.x * w12.y; \
    float w4 = w12.x * w3.y; \
    /* Fallback to custom bilinear */ \
    w = useBicubic ? w : bilinearCustomWeights; \
    w4 = useBicubic ? w4 : 0.0; \
    float sum = dot( w, 1.0 ) + w4; \
    /* Texture coordinates */ \
    float4 uv01 = centerPos.xyxy + ( useBicubic ? float4( tc.x, -1.0, -1.0, tc.y ) : float4( 0, 0, 1, 0 ) ); \
    float4 uv23 = centerPos.xyxy + ( useBicubic ? float4( tc.x, tc.y, 2.0, tc.y ) : float4( 0, 1, 1, 1 ) ); \
    float2 uv4 = centerPos + ( useBicubic ? float2( tc.x, 2.0 ) : f ); \
    uv01 *= invResourceSize.xyxy; \
    uv23 *= invResourceSize.xyxy; \
    uv4 *= invResourceSize; \
    int3 bilinearOrigin = int3( centerPos, 0 );

/*
IMPORTANT:
- 0 can be returned if only a single tap is valid from the 2x2 footprint and pure bilinear weights
  are close to 0 near this tap. The caller must handle this case manually. "Footprint quality"
  can be used to accelerate accumulation and avoid the problem.
- can return negative values
*/
#define _BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights_Color( color, tex ) \
    /* Sampling */ \
    color = tex.SampleLevel( gLinearClamp, uv01.xy, 0 ) * w.x; \
    color += tex.SampleLevel( gLinearClamp, uv01.zw, 0 ) * w.y; \
    color += tex.SampleLevel( gLinearClamp, uv23.xy, 0 ) * w.z; \
    color += tex.SampleLevel( gLinearClamp, uv23.zw, 0 ) * w.w; \
    color += tex.SampleLevel( gLinearClamp, uv4, 0 ) * w4; \
    /* Normalize similarly to "Filtering::ApplyBilinearCustomWeights()" */ \
    color = sum < 0.0001 ? 0 : color / sum;

#define _BilinearFilterWithCustomWeights_Color( color, tex ) \
    /* Sampling */ \
    color = tex.Load( bilinearOrigin ) * bilinearCustomWeights.x; \
    color += tex.Load( bilinearOrigin, int2( 1, 0 ) ) * bilinearCustomWeights.y; \
    color += tex.Load( bilinearOrigin, int2( 0, 1 ) ) * bilinearCustomWeights.z; \
    color += tex.Load( bilinearOrigin, int2( 1, 1 ) ) * bilinearCustomWeights.w; \
    /* Normalize similarly to "Filtering::ApplyBilinearCustomWeights()" */ \
    sum = dot( bilinearCustomWeights, 1.0 ); \
    color = sum < 0.0001 ? 0 : color / sum;
