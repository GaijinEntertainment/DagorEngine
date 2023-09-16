//
//  Wombat
//  An efficient texture-free GLSL procedural noise library
//  Source: https://github.com/BrianSharpe/Wombat
//  Derived from: https://github.com/BrianSharpe/GPU-Noise-Lib
//
//  Perlin Noise 2D Deriv
//  Return value range of -1.0->1.0, with format float3( value, xderiv, yderiv )
//
float3 noise_Perlin2D_Deriv( float2 P )
{
    //  https://github.com/BrianSharpe/Wombat/blob/master/Perlin2D_Deriv.glsl

    // establish our grid cell and unit position
    float2 Pi = floor(P);
    float4 Pf_Pfmin1 = P.xyxy - float4( Pi, Pi + 1.0 );

    // calculate the hash
    float4 Pt = float4( Pi.xy, Pi.xy + 1.0 );
    Pt = Pt - floor(Pt * ( 1.0 / 71.0 )) * 71.0;
    Pt += float2( 26.0, 161.0 ).xyxy;
    Pt *= Pt;
    Pt = Pt.xzxz * Pt.yyww;
    float4 hash_x = frac( Pt * ( 1.0 / 951.135664 ) );
    float4 hash_y = frac( Pt * ( 1.0 / 642.949883 ) );

    // calculate the gradient results
    float4 grad_x = hash_x - 0.49999;
    float4 grad_y = hash_y - 0.49999;
    float4 norm = rsqrt( grad_x * grad_x + grad_y * grad_y );
    grad_x *= norm;
    grad_y *= norm;
    float4 dotval = ( grad_x * Pf_Pfmin1.xzxz + grad_y * Pf_Pfmin1.yyww );

    //	C2 Interpolation
    float4 blend = Pf_Pfmin1.xyxy * Pf_Pfmin1.xyxy * ( Pf_Pfmin1.xyxy * ( Pf_Pfmin1.xyxy * ( Pf_Pfmin1.xyxy * float2( 6.0, 0.0 ).xxyy + float2( -15.0, 30.0 ).xxyy ) + float2( 10.0, -60.0 ).xxyy ) + float2( 0.0, 30.0 ).xxyy );

    //	Convert our data to a more parallel format
    float3 dotval0_grad0 = float3( dotval.x, grad_x.x, grad_y.x );
    float3 dotval1_grad1 = float3( dotval.y, grad_x.y, grad_y.y );
    float3 dotval2_grad2 = float3( dotval.z, grad_x.z, grad_y.z );
    float3 dotval3_grad3 = float3( dotval.w, grad_x.w, grad_y.w );

    //	evaluate common constants
    float3 k0_gk0 = dotval1_grad1 - dotval0_grad0;
    float3 k1_gk1 = dotval2_grad2 - dotval0_grad0;
    float3 k2_gk2 = dotval3_grad3 - dotval2_grad2 - k0_gk0;

    //	calculate final noise + deriv
    float3 results = dotval0_grad0
                    + blend.x * k0_gk0
                    + blend.y * ( k1_gk1 + blend.x * k2_gk2 );
    results.yz += blend.zw * ( float2( k0_gk0.x, k1_gk1.x ) + blend.yx * k2_gk2.xx );
    return results * 1.4142135623730950488016887242097;  // scale things to a strict -1.0->1.0 range  *= 1.0/sqrt(0.5)
}
