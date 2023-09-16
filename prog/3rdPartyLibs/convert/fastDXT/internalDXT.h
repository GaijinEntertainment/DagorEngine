#pragma once
#include <vecmath/dag_vecMath.h>
#include <string.h>

#define C565_5_MASK 0xF8 // 0xFF minus last three bits
#define C565_6_MASK 0xFC // 0xFF minus last two bits

#define INSET_SHIFT 4 // inset the bounding box with ( range >> shift )

#define R_SHUFFLE_D( x, y, z, w ) (( (w) & 3 ) << 6 | ( (z) & 3 ) << 4 | ( (y) & 3 ) << 2 | ( (x) & 3 ))


#if defined(__GNUC__)
#define   ALIGN16(_x)   _x __attribute((aligned(16)))
#else
#define   ALIGN16( x ) __declspec(align(16)) x
#endif

#ifdef DXT_INTR
#include <emmintrin.h>
#endif

namespace fastDXT {
__forceinline void ExtractBlockA( const unsigned char *__restrict inPtr_, int width, unsigned char *__restrict colorBlock_ )
{
  int  *__restrict inPtr = (int  *__restrict)inPtr_;
  uint32_t  *__restrict colorBlock = (uint32_t  *__restrict)colorBlock_;
  v_sti(colorBlock, v_ldi(inPtr));inPtr += width;
  v_sti((colorBlock + 4), v_ldi(inPtr));inPtr += width;
  v_sti((colorBlock + 8), v_ldi(inPtr));inPtr += width;
  v_sti((colorBlock + 12), v_ldi(inPtr));
}
__forceinline void ExtractBlockU( const unsigned char *__restrict inPtr_, int width, unsigned char *__restrict colorBlock_ )
{
  int  *__restrict inPtr = (int  *__restrict)inPtr_;
  uint32_t  *__restrict colorBlock = (uint32_t  *__restrict)colorBlock_;
  v_sti(colorBlock, v_ldui(inPtr));inPtr += width;
  v_sti((colorBlock + 4), v_ldui(inPtr));inPtr += width;
  v_sti((colorBlock + 8), v_ldui(inPtr));inPtr += width;
  v_sti((colorBlock + 12), v_ldui(inPtr));
}

#ifdef DXT_INTR
ALIGN16( static uint8_t SIMD_SSE2_byte_0[16] ) = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };


ALIGN16( static short SIMD_SSE2_word_31_63_31_0[8] ) = { 31, 63, 31, 0, 31, 63, 31, 0};
ALIGN16( static float SIMD_SSE2_float_3d31[4] ) = { 3.0f*31.0f/255.0f, 3.0f*63.0f/255.0f, 3.0f*31.0f/255.0f, 0};
ALIGN16( static uint16_t SIMD_SSE2_word_8_4_8[8] )   = { 8, 4, 8, 0, 8, 4, 8, 0};
ALIGN16( static uint16_t SIMD_SSE2_word_31_63_31[8] )   = { 31, 63, 31, 0, 31, 63, 31, 0};
ALIGN16( static uint16_t SIMD_SSE2_word_128[8] )   = { 128, 128, 128, 0, 128, 128, 128, 0};

static __forceinline __m128i cvt64bit_to16bit64(__m128i v_color)
{
//   return (stb__Mul8Bit(color[2],31) << 11) | (stb__Mul8Bit(color[1],63) << 5) | (stb__Mul8Bit(color[0],31) << 0);
  v_color = _mm_mullo_epi16(v_color, *((__m128i*)SIMD_SSE2_word_31_63_31));
  v_color = _mm_add_epi16(v_color, *((__m128i*)SIMD_SSE2_word_128));
  v_color = _mm_add_epi16(v_color, _mm_srli_epi16(v_color, 8));
  v_color = _mm_srli_epi16(v_color, 8);
  return _mm_mullo_epi16(v_color, *((__m128i*)SIMD_SSE2_word_8_4_8));//replace with slli?
}

static __forceinline __m128i cvt32bit_to16bit32(__m128i v_color)
{
  v_color = cvt64bit_to16bit64(_mm_unpacklo_epi8(v_color, *(__m128i*)fastDXT::SIMD_SSE2_byte_0));
  return _mm_packus_epi16(v_color, v_color);
}

__forceinline void stb__OptimizeColorBlock( const uint8_t *__restrict colorBlock, uint8_t *__restrict maxMinColor )
{
  // Compute the min/max of each column of pixels.
  __m128i min = _mm_load_si128((__m128i*)colorBlock);
  __m128i max = _mm_load_si128((__m128i*)colorBlock);
  min = _mm_min_epu8(min, *((__m128i*)(colorBlock + 16)));
  max = _mm_max_epu8(max, *((__m128i*)(colorBlock + 16)));
  min = _mm_min_epu8(min, *((__m128i*)(colorBlock + 32)));
  max = _mm_max_epu8(max, *((__m128i*)(colorBlock + 32)));
  min = _mm_min_epu8(min, *((__m128i*)(colorBlock + 48)));
  max = _mm_max_epu8(max, *((__m128i*)(colorBlock + 48)));

  // Compute the min/max of the 1st and 3rd DWORD and the 2nd and 4th DWORD.
  __m128i minShuf = _mm_shuffle_epi32(min, R_SHUFFLE_D(2, 3, 2, 3));
  __m128i maxShuf = _mm_shuffle_epi32(max, R_SHUFFLE_D(2, 3, 2, 3));
  min = _mm_min_epu8(min, minShuf);
  max = _mm_max_epu8(max, maxShuf);

  // Compute the min/max of the 1st and 2nd DWORD.
  minShuf = _mm_shufflelo_epi16(min, R_SHUFFLE_D(2, 3, 2, 3));
  maxShuf = _mm_shufflelo_epi16(max, R_SHUFFLE_D(2, 3, 2, 3));
  min = _mm_min_epu8(min, minShuf);
  max = _mm_max_epu8(max, maxShuf);

  min = _mm_unpacklo_epi8(min, *((__m128i*)SIMD_SSE2_byte_0));
  max = _mm_unpacklo_epi8(max, *((__m128i*)SIMD_SSE2_byte_0));
  //fast, but inaccurate
  //__m128i inset = _mm_sub_epi16(max, min);
  //inset = _mm_srli_epi16(inset, INSET_SHIFT);
  //accurate
  __m128i center = _mm_avg_epu16(max, min);
    __m128 cov = _mm_setzero_ps();
    center = _mm_unpacklo_epi16(center, *((__m128i*)SIMD_SSE2_byte_0));
    for (int i = 0; i < 4; i++)
    {
      __m128i colrow = _mm_load_si128((__m128i*)(colorBlock+i*16));
      __m128i col = _mm_unpacklo_epi8(colrow, *((__m128i*)SIMD_SSE2_byte_0));
      __m128 ccov1, ccov2, ccov;
      #define COL2()\
      ccov1 = _mm_cvtepi32_ps(_mm_sub_epi32(_mm_unpacklo_epi16(col, *((__m128i*)SIMD_SSE2_byte_0)),center));\
      ccov2 = _mm_cvtepi32_ps(_mm_sub_epi32(_mm_unpackhi_epi16(col, *((__m128i*)SIMD_SSE2_byte_0)),center));\
      ccov = _mm_shuffle_ps(ccov1, ccov2, _MM_SHUFFLE(1,0,1,0));\
      ccov2 = _mm_shuffle_ps(ccov1, ccov2, _MM_SHUFFLE(2,2,2,2));\
      cov = _mm_add_ps(cov, _mm_mul_ps(ccov, ccov2));
      COL2()
      col = _mm_unpackhi_epi8(colrow, *((__m128i*)SIMD_SSE2_byte_0));
      COL2()
    }
    cov = _mm_add_ps(cov, _mm_shuffle_ps(cov, cov, _MM_SHUFFLE(3,2,3,2)));
    //cov = _mm_shuffle_ps(_mm_setzero_ps(), cov, _MM_SHUFFLE(0,0,1,0));
    __m128i selmask = _mm_castps_si128(v_cmp_gt(_mm_setzero_ps(), cov));
    selmask = _mm_and_si128(selmask, ((__m128i)V_CI_MASK1100));
    selmask = _mm_packs_epi32(selmask,selmask);
    __m128i nmax = v_btseli(max, min, selmask);
    min = v_btseli(min, max, selmask);
    max = nmax;
  // Compute the inset value.
    __m128i inset = _mm_sub_epi16(max, min);
    inset = _mm_insert_epi16(inset, 0, 3);
  //
  inset = _mm_srai_epi16(inset, INSET_SHIFT);
  // Inset the bounding box.
  min = _mm_add_epi16(min, inset);
  max = _mm_sub_epi16(max, inset);

  // Store the bounding box.
  min = _mm_packus_epi16(min, min);
  max = _mm_packus_epi16(max, max);
  //_mm_storel_epi64((__m128i*)maxMinColor,  _mm_unpacklo_epi32(max, min));

  _mm_storel_epi64((__m128i*)maxMinColor, cvt32bit_to16bit32(_mm_unpacklo_epi32(max, min)));
}

static __forceinline int stb__RefineBlockIntrinsics(unsigned char *block, unsigned int *maxMin32, unsigned int mask)
{
   //static const int w1Tab[4] = { 3,0,2,1 };
   static const int prods[4] = { 0x090000,0x000900,0x040102,0x010402 };
   // ^some magic to save a lot of multiplies in the accumulating loop...
   // (precomputed products of weights for least squares system, accumulated inside one 32-bit register)
  ALIGN16(static const short w1Tab[8*4]) = { 3,3,3,3,3,3,3,3,0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1 };

   float frb,fg;
   int i, akku = 0, xx,xy,yy;
   unsigned int cm = mask;
   ALIGN16(int At1[4]);
   ALIGN16(int At2[4]);
   ALIGN16(unsigned maxMin[2]);
   _mm_storel_epi64((__m128i*)maxMin, _mm_loadl_epi64((__m128i*)maxMin32));

   if((mask ^ (mask<<2)) < 4) // all pixels have the same index?
   {
      // yes, linear system would be singular; solve using optimal
      // single-color match on average color
      __m128i rgb = _mm_set1_epi16(8);
      for (i=0;i<64;i+=8)
      {
        __m128i color = _mm_loadl_epi64((__m128i*)(block +  i));
        color = _mm_unpacklo_epi8(color, *(__m128i*)fastDXT::SIMD_SSE2_byte_0);
        rgb = _mm_add_epi16(rgb, color);
      }
      rgb = _mm_add_epi16(rgb, _mm_shuffle_epi32(rgb, R_SHUFFLE_D(2,3,0,1)));
      rgb = _mm_srli_epi16(rgb, 4);

      rgb = cvt64bit_to16bit64(rgb);

      _mm_storel_epi64((__m128i*)maxMin32, _mm_packus_epi16(rgb,rgb));
      //max16 = (stb__OMatch5[r][0]<<0) | (stb__OMatch6[g][0]<<5) | (stb__OMatch5[b][0] << 11);
      //min16 = (stb__OMatch5[r][1]<<0) | (stb__OMatch6[g][1]<<5) | (stb__OMatch5[b][1] << 11);
   } else {
      __m128i v_At2, v_At1;
      v_At2 = v_At1 = _mm_setzero_si128();
      for (i=0;i<64;i+=8,cm>>=4)
      {
        __m128i color = _mm_loadl_epi64((__m128i*)(block +  i));
        color = _mm_unpacklo_epi8(color, *(__m128i*)SIMD_SSE2_byte_0);
        int step1 = cm&3, step2 = (cm>>2)&3;
        __m128i v_w12 = ((__m128i*)w1Tab)[step1];
        v_w12 = _mm_unpacklo_epi64(v_w12, ((__m128i*)w1Tab)[step2]);
        akku    += prods[step1] + prods[step2];
        v_At1 = _mm_add_epi16(v_At1, _mm_mullo_epi16(color, v_w12));
        v_At2 = _mm_add_epi16(v_At2, color);
      }
      v_At1 = _mm_add_epi16(v_At1, _mm_shuffle_epi32(v_At1, R_SHUFFLE_D(2,3,0,1)));
      v_At2 = _mm_add_epi16(v_At2, _mm_shuffle_epi32(v_At2, R_SHUFFLE_D(2,3,0,1)));
      v_At1 = _mm_unpacklo_epi16(v_At1, *(__m128i*)SIMD_SSE2_byte_0);
      v_At2 = _mm_unpacklo_epi16(v_At2, *(__m128i*)SIMD_SSE2_byte_0);

      xx = akku >> 16;
      yy = (akku >> 8) & 0xff;
      xy = (akku >> 0) & 0xff;

      __m128 v_fr = v_splat_x(v_rcp_x(v_cvt_vec4f(_mm_cvtsi32_si128((xx*yy - xy*xy)))));
      v_fr = v_mul(v_fr, *(__m128*)SIMD_SSE2_float_3d31);

      //v_At2 = _mm_load_si128((__m128i*)At2);
      v_At2 = _mm_add_epi32(_mm_add_epi32(v_At2, v_At2), v_At2);
      v_At2 = _mm_sub_epi32(v_At2, v_At1);
      //v_At1 = _mm_load_si128((__m128i*)At1);
      __m128 v_At2f, v_At1f;
      v_At2f = _mm_cvtepi32_ps(v_At2);
      v_At1f = _mm_cvtepi32_ps(v_At1);

      // extract solutions and decide solvability

      //frb = (3.0f * 31.0f / 255.0f) / (xx*yy - xy*xy);
      //fg = frb * (63.0f / 31.0f);
      //__m128 v_fr = _mm_setr_ps(frb, fg, frb, 0);

      __m128 v_xy = _mm_cvtepi32_ps(_mm_set1_epi32(xy));
      __m128 v_yy = _mm_cvtepi32_ps(_mm_set1_epi32(yy));
      __m128 rgbfmax, rgbfmin;
      rgbfmax = v_sub(v_mul(v_At1f, v_yy), v_mul(v_At2f, v_xy));
      rgbfmax = v_mul(rgbfmax, v_fr);

      __m128 v_xx = _mm_cvtepi32_ps(_mm_set1_epi32(xx));
      rgbfmin = v_sub(v_mul(v_At2f, v_xx), v_mul(v_At1f, v_xy));
      rgbfmin = v_mul(rgbfmin, v_fr);
      __m128i rgbmax = _mm_cvtps_epi32(rgbfmax), rgbmin = _mm_cvtps_epi32(rgbfmin);
      __m128i rgbi = _mm_packs_epi32(rgbmax, rgbmin);
      rgbi = _mm_min_epi16(rgbi, *(__m128i*)SIMD_SSE2_word_31_63_31_0);
      rgbi = _mm_mullo_epi16(rgbi, *((__m128i*)SIMD_SSE2_word_8_4_8));//replace with slli?
      rgbi = _mm_packus_epi16(rgbi, rgbi);
      _mm_storel_epi64((__m128i*)maxMin32, rgbi);
   }
   if (maxMin32[0] == maxMin[0] && maxMin32[1] == maxMin[1])
     return false;
   return true;
}

ALIGN16( static uint16_t SIMD_SSE2_word_0[8] ) = { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 };
ALIGN16( static uint16_t SIMD_SSE2_word_1[8] ) = { 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001 };
ALIGN16( static uint16_t SIMD_SSE2_word_2[8] ) = { 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002 };
ALIGN16( static uint16_t SIMD_SSE2_word_div_by_3[8] ) = { (1<<16)/3+1, (1<<16)/3+1, (1<<16)/3+1, (1<<16)/3+1, (1<<16)/3+1, (1<<16)/3+1, (1<<16)/3+1, (1<<16)/3+1 };
ALIGN16( static uint8_t SIMD_SSE2_byte_colorMask[16] ) = { C565_5_MASK, C565_6_MASK, C565_5_MASK, 0x00, 0x00, 0x00, 0x00, 0x00, C565_5_MASK, C565_6_MASK, C565_5_MASK, 0x00, 0x00, 0x00, 0x00, 0x00 };

ALIGN16( static uint16_t SIMD_SSE2_word_colorMask16[8] ) = { (unsigned)C565_5_MASK, (unsigned)C565_6_MASK, (unsigned)C565_5_MASK, 0x00, (unsigned)C565_5_MASK, (unsigned)C565_6_MASK, (unsigned)C565_5_MASK, 0x00 };
//Use Manhattan distance
__forceinline unsigned stb__GetColorIndices_Intrinsics( const uint8_t *__restrict colorBlock, const uint8_t *__restrict maxminColor)
{
  const __m128i zero = _mm_setzero_si128();
  __m128i color01;
  ///*
  __m128i color0, color1;
  //const __m128i RGB565Mask = _mm_load_si128((__m128i*)SIMD_SSE2_byte_colorMask);
  color01 = _mm_loadl_epi64((__m128i*)maxminColor);
  color01 = _mm_unpacklo_epi8(color01, zero);

  color01 = _mm_and_si128(color01, *(__m128i*)SIMD_SSE2_word_colorMask16);

  __m128i redBlue = _mm_shufflelo_epi16(color01, R_SHUFFLE_D(0, 3, 2, 3));
  __m128i green = _mm_shufflelo_epi16(color01, R_SHUFFLE_D(3, 1, 3, 3));

  redBlue = _mm_shufflehi_epi16(redBlue, R_SHUFFLE_D(0, 3, 2, 3));
  green = _mm_shufflehi_epi16(green, R_SHUFFLE_D(3, 1, 3, 3));

  redBlue = _mm_srli_epi16(redBlue, 5);
  green = _mm_srli_epi16(green, 6);
  color01 = _mm_or_si128(color01, redBlue);
  color01 = _mm_or_si128(color01, green);

  __m128i color23 = _mm_add_epi16(color01, color01);
  color23 = _mm_add_epi16(color23, _mm_shuffle_epi32(color01, R_SHUFFLE_D(2,3,0,1)));
  color23 = _mm_mulhi_epi16(color23, *((__m128i*)SIMD_SSE2_word_div_by_3));

  color01 = _mm_packus_epi16(color01, zero);
  color23 = _mm_packus_epi16(color23, zero);

  color0 = _mm_shuffle_epi32(color01, R_SHUFFLE_D(0, 2, 0, 2));
  color1 = _mm_shuffle_epi32(color01, R_SHUFFLE_D(1, 2, 1, 2));
  __m128i color2,color3;
  color2 = _mm_shuffle_epi32(color23, R_SHUFFLE_D(0, 2, 0, 2));
  color3 = _mm_shuffle_epi32(color23, R_SHUFFLE_D(1, 2, 1, 2));

  // Assign a color index for each of the 16 colors in the colorblock.
  // This loop iterates twice (computes 8 indexes per iteration).
  __m128i result = zero;
  for(int i = 32; i >= 0; i -= 32)
  {
  	// Load 4 colors.
  	__m128i colorHi = _mm_loadl_epi64((__m128i*)(colorBlock + i));
  	colorHi = _mm_shuffle_epi32(colorHi, R_SHUFFLE_D(0, 2, 1, 3));
  	__m128i colorLo = _mm_loadl_epi64((__m128i*)(colorBlock + i + 8));
  	colorLo = _mm_shuffle_epi32(colorLo, R_SHUFFLE_D(0, 2, 1, 3));

  	// Compute the sum of absolute differences for each color.
  	__m128i dHi = _mm_sad_epu8(colorHi, color0);
  	__m128i dLo = _mm_sad_epu8(colorLo, color0);
  	__m128i d0 = _mm_packs_epi32(dHi, dLo);
  	dHi = _mm_sad_epu8(colorHi, color1);
  	dLo = _mm_sad_epu8(colorLo, color1);
  	__m128i d1 = _mm_packs_epi32(dHi, dLo);
  	dHi = _mm_sad_epu8(colorHi, color2);
  	dLo = _mm_sad_epu8(colorLo, color2);
  	__m128i d2 = _mm_packs_epi32(dHi, dLo);
  	dHi = _mm_sad_epu8(colorHi, color3);
  	dLo = _mm_sad_epu8(colorLo, color3);
  	__m128i d3 = _mm_packs_epi32(dHi, dLo);

  	// Load 4 more colors.
  	colorHi = _mm_loadl_epi64((__m128i*)(colorBlock + i + 16));
  	colorHi = _mm_shuffle_epi32(colorHi, R_SHUFFLE_D(0, 2, 1, 3));
  	colorLo = _mm_loadl_epi64((__m128i*)(colorBlock + i + 24));
  	colorLo = _mm_shuffle_epi32(colorLo, R_SHUFFLE_D(0, 2, 1, 3));

  	// Compute the sum of absolute differences for each color.  Pack result into previous 4 results.
  	dHi = _mm_sad_epu8(colorHi, color0);
  	dLo = _mm_sad_epu8(colorLo, color0);
  	dLo = _mm_packs_epi32(dHi, dLo);
  	d0 = _mm_packs_epi32(d0, dLo);
  	dHi = _mm_sad_epu8(colorHi, color1);
  	dLo = _mm_sad_epu8(colorLo, color1);
  	dLo = _mm_packs_epi32(dHi, dLo);
  	d1 = _mm_packs_epi32(d1, dLo);
  	dHi = _mm_sad_epu8(colorHi, color2);
  	dLo = _mm_sad_epu8(colorLo, color2);
  	dLo = _mm_packs_epi32(dHi, dLo);
  	d2 = _mm_packs_epi32(d2, dLo);
  	dHi = _mm_sad_epu8(colorHi, color3);
  	dLo = _mm_sad_epu8(colorLo, color3);
  	dLo = _mm_packs_epi32(dHi, dLo);
  	d3 = _mm_packs_epi32(d3, dLo);

  	// Compare the distances.
  	__m128i b0 = _mm_cmpgt_epi16(d0, d3);
  	__m128i b1 = _mm_cmpgt_epi16(d1, d2);
  	__m128i b2 = _mm_cmpgt_epi16(d0, d2);
  	__m128i b3 = _mm_cmpgt_epi16(d1, d3);
  	__m128i b4 = _mm_cmpgt_epi16(d2, d3);
  	__m128i b5 = _mm_cmpgt_epi16(d0, d1);

  	// Compute the color index.
  	__m128i x0 = _mm_and_si128(b2, b1);
  	__m128i x1 = _mm_and_si128(b3, b0);
  	__m128i x2 = _mm_and_si128(b4, b0);
  	__m128i x3 = _mm_andnot_si128(b1, b5);

  	__m128i indexBit0 = _mm_or_si128(x0, x1);
  	indexBit0 = _mm_and_si128(indexBit0, *((__m128i*)SIMD_SSE2_word_2));
  	__m128i indexBit1 = _mm_and_si128(_mm_or_si128(x2, x3), *((__m128i*)SIMD_SSE2_word_1));
  	//__m128i indexBit1 = _mm_and_si128(x2, *((__m128i*)SIMD_SSE2_word_1));
  	__m128i index = _mm_or_si128(indexBit1, indexBit0);

  	// Pack the index into the result.
  	__m128i indexHi = _mm_shuffle_epi32(index, R_SHUFFLE_D(2, 3, 0, 1));
  	indexHi = _mm_unpacklo_epi16(indexHi, *((__m128i*)SIMD_SSE2_word_0));
  	indexHi = _mm_slli_epi32(indexHi, 8);
  	__m128i indexLo = _mm_unpacklo_epi16(index, *((__m128i*)SIMD_SSE2_word_0));
  	result = _mm_slli_epi32(result, 16);
  	result = _mm_or_si128(result, indexHi);
  	result = _mm_or_si128(result, indexLo);
  }

  // Pack the 16 2-bit color indices into a single 32-bit value.
  __m128i result1 = _mm_shuffle_epi32(result, R_SHUFFLE_D(1, 2, 3, 0));
  __m128i result2 = _mm_shuffle_epi32(result, R_SHUFFLE_D(2, 3, 0, 1));
  __m128i result3 = _mm_shuffle_epi32(result, R_SHUFFLE_D(3, 0, 1, 2));
  result1 = _mm_slli_epi32(result1, 2);
  result2 = _mm_slli_epi32(result2, 4);
  result3 = _mm_slli_epi32(result3, 6);
  result = _mm_or_si128(result, result1);
  result = _mm_or_si128(result, result2);
  result = _mm_or_si128(result, result3);

  // Store the result.
  return _mm_cvtsi128_si32(result);
}

ALIGN16( static uint32_t SIMD_SSE2_dword_1[4] ) = { 0x0001, 0x0001, 0x0001, 0x0001};
ALIGN16( static uint32_t SIMD_SSE2_dword_2[4] ) = { 0x0002, 0x0002, 0x0002, 0x0002};
//Use Eucledian distance
__forceinline unsigned stb__GetColorIndices_IntrinsicsPrecise( const uint8_t *__restrict colorBlock, const uint8_t *__restrict color)
{
  const __m128i zero = _mm_setzero_si128();
  __m128i color01;
  color01 = _mm_loadl_epi64((__m128i*)color);
  color01 = _mm_unpacklo_epi8(color01, zero);
  //more precise version.
  //color01 = cvt64bit_to16bit64(color01);

  color01 = _mm_and_si128(color01, *(__m128i*)SIMD_SSE2_word_colorMask16);

  __m128i redBlue = _mm_shufflelo_epi16(color01, R_SHUFFLE_D(0, 3, 2, 3));
  __m128i green = _mm_shufflelo_epi16(color01, R_SHUFFLE_D(3, 1, 3, 3));

  redBlue = _mm_shufflehi_epi16(redBlue, R_SHUFFLE_D(0, 3, 2, 3));
  green = _mm_shufflehi_epi16(green, R_SHUFFLE_D(3, 1, 3, 3));

  redBlue = _mm_srli_epi16(redBlue, 5);
  green = _mm_srli_epi16(green, 6);
  color01 = _mm_or_si128(color01, redBlue);
  color01 = _mm_or_si128(color01, green);

  __m128i color23 = _mm_add_epi16(color01, color01);
  color23 = _mm_add_epi16(color23, _mm_shuffle_epi32(color01, R_SHUFFLE_D(2,3,0,1)));
  color23 = _mm_mulhi_epi16(color23, *((__m128i*)SIMD_SSE2_word_div_by_3));


  // Assign a color index for each of the 16 colors in the colorblock.
  // This loop iterates twice (computes 8 indexes per iteration).
  __m128i result = zero;
  __m128i color0 = _mm_shuffle_epi32(color01, R_SHUFFLE_D(0, 1, 0, 1));
  __m128i color1 = _mm_shuffle_epi32(color01, R_SHUFFLE_D(2, 3, 2, 3));
  __m128i color2 = _mm_shuffle_epi32(color23, R_SHUFFLE_D(0, 1, 0, 1));
  __m128i color3 = _mm_shuffle_epi32(color23, R_SHUFFLE_D(2, 3, 2, 3));
  //for(int i = 32; i >= 0; i -= 32)
  for(int i = 48; i >= 0; i -= 16)
  {
  	// Load 4 colors.
  	__m128i color = _mm_load_si128((__m128i*)(colorBlock + i));
        __m128i colorLo = _mm_unpacklo_epi8(color, zero);
        __m128i colorHi = _mm_unpackhi_epi8(color, zero);
        //cl.x*c0.x +cl.y*c0.y
        //cl.z*c0.z +0

        //ch.x*c0.x +ch.y*c0.y
        //ch.z*c0.z +0
        __m128i d0Lo = _mm_sub_epi16(colorLo, color0);
        d0Lo = _mm_madd_epi16(d0Lo, d0Lo);
        //d0Lo = _mm_add_epi32(d0Lo, _mm_shuffle_epi32(d0Lo, R_SHUFFLE_D(1, 0, 3, 2));
        d0Lo = _mm_add_epi32(_mm_shuffle_epi32(d0Lo, R_SHUFFLE_D(0, 2, 0, 0)), _mm_shuffle_epi32(d0Lo, R_SHUFFLE_D(1, 3, 0, 0)));

        __m128i d0Hi = _mm_sub_epi16(colorHi, color0);
        d0Hi = _mm_madd_epi16(d0Hi, d0Hi);
        d0Hi = _mm_add_epi32(_mm_shuffle_epi32(d0Hi, R_SHUFFLE_D(0, 2, 0, 0)), _mm_shuffle_epi32(d0Hi, R_SHUFFLE_D(1, 3, 0, 0)));
        __m128i d0 = _mm_unpacklo_epi32(d0Lo, d0Hi);//4 d0

        __m128i d1Lo = _mm_sub_epi16(colorLo, color1);
        d1Lo = _mm_madd_epi16(d1Lo, d1Lo);
        d1Lo = _mm_add_epi32(_mm_shuffle_epi32(d1Lo, R_SHUFFLE_D(0, 2, 0, 0)), _mm_shuffle_epi32(d1Lo, R_SHUFFLE_D(1, 3, 0, 0)));

        __m128i d1Hi = _mm_sub_epi16(colorHi, color1);
        d1Hi = _mm_madd_epi16(d1Hi, d1Hi);
        d1Hi = _mm_add_epi32(_mm_shuffle_epi32(d1Hi, R_SHUFFLE_D(0, 2, 0, 0)), _mm_shuffle_epi32(d1Hi, R_SHUFFLE_D(1, 3, 0, 0)));
        __m128i d1 = _mm_unpacklo_epi32(d1Lo, d1Hi);

        __m128i d2Lo = _mm_sub_epi16(colorLo, color2);
        d2Lo = _mm_madd_epi16(d2Lo, d2Lo);
        d2Lo = _mm_add_epi32(_mm_shuffle_epi32(d2Lo, R_SHUFFLE_D(0, 2, 0, 0)), _mm_shuffle_epi32(d2Lo, R_SHUFFLE_D(1, 3, 0, 0)));

        __m128i d2Hi = _mm_sub_epi16(colorHi, color2);
        d2Hi = _mm_madd_epi16(d2Hi, d2Hi);
        d2Hi = _mm_add_epi32(_mm_shuffle_epi32(d2Hi, R_SHUFFLE_D(0, 2, 0, 0)), _mm_shuffle_epi32(d2Hi, R_SHUFFLE_D(1, 3, 0, 0)));
        __m128i d2 = _mm_unpacklo_epi32(d2Lo, d2Hi);

        __m128i d3Lo = _mm_sub_epi16(colorLo, color3);
        d3Lo = _mm_madd_epi16(d3Lo, d3Lo);
        d3Lo = _mm_add_epi32(_mm_shuffle_epi32(d3Lo, R_SHUFFLE_D(0, 2, 0, 0)), _mm_shuffle_epi32(d3Lo, R_SHUFFLE_D(1, 3, 0, 0)));

        __m128i d3Hi = _mm_sub_epi16(colorHi, color3);
        d3Hi = _mm_madd_epi16(d3Hi, d3Hi);
        d3Hi = _mm_add_epi32(_mm_shuffle_epi32(d3Hi, R_SHUFFLE_D(0, 2, 0, 0)), _mm_shuffle_epi32(d3Hi, R_SHUFFLE_D(1, 3, 0, 0)));
        __m128i d3 = _mm_unpacklo_epi32(d3Lo, d3Hi);


  	// Compare the distances.
  	__m128i b0 = _mm_cmpgt_epi32(d0, d3);
  	__m128i b1 = _mm_cmpgt_epi32(d1, d2);
  	__m128i b2 = _mm_cmpgt_epi32(d0, d2);
  	__m128i b3 = _mm_cmpgt_epi32(d1, d3);
  	__m128i b4 = _mm_cmpgt_epi32(d2, d3);
   	__m128i b5 = _mm_cmpgt_epi16(d0, d1);

  	// Compute the color index.
  	__m128i x0 = _mm_and_si128(b2, b1);
  	__m128i x1 = _mm_and_si128(b3, b0);
  	__m128i x2 = _mm_and_si128(b4, b0);
  	__m128i x3 = _mm_andnot_si128(b1, b5);

  	__m128i indexBit0 = _mm_or_si128(x0, x1);
  	indexBit0 = _mm_and_si128(indexBit0, *((__m128i*)SIMD_SSE2_dword_2));
  	__m128i indexBit1 = _mm_and_si128(_mm_or_si128(x2, x3), *((__m128i*)SIMD_SSE2_dword_1));
  	__m128i index = _mm_or_si128(indexBit1, indexBit0);
  	result = _mm_slli_epi32(result, 8);
  	result = _mm_or_si128(result, index);

  	// Pack the index into the result.
  	/*__m128i indexHi = _mm_shuffle_epi32(index, R_SHUFFLE_D(2, 3, 0, 1));
  	indexHi = _mm_unpacklo_epi16(indexHi, *((__m128i*)SIMD_SSE2_word_0));
  	indexHi = _mm_slli_epi32(indexHi, 8);
  	__m128i indexLo = _mm_unpacklo_epi16(index, *((__m128i*)SIMD_SSE2_word_0));
  	result = _mm_slli_epi32(result, 16);
  	result = _mm_or_si128(result, indexHi);
  	result = _mm_or_si128(result, indexLo);*/
  }

  // Pack the 16 2-bit color indices into a single 32-bit value.
  __m128i result1 = _mm_shuffle_epi32(result, R_SHUFFLE_D(2, 2, 3, 0));
  __m128i result2 = _mm_shuffle_epi32(result, R_SHUFFLE_D(1, 3, 0, 1));
  __m128i result3 = _mm_shuffle_epi32(result, R_SHUFFLE_D(3, 0, 1, 2));
  result1 = _mm_slli_epi32(result1, 2);
  result2 = _mm_slli_epi32(result2, 4);
  result3 = _mm_slli_epi32(result3, 6);
  result = _mm_or_si128(result, result1);
  result = _mm_or_si128(result, result2);
  result = _mm_or_si128(result, result3);

  // Store the result.
  return _mm_cvtsi128_si32(result);
}

#endif

static inline void ExtractBlockSmall( const unsigned char *__restrict inPtr, int width, int height, unsigned char *__restrict colorBlock )
{
  memset(colorBlock,0,64);
  int blockW = width < 4 ? width : 4;
  int blockH = height < 4 ? height : 4;
  for ( int j = 0; j < blockH; j++ )
  {
    memcpy( &colorBlock[j*4*4], inPtr, blockW*4 );
    inPtr += width * 4;
  }
}

static inline void compress_dxt3_alpha(const unsigned char *__restrict block, unsigned char *__restrict outData)
{
  outData[0] = (block[ 3] >> 4) | (block[ 7] & 0xF0);
  outData[1] = (block[11] >> 4) | (block[15] & 0xF0);
  outData[2] = (block[19] >> 4) | (block[23] & 0xF0);
  outData[3] = (block[27] >> 4) | (block[31] & 0xF0);
  outData[4] = (block[35] >> 4) | (block[39] & 0xF0);
  outData[5] = (block[43] >> 4) | (block[47] & 0xF0);
  outData[6] = (block[51] >> 4) | (block[55] & 0xF0);
  outData[7] = (block[59] >> 4) | (block[63] & 0xF0);
}

void write_BC4_block_rgba( const uint8_t *__restrict block, const uint8_t minAlpha, const uint8_t maxAlpha, uint8_t *__restrict outData );
}
