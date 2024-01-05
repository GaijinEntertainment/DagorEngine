#include <math/dag_imageFunctions.h>
#include <math.h>
#include <supp/dag_prefetch.h>
#include <math/dag_mathBase.h>
#include <vecmath/dag_vecMath.h>
namespace imagefunctions
{

alignas(16) static float global_gamma_lut[256];
alignas(16) static float global_linear[256];
static float global_gamma = 0.f;


static void init_global_lut(float power)
{
  global_gamma = power;
  for (int i = 1; i < 255; ++i)
    global_linear[i] = i / 255.0f;
  for (int i = 1; i < 255; ++i)
    global_gamma_lut[i] = powf(global_linear[i], power);
  global_linear[0] = global_gamma_lut[0] = 0.f;
  global_linear[255] = global_gamma_lut[255] = 1.f;
}

static struct InitGamma2_2
{
  InitGamma2_2() { init_global_lut(2.2f); }
} init_22;


static void preZero(uint8_t *, unsigned) {}

#ifdef _MSC_VER
#pragma optimize("gt", on)
#endif


template <typename t_type>
static inline t_type roundDownToPowerOf2(const t_type &t, unsigned int power_of_2)
{
  return (t_type)(((uintptr_t)t) & ~(power_of_2 - 1));
}

template <typename t_type>
static inline t_type roundUpToPowerOf2(const t_type &t, unsigned int power_of_2)
{
  return (t_type)((((uintptr_t)t) + (power_of_2 - 1)) & ~(power_of_2 - 1));
}

static __forceinline void prefetch(const unsigned char *data, unsigned int size)
{
  static const unsigned int cacheLineSize = 128;

  // Expand input range to be cache-line aligned:
  const unsigned char *end = data + size;
  data = roundDownToPowerOf2(data, cacheLineSize);
  size = roundUpToPowerOf2(end, cacheLineSize) - data;

  for (size_t i = 0; i < size; i += cacheLineSize)
  {
    PREFETCH_DATA(i, data);
  }
}


void downsample4x_c(unsigned char *dstPixel, const unsigned char *srcPixel, int dstWidth, int dstHeight)
{
  unsigned int dstPitch = dstWidth * 4;       // 4bpp = RGBA
  unsigned int srcPitch = dstWidth * 2 * 4;   // 4bpp = RGBA, 2 because of 2x downsample
  unsigned int srcPitchDouble = srcPitch * 2; // doubled
                                              // PURE C implentation
  prefetch(srcPixel, srcPitchDouble);
  for (unsigned int y = 0; y < dstHeight; y++)
  {
    if (y < dstHeight - 1)
    {
      prefetch(srcPixel + srcPitchDouble, srcPitchDouble);
      if (y > 0)
        preZero(dstPixel + dstPitch, dstPitch);
    }

    for (unsigned int x = 0; x < dstWidth; x++)
    {
      unsigned int a = srcPixel[0];
      unsigned int r = srcPixel[1];
      unsigned int g = srcPixel[2];
      unsigned int b = srcPixel[3];
      a += srcPixel[4];
      r += srcPixel[5];
      g += srcPixel[6];
      b += srcPixel[7];
      a += srcPixel[srcPitch + 0];
      r += srcPixel[srcPitch + 1];
      g += srcPixel[srcPitch + 2];
      b += srcPixel[srcPitch + 3];
      a += srcPixel[srcPitch + 4];
      r += srcPixel[srcPitch + 5];
      g += srcPixel[srcPitch + 6];
      b += srcPixel[srcPitch + 7];

      dstPixel[0] = (unsigned char)(a / 4);
      dstPixel[1] = (unsigned char)(r / 4);
      dstPixel[2] = (unsigned char)(g / 4);
      dstPixel[3] = (unsigned char)(b / 4);

      srcPixel += 8;
      dstPixel += 4;
    }

    srcPixel += srcPitch;
  }
}

void convert_to_float_c(const unsigned char *__restrict data, float *__restrict dest, int num)
{
  const unsigned char *enddata = data + num * 4;
  for (; data < enddata; data += 4, dest += 4)
  {
    dest[0] = global_linear[data[0]];
    dest[1] = global_linear[data[1]];
    dest[2] = global_linear[data[2]];
    dest[3] = global_linear[data[3]];
  }
}

void convert_from_float_c(const float *__restrict data, unsigned char *__restrict dest, int num)
{
  const float *enddata = data + num * 4;
  for (; data < enddata; data += 4, dest += 4)
  {
    dest[0] = clamp((int)(data[0] * 255.0f + 0.5f), 0, 255);
    dest[1] = clamp((int)(data[1] * 255.0f + 0.5f), 0, 255);
    dest[2] = clamp((int)(data[2] * 255.0f + 0.5f), 0, 255);
    dest[3] = clamp((int)(data[3] * 255.0f + 0.5f), 0, 255);
  }
}

// fixme: support different gammas?
void exponentiate4_c(const unsigned char *__restrict data, float *__restrict dest, int num, float power)
{
  const unsigned char *enddata = data + num * 4;
  float *__restrict lut = global_gamma_lut;
  alignas(16) float local_lut[256];
  if (fabsf(power - global_gamma) > 0.001f)
  {
    lut = local_lut;
    for (int i = 1; i < 255; ++i)
      lut[i] = powf(global_linear[i], power);
    lut[0] = 0.f;
    lut[255] = 1.f;
  }

  for (; data < enddata; data += 4, dest += 4)
  {
#if _TARGET_CPU_BE
    const float d0 = global_linear[data[0]];
    const float d1 = lut[data[1]];
    const float d2 = lut[data[2]];
    const float d3 = lut[data[3]];
#else
    const float d0 = lut[data[0]];
    const float d1 = lut[data[1]];
    const float d2 = lut[data[2]];
    const float d3 = global_linear[data[3]];
#endif

    dest[0] = d0;
    dest[1] = d1;
    dest[2] = d2;
    dest[3] = d3;
  }
}

/*
//slower version
void exponentiate4_c(unsigned char *__restrict data, float *__restrict dest, int num, float power)
{
  const unsigned char *enddata = data+num*4;
  for (; data < enddata; data += 4, dest+=4)
  {
    dest[0] = powf(data[0]/255.0f, power);
    dest[1] = powf(data[1]/255.0f, power);
    dest[2] = powf(data[2]/255.0f, power);
    dest[3] = data[3]/255.0f;
  }
}*/


void convert_to_linear_c(const float *__restrict data, unsigned char *__restrict dest, int num, float power)
{
  // todo: our vec math
  const float *enddata = data + num * 4;
  for (; data < enddata; data += 4, dest += 4)
  {
#if _TARGET_CPU_BE
    dest[0] = clamp((int)(data[0] * 255.0f + 0.5f), 0, 255);
    dest[1] = clamp((int)(powf(data[1], power) * 255.0f + 0.5f), 0, 255);
    dest[2] = clamp((int)(powf(data[2], power) * 255.0f + 0.5f), 0, 255);
    dest[3] = clamp((int)(powf(data[3], power) * 255.0f + 0.5f), 0, 255);
#else
    dest[0] = clamp((int)(powf(data[0], power) * 255.0f + 0.5f), 0, 255);
    dest[1] = clamp((int)(powf(data[1], power) * 255.0f + 0.5f), 0, 255);
    dest[2] = clamp((int)(powf(data[2], power) * 255.0f + 0.5f), 0, 255);
    dest[3] = clamp((int)(data[3] * 255.0f + 0.5f), 0, 255);
#endif
  }
}


#if _TARGET_SIMD_SSE
void convert_to_float_simda(const unsigned char *__restrict data, float *__restrict dest, int num)
{
  // todo: our vec math
  const unsigned char *enddata = data + num * 4;
  __m128 valinv255 = _mm_set_ps1(1.0f / 255.0f);
  __m128i zero = _mm_setzero_si128();
  for (; data < enddata; data += 16, dest += 16)
  {
    __m128i src0Fi = _mm_load_si128((const __m128i *)data);
    __m128i src07i = _mm_unpacklo_epi8(src0Fi, zero);
    __m128i src7Fi = _mm_unpackhi_epi8(src0Fi, zero);
    __m128i src03i = _mm_unpacklo_epi16(src07i, zero);
    __m128i src37i = _mm_unpackhi_epi16(src07i, zero);
    __m128i src7Ci = _mm_unpacklo_epi16(src7Fi, zero);
    __m128i srcCFi = _mm_unpackhi_epi16(src7Fi, zero);
    // fixme: SoA can speed up signinficantly, as 4 of pows, are pow(x,1)!
    _mm_store_ps(dest, _mm_mul_ps(_mm_cvtepi32_ps(src03i), valinv255));
    _mm_store_ps(dest + 4, _mm_mul_ps(_mm_cvtepi32_ps(src37i), valinv255));
    _mm_store_ps(dest + 8, _mm_mul_ps(_mm_cvtepi32_ps(src7Ci), valinv255));
    _mm_store_ps(dest + 12, _mm_mul_ps(_mm_cvtepi32_ps(srcCFi), valinv255));
  }
}

void convert_from_float_simda(const float *__restrict data, unsigned char *__restrict dest, int num)
{
  // todo: our vec math
  const float *enddata = data + num * 4;
  __m128 val255 = _mm_set_ps1(255.0f);
  for (; data < enddata; data += 16, dest += 16)
  {
    __m128 src03 = _mm_load_ps(data);
    __m128 src37 = _mm_load_ps(data + 4);
    __m128 src7C = _mm_load_ps(data + 8);
    __m128 srcCF = _mm_load_ps(data + 12);
    // fixme: SoA can speed up signinficantly, as 4 of pows, are pow(x,1)!
    __m128i src03i = _mm_cvtps_epi32(_mm_mul_ps(val255, src03));
    __m128i src37i = _mm_cvtps_epi32(_mm_mul_ps(val255, src37));
    __m128i src7Ci = _mm_cvtps_epi32(_mm_mul_ps(val255, src7C));
    __m128i srcCFi = _mm_cvtps_epi32(_mm_mul_ps(val255, srcCF));
    __m128i result = _mm_packus_epi16(_mm_packs_epi32(src03i, src37i), _mm_packs_epi32(src7Ci, srcCFi));
    _mm_store_si128((__m128i *)dest, result);
  }
}

/*
__forceinline __m128 _mm_fastpow_0_1_ps_approx(__m128 x, __m128 y)
{
  static const __m128 fourOne = _mm_set1_ps(1.0f);
  static const __m128 fourHalf = _mm_set1_ps(0.5f);

  __m128 a = _mm_sub_ps(fourOne, y);
  __m128 b = _mm_sub_ps(x, fourOne);
  __m128 aSq = _mm_mul_ps(a, a);
  __m128 bSq = _mm_mul_ps(b, b);
  __m128 c = _mm_mul_ps(fourHalf, bSq);
  __m128 d = _mm_sub_ps(b, c);
  __m128 dSq = _mm_mul_ps(d, d);
  __m128 e = _mm_mul_ps(aSq, dSq);
  __m128 f = _mm_mul_ps(a, d);
  __m128 g = _mm_mul_ps(fourHalf, e);
  __m128 h = _mm_add_ps(fourOne, f);
  __m128 i = _mm_add_ps(h, g);
  __m128 iRcp = _mm_rcp_ps(i);
  __m128 result = _mm_mul_ps(x, iRcp);

  return result;
}*/

// #include "sse_mathfun.h"
__forceinline vec4f _mm_fastpow_0_1_ps(vec4f x, vec4f y)
{
  return v_pow_est(v_max(x, v_zero()), y);
  // return exp2f4(_mm_mul_ps(log2f4(x), y));
}

#define SOA 1

#ifndef SOA
void convert_to_linear_simda(const float *__restrict data, unsigned char *__restrict dest, int num, float power)
{
  // todo: our vec math
  const float *enddata = data + num * 4;
  __m128 powVal = _mm_setr_ps(power, power, power, 1);
  __m128 val255 = _mm_set_ps1(255.0f);
  __m128 zero = _mm_setzero_ps();
  static const __m128 fourOne = _mm_set1_ps(1.0f);
  for (; data < enddata; data += 16, dest += 16)
  {
    __m128 src03 = _mm_load_ps(data);
    __m128 src37 = _mm_load_ps(data + 4);
    __m128 src7C = _mm_load_ps(data + 8);
    __m128 srcCF = _mm_load_ps(data + 12);
    // no need in minmax as will be saturated
    // SoA can speed up signinficantly, as 4 of pows, are pow(x,1)!
    __m128i src03i = _mm_cvtps_epi32(_mm_mul_ps(val255, _mm_fastpow_0_1_ps(src03, powVal)));
    __m128i src37i = _mm_cvtps_epi32(_mm_mul_ps(val255, _mm_fastpow_0_1_ps(src37, powVal)));
    __m128i src7Ci = _mm_cvtps_epi32(_mm_mul_ps(val255, _mm_fastpow_0_1_ps(src7C, powVal)));
    __m128i srcCFi = _mm_cvtps_epi32(_mm_mul_ps(val255, _mm_fastpow_0_1_ps(srcCF, powVal)));
    __m128i result = _mm_packus_epi16(_mm_packs_epi32(src03i, src37i), _mm_packs_epi32(src7Ci, srcCFi));
    _mm_store_si128((__m128i *)dest, result);
  }
}

void exponentiate4_simda(const unsigned char *__restrict data, float *__restrict dest, int num, float power)
{
  // todo: our vec math
  const unsigned char *enddata = data + num * 4;
  __m128 powVal = _mm_setr_ps(power, power, power, 1);
  __m128 valinv255 = _mm_set_ps1(1.0f / 255.0f);
  __m128i zero = _mm_setzero_si128();
  for (; data < enddata; data += 16, dest += 16)
  {

    __m128i src0Fi = _mm_load_si128((const __m128i *)data);
    __m128i src07i = _mm_unpacklo_epi8(src0Fi, zero);
    __m128i src7Fi = _mm_unpackhi_epi8(src0Fi, zero);
    __m128i src03i = _mm_unpacklo_epi16(src07i, zero);
    __m128i src37i = _mm_unpackhi_epi16(src07i, zero);
    __m128i src7Ci = _mm_unpacklo_epi16(src7Fi, zero);
    __m128i srcCFi = _mm_unpackhi_epi16(src7Fi, zero);
    // fixme: SoA can speed up signinficantly, as 4 of pows, are pow(x,1)!
    _mm_store_ps(dest, _mm_fastpow_0_1_ps(_mm_mul_ps(_mm_cvtepi32_ps(src03i), valinv255), powVal));
    _mm_store_ps(dest + 4, _mm_fastpow_0_1_ps(_mm_mul_ps(_mm_cvtepi32_ps(src37i), valinv255), powVal));
    _mm_store_ps(dest + 8, _mm_fastpow_0_1_ps(_mm_mul_ps(_mm_cvtepi32_ps(src7Ci), valinv255), powVal));
    _mm_store_ps(dest + 12, _mm_fastpow_0_1_ps(_mm_mul_ps(_mm_cvtepi32_ps(srcCFi), valinv255), powVal));
  }
}

#else
// SoA
void convert_to_linear_simda(const float *__restrict data, unsigned char *__restrict dest, int num, float power)
{
  // todo: our vec math
  const float *enddata = data + num * 4;
  __m128 powVal = _mm_setr_ps(power, power, power, power);
  __m128 val255 = _mm_set_ps1(255.0f);
  for (; data < enddata; data += 16, dest += 16)
  {
    __m128 col0 = _mm_load_ps(data);
    __m128 col1 = _mm_load_ps(data + 4);
    __m128 col2 = _mm_load_ps(data + 8);
    __m128 col3 = _mm_load_ps(data + 12);

    // SoA speed up signinficantly, as otherwise 4 of pows, are pow(x,1)
    __m128 tmp3, tmp2, tmp1, tmp0;

    tmp0 = _mm_shuffle_ps(col0, col1, 0x44);
    tmp1 = _mm_shuffle_ps(col2, col3, 0x44);
    tmp2 = _mm_shuffle_ps(col0, col1, 0xEE);
    tmp3 = _mm_shuffle_ps(col2, col3, 0xEE);

    col0 = _mm_mul_ps(_mm_fastpow_0_1_ps(_mm_shuffle_ps(tmp0, tmp1, 0x88), powVal), val255);
    col1 = _mm_mul_ps(_mm_fastpow_0_1_ps(_mm_shuffle_ps(tmp0, tmp1, 0xDD), powVal), val255);
    col2 = _mm_mul_ps(_mm_fastpow_0_1_ps(_mm_shuffle_ps(tmp2, tmp3, 0x88), powVal), val255);
    col3 = _mm_mul_ps(_mm_shuffle_ps(tmp2, tmp3, 0xDD), val255);

    tmp0 = _mm_shuffle_ps(col0, col1, 0x44);
    tmp1 = _mm_shuffle_ps(col2, col3, 0x44);
    tmp2 = _mm_shuffle_ps(col0, col1, 0xEE);
    tmp3 = _mm_shuffle_ps(col2, col3, 0xEE);
    __m128i src03i = _mm_cvtps_epi32(_mm_shuffle_ps(tmp0, tmp1, 0x88));
    __m128i src37i = _mm_cvtps_epi32(_mm_shuffle_ps(tmp0, tmp1, 0xDD));
    __m128i src7Ci = _mm_cvtps_epi32(_mm_shuffle_ps(tmp2, tmp3, 0x88));
    __m128i srcCFi = _mm_cvtps_epi32(_mm_shuffle_ps(tmp2, tmp3, 0xDD));
    __m128i result = _mm_packus_epi16(_mm_packs_epi32(src03i, src37i), _mm_packs_epi32(src7Ci, srcCFi));
    _mm_store_si128((__m128i *)dest, result);
  }
}

void exponentiate4_simda(const unsigned char *__restrict data, float *__restrict dest, int num, float power)
{
  // todo: our vec math
  const unsigned char *enddata = data + num * 4;
  __m128 powVal = _mm_setr_ps(power, power, power, power);
  __m128 valinv255 = _mm_set_ps1(1.0f / 255.0f);
  __m128i zero = _mm_setzero_si128();
  for (; data < enddata; data += 16, dest += 16)
  {

    __m128i src0Fi = _mm_load_si128((const __m128i *)data);
    __m128i src07i = _mm_unpacklo_epi8(src0Fi, zero);
    __m128i src7Fi = _mm_unpackhi_epi8(src0Fi, zero);
    __m128i src03i = _mm_unpacklo_epi16(src07i, zero);
    __m128i src37i = _mm_unpackhi_epi16(src07i, zero);
    __m128i src7Ci = _mm_unpacklo_epi16(src7Fi, zero);
    __m128i srcCFi = _mm_unpackhi_epi16(src7Fi, zero);
    // SoA speed up signinficantly, as otherwise 4 of pows, are pow(x,1)
    __m128 tmp3, tmp2, tmp1, tmp0;
    __m128 col0 = _mm_cvtepi32_ps(src03i);
    __m128 col1 = _mm_cvtepi32_ps(src37i);
    __m128 col2 = _mm_cvtepi32_ps(src7Ci);
    __m128 col3 = _mm_cvtepi32_ps(srcCFi);

    tmp0 = _mm_shuffle_ps(col0, col1, 0x44);
    tmp2 = _mm_shuffle_ps(col0, col1, 0xEE);
    tmp1 = _mm_shuffle_ps(col2, col3, 0x44);
    tmp3 = _mm_shuffle_ps(col2, col3, 0xEE);

    col0 = _mm_fastpow_0_1_ps(_mm_mul_ps(_mm_shuffle_ps(tmp0, tmp1, 0x88), valinv255), powVal);
    col1 = _mm_fastpow_0_1_ps(_mm_mul_ps(_mm_shuffle_ps(tmp0, tmp1, 0xDD), valinv255), powVal);
    col2 = _mm_fastpow_0_1_ps(_mm_mul_ps(_mm_shuffle_ps(tmp2, tmp3, 0x88), valinv255), powVal);
    col3 = _mm_mul_ps(_mm_shuffle_ps(tmp2, tmp3, 0xDD), valinv255);

    tmp0 = _mm_shuffle_ps(col0, col1, 0x44);
    tmp2 = _mm_shuffle_ps(col0, col1, 0xEE);
    tmp1 = _mm_shuffle_ps(col2, col3, 0x44);
    tmp3 = _mm_shuffle_ps(col2, col3, 0xEE);

    _mm_store_ps(dest, _mm_shuffle_ps(tmp0, tmp1, 0x88));
    _mm_store_ps(dest + 4, _mm_shuffle_ps(tmp0, tmp1, 0xDD));
    _mm_store_ps(dest + 8, _mm_shuffle_ps(tmp2, tmp3, 0x88));
    _mm_store_ps(dest + 12, _mm_shuffle_ps(tmp2, tmp3, 0xDD));
  }
}

void exponentiate4_simdu(const unsigned char *__restrict data, float *__restrict dest, int num, float power)
{
  // todo: our vec math
  const unsigned char *enddata = data + num * 4;
  __m128 powVal = _mm_setr_ps(power, power, power, power);
  __m128 valinv255 = _mm_set_ps1(1.0f / 255.0f);
  __m128i zero = _mm_setzero_si128();
  for (; data < enddata; data += 16, dest += 16)
  {

    __m128i src0Fi = _mm_loadu_si128((const __m128i *)data);
    __m128i src07i = _mm_unpacklo_epi8(src0Fi, zero);
    __m128i src7Fi = _mm_unpackhi_epi8(src0Fi, zero);
    __m128i src03i = _mm_unpacklo_epi16(src07i, zero);
    __m128i src37i = _mm_unpackhi_epi16(src07i, zero);
    __m128i src7Ci = _mm_unpacklo_epi16(src7Fi, zero);
    __m128i srcCFi = _mm_unpackhi_epi16(src7Fi, zero);
    // SoA speed up signinficantly, as otherwise 4 of pows, are pow(x,1)
    __m128 tmp3, tmp2, tmp1, tmp0;
    __m128 col0 = _mm_cvtepi32_ps(src03i);
    __m128 col1 = _mm_cvtepi32_ps(src37i);
    __m128 col2 = _mm_cvtepi32_ps(src7Ci);
    __m128 col3 = _mm_cvtepi32_ps(srcCFi);

    tmp0 = _mm_shuffle_ps(col0, col1, 0x44);
    tmp2 = _mm_shuffle_ps(col0, col1, 0xEE);
    tmp1 = _mm_shuffle_ps(col2, col3, 0x44);
    tmp3 = _mm_shuffle_ps(col2, col3, 0xEE);

    col0 = _mm_fastpow_0_1_ps(_mm_mul_ps(_mm_shuffle_ps(tmp0, tmp1, 0x88), valinv255), powVal);
    col1 = _mm_fastpow_0_1_ps(_mm_mul_ps(_mm_shuffle_ps(tmp0, tmp1, 0xDD), valinv255), powVal);
    col2 = _mm_fastpow_0_1_ps(_mm_mul_ps(_mm_shuffle_ps(tmp2, tmp3, 0x88), valinv255), powVal);
    col3 = _mm_mul_ps(_mm_shuffle_ps(tmp2, tmp3, 0xDD), valinv255);

    tmp0 = _mm_shuffle_ps(col0, col1, 0x44);
    tmp2 = _mm_shuffle_ps(col0, col1, 0xEE);
    tmp1 = _mm_shuffle_ps(col2, col3, 0x44);
    tmp3 = _mm_shuffle_ps(col2, col3, 0xEE);

    _mm_storeu_ps(dest, _mm_shuffle_ps(tmp0, tmp1, 0x88));
    _mm_storeu_ps(dest + 4, _mm_shuffle_ps(tmp0, tmp1, 0xDD));
    _mm_storeu_ps(dest + 8, _mm_shuffle_ps(tmp2, tmp3, 0x88));
    _mm_storeu_ps(dest + 12, _mm_shuffle_ps(tmp2, tmp3, 0xDD));
  }
}

#endif // SoA

void downsample4x_simda_gamma_correct(unsigned char *destData, const unsigned char *srcData, int destW, int destH)
{
  float power = 1.0 / global_gamma;
  unsigned int srcPitch = destW * 2 * 4; // 4bpp = RGBA, 2 because of 2x downsample
  vec4f powVal = v_make_vec4f(power, power, power, 1.f);
  vec4f val255 = v_splats(255.0f);
  vec4f quarter = v_splats(0.25f);
#if _TARGET_CPU_BE
#define CONVERT_PIXEL(dest, sdata)       \
  {                                      \
    const uint8_t *data = sdata;         \
    dest[0] = global_linear[data[0]];    \
    dest[1] = global_gamma_lut[data[1]]; \
    dest[2] = global_gamma_lut[data[2]]; \
    dest[3] = global_gamma_lut[data[3]]; \
  }
#else
#define CONVERT_PIXEL(dest, sdata)       \
  {                                      \
    const uint8_t *data = sdata;         \
    dest[0] = global_gamma_lut[data[0]]; \
    dest[1] = global_gamma_lut[data[1]]; \
    dest[2] = global_gamma_lut[data[2]]; \
    dest[3] = global_linear[data[3]];    \
  }
#endif

#if _TARGET_SIMD_SSE
  for (int y = 0; y < destH; y++, srcData += srcPitch)
  {
    for (int x = 0; x < destW; x += 2, srcData += 16, destData += 8)
    {
      alignas(16) float pixel[2][4][4];
      CONVERT_PIXEL(pixel[0][0], srcData);
      CONVERT_PIXEL(pixel[0][1], srcData + 4);
      CONVERT_PIXEL(pixel[0][2], srcData + 8);
      CONVERT_PIXEL(pixel[0][3], srcData + 12);
      const uint8_t *nextLine = srcData + srcPitch;
      CONVERT_PIXEL(pixel[1][0], nextLine);
      CONVERT_PIXEL(pixel[1][1], nextLine + 4);
      CONVERT_PIXEL(pixel[1][2], nextLine + 8);
      CONVERT_PIXEL(pixel[1][3], nextLine + 12);
      vec4f up0 = v_ld(pixel[0][0]);
      vec4f up1 = v_ld(pixel[0][1]);
      vec4f down0 = v_ld(pixel[1][0]);
      vec4f down1 = v_ld(pixel[1][1]);
      vec4f result0 = v_mul(v_add(v_add(up0, up1), v_add(down0, down1)), quarter);
      result0 = v_mul(_mm_fastpow_0_1_ps(result0, powVal), val255);
      vec4i result0i = _mm_cvtps_epi32(result0);

      vec4f up2 = v_ld(pixel[0][2]);
      vec4f up3 = v_ld(pixel[0][3]);
      vec4f down2 = v_ld(pixel[1][2]);
      vec4f down3 = v_ld(pixel[1][3]);
      vec4f result1 = v_mul(v_add(v_add(up2, up3), v_add(down2, down3)), quarter);
      result1 = v_mul(_mm_fastpow_0_1_ps(result1, powVal), val255);
      vec4i result1i = _mm_cvtps_epi32(result1);
      vec4i result12 = _mm_packs_epi32(result0i, result1i);
      _mm_storel_epi64((__m128i *)destData, _mm_packus_epi16(result12, result12));
    }
  }
#else
  for (int y = 0; y < destH; y++, srcData += srcPitch)
  {
    for (int x = 0; x < destW; x += 2, srcData += 8, destData += 4)
    {
      alignas(16) float pixel[2][4];
      CONVERT_PIXEL(pixel[0][0], srcData);
      CONVERT_PIXEL(pixel[0][1], srcData + 4);
      const uint8_t *nextLine = srcData + srcPitch;
      CONVERT_PIXEL(pixel[1][0], nextLine);
      CONVERT_PIXEL(pixel[1][1], nextLine + 4);

      vec4f up0 = v_ld(pixel[0][0]);
      vec4f up1 = v_ld(pixel[0][1]);
      vec4f down0 = v_ld(pixel[1][0]);
      vec4f down1 = v_ld(pixel[1][1]);
      vec4f result0 = v_max(v_min(v_mul(v_add(v_add(up0, up1), v_add(down0, down1)), quarter), V_C_ONE), v_zero());
      alignas(16) float result[4];
      v_st(result, result0);
#if _TARGET_CPU_BE
      destData[0] = result[0] * 255.f;
      destData[1] = powf(result[1], global_gamma) * 255.f;
      destData[2] = powf(result[2], global_gamma) * 255.f;
      destData[3] = powf(result[3], global_gamma) * 255.f;
#else
      destData[0] = powf(result[0], global_gamma) * 255.f;
      destData[1] = powf(result[1], global_gamma) * 255.f;
      destData[2] = powf(result[2], global_gamma) * 255.f;
      destData[3] = result[3] * 255.f;
#endif
    }
  }
#endif

#undef CONVERT_PIXEL
}

void downsample4x_simda(unsigned char *destData, const unsigned char *srcData, int destW, int destH)
{
  // todo: our vec math
  unsigned int srcPitch = destW * 2 * 4; // 4bpp = RGBA, 2 because of 2x downsample
  __m128i zero = _mm_setzero_si128();
  for (int y = 0; y < destH; y++, srcData += srcPitch)
  {
    for (int x = 0; x < destW; x += 2, srcData += 16, destData += 8)
    {
      __m128i src0Fi, src07i, src7Fi;
      // read four pixels
      src0Fi = _mm_load_si128((const __m128i *)srcData);
      __m128i srcup_07i = _mm_unpacklo_epi8(src0Fi, zero);
      __m128i srcup_7Fi = _mm_unpackhi_epi8(src0Fi, zero);

      src0Fi = _mm_load_si128((const __m128i *)(srcData + srcPitch));
      __m128i srcdown_07i = _mm_unpacklo_epi8(src0Fi, zero);
      __m128i srcdown_7Fi = _mm_unpackhi_epi8(src0Fi, zero);

      // vertical sum
      src07i = _mm_add_epi16(srcup_07i, srcdown_07i);
      src7Fi = _mm_add_epi16(srcup_7Fi, srcdown_7Fi);

      __m128i c2323;
      c2323 = _mm_shuffle_epi32(src07i, _MM_SHUFFLE(3, 2, 3, 2));
      __m128i hor_sum1 = _mm_add_epi16(src07i, c2323);

      c2323 = _mm_shuffle_epi32(src7Fi, _MM_SHUFFLE(3, 2, 3, 2));
      __m128i hor_sum2 = _mm_add_epi16(src7Fi, c2323);

      __m128i hor_sum = _mm_unpacklo_epi64(hor_sum1, hor_sum2);

      __m128i res = _mm_srli_epi16(hor_sum, 2);
      res = _mm_packus_epi16(res, zero);
      _mm_storel_epi64((__m128i *)destData, res);
    }
  }
}
void downsample4x_simdu(unsigned char *destData, const unsigned char *srcData, int destW, int destH)
{
  // todo: our vec math
  unsigned int srcPitch = destW * 2 * 4; // 4bpp = RGBA, 2 because of 2x downsample
  __m128i zero = _mm_setzero_si128();
  for (int y = 0; y < destH - 1; y++, srcData += srcPitch)
  {
    for (int x = 0; x < destW; x++, srcData += 8, destData += 4)
    {
      // read four pixels
      __m128i c01 = _mm_loadl_epi64((const __m128i *)srcData);
      __m128i c23 = _mm_loadl_epi64((const __m128i *)(srcData + srcPitch));
      c01 = _mm_unpacklo_epi8(c01, zero);
      c23 = _mm_unpacklo_epi8(c23, zero);
      __m128i c0123 = _mm_add_epi16(c01, c23);
      __m128i c2323 = _mm_shuffle_epi32(c0123, _MM_SHUFFLE(3, 2, 3, 2));
      __m128i res = _mm_srli_epi16(_mm_add_epi16(c0123, c2323), 2);
      res = _mm_packus_epi16(res, zero);
      _mm_storel_epi64((__m128i *)destData, res); // intentionally overwrites 4 bytes after, as it is faster.
    }
  }
  for (int x = 0; x < destW; x++, srcData += 8, destData += 4)
  {
    // read four pixels
    __m128i c01 = _mm_loadl_epi64((const __m128i *)srcData);
    __m128i c23 = _mm_loadl_epi64((const __m128i *)(srcData + srcPitch));
    c01 = _mm_unpacklo_epi8(c01, zero);
    c23 = _mm_unpacklo_epi8(c23, zero);
    __m128i c0123 = _mm_add_epi16(c01, c23);
    __m128i c2323 = _mm_shuffle_epi32(c0123, _MM_SHUFFLE(3, 2, 3, 2));
    __m128i res = _mm_srli_epi16(_mm_add_epi16(c0123, c2323), 2);
    res = _mm_packus_epi16(res, zero);
    *((unsigned *)destData) = _mm_cvtsi128_si32(res); // not overwriting any bytes
  }
}
void downsample2x_simdu(unsigned char *destData, const unsigned char *srcData, int destW)
{
  // todo: our vec math
  __m128i zero = _mm_setzero_si128();
  for (int x = 0; x < destW; x++, srcData += 8, destData += 4)
  {
    // read two pixels
    __m128i c01 = _mm_loadl_epi64((const __m128i *)srcData);
    c01 = _mm_unpacklo_epi8(c01, zero);
    __m128i c2323 = _mm_shuffle_epi32(c01, _MM_SHUFFLE(3, 2, 3, 2));
    __m128i res = _mm_srli_epi16(_mm_add_epi16(c01, c2323), 1);
    res = _mm_packus_epi16(res, zero);
    *((unsigned *)destData) = _mm_cvtsi128_si32(res); // not overwriting any bytes
  }
}

// SIMD SSE
#elif _TARGET_SIMD_NEON

void convert_to_linear_simda(const float *__restrict data, unsigned char *__restrict dest, int num_pixels, float invgamma)
{
  const float *enddata = data + num_pixels * 4;
  unsigned char *tmpDest = dest;
  vec4f m = v_splats(255.0f);
  vec4f p = v_make_vec4f(invgamma, invgamma, invgamma, 1);
  vec4f half = v_splats(0.5f);

  for (; data < enddata; data += 8, dest += 8)
  {
    vec4f d1 = v_ld(data);
    vec4f d2 = v_ld(data + 4);
    vec4f rf1 = v_add(v_mul(v_pow_est(d1, p), m), half);
    vec4f rf2 = v_add(v_mul(v_pow_est(d2, p), m), half);
    vec4f ri1 = v_cvt_vec4i(rf1);
    vec4f ri2 = v_cvt_vec4i(rf2);

    uint16x4_t rui1 = vqmovun_s32(ri1);
    uint16x4_t rui2 = vqmovun_s32(ri2);
    uint16x8_t rui = vcombine_u16(rui1, rui2);
    uint8x8_t r = vqmovn_u16(rui);

    vst1_u8(dest, r);
  }
}

void exponentiate4_simda(const unsigned char *__restrict data, float *__restrict dest, int num_pixels, float gamma)
{
  exponentiate4_c(data, dest, num_pixels, gamma);
}

void convert_to_float_simda(const unsigned char *__restrict data, float *__restrict dest, int num)
{
  convert_to_float_c(data, dest, num);
}
void convert_from_float_simda(const float *__restrict data, unsigned char *__restrict dest, int num)
{
  convert_from_float_c(data, dest, num);
}
void downsample4x_simda(unsigned char *destData, const unsigned char *srcData, int destW, int destH)
{
  downsample4x_c(destData, srcData, destW, destH);
}

void exponentiate4_simdu(const unsigned char *__restrict data, float *__restrict dest, int num_pixels, float gamma)
{
  exponentiate4_c(data, dest, num_pixels, gamma);
}
void convert_to_linear_simdu(const float *__restrict data, unsigned char *__restrict dest, int num_pixels, float invgamma)
{
  convert_to_linear_c(data, dest, num_pixels, invgamma);
}

void downsample4x_simdu(unsigned char *destData, const unsigned char *srcData, int destW, int destH)
{
  downsample4x_c(destData, srcData, destW, destH);
}

void downsample2x_simdu(unsigned char *destData, const unsigned char *srcData, int destW)
{
  downsample4x_c(destData, srcData, destW, 1);
}

void downsample4x_simda_gamma_correct(unsigned char *destData, const unsigned char *srcData, int destW, int destH)
{
  downsample4x_c(destData, srcData, destW, destH);
}

#else // SIMD NEON

void convert_to_linear_simda(const float *__restrict data, unsigned char *__restrict dest, int num_pixels, float invgamma)
{
  convert_to_linear_c(data, dest, num_pixels, invgamma);
}

void exponentiate4_simda(const unsigned char *__restrict data, float *__restrict dest, int num_pixels, float gamma)
{
  exponentiate4_c(data, dest, num_pixels, gamma);
}

void convert_to_float_simda(const unsigned char *__restrict data, float *__restrict dest, int num)
{
  convert_to_float_c(data, dest, num);
}
void convert_from_float_simda(const float *__restrict data, unsigned char *__restrict dest, int num)
{
  convert_from_float_c(data, dest, num);
}
void downsample4x_simda(unsigned char *destData, const unsigned char *srcData, int destW, int destH)
{
  downsample4x_c(destData, srcData, destW, destH);
}

void exponentiate4_simdu(const unsigned char *__restrict data, float *__restrict dest, int num_pixels, float gamma)
{
  exponentiate4_c(data, dest, num_pixels, gamma);
}
void convert_to_linear_simdu(const float *__restrict data, unsigned char *__restrict dest, int num_pixels, float invgamma)
{
  convert_to_linear_c(data, dest, num_pixels, invgamma);
}

void downsample4x_simdu(unsigned char *destData, const unsigned char *srcData, int destW, int destH)
{
  downsample4x_c(destData, srcData, destW, destH);
}

void downsample2x_simdu(unsigned char *destData, const unsigned char *srcData, int destW)
{
  downsample4x_c(destData, srcData, destW, 1);
}

#endif

void downsample4x_simda(float *__restrict destData, const float *__restrict srcData, int destW, int destH)
{
  unsigned int srcPitch = destW * 2 * 4; // 4bpp = RGBA, 2 because of 2x downsample
  vec4f quarter = v_splats(0.25f);
  for (int y = 0; y < destH; y++, srcData += srcPitch)
  {
#define FETCH(dstup, dstdn)                   \
  vec4f up0 = v_ld(srcData);                  \
  vec4f up1 = v_ld(srcData + 4);              \
  vec4f down0 = v_ld(srcData + srcPitch);     \
  vec4f down1 = v_ld(srcData + srcPitch + 4); \
  dstup = v_add(up0, up1);                    \
  dstdn = v_add(down0, down1);

    if (destW < 3)
    {
      for (int x = 0; x < destW; x++, srcData += 8, destData += 4)
      {
        vec4f up, down;
        FETCH(up, down);
        v_st(destData, v_mul(v_add(up, down), quarter));
      }
    }
    else
    {
      // 3-stage modulo scheduling to avoid bubbles on PPC/360

      vec4f up, down, vsum;
      {
        FETCH(up, down); //-2
        srcData += 8;
      }
      {
        vec4f up2, down2;
        FETCH(up2, down2); //-1

        vsum = v_add(up, down); //-2

        up = up2;
        down = down2;
        srcData += 8;
      }

      for (int x = 2; x < destW; x++, srcData += 8, destData += 4)
      {
        vec4f up2, down2;
        FETCH(up2, down2); // 0

        vec4f vsum2 = v_add(up, down); //-1

        v_st(destData, v_mul(vsum, quarter)); //-2

        up = up2;
        down = down2;
        vsum = vsum2;
      }

      v_st(destData, v_mul(vsum, quarter)); //-1
      destData += 4;
      v_st(destData, v_mul(v_add(up, down), quarter)); // 0
      destData += 4;
#undef FETCH
    }
  }
}

void downsample4x_simdu(float *destData, const float *srcData, int destW, int destH)
{
  unsigned int srcPitch = destW * 2 * 4; // 4bpp = RGBA, 2 because of 2x downsample
  vec4f quorter = v_splats(0.25f);
  for (int y = 0; y < destH; y++, srcData += srcPitch)
  {
    for (int x = 0; x < destW; x++, srcData += 8, destData += 4)
    {
      vec4f up0 = v_ldu(srcData);
      vec4f up1 = v_ldu(srcData + 4);
      vec4f down0 = v_ldu(srcData + srcPitch);
      vec4f down1 = v_ldu(srcData + srcPitch + 4);
      up0 = v_add(up0, up1);
      down0 = v_add(down0, down1);
      v_stu(destData, v_mul(v_add(up0, down0), quorter));
    }
  }
}
void downsample2x_simdu(float *destData, const float *srcData, int destW)
{
  vec4f half = v_splats(0.5f);
  for (int x = 0; x < destW; x++, srcData += 8, destData += 4)
  {
    vec4f up0 = v_ldu(srcData);
    vec4f up1 = v_ldu(srcData + 4);
    up0 = v_add(up0, up1);
    v_stu(destData, v_mul(up0, half));
  }
}
void downsample4x_c(float *destData, const float *srcData, int destW, int destH)
{
  if ((((size_t)destData) & 15) || (((size_t)srcData) & 15))
    downsample4x_simdu(destData, srcData, destW, destH);
  else
    downsample4x_simda(destData, srcData, destW, destH);
}

}; // namespace imagefunctions
