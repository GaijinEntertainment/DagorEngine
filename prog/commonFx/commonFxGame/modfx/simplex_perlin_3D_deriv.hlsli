//
//  Wombat
//  An efficient texture-free GLSL procedural noise library
//  Source: https://github.com/BrianSharpe/Wombat
//  Derived from: https://github.com/BrianSharpe/GPU-Noise-Lib
//
//  I'm not one for copyrights.  Use the code however you wish.
//  All I ask is that credit be given back to the blog or myself when appropriate.
//  And also to let me know if you come up with any changes, improvements, thoughts or interesting uses for this stuff. :)
//  Thanks!
//
//  Brian Sharpe
//  brisharpe CIRCLE_A yahoo DOT com
//  http://briansharpe.wordpress.com
//  https://github.com/BrianSharpe
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
//  Simplex Perlin Noise 3D Deriv
//  Return value range of -1.0->1.0, with format vec4( value, xderiv, yderiv, zderiv )
//
float4 simplex_perlin_3D_deriv(float3 P)
{
    //  https://github.com/BrianSharpe/Wombat/blob/master/SimplexPerlin3D_Deriv.glsl

    //  simplex math constants
    const float SKEWFACTOR = 1.0/3.0;
    const float UNSKEWFACTOR = 1.0/6.0;
    const float SIMPLEX_CORNER_POS = 0.5;
    const float SIMPLEX_TETRAHEDRON_HEIGHT = 0.70710678118654752440084436210485;    // sqrt( 0.5 )

    //  establish our grid cell.
    P *= SIMPLEX_TETRAHEDRON_HEIGHT;    // scale space so we can have an approx feature size of 1.0
    float3 Pi = floor( P + float_xxx( dot( P, float3( SKEWFACTOR, SKEWFACTOR, SKEWFACTOR ) ) ) );

    //  Find the vectors to the corners of our simplex tetrahedron
    float3 x0 = P - Pi + float_xxx(dot(Pi, float3( UNSKEWFACTOR, UNSKEWFACTOR, UNSKEWFACTOR ) ));
    float3 g = step(float_yzx(x0), x0);
    float3 l = float3(1.f, 1.f, 1.f) - g;
    float3 Pi_1 = min( g, float_zxy(l) );
    float3 Pi_2 = max( g, float_zxy(l) );
    float3 x1 = x0 - Pi_1 + float3( UNSKEWFACTOR, UNSKEWFACTOR, UNSKEWFACTOR );
    float3 x2 = x0 - Pi_2 + float3( SKEWFACTOR, SKEWFACTOR, SKEWFACTOR );
    float3 x3 = x0 - float3( SIMPLEX_CORNER_POS, SIMPLEX_CORNER_POS, SIMPLEX_CORNER_POS );

    //  pack them into a parallel-friendly arrangement
    float4 v1234_x = float4( x0.x, x1.x, x2.x, x3.x );
    float4 v1234_y = float4( x0.y, x1.y, x2.y, x3.y );
    float4 v1234_z = float4( x0.z, x1.z, x2.z, x3.z );

    // clamp the domain of our grid cell
    Pi = Pi - floor(Pi * ( 1.f / 69.f )) * 69.f;
    float3 Pi_inc1 = step( Pi, float_xxx( 69.f - 1.5f ) ) * ( Pi + float_xxx( 1.f ) );

    //  generate the random vectors
    float4 Pt = float4( float_xy( Pi ), float_xy( Pi_inc1 ) ) + float_xyxy( float2( 50.0f, 161.0f ) );
    Pt *= Pt;
    float4 V1xy_V2xy = lerp( float_xyxy( Pt ), float_zwzw( Pt ), float4( float_xy( Pi_1 ), float_xy( Pi_2 ) ) );
    Pt = float4( Pt.x, float_xz(V1xy_V2xy), Pt.z ) * float4( Pt.y, float_yw(V1xy_V2xy), Pt.w );
    const float3 SOMELARGEFLOATS = float3( 635.298681f, 682.357502f, 668.926525f );
    const float3 ZINC = float3( 48.500388f, 65.294118f, 63.934599f );
    float3 lowz_mods = float3( float_xxx( 1.0f ) / ( SOMELARGEFLOATS + float_zzz(Pi) * ZINC ) );
    float3 highz_mods = float3( float_xxx( 1.0f ) / ( SOMELARGEFLOATS + float_zzz(Pi_inc1) * ZINC ) );
    Pi_1 = ( Pi_1.z < 0.5f ) ? lowz_mods : highz_mods;
    Pi_2 = ( Pi_2.z < 0.5f ) ? lowz_mods : highz_mods;
    float4 hash_0 = frac( Pt * float4( lowz_mods.x, Pi_1.x, Pi_2.x, highz_mods.x ) ) - float_xxxx(0.49999f);
    float4 hash_1 = frac( Pt * float4( lowz_mods.y, Pi_1.y, Pi_2.y, highz_mods.y ) ) - float_xxxx(0.49999f);
    float4 hash_2 = frac( Pt * float4( lowz_mods.z, Pi_1.z, Pi_2.z, highz_mods.z ) ) - float_xxxx(0.49999f);

    //  normalize random gradient vectors
    float4 norm = rsqrt_hlsl( hash_0 * hash_0 + hash_1 * hash_1 + hash_2 * hash_2 );
    hash_0 *= norm;
    hash_1 *= norm;
    hash_2 *= norm;

    //  evaluate gradients
    float4 grad_results = hash_0 * v1234_x + hash_1 * v1234_y + hash_2 * v1234_z;

    //  evaulate the kernel weights ( use (0.5-x*x)^3 instead of (0.6-x*x)^4 to fix discontinuities )
    float4 m = v1234_x * v1234_x + v1234_y * v1234_y + v1234_z * v1234_z;
    m = max(float_xxxx(0.5f) - m, float_xxxx(0.0f));
    float4 m2 = m*m;
    float4 m3 = m*m2;

    //  calc the derivatives
    float4 temp = -6.0 * m2 * grad_results;
    float xderiv = dot( temp, v1234_x ) + dot( m3, hash_0 );
    float yderiv = dot( temp, v1234_y ) + dot( m3, hash_1 );
    float zderiv = dot( temp, v1234_z ) + dot( m3, hash_2 );

    //  Normalization factor to scale the final result to a strict 1.0->-1.0 range
    //  http://briansharpe.wordpress.com/2012/01/13/simplex-noise/#comment-36
    const float FINAL_NORMALIZATION = 37.837227241611314102871574478976;

    //  sum and return all results as a vec3
    return float4( dot( m3, grad_results ), xderiv, yderiv, zderiv ) * FINAL_NORMALIZATION;
}