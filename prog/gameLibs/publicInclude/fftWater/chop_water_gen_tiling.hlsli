#ifndef CHOP_WATER_GEN_TILING_HLSL
#define CHOP_WATER_GEN_TILING_HLSL

// ===================== HLSL ========================
#ifndef __cplusplus
float SampleXTiled(float2 uv, float overlap)
{
    float2 uv_0 = float2(uv.x * overlap,uv.y);
    float2 uv_1 = float2((uv.x + 1.0) * overlap,uv.y);

    float s0 = tex2Dlod(hm_wave_texture, float4(uv_0, 0, 0)).r; // smp_linear_wrap is assumed
    float s1 = tex2Dlod(hm_wave_texture, float4(uv_1, 0, 0)).r; // smp_linear_wrap is assumed

    return lerp(s0, s1, smoothstep(1.0 - overlap, 0.0, uv.x));
}

float SampleLinearWrapTiled(float2 uv, float overlap)
{
    float2 uv_0 = float2(uv.x, uv.y * overlap);
    float2 uv_1 = float2(uv.x, (uv.y + 1.0) * overlap);

    float s0 = SampleXTiled(uv_0, overlap);
    float s1 = SampleXTiled(uv_1, overlap);

    return lerp(s0, s1, smoothstep(1.0 - overlap, 0.0, uv.y));
}
#else
// ===================== C++ ========================
static inline float frac01(float v)
{
  int i = (int)v;
  i -= (i > v);
  return v - (float)i;
}

static inline float smooth01(float e0, float e1, float x)
{
  float t = (x - e0) / (e1 - e0);
  t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
  return t * t * (3.0f - 2.0f * t);
}

static inline int fast_floor_to_int(float v)
{
  int i = (int)v;
  i -= (i > v);
  return i;
}

static inline float SampleFromFrac(float u_frac, float v_frac, float *m_data, int32_t m_res_x,
    int32_t m_mask_x, int32_t m_mask_y, float m_res_x_f, float m_res_y_f)
{
  float fx = u_frac * m_res_x_f - 0.5f;
  float fy = v_frac * m_res_y_f - 0.5f;

  int32_t ix0 = fast_floor_to_int(fx);
  int32_t iy0 = fast_floor_to_int(fy);
  int32_t ix1 = ix0 + 1;
  int32_t iy1 = iy0 + 1;

  float wx = fx - (float)ix0;
  float wy = fy - (float)iy0;

  int32_t x0 = ix0 & m_mask_x;
  int32_t y0 = iy0 & m_mask_y;
  int32_t x1 = ix1 & m_mask_x;
  int32_t y1 = iy1 & m_mask_y;

  const int row0 = y0 * m_res_x;
  const int row1 = y1 * m_res_x;
  const float t00 = m_data[row0 + x0];
  const float t10 = m_data[row0 + x1];
  const float t01 = m_data[row1 + x0];
  const float t11 = m_data[row1 + x1];

  float a0 = t00 + wx * (t10 - t00);
  float a1 = t01 + wx * (t11 - t01);
  return a0 + wy * (a1 - a0);
}

static inline float sample_linear_wrap_tiled_impl(const Point2 &uv,
    float overlap,
    float *data,
    int32_t res_x,
    int32_t mask_x,
    int32_t mask_y,
    float res_x_f,
    float res_y_f)
{
  const float inv_overlap = 1.0f - overlap;
  const float tx = 1.0f - smooth01(0.0f, inv_overlap, uv.x);
  const float ty = 1.0f - smooth01(0.0f, inv_overlap, uv.y);

  float ux0 = frac01(uv.x * overlap);
  float uy0 = frac01(uv.y * overlap);

  float ux1 = ux0 + overlap;
  if (ux1 >= 1.0f)
    ux1 -= 1.0f;
  float uy1 = uy0 + overlap;
  if (uy1 >= 1.0f)
    uy1 -= 1.0f;

  const float s00 = SampleFromFrac(ux0, uy0, data, res_x, mask_x, mask_y, res_x_f, res_y_f);
  const float s10 = SampleFromFrac(ux1, uy0, data, res_x, mask_x, mask_y, res_x_f, res_y_f);
  const float s01 = SampleFromFrac(ux0, uy1, data, res_x, mask_x, mask_y, res_x_f, res_y_f);
  const float s11 = SampleFromFrac(ux1, uy1, data, res_x, mask_x, mask_y, res_x_f, res_y_f);

  const float sx0 = s00 + tx * (s10 - s00);
  const float sx1 = s01 + tx * (s11 - s01);
  return sx0 + ty * (sx1 - sx0);
}

#include <fftWater/chop_water_defines.hlsli>

#ifdef CHOP_WATER_USE_SSE
static inline void sample_linear_wrap_tiled4_impl(float u0,
                                                  float u1,
                                                  float u2,
                                                  float u3,
                                                  float v,
                                                  float overlap,
                                                  float out[4],
                                                  float *data,
                                                  int32_t res_x,
                                                  int32_t mask_x,
                                                  int32_t mask_y,
                                                  float res_x_f,
                                                  float res_y_f)
{
  const float inv_overlap = 1.0f - overlap;
  const float ty = 1.0f - smooth01(0.0f, inv_overlap, v);

  float txs[4];
  txs[0] = 1.0f - smooth01(0.0f, inv_overlap, u0);
  txs[1] = 1.0f - smooth01(0.0f, inv_overlap, u1);
  txs[2] = 1.0f - smooth01(0.0f, inv_overlap, u2);
  txs[3] = 1.0f - smooth01(0.0f, inv_overlap, u3);

  float ux0[4] = {frac01(u0 * overlap), frac01(u1 * overlap), frac01(u2 * overlap), frac01(u3 * overlap)};
  float uy0 = frac01(v * overlap);
  float ux1[4] = {ux0[0] + overlap, ux0[1] + overlap, ux0[2] + overlap, ux0[3] + overlap};
  float uy1 = uy0 + overlap;
  if (uy1 >= 1.0f)
    uy1 -= 1.0f;
  for (int i = 0; i < 4; ++i)
    if (ux1[i] >= 1.0f)
      ux1[i] -= 1.0f;

  float s00[4], s10[4], s01[4], s11[4];
  for (int i = 0; i < 4; ++i)
  {
    s00[i] = SampleFromFrac(ux0[i], uy0, data, res_x, mask_x, mask_y, res_x_f, res_y_f);
    s10[i] = SampleFromFrac(ux1[i], uy0, data, res_x, mask_x, mask_y, res_x_f, res_y_f);
    s01[i] = SampleFromFrac(ux0[i], uy1, data, res_x, mask_x, mask_y, res_x_f, res_y_f);
    s11[i] = SampleFromFrac(ux1[i], uy1, data, res_x, mask_x, mask_y, res_x_f, res_y_f);
  }

  vec4f vtx = v_ldu(txs);
  vec4f vs00 = v_ldu(s00);
  vec4f vs10 = v_ldu(s10);
  vec4f vs01 = v_ldu(s01);
  vec4f vs11 = v_ldu(s11);

  vec4f sx0 = v_add(vs00, v_mul(vtx, v_sub(vs10, vs00)));
  vec4f sx1 = v_add(vs01, v_mul(vtx, v_sub(vs11, vs01)));
  vec4f vty = v_splats(ty);
  vec4f vres = v_add(sx0, v_mul(vty, v_sub(sx1, sx0)));
  v_stu(out, vres);
}
#endif // CHOP_WATER_USE_SSE
#endif // __cplusplus

#endif // CHOP_WATER_GEN_TILING_HLSL