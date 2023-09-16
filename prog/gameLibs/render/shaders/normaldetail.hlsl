#ifndef NORMAL_DETAIL_SH
#define NORMAL_DETAIL_SH 1

//http://blog.selfshadow.com/publications/blending-in-detail/

//n1, n2 - is -1..1
//6 instruction
half3 whiteout_ndetail(half3 n1, half3 n2)
{
  return normalize(half3(n1.xy + n2.xy, n1.z*n2.z));
}
//5 instructions
half3 UDN_ndetail(half3 n1, half3 n2)
{
  return normalize(half3(n1.xy + n2.xy, n1.z));
}
//texBase,texDetail is 0...1
//7 instructions
half3 RNM_ntexdetail(half3 texBase, half3 texDetail)
{
  half3 t = texBase*half3( 2.h,  2.h, 2.h) + half3(-1.h, -1.h,  0.h);
  half3 u = texDetail*half3(-2.h, -2.h, 2.h) + half3( 1.h,  1.h, -1.h);
  half3 r =  normalize(t*dot(t, u) - u*t.z);
  return r;
}
//7 instructions
half3 RNM_ndetail(half3 n1, half3 n2)
{
  half3 t = n1 + half3(0.h, 0.h,  1.h);
  half3 u = n2*half3(-1.h, -1.h, 1.h);
  half3 r =  normalize(t*dot(t, u) - u*t.z);
  return r;
}
//7 instructions, n1 - normal (-1..1), texDetail - tex(0..1)
half3 RNM_n_texdetail(half3 n1, half3 texDetail)
{
  half3 t = n1 + half3(0.h, 0.h,  1.h);
  half3 u = texDetail*half3(-2.h, -2.h, 2.h) + half3( 1.h,  1.h, -1.h);
  half3 r =  normalize(t*dot(t, u) - u*t.z);
  return r;
}
//texBase,texDetail is assumed to be of a 'unit' length (i.e. (texBase*2 - 1) is normalized)
//5 instructions
half3 RNM_ntexdetail_normalized(half3 texBase, half3 texDetail)
{
  half3 t = texBase*half3( 2.h,  2.h, 2.h) + half3(-1.h, -1.h,  0.h);
  half3 u = texDetail*half3(-2.h, -2.h, 2.h) + half3( 1.h,  1.h, -1.h);
  half3 r = t*(dot(t, u)*rcp(t.z)) - u;
  return r;
}
//n1,n2 is assumed to be of a 'unit' length
//5 instructions
half3 RNM_ndetail_normalized(half3 n1, half3 n2)
{
  half3 t = n1 + half3(0.h, 0.h,  1.h);
  half3 u = n2*half3(-1.h, -1.h, 1.h);
  half3 r = t*(dot(t, u)*rcp(t.z)) - u;
  return r;
}
//8 instructions, and actually not best
half3 unity_ndetail(half3 n1, half3 n2)
{
  half3x3 nBasis = half3x3(
      half3(n1.z, n1.y, -n1.x), // +90 degree rotation around y axis
      half3(n1.x, n1.z, -n1.y), // -90 degree rotation around x axis
      half3(n1.x, n1.y,  n1.z));

  return normalize(n2.x*nBasis[0] + n2.y*nBasis[1] + n2.z*nBasis[2]);
}


//n1,n2 is assumed to be of a 'unit' length
half3 RNM_ndetail_normalized_worldspace(half3 n1, half3 n2, half3 ws_normal)
{
  half3 t = n1 + ws_normal;
  half3 u = -n2 + ws_normal * (2.0h * dot(n2, ws_normal));
  half3 r = normalize( t*(dot(t, u)*rcp( dot(t, ws_normal) )) - u );
  return r;
}

half3 blend_normals_worldspace(half3 baseNormal, half3 detailNormal1, half3 detailNormal2, half mix_weight, half3 normal, float3 pointToEye,  float2 baseTC, float4 detailTC12)
{
  //mix 3 normal maps, 1-base 2&3 - detailed
  //1.calculate world normals (current normals are in texture space)
  //2.mix normals in world space

  //1.calculate world normals (current normals in texture space)
  //we have 3 different uv's, that's why we need translate all 3 normals to world space separately using right uv's
  half3 ws_normal = normalize(normal);

  // get edge vectors of the pixel triangle (only once, look perturb_normal() )
  float3 dp1 = ddx( pointToEye ); //normalize
  float3 dp2 = ddy( pointToEye );
  float3 dp2perp = cross( ws_normal, dp2 );
  float3 dp1perp = cross( dp1, ws_normal );

  //for each normal map we have different uv mapping, get world normals using right uv's
  baseNormal = get_world_normal_from_local(baseNormal, ws_normal, dp2perp, dp1perp, baseTC);
  half3 detailWorldNormal1 = get_world_normal_from_local(detailNormal1, ws_normal, dp2perp, dp1perp, detailTC12.xy);
  half3 detailWorldNormal2 = get_world_normal_from_local(detailNormal2, ws_normal, dp2perp, dp1perp, detailTC12.zw);

  //2.mix normals in world space
  half3 detailNormal = lerp(detailWorldNormal1, detailWorldNormal2, mix_weight);
  half3 worldNormal = RNM_ndetail_normalized_worldspace(baseNormal, detailNormal, ws_normal);
  return worldNormal;
}

half3 blend_normals_worldspace(half3 base_normal, half3 detail_normal, half3 vertex_normal, float3 point_to_eye, float2 base_texcoord, float2 detail_texcoord)
{
  half3 vertexWorldNormal = normalize(vertex_normal);

  // get edge vectors of the pixel triangle (only once, look perturb_normal() )
  float3 dp1 = ddx(point_to_eye);
  float3 dp2 = ddy(point_to_eye);
  float3 dp2perp = cross(vertexWorldNormal, dp2);
  float3 dp1perp = cross(dp1, vertexWorldNormal);

  // for each normal map we have different uv mapping, get world normals using right uv's
  half3 baseWorldNormal = get_world_normal_from_local(base_normal, vertexWorldNormal, dp2perp, dp1perp, base_texcoord);
  half3 detailWorldNormal = get_world_normal_from_local(detail_normal, vertexWorldNormal, dp2perp, dp1perp, detail_texcoord);

  // mix normals in world space
  return RNM_ndetail_normalized_worldspace(baseWorldNormal, detailWorldNormal, vertexWorldNormal);
}

half3 blend_normals_worldspace_safe(half3 base_normal, half3 detail_normal, half3 vertex_normal, float3 point_to_eye, float2 base_texcoord, float2 detail_texcoord)
{
  // Safe with zero texcoord gradients

  half3 vertexWorldNormal = normalize(vertex_normal);

  // get edge vectors of the pixel triangle (only once, look perturb_normal() )
  float3 dp1 = ddx(point_to_eye);
  float3 dp2 = ddy(point_to_eye);
  float3 dp2perp = cross(vertexWorldNormal, dp2);
  float3 dp1perp = cross(dp1, vertexWorldNormal);

  // for each normal map we have different uv mapping, get world normals using right uv's
  half3 baseWorldNormal = get_world_normal_from_local_safe(base_normal, vertexWorldNormal, dp2perp, dp1perp, base_texcoord);
  half3 detailWorldNormal = get_world_normal_from_local_safe(detail_normal, vertexWorldNormal, dp2perp, dp1perp, detail_texcoord);

  // mix normals in world space
  return RNM_ndetail_normalized_worldspace(baseWorldNormal, detailWorldNormal, vertexWorldNormal);
}
#endif
