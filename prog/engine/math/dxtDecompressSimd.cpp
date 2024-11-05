// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <image/dag_dxtCompress.h>
#include <util/dag_globDef.h>
#include <vecmath/dag_vecMath.h>
#include <stdio.h>

#define R_SHUFFLE_D(x, y, z, w) (((w)&3) << 6 | ((z)&3) << 4 | ((y)&3) << 2 | ((x)&3))
alignas(16) static int SIMD_SSE2_minus1[4] = {-1, 0, -1, 0};
alignas(16) static unsigned short SIMD_SSE2_word_m3[8] = {0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003};
alignas(16) static unsigned int SIMD_SSE2_3[4] = {0x0003, 0, 0x0003, 0};
alignas(16) static unsigned int SIMD_SSE2_1[4] = {0x0001, 0x0001, 0x0001, 0};

alignas(16) static unsigned short SIMD_SSE2_RED_MASK[8] = {0x1F << 11, 0x1F << 11, 0, 0, 0, 0, 0, 0};
alignas(16) static unsigned short SIMD_SSE2_GREEN_MASK[8] = {0x3F << 5, 0x3F << 5, 0, 0, 0, 0, 0, 0};
alignas(16) static unsigned short SIMD_SSE2_BLUE_MASK[8] = {0x1F, 0x1F, 0, 0, 0, 0, 0, 0};
alignas(16) static unsigned short SIMD_SSE2_ALPHA_MASK[8] = {0, 0, 0, 0xFFFF, 0, 0, 0, 0xFFFF};
alignas(16) static unsigned short SIMD_SSE2_word_div_by_3[8] = {(1 << 16) / 3 + 1, (1 << 16) / 3 + 1, (1 << 16) / 3 + 1,
  (1 << 16) / 3 + 1, (1 << 16) / 3 + 1, (1 << 16) / 3 + 1, (1 << 16) / 3 + 1, (1 << 16) / 3 + 1};

__forceinline __m128i unpack_565_colors(__m128i color01)
{
  const __m128i zero = _mm_setzero_si128();
  // color01 = _mm_unpacklo_epi8(_mm_srli_epi16(color01, 8), color01);

  /*const __m128i RGB565Mask = _mm_load_si128((__m128i*)SIMD_SSE2_word_colorMask16);
  color01 = _mm_loadl_epi64((__m128i*)maxColor);
  color01 = _mm_unpacklo_epi8(color01, zero);
  color01 = _mm_and_si128(color01, RGB565Mask);*/
  __m128i reds = _mm_and_si128(color01, *(__m128i *)SIMD_SSE2_RED_MASK);
  __m128i greens = _mm_and_si128(color01, *(__m128i *)SIMD_SSE2_GREEN_MASK);
  greens = _mm_slli_epi16(greens, 5);

  __m128i blues = _mm_and_si128(color01, *(__m128i *)SIMD_SSE2_BLUE_MASK);
  blues = _mm_slli_epi16(blues, 11);

  reds = _mm_unpacklo_epi16(reds, zero);
  color01 = _mm_unpacklo_epi16(blues, greens);
  color01 = _mm_unpacklo_epi32(color01, zero);
  color01 = _mm_or_si128(color01, _mm_unpacklo_epi32(zero, reds));


  __m128i redBlue = _mm_shufflelo_epi16(color01, R_SHUFFLE_D(0, 3, 2, 3));
  __m128i green = _mm_shufflelo_epi16(color01, R_SHUFFLE_D(3, 1, 3, 1));

  redBlue = _mm_shufflehi_epi16(redBlue, R_SHUFFLE_D(0, 3, 2, 3));
  green = _mm_shufflehi_epi16(green, R_SHUFFLE_D(3, 1, 3, 3));

  redBlue = _mm_srli_epi16(redBlue, 5);
  green = _mm_srli_epi16(green, 6);
  color01 = _mm_or_si128(color01, redBlue);
  color01 = _mm_or_si128(color01, green);
  return color01;
}

static inline void decompress_dxt5_color_block_sse_unrolled(unsigned char *rgba, size_t stride, __m128i data)
{
  // get the block bytes
  // unpack the endpoints
  __m128i maxMinColor = _mm_srli_epi16(unpack_565_colors(data), 8);

  // Compute color0 (maxColor).
  __m128i maxColor = _mm_unpacklo_epi64(maxMinColor, maxMinColor);
  __m128i minColor = _mm_unpackhi_epi64(maxMinColor, maxMinColor);


  // const __m128i one = _mm_cvtsi32_si128(1);
  __m128i mask = _mm_shuffle_epi32(data, R_SHUFFLE_D(1, 1, 1, 1));

  __m128i maski, mask2;
  // 00 -> 11 | 0 -> 0
  // 01 -> 00 | 1 -> 3
  // 10 -> 01 | 2 -> 1
  // 11 -> 10 | 3 -> 2
  // y = (x>>1);
  // y |= y<<1;
  // mask = (mask-1)&3 ^ (~y)&3
  __m128i signbits, three_sub_maski, color1, color2;

#define UNPACK_2_COLORS(colori, offset)                                                                                          \
  maski = offset ? _mm_srli_epi32(mask, offset) : mask;                                                                          \
  mask2 = _mm_srli_epi32(mask, offset + 2);                                                                                      \
  maski = _mm_and_si128(_mm_unpacklo_epi64(maski, mask2), *((__m128i *)SIMD_SSE2_3));                                            \
  signbits = _mm_cmpgt_epi32(maski, *((__m128i *)SIMD_SSE2_1));                                                                  \
  maski = _mm_add_epi32(maski, *((__m128i *)SIMD_SSE2_minus1));                                                                  \
  maski = _mm_xor_si128(_mm_and_si128(maski, *((__m128i *)SIMD_SSE2_3)), _mm_andnot_si128(signbits, *((__m128i *)SIMD_SSE2_3))); \
  maski = _mm_shufflelo_epi16(maski, R_SHUFFLE_D(0, 0, 0, 0));                                                                   \
  maski = _mm_shufflehi_epi16(maski, R_SHUFFLE_D(0, 0, 0, 0));                                                                   \
  three_sub_maski = _mm_xor_si128(maski, *((__m128i *)SIMD_SSE2_word_m3));                                                       \
  colori = _mm_add_epi16(_mm_mullo_epi16(maxColor, three_sub_maski), _mm_mullo_epi16(minColor, maski));                          \
  colori = _mm_mulhi_epu16(colori, *((__m128i *)SIMD_SSE2_word_div_by_3)); // division by 3

// colori = _mm_packus_epi16(colori, zero);
//_mm_storel_epi64((__m128i*)(rgba + i * 2), colori);
#define UNPACK_ROW(dest, offset)      \
  UNPACK_2_COLORS(color1, offset)     \
  UNPACK_2_COLORS(color2, offset + 4) \
  _mm_storeu_si128((__m128i *)(dest), _mm_packus_epi16(color1, color2));

  UNPACK_ROW(rgba, 0)
  rgba += stride;
  UNPACK_ROW(rgba, 8)
  rgba += stride;
  UNPACK_ROW(rgba, 16)
  rgba += stride;
  UNPACK_ROW(rgba, 24)
#undef UNPACK_ROW
}

static inline void decompress_dxt1a_color_block_sse(unsigned char *rgba, size_t stride, __m128i data)
{
  // get the block bytes
  // unpack the endpoints
  __m128i maxMinColor = _mm_srli_epi16(unpack_565_colors(data), 8);
  maxMinColor = _mm_or_si128(maxMinColor, *(__m128i *)SIMD_SSE2_ALPHA_MASK);

  __m128i color23 = _mm_add_epi16(maxMinColor, maxMinColor);
  color23 = _mm_add_epi16(color23, _mm_shuffle_epi32(maxMinColor, R_SHUFFLE_D(2, 3, 0, 1)));
  color23 = _mm_mulhi_epi16(color23, *((__m128i *)SIMD_SSE2_word_div_by_3));
  color23 = _mm_packus_epi16(maxMinColor, color23);

  __m128i color23_2 = _mm_add_epi16(maxMinColor, maxMinColor);
  color23_2 = _mm_srli_epi16(color23_2, 1);
  color23_2 = _mm_unpacklo_epi64(color23_2, _mm_setzero_si128());

  __m128i comparison = _mm_cmpgt_epi16(_mm_shufflelo_epi16(data, 0), _mm_shufflelo_epi16(data, R_SHUFFLE_D(1, 1, 1, 1)));
  comparison = _mm_unpacklo_epi64(comparison, comparison);
  color23 = _mm_or_si128(_mm_and_si128(comparison, color23), _mm_andnot_si128(comparison, color23_2));

  color23 = _mm_packus_epi16(maxMinColor, color23);

  alignas(16) unsigned colors[4];
  _mm_store_si128((__m128i *)colors, color23);
  unsigned indices = _mm_cvtsi128_si32(_mm_shuffle_epi32(data, R_SHUFFLE_D(1, 1, 1, 1)));
  for (int y = 0; y < 4; ++y, rgba += stride)
  {
    ((unsigned *)rgba)[0] = colors[indices & 3];
    ((unsigned *)rgba)[1] = colors[(indices >> 2) & 3];
    ((unsigned *)rgba)[2] = colors[(indices >> 4) & 3];
    ((unsigned *)rgba)[3] = colors[(indices >> 6) & 3];
    indices >>= 8;
  }
}
template <bool setAlpha255> // otherwise set to zero
static inline void decompress_dxt5_color_block_sse(unsigned char *rgba, size_t stride, __m128i data)
{
  // get the block bytes
  // unpack the endpoints
  __m128i maxMinColor = _mm_srli_epi16(unpack_565_colors(data), 8);
  if (setAlpha255)
    maxMinColor = _mm_or_si128(maxMinColor, *(__m128i *)SIMD_SSE2_ALPHA_MASK);

  __m128i color23 = _mm_add_epi16(maxMinColor, maxMinColor);
  color23 = _mm_add_epi16(color23, _mm_shuffle_epi32(maxMinColor, R_SHUFFLE_D(2, 3, 0, 1)));
  color23 = _mm_mulhi_epi16(color23, *((__m128i *)SIMD_SSE2_word_div_by_3));
  color23 = _mm_packus_epi16(maxMinColor, color23);
  alignas(16) unsigned colors[4];
  _mm_store_si128((__m128i *)colors, color23);
  unsigned indices = _mm_cvtsi128_si32(_mm_shuffle_epi32(data, R_SHUFFLE_D(1, 1, 1, 1)));
  for (int y = 0; y < 4; ++y, rgba += stride)
  {
    ((unsigned *)rgba)[0] = colors[indices & 3];
    ((unsigned *)rgba)[1] = colors[(indices >> 2) & 3];
    ((unsigned *)rgba)[2] = colors[(indices >> 4) & 3];
    ((unsigned *)rgba)[3] = colors[(indices >> 6) & 3];
    indices >>= 8;
  }
}

void decompress_dxt1a(unsigned char *decompressedData, int lw, int lh, int row_pitch, unsigned char *src_data)
{
  if (lw > 8)
  {
    for (int y = 0; y < lh; y += 4, decompressedData += 3 * row_pitch)
      for (int x = 0; x < lw; x += 8, decompressedData += 32, src_data += 16)
      {
        __m128i data = _mm_loadu_si128((__m128i *)src_data);
        decompress_dxt1a_color_block_sse(decompressedData, row_pitch, data);
        data = _mm_unpackhi_epi64(data, data);
        decompress_dxt1a_color_block_sse(decompressedData + 16, row_pitch, data);
      }
  }
  else
  {
    for (int y = 0; y < lh; y += 4, decompressedData += 3 * row_pitch)
      for (int x = 0; x < lw; x += 4, decompressedData += 16, src_data += 8)
      {
        __m128i data = _mm_loadl_epi64((__m128i *)src_data);
        decompress_dxt1a_color_block_sse(decompressedData, row_pitch, data);
      }
  }
}

void decompress_dxt1(unsigned char *decompressedData, int lw, int lh, int row_pitch, unsigned char *src_data)
{
  if (lw > 8)
  {
    for (int y = 0; y < lh; y += 4, decompressedData += 3 * row_pitch)
      for (int x = 0; x < lw; x += 8, decompressedData += 32, src_data += 16)
      {
        __m128i data = _mm_loadu_si128((__m128i *)src_data);
        decompress_dxt5_color_block_sse<true>(decompressedData, row_pitch, data);
        data = _mm_unpackhi_epi64(data, data);
        decompress_dxt5_color_block_sse<true>(decompressedData + 16, row_pitch, data);
      }
  }
  else
  {
    for (int y = 0; y < lh; y += 4, decompressedData += 3 * row_pitch)
      for (int x = 0; x < lw; x += 4, decompressedData += 16, src_data += 8)
      {
        __m128i data = _mm_loadl_epi64((__m128i *)src_data);
        decompress_dxt5_color_block_sse<true>(decompressedData, row_pitch, data);
      }
  }
}

namespace imagefunctions
{
extern void downsample4x_simda(unsigned char *destData, const unsigned char *srcData, int destW, int destH);
extern void downsample4x_c(unsigned char *destData, const unsigned char *srcData, int destW, int destH);
}; // namespace imagefunctions

void decompress_dxt1_downsample4x(unsigned char *downsampledData, int lw, int lh, int row_pitch, unsigned char *src_data,
  unsigned char *decodedData)
{
  G_ASSERT(lw >= 4 && (lw & 3) == 0 && (lh & 3) == 0 && (row_pitch == lw * 4));
  for (int y = 0; y < lh; y += 4, downsampledData += row_pitch)
  {
    unsigned char *decompressedData = decodedData;
    for (int x = 0; x < lw; x += 4, decompressedData += 16, src_data += 8)
    {
      __m128i data = _mm_loadl_epi64((__m128i *)src_data);
      decompress_dxt5_color_block_sse<true>(decompressedData, row_pitch, data);
    }
    imagefunctions::downsample4x_simda(downsampledData, decodedData, lw / 2, 2);
  }
}

alignas(16) static unsigned short SIMD_SSE2_word_div_by_7[8] = {(1 << 16) / 7 + 1, (1 << 16) / 7 + 1, (1 << 16) / 7 + 1,
  (1 << 16) / 7 + 1, (1 << 16) / 7 + 1, (1 << 16) / 7 + 1, (1 << 16) / 7 + 1, (1 << 16) / 7 + 1};

alignas(16) static unsigned short SIMD_SSE2_word_div_by_5[8] = {(1 << 16) / 5 + 1, (1 << 16) / 5 + 1, (1 << 16) / 5 + 1,
  (1 << 16) / 5 + 1, (1 << 16) / 5 + 1, (1 << 16) / 5 + 1, (1 << 16) / 5 + 1, (1 << 16) / 5 + 1};

alignas(16) static unsigned short SIMD_SSE2_word_70654321[8] = {7, 0, 6, 5, 4, 3, 2, 1};
alignas(16) static unsigned short SIMD_SSE2_word_07123456[8] = {0, 7, 1, 2, 3, 4, 5, 6};

alignas(16) static unsigned short SIMD_SSE2_word_50432100[8] = {5, 0, 4, 3, 2, 1, 0, 0};
alignas(16) static unsigned short SIMD_SSE2_word_05123400[8] = {0, 5, 1, 2, 3, 4, 0, 0};

alignas(16) static unsigned short SIMD_SSE2_word_00FF0000000000[8] = {0, 0, 0, 0, 0, 0, 0, 0x00FF};

void decompress_dxt5(unsigned char *decompressedData, int lw, int lh, int row_pitch, unsigned char *src_data)
{
  for (int y = 0; y < lh; y += 4, decompressedData += 3 * row_pitch)
    for (int x = 0; x < lw; x += 4, decompressedData += 16, src_data += 16)
    {
      __m128i data = _mm_loadu_si128((__m128i *)src_data);
      decompress_dxt5_color_block_sse<false>(decompressedData, row_pitch, _mm_unpackhi_epi64(data, data));
      alignas(16) unsigned char alphas[8];

      __m128i zero = _mm_setzero_si128();
      __m128i alpha0, alpha1;
      // alpha0 = _mm_cvtsi32_si128(src_data[0]);
      alpha0 = _mm_unpacklo_epi8(data, zero);
      alpha0 = _mm_unpacklo_epi16(alpha0, alpha0);
      alpha0 = _mm_shuffle_epi32(alpha0, 0);

      // alpha1 = _mm_cvtsi32_si128(src_data[1]);
      alpha1 = _mm_unpacklo_epi8(_mm_srli_epi16(data, 8), zero);
      alpha1 = _mm_unpacklo_epi16(alpha1, alpha1);
      alpha1 = _mm_shuffle_epi32(alpha1, 0);
      __m128i allAlpha;
      allAlpha = _mm_add_epi16(_mm_mullo_epi16(alpha0, *(__m128i *)SIMD_SSE2_word_70654321),
        _mm_mullo_epi16(alpha1, *(__m128i *)SIMD_SSE2_word_07123456));
      allAlpha = _mm_mulhi_epu16(allAlpha, *((__m128i *)SIMD_SSE2_word_div_by_7)); // division by 7
      __m128i allAlphamx;
      allAlphamx = _mm_add_epi16(_mm_mullo_epi16(alpha0, *(__m128i *)SIMD_SSE2_word_50432100),
        _mm_mullo_epi16(alpha1, *(__m128i *)SIMD_SSE2_word_05123400));
      allAlphamx = _mm_mulhi_epu16(allAlphamx, *((__m128i *)SIMD_SSE2_word_div_by_5)); // division by 5
      allAlphamx = _mm_or_si128(allAlphamx, *((__m128i *)SIMD_SSE2_word_00FF0000000000));

      __m128i comparison = _mm_cmpgt_epi16(alpha0, alpha1);
      allAlpha = _mm_or_si128(_mm_and_si128(comparison, allAlpha), _mm_andnot_si128(comparison, allAlphamx));

      allAlpha = _mm_packus_epi16(allAlpha, allAlpha);
      _mm_storel_epi64((__m128i *)alphas, allAlpha);

      unsigned char *outData = decompressedData + 3;

      // uint64_t bits = *((uint64_t*)(src_data + 2));
      // for (int j = 0; j < 4; j++, outData += row_pitch-16)
      //   for (int i = 0; i < 4; i++, outData += 4, bits>>=3)
      //     *outData = alphas[bits & 0x07];
      uint32_t bits = *((uint64_t *)(src_data + 2));
#define UNPACK_ROW()                                    \
  for (int i = 0; i < 4; i++, outData += 4, bits >>= 3) \
    *outData = alphas[bits & 0x07];

      UNPACK_ROW()
      outData += row_pitch - 16;
      UNPACK_ROW()
      outData += row_pitch - 16;

      bits = *((uint32_t *)(src_data + 5));
      UNPACK_ROW()
      outData += row_pitch - 16;
      UNPACK_ROW()
#undef UNPACK_ROW
    }
}

void decompress_dxt5_downsample4x(unsigned char *downsampledData, int lw, int lh, int row_pitch, unsigned char *src_data,
  unsigned char *decodedData)
{
  G_ASSERT(lw >= 4 && (lw & 3) == 0 && (lh & 3) == 0 && (row_pitch == lw * 4));
  for (int y = 0; y < lh; y += 4, downsampledData += row_pitch)
  {
    unsigned char *decompressedData = decodedData;
    for (int x = 0; x < lw; x += 4, decompressedData += 16, src_data += 16)
    {
      __m128i data = _mm_loadu_si128((__m128i *)src_data);
      decompress_dxt5_color_block_sse<false>(decompressedData, row_pitch, _mm_unpackhi_epi64(data, data));
      alignas(16) unsigned char alphas[8];

      __m128i zero = _mm_setzero_si128();
      __m128i alpha0, alpha1;
      // alpha0 = _mm_cvtsi32_si128(src_data[0]);
      alpha0 = _mm_unpacklo_epi8(data, zero);
      alpha0 = _mm_unpacklo_epi16(alpha0, alpha0);
      alpha0 = _mm_shuffle_epi32(alpha0, 0);

      // alpha1 = _mm_cvtsi32_si128(src_data[1]);
      alpha1 = _mm_unpacklo_epi8(_mm_srli_epi16(data, 8), zero);
      alpha1 = _mm_unpacklo_epi16(alpha1, alpha1);
      alpha1 = _mm_shuffle_epi32(alpha1, 0);
      __m128i allAlpha;
      allAlpha = _mm_add_epi16(_mm_mullo_epi16(alpha0, *(__m128i *)SIMD_SSE2_word_70654321),
        _mm_mullo_epi16(alpha1, *(__m128i *)SIMD_SSE2_word_07123456));
      allAlpha = _mm_mulhi_epu16(allAlpha, *((__m128i *)SIMD_SSE2_word_div_by_7)); // division by 7
      __m128i allAlphamx;
      allAlphamx = _mm_add_epi16(_mm_mullo_epi16(alpha0, *(__m128i *)SIMD_SSE2_word_50432100),
        _mm_mullo_epi16(alpha1, *(__m128i *)SIMD_SSE2_word_05123400));
      allAlphamx = _mm_mulhi_epu16(allAlphamx, *((__m128i *)SIMD_SSE2_word_div_by_5)); // division by 5
      allAlphamx = _mm_or_si128(allAlphamx, *((__m128i *)SIMD_SSE2_word_00FF0000000000));

      __m128i comparison = _mm_cmpgt_epi16(alpha0, alpha1);
      allAlpha = _mm_or_si128(_mm_and_si128(comparison, allAlpha), _mm_andnot_si128(comparison, allAlphamx));

      allAlpha = _mm_packus_epi16(allAlpha, allAlpha);
      _mm_storel_epi64((__m128i *)alphas, allAlpha);

      unsigned char *outData = decompressedData + 3;

      // uint64_t bits = *((uint64_t*)(src_data + 2));
      // for (int j = 0; j < 4; j++, outData += row_pitch-16)
      //   for (int i = 0; i < 4; i++, outData += 4, bits>>=3)
      //     *outData = alphas[bits & 0x07];
      uint32_t bits = *((uint64_t *)(src_data + 2));
#define UNPACK_ROW()                                    \
  for (int i = 0; i < 4; i++, outData += 4, bits >>= 3) \
    *outData = alphas[bits & 0x07];

      UNPACK_ROW()
      outData += row_pitch - 16;
      UNPACK_ROW()
      outData += row_pitch - 16;

      bits = *((uint32_t *)(src_data + 5));
      UNPACK_ROW()
      outData += row_pitch - 16;
      UNPACK_ROW()
#undef UNPACK_ROW
    }
    imagefunctions::downsample4x_simda(downsampledData, decodedData, lw / 2, 2);
  }
}

void decompress_dxt3(unsigned char *decompressedData, int lw, int lh, int row_pitch, unsigned char *src_data)
{
  for (int y = 0; y < lh; y += 4, decompressedData += 3 * row_pitch)
    for (int x = 0; x < lw; x += 4, decompressedData += 16, src_data += 16)
    {
      __m128i data = _mm_loadl_epi64((__m128i *)(src_data + 8));
      decompress_dxt5_color_block_sse<false>(decompressedData, row_pitch, data);

      unsigned offset = 3;
      int id = 0, bitshift = 4;
      for (int j = 0; j < 4; j++, offset += row_pitch - 16)
        for (int i = 0; i < 4; i++, offset += 4, id++, bitshift = (~bitshift) & 4)
          decompressedData[offset] = (src_data[id >> 1] << bitshift);
    }
}

void decompress_dxt3_downsample4x(unsigned char *downsampledData, int lw, int lh, int row_pitch, unsigned char *src_data,
  unsigned char *decodedData)
{
  G_ASSERT(lw >= 4 && (lw & 3) == 0 && (lh & 3) == 0 && (row_pitch == lw * 4));
  for (int y = 0; y < lh; y += 4, downsampledData += row_pitch)
  {
    unsigned char *decompressedData = decodedData;
    for (int x = 0; x < lw; x += 4, decompressedData += 16, src_data += 16)
    {
      __m128i data = _mm_loadl_epi64((__m128i *)(src_data + 8));
      decompress_dxt5_color_block_sse<false>(decompressedData, row_pitch, data);

      unsigned offset = 3;
      int id = 0, bitshift = 4;
      for (int j = 0; j < 4; j++, offset += row_pitch - 16)
        for (int i = 0; i < 4; i++, offset += 4, id++, bitshift = (~bitshift) & 4)
          decompressedData[offset] = (src_data[id >> 1] << bitshift);
    }
    imagefunctions::downsample4x_simda(downsampledData, decodedData, lw / 2, 2);
  }
}

void decompress_dxt(unsigned char *decompressedData, int lw, int lh, int row_pitch, unsigned char *src_data, bool is_dxt1)
{
  if (is_dxt1) // src_fmt == D3DFMT_DXT1
    decompress_dxt1(decompressedData, lw, lh, row_pitch, src_data);
  else
    decompress_dxt5(decompressedData, lw, lh, row_pitch, src_data);
}
