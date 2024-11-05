// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "stdio.h"
#include "math.h"
#include <math/dag_mathBase.h>
#include <memory/dag_mem.h>
#include <util/dag_globDef.h>
#include <string.h>
#include <vecmath/dag_vecMath.h>
#include <supp/dag_prefetch.h>

#ifdef _MSC_VER
#pragma optimize("gt", on)
#endif

namespace imagefunctions
{

#define PRECISE_PI float(3.1415926535897932384626433833)

namespace
{
// Sinc function.
#define NV_EPSILON (0.0001f)
inline static float sincf(const float x)
{
  if (fabs(x) < NV_EPSILON)
    // return 1.0;
    return 1.0f + x * x * (-1.0f / 6.0f + x * x * 1.0f / 120.0f);
  else
  {
    return sin(x) / x;
  }
}

// Bessel function of the first kind from Jon Blow's article.
// http://mathworld.wolfram.com/BesselFunctionoftheFirstKind.html
// http://en.wikipedia.org/wiki/Bessel_function
inline static float bessel0(float x)
{
  const float EPSILON_RATIO = 1e-6f;
  float xh, sum, pow, ds;
  int k;

  xh = 0.5f * x;
  sum = 1.0f;
  pow = 1.0f;
  k = 0;
  ds = 1.0;
  while (ds > sum * EPSILON_RATIO)
  {
    ++k;
    pow = pow * (xh / k);
    ds = pow * pow;
    sum = sum + ds;
  }

  return sum;
}

} // namespace

class KaiserFilter
{
public:
  KaiserFilter() { setParameters(4.0f, 1.0f); }

  float sampleBox(float x, float scale, int samples) const
  {
    float sum = 0;
    float isamples = 1.0f / float(samples);

    for (int s = 0; s < samples; s++)
    {
      float p = (x + (float(s) + 0.5f) * isamples) * scale;
      float value = evaluate(p);
      sum += value;
    }

    return sum * isamples;
  }

  void setParameters(float alpha_, float stretch_)
  {
    alpha = alpha_;
    stretch = stretch_;
    bessel0aplha = bessel0(alpha);
  }

  float evaluate(float x) const
  {
    const float sinc_value = sincf(PRECISE_PI * x * stretch);
    const float t = x / 3.0f;
    if ((1 - t * t) >= 0)
      return sinc_value * bessel0(alpha * sqrtf(1 - t * t)) / bessel0aplha;
    else
      return 0;
  }

protected:
  float alpha, bessel0aplha;
  float stretch;
};

void prepare_kaiser_kernel(float alpha, float stretch, float *m_data)
{

  KaiserFilter f;
  f.setParameters(alpha, stretch);
  int m_windowSize = 12;

  float total = 0.0f;
  for (int j = 0; j < m_windowSize; j++)
  {
    float sample = f.sampleBox(j - 6.f, 0.5f, 32);
    m_data[j] = sample;
    // m_data[j*4+0] = m_data[j*4+1] = m_data[j*4+2] = m_data[j*4+3] = sample;
    //((__m128*)m_data)[j] = _mm_set_ps1(sample);
    //_mm_store_ps(m_data+j*4, _mm_set_ps1(sample));
    total += sample;
  }

  // normalize weights.
  for (int j = 0; j < m_windowSize; j++)
    m_data[j] /= total;
}

struct ClampIndexHorizontal
{
  int w, h, val;
  ClampIndexHorizontal(int w_, int h_, int v) : w(w_ - 1), h(h_), val(v * w_) {}
  int index(int at) const { return clamp(at, 0, w) + val; }
  __forceinline int offset() const { return 1; }
};

/// Apply 1D vertical kernel at the given coordinates and return result.
template <class T>
static void applyKernel(const T &indexData, const float *__restrict data, const float *__restrict kernel, int length,
  float *__restrict output, int stride)
{
  PREFETCH_DATA(0, data);
  PREFETCH_DATA(256, data);
  vec4f kreg0 = ((const vec4f *)kernel)[0];
  vec4f kreg1 = ((const vec4f *)kernel)[1];
  vec4f kreg2 = ((const vec4f *)kernel)[2];

  vec4f *out = (vec4f *)output;
  for (int i = 0; i < min(3, length); i++, out += stride)
  {
    int left = i * 2 + 1 - 6;
    vec4f data00, data01, data02, data03;
    vec4f sum = v_zero();
#define SUMM_ADD(ofs)                                                                          \
  data00 = v_ld(data + indexData.index(left + ofs * 4 + 0) * 4);                               \
  data01 = v_ld(data + indexData.index(left + ofs * 4 + 1) * 4);                               \
  data02 = v_ld(data + indexData.index(left + ofs * 4 + 2) * 4);                               \
  data03 = v_ld(data + indexData.index(left + ofs * 4 + 3) * 4);                               \
  sum = ofs ? v_madd(v_splat_x(kreg##ofs), data00, sum) : v_mul(v_splat_x(kreg##ofs), data00); \
  sum = v_madd(v_splat_y(kreg##ofs), data01, sum);                                             \
  sum = v_madd(v_splat_z(kreg##ofs), data02, sum);                                             \
  sum = v_madd(v_splat_w(kreg##ofs), data03, sum);
    SUMM_ADD(0);
    SUMM_ADD(1);
    SUMM_ADD(2);
#undef SUMM_ADD

    *out = sum;
  }
  if (length <= 3)
    return;

  const float *inData = data + indexData.index(1) * 4;
  for (int i = 3; i < length - 3; i++, out += stride, inData += 8)
  {
    PREFETCH_DATA(256, inData);
    vec4f data00, data01, data02, data03;
    vec4f sum = v_zero(), sum1, sum2;
#define SUMM_ADD(ofs)                                            \
  data00 = v_ld(inData + ((ofs * 4 + 0)) * 4);                   \
  data01 = v_ld(inData + ((ofs * 4 + 1)) * 4);                   \
  data02 = v_ld(inData + ((ofs * 4 + 2)) * 4);                   \
  data03 = v_ld(inData + ((ofs * 4 + 3)) * 4);                   \
  sum1 = v_mul(v_splat_x(kreg##ofs), data00);                    \
  sum2 = v_mul(v_splat_y(kreg##ofs), data01);                    \
  sum = ofs ? v_add(v_add(sum1, sum2), sum) : v_add(sum1, sum2); \
  sum1 = v_mul(v_splat_z(kreg##ofs), data02);                    \
  sum2 = v_mul(v_splat_w(kreg##ofs), data03);                    \
  sum = v_add(v_add(sum1, sum2), sum);
    SUMM_ADD(0);
    SUMM_ADD(1);
    SUMM_ADD(2);
#undef SUMM_ADD

    *out = sum;
  }
  for (int i = max(3, length - 3); i < length; i++, out += stride)
  {
    int left = i * 2 + 1 - 6;
    vec4f data00, data01, data02, data03;
    vec4f sum = v_zero();
#define SUMM_ADD(ofs)                                                                          \
  data00 = v_ld(data + indexData.index(left + ofs * 4 + 0) * 4);                               \
  data01 = v_ld(data + indexData.index(left + ofs * 4 + 1) * 4);                               \
  data02 = v_ld(data + indexData.index(left + ofs * 4 + 2) * 4);                               \
  data03 = v_ld(data + indexData.index(left + ofs * 4 + 3) * 4);                               \
  sum = ofs ? v_madd(v_splat_x(kreg##ofs), data00, sum) : v_mul(v_splat_x(kreg##ofs), data00); \
  sum = v_madd(v_splat_y(kreg##ofs), data01, sum);                                             \
  sum = v_madd(v_splat_z(kreg##ofs), data02, sum);                                             \
  sum = v_madd(v_splat_w(kreg##ofs), data03, sum);
    SUMM_ADD(0);
    SUMM_ADD(1);
    SUMM_ADD(2);
#undef SUMM_ADD

    *out = sum;
  }
  // G_ASSERT(out == ((vec4f*)output) + length);
}

void *kaiser_allocate_tmp_float_image(int w, int h) { return memalloc(w * h * 2 * 4 * sizeof(float), tmpmem); }

void *kaiser_allocate_tmp_char_image(int w, int h) { return memalloc(w * h * 2 * 4, tmpmem); }

void kaiser_free_tmp_image(void *image) { memfree(image, tmpmem); }

void kaiser_downsample(int w, int h, const float *src_image, float *dst_image, float *kernel, void *tmp_image)
{
  int srcH = h * 2, srcW = w * 2;
  float *dest = (float *)tmp_image;
  // rotate image w<>h, so we can use horizontal filters
  for (int y = 0; y < srcH; y++, dest += 4)
    applyKernel(ClampIndexHorizontal(srcW, srcH, y), src_image, kernel, w, dest, srcH);

  dest = dst_image;
  // rotate image w<>h again
  for (int x = 0; x < w; x++, dest += 4)
    applyKernel(ClampIndexHorizontal(srcH, w, x), (float *)tmp_image, kernel, h, dest, w);
}
#if _TARGET_SIMD_SSE
alignas(16) static const float SSE2_float_inv_255[4] = {1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f};

#define VALINV255 (*(vec4f *)SSE2_float_inv_255)
#define VAL255    (*(vec4f *)SSE2_float_255)
#define ZEROI     V_CI_MASK0000

__forceinline vec4f from_char_tofloat(vec4f, const unsigned char *data)
{
  __m128i src07i = _mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(*(const int *)data), ZEROI), ZEROI);
  return _mm_mul_ps(_mm_cvtepi32_ps(src07i), VALINV255);
}

__forceinline void from_char_tofloat(vec4f, const unsigned char *data, vec4f &a, vec4f &b, vec4f &c, vec4f &d)
{
  __m128i src0Fi = _mm_loadu_si128((const __m128i *)data);
  __m128i src07i = _mm_unpacklo_epi8(src0Fi, ZEROI);
  __m128i src7Fi = _mm_unpackhi_epi8(src0Fi, ZEROI);
  __m128i src03i = _mm_unpacklo_epi16(src07i, ZEROI);
  __m128i src37i = _mm_unpackhi_epi16(src07i, ZEROI);
  __m128i src7Ci = _mm_unpacklo_epi16(src7Fi, ZEROI);
  __m128i srcCFi = _mm_unpackhi_epi16(src7Fi, ZEROI);
  a = _mm_mul_ps(_mm_cvtepi32_ps(src03i), VALINV255);
  b = _mm_mul_ps(_mm_cvtepi32_ps(src37i), VALINV255);
  c = _mm_mul_ps(_mm_cvtepi32_ps(src7Ci), VALINV255);
  d = _mm_mul_ps(_mm_cvtepi32_ps(srcCFi), VALINV255);
}

__forceinline unsigned from_float_tochar(vec4f val255, vec4f val)
{
  __m128i src03i = _mm_cvtps_epi32(v_mul(val, val255));
  __m128i result = _mm_packus_epi16(_mm_packs_epi32(src03i, src03i), src03i);
  return (unsigned)_mm_cvtsi128_si32(result);
}
#undef VALINV255
#undef VAL255
#undef ZEROI
#else

__forceinline void from_char_tofloat(vec4f valinv255, const unsigned char *data, vec4f &a, vec4f &b, vec4f &c, vec4f &d)
{
  a = v_mul(v_make_vec4f(data[0], data[1], data[2], data[3]), valinv255);
  b = v_mul(v_make_vec4f(data[4], data[5], data[6], data[7]), valinv255);
  c = v_mul(v_make_vec4f(data[8], data[9], data[10], data[11]), valinv255);
  d = v_mul(v_make_vec4f(data[12], data[13], data[14], data[15]), valinv255);
}
__forceinline vec4f from_char_tofloat(vec4f valinv255, const unsigned char *data)
{
  return v_mul(v_make_vec4f(data[0], data[1], data[2], data[3]), valinv255);
}

__forceinline unsigned from_float_tochar(vec4f val255, vec4f val)
{
  vec4i v = v_cvt_vec4i(v_round(v_min(val255, v_mul(val255, v_max(v_zero(), val)))));
  struct RetI
  {
    int x[4];
  };
  alignas(16) RetI ret;
  v_sti(ret.x, v);
  return ret.x[0] | (ret.x[1] << 8) | (ret.x[2] << 16) | (ret.x[3] << 24);
}

#endif

template <class T>
static void applyKernelHor(const T &indexData, const unsigned char *__restrict data, const float *__restrict kernel, int length,
  unsigned char *__restrict output, unsigned stride)
{
  vec4f val255 = v_splats(255.0f);
  vec4f valinv255 = v_splats(1.0f / 255.0f);
  vec4f kreg0 = ((const vec4f *)kernel)[0];
  vec4f kreg1 = ((const vec4f *)kernel)[1];
  vec4f kreg2 = ((const vec4f *)kernel)[2];

  unsigned *out = (unsigned *)output;
  for (int i = 0; i < min(3, length); i++, out += stride)
  {
    int left = i * 2 + 1 - 6;
    vec4f data00, data01, data02, data03;
    vec4f sum = v_zero();
#define SUMM_ADD(ofs)                                                                          \
  data00 = from_char_tofloat(valinv255, data + indexData.index(left + ofs * 4 + 0) * 4);       \
  data01 = from_char_tofloat(valinv255, data + indexData.index(left + ofs * 4 + 1) * 4);       \
  data02 = from_char_tofloat(valinv255, data + indexData.index(left + ofs * 4 + 2) * 4);       \
  data03 = from_char_tofloat(valinv255, data + indexData.index(left + ofs * 4 + 3) * 4);       \
  sum = ofs ? v_madd(v_splat_x(kreg##ofs), data00, sum) : v_mul(v_splat_x(kreg##ofs), data00); \
  sum = v_madd(v_splat_y(kreg##ofs), data01, sum);                                             \
  sum = v_madd(v_splat_z(kreg##ofs), data02, sum);                                             \
  sum = v_madd(v_splat_w(kreg##ofs), data03, sum);
    SUMM_ADD(0);
    SUMM_ADD(1);
    SUMM_ADD(2);
#undef SUMM_ADD

    *out = from_float_tochar(val255, sum);
  }
  if (length <= 3)
    return;

  const unsigned char *inData = data + indexData.index(1) * 4;
  for (int i = 3; i < length - 3; i++, out += stride, inData += 8)
  {
    vec4f data00, data01, data02, data03;
    vec4f sum = v_zero();
#define SUMM_ADD(ofs)                                                                          \
  from_char_tofloat(valinv255, inData + ofs * 16, data00, data01, data02, data03);             \
  sum = ofs ? v_madd(v_splat_x(kreg##ofs), data00, sum) : v_mul(v_splat_x(kreg##ofs), data00); \
  sum = v_madd(v_splat_y(kreg##ofs), data01, sum);                                             \
  sum = v_madd(v_splat_z(kreg##ofs), data02, sum);                                             \
  sum = v_madd(v_splat_w(kreg##ofs), data03, sum);
    SUMM_ADD(0);
    SUMM_ADD(1);
    SUMM_ADD(2);
#undef SUMM_ADD

    *out = from_float_tochar(val255, sum);
  }
  for (int i = max(3, length - 3); i < length; i++, out += stride)
  {
    int left = i * 2 + 1 - 6;
    vec4f data00, data01, data02, data03;
    vec4f sum = v_zero();
#define SUMM_ADD(ofs)                                                                          \
  data00 = from_char_tofloat(valinv255, data + indexData.index(left + ofs * 4 + 0) * 4);       \
  data01 = from_char_tofloat(valinv255, data + indexData.index(left + ofs * 4 + 1) * 4);       \
  data02 = from_char_tofloat(valinv255, data + indexData.index(left + ofs * 4 + 2) * 4);       \
  data03 = from_char_tofloat(valinv255, data + indexData.index(left + ofs * 4 + 3) * 4);       \
  sum = ofs ? v_madd(v_splat_x(kreg##ofs), data00, sum) : v_mul(v_splat_x(kreg##ofs), data00); \
  sum = v_madd(v_splat_y(kreg##ofs), data01, sum);                                             \
  sum = v_madd(v_splat_z(kreg##ofs), data02, sum);                                             \
  sum = v_madd(v_splat_w(kreg##ofs), data03, sum);
    SUMM_ADD(0);
    SUMM_ADD(1);
    SUMM_ADD(2);
#undef SUMM_ADD
    *out = from_float_tochar(val255, sum);
  }
}


void kaiser_downsample(int w, int h, const unsigned char *src_image, unsigned char *dst_image, float *kernel, void *tmp_image)
{
  // @@ Use monophase filters when frac(m_width / w) == 0
  // rotate image w<>h, so we can use horizontal filters
  int srcH = h * 2, srcW = w * 2;
  unsigned char *dest = (unsigned char *)tmp_image;
  for (int y = 0; y < srcH; y++, dest += 4)
    applyKernelHor(ClampIndexHorizontal(srcW, srcH, y), src_image, kernel, w, dest, srcH);
  // rotate image w<>h, so we can use horizontal filters
  dest = dst_image;
  for (int x = 0; x < w; x++, dest += 4)
    applyKernelHor(ClampIndexHorizontal(srcH, w, x), (unsigned char *)tmp_image, kernel, h, dest, w);
}

}; // namespace imagefunctions