#pragma once
#include <stdint.h>

alignas(16) static uint8_t SIMD_SSE2_byte_1[16] = { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 };
alignas(16) static uint8_t SIMD_SSE2_byte_2[16] = { 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02 };
alignas(16) static uint8_t SIMD_SSE2_byte_7[16] = { 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07 };
alignas(16) static uint16_t SIMD_SSE2_word_div_by_7[8] = { (1<<16)/7+1, (1<<16)/7+1, (1<<16)/7+1, (1<<16)/7+1, (1<<16)/7+1, (1<<16)/7+1, (1<<16)/7+1, (1<<16)/7+1 };
alignas(16) static uint16_t SIMD_SSE2_word_div_by_14[8] = { (1<<16)/14+1, (1<<16)/14+1, (1<<16)/14+1, (1<<16)/14+1, (1<<16)/14+1, (1<<16)/14+1, (1<<16)/14+1, (1<<16)/14+1 };
alignas(16) static uint16_t SIMD_SSE2_word_scale66554400[8] = { 6, 6, 5, 5, 4, 4, 0, 0 };
alignas(16) static uint16_t SIMD_SSE2_word_scale11223300[8] = { 1, 1, 2, 2, 3, 3, 0, 0 };
alignas(16) static uint32_t SIMD_SSE2_dword_alpha_bit_mask0[4] = { 7<<0, 0, 7<<0, 0 };
alignas(16) static uint32_t SIMD_SSE2_dword_alpha_bit_mask1[4] = { 7<<3, 0, 7<<3, 0 };
alignas(16) static uint32_t SIMD_SSE2_dword_alpha_bit_mask2[4] = { 7<<6, 0, 7<<6, 0 };
alignas(16) static uint32_t SIMD_SSE2_dword_alpha_bit_mask3[4] = { 7<<9, 0, 7<<9, 0 };
alignas(16) static uint32_t SIMD_SSE2_dword_alpha_bit_mask4[4] = { 7<<12, 0, 7<<12, 0 };
alignas(16) static uint32_t SIMD_SSE2_dword_alpha_bit_mask5[4] = { 7<<15, 0, 7<<15, 0 };
alignas(16) static uint32_t SIMD_SSE2_dword_alpha_bit_mask6[4] = { 7<<18, 0, 7<<18, 0 };
alignas(16) static uint32_t SIMD_SSE2_dword_alpha_bit_mask7[4] = { 7<<21, 0, 7<<21, 0 };

__forceinline void emitDXT5_alpha_block(__m128i alpha, __m128i min_alpha16, __m128i max_alpha16, uint32_t* __restrict outBuf_)//ouputs 8 byte
{
  __m128i max = _mm_shufflelo_epi16(max_alpha16, R_SHUFFLE_D(0, 0, 0, 0));
  max = _mm_shuffle_epi32(max, R_SHUFFLE_D(0, 0, 0, 0));

  __m128i min = _mm_shufflelo_epi16(min_alpha16, R_SHUFFLE_D(0, 0, 0, 0));
  min = _mm_shuffle_epi32(min, R_SHUFFLE_D(0, 0, 0, 0));

  // Compute the midpoint offset between any two interpolated alpha values.
  __m128i mid = _mm_sub_epi16(max, min);
  mid = _mm_mulhi_epi16(mid, *((__m128i*)SIMD_SSE2_word_div_by_14));

  // Compute the first midpoint.
  __m128i ab1 = min;
  ab1 = _mm_add_epi16(ab1, mid);
  ab1 = _mm_packus_epi16(ab1, ab1);

  // Compute the next three midpoints.
  __m128i max456 = _mm_mullo_epi16(max, *((__m128i*)SIMD_SSE2_word_scale66554400));
  __m128i min123 = _mm_mullo_epi16(min, *((__m128i*)SIMD_SSE2_word_scale11223300));
  __m128i ab234 = _mm_add_epi16(max456, min123);
  ab234 = _mm_mulhi_epi16(ab234, *((__m128i*)SIMD_SSE2_word_div_by_7));
  ab234 = _mm_add_epi16(ab234, mid);
  __m128i ab2 = _mm_shuffle_epi32(ab234, R_SHUFFLE_D(0, 0, 0, 0));
  ab2 = _mm_packus_epi16(ab2, ab2);
  __m128i ab3 = _mm_shuffle_epi32(ab234, R_SHUFFLE_D(1, 1, 1, 1));
  ab3 = _mm_packus_epi16(ab3, ab3);
  __m128i ab4 = _mm_shuffle_epi32(ab234, R_SHUFFLE_D(2, 2, 2, 2));
  ab4 = _mm_packus_epi16(ab4, ab4);

  // Compute the last three midpoints.
  __m128i max123 = _mm_mullo_epi16(max, *((__m128i*)SIMD_SSE2_word_scale11223300));
  __m128i min456 = _mm_mullo_epi16(min, *((__m128i*)SIMD_SSE2_word_scale66554400));
  __m128i ab567 = _mm_add_epi16(max123, min456);
  ab567 = _mm_mulhi_epi16(ab567, *((__m128i*)SIMD_SSE2_word_div_by_7));
  ab567 = _mm_add_epi16(ab567, mid);
  __m128i ab5 = _mm_shuffle_epi32(ab567, R_SHUFFLE_D(2, 2, 2, 2));
  ab5 = _mm_packus_epi16(ab5, ab5);
  __m128i ab6 = _mm_shuffle_epi32(ab567, R_SHUFFLE_D(1, 1, 1, 1));
  ab6 = _mm_packus_epi16(ab6, ab6);
  __m128i ab7 = _mm_shuffle_epi32(ab567, R_SHUFFLE_D(0, 0, 0, 0));
  ab7 = _mm_packus_epi16(ab7, ab7);

  // Compare the alpha values to the midpoints.
  __m128i b1 = _mm_min_epu8(ab1, alpha);
  b1 = _mm_cmpeq_epi8(b1, alpha);
  b1 = _mm_and_si128(b1, *((__m128i*)SIMD_SSE2_byte_1));
  __m128i b2 = _mm_min_epu8(ab2, alpha);
  b2 = _mm_cmpeq_epi8(b2, alpha);
  b2 = _mm_and_si128(b2, *((__m128i*)SIMD_SSE2_byte_1));
  __m128i b3 = _mm_min_epu8(ab3, alpha);
  b3 = _mm_cmpeq_epi8(b3, alpha);
  b3 = _mm_and_si128(b3, *((__m128i*)SIMD_SSE2_byte_1));
  __m128i b4 = _mm_min_epu8(ab4, alpha);
  b4 = _mm_cmpeq_epi8(b4, alpha);
  b4 = _mm_and_si128(b4, *((__m128i*)SIMD_SSE2_byte_1));
  __m128i b5 = _mm_min_epu8(ab5, alpha);
  b5 = _mm_cmpeq_epi8(b5, alpha);
  b5 = _mm_and_si128(b5, *((__m128i*)SIMD_SSE2_byte_1));
  __m128i b6 = _mm_min_epu8(ab6, alpha);
  b6 = _mm_cmpeq_epi8(b6, alpha);
  b6 = _mm_and_si128(b6, *((__m128i*)SIMD_SSE2_byte_1));
  __m128i b7 = _mm_min_epu8(ab7, alpha);
  b7 = _mm_cmpeq_epi8(b7, alpha);
  b7 = _mm_and_si128(b7, *((__m128i*)SIMD_SSE2_byte_1));

  // Compute the alpha indexes.
  __m128i index = _mm_adds_epu8(b1, b2);
  index = _mm_adds_epu8(index, b3);
  index = _mm_adds_epu8(index, b4);
  index = _mm_adds_epu8(index, b5);
  index = _mm_adds_epu8(index, b6);
  index = _mm_adds_epu8(index, b7);

  // Convert natural index ordering to DXT index ordering.
  __m128i byte1 = _mm_load_si128((__m128i*)SIMD_SSE2_byte_1);
  index = _mm_adds_epu8(index, byte1);
  __m128i byte7 = _mm_load_si128((__m128i*)SIMD_SSE2_byte_7);
  index = _mm_and_si128(index, byte7);
  __m128i byte2 = _mm_load_si128((__m128i*)SIMD_SSE2_byte_2);
  __m128i swapMinMax = _mm_cmpgt_epi8(byte2, index);
  swapMinMax = _mm_and_si128(swapMinMax, byte1);
  index = _mm_xor_si128(index, swapMinMax);

  // Pack the 16 3-bit indices into 6 bytes.
  __m128i index0 = _mm_and_si128(index, *(__m128i*)SIMD_SSE2_dword_alpha_bit_mask0);
  __m128i index1 = _mm_srli_epi64(index, 8 - 3);
  index1 = _mm_and_si128(index1, *(__m128i*)SIMD_SSE2_dword_alpha_bit_mask1);
  __m128i index2 = _mm_srli_epi64(index, 16 - 6);
  index2 = _mm_and_si128(index2, *(__m128i*)SIMD_SSE2_dword_alpha_bit_mask2);
  __m128i index3 = _mm_srli_epi64(index, 24 - 9);
  index3 = _mm_and_si128(index3, *(__m128i*)SIMD_SSE2_dword_alpha_bit_mask3);
  __m128i index4 = _mm_srli_epi64(index, 32 - 12);
  index4 = _mm_and_si128(index4, *(__m128i*)SIMD_SSE2_dword_alpha_bit_mask4);
  __m128i index5 = _mm_srli_epi64(index, 40 - 15);
  index5 = _mm_and_si128(index5, *(__m128i*)SIMD_SSE2_dword_alpha_bit_mask5);
  __m128i index6 = _mm_srli_epi64(index, 48 - 18);
  index6 = _mm_and_si128(index6, *(__m128i*)SIMD_SSE2_dword_alpha_bit_mask6);
  __m128i index7 = _mm_srli_epi64(index, 56 - 21);
  index7 = _mm_and_si128(index7, *(__m128i*)SIMD_SSE2_dword_alpha_bit_mask7);

  index = _mm_or_si128(index0, index1);
  index = _mm_or_si128(index, index2);
  index = _mm_or_si128(index, index3);
  index = _mm_or_si128(index, index4);
  index = _mm_or_si128(index, index5);
  index = _mm_or_si128(index, index6);
  index = _mm_or_si128(index, index7);

  _mm_storel_epi64((__m128i*)outBuf_, _mm_unpacklo_epi32(
    _mm_unpacklo_epi16(_mm_unpacklo_epi8(max_alpha16, min_alpha16), index),
    _mm_or_si128(_mm_slli_epi32(_mm_shuffle_epi32(index, R_SHUFFLE_D(2, 3, 0, 1)), 8),
                 _mm_unpacklo_epi8(_mm_srli_epi32(index, 16), _mm_setzero_si128()))
  ));
}
