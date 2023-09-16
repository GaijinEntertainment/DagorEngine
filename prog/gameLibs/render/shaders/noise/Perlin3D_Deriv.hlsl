//
//  Wombat
//  An efficient texture-free GLSL procedural noise library
//  Source: https://github.com/BrianSharpe/Wombat
//  Derived from: https://github.com/BrianSharpe/GPU-Noise-Lib
//
//
//  Perlin Noise 3D Deriv
//  Return value range of -1.0->1.0, with format float4( value, xderiv, yderiv, zderiv )
//
float4 noise_Perlin3D_Deriv( float3 P )
{
    //  https://github.com/BrianSharpe/Wombat/blob/master/Perlin3D_Deriv.glsl

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
    const float3 SOMELARGEFLOATS = float3( 635.298681, 682.357502, 668.926525 );
    const float3 ZINC = float3( 48.500388, 65.294118, 63.934599 );
    float3 lowz_mod = float3( 1.0 / ( SOMELARGEFLOATS + Pi.zzz * ZINC ) );
    float3 highz_mod = float3( 1.0 / ( SOMELARGEFLOATS + Pi_inc1.zzz * ZINC ) );
    float4 hashx0 = frac( Pt * lowz_mod.xxxx );
    float4 hashx1 = frac( Pt * highz_mod.xxxx );
    float4 hashy0 = frac( Pt * lowz_mod.yyyy );
    float4 hashy1 = frac( Pt * highz_mod.yyyy );
    float4 hashz0 = frac( Pt * lowz_mod.zzzz );
    float4 hashz1 = frac( Pt * highz_mod.zzzz );

    //	calculate the gradients
    float4 grad_x0 = hashx0 - 0.49999;
    float4 grad_y0 = hashy0 - 0.49999;
    float4 grad_z0 = hashz0 - 0.49999;
    float4 grad_x1 = hashx1 - 0.49999;
    float4 grad_y1 = hashy1 - 0.49999;
    float4 grad_z1 = hashz1 - 0.49999;
    float4 norm_0 = rsqrt( grad_x0 * grad_x0 + grad_y0 * grad_y0 + grad_z0 * grad_z0 );
    float4 norm_1 = rsqrt( grad_x1 * grad_x1 + grad_y1 * grad_y1 + grad_z1 * grad_z1 );
    grad_x0 *= norm_0;
    grad_y0 *= norm_0;
    grad_z0 *= norm_0;
    grad_x1 *= norm_1;
    grad_y1 *= norm_1;
    grad_z1 *= norm_1;

    //	calculate the dot products
    float4 dotval_0 = float2( Pf.x, Pf_min1.x ).xyxy * grad_x0 + float2( Pf.y, Pf_min1.y ).xxyy * grad_y0 + Pf.zzzz * grad_z0;
    float4 dotval_1 = float2( Pf.x, Pf_min1.x ).xyxy * grad_x1 + float2( Pf.y, Pf_min1.y ).xxyy * grad_y1 + Pf_min1.zzzz * grad_z1;

    //	C2 Interpolation
    float3 blend = Pf * Pf * Pf * (Pf * (Pf * 6.0 - 15.0) + 10.0);
    float3 blendDeriv = Pf * Pf * (Pf * (Pf * 30.0 - 60.0) + 30.0);

    //  the following is based off Milo Yips derivation, but modified for parallel execution
    //  http://stackoverflow.com/a/14141774

    //	Convert our data to a more parallel format
    float4 dotval0_grad0 = float4( dotval_0.x, grad_x0.x, grad_y0.x, grad_z0.x );
    float4 dotval1_grad1 = float4( dotval_0.y, grad_x0.y, grad_y0.y, grad_z0.y );
    float4 dotval2_grad2 = float4( dotval_0.z, grad_x0.z, grad_y0.z, grad_z0.z );
    float4 dotval3_grad3 = float4( dotval_0.w, grad_x0.w, grad_y0.w, grad_z0.w );
    float4 dotval4_grad4 = float4( dotval_1.x, grad_x1.x, grad_y1.x, grad_z1.x );
    float4 dotval5_grad5 = float4( dotval_1.y, grad_x1.y, grad_y1.y, grad_z1.y );
    float4 dotval6_grad6 = float4( dotval_1.z, grad_x1.z, grad_y1.z, grad_z1.z );
    float4 dotval7_grad7 = float4( dotval_1.w, grad_x1.w, grad_y1.w, grad_z1.w );

    //	evaluate common constants
    float4 k0_gk0 = dotval1_grad1 - dotval0_grad0;
    float4 k1_gk1 = dotval2_grad2 - dotval0_grad0;
    float4 k2_gk2 = dotval4_grad4 - dotval0_grad0;
    float4 k3_gk3 = dotval3_grad3 - dotval2_grad2 - k0_gk0;
    float4 k4_gk4 = dotval5_grad5 - dotval4_grad4 - k0_gk0;
    float4 k5_gk5 = dotval6_grad6 - dotval4_grad4 - k1_gk1;
    float4 k6_gk6 = (dotval7_grad7 - dotval6_grad6) - (dotval5_grad5 - dotval4_grad4) - k3_gk3;

    //	calculate final noise + deriv
    float u = blend.x;
    float v = blend.y;
    float w = blend.z;
    float4 result = dotval0_grad0
        + u * ( k0_gk0 + v * k3_gk3 )
        + v * ( k1_gk1 + w * k5_gk5 )
        + w * ( k2_gk2 + u * ( k4_gk4 + v * k6_gk6 ) );
    result.y += dot( float4( k0_gk0.x, k3_gk3.x * v, float2( k4_gk4.x, k6_gk6.x * v ) * w ), float4( blendDeriv.xxxx ) );
    result.z += dot( float4( k1_gk1.x, k3_gk3.x * u, float2( k5_gk5.x, k6_gk6.x * u ) * w ), float4( blendDeriv.yyyy ) );
    result.w += dot( float4( k2_gk2.x, k4_gk4.x * u, float2( k5_gk5.x, k6_gk6.x * u ) * v ), float4( blendDeriv.zzzz ) );
    return result * 1.1547005383792515290182975610039;  // scale things to a strict -1.0->1.0 range  *= 1.0/sqrt(0.75)
}
