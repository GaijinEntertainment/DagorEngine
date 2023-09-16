//
//  Wombat
//  An efficient texture-free GLSL procedural noise library
//  Source: https://github.com/BrianSharpe/Wombat
//  Derived from: https://github.com/BrianSharpe/GPU-Noise-Lib
//
//
//  Perlin Noise 4D
//  Return value range of -1.0->1.0
//
float noise_Perlin4D( float4 P )
{
    //  https://github.com/BrianSharpe/Wombat/blob/master/Perlin4D.glsl

    // establish our grid cell and unit position
    float4 Pi = floor(P);
    float4 Pf = P - Pi;
    float4 Pf_min1 = Pf - 1.0;

    // clamp the domain
    Pi = Pi - floor(Pi * ( 1.0 / 69.0 )) * 69.0;
    float4 Pi_inc1 = step( Pi, float( 69.0 - 1.5 ).xxxx ) * ( Pi + 1.0 );

    // calculate the hash.
    const float4 OFFSET = float4( 16.841230, 18.774548, 16.873274, 13.664607 );
    const float4 NOISE_SCALE = float4( 0.102007, 0.114473, 0.139651, 0.084550 );
    Pi = ( Pi * NOISE_SCALE ) + OFFSET;
    Pi_inc1 = ( Pi_inc1 * NOISE_SCALE ) + OFFSET;
    Pi *= Pi;
    Pi_inc1 *= Pi_inc1;
    float4 x0y0_x1y0_x0y1_x1y1 = float4( Pi.x, Pi_inc1.x, Pi.x, Pi_inc1.x ) * float4( Pi.yy, Pi_inc1.yy );
    float4 z0w0_z1w0_z0w1_z1w1 = float4( Pi.z, Pi_inc1.z, Pi.z, Pi_inc1.z ) * float4( Pi.ww, Pi_inc1.ww );
    const float4 SOMELARGEFLOATS = float4( 56974.746094, 47165.636719, 55049.667969, 49901.273438 );
    float4 hashval = x0y0_x1y0_x0y1_x1y1 * z0w0_z1w0_z0w1_z1w1.xxxx;
    float4 lowz_loww_hash_0 = frac( hashval * ( 1.0 / SOMELARGEFLOATS.x ) );
    float4 lowz_loww_hash_1 = frac( hashval * ( 1.0 / SOMELARGEFLOATS.y ) );
    float4 lowz_loww_hash_2 = frac( hashval * ( 1.0 / SOMELARGEFLOATS.z ) );
    float4 lowz_loww_hash_3 = frac( hashval * ( 1.0 / SOMELARGEFLOATS.w ) );
    hashval = x0y0_x1y0_x0y1_x1y1 * z0w0_z1w0_z0w1_z1w1.yyyy;
    float4 highz_loww_hash_0 = frac( hashval * ( 1.0 / SOMELARGEFLOATS.x ) );
    float4 highz_loww_hash_1 = frac( hashval * ( 1.0 / SOMELARGEFLOATS.y ) );
    float4 highz_loww_hash_2 = frac( hashval * ( 1.0 / SOMELARGEFLOATS.z ) );
    float4 highz_loww_hash_3 = frac( hashval * ( 1.0 / SOMELARGEFLOATS.w ) );
    hashval = x0y0_x1y0_x0y1_x1y1 * z0w0_z1w0_z0w1_z1w1.zzzz;
    float4 lowz_highw_hash_0 = frac( hashval * ( 1.0 / SOMELARGEFLOATS.x ) );
    float4 lowz_highw_hash_1 = frac( hashval * ( 1.0 / SOMELARGEFLOATS.y ) );
    float4 lowz_highw_hash_2 = frac( hashval * ( 1.0 / SOMELARGEFLOATS.z ) );
    float4 lowz_highw_hash_3 = frac( hashval * ( 1.0 / SOMELARGEFLOATS.w ) );
    hashval = x0y0_x1y0_x0y1_x1y1 * z0w0_z1w0_z0w1_z1w1.wwww;
    float4 highz_highw_hash_0 = frac( hashval * ( 1.0 / SOMELARGEFLOATS.x ) );
    float4 highz_highw_hash_1 = frac( hashval * ( 1.0 / SOMELARGEFLOATS.y ) );
    float4 highz_highw_hash_2 = frac( hashval * ( 1.0 / SOMELARGEFLOATS.z ) );
    float4 highz_highw_hash_3 = frac( hashval * ( 1.0 / SOMELARGEFLOATS.w ) );

    // calculate the gradients
    lowz_loww_hash_0 -= 0.49999;
    lowz_loww_hash_1 -= 0.49999;
    lowz_loww_hash_2 -= 0.49999;
    lowz_loww_hash_3 -= 0.49999;
    highz_loww_hash_0 -= 0.49999;
    highz_loww_hash_1 -= 0.49999;
    highz_loww_hash_2 -= 0.49999;
    highz_loww_hash_3 -= 0.49999;
    lowz_highw_hash_0 -= 0.49999;
    lowz_highw_hash_1 -= 0.49999;
    lowz_highw_hash_2 -= 0.49999;
    lowz_highw_hash_3 -= 0.49999;
    highz_highw_hash_0 -= 0.49999;
    highz_highw_hash_1 -= 0.49999;
    highz_highw_hash_2 -= 0.49999;
    highz_highw_hash_3 -= 0.49999;

    float4 grad_results_lowz_loww = rsqrt( lowz_loww_hash_0 * lowz_loww_hash_0 + lowz_loww_hash_1 * lowz_loww_hash_1 + lowz_loww_hash_2 * lowz_loww_hash_2 + lowz_loww_hash_3 * lowz_loww_hash_3 );
    grad_results_lowz_loww *= ( float2( Pf.x, Pf_min1.x ).xyxy * lowz_loww_hash_0 + float2( Pf.y, Pf_min1.y ).xxyy * lowz_loww_hash_1 + Pf.zzzz * lowz_loww_hash_2 + Pf.wwww * lowz_loww_hash_3 );

    float4 grad_results_highz_loww = rsqrt( highz_loww_hash_0 * highz_loww_hash_0 + highz_loww_hash_1 * highz_loww_hash_1 + highz_loww_hash_2 * highz_loww_hash_2 + highz_loww_hash_3 * highz_loww_hash_3 );
    grad_results_highz_loww *= ( float2( Pf.x, Pf_min1.x ).xyxy * highz_loww_hash_0 + float2( Pf.y, Pf_min1.y ).xxyy * highz_loww_hash_1 + Pf_min1.zzzz * highz_loww_hash_2 + Pf.wwww * highz_loww_hash_3 );

    float4 grad_results_lowz_highw = rsqrt( lowz_highw_hash_0 * lowz_highw_hash_0 + lowz_highw_hash_1 * lowz_highw_hash_1 + lowz_highw_hash_2 * lowz_highw_hash_2 + lowz_highw_hash_3 * lowz_highw_hash_3 );
    grad_results_lowz_highw *= ( float2( Pf.x, Pf_min1.x ).xyxy * lowz_highw_hash_0 + float2( Pf.y, Pf_min1.y ).xxyy * lowz_highw_hash_1 + Pf.zzzz * lowz_highw_hash_2 + Pf_min1.wwww * lowz_highw_hash_3 );

    float4 grad_results_highz_highw = rsqrt( highz_highw_hash_0 * highz_highw_hash_0 + highz_highw_hash_1 * highz_highw_hash_1 + highz_highw_hash_2 * highz_highw_hash_2 + highz_highw_hash_3 * highz_highw_hash_3 );
    grad_results_highz_highw *= ( float2( Pf.x, Pf_min1.x ).xyxy * highz_highw_hash_0 + float2( Pf.y, Pf_min1.y ).xxyy * highz_highw_hash_1 + Pf_min1.zzzz * highz_highw_hash_2 + Pf_min1.wwww * highz_highw_hash_3 );

    // Classic Perlin Interpolation
    float4 blend = Pf * Pf * Pf * (Pf * (Pf * 6.0 - 15.0) + 10.0);
    float4 res0 = grad_results_lowz_loww + ( grad_results_lowz_highw - grad_results_lowz_loww ) * blend.wwww;
    float4 res1 = grad_results_highz_loww + ( grad_results_highz_highw - grad_results_highz_loww ) * blend.wwww;
    res0 = res0 + ( res1 - res0 ) * blend.zzzz;
    blend.zw = float( 1.0 ).xx - blend.xy;
    return dot( res0, blend.zxzx * blend.wwyy );
}
