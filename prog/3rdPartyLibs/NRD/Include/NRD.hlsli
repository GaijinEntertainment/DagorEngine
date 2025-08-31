/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

// NRD v4.14

// IMPORTANT: DO NOT MODIFY THIS FILE WITHOUT FULL RECOMPILATION OF NRD LIBRARY!

//=================================================================================================================================
// INPUT PARAMETERS
//=================================================================================================================================
/*
NON-NOISY INPUTS:
    float viewZ:
        - linear view space Z for primary rays ( linearized camera depth )

    float normal:
        - world-space normal

    float roughness:
        - "linear roughness" = sqrt( "m" ), where "m" = "alpha" - GGX roughness
        - usage: "isDiffuse ? 1.0 : roughness"

    float tanOfLightAngularRadius:
        - tan( lightAngularSize * 0.5 )
        - angular size is computed from the shadow receiving point
        - in other words, tanOfLightAngularRadius = lightRadius / distanceToLight

NOISY INPUTS:
    float3 radiance:
        - radiance should not include material information ( use material de-modulation to decouple materials )
        - radiance should not be premultiplied by "exposure"
        - for Primary Surface Replacements ( PSR ) throughput should be de-modulated as much as possible ( see test 184 from the sample and TraceOpaque.hlsl )
        - for diffuse rays
            - use COS-distribution ( or custom importance sampling )
            - if radiance is the result of path tracing, pass normalized hit distance as the sum of 1-all hits (always ignore primary hit!)
        - for specular
            - use VNDF sampling ( or custom importance sampling )
                - most advanced v3 version: https://gpuopen.com/download/publications/Bounded_VNDF_Sampling_for_Smith-GGX_Reflections.pdf
            - if radiance is the result of path tracing, pass hit distance for the 1st bounce for the first time (always ignore primary hit!)

    float hitDist:
        - can't be negative
        - must not include primary hit distance
        - for the first bounce after the primary hit or PSR must be provided "as is"
        - for susequent bounces must be adjusted by curvature and lobe energy dissipation on the application side
        - must be explicitly set to 0 for rays pointing inside the surface ( better to nopt cast such rays )

    float normHitDist:
        - logically same as "hitDist", but normalized to [0; 1] range using "REBLUR_FrontEnd_GetNormHitDist"
        - REBLUR must be aware of the normalization function via "nrd::HitDistanceParameters"
        - by definition, normalized hit distance is AO ( ambient occlusion ) for diffuse and SO ( specular occlusion ) for specular
        - AO can be used to emulate 2nd+ diffuse bounces
        - SO can be used to adjust IBL lighting
        - ".w" channel of diffuse / specular output is AO / SO
        - if you don't know which normalization function to choose use default values of "nrd::HitDistanceParameters"

    float distanceToOccluder:
        - distance to occluder, must follow the rules:
            - NoL <= 0         - 0 ( it's very important )
            - NoL > 0 ( hit )  - hit distance
            - NoL > 0 ( miss ) - >= NRD_FP16_MAX
*/

#ifndef NRD_INCLUDED
#define NRD_INCLUDED

//=================================================================================================================================
// SETTINGS
//=================================================================================================================================

// ( Optional ) Bindings
#define NRD_CONSTANT_BUFFER_SPACE_INDEX                                                 0
#define NRD_CONSTANT_BUFFER_REGISTER_INDEX                                              0

#define NRD_SAMPLERS_SPACE_INDEX                                                        0 // TODO: better keep in a separate space for sharing

#define NRD_RESOURCES_SPACE_INDEX                                                       0

// ( Optional ) Entry point
#ifndef NRD_CS_MAIN
    #define NRD_CS_MAIN                                                                 main
#endif

//=================================================================================================================================
// BINDINGS
//=================================================================================================================================

#ifndef __cplusplus

#define NRD_MERGE_TOKENS_( _0, _1 )                                                     _0 ## _1
#define NRD_MERGE_TOKENS( _0, _1 )                                                      NRD_MERGE_TOKENS_( _0, _1 )

// Custom engine that defined all the macros
#if( defined( NRD_CONSTANTS_START ) && \
     defined( NRD_CONSTANT ) && \
     defined( NRD_CONSTANTS_END ) && \
     defined( NRD_INPUTS_START ) && \
     defined( NRD_INPUT ) && \
     defined( NRD_INPUTS_END ) && \
     defined( NRD_OUTPUTS_START ) && \
     defined( NRD_OUTPUT ) && \
     defined( NRD_OUTPUTS_END ) && \
     defined( NRD_SAMPLERS_START ) && \
     defined( NRD_SAMPLER ) && \
     defined( NRD_SAMPLERS_END ) )

    #define NRD_EXPORT

// DXC
#elif( defined( NRD_COMPILER_DXC ) || defined( __hlsl_dx_compiler ) )

    #define NRD_CONSTANTS_START( resourceName )                                         cbuffer resourceName : register( NRD_MERGE_TOKENS( b, NRD_CONSTANT_BUFFER_REGISTER_INDEX ), NRD_MERGE_TOKENS( space, NRD_CONSTANT_BUFFER_SPACE_INDEX ) ) {
    #define NRD_CONSTANT( constantType, constantName )                                  constantType constantName;
    #define NRD_CONSTANTS_END                                                           };

    #define NRD_INPUTS_START
    #define NRD_INPUT( resourceType, resourceName, regName, bindingIndex )              resourceType resourceName : register( NRD_MERGE_TOKENS( regName, bindingIndex ), NRD_MERGE_TOKENS( space, NRD_RESOURCES_SPACE_INDEX ) );
    #define NRD_INPUTS_END

    #define NRD_OUTPUTS_START
    #define NRD_OUTPUT( resourceType, resourceName, regName, bindingIndex )             resourceType resourceName : register( NRD_MERGE_TOKENS( regName, bindingIndex ), NRD_MERGE_TOKENS( space, NRD_RESOURCES_SPACE_INDEX ) );
    #define NRD_OUTPUTS_END

    #define NRD_SAMPLERS_START
    #define NRD_SAMPLER( resourceType, resourceName, regName, bindingIndex )            resourceType resourceName : register( NRD_MERGE_TOKENS( regName, bindingIndex ), NRD_MERGE_TOKENS( space, NRD_SAMPLERS_SPACE_INDEX ) );
    #define NRD_SAMPLERS_END

    #define NRD_EXPORT

    #ifndef NRD_COMPILER_DXC
        #define NRD_COMPILER_DXC
    #endif

// PlayStation // TODO: register spaces?
#elif( defined( NRD_COMPILER_PSSLC ) || defined( __PSSL__ ) )

    #define EXPAND( x )                                                                 x
    #define GET_NTH_MACRO_4_arg( a, b, c, d, NAME, ... )                                NAME
    #define GET_NTH_MACRO_3_arg( a, b, c, NAME, ... )                                   NAME
    #define SampleLevel3( a, b, c )                                                     SampleLOD( a, b, ( float )( c ) )
    #define SampleLevel4( a, b, c, d )                                                  SampleLOD( a, b, ( float )( c ), d )
    #define GatherRed3( a, b, c )                                                       GatherRed( ( a ), ( b ), int2( c ) )
    #define GatherRed2( a, b )                                                          GatherRed( ( a ), ( b ) )
    #define GatherGreen3( a, b, c )                                                     GatherGreen( ( a ), ( b ), int2( c ) )
    #define GatherGreen2( a, b )                                                        GatherGreen( ( a ), ( b ) )

    #define NRD_CONSTANTS_START( resourceName )                                         ConstantBuffer resourceName : register( NRD_MERGE_TOKENS( b, NRD_CONSTANT_BUFFER_REGISTER_INDEX ) ) {
    #define NRD_CONSTANT( constantType, constantName )                                  constantType constantName;
    #define NRD_CONSTANTS_END                                                           };

    #define NRD_INPUTS_START
    #define NRD_INPUT( resourceType, resourceName, regName, bindingIndex )              resourceType resourceName : register( NRD_MERGE_TOKENS( regName, bindingIndex ) );
    #define NRD_INPUTS_END

    #define NRD_OUTPUTS_START
    #define NRD_OUTPUT( resourceType, resourceName, regName, bindingIndex )             resourceType resourceName : register( NRD_MERGE_TOKENS( regName, bindingIndex ) );
    #define NRD_OUTPUTS_END

    #define NRD_SAMPLERS_START
    #define NRD_SAMPLER( resourceType, resourceName, regName, bindingIndex )            resourceType resourceName : register( NRD_MERGE_TOKENS( regName, bindingIndex ) );
    #define NRD_SAMPLERS_END

    #define numthreads                                                                  NUM_THREADS
    #define groupshared                                                                 thread_group_memory
    #define SV_GroupId                                                                  S_GROUP_ID
    #define SV_GroupIndex                                                               S_GROUP_INDEX
    #define SV_GroupThreadId                                                            S_GROUP_THREAD_ID
    #define SV_DispatchThreadId                                                         S_DISPATCH_THREAD_ID
    #define GroupMemoryBarrierWithGroupSync                                             ThreadGroupMemoryBarrierSync
    #define GroupMemoryBarrier                                                          ThreadGroupMemoryBarrier
    #define RWTexture2D                                                                 RW_Texture2D
    #define cbuffer                                                                     ConstantBuffer
    #define SampleLevel( ... )                                                          EXPAND( GET_NTH_MACRO_4_arg( __VA_ARGS__, SampleLevel4, SampleLevel3 )( __VA_ARGS__ ) )
    #define GatherRed( ... )                                                            EXPAND( GET_NTH_MACRO_3_arg( __VA_ARGS__, GatherRed3, GatherRed2 )( __VA_ARGS__ ) )
    #define GatherGreen( ... )                                                          EXPAND( GET_NTH_MACRO_3_arg( __VA_ARGS__, GatherGreen3, GatherGreen2 )( __VA_ARGS__ ) )
    #define reversebits                                                                 ReverseBits
    #define InterlockedAdd( ... )                                                       AtomicAdd( __VA_ARGS__ )
    #define InterlockedMax( ... )                                                       AtomicMax( __VA_ARGS__ )
    #define unorm

    #ifdef EXPORT_NAME
        #define NRD_EXPORT                                                              [ CxxSymbol( EXPORT_NAME ) ]
    #else
        #define NRD_EXPORT
    #endif

    #ifndef NRD_COMPILER_PSSLC
        #define NRD_COMPILER_PSSLC
    #endif

// Unreal Engine
#elif( defined( NRD_COMPILER_UNREAL_ENGINE ) ) // TODO: is there a predefined macro in UE?

    #define NRD_CONSTANTS_START( resourceName )
    #define NRD_CONSTANT( constantType, constantName )                                  constantType constantName;
    #define NRD_CONSTANTS_END

    #define NRD_INPUTS_START
    #define NRD_INPUT( resourceType, resourceName, regName, bindingIndex )              resourceType resourceName;
    #define NRD_INPUTS_END

    #define NRD_OUTPUTS_START
    #define NRD_OUTPUT( resourceType, resourceName, regName, bindingIndex )             resourceType resourceName;
    #define NRD_OUTPUTS_END

    #define NRD_SAMPLERS_START
    #define NRD_SAMPLER( resourceType, resourceName, regName, bindingIndex )            resourceType resourceName;
    #define NRD_SAMPLERS_END

    #define NRD_EXPORT

// FXC
#else // TODO: is there a predefined macro in FXC?

    #define NRD_CONSTANTS_START( resourceName )                                         cbuffer resourceName : register( NRD_MERGE_TOKENS( b, NRD_CONSTANT_BUFFER_REGISTER_INDEX ) ) {
    #define NRD_CONSTANT( constantType, constantName )                                  constantType constantName;
    #define NRD_CONSTANTS_END                                                           };

    #define NRD_INPUTS_START
    #define NRD_INPUT( resourceType, resourceName, regName, bindingIndex )              resourceType resourceName : register( NRD_MERGE_TOKENS( regName, bindingIndex ) );
    #define NRD_INPUTS_END

    #define NRD_OUTPUTS_START
    #define NRD_OUTPUT( resourceType, resourceName, regName, bindingIndex )             resourceType resourceName : register( NRD_MERGE_TOKENS( regName, bindingIndex ) );
    #define NRD_OUTPUTS_END

    #define NRD_SAMPLERS_START
    #define NRD_SAMPLER( resourceType, resourceName, regName, bindingIndex )            resourceType resourceName : register( NRD_MERGE_TOKENS( regName, bindingIndex ) );
    #define NRD_SAMPLERS_END

    #define NRD_EXPORT

#endif

//=================================================================================================================================
// GLSL
//=================================================================================================================================

#ifdef NRD_GLSL
    #ifndef float4
        #define float4                                                                  vec4
    #endif
    #ifndef float3
        #define float3                                                                  vec3
    #endif
    #ifndef float2
        #define float2                                                                  vec2
    #endif
    #ifndef float2x2
        #define float2x2                                                                mat2x2
    #endif
    #ifndef float2x3
        #define float2x3                                                                mat2x3
    #endif
    #ifndef float3x3
        #define float3x3                                                                mat3x3
    #endif
    #ifndef rsqrt
        #define rsqrt                                                                   inversesqrt
    #endif
    #ifndef saturate
        #define saturate( x )                                                           clamp( x, 0.0, 1.0 )
    #endif
    #ifndef lerp
        #define lerp                                                                    mix
    #endif
    #ifndef mul
        #define mul( x, y )                                                             ( ( x ) * ( y ) )
    #endif
    #ifndef rcp
        #define rcp( x )                                                                ( 1.0 / ( x ) )
    #endif
#endif

//=================================================================================================================================
// PRIVATE
//=================================================================================================================================

#ifdef NRD_INTERNAL
    // Explicitly set matrix layout for shader compilation outside of NRD environment
    #pragma pack_matrix( column_major )
#endif

#if( !defined( NRD_NORMAL_ENCODING ) || !defined( NRD_ROUGHNESS_ENCODING ) )
    #ifdef NRD_INTERNAL
        #error "For NRD project compilation, encoding variants must be set using Cmake parameters."
    #else
        #error "Include 'NRDEncoding.hlsli' file beforehand to get a match with the settings NRD has been compiled with. Or define encoding variants using Cmake parameters."
    #endif
#endif

// Normal encoding variants ( match NormalEncoding )
#define NRD_NORMAL_ENCODING_RGBA8_UNORM                                                 0
#define NRD_NORMAL_ENCODING_RGBA8_SNORM                                                 1
#define NRD_NORMAL_ENCODING_R10G10B10A2_UNORM                                           2 // supports material ID bits
#define NRD_NORMAL_ENCODING_RGBA16_UNORM                                                3
#define NRD_NORMAL_ENCODING_RGBA16_SNORM                                                4 // also can be used with FP formats

// Roughness encoding variants ( match RoughnessEncoding )
#define NRD_ROUGHNESS_ENCODING_SQ_LINEAR                                                0 // linearRoughness * linearRoughness
#define NRD_ROUGHNESS_ENCODING_LINEAR                                                   1 // linearRoughness
#define NRD_ROUGHNESS_ENCODING_SQRT_LINEAR                                              2 // sqrt( linearRoughness )

#define NRD_FP16_MAX                                                                    65504.0
#define NRD_PI                                                                          3.14159265358979323846
#define NRD_EPS                                                                         1e-6
#define NRD_REJITTER_VIEWZ_THRESHOLD                                                    0.01 // normalized %
#define NRD_ROUGHNESS_EPS                                                               sqrt( sqrt( NRD_EPS ) ) // "m2" fitting in FP32 to "linear roughness"
#define NRD_INF                                                                         1e6
#define NRD_MATERIAL_FACTOR_MIN_SCALE                                                   0.02 // very small scales can lead to instabilities
#define NRD_ROUGHNESS_FACTOR_MIN_SCALE                                                  0.1 // very small scales can lead to instabilities and bias

// Misc
float3 _NRD_SafeNormalize( float3 v )
{
    return v * rsqrt( dot( v, v ) + 1e-9 );
}

// Oct packing
float2 _NRD_EncodeUnitVector( float3 v, const bool bSigned )
{
    v /= dot( abs( v ), float3( 1.0, 1.0, 1.0 ) );

    float2 octWrap = ( 1.0 - abs( v.yx ) ) * ( step( 0.0, v.xy ) * 2.0 - 1.0 );
    v.xy = v.z >= 0.0 ? v.xy : octWrap;

    return bSigned ? v.xy : v.xy * 0.5 + 0.5;
}

float3 _NRD_DecodeUnitVector( float2 p, const bool bSigned, const bool bNormalize )
{
    p = bSigned ? p : ( p * 2.0 - 1.0 );

    // https://twitter.com/Stubbesaurus/status/937994790553227264
    float3 n = float3( p.xy, 1.0 - abs( p.x ) - abs( p.y ) );
    float t = saturate( -n.z );
    n.xy -= t * ( step( 0.0, n.xy ) * 2.0 - 1.0 );

    return bNormalize ? normalize( n ) : n;
}

// Color space
float _NRD_Luminance( float3 linearColor )
{
    // IMPORTANT: must be in sync with ML_LUMINANCE_DEFAULT
    return dot( linearColor, float3( 0.2126, 0.7152, 0.0722 ) );
}

float3 _NRD_LinearToYCoCg( float3 color )
{
    float Y = dot( color, float3( 0.25, 0.5, 0.25 ) );
    float Co = dot( color, float3( 0.5, 0.0, -0.5 ) );
    float Cg = dot( color, float3( -0.25, 0.5, -0.25 ) );

    return float3( Y, Co, Cg );
}

float3 _NRD_YCoCgToLinear( float3 color )
{
    float t = color.x - color.z;

    float3 r;
    r.y = color.x + color.z;
    r.x = t + color.y;
    r.z = t - color.y;

    return max( r, 0.0 );
}

float3 _NRD_YCoCgToLinear_Corrected( float Y, float Y0, float2 CoCg )
{
    Y = max( Y, 0.0 );
    CoCg *= ( Y + NRD_EPS ) / ( Y0 + NRD_EPS );

    return _NRD_YCoCgToLinear( float3( Y, CoCg ) );
}

// GGX dominant direction
float _NRD_GetSpecularDominantFactor( float NoV, float roughness )
{
    float a = 0.298475 * log( 39.4115 - 39.0029 * roughness );
    float dominantFactor = pow( saturate( 1.0 - NoV ), 10.8649 ) * ( 1.0 - a ) + a;

    return saturate( dominantFactor );
}

float3 _NRD_GetSpecularDominantDirection( float3 N, float3 V, float dominantFactor )
{
    float3 R = reflect( -V, N );
    float3 D = lerp( N, R, dominantFactor );

    return normalize( D );
}

float _NRD_GetSpecMagicCurve( float roughness )
{
    return 1.0 - exp2( -30.0 * roughness * roughness );
}

// BRDF
float _NRD_Pow5( float x )
{
    return pow( saturate( 1.0 - x ), 5.0 );
}

float _NRD_FresnelTerm( float Rf0, float VoNH )
{
    return Rf0 + ( 1.0 - Rf0 ) * _NRD_Pow5( VoNH );
}

float _NRD_DistributionTerm( float roughness, float NoH )
{
    float m = roughness * roughness;
    float m2 = m * m;

    float t = ( NoH * m2 - NoH ) * NoH + 1.0;
    float a = m / t;
    float d = a * a;

    return d / NRD_PI;
}

float _NRD_GeometryTerm( float roughness, float NoL, float NoV )
{
    float m = roughness * roughness;
    float m2 = m * m;

    float a = NoL + sqrt( saturate( ( NoL - m2 * NoL ) * NoL + m2 ) );
    float b = NoV + sqrt( saturate( ( NoV - m2 * NoV ) * NoV + m2 ) );

    return 1.0 / max( a * b, NRD_EPS );
}

float _NRD_DiffuseTerm( float roughness, float NoL, float NoV, float VoH )
{
    float m = roughness * roughness;

    float f = 2.0 * VoH * VoH * m - 0.5;
    float FdV = f * _NRD_Pow5( NoV ) + 1.0;
    float FdL = f * _NRD_Pow5( NoL ) + 1.0;
    float d = FdV * FdL;

    return d / NRD_PI;
}

float2 _NRD_ComputeBrdfs( float3 Ld, float3 Ls, float3 N, float3 V, float Rf0, float roughness )
{
    float2 result;
    float NoV = abs( dot( N, V ) );

    // Diffuse
    {
        float3 H = normalize( Ld + V );

        float NoL = saturate( dot( N, Ld ) );
        float VoH = saturate( dot( V, H ) );

        float F = _NRD_FresnelTerm( Rf0, VoH );
        float Kdiff = _NRD_DiffuseTerm( roughness, NoL, NoV, VoH );

        result.x = ( 1.0 - F ) * Kdiff * NoL;
    }

    // Specular
    {
        float3 H = normalize( Ls + V );
        H = normalize( lerp( N, H, roughness ) ); // Fixed H // TODO: roughness => smc?

        float NoL = saturate( dot( N, Ls ) );
        float NoH = saturate( dot( N, H ) );
        float VoH = saturate( dot( V, H ) );

        float F = _NRD_FresnelTerm( Rf0, VoH );
        float D = _NRD_DistributionTerm( roughness, NoH );
        float G = _NRD_GeometryTerm( roughness, NoL, NoV );

        result.y = F * D * G * NoL;
    }

    return result;
}

float3 _NRD_EnvironmentTerm_Rtg( float3 Rf0, float NoV, float roughness )
{
    // "Ray Tracing Gems", Chapter 32, Equation 4 - the approximation assumes GGX VNDF and Schlick's approximation
    float m = saturate( roughness * roughness );

    float4 X;
    X.x = 1.0;
    X.y = NoV;
    X.z = NoV * NoV;
    X.w = NoV * X.z;

    float4 Y;
    Y.x = 1.0;
    Y.y = m;
    Y.z = m * m;
    Y.w = m * Y.z;

    const float2x2 M1 = float2x2( 0.99044, -1.28514, 1.29678, -0.755907 );
    const float3x3 M2 = float3x3( 1.0, 2.92338, 59.4188, 20.3225, -27.0302, 222.592, 121.563, 626.13, 316.627 );

    const float2x2 M3 = float2x2( 0.0365463, 3.32707, 9.0632, -9.04756 );
    const float3x3 M4 = float3x3( 1.0, 3.59685, -1.36772, 9.04401, -16.3174, 9.22949, 5.56589, 19.7886, -20.2123 );

    float bias = dot( mul( M1, X.xy ), Y.xy ) * rcp( max( dot( mul( M2, X.xyw ), Y.xyw ), NRD_EPS ) );
    float scale = dot( mul( M3, X.xy ), Y.xy ) * rcp( max( dot( mul( M4, X.xzw ), Y.xyw ), NRD_EPS ) );

    return saturate( Rf0 * scale + bias );
}

// Hit distance normalization
float _REBLUR_GetHitDistanceNormalization( float viewZ, float4 hitDistParams, float roughness )
{
    return ( hitDistParams.x + abs( viewZ ) * hitDistParams.y ) * lerp( 1.0, hitDistParams.z, saturate( exp2( hitDistParams.w * roughness * roughness ) ) );
}

// Is valid?
bool _NRD_IsInvalid( float3 x )
{
    return any( isnan( x ) ) || any( isinf( x ) );
}

bool _NRD_IsInvalid( float x )
{
    return isnan( x ) || isinf( x );
}

//==============================================================================================================================================
// SPHERICAL HARMONICS: https://media.contentapi.ea.com/content/dam/eacom/frostbite/files/gdc2018-precomputedgiobalilluminationinfrostbite.pdf
// SPHERICAL GAUSSIAN: https://therealmjp.github.io/posts/sg-series-part-1-a-brief-and-incomplete-history-of-baked-lighting-representations/
//==============================================================================================================================================

struct NRD_SG
{
    float c0;
    float2 chroma;
    float normHitDist;

    float3 c1;
    float sharpness;
};

NRD_SG _NRD_SG_Create( float3 radiance, float3 direction, float normHitDist )
{
    float3 YCoCg = _NRD_LinearToYCoCg( radiance );

    NRD_SG sg;
    sg.c0 = YCoCg.x;
    sg.chroma = YCoCg.yz;
    sg.c1 = direction * YCoCg.x;
    sg.normHitDist = normHitDist;
    sg.sharpness = 0.0; // TODO: currently not used

    return sg;
}

float3 _NRD_SG_ExtractDirection( NRD_SG sg )
{
    return sg.c1 / max( length( sg.c1 ), NRD_EPS );
}

float _NRD_SG_IntegralApprox( NRD_SG sg )
{
    return 2.0 * NRD_PI * ( sg.c0 / sg.sharpness );
}

float _NRD_SG_Integral( NRD_SG sg )
{
    float expTerm = 1.0 - exp( -2.0 * sg.sharpness );

    return _NRD_SG_IntegralApprox( sg ) * expTerm;
}

float _NRD_SG_InnerProduct( NRD_SG a, NRD_SG b )
{
    // Integral of the product of two SGs
    float d = length( a.sharpness * _NRD_SG_ExtractDirection( a ) + b.sharpness * _NRD_SG_ExtractDirection( b ) );
    float c = exp( d - a.sharpness - b.sharpness );
    c *= 1.0 - exp( -2.0 * d );
    c /= max( d, NRD_EPS );

    // Original version is without "saturate" ( needed to avoid rare fireflies in our case, energy is already preserved )
    return NRD_PI * saturate( 2.0 * c * a.c0 ) * b.c0;
}

//=================================================================================================================================
// FRONT-END - GENERAL
//=================================================================================================================================

// This function is used in all denoisers to decode normal, roughness and optional materialID
// IN_NORMAL_ROUGHNESS => X
float4 NRD_FrontEnd_UnpackNormalAndRoughness( float4 p, out float materialID )
{
    float4 r;
    #if( NRD_NORMAL_ENCODING == NRD_NORMAL_ENCODING_R10G10B10A2_UNORM )
        r.xyz = _NRD_DecodeUnitVector( p.xy, false, false );
        r.w = p.z;

        materialID = p.w * 3.0;
    #else
        #if( NRD_NORMAL_ENCODING == NRD_NORMAL_ENCODING_RGBA8_UNORM || NRD_NORMAL_ENCODING == NRD_NORMAL_ENCODING_RGBA16_UNORM )
            p.xyz = p.xyz * 2.0 - 1.0;
        #endif

        r.xyz = p.xyz;
        r.w = p.w;

        materialID = 0.0;
    #endif

    r.xyz = _NRD_SafeNormalize( r.xyz );

    #if( NRD_ROUGHNESS_ENCODING == NRD_ROUGHNESS_ENCODING_SQRT_LINEAR )
        r.w *= r.w;
    #elif( NRD_ROUGHNESS_ENCODING == NRD_ROUGHNESS_ENCODING_SQ_LINEAR )
        r.w = sqrt( saturate( r.w ) );
    #endif

    return r;
}

// IN_NORMAL_ROUGHNESS => X
float4 NRD_FrontEnd_UnpackNormalAndRoughness( float4 p )
{
    float unused;

    return NRD_FrontEnd_UnpackNormalAndRoughness( p, unused );
}

// Not used in NRD
// X => IN_NORMAL_ROUGHNESS
float4 NRD_FrontEnd_PackNormalAndRoughness( float3 N, float roughness, float materialID )
{
    float4 p;

    #if( NRD_ROUGHNESS_ENCODING == NRD_ROUGHNESS_ENCODING_SQRT_LINEAR )
        roughness = sqrt( saturate( roughness ) );
    #elif( NRD_ROUGHNESS_ENCODING == NRD_ROUGHNESS_ENCODING_SQ_LINEAR )
        roughness *= roughness;
    #endif

    #if( NRD_NORMAL_ENCODING == NRD_NORMAL_ENCODING_R10G10B10A2_UNORM )
        p.xy = _NRD_EncodeUnitVector( N, false );
        p.z = roughness;
        p.w = saturate( materialID / 3.0 );
    #else
        // Best fit ( optional )
        N /= max( abs( N.x ), max( abs( N.y ), abs( N.z ) ) );

        #if( NRD_NORMAL_ENCODING == NRD_NORMAL_ENCODING_RGBA8_UNORM || NRD_NORMAL_ENCODING == NRD_NORMAL_ENCODING_RGBA16_UNORM )
            N = N * 0.5 + 0.5;
        #endif

        p.xyz = N;
        p.w = roughness;
    #endif

    return p;
}

// Material de-modulation
//   Front-end usage ( before NRD, convert irradiance into radiance ):
//      diffIrradiance /= diffFactor
//      specIrradiance /= specFactor
//   Back-end usage ( after NRD, convert radiance back into irradiance ):
//      diffRadiance *= diffFactor
//      specRadiance *= specFactor
void NRD_MaterialFactors( float3 N, float3 V, float3 albedo, float3 Rf0, float roughness, out float3 diffFactor, out float3 specFactor )
{
    float NoV = abs( dot( N, V ) );
    float3 Fenv = _NRD_EnvironmentTerm_Rtg( Rf0, NoV, roughness );

    diffFactor = ( 1.0 - Fenv ) * albedo;
    diffFactor = lerp( NRD_MATERIAL_FACTOR_MIN_SCALE.xxx, float3( 1.0, 1.0, 1.0 ), diffFactor );

    specFactor = Fenv;
    specFactor *= lerp( NRD_ROUGHNESS_FACTOR_MIN_SCALE.xxx, float3( 1.0, 1.0, 1.0 ), roughness ); // don't be greedy, it's a biased solution
    specFactor = lerp( NRD_MATERIAL_FACTOR_MIN_SCALE.xxx, float3( 1.0, 1.0, 1.0 ), specFactor );
}

//=================================================================================================================================
// FRONT-END - SPECULAR HIT DISTANCE AVERAGING ( in case of rpp > 1 )
//=================================================================================================================================

float NRD_FrontEnd_SpecHitDistAveraging_Begin( )
{
    return NRD_INF;
}

float NRD_FrontEnd_TrimHitDistance( float hitDist, float threshold ) // optional
{
    // Sampling can produce rays pointing inside the surface, leading to "hitDist = 0". But due to ray offsetting actual "hitDist" can be a very
    // small value in this case. Since NRD has been designed to handle "hitDist = 0" case, accidentally small "hitDist" values better trim to 0

    return hitDist < threshold ? 0.0 : hitDist;
}

void NRD_FrontEnd_SpecHitDistAveraging_Add( inout float accumulatedSpecHitDist, float hitDist )
{
    // TODO: for high roughness it can be blended to average
    accumulatedSpecHitDist = min( accumulatedSpecHitDist, hitDist == 0.0 ? NRD_INF : hitDist );
}

void NRD_FrontEnd_SpecHitDistAveraging_End( inout float accumulatedSpecHitDist )
{
    accumulatedSpecHitDist = accumulatedSpecHitDist == NRD_INF ? 0.0 : accumulatedSpecHitDist;
}

//=================================================================================================================================
// FRONT-END - REBLUR
//=================================================================================================================================

// This function returns AO / SO which REBLUR can decode back to "hit distance" internally
float REBLUR_FrontEnd_GetNormHitDist( float hitDist, float viewZ, float4 hitDistParams, float roughness )
{
    float f = _REBLUR_GetHitDistanceNormalization( viewZ, hitDistParams, roughness );

    return saturate( hitDist / f );
}

// X => IN_DIFF_RADIANCE_HITDIST
// X => IN_SPEC_RADIANCE_HITDIST
// normHitDist must be packed by "REBLUR_FrontEnd_GetNormHitDist"
float4 REBLUR_FrontEnd_PackRadianceAndNormHitDist( float3 radiance, float normHitDist, bool sanitize )
{
    if( sanitize )
    {
        radiance = _NRD_IsInvalid( radiance ) ? float3( 0, 0, 0 ) : clamp( radiance, 0, NRD_FP16_MAX );
        normHitDist = _NRD_IsInvalid( normHitDist ) ? 0 : saturate( normHitDist );
    }

    radiance = _NRD_LinearToYCoCg( radiance );

    return float4( radiance, normHitDist );
}

// X => IN_DIFF_SH0 and IN_DIFF_SH1
// X => IN_SPEC_SH0 and IN_SPEC_SH1
// normHitDist must be packed by "REBLUR_FrontEnd_GetNormHitDist"
float4 REBLUR_FrontEnd_PackSh( float3 radiance, float normHitDist, float3 direction, out float4 out1, bool sanitize )
{
    if( sanitize )
    {
        radiance = _NRD_IsInvalid( radiance ) ? float3( 0, 0, 0 ) : clamp( radiance, 0, NRD_FP16_MAX );
        normHitDist = _NRD_IsInvalid( normHitDist ) ? 0 : saturate( normHitDist );
        direction = _NRD_IsInvalid( direction ) ? float3( 0, 0, 0 ) : clamp( direction, -1.0, 1.0 );
    }

    NRD_SG sg = _NRD_SG_Create( radiance, direction, normHitDist );

    // IN_DIFF_SH0 / IN_SPEC_SH0
    float4 out0 = float4( sg.c0, sg.chroma, sg.normHitDist );

    // IN_DIFF_SH1 / IN_SPEC_SH1
    out1 = float4( sg.c1, sg.sharpness );

    return out0;
}

// X => IN_DIFF_DIRECTION_HITDIST
// normHitDist must be packed by "REBLUR_FrontEnd_GetNormHitDist"
float4 REBLUR_FrontEnd_PackDirectionalOcclusion( float3 direction, float normHitDist, bool sanitize )
{
    if( sanitize )
    {
        direction = _NRD_IsInvalid( direction ) ? float3( 0, 0, 0 ) : clamp( direction, -1.0, 1.0 );
        normHitDist = _NRD_IsInvalid( normHitDist ) ? 0 : saturate( normHitDist );
    }

    NRD_SG sg = _NRD_SG_Create( normHitDist.xxx, direction, normHitDist );

    return float4( sg.c1, sg.c0 );
}

//=================================================================================================================================
// FRONT-END - RELAX
//=================================================================================================================================

// X => IN_DIFF_RADIANCE_HITDIST
// X => IN_SPEC_RADIANCE_HITDIST
float4 RELAX_FrontEnd_PackRadianceAndHitDist( float3 radiance, float hitDist, bool sanitize )
{
    if( sanitize )
    {
        radiance = _NRD_IsInvalid( radiance ) ? float3( 0, 0, 0 ) : clamp( radiance, 0, NRD_FP16_MAX );
        hitDist = _NRD_IsInvalid( hitDist ) ? 0 : clamp( hitDist, 0, NRD_FP16_MAX );
    }

    return float4( radiance, hitDist );
}

// X => IN_DIFF_SH0 and IN_DIFF_SH1
// X => IN_SPEC_SH0 and IN_SPEC_SH1
float4 RELAX_FrontEnd_PackSh( float3 radiance, float hitDist, float3 direction, out float4 out1, bool sanitize )
{
    if( sanitize )
    {
        radiance = _NRD_IsInvalid( radiance ) ? float3( 0, 0, 0 ) : clamp( radiance, 0, NRD_FP16_MAX );
        hitDist = _NRD_IsInvalid( hitDist ) ? 0 : clamp( hitDist, 0, NRD_FP16_MAX );
        direction = _NRD_IsInvalid( direction ) ? float3( 0, 0, 0 ) : clamp( direction, -1.0, 1.0 );
    }

    // IN_DIFF_SH0 / IN_SPEC_SH0
    float4 out0 = float4( radiance, hitDist );

    // IN_DIFF_SH1 / IN_SPEC_SH1
    out1 = float4( direction * _NRD_Luminance( radiance ), 0 );

    return out0;
}

//=================================================================================================================================
// FRONT-END - SIGMA
//=================================================================================================================================

// SIGMA single light

// Infinite ( directional ) light source
// X => IN_PENUMBRA
float SIGMA_FrontEnd_PackPenumbra( float distanceToOccluder, float tanOfLightAngularRadius )
{
    float penumbraSize = distanceToOccluder * tanOfLightAngularRadius;
    float penumbraRadius = penumbraSize * 0.5;

    return distanceToOccluder >= NRD_FP16_MAX ? NRD_FP16_MAX : min( penumbraRadius, 32768.0 );
}

// Local light source
// X => IN_PENUMBRA
// "lightSize" must be an acceptable projection to the plane perpendicular to the light direction
float SIGMA_FrontEnd_PackPenumbra( float distanceToOccluder, float distanceToLight, float lightSize )
{
    float penumbraSize = lightSize * distanceToOccluder / max( distanceToLight - distanceToOccluder, NRD_EPS );
    float penumbraRadius = penumbraSize * 0.5;

    return distanceToOccluder >= NRD_FP16_MAX ? NRD_FP16_MAX : min( penumbraRadius, 32768.0 );
}

// X => IN_TRANSLUCENCY
float4 SIGMA_FrontEnd_PackTranslucency( float distanceToOccluder, float3 translucency )
{
    float4 r;
    r.x = float( distanceToOccluder >= NRD_FP16_MAX );
    r.yzw = saturate( translucency );

    return r;
}

//=================================================================================================================================
// BACK-END - REBLUR
//=================================================================================================================================

// OUT_DIFF_RADIANCE_HITDIST => X
// OUT_SPEC_RADIANCE_HITDIST => X
float4 REBLUR_BackEnd_UnpackRadianceAndNormHitDist( float4 data )
{
    data.xyz = _NRD_YCoCgToLinear( data.xyz );

    return data;
}

// OUT_DIFF_SH0 and OUT_DIFF_SH1 => X
// OUT_SPEC_SH0 and OUT_SPEC_SH1 => X
NRD_SG REBLUR_BackEnd_UnpackSh( float4 sh0, float4 sh1 )
{
    NRD_SG sg;
    sg.c0 = sh0.x;
    sg.chroma = sh0.yz;
    sg.normHitDist = sh0.w;
    sg.c1 = sh1.xyz;
    sg.sharpness = sh1.w;

    return sg;
}

// OUT_DIFF_DIRECTION_HITDIST => X
NRD_SG REBLUR_BackEnd_UnpackDirectionalOcclusion( float4 data )
{
    NRD_SG sg;
    sg.c0 = data.w;
    sg.chroma = float2( 0, 0 );
    sg.normHitDist = data.w;
    sg.c1 = data.xyz;
    sg.sharpness = 0.0;

    return sg;
}

//=================================================================================================================================
// BACK-END - RELAX
//=================================================================================================================================

// OUT_DIFF_RADIANCE_HITDIST => X
// OUT_SPEC_RADIANCE_HITDIST => X
float4 RELAX_BackEnd_UnpackRadiance( float4 color )
{
    return color;
}

// OUT_DIFF_SH0 and OUT_DIFF_SH1 => X
// OUT_SPEC_SH0 and OUT_SPEC_SH1 => X
NRD_SG RELAX_BackEnd_UnpackSh( float4 sh0, float4 sh1 )
{
    NRD_SG sg;
    sg.c0 = sh0.x;
    sg.chroma = sh0.yz;
    sg.normHitDist = sh0.w;
    sg.c1 = sh1.xyz;
    sg.sharpness = sh1.w;

    return sg;
}

//=================================================================================================================================
// BACK-END - SIGMA
//=================================================================================================================================

// OUT_SHADOW_TRANSLUCENCY => X
//   SIGMA_SHADOW / SIGMA_SHADOW_TRANSLUCENCY:
//      float shadow = SIGMA_BackEnd_UnpackShadow( OUT_SHADOW_TRANSLUCENCY ).x;
//   SIGMA_SHADOW_TRANSLUCENCY:
//      float3 translucentShadow = SIGMA_BackEnd_UnpackShadow( OUT_SHADOW_TRANSLUCENCY ).yzw;
#define SIGMA_BackEnd_UnpackShadow( shadow ) ( shadow * shadow )

//=================================================================================================================================
// BACK-END - HIGH QUALITY RESOLVE
//=================================================================================================================================

float3 NRD_SG_ExtractColor( NRD_SG sg )
{
    return _NRD_YCoCgToLinear( float3( sg.c0, sg.chroma ) );
}

float3 NRD_SG_ExtractDirection( NRD_SG sg )
{
    return _NRD_SG_ExtractDirection( sg );
}

float NRD_SG_ExtractRoughnessAA( NRD_SG sg )
{
    return sg.sharpness;
}

void NRD_SG_Rotate( inout NRD_SG sg, float3x3 rotation )
{
    sg.c1 = mul( rotation, sg.c1 );
}

float3 NRD_SG_ResolveDiffuse( NRD_SG sg, float3 N )
{
    // https://therealmjp.github.io/posts/sg-series-part-3-diffuse-lighting-from-an-sg-light-source/

#if 1
    // Numerical integration of the resulting irradiance from an SG diffuse light source ( with sharpness of 4.0 )
    sg.sharpness = 4.0;

    float c0 = 0.36;
    float c1 = 1.0 / ( 4.0 * c0 );

    float e = exp( -sg.sharpness );
    float e2 = e * e;
    float r = 1.0 / sg.sharpness;

    float scale = 1.0 + 2.0 * e2 - r;
    float bias = ( e - e2 ) * r - e2;

    float NoL = dot( N, _NRD_SG_ExtractDirection( sg ) );
    float x = sqrt( saturate( 1.0 - scale ) );
    float x0 = c0 * NoL;
    float x1 = c1 * x;
    float n = x0 + x1;

    float y = saturate( NoL );
    if( abs( x0 ) <= x1 )
        y = n * n / x;

    float Y = scale * y + bias;
    Y *= _NRD_SG_IntegralApprox( sg );
#else
    // "SG light" sharpness
    sg.sharpness = 2.0; // TODO: another sharpness = another normalization needed...

    // Approximate NDF
    NRD_SG ndf;
    ndf.c0 = 1.0;
    ndf.c1 = N;
    ndf.sharpness = 2.0;
    ndf.chroma = float2( 0, 0 );
    ndf.normHitDist = 0;

    // Non-magic scale
    ndf.c0 *= 0.75;

    // Multiply two SGs and integrate the result
    float Y = _NRD_SG_InnerProduct( ndf, sg );
#endif

    return _NRD_YCoCgToLinear_Corrected( Y, sg.c0, sg.chroma );
}

float3 NRD_SG_ResolveSpecular( NRD_SG sg, float3 N, float3 V, float roughness )
{
    // https://therealmjp.github.io/posts/sg-series-part-4-specular-lighting-from-an-sg-light-source/

    // Clamp roughness to avoid numerical imprecision
    roughness = max( roughness, NRD_ROUGHNESS_EPS );

    // "SG light" sharpness
    sg.sharpness = 2.0; // TODO: another sharpness = another normalization needed...

    // Approximate NDF
    float3 H = normalize( _NRD_SG_ExtractDirection( sg ) + V );
    H = normalize( lerp( N, H, roughness ) ); // Fixed H // TODO: roughness => smc?

    float m = roughness * roughness;
    float m2 = m * m;

    NRD_SG ndf;
    ndf.c0 = 1.0 / ( NRD_PI * m2 );
    ndf.c1 = H;
    ndf.sharpness = 2.0 / max( m2, NRD_EPS );
    ndf.chroma = float2( 0, 0 );
    ndf.normHitDist = 0;

    // Non-magic scale
    ndf.c0 *= lerp( 1.0, 0.75 * 2.0 * NRD_PI, m2 );

    // Warp NDF
    NRD_SG ndfWarped;
    ndfWarped.c0 = ndf.c0;
    ndfWarped.c1 = reflect( -V, ndf.c1 );
    ndfWarped.sharpness = ndf.sharpness / max( 4.0 * abs( dot( ndf.c1, V ) ), NRD_EPS );
    ndfWarped.chroma = float2( 0, 0 );
    ndfWarped.normHitDist = 0;

    // Cosine term & visibility term evaluated at the center of the warped BRDF lobe
    float NoV = abs( dot( N, V ) );
    float NoL = saturate( dot( N, ndfWarped.c1 ) );

    ndfWarped.c0 *= NoL;
    ndfWarped.c0 *= _NRD_GeometryTerm( roughness, NoL, NoV );

    // Multiply two SGs and integrate the result
    float Y = _NRD_SG_InnerProduct( ndfWarped, sg );

    return _NRD_YCoCgToLinear_Corrected( Y, sg.c0, sg.chroma );
}

/*
Offsets:
e = int2( 1,  0 )
w = int2(-1,  0 )
n = int2( 0,  1 )
s = int2( 0, -1 )
*/
float2 NRD_SG_ReJitter(
    NRD_SG diffSg, NRD_SG specSg,
    float3 Rf0, float3 V, float roughness,
    float Z, float Ze, float Zw, float Zn, float Zs,
    float3 N, float3 Ne, float3 Nw, float3 Nn, float3 Ns
)
{
    // Clamp roughness to avoid numerical imprecision
    roughness = max( roughness, NRD_ROUGHNESS_EPS );

    // Extract Rf0 and diff & spec dominant light directions
    float rf0 = _NRD_Luminance( Rf0 );
    float3 Ld = _NRD_SG_ExtractDirection( diffSg );
    float3 Ls = _NRD_SG_ExtractDirection( specSg );

    // Ls is accumulated, ideally it shouldn't for low roughness, but it's not a bug on the NRD side.
    // The hack below is not needed if stochastic per-pixel jittering is used. Otherwise, the best approach
    // is to resolve against jittered "view" vector. Despite that this approach looks very biased, it doesn't
    // changes the energy of the output signal
    float smc = _NRD_GetSpecMagicCurve( roughness );
    Ls = normalize( lerp( V, Ls, smc ) );

    // BRDF at center
    float2 brdfCenter = _NRD_ComputeBrdfs( Ld, Ls, N, V, rf0, roughness );

    // BRDFs at neighbors
    float2 brdfAverage = _NRD_ComputeBrdfs( Ld, Ls, Ne, V, rf0, roughness );
    brdfAverage += _NRD_ComputeBrdfs( Ld, Ls, Nn, V, rf0, roughness );
    brdfAverage += _NRD_ComputeBrdfs( Ld, Ls, Nw, V, rf0, roughness );
    brdfAverage += _NRD_ComputeBrdfs( Ld, Ls, Ns, V, rf0, roughness );

    // Viewing angle corrected Z threshold
    float NoV = abs( dot( N, V ) );
    float zThreshold = NRD_REJITTER_VIEWZ_THRESHOLD * abs( Z ) / ( NoV * 0.95 + 0.05 );

    // Sum of all weights
    // Exploit: out of screen fetches return "0", which auto-disables resolve on screen edges
    uint sum = abs( Ze - Z ) < zThreshold && dot( Ne, N ) > 0.0 ? 1 : 0;
    sum += abs( Zn - Z ) < zThreshold && dot( Nn, N ) > 0.0 ? 1 : 0;
    sum += abs( Zw - Z ) < zThreshold && dot( Nw, N ) > 0.0 ? 1 : 0;
    sum += abs( Zs - Z ) < zThreshold && dot( Ns, N ) > 0.0 ? 1 : 0;

    // Jacobian
    float2 f = ( brdfCenter * 4.0 + NRD_EPS ) / ( brdfAverage + NRD_EPS );

    // Use re-jitter only if all samples are valid to minimize ringing
    return sum != 4 ? float2( 1, 1 ) : clamp( f, 1.0 / NRD_PI, NRD_PI );
}

//=================================================================================================================================
// SPHERICAL HARMONICS ( MEDIUM QUALITY )
//=================================================================================================================================

float3 NRD_SH_ResolveDiffuse( NRD_SG sh, float3 N )
{
    float Y = dot( N, sh.c1 ) + 0.5 * sh.c0;

    return _NRD_YCoCgToLinear_Corrected( Y, sh.c0, sh.chroma );
}

float3 NRD_SH_ResolveSpecular( NRD_SG sh, float3 N, float3 V, float roughness )
{
    float NoV = abs( dot( N, V ) );
    float f = _NRD_GetSpecularDominantFactor( NoV, roughness );
    float3 D = _NRD_GetSpecularDominantDirection( N, V, f );

    float Y = dot( D, sh.c1 ) + 0.5 * sh.c0;

    return _NRD_YCoCgToLinear_Corrected( Y, sh.c0, sh.chroma );
}

//=================================================================================================================================
// MISC
//=================================================================================================================================

// Needs to be used to avoid summing up NAN/INF values in many rays per pixel scenarios
bool NRD_IsValidRadiance( float3 radiance )
{
    return !_NRD_IsInvalid( radiance );
}

// Scales normalized hit distance back to real length
float REBLUR_GetHitDist( float normHitDist, float viewZ, float4 hitDistParams, float roughness )
{
    float scale = _REBLUR_GetHitDistanceNormalization( viewZ, hitDistParams, roughness );

    return normHitDist * scale;
}

// Normalized strand thickness factor in range [0; 1]
// 0 - thickness in pixels is good enough to guarantee stable multi-pixel projection
// 1 - thickness in pixels is very thick, transforming the geomtery into a "pixel soup"
// pixelSize = size of a pixel in world units for provided "viewZ"
// pixelSize = gUnproject * ( isOrtho ? 1.0 : abs( viewZ ) )
float NRD_GetNormalizedStrandThickness( float strandThickness, float pixelSize )
{
    return pixelSize / ( pixelSize + strandThickness );
}

#endif // __cplusplus

#endif // NRD_INCLUDED
