//
//  Wombat
//  An efficient texture-free GLSL procedural noise library
//  Source: https://github.com/BrianSharpe/Wombat
//  Derived from: https://github.com/BrianSharpe/GPU-Noise-Lib
//
//  Value Noise 3D Deriv
//  Return value range of 0.0->1.0, with format float4( value, xderiv, yderiv, zderiv )
float4 noise_Value3D_Deriv( float3 P )
{
    //  https://github.com/BrianSharpe/Wombat/blob/master/Value3D_Deriv.glsl

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
    float3 blendDeriv = Pf * Pf * (Pf * (Pf * 30.0 - 60.0) + 30.0);
    float4 res0 = lerp( hash_lowz, hash_highz, blend.z );
    float4 res1 = lerp( res0.xyxz, res0.zwyw, blend.yyxx );
    float4 res3 = lerp( float4( hash_lowz.xy, hash_highz.xy ), float4( hash_lowz.zw, hash_highz.zw ), blend.y );
    float2 res4 = lerp( res3.xz, res3.yw, blend.x );
    return float4( res1.x, 0.0, 0.0, 0.0 ) + ( float4( res1.yyw, res4.y ) - float4( res1.xxz, res4.x ) ) * float4( blend.x, blendDeriv );
}
