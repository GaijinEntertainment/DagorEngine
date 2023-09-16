#ifndef NOISE_VALUE_2D
#define NOISE_VALUE_2D 1
//
//  Wombat
//  An efficient texture-free GLSL procedural noise library
//  Source: https://github.com/BrianSharpe/Wombat
//  Derived from: https://github.com/BrianSharpe/GPU-Noise-Lib
//
//  Value Noise 2D
//  Return value range of 0.0->1.0
//
float noise_Value2D( float2 P )
{
    //  https://github.com/BrianSharpe/Wombat/blob/master/Value2D.glsl

    //	establish our grid cell and unit position
    float2 Pi = floor(P);
    float2 Pf = P - Pi;

    //	calculate the hash.
    float4 Pt = float4( Pi.xy, Pi.xy + 1.0 );
    Pt = Pt - floor(Pt * ( 1.0 / 71.0 )) * 71.0;
    Pt += float2( 26.0, 161.0 ).xyxy;
    Pt *= Pt;
    Pt = Pt.xzxz * Pt.yyww;
    float4 hash = frac( Pt * ( 1.0 / 951.135664 ) );

    //	blend the results and return
    float2 blend = Pf * Pf * Pf * (Pf * (Pf * 6.0 - 15.0) + 10.0);
    float4 blend2 = float4( blend, float2( 1.0 - blend ) );
    return dot( hash, blend2.zxzx * blend2.wwyy );
}
#endif
