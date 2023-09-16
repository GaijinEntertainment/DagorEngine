//
//  Wombat
//  An efficient texture-free GLSL procedural noise library
//  Source: https://github.com/BrianSharpe/Wombat
//  Derived from: https://github.com/BrianSharpe/GPU-Noise-Lib
//
//  This represents a modified version of Stefan Gustavson's work at http://www.itn.liu.se/~stegu/GLSL-cellular
//  The noise is optimized to use a 2x2 search window instead of 3x3
//  Modifications are...
//  - faster random number generation
//  - analytical final normalization
//  - random point offset is restricted to prevent artifacts
//

//
//  Cellular Noise 2D Deriv
//  Return value range of 0.0->1.0, with format float3( value, xderiv, yderiv )
//
float3 noise_Cellular2D_Deriv( float2 P )
{
    //  https://github.com/BrianSharpe/Wombat/blob/master/Cellular2D_Deriv.glsl

    //  establish our grid cell and unit position
    float2 Pi = floor(P);
    float2 Pf = P - Pi;

    //  calculate the hash
    float4 Pt = float4( Pi.xy, Pi.xy + 1.0 );
    Pt = Pt - floor(Pt * ( 1.0 / 71.0 )) * 71.0;
    Pt += float2( 26.0, 161.0 ).xyxy;
    Pt *= Pt;
    Pt = Pt.xzxz * Pt.yyww;
    float4 hash_x = frac( Pt * ( 1.0 / 951.135664 ) );
    float4 hash_y = frac( Pt * ( 1.0 / 642.949883 ) );

    //  generate the 4 points
    hash_x = hash_x * 2.0 - 1.0;
    hash_y = hash_y * 2.0 - 1.0;
    const float JITTER_WINDOW = 0.25;   // 0.25 will guarentee no artifacts
    hash_x = ( ( hash_x * hash_x * hash_x ) - sign( hash_x ) ) * JITTER_WINDOW + float4( 0.0, 1.0, 0.0, 1.0 );
    hash_y = ( ( hash_y * hash_y * hash_y ) - sign( hash_y ) ) * JITTER_WINDOW + float4( 0.0, 0.0, 1.0, 1.0 );

    //	return the closest squared distance + derivatives ( thanks to Jonathan Dupuy )
    float4 dx = Pf.xxxx - hash_x;
    float4 dy = Pf.yyyy - hash_y;
    float4 d = dx * dx + dy * dy;
    float3 t1 = d.x < d.y ? float3( d.x, dx.x, dy.x ) : float3( d.y, dx.y, dy.y );
    float3 t2 = d.z < d.w ? float3( d.z, dx.z, dy.z ) : float3( d.w, dx.w, dy.w );
    return ( t1.x < t2.x ? t1 : t2 ) * float3( 1.0, 2.0, 2.0 ) * ( 1.0 / 1.125 ); // return a value scaled to 0.0->1.0
}
