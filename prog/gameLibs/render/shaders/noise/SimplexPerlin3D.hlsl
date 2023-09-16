//
//  Wombat
//  An efficient texture-free GLSL procedural noise library
//  Source: https://github.com/BrianSharpe/Wombat
//  Derived from: https://github.com/BrianSharpe/GPU-Noise-Lib
//
//
//  This is a modified version of Stefan Gustavson's and Ian McEwan's work at http://github.com/ashima/webgl-noise
//  Modifications are...
//  - faster random number generation
//  - analytical final normalization
//  - space scaled can have an approx feature size of 1.0
//  - filter kernel changed to fix discontinuities at tetrahedron boundaries
//

//
//  Simplex Perlin Noise 3D
//  Return value range of -1.0->1.0
//
float noise_SimplexPerlin3D( float3 P )
{
    //  https://github.com/BrianSharpe/Wombat/blob/master/SimplexPerlin3D.glsl

    //  simplex math constants
    const float SKEWFACTOR = 1.0/3.0;
    const float UNSKEWFACTOR = 1.0/6.0;
    const float SIMPLEX_CORNER_POS = 0.5;
    const float SIMPLEX_TETRAHADRON_HEIGHT = 0.70710678118654752440084436210485;    // sqrt( 0.5 )

    //  establish our grid cell.
    P *= SIMPLEX_TETRAHADRON_HEIGHT;    // scale space so we can have an approx feature size of 1.0
    float3 Pi = floor( P + dot( P, float( SKEWFACTOR).xxx ) );

    //  Find the vectors to the corners of our simplex tetrahedron
    float3 x0 = P - Pi + dot(Pi, float( UNSKEWFACTOR ).xxx );
    float3 g = step(x0.yzx, x0.xyz);
    float3 l = 1.0 - g;
    float3 Pi_1 = min( g.xyz, l.zxy );
    float3 Pi_2 = max( g.xyz, l.zxy );
    float3 x1 = x0 - Pi_1 + UNSKEWFACTOR;
    float3 x2 = x0 - Pi_2 + SKEWFACTOR;
    float3 x3 = x0 - SIMPLEX_CORNER_POS;

    //  pack them into a parallel-friendly arrangement
    float4 v1234_x = float4( x0.x, x1.x, x2.x, x3.x );
    float4 v1234_y = float4( x0.y, x1.y, x2.y, x3.y );
    float4 v1234_z = float4( x0.z, x1.z, x2.z, x3.z );

    // clamp the domain of our grid cell
    Pi.xyz = Pi.xyz - floor(Pi.xyz * ( 1.0 / 69.0 )) * 69.0;
    float3 Pi_inc1 = step( Pi, float( 69.0 - 1.5 ).xxx ) * ( Pi + 1.0 );

    //	generate the random vectors
    float4 Pt = float4( Pi.xy, Pi_inc1.xy ) + float2( 50.0, 161.0 ).xyxy;
    Pt *= Pt;
    float4 V1xy_V2xy = lerp( Pt.xyxy, Pt.zwzw, float4( Pi_1.xy, Pi_2.xy ) );
    Pt = float4( Pt.x, V1xy_V2xy.xz, Pt.z ) * float4( Pt.y, V1xy_V2xy.yw, Pt.w );
    const float3 SOMELARGEFLOATS = float3( 635.298681, 682.357502, 668.926525 );
    const float3 ZINC = float3( 48.500388, 65.294118, 63.934599 );
    float3 lowz_mods = float3( 1.0 / ( SOMELARGEFLOATS.xyz + Pi.zzz * ZINC.xyz ) );
    float3 highz_mods = float3( 1.0 / ( SOMELARGEFLOATS.xyz + Pi_inc1.zzz * ZINC.xyz ) );
    Pi_1 = ( Pi_1.z < 0.5 ) ? lowz_mods : highz_mods;
    Pi_2 = ( Pi_2.z < 0.5 ) ? lowz_mods : highz_mods;
    float4 hash_0 = frac( Pt * float4( lowz_mods.x, Pi_1.x, Pi_2.x, highz_mods.x ) ) - 0.49999;
    float4 hash_1 = frac( Pt * float4( lowz_mods.y, Pi_1.y, Pi_2.y, highz_mods.y ) ) - 0.49999;
    float4 hash_2 = frac( Pt * float4( lowz_mods.z, Pi_1.z, Pi_2.z, highz_mods.z ) ) - 0.49999;

    //	evaluate gradients
    float4 grad_results = rsqrt( hash_0 * hash_0 + hash_1 * hash_1 + hash_2 * hash_2 ) * ( hash_0 * v1234_x + hash_1 * v1234_y + hash_2 * v1234_z );

    //	Normalization factor to scale the final result to a strict 1.0->-1.0 range
    //	http://briansharpe.wordpress.com/2012/01/13/simplex-noise/##comment-36
    const float FINAL_NORMALIZATION = 37.837227241611314102871574478976;

    //  evaulate the kernel weights ( use (0.5-x*x)^3 instead of (0.6-x*x)^4 to fix discontinuities )
    float4 kernel_weights = v1234_x * v1234_x + v1234_y * v1234_y + v1234_z * v1234_z;
    kernel_weights = max(0.5 - kernel_weights, 0.0);
    kernel_weights = kernel_weights*kernel_weights*kernel_weights;

    //	sum with the kernel and return
    return dot( kernel_weights, grad_results ) * FINAL_NORMALIZATION;
}
