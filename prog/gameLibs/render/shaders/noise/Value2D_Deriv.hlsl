//
//  Wombat
//  An efficient texture-free GLSL procedural noise library
//  Source: https://github.com/BrianSharpe/Wombat
//  Derived from: https://github.com/BrianSharpe/GPU-Noise-Lib
//
//
//  Value Noise 2D Deriv
//  Return value range of 0.0->1.0, with format float3( value, xderiv, yderiv )
//
float3 noise_Value2D_Deriv( float2 P )
{
    //  https://github.com/BrianSharpe/Wombat/blob/master/Value2D_Deriv.glsl

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
    float4 blend = Pf.xyxy * Pf.xyxy * ( Pf.xyxy * ( Pf.xyxy * ( Pf.xyxy * float2( 6.0, 0.0 ).xxyy + float2( -15.0, 30.0 ).xxyy ) + float2( 10.0, -60.0 ).xxyy ) + float2( 0.0, 30.0 ).xxyy );
    float4 res0 = lerp( hash.xyxz, hash.zwyw, blend.yyxx );
    return float3( res0.x, 0.0, 0.0 ) + ( res0.yyw - res0.xxz ) * blend.xzw;
}
