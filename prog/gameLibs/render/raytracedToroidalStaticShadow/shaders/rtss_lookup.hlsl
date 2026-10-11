#ifndef RTSS_LOOKUP_HLSL
#define RTSS_LOOKUP_HLSL

#include <rtss_common.hlsli>
#include <pcg_hash.hlsl>

#define RTSS_NO_LEVEL 0xffffffffu

#ifndef RTSS_SCREEN_HARDEN
#define RTSS_SCREEN_HARDEN 0
#endif
#ifndef RTSS_SCREEN_TENT
#define RTSS_SCREEN_TENT 0
#endif
#ifndef RTSS_HW_COMPARE
#define RTSS_HW_COMPARE 1
#endif

struct RtssTap
{
  float3 ls;
  int2 pageRel;
  uint2 slot;
  uint bit, level;
  float zBase, invRange; // the depth range of the slot, as the atlas stores it
};

float rtss_atlas_z(RtssTap tap, float z) { return (z - tap.zBase) * tap.invRange; }

// p.w: 1 for a position relative to rtss_origin, 0 for a direction
float3 rtss_to_ls(uint level, uint bit, float4 p)
{
  uint row = bit * 3;
  return float3(dot(rtss_level_row(level, row), p), dot(rtss_level_row(level, row + 1), p), dot(rtss_level_row(level, row + 2), p));
}

float2 rtss_slot_pos(RtssTap tap)
{
  return tap.ls.xy / rtss_texel(tap.level) - float2(tap.pageRel * rtss_params_i.w) + ((rtss_params_i.z - rtss_params_i.w) >> 1);
}

// pos and z are affine in the offset
struct RtssPcf
{
  float2 pos; // uv, or atlas texels - 0.5 with gather
  float posPerLs;
  float z;    // biased atlas z of the tap
  float invRange;
  uint level;
};

RtssPcf rtss_pcf_base(RtssTap tap)
{
  RtssPcf b;
  uint S = uint(rtss_params_i.z);
  float texel = rtss_texel(tap.level);
  float2 atlasPos = float2(tap.slot * S) + rtss_slot_pos(tap);
#if RTSS_HW_COMPARE
  b.pos = atlasPos * rtss_atlas_texel;
  b.posPerLs = rtss_atlas_texel / texel;
#else
  b.pos = atlasPos - 0.5;
  b.posPerLs = rcp(texel);
#endif
  b.z = rtss_atlas_z(tap, tap.ls.z - rtss_bias.x * texel);
  b.invRange = tap.invRange;
  b.level = tap.level;
  return b;
}

// ls_axis: light space, within border - 0.5 texels
float3 rtss_pcf_axis(RtssPcf b, float3 ls_axis) { return ls_axis * float3(b.posPerLs.xx, b.invRange); }

half rtss_pcf2x2_at(RtssPcf b, float3 ofs_atlas)
{
  float ref = b.z + ofs_atlas.z;
#if RTSS_HW_COMPARE
  return half(rtss_atlas.SampleCmpLevelZero(rtss_cmp_sampler, float3(b.pos + ofs_atlas.xy, b.level), ref));
#else
  float2 atlasPos = b.pos + ofs_atlas.xy;
  float2 base = floor(atlasPos);
  float2 f = atlasPos - base;
  // gathered at the shared corner of the block of base, so the texels match f whatever the subtexel rounding
  float4 z = rtss_atlas.GatherRed(rtss_gather_sampler, float3(base * rtss_atlas_texel + rtss_atlas_texel, b.level));
  float4 lit = float4(ref.xxxx <= z); // LESS_EQUAL, as every driver compares
  return half(lerp(lerp(lit.w, lit.z, f.x), lerp(lit.x, lit.y, f.x), f.y));
#endif
}

// ofs: light space, within border - 0.5 texels, so the read stays in the slot of tap
half rtss_pcf2x2(RtssTap tap, float3 ofs)
{
  RtssPcf b = rtss_pcf_base(tap);
  return rtss_pcf2x2_at(b, rtss_pcf_axis(b, ofs));
}

float rtss_caster_z(RtssTap tap)
{
  return tap.zBase + rtss_atlas[uint3(tap.slot * uint(rtss_params_i.z) + uint2(rtss_slot_pos(tap)), tap.level)] / tap.invRange;
}

// RTSS_TAP_HIT, else the miss of the read that passed the most checks
uint rtss_find_tap(uint level, float3 pos, out RtssTap tap)
{
  tap.level = level;
  uint W = uint(rtss_params_i.y);
  uint4 window = rtss_level_window(level);
  uint bit = window.z & RTSS_LEVEL_NEWEST_BIT, miss = RTSS_TAP_RELEASED;
  for (uint i = 0; i < ((window.z & RTSS_LEVEL_SWEEPING) ? 2u : 1u); ++i, bit ^= 1)
  {
    tap.ls = rtss_to_ls(level, bit, float4(pos, 1));
    tap.pageRel = int2(floor(tap.ls.xy * asfloat(window.w)));
    tap.bit = bit;
    int2 page = int2(window.xy) + tap.pageRel;
    tap.slot = uint2(page) & (W - 1);
    uint4 s = rtss_slot((level * W + tap.slot.y) * W + tap.slot.x);
    float2 depth = asfloat(s.zw);
    bool samePage = (s.y & RTSS_SLOT_VALID) != 0 && s.x == rtss_pack_page(page);
    bool sameBit = ((s.y & RTSS_SLOT_BIT) != 0) == (bit != 0);
    tap.zBase = depth.x;
    tap.invRange = rcp(max(depth.y - depth.x, 1e-6));
    if (samePage && sameBit && tap.ls.z >= depth.x && tap.ls.z <= depth.y)
      return RTSS_TAP_HIT;
    miss = max(miss, !samePage ? RTSS_TAP_RELEASED : !sameBit ? RTSS_TAP_BIT : RTSS_TAP_DEPTH);
  }
  return miss;
}

float3 rtss_normal_offset_pos(float3 pos_rel, float3 world_normal, float lod)
{
  float3 d = rtss_level_row(0, uint(rtss_params.z) * 3 + 2).xyz;
  float NoL = saturate(dot(world_normal, -d));
  return pos_rel + world_normal * (rtss_bias.y * rtss_texel0 * exp2(max(lod, 0.0)) * (1 - NoL));
}

float rtss_dither(float3 world_pos) { return pcg3d_hash_16bit(asuint(world_pos)).x * (1.0 / 65536); }

// in the seam band a dither selects the next level; the band of the coarsest level, in its own sy, fades to lit with no dither;
// lod: log2 of the light space distance in level 0 extents, with y at the smallest sy of the live transforms of all levels
uint rtss_select_level(float3 pos_rel, float dither, out float lod, out float fade)
{
  float3 fromCenter = pos_rel - float3(0, rtss_center_y, 0);
  float3 dc = rtss_to_ls(0, uint(rtss_params.z), float4(fromCenter, 0));
  float m = max(max(max(abs(dc.x), abs(dc.y) * rtss_select_y), abs(dc.z)) * rtss_params.x, 1e-6);
  lod = log2(m);
  uint first = uint(max(0, int(ceil(lod)))), last = uint(max(rtss_params_i.x - 1, 0));
  float band = saturate((m * asfloat((127u - first) << 23) - 1) * rtss_params.w + 1); // read with first < last alone
  float3 dl = abs(rtss_to_ls(last, rtss_level_window(last).z & RTSS_LEVEL_NEWEST_BIT, float4(fromCenter, 0)));
  float ml = max(max(dl.x, dl.y), dl.z) * rtss_params.x * asfloat((127u - last) << 23);
  fade = first >= last ? saturate((ml - 1) * rtss_params.w + 1) : 0;
  return first < last && dither < band ? first + 1 : min(first, last);
}

// RTSS_NO_LEVEL when no level from the selected one on has data at the normal offset position
uint rtss_find_level(float3 pos_rel, float3 world_normal, float dither, out float lod, out RtssTap tap, out float fade)
{
  // constraint: fxc rejects an output read it hoists above the hit test, and the loop can run zero times
  tap = (RtssTap)0;
  lod = fade = 0;
  uint k = rtss_select_level(pos_rel, dither, lod, fade);
  float3 pos = rtss_normal_offset_pos(pos_rel, world_normal, lod);
  LOOP
  for (; k < uint(rtss_params_i.x); ++k)
  {
    if (rtss_find_tap(k, pos, tap) == RTSS_TAP_HIT)
      return k;
  }
  return RTSS_NO_LEVEL;
}

// RTSS_NO_LEVEL with 0 below the horizon and with 1 when no level has data
half rtss_lookup_rel(float3 pos_rel, float3 world_normal, float dither, out uint level, out RtssTap tap)
{
  level = RTSS_NO_LEVEL;
  if (rtss_params.y != 0)
    return 0;
  float lod, fade;
  level = rtss_find_level(pos_rel, world_normal, dither, lod, tap, fade);
  if (level == RTSS_NO_LEVEL)
    return 1;
  return lerp(rtss_pcf2x2(tap, 0), 1, fade);
}

// false when no level has data and the sun is above the horizon
bool rtss_sample_rel(float3 pos_rel, float3 world_normal, float dither, out half visibility, out uint level)
{
  RtssTap tap;
  visibility = rtss_lookup_rel(pos_rel, world_normal, dither, level, tap);
  return level != RTSS_NO_LEVEL || rtss_params.y != 0;
}

// 1 (lit) when no level has data, 0 when the sun is below the horizon
half rtss_sun_visibility_rel(float3 pos_rel, float3 world_normal, float dither)
{
  uint level;
  RtssTap tap;
  return rtss_lookup_rel(pos_rel, world_normal, dither, level, tap);
}

half rtss_sun_visibility(float3 world_pos, float3 world_normal, float dither)
{
  return rtss_sun_visibility_rel(world_pos - rtss_origin, world_normal, dither);
}

#if !RTSS_SCREEN_TENT
#include <noise/poisson_disc_8.hlsl>
#endif

half rtss_screen_filter(RtssTap center, float3 world_normal, float dither, float radius_scale, float2 harden, float lod)
{
  uint level = center.level, row = center.bit * 3;
  float texel = rtss_texel(level);
  float maxOfs = (((rtss_params_i.z - rtss_params_i.w) >> 1) - 0.5) * texel;
  float grow = exp2(max(max(lod, 0.0) - float(level), -1.0));
  float3 r0 = rtss_level_row(level, row).xyz, r1 = rtss_level_row(level, row + 1).xyz;
#if RTSS_SCREEN_TENT
  float3 n = float3(dot(world_normal, r0), dot(world_normal, r1) * rcp(dot(r1, r1)), dot(world_normal, rtss_level_row(level, row + 2).xyz));
  float2 slope = n.z != 0 ? clamp(-n.xy / n.z, -2, 2) : 0;
  float tapStep = min(min(radius_scale, 1) * texel, maxOfs) * grow;
  RtssPcf b = rtss_pcf_base(center);
  float3 ax = rtss_pcf_axis(b, float3(tapStep, 0, slope.x * tapStep)), ay = rtss_pcf_axis(b, float3(0, tapStep, slope.y * tapStep));
  half edges = rtss_pcf2x2_at(b, -ax) + rtss_pcf2x2_at(b, ax) + rtss_pcf2x2_at(b, -ay) + rtss_pcf2x2_at(b, ay);
  half corners =
    rtss_pcf2x2_at(b, -ax - ay) + rtss_pcf2x2_at(b, ax - ay) + rtss_pcf2x2_at(b, ay - ax) + rtss_pcf2x2_at(b, ax + ay);
  return (rtss_pcf2x2_at(b, 0) * 4 + edges * 2 + corners) * half(1.0 / 16);
#else
  float radiusTexels = 1.5;
  #if RTSS_SCREEN_HARDEN
  float distToCaster = center.ls.z - rtss_caster_z(center);
  distToCaster = distToCaster > 0 ? distToCaster : rcp(harden.x);
  float hardenK = saturate(distToCaster * harden.x);
  radiusTexels = lerp(0.5, radiusTexels, lerp(hardenK * hardenK, 1, harden.y));
  #endif
  float3 tangent = r0, bitangent = normalize(r1);
  if (dot(world_normal, world_normal) > 1e-6)
  {
    tangent = normalize(cross(abs(world_normal.y) > 0.5 ? float3(1, 0, 0) : float3(0, 1, 0), world_normal));
    bitangent = cross(tangent, world_normal);
  }
  float2 sc;
  sincos(6.2831853 * dither, sc.x, sc.y);
  float2x2 rotation = {sc.x, sc.y, -sc.y, sc.x};
  float2x3 axes = mul(rotation * (min(radiusTexels * radius_scale * texel, maxOfs) * grow), float2x3(tangent, bitangent));
  float3 ax = rtss_to_ls(level, center.bit, float4(axes[0], 0)), ay = rtss_to_ls(level, center.bit, float4(axes[1], 0));
  float scale = min(1, maxOfs * rsqrt(max(max(ax.x * ax.x + ay.x * ay.x, ax.y * ax.y + ay.y * ay.y), 1e-12)));
  RtssPcf b = rtss_pcf_base(center);
  ax = rtss_pcf_axis(b, ax * scale);
  ay = rtss_pcf_axis(b, ay * scale);
  half sum = 0;
  UNROLL
  for (uint i = 0; i < 8; ++i)
    sum += rtss_pcf2x2_at(b, ax * g_poisson_disc_8[i].x + ay * g_poisson_disc_8[i].y);
  return sum * half(1.0 / 8);
#endif
}

// 1 (lit) when no level has data, 0 when the sun is below the horizon; harden.x: 1 / full-blur meters, harden.y: relax
half rtss_sun_visibility_screen_rel(float3 pos_rel, float3 world_normal, float dither, float radius_scale, float2 harden)
{
  if (rtss_params.y != 0)
    return 0;
  float lod, fade;
  RtssTap center;
  // constraint: no ?: here, HLSL 2018 evaluates both sides, a filter of a missed tap with no level
  if (rtss_find_level(pos_rel, world_normal, frac(dither * 16), lod, center, fade) == RTSS_NO_LEVEL)
    return 1;
  return lerp(rtss_screen_filter(center, world_normal, dither, radius_scale, harden, lod), 1, fade);
}

#endif
