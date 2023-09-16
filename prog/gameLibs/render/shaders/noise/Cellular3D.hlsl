//
//  Wombat
//  An efficient texture-free GLSL procedural noise library
//  Source: https://github.com/BrianSharpe/Wombat
//  Derived from: https://github.com/BrianSharpe/GPU-Noise-Lib
//
//  This represents a modified version of Stefan Gustavson's work at http://www.itn.liu.se/~stegu/GLSL-cellular
//  The noise is optimized to use a 2x2x2 search window instead of 3x3x3
//  Modifications are...
//  - faster random number generation
//  - analytical final normalization
//  - random point offset is restricted to prevent artifacts
//

//
//  Cellular Noise 3D
//  produces a range of 0.0->1.0
//
float noise_Cellular3D(float3 P)
{
    //	establish our grid cell and unit position
    float3 Pi = floor(P);
    float3 Pf = P - Pi;

    // clamp the domain
    Pi.xyz = Pi.xyz - floor(Pi.xyz * ( 1.0 / 69.0 )) * 69.0;
    float3 Pi_inc1 = step( Pi, float( 69.0 - 1.5 ).xxx ) * ( Pi + 1.0 );

    // calculate the hash ( over -1.0->1.0 range )
    float4 Pt = float4( Pi.xy, Pi_inc1.xy ) + float2( 50.0, 161.0 ).xyxy;
    Pt *= Pt;
    Pt = Pt.xzxz * Pt.yyww;
    const float3 SOMELARGEFLOATS = float3( 635.298681, 682.357502, 668.926525 );
    const float3 ZINC = float3( 48.500388, 65.294118, 63.934599 );
    float3 lowz_mod = float3( 1.0 / ( SOMELARGEFLOATS + Pi.zzz * ZINC ) );
    float3 highz_mod = float3( 1.0 / ( SOMELARGEFLOATS + Pi_inc1.zzz * ZINC ) );
    float4 hash_x0 = frac( Pt * lowz_mod.xxxx ) * 2.0 - 1.0;
    float4 hash_x1 = frac( Pt * highz_mod.xxxx ) * 2.0 - 1.0;
    float4 hash_y0 = frac( Pt * lowz_mod.yyyy ) * 2.0 - 1.0;
    float4 hash_y1 = frac( Pt * highz_mod.yyyy ) * 2.0 - 1.0;
    float4 hash_z0 = frac( Pt * lowz_mod.zzzz ) * 2.0 - 1.0;
    float4 hash_z1 = frac( Pt * highz_mod.zzzz ) * 2.0 - 1.0;

    //  generate the 8 point positions
    const float JITTER_WINDOW = 0.166666666;	// 0.166666666 will guarentee no artifacts.
    hash_x0 = ( ( hash_x0 * hash_x0 * hash_x0 ) - sign( hash_x0 ) ) * JITTER_WINDOW + float4( 0.0, 1.0, 0.0, 1.0 );
    hash_y0 = ( ( hash_y0 * hash_y0 * hash_y0 ) - sign( hash_y0 ) ) * JITTER_WINDOW + float4( 0.0, 0.0, 1.0, 1.0 );
    hash_x1 = ( ( hash_x1 * hash_x1 * hash_x1 ) - sign( hash_x1 ) ) * JITTER_WINDOW + float4( 0.0, 1.0, 0.0, 1.0 );
    hash_y1 = ( ( hash_y1 * hash_y1 * hash_y1 ) - sign( hash_y1 ) ) * JITTER_WINDOW + float4( 0.0, 0.0, 1.0, 1.0 );
    hash_z0 = ( ( hash_z0 * hash_z0 * hash_z0 ) - sign( hash_z0 ) ) * JITTER_WINDOW + float4( 0.0, 0.0, 0.0, 0.0 );
    hash_z1 = ( ( hash_z1 * hash_z1 * hash_z1 ) - sign( hash_z1 ) ) * JITTER_WINDOW + float4( 1.0, 1.0, 1.0, 1.0 );

    //	return the closest squared distance
    float4 dx1 = Pf.xxxx - hash_x0;
    float4 dy1 = Pf.yyyy - hash_y0;
    float4 dz1 = Pf.zzzz - hash_z0;
    float4 dx2 = Pf.xxxx - hash_x1;
    float4 dy2 = Pf.yyyy - hash_y1;
    float4 dz2 = Pf.zzzz - hash_z1;
    float4 d1 = dx1 * dx1 + dy1 * dy1 + dz1 * dz1;
    float4 d2 = dx2 * dx2 + dy2 * dy2 + dz2 * dz2;
    d1 = min(d1, d2);
    d1.xy = min(d1.xy, d1.wz);
    return min(d1.x, d1.y) * ( 9.0 / 12.0 ); // return a value scaled to 0.0->1.0
}
