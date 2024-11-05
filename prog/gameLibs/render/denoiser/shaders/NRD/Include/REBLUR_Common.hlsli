/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

// Spatial filtering

#define REBLUR_PRE_BLUR                                 0
#define REBLUR_BLUR                                     1
#define REBLUR_POST_BLUR                                2

// Kernels
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

// Storage

#define REBLUR_MAX_ACCUM_FRAME_NUM                      63.0
#define REBLUR_ACCUMSPEED_BITS                          7 // "( 1 << REBLUR_ACCUMSPEED_BITS ) - 1" must be >= REBLUR_MAX_ACCUM_FRAME_NUM
#define REBLUR_MATERIALID_BITS                          ( 16 - REBLUR_ACCUMSPEED_BITS - REBLUR_ACCUMSPEED_BITS )

// Internal data ( from the previous frame )

#define PackViewZ( p )      min( p * NRD_FP16_VIEWZ_SCALE, NRD_FP16_MAX )
#define UnpackViewZ( p )    ( p / NRD_FP16_VIEWZ_SCALE )

float4 PackNormalRoughness( float4 p )
{
    return float4( p.xyz * 0.5 + 0.5, p.w );
}

float4 UnpackNormalAndRoughness( float4 p, bool isNormalized = true )
{
    p.xyz = p.xyz * 2.0 - 1.0;

    if( isNormalized )
        p.xyz = _NRD_SafeNormalize( p.xyz );

    return p;
}

uint PackInternalData( float diffAccumSpeed, float specAccumSpeed, float materialID )
{
    float3 t = float3( diffAccumSpeed, specAccumSpeed, materialID );
    t.xy /= REBLUR_MAX_ACCUM_FRAME_NUM;

    uint p = STL::Packing::RgbaToUint( t.xyzz, REBLUR_ACCUMSPEED_BITS, REBLUR_ACCUMSPEED_BITS, REBLUR_MATERIALID_BITS, 0 );

    return p;
}

float3 UnpackInternalData( uint p )
{
    float3 t = STL::Packing::UintToRgba( p, REBLUR_ACCUMSPEED_BITS, REBLUR_ACCUMSPEED_BITS, REBLUR_MATERIALID_BITS, 0 ).xyz;
    t.xy *= REBLUR_MAX_ACCUM_FRAME_NUM;

    return t;
}

// Intermediate data ( in the current frame )

float4 PackData1( float diffAccumSpeed, float diffError, float specAccumSpeed, float specError )
{
    float4 r;
    r.x = saturate( diffAccumSpeed / REBLUR_MAX_ACCUM_FRAME_NUM );
    r.y = diffError;
    r.z = saturate( specAccumSpeed / REBLUR_MAX_ACCUM_FRAME_NUM );
    r.w = specError;

    // Allow RG8_UNORM for specular only denoiser
    #ifndef REBLUR_DIFFUSE
        r.xy = r.zw;
    #endif

    return r;
}

float4 UnpackData1( float4 p )
{
    // Allow R8_UNORM for specular only denoiser
    #ifndef REBLUR_DIFFUSE
        p.zw = p.xy;
    #endif

    p.xz *= REBLUR_MAX_ACCUM_FRAME_NUM;

    return p;
}

uint PackData2( float fbits, float curvature, float virtualHistoryAmount )
{
    // BITS:
    // 0-3 - smbOcclusion 2x2
    // 4-7 - vmbOcclusion 2x2

    uint p = uint( fbits + 0.5 );
    p |= uint( saturate( virtualHistoryAmount ) * 255.0 + 0.5 ) << 8;
    p |= f32tof16( curvature ) << 16;

    return p;
}

float2 UnpackData2( uint p, out uint bits )
{
    bits = p & 0xFF;

    float virtualHistoryAmount = float( ( p >> 8 ) & 0xFF ) / 255.0;
    float curvature = f16tof32( p >> 16 );

    return float2( virtualHistoryAmount, curvature );
}

// Misc

float3 GetViewVector( float3 X, bool isViewSpace = false )
{
    return gOrthoMode == 0.0 ? normalize( -X ) : ( isViewSpace ? float3( 0, 0, -1 ) : gViewVectorWorld.xyz );
}

float3 GetViewVectorPrev( float3 Xprev, float3 cameraDelta )
{
    return gOrthoMode == 0.0 ? normalize( cameraDelta - Xprev ) : gViewVectorWorldPrev.xyz;
}

float GetMinAllowedLimitForHitDistNonLinearAccumSpeed( float roughness )
{
    // TODO: accelerate hit dist accumulation instead of limiting max number of frames?
    // Despite that hit distance weight is exponential, this function can't return 0, because
    // strict "hitDist" weight and effects of feedback loop lead to color banding ( crunched colors )
    float frameNum = 0.5 * GetSpecMagicCurve( roughness ) * gMaxAccumulatedFrameNum;

    return 1.0 / ( 1.0 + frameNum );
}

float GetFadeBasedOnAccumulatedFrames( float accumSpeed )
{
    float a = gHistoryFixFrameNum * 2.0 / 3.0 + 1e-6;
    float b = gHistoryFixFrameNum * 4.0 / 3.0 + 2e-6;

    return STL::Math::LinearStep( a, b, accumSpeed );
}

float GetNonLinearAccumSpeed( float accumSpeed, float maxAccumSpeed, float confidence, bool hasData )
{
    #if( REBLUR_USE_CONFIDENCE_NON_LINEARLY == 1 )
        float nonLinearAccumSpeed = max( 1.0 - confidence, 1.0 / ( 1.0 + min( accumSpeed, maxAccumSpeed ) ) );
    #else
        float nonLinearAccumSpeed = 1.0 / ( 1.0 + min( accumSpeed, maxAccumSpeed * confidence ) );
    #endif

    if( !hasData )
        nonLinearAccumSpeed *= lerp( 1.0 - gCheckerboardResolveAccumSpeed, 1.0, nonLinearAccumSpeed );

    return nonLinearAccumSpeed;
}

// Misc ( templates )

// Hit distance is normalized
float ClampNegativeHitDistToZero( float hitDist )
{ return saturate( hitDist ); }

float GetLumaScale( float currLuma, float newLuma )
{
    // IMPORTANT" "saturate" of below must be used if "vmbAllowCatRom = vmbAllowCatRom && specAllowCatRom"
    // is not used. But we can't use "saturate" because otherwise fast history clamping won't be able to
    // increase energy ( i.e. clamp a lower value to a bigger one from fast history ).

    return ( newLuma + NRD_EPS ) / ( currLuma + NRD_EPS );
}

#ifdef REBLUR_OCCLUSION

    #define REBLUR_TYPE float

    float MixHistoryAndCurrent( float history, float current, float f, float roughness = 1.0 )
    {
        float r = lerp( history, current, max( f, GetMinAllowedLimitForHitDistNonLinearAccumSpeed( roughness ) ) );

        return r;
    }

    float ExtractHitDist( float input )
    { return input; }

    float GetLuma( float input )
    { return input; }

    float ChangeLuma( float input, float newLuma )
    { return input * GetLumaScale( input, newLuma ); }

    float ClampNegativeToZero( float input )
    { return ClampNegativeHitDistToZero( input ); }

    float Xxxy( float2 w )
    { return w.y; }

#elif( defined REBLUR_DIRECTIONAL_OCCLUSION )

    #define REBLUR_TYPE float4

    float4 MixHistoryAndCurrent( float4 history, float4 current, float f, float roughness = 1.0 )
    {
        float4 r;
        r.xyz = lerp( history.xyz, current.xyz, f );
        r.w = lerp( history.w, current.w, max( f, GetMinAllowedLimitForHitDistNonLinearAccumSpeed( roughness ) ) );

        return r;
    }

    float ExtractHitDist( float4 input )
    { return input.w; }

    float GetLuma( float4 input )
    { return input.w; }

    float4 ChangeLuma( float4 input, float newLuma )
    {
        return input * GetLumaScale( GetLuma( input ), newLuma );
    }

    float4 ClampNegativeToZero( float4 input )
    { return ChangeLuma( input, ClampNegativeHitDistToZero( input.w ) ); }

    float4 Xxxy( float2 w )
    { return w.xxxy; }

#else

    #define REBLUR_TYPE float4

    float4 MixHistoryAndCurrent( float4 history, float4 current, float f, float roughness = 1.0 )
    {
        float4 r;
        r.xyz = lerp( history.xyz, current.xyz, f );
        r.w = lerp( history.w, current.w, max( f, GetMinAllowedLimitForHitDistNonLinearAccumSpeed( roughness ) ) );

        return r;
    }

    float ExtractHitDist( float4 input )
    { return input.w; }

    float GetLuma( float4 input )
    {
        #if( REBLUR_USE_YCOCG == 1 )
            return input.x;
        #else
            return _NRD_Luminance( input.xyz );
        #endif
    }

    float4 ChangeLuma( float4 input, float newLuma )
    {
        input.xyz *= GetLumaScale( GetLuma( input ), newLuma );

        return input;
    }

    float4 ClampNegativeToZero( float4 input )
    {
        #if( REBLUR_USE_YCOCG == 1 )
            input.xyz = _NRD_YCoCgToLinear( input.xyz );
            input.xyz = _NRD_LinearToYCoCg( input.xyz );
        #else
            input.xyz = max( input.xyz, 0.0 );
        #endif

        input.w = ClampNegativeHitDistToZero( input.w );

        return input;
    }

    float4 Xxxy( float2 w )
    { return w.xxxy; }

#endif

float GetColorErrorForAdaptiveRadiusScale( REBLUR_TYPE curr, REBLUR_TYPE prev, float accumSpeed, float roughness = 1.0 )
{
    // TODO: track temporal variance instead
    float2 p = float2( GetLuma( prev ), ExtractHitDist( prev ) );
    float2 c = float2( GetLuma( curr ), ExtractHitDist( curr ) );

    float2 a = abs( c - p ) - NRD_EPS;
    float2 b = max( c, p ) + NRD_EPS;
    float2 d = a / b;

    float error = STL::Math::SmoothStep01( max( d.x, d.y ) );
    error *= GetFadeBasedOnAccumulatedFrames( accumSpeed );

    return error;
}

float ComputeAntilag( REBLUR_TYPE history, REBLUR_TYPE signal, REBLUR_TYPE sigma, float4 antilagParams, float accumSpeed )
{
    float2 slow = float2( GetLuma( history ), ExtractHitDist( history ) );
    float2 fast = float2( GetLuma( signal ), ExtractHitDist( signal ) );
    float2 s = float2( GetLuma( sigma ), ExtractHitDist( sigma ) );

    float2 a = abs( slow - fast ) - antilagParams.xy * s - NRD_EPS;
    float2 b = max( slow, fast ) + antilagParams.xy * s + NRD_EPS;
    float2 d = a / b;

    d = STL::Math::SmoothStep01( 1.0 - d );
    d = STL::Math::Pow01( d, antilagParams.zw );

    // Needed at least for test 156, especially when motion is accelerated
    d = lerp( 1.0, d, GetFadeBasedOnAccumulatedFrames( accumSpeed ) );

    return REBLUR_SHOW == 0 ? min( d.x, d.y ) : 1.0;
}

// Kernel

float GetResponsiveAccumulationAmount( float roughness )
{
    float amount = 1.0 - ( roughness + NRD_EPS ) / ( gResponsiveAccumulationRoughnessThreshold + NRD_EPS );

    return STL::Math::SmoothStep01( amount );
}

float2x3 GetKernelBasis( float3 D, float3 N, float NoD, float roughness = 1.0, float anisoFade = 1.0 )
{
    float3x3 basis = STL::Geometry::GetBasis( N );

    float3 T = basis[ 0 ];
    float3 B = basis[ 1 ];

    if( NoD < 0.999 )
    {
        float3 R = reflect( -D, N );
        T = normalize( cross( N, R ) );
        B = cross( R, T );

        float skewFactor = lerp( 0.5 + 0.5 * roughness, 1.0, NoD );
        T *= lerp( skewFactor, 1.0, anisoFade );
    }

    return float2x3( T, B );
}

// Weight parameters

float GetNormalWeightParams( float nonLinearAccumSpeed, float fraction, float roughness = 1.0 )
{
    float angle = STL::ImportanceSampling::GetSpecularLobeHalfAngle( roughness );
    angle *= lerp( saturate( fraction ), 1.0, nonLinearAccumSpeed ); // TODO: use as "percentOfVolume" instead?

    return 1.0 / max( angle, NRD_NORMAL_ULP );
}

float2 GetTemporalAccumulationParams( float isInScreenMulFootprintQuality, float accumSpeed )
{
    accumSpeed *= REBLUR_SAMPLES_PER_FRAME;

    float w = isInScreenMulFootprintQuality;
    w *= accumSpeed / ( 1.0 + accumSpeed );
    w *= float( REBLUR_SHOW == 0 );

    return float2( w, 1.0 + 3.0 * gFramerateScale * w );
}

// Weights

void BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights(
    float2 samplePos, float2 invResourceSize,
    float4 bilinearCustomWeights, bool useBicubic,
    Texture2D<REBLUR_TYPE> tex0, out REBLUR_TYPE c0 ) // main - CatRom
{
    _BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights_Init;
    _BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights_Color( c0, tex0 );
}

void BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights(
    float2 samplePos, float2 invResourceSize,
    float4 bilinearCustomWeights, bool useBicubic,
    Texture2D<REBLUR_TYPE> tex0, out REBLUR_TYPE c0, // main - CatRom
    Texture2D<REBLUR_FAST_TYPE> tex1, out REBLUR_FAST_TYPE c1 ) // fast - bilinear
{
    _BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights_Init;
    _BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights_Color( c0, tex0 );
    _BilinearFilterWithCustomWeights_Color( c1, tex1 );
}

void BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights(
    float2 samplePos, float2 invResourceSize,
    float4 bilinearCustomWeights, bool useBicubic,
    Texture2D<REBLUR_TYPE> tex0, out REBLUR_TYPE c0, // main - CatRom
    Texture2D<REBLUR_SH_TYPE> tex1, out REBLUR_SH_TYPE c1 ) // SH - bilinear
{
    _BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights_Init;
    _BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights_Color( c0, tex0 );
    _BilinearFilterWithCustomWeights_Color( c1, tex1 );
}

void BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights(
    float2 samplePos, float2 invResourceSize,
    float4 bilinearCustomWeights, bool useBicubic,
    Texture2D<REBLUR_TYPE> tex0, out REBLUR_TYPE c0, // main - CatRom
    Texture2D<REBLUR_FAST_TYPE> tex1, out REBLUR_FAST_TYPE c1, // fast - bilinear
    Texture2D<REBLUR_SH_TYPE> tex2, out REBLUR_SH_TYPE c2 ) // SH - bilinear
{
    _BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights_Init;
    _BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights_Color( c0, tex0 );
    _BilinearFilterWithCustomWeights_Color( c1, tex1 );
    _BilinearFilterWithCustomWeights_Color( c2, tex2 );
}
