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
//

//
//  Simplex Perlin Noise 2D Deriv
//  Return value range of -1.0->1.0, with format float3( value, xderiv, yderiv )
//
float3 noise_SimplexPerlin2D_Deriv( float2 P )
{
    //  https://github.com/BrianSharpe/Wombat/blob/master/SimplexPerlin2D_Deriv.glsl

    //  simplex math constants
    const float SKEWFACTOR = 0.36602540378443864676372317075294;            // 0.5*(sqrt(3.0)-1.0)
    const float UNSKEWFACTOR = 0.21132486540518711774542560974902;          // (3.0-sqrt(3.0))/6.0
    const float SIMPLEX_TRI_HEIGHT = 0.70710678118654752440084436210485;    // sqrt( 0.5 )	height of simplex triangle
    const float3 SIMPLEX_POINTS = float3( 1.0-UNSKEWFACTOR, -UNSKEWFACTOR, 1.0-2.0*UNSKEWFACTOR );  //  simplex triangle geo

    //  establish our grid cell.
    P *= SIMPLEX_TRI_HEIGHT;    // scale space so we can have an approx feature size of 1.0
    float2 Pi = floor( P + dot( P, float( SKEWFACTOR ).xx ) );

    // calculate the hash
    float4 Pt = float4( Pi.xy, Pi.xy + 1.0 );
    Pt = Pt - floor(Pt * ( 1.0 / 71.0 )) * 71.0;
    Pt += float2( 26.0, 161.0 ).xyxy;
    Pt *= Pt;
    Pt = Pt.xzxz * Pt.yyww;
    float4 hash_x = frac( Pt * ( 1.0 / 951.135664 ) );
    float4 hash_y = frac( Pt * ( 1.0 / 642.949883 ) );

    //  establish vectors to the 3 corners of our simplex triangle
    float2 v0 = Pi - dot( Pi, float( UNSKEWFACTOR ).xx ) - P;
    float4 v1pos_v1hash = (v0.x < v0.y) ? float4(SIMPLEX_POINTS.xy, hash_x.y, hash_y.y) : float4(SIMPLEX_POINTS.yx, hash_x.z, hash_y.z);
    float4 v12 = float4( v1pos_v1hash.xy, SIMPLEX_POINTS.zz ) + v0.xyxy;

    //  calculate the dotproduct of our 3 corner vectors with 3 random normalized vectors
    float3 grad_x = float3( hash_x.x, v1pos_v1hash.z, hash_x.w ) - 0.49999;
    float3 grad_y = float3( hash_y.x, v1pos_v1hash.w, hash_y.w ) - 0.49999;
    float3 norm = rsqrt( grad_x * grad_x + grad_y * grad_y );
    grad_x *= norm;
    grad_y *= norm;
    float3 grad_results = grad_x * float3( v0.x, v12.xz ) + grad_y * float3( v0.y, v12.yw );

    //	evaluate the kernel
    float3 m = float3( v0.x, v12.xz ) * float3( v0.x, v12.xz ) + float3( v0.y, v12.yw ) * float3( v0.y, v12.yw );
    m = max(0.5 - m, 0.0);
    float3 m2 = m*m;
    float3 m4 = m2*m2;

    //  calc the derivatives
    float3 temp = 8.0 * m2 * m * grad_results;
    float xderiv = dot( temp, float3( v0.x, v12.xz ) ) - dot( m4, grad_x );
    float yderiv = dot( temp, float3( v0.y, v12.yw ) ) - dot( m4, grad_y );

    //  Normalization factor to scale the final result to a strict 1.0->-1.0 range
    //  http://briansharpe.wordpress.com/2012/01/13/simplex-noise/##comment-36
    const float FINAL_NORMALIZATION = 99.204334582718712976990005025589;

    //  sum and return all results as a float3
    return float3( dot( m4, grad_results ), xderiv, yderiv ) * FINAL_NORMALIZATION;
}
