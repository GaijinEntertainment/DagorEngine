#pragma once
#include <util/dag_globDef.h>
#include <math/dag_Point4.h>
#include <vecmath/dag_vecMath.h>

__forceinline Point3_vec4 get_3_float(const uint16_t *__restrict in)
{
  // #if _TARGET_SIMD_SSE
  DECL_ALIGN16(Point3_vec4, result2);
  v_st(&result2.x, v_half_to_float(in));
  return result2;
  // #else
  //   Point3_vec4 result;
  //   result.x = float32(in[0]);
  //   result.y = float32(in[1]);
  //   result.z = float32(in[2]);
  //   return result;
  // #endif
}
__forceinline vec4f get_float4_u16(const uint16_t *__restrict in, vec4f scale, vec4f ofs)
{
  return v_madd(v_cvt_vec4f(v_lduush(in)), scale, ofs);
}

__forceinline vec4f get_float4_u16(const uint16_t *__restrict in, vec4f scaleOfs)
{
  return v_madd(v_cvt_vec4f(v_lduush(in)), scaleOfs, v_splat_w(scaleOfs));
}

__forceinline Point3_vec4 get_displacement_half(int dmap_dim, float size, const Point2 &in, const uint16_t *__restrict data)
{
  const float f_dmap_dim = float(dmap_dim);
  const float uv_scale = f_dmap_dim / size;

  // Calculate the UV coords, in texels
  const Point2 uv = in * uv_scale - Point2(0.5f, 0.5f);
  Point2 uv_wrap = Point2(fmodf(uv.x, f_dmap_dim), fmodf(uv.y, f_dmap_dim));
  if (uv_wrap.x < 0.f)
    uv_wrap.x += f_dmap_dim;
  if (uv_wrap.y < 0.f)
    uv_wrap.y += f_dmap_dim;

  const Point2 uv_round(floorf(uv_wrap.x), floorf(uv_wrap.y));
  const Point2 uv_frac = uv_wrap - uv_round;

  const int uv_x = (int)uv_round.x;
  const int uv_y = (int)uv_round.y;
  const int uv_x_1 = (uv_x + 1) % dmap_dim;
  const int uv_y_1 = (uv_y + 1) % dmap_dim;

  // Ensure we wrap round during the lerp too
  const uint16_t *pTL = data + ((uv_y * dmap_dim) << 2);
  const uint16_t *pTR = pTL + (uv_x_1 << 2);
  pTL += uv_x << 2;
  const uint16_t *pBL = data + ((uv_y_1 * dmap_dim) << 2);
  const uint16_t *pBR = pBL + (uv_x_1 << 2);
  pBL += uv_x << 2;

  float fInvDx = 1.f - uv_frac.x;

  return (get_3_float(pTL) * fInvDx + get_3_float(pTR) * uv_frac.x) * (1.f - uv_frac.y) +
         (get_3_float(pBL) * fInvDx + get_3_float(pBR) * uv_frac.x) * uv_frac.y;
}

__forceinline vec4f get_displacement_u16_scalar(int bits, float size, const Point2 &in, const uint16_t *__restrict data, float maxWave)
{
  int dmap_dim = 1 << bits;
  const int dmap_mask = dmap_dim - 1;
  const float f_dmap_dim = float(dmap_dim);
  const float uv_scale = f_dmap_dim / size;
  vec4f scale = v_splats(maxWave * (2.f / 65535));
  vec4f ofs = v_splats(-maxWave);

  // Calculate the UV coords, in texels
  const Point2 uv = in * uv_scale - Point2(0.5f, 0.5f);
  Point2 uv_wrap = Point2(fmodf(uv.x, f_dmap_dim), fmodf(uv.y, f_dmap_dim));
  if (uv_wrap.x < 0.f)
    uv_wrap.x += f_dmap_dim;
  if (uv_wrap.y < 0.f)
    uv_wrap.y += f_dmap_dim;

  const Point2 uv_round(floorf(uv_wrap.x), floorf(uv_wrap.y));
  const Point2 uv_frac = uv_wrap - uv_round;
  vec4f fracPart = v_make_vec4f(uv_frac.x, uv_frac.x, uv_frac.x, uv_frac.y);

  int uv_x = ((int)uv_round.x);
  const int uv_y = (int)uv_round.y;
  const int uv_x_1 = 3 * ((uv_x + 1) & dmap_mask);
  uv_x *= 3;
  const int uv_y_1 = (uv_y + 1) & dmap_mask;

  // Ensure we wrap round during the lerp too
  dmap_dim = 3 << bits;
  const uint16_t *pTL = data + (uv_y * dmap_dim);
  const uint16_t *pTR = pTL + uv_x_1;
  pTL += uv_x;
  const uint16_t *pBL = data + (uv_y_1 * dmap_dim);
  const uint16_t *pBR = pBL + uv_x_1;
  pBL += uv_x;

  vec4f tl = get_float4_u16(pTL, scale, ofs), tr = get_float4_u16(pTR, scale, ofs);
  vec4f t = v_madd(v_sub(tr, tl), fracPart, tl);

  vec4f bl = get_float4_u16(pBL, scale, ofs), br = get_float4_u16(pBR, scale, ofs);
  vec4f b = v_madd(v_sub(br, bl), fracPart, bl);
  return v_madd(v_sub(b, t), v_splat_w(fracPart), t);
}

__forceinline vec4f get_bilinear_adr(int bits, float size, vec4f inV, int *adr)
{
  vec4i dmapi = v_splatsi(1 << bits);
  vec4f dmap = v_cvt_vec4f(dmapi);
  vec4f uv_wrap = v_div(inV, v_splats(size)); // v_rcp(size) - calc once!
// don't use a pixel centered bilinear filtration because it creates shifts for different
// fft resolutions (addresses issues of the convergence)
// uv_wrap = v_sub(uv_wrap, v_rcp(v_cvt_vec4f(v_slli(dmapi,1)))); // v_rcp(1<<(bits+1)) - calc once!
#if _TARGET_SIMD_SSE >= 4 || defined(__SSE4_1__)
  uv_wrap = v_mul(v_sub(uv_wrap, sse4_floor(uv_wrap)), dmap);
#else
  uv_wrap = v_mul(v_sub(uv_wrap, v_floor(uv_wrap)), dmap);
#endif
  vec4i uvIdx = v_cvt_vec4i(uv_wrap);
  vec4f uvFrac = v_sub(uv_wrap, v_cvt_vec4f(uvIdx));

  vec4i bitMask = v_splatsi((1 << bits) - 1);
  uvIdx = v_andi(bitMask, uvIdx); // It is actually needed due to floating point imprecision
  vec4i uvNext = v_andi(v_addi(uvIdx, V_CI_1), bitMask);
  uvIdx = v_cast_vec4i(v_perm_xyab(v_cast_vec4f(uvIdx), v_cast_vec4f(uvNext)));
  uvIdx = v_addi(v_cast_vec4i(v_perm_xzxz(v_cast_vec4f(uvIdx))), v_cast_vec4i(v_rot_2(v_perm_yyww(v_cast_vec4f(v_sll(uvIdx, bits))))));
  uvIdx = v_addi(uvIdx, v_slli(uvIdx, 1)); //*3
  uvIdx = v_slli(uvIdx, 1);                //*2
  v_sti(adr, uvIdx);
  return uvFrac;
}

__forceinline vec4f get_displacement_u16_bilinear(int *adr, vec4f uvFrac, const uint16_t *__restrict data, vec4f scaleOfs)
{
  const uint16_t *pTL = (const uint16_t *)((const uint8_t *)data + adr[2]);
  const uint16_t *pTR = (const uint16_t *)((const uint8_t *)data + adr[3]);
  const uint16_t *pBL = (const uint16_t *)((const uint8_t *)data + adr[0]);
  const uint16_t *pBR = (const uint16_t *)((const uint8_t *)data + adr[1]);
  // vec4f ofs = v_splat_w(scaleOfs);
  vec4f tl = get_float4_u16(pTL, scaleOfs), tr = get_float4_u16(pTR, scaleOfs);
  vec4f t = v_madd(v_sub(tr, tl), v_splat_x(uvFrac), tl);

  vec4f bl = get_float4_u16(pBL, scaleOfs), br = get_float4_u16(pBR, scaleOfs);
  vec4f b = v_madd(v_sub(br, bl), v_splat_x(uvFrac), bl);
  return v_madd(v_sub(b, t), v_splat_y(uvFrac), t);
}

__forceinline vec4f get_displacement_u16_fast(int bits, float size, const vec4f inV, const uint16_t *__restrict data, vec4f scaleOfs)
{
  DECL_ALIGN16(int, adr[4]);
  vec4f uvFrac = get_bilinear_adr(bits, size, inV, adr);
  return get_displacement_u16_bilinear(adr, uvFrac, data, scaleOfs);
}

__forceinline uint32_t get_nearest_adr(int bits, float size, vec4f inV)
{
  vec4i dmapi = v_splatsi(1 << bits);
  vec4f dmap = v_cvt_vec4f(dmapi);
  vec4f uv_wrap = v_sub(v_div(inV, v_splats(size)), v_rcp(v_cvt_vec4f(v_slli(dmapi, 1)))); // //v_rcp(size), v_rcp(1<<(bits+1)) - calc
                                                                                           // once!
  uv_wrap = v_mul(v_sub(uv_wrap, v_floor(uv_wrap)), dmap);
  vec4i uvIdx = v_cvt_vec4i(uv_wrap);
  return (v_extract_xi(uvIdx) + (v_extract_yi(uvIdx) << bits)) * 6;
}

__forceinline vec4f get_displacement_u16_nearest(uint32_t adr, const uint16_t *__restrict data, vec4f scaleOfs)
{
  const uint16_t *pTL = (const uint16_t *)((const uint8_t *)data + adr);
  return get_float4_u16(pTL, scaleOfs);
}
