// [[input_declaration]]

#define PI 3.1415926535


// copy-paste from from hardware_defines.sh
#define LOOP [loop]
#define UNROLL [unroll]
#define BRANCH [branch]
#define FLATTEN [flatten]

#ifndef IS_DISTANT_FOG
#define IS_DISTANT_FOG 0
#endif

#ifndef shadow2DArray
  #define shadow2DArray(a, uv) a.SampleCmpLevelZero(a##_cmpSampler, (uv).xyz, (uv).w)
#endif

#define tex_hmap_low_samplerstate  land_heightmap_tex_samplerstate
#define volfog_mask_tex_samplerstate  land_heightmap_tex_samplerstate
#define prev_volfog_shadow_samplerstate  land_heightmap_tex_samplerstate

#define downsampled_checkerboard_depth_tex_samplerstate  downsampled_far_depth_tex_samplerstate
#define prev_downsampled_far_depth_tex_samplerstate     downsampled_far_depth_tex_samplerstate
#define downsampled_close_depth_tex_samplerstate        downsampled_far_depth_tex_samplerstate
#define distant_heightmap_tex_samplerstate              land_heightmap_tex_samplerstate


#define HAS_STATIC_SHADOW 1
#define FASTEST_STATIC_SHADOW_PCF 1
#define STATIC_SHADOW_CASCADE_NUM 2
#define static_shadow_cascade_0_tor static_shadow_cascade_0_scale_ofs_z_tor.zw
#define static_shadow_cascade_1_tor static_shadow_cascade_1_scale_ofs_z_tor.zw

#define STATIC_SHADOW_SAMPLER_STATE_REF SamplerComparisonState
#define STATIC_SHADOW_TEXTURE_REF Texture2DArray<float4>

half static_shadow_sample(float2 uv, float z, STATIC_SHADOW_TEXTURE_REF tex, STATIC_SHADOW_SAMPLER_STATE_REF tex_cmpSampler, float layer)
{
  return (half)shadow2DArray(tex, float4(uv, layer, z));
}


#define NaN asfloat(0x7F800001)

float lengthSq(float2 v) { return dot(v,v); }
float lengthSq(float3 v) { return dot(v,v); }
float lengthSq(float4 v) { return dot(v,v); }

float get_blur_kernel_weight(bool gauss, float invSqSigma, float x)
{
  if (gauss)
  {
    float d = x * x;
    d = d * d;
    d = d * d;
    return exp(invSqSigma * (x * x + 1.0)) * max(1.0 - d, 0.0);
  }
  else
  {
    float d = 1.0 - x * x;
    return max(d * d * d, 0.0);
  }
}

float4 linear_sample(Texture2D tex, float2 tc, bool wrap, int2 offset = int2(0, 0))
{
  uint w, h;
  tex.GetDimensions(w, h);
  float2 scaledTc = tc * float2(w, h) - 0.5;
  int2 intTc = int2(floor(scaledTc));

  int2 p00 = intTc + int2(0, 0) + offset;
  int2 p10 = intTc + int2(1, 0) + offset;
  int2 p01 = intTc + int2(0, 1) + offset;
  int2 p11 = intTc + int2(1, 1) + offset;
  if (wrap)
  {
    p00 = uint2(p00) & (uint2(w, h) - 1);
    p10 = uint2(p10) & (uint2(w, h) - 1);
    p01 = uint2(p01) & (uint2(w, h) - 1);
    p11 = uint2(p11) & (uint2(w, h) - 1);
  }
  else
  {
    int2 maxIndex = int2(w, h) - 1;
    p00 = clamp(p00, int2(0, 0), maxIndex);
    p10 = clamp(p10, int2(0, 0), maxIndex);
    p01 = clamp(p01, int2(0, 0), maxIndex);
    p11 = clamp(p11, int2(0, 0), maxIndex);
  }

  float2 delta = scaledTc - intTc;

  float4 v0 = lerp(tex[p00], tex[p10], delta.x);
  float4 v1 = lerp(tex[p01], tex[p11], delta.x);
  return lerp(v0, v1, delta.y);
}

float4 get_texel(Texture2D tex, float2 tc, bool wrap, int2 offset = int2(0, 0))
{
  uint w, h;
  tex.GetDimensions(w, h);
  int2 intTc = int2(floor(tc * float2(w, h) - 0.5)) + offset;
  int2 p00 = wrap ? uint2(intTc) & (uint2(w, h) - 1) : clamp(intTc, int2(0, 0), int2(w, h) - 1);
  return tex[p00];
}

float2 get_dimensions(Texture2D tex)
{
  float2 dim;
  tex.GetDimensions(dim.x, dim.y);
  return dim;
}

float wrappedDistanceSq(float2 tc0, float2 tc1)
{
  float2 tcTo = tc1 - tc0;
  tcTo = min(abs(tcTo - 1), min(abs(tcTo + 1), abs(tcTo)));
  return lengthSq(tcTo);
}

float wrappedDistanceSqi(float2 tc0, float2 tc1, float2 dim)
{
  float2 tcTo = tc1 - tc0;
  tcTo = min(abs(tcTo - dim), min(abs(tcTo + dim), abs(tcTo)));
  return lengthSq(tcTo);
}

float4 safe_normalize(float4 v)
{
  v = normalize(v);
  return isfinite(v) ? v : 0;
}

float2 encodeSDF(int2 tci, bool above, bool valid)
{
 return float2(tci | int2(above ? 32768 : 0, valid ? 32768 : 0)) / 65535.0;
}

int2 decodeSDF(float2 tc, out bool above, out bool valid)
{
 int2 tci = tc * 65535;
 above = tci.x >= 32768;
 valid = tci.y >= 32768;
 return tci & 0x7FFF;
}

float3 safe_normalize(float3 v)
{
  v = normalize(v);
  return isfinite(v) ? v : 0;
}

float3 getNormal(Texture2D tex, bool wrap, float2 tc, float cellSize) 
{
  float w, h;
  tex.GetDimensions(w, h);
  float hu = (get_texel(tex, tc, wrap, int2(-1, 0)) - get_texel(tex, tc, wrap, int2(1, 0))).r;
  float hv = (get_texel(tex, tc, wrap, int2(0, -1)) - get_texel(tex, tc, wrap, int2(0, 1))).r;
  return safe_normalize(float3(hu * w, hv * h, cellSize));
}

float2 safe_normalize(float2 v)
{
  v = normalize(v);
  return isfinite(v) ? v : 0;
}

float2 get2DGradient(Texture2D tex, bool wrap, float2 tc) 
{
  float w, h;
  tex.GetDimensions(w, h);
  float hu = (get_texel(tex, tc, wrap, int2(-1, 0)) - get_texel(tex, tc, wrap, int2(1, 0))).r;
  float hv = (get_texel(tex, tc, wrap, int2(0, -1)) - get_texel(tex, tc, wrap, int2(0, 1))).r;
  return safe_normalize(float2(hu, hv));
}

float getCurvature(Texture2D tex, bool wrap, float2 tc, float curvScale) //curvScale = 1/gridCellSize
{
  #define FC 4
  #define FSZ (FC * 2 + 1)
  float curvWt[FSZ][FSZ];
  float wtSum = 0, kSum = 0;
  int fx, fy;


  UNROLL
  for ( fy = -FC; fy <= FC; ++fy)
  {
    UNROLL
    for (fx = -FC; fx <= FC; ++fx)
    {
      if (fx == 0 && fy == 0)
        continue;

      int dx = fx;
      int dy = fy;
      float d = sqrt(float(dx * dx + dy * dy));

      float k = max(0, (FC + 1 - d) / (FC + 1));

      k = (3 - 2 * k) * k * k;

      curvWt[fy + FC][fx + FC] = k / d;
      wtSum += k / d;
      kSum += k;
    }
  }

  curvWt[FC][FC] = -wtSum;

  UNROLL
  for (fy = 0; fy < FSZ; ++fy)
  {
    UNROLL
    for (fx = 0; fx < FSZ; ++fx)
      curvWt[fy][fx] /= kSum;
  }

  float curv=0;
  UNROLL
  for (fy= -FC; fy <= FC; ++fy)
  {
    UNROLL
    for (fx = -FC; fx <= FC; ++fx)
      curv += curvWt[fy + FC][fx + FC] * get_texel(tex, tc, wrap, int2(fx, fy)).r;
  }

  float w, h;
  tex.GetDimensions(w, h);
  return curv * (w * curvScale);
}


float calc_polynom4(in float4 a, float t) // t = [0..1]
{
  float t2 = t * t;
  return saturate(a[0] + a[1] * t + a[2] * t2 + a[3] * t * t2);
}


uint uint_noise1D(uint position, uint seed)
{
  uint BIT_NOISE1 = 0x68E31DA4;
  uint BIT_NOISE2 = 0xB5297A4D;
  uint BIT_NOISE3 = 0x1B56C4E9;
  uint mangled = position;
  mangled *= BIT_NOISE1;
  mangled += seed;
  mangled ^= (mangled >> 8);
  mangled += BIT_NOISE2;
  mangled ^= (mangled << 8);
  mangled *= BIT_NOISE3;
  mangled ^= (mangled >> 8);
  return mangled;
}

uint uint_noise2D(int posX, int posY, uint seed)
{
  int PRIME_NUMBER = 198491317;
  return uint_noise1D(posX + (PRIME_NUMBER * posY), seed);
}

uint uint_noise3D(int posX, int posY, int posZ, uint seed)
{
  int PRIME_NUMBER1 = 198491317;
  int PRIME_NUMBER2 = 6542989;
  return uint_noise1D(posX + (PRIME_NUMBER1 * posY) + (PRIME_NUMBER2 * posZ), seed);
}

uint2 scramble_TEA(uint2 v)
{
  uint k[4] = { 0xA341316Cu, 0xC8013EA4u, 0xAD90777Du, 0x7E95761Eu };

  uint y = v[0];
  uint z = v[1];
  uint sum = 0;

  #define TEA_ITERATIONS 4
  UNROLL
  for(uint i = 0; i < TEA_ITERATIONS; ++i)
  {
    sum += 0x9e3779b9;
    y += (z << 4u) + k[0] ^ z + sum ^ (z >> 5u) + k[1];
    z += (y << 4u) + k[2] ^ y + sum ^ (y >> 5u) + k[3];
  }

  return uint2(y, z);
}

float2 rand2(float2 co)
{
  return frac(sin(co.xy * 12.9898) * 43758.5453);
}

float rand2d(float2 co)
{
  return frac(sin(co.x * 12.9898 - co.y * 43.44431) * 43758.5453);
}

float rand(float co)
{
  return frac(sin(co * 12.9898) * 43758.5453);
}

uint rand_seed(float2 texcoord)
{
  float2 seed = rand2(texcoord);
  return uint(seed.x * 7829 + seed.y * 113);
}

uint rand_lcg(inout uint g_rng_state)
{
  // LCG values from Numerical Recipes
  g_rng_state = 1664525U * g_rng_state + 1013904223U;
  return g_rng_state;
}

float rand_lcg_float(inout uint g_rng_state)
{
  uint ret = rand_lcg(g_rng_state);
  //return float(ret) * (1.0 / 4294967296.0);
  return frac(float(ret) * (1.0 / 262144.0));
}


float noise_Perlin2D( float2 P )
{
  //  https://github.com/BrianSharpe/Wombat/blob/master/Perlin2D.glsl

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
  float4 grad_results = rsqrt( grad_x * grad_x + grad_y * grad_y ) * ( grad_x * Pf_Pfmin1.xzxz + grad_y * Pf_Pfmin1.yyww );

  // Classic Perlin Interpolation
  grad_results *= 1.4142135623730950488016887242097;  // scale things to a strict -1.0->1.0 range  *= 1.0/sqrt(0.5)
  float2 blend = Pf_Pfmin1.xy * Pf_Pfmin1.xy * Pf_Pfmin1.xy * (Pf_Pfmin1.xy * (Pf_Pfmin1.xy * 6.0 - 15.0) + 10.0);
  float4 blend2 = float4( blend, float2( 1.0 - blend ) );
  return dot( grad_results, blend2.zxzx * blend2.wwyy );
}

float noise_Value2D( float2 P )
{
  //  https://github.com/BrianSharpe/Wombat/blob/master/Value2D.glsl

  // establish our grid cell and unit position
  float2 Pi = floor(P);
  float2 Pf = P - Pi;

  // calculate the hash.
  float4 Pt = float4( Pi.xy, Pi.xy + 1.0 );
  Pt = Pt - floor(Pt * ( 1.0 / 71.0 )) * 71.0;
  Pt += float2( 26.0, 161.0 ).xyxy;
  Pt *= Pt;
  Pt = Pt.xzxz * Pt.yyww;
  float4 hash = frac( Pt * ( 1.0 / 951.135664 ) );

  // blend the results and return
  float2 blend = Pf * Pf * Pf * (Pf * (Pf * 6.0 - 15.0) + 10.0);
  float4 blend2 = float4( blend, float2( 1.0 - blend ) );
  return dot( hash, blend2.zxzx * blend2.wwyy );
}

int pow2(int x)
{
  return x * x;
}

float pow2(float x)
{
  return x * x;
}

float2 pow2(float2 x)
{
  return x * x;
}

float3 pow2(float3 x)
{
  return x * x;
}

float4 pow2(float4 x)
{
  return x * x;
}

#define tex2Dlod(a, uv) a.SampleLevel(a##_samplerstate, (uv).xy, (uv).w)
#define tex3Dlod(a, uv) a.SampleLevel(a##_samplerstate, (uv).xyz, (uv).w)

float get_smooth_noise3d(float3 v)
{
  float3 i = floor(v);
  float3 f = v - i;

  f = f * f * (-2.0f * f + 3.0f);

  float2 uv = ((i.xy + float2(7.0f, 17.0f) * i.z)) + f.xy;
  float lowz = tex2Dlod(noise_64_tex_l8, float4((uv.xy + 0.5f) / 64.0f, 0, 0)).x;
  uv.xy += float2(7.0f, 17.0f);
  float highz = tex2Dlod(noise_64_tex_l8, float4((uv.xy + 0.5f) / 64.0f, 0, 0)).x;
  float r0 = lerp(lowz, highz, f.z);

  return r0;
}


float sample_volfog_mask(float2 world_xz)
{
  float2 tc = world_xz * world_to_volfog_mask.xy + world_to_volfog_mask.zw;
  return tex2Dlod(volfog_mask_tex, float4(tc, 0, 0)).x;
}

float sample_distant_heightmap(float2 world_xz)
{
  float2 tc_low = world_xz * distant_world_to_hmap_low.xy + distant_world_to_hmap_low.zw;
  return tex2Dlod(distant_heightmap_tex, float4(tc_low, 0, 0)).x * distant_heightmap_scale.x + distant_heightmap_scale.y;
}
float sample_heightmap(float2 world_xz, float lod)
{
  float2 tc_low = world_xz * world_to_hmap_low.xy + world_to_hmap_low.zw;
  return tex2Dlod(tex_hmap_low, float4(tc_low, 0, lod)).x * heightmap_scale.x + heightmap_scale.y;
}

float get_ground_height_lod(float3 world_pos, float lod)
{
  float2 world_xz = world_pos.xz;
  float2 dist2 = abs(world_xz - distant_heightmap_target_box.xy) - distant_heightmap_target_box.zw;
  float dist = max(dist2.x, dist2.y);

  BRANCH
  if (dist > 0)
    return sample_distant_heightmap(world_pos.xz);
  else
    return sample_heightmap(world_pos.xz, lod);
}

struct BiomeData
{
  int4 indices;
  float4 weights;
};

// TODO: use common code with hardcoded version (biomes.sh)
BiomeData getBiomeData(float3 world_pos)
{
  float2 tc = world_pos.xz * land_detail_mul_offset.xy + land_detail_mul_offset.zw;
  float2 tcCorner = tc * biome_indices_tex_size.xy - 0.5f;
  float2 tcCornerI = floor(tcCorner);
  float2 useDetailTC = (tcCornerI + 0.5f) * biome_indices_tex_size.zw;
  float4 bilWeights;
  bilWeights.xy = tcCorner - tcCornerI;
  bilWeights.zw = saturate(1 - bilWeights.xy);
  BiomeData result;
  result.indices = (int4)(biomeIndicesTex.GatherRed(biomeIndicesTex_samplerstate, useDetailTC) * 255 + 0.1f);
  result.weights = bilWeights.zxxz*bilWeights.yyww;
  return result;
}

float calcBiomeInfluence(BiomeData biome_data, int index)
{
  return dot(biome_data.indices == index, biome_data.weights);
}

// TODO: deprecate it (remove from levels first)
int4 getBiomeIndices(float3 world_pos)
{
  return getBiomeData(world_pos).indices;
}

// TODO: deprecate it (remove from levels first)
float getBiomeInfluence(float3 world_pos, int index)
{
  return calcBiomeInfluence(getBiomeData(world_pos), index);
}

float3 getWorldNormal(float3 worldPos)
{
  float2 uvLow  = worldPos.xz * world_to_hmap_low.xy + world_to_hmap_low.zw;
  float3 offset = float3(tex_hmap_inv_sizes.x, 0, tex_hmap_inv_sizes.y);
  float W = tex2Dlod(land_heightmap_tex, float4(uvLow - offset.xy,0,0)).x;
  float E = tex2Dlod(land_heightmap_tex, float4(uvLow + offset.xy,0,0)).x;
  float N = tex2Dlod(land_heightmap_tex, float4(uvLow - offset.yz,0,0)).x;
  float S = tex2Dlod(land_heightmap_tex, float4(uvLow + offset.yz,0,0)).x;
  return normalize(half3(W-E, tex_hmap_inv_sizes.x/world_to_hmap_low.x/heightmap_scale.x, N-S));
}

float3 lerp_view_vec(float2 tc)
{
  return lerp(lerp(view_vecLT, view_vecRT, tc.x), lerp(view_vecLB, view_vecRB, tc.x), tc.y).xyz;
}

float linearize_z(float rawDepth, float2 decode_depth)
{
  return rcp(decode_depth.x + decode_depth.y * rawDepth);
}

float decode_depth_above(float depthHt)
{
  return depthHt * depth_ao_heights.x + depth_ao_heights.y;
}
float sample_spheres_density(StructuredBuffer<float4> spheres, int spheres_count, float3 world_pos)
{
  if (IS_DISTANT_FOG)
    return 0;

  float density = 0;
  for (int i = 0; i < spheres_count; ++i)
  {
    float4 sphere = spheres[2 * i];
    float2 densityParams = spheres[2 * i + 1].xy;
    float dist = distance(sphere.xyz, world_pos);
    float radiusInv = sphere.w;
    float weight = dist * radiusInv;
    FLATTEN if (weight <= 1)
      density += lerp(densityParams.x, densityParams.y, weight);
  }
  return density;
}

float2 getPrevTc(float3 world_pos, out float3 prev_clipSpace)
{
  // TODO: very ugly and subpotimal
  float4x4 prev_globtm_psf;
  prev_globtm_psf._m00_m10_m20_m30 = prev_globtm_psf_0;
  prev_globtm_psf._m01_m11_m21_m31 = prev_globtm_psf_1;
  prev_globtm_psf._m02_m12_m22_m32 = prev_globtm_psf_2;
  prev_globtm_psf._m03_m13_m23_m33 = prev_globtm_psf_3;
  float4 lastClipSpacePos = mul(float4(world_pos, 1), prev_globtm_psf);
  prev_clipSpace = lastClipSpacePos.w < 0.0001 ? float3(2,2,2) : lastClipSpacePos.xyz/lastClipSpacePos.w;
  return prev_clipSpace.xy*float2(0.5,-0.5) + float2(0.5,0.5);
}
float2 getPrevTc(float3 world_pos)
{
  float3 prevClip;
  return getPrevTc(world_pos, prevClip);
}
bool checkOffscreenTc3d(float3 screen_tc)
{
  return any(abs(screen_tc - 0.5) >= 0.4999);
}
bool checkOffscreenTc2d(float2 screen_tc)
{
  return checkOffscreenTc3d(float3(screen_tc, 0.5));
}

float4 get_transformed_zn_zfar()
{
  return float4(zn_zfar.x, zn_zfar.y, 1.0 / zn_zfar.y, (zn_zfar.y-zn_zfar.x)/(zn_zfar.x * zn_zfar.y));
}

float4 texelFetch(Texture2D<float4> a, int2 tc, int lod) { return a.Load(int3(tc, lod)); }
float3 texelFetch(Texture2D<float3> a, int2 tc, int lod) { return a.Load(int3(tc, lod)); }
float2 texelFetch(Texture2D<float2> a, int2 tc, int lod) { return a.Load(int3(tc, lod)); }
float  texelFetch(Texture2D<float>  a, int2 tc, int lod) { return a.Load(int3(tc, lod)); }
float4 texelFetch(Texture2DArray<float4> a, int3 tc, int lod) { return a.Load(int4(tc, lod)); }
float3 texelFetch(Texture2DArray<float3> a, int3 tc, int lod) { return a.Load(int4(tc, lod)); }
float2 texelFetch(Texture2DArray<float2> a, int3 tc, int lod) { return a.Load(int4(tc, lod)); }
float  texelFetch(Texture2DArray<float>  a, int3 tc, int lod) { return a.Load(int4(tc, lod)); }
float4 texelFetch(Texture3D<float4> a, int3 tc, int lod) { return a.Load(int4(tc, lod)); }
float3 texelFetch(Texture3D<float3> a, int3 tc, int lod) { return a.Load(int4(tc, lod)); }
float2 texelFetch(Texture3D<float2> a, int3 tc, int lod) { return a.Load(int4(tc, lod)); }
float  texelFetch(Texture3D<float>  a, int3 tc, int lod) { return a.Load(int4(tc, lod)); }
