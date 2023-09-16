#ifndef NOISE_VALUE_3D
#define NOISE_VALUE_3D 1
//
//  Wombat
//  An efficient texture-free GLSL procedural noise library
//  Source: https://github.com/BrianSharpe/Wombat
//  Derived from: https://github.com/BrianSharpe/GPU-Noise-Lib
//  Value Noise 3D
//  Return value range of 0.0->1.0
float noise_Value3D( float3 P )
{
    //  https://github.com/BrianSharpe/Wombat/blob/master/Value3D.glsl

    // establish our grid cell and unit position
    float3 Pi = floor(P);
    float3 Pf = P - Pi;
    float3 Pf_min1 = Pf - 1.0;

    // clamp the domain
    Pi.xyz = Pi.xyz - floor(Pi.xyz * ( 1.0 / 69.0 )) * 69.0;
    float3 Pi_inc1 = step( Pi, float( 69.0 - 1.5 ).xxx ) * ( Pi + 1.0 );

    // calculate the hash
    float4 Pt = float4( Pi.xy, Pi_inc1.xy ) + float2( 50.0, 161.0 ).xyxy;
    Pt *= Pt;
    Pt = Pt.xzxz * Pt.yyww;
    float2 hash_mod = float2( 1.0 / ( 635.298681 + float2( Pi.z, Pi_inc1.z ) * 48.500388 ) );
    float4 hash_lowz = frac( Pt * hash_mod.xxxx );
    float4 hash_highz = frac( Pt * hash_mod.yyyy );

    //	blend the results and return
    float3 blend = Pf * Pf * Pf * (Pf * (Pf * 6.0 - 15.0) + 10.0);
    float4 res0 = lerp( hash_lowz, hash_highz, blend.z );
    float4 blend2 = float4( blend.xy, float2( 1.0 - blend.xy ) );
    return dot( res0, blend2.zxzx * blend2.wwyy );
}
#endif
