/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

// Internal data ( from the previous frame )

uint PackInternalData( float diffAccumSpeed, float specAccumSpeed, float materialID )
{
    float3 t;
    t.xy = float2( diffAccumSpeed, specAccumSpeed ) / REBLUR_MAX_ACCUM_FRAME_NUM;
    t.z = materialID / REBLUR_MAX_MATERIALID_NUM;

    uint p = Packing::RgbaToUint( t.xyzz, REBLUR_ACCUMSPEED_BITS, REBLUR_ACCUMSPEED_BITS, REBLUR_MATERIALID_BITS, 0 );

    return p;
}

float3 UnpackInternalData( uint p )
{
    float3 t = Packing::UintToRgba( p, REBLUR_ACCUMSPEED_BITS, REBLUR_ACCUMSPEED_BITS, REBLUR_MATERIALID_BITS, 0 ).xyz;
    t.xy *= REBLUR_MAX_ACCUM_FRAME_NUM;
    t.z *= REBLUR_MAX_MATERIALID_NUM;

    return t;
}

// Intermediate data ( in the current frame )

float2 PackData1( float diffAccumSpeed, float specAccumSpeed )
{
    float2 r;
    r.x = saturate( diffAccumSpeed / REBLUR_MAX_ACCUM_FRAME_NUM );
    r.y = saturate( specAccumSpeed / REBLUR_MAX_ACCUM_FRAME_NUM );

    // Allow R8_UNORM for specular only denoiser
    #ifndef REBLUR_DIFFUSE
        r.x = r.y;
    #endif

    return r;
}

float2 UnpackData1( float2 p )
{
    // Allow R8_UNORM for specular only denoiser
    #ifndef REBLUR_DIFFUSE
        p.y = p.x;
    #endif

    return p * REBLUR_MAX_ACCUM_FRAME_NUM;
}

uint PackData2( float fbits, float curvature, float virtualHistoryAmount, bool smbAllowCatRom )
{
    // BITS:
    // 0-3 - smbOcclusion 2x2
    // 4-7 - vmbOcclusion 2x2
    // 8-14 - virtualHistoryAmount
    // 15 - smbAllowCatRom
    // 16-31 - curvature

    uint p = uint( fbits + 0.5 );
    p |= uint( saturate( virtualHistoryAmount ) * 127.0 + 0.5 ) << 8;
    p |= smbAllowCatRom ? ( 1 << 15 ) : 0;
    p |= f32tof16( curvature ) << 16;

    return p;
}

float2 UnpackData2( uint p, out uint bits, out bool smbAllowCatRom )
{
    bits = p & 0xFF;
    smbAllowCatRom = ( p & ( 1 << 15 ) ) != 0;

    float virtualHistoryAmount = float( ( p >> 8 ) & 127 ) / 127.0;
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

    return Math::LinearStep( a, b, accumSpeed );
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

float RemapRoughnessToResponsiveFactor( float roughness )
{
    float amount = ( roughness + NRD_EPS ) / ( gResponsiveAccumulationRoughnessThreshold + NRD_EPS );

    return Math::SmoothStep01( amount );
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
    { return newLuma; }

    float ClampNegativeToZero( float input )
    { return ClampNegativeHitDistToZero( input ); }

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
    { return float4( input.xyz * GetLumaScale( input.w, newLuma ), newLuma ); }

    float4 ClampNegativeToZero( float4 input )
    { return ChangeLuma( input, ClampNegativeHitDistToZero( input.w ) ); }

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

#endif

float ComputeAntilag( float history, float avg, float sigma, float accumSpeed )
{
    // Tests 4, 36, 44, 47, 95 ( no SHARC, stop animation )
    float h = history;
    float a = avg;
    float s = sigma * gAntilagParams.x;
    float magic = gAntilagParams.y * gFramerateScale * gFramerateScale;

    #if( REBLUR_ANTILAG_MODE == 0 )
        // Old mode, but uses better threshold ( not bad )
        float x = abs( h - a ) - s - NRD_EPS;
        float y = max( h, a ) + s + NRD_EPS;
        float d = x / y;

        d = Math::LinearStep( magic / ( 1.0 + accumSpeed ), 0.0, d );
    #elif( REBLUR_ANTILAG_MODE == 1 )
        // New mode, but overly reactive at low FPS ( undesired )
        float hc = Color::Clamp( a, s, h );
        float d = abs( h - hc ) / ( max( h, hc ) + NRD_EPS );

        d = Math::LinearStep( magic / ( 1.0 + accumSpeed ), 0.0, d );
    #else
        // New mode, pretends to respect "no harm" idiom ( best? )
        float hc = Color::Clamp( a, s, h );
        float d = abs( h - hc ) / ( max( h, hc ) + NRD_EPS );

        d = 1.0 / ( 1.0 + d * accumSpeed / magic );
    #endif

    return REBLUR_SHOW == 0 ? d : 1.0;
}

// Kernel

float2x3 GetKernelBasis( float3 D, float3 N )
{
    float3x3 basis = Geometry::GetBasis( N );

    float3 T = basis[ 0 ];
    float3 B = basis[ 1 ];

    if( abs( dot( D, N ) ) < 0.999 )
    {
        float3 R = reflect( -D, N );
        T = normalize( cross( N, R ) );
        B = cross( R, T );
    }

    return float2x3( T, B );
}

// Weight parameters

float2 GetTemporalAccumulationParams( float isInScreenMulFootprintQuality, float accumSpeed )
{
    accumSpeed *= REBLUR_SAMPLES_PER_FRAME;

    float w = isInScreenMulFootprintQuality;
    w *= accumSpeed / ( 1.0 + accumSpeed );
    w *= float( REBLUR_SHOW == 0 );

    return float2( w, 1.0 + 3.0 * gFramerateScale * w );
}

// Filtering

void BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights1(
    float2 samplePos, float2 invResourceSize,
    float4 bilinearCustomWeights, bool useBicubic,
    Texture2D<float> tex0, out float c0 ) // CatRom
{
    _BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights_Init;
    _BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights_Color( c0, tex0 );
}

void BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights(
    float2 samplePos, float2 invResourceSize,
    float4 bilinearCustomWeights, bool useBicubic,
    Texture2D<REBLUR_TYPE> tex0, out REBLUR_TYPE c0 ) // CatRom
{
    _BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights_Init;
    _BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights_Color( c0, tex0 );
}

void BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights(
    float2 samplePos, float2 invResourceSize,
    float4 bilinearCustomWeights, bool useBicubic,
    Texture2D<REBLUR_TYPE> tex0, out REBLUR_TYPE c0, // CatRom
    Texture2D<REBLUR_FAST_TYPE> tex1, out REBLUR_FAST_TYPE c1 ) // bilinear
{
    _BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights_Init;
    _BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights_Color( c0, tex0 );
    _BilinearFilterWithCustomWeights_Color( c1, tex1 );
}

void BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights(
    float2 samplePos, float2 invResourceSize,
    float4 bilinearCustomWeights, bool useBicubic,
    Texture2D<REBLUR_TYPE> tex0, out REBLUR_TYPE c0, // CatRom
    Texture2D<REBLUR_SH_TYPE> tex1, out REBLUR_SH_TYPE c1 ) // bilinear
{
    _BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights_Init;
    _BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights_Color( c0, tex0 );
    _BilinearFilterWithCustomWeights_Color( c1, tex1 );
}

void BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights(
    float2 samplePos, float2 invResourceSize,
    float4 bilinearCustomWeights, bool useBicubic,
    Texture2D<REBLUR_TYPE> tex0, out REBLUR_TYPE c0, // CatRom
    Texture2D<REBLUR_FAST_TYPE> tex1, out REBLUR_FAST_TYPE c1, // bilinear
    Texture2D<REBLUR_SH_TYPE> tex2, out REBLUR_SH_TYPE c2 ) // bilinear
{
    _BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights_Init;
    _BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights_Color( c0, tex0 );
    _BilinearFilterWithCustomWeights_Color( c1, tex1 );
    _BilinearFilterWithCustomWeights_Color( c2, tex2 );
}
