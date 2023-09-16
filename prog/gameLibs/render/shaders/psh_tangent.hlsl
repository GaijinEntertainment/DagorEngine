#ifndef PIXEL_TANGENT_SPACE

#define PIXEL_TANGENT_SPACE 1
half3x3 cotangent_frame( half3 N, float3 p, float2 uv )
{
    // get edge vectors of the pixel triangle
    float3 dp1 = ddx( p );
    float3 dp2 = ddy( p );
    float2 duv1 = ddx( uv );
    float2 duv2 = ddy( uv );
 
    // solve the linear system
    float3 dp2perp = cross( dp2, N );
    float3 dp1perp = cross( N, dp1 );
    float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    float3 B = dp2perp * duv1.y + dp1perp * duv2.y;
 
    // construct a scale-invariant frame 
    float invmax = rsqrt( max( dot(T,T), dot(B,B) ) );
    return half3x3( T * invmax, B * invmax, N );// for some reason, half3x3( T * invmax, B * invmax, N ), as in original paper, spoils on seams
}
half3 perturb_normal_original( half3 mapNormal, half3 vN, float3 V, float2 texcoord )
{
/*##ifdef WITH_NORMALMAP_UNSIGNED
    map = map * 255./127. - 128./127.;
#endif
#ifdef WITH_NORMALMAP_2CHANNEL
    map.z = sqrt( 1. - dot( map.xy, map.xy ) );
#endif
#ifdef WITH_NORMALMAP_GREEN_UP
    map.y = -map.y;
#endif*/
    half3x3 TBN = cotangent_frame( vN, -V, texcoord );
#if HALF_PRECISION
    return normalize( mapNormal * TBN );
#else
    return normalize( mul(mapNormal, TBN) );
#endif
}
half3 perturb_normal( half3 localNorm, half3 N, float3 p, float2 uv )
{
  #if PERTURB_NORMAL_DISABLE
    return N;
  #else
    // get edge vectors of the pixel triangle
    float3 dp1 = ddx( p );
    float3 dp2 = ddy( p );
    float2 duv1 = ddx( uv );
    float2 duv2 = ddy( uv );
 
    // solve the linear system
    float3 dp2perp = cross( N, dp2 );
    float3 dp1perp = cross( dp1, N );
    float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    float3 B = dp2perp * duv1.y + dp1perp * duv2.y;
 
    // construct a scale-invariant frame 
    float invmax = rsqrt( max( dot(T,T), dot(B,B) ) );
    return half3(localNorm.z*N + (localNorm.x * invmax)*T + (localNorm.y * invmax)*B);
  #endif
}
half3 perturb_normal_precise( half3 localNorm, half3 N, float3 p, float2 uv )
{
  #if PERTURB_NORMAL_PRECISE_FAST || PERTURB_NORMAL_DISABLE
    return normalize(perturb_normal(localNorm, N, p, uv));//for performance reasons
  #else
    // get edge vectors of the pixel triangle
    float3 dp1 = ddx( p );
    float3 dp2 = ddy( p );
    float2 duv1 = ddx( uv );
    float2 duv2 = ddy( uv );
 
    // solve the linear system
    float3 dp2perp = cross( N, dp2 );
    float3 dp1perp = cross( dp1, N );
    float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    float3 B = dp2perp * duv1.y + dp1perp * duv2.y;
 
    float2 TBlen = float2(sqrt(dot(T, T)), sqrt(dot(B, B)));
    float2 TBlenRcp = TBlen > float2(1e-10, 1e-10) ? rcp(TBlen) : float2(0, 0);

    // construct a scale-invariant frame 
    return half3(localNorm.z * normalize(N) + (localNorm.x * TBlenRcp.x) * T + (localNorm.y * TBlenRcp.y) * B);
  #endif
}
void get_du_dv( half3 N, float3 p, float2 uv, out float3 dU, out float3 dV )
{
    // get edge vectors of the pixel triangle
    float3 dp1 = ddx( p );
    float3 dp2 = ddy( p );
    float2 duv1 = ddx( uv );
    float2 duv2 = ddy( uv );
 
    // solve the linear system
    float3 dp2perp = cross( N, dp2 );
    float3 dp1perp = cross( dp1, N );
    float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    float3 B = dp2perp * duv1.y + dp1perp * duv2.y;
 
    // construct a scale-invariant frame 
    dU = normalize(T);
    dV = normalize(B);
}
half3 get_world_normal_from_local(half3 localNorm, half3 N, float3 dp2perp, float3 dp1perp, float2 uv)
{
  //the same as perturb_normal(), but calc dp1perp & dp2perp separately
  float2 duv1 = ddx( uv );
  float2 duv2 = ddy( uv );

  float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
  float3 B = dp2perp * duv1.y + dp1perp * duv2.y;
 
  // construct a scale-invariant frame 
  float invmax = rsqrt( max( dot(T,T), dot(B,B) ) );
  return half3((localNorm.z*N + (localNorm.x * invmax)*T + (localNorm.y * invmax)*B));
}

half3 get_world_normal_from_local_safe(half3 localNorm, half3 N, float3 dp2perp, float3 dp1perp, float2 uv)
{
  //the same as perturb_normal(), but calc dp1perp & dp2perp separately
  float2 duv1 = ddx( uv );
  float2 duv2 = ddy( uv );

  float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
  float3 B = dp2perp * duv1.y + dp1perp * duv2.y;

  // construct a scale-invariant frame
  float grad = max(dot(T, T), dot(B, B));
  float invmax = rsqrt(grad);
  return (grad > 0) ? half3(localNorm.z * N + (localNorm.x * invmax) * T + (localNorm.y * invmax) * B) : N;
}

// assume vN, the interpolated vertex normal and 
// p is vertex to eeye
#endif
