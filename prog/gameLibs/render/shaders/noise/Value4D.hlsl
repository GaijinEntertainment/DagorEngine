//
//  Wombat
//  An efficient texture-free GLSL procedural noise library
//  Source: https://github.com/BrianSharpe/Wombat
//  Derived from: https://github.com/BrianSharpe/GPU-Noise-Lib

//	Value Noise 4D
//	Return value range of 0.0->1.0
//
float noise_Value4D( float4 P )
{
    //  https://github.com/BrianSharpe/Wombat/blob/master/Value4D.glsl

    // establish our grid cell and unit position
    float4 Pi = floor(P);
    float4 Pf = P - Pi;

    // clamp the domain
    Pi = Pi - floor(Pi * ( 1.0 / 69.0 )) * 69.0;
    float4 Pi_inc1 = step( Pi, float( 69.0 - 1.5 ).xxxx ) * ( Pi + 1.0 );

    // calculate the hash
    const float4 OFFSET = float4( 16.841230, 18.774548, 16.873274, 13.664607 );
    const float4 NOISE_SCALE = float4( 0.102007, 0.114473, 0.139651, 0.084550 );
    Pi = ( Pi * NOISE_SCALE ) + OFFSET;
    Pi_inc1 = ( Pi_inc1 * NOISE_SCALE ) + OFFSET;
    Pi *= Pi;
    Pi_inc1 *= Pi_inc1;
    float4 x0y0_x1y0_x0y1_x1y1 = float4( Pi.x, Pi_inc1.x, Pi.x, Pi_inc1.x ) * float4( Pi.yy, Pi_inc1.yy );
    float4 z0w0_z1w0_z0w1_z1w1 = float4( Pi.z, Pi_inc1.z, Pi.z, Pi_inc1.z ) * float4( Pi.ww, Pi_inc1.ww ) * float( 1.0 / 56974.746094 );
    float4 z0w0_hash = frac( x0y0_x1y0_x0y1_x1y1 * z0w0_z1w0_z0w1_z1w1.xxxx );
    float4 z1w0_hash = frac( x0y0_x1y0_x0y1_x1y1 * z0w0_z1w0_z0w1_z1w1.yyyy );
    float4 z0w1_hash = frac( x0y0_x1y0_x0y1_x1y1 * z0w0_z1w0_z0w1_z1w1.zzzz );
    float4 z1w1_hash = frac( x0y0_x1y0_x0y1_x1y1 * z0w0_z1w0_z0w1_z1w1.wwww );

    //	blend the results and return
    float4 blend = Pf * Pf * Pf * (Pf * (Pf * 6.0 - 15.0) + 10.0);
    float4 res0 = z0w0_hash + ( z0w1_hash - z0w0_hash ) * blend.wwww;
    float4 res1 = z1w0_hash + ( z1w1_hash - z1w0_hash ) * blend.wwww;
    res0 = res0 + ( res1 - res0 ) * blend.zzzz;
    blend.zw = float2( 1.0 - blend.xy );
    return dot( res0, blend.zxzx * blend.wwyy );
}
