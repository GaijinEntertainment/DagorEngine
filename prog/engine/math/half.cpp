// Branch-free implementation of half-precision (16 bit) floating point
// Copyright 2006 Mike Acton <macton@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE
//
// Half-precision floating point format
// ------------------------------------
//
//   | Field    | Last | First | Note
//   |----------|------|-------|----------
//   | Sign     | 15   | 15    |
//   | Exponent | 14   | 10    | Bias = 15
//   | Mantissa | 9    | 0     |
//
// Compiling
// ---------
//
//  Preferred compile flags for GCC:
//     -O3 -fstrict-aliasing -std=c99 -pedantic -Wall -Wstrict-aliasing
//
//     This file is a C99 source file, intended to be compiled with a C99
//     compliant compiler. However, for the moment it remains combatible
//     with C++98. Therefore if you are using a compiler that poorly implements
//     C standards (e.g. MSVC), it may be compiled as C++. This is not
//     guaranteed for future versions.
//

#include <math/dag_half.h>

// Load immediate
static inline uint32_t _uint32_li(uint32_t a) { return (a); }

// Decrement
static inline uint32_t _uint32_dec(uint32_t a) { return (a - 1); }

// Increment
static inline uint32_t _uint32_inc(uint32_t a) { return (a + 1); }

// Complement
static inline uint32_t _uint32_not(uint32_t a) { return (~a); }

// Negate
static inline uint32_t _uint32_neg(uint32_t a) { return (uint32_t)(-(int32_t)a); }

// Extend sign
static inline uint32_t _uint32_ext(uint32_t a) { return (((int32_t)a) >> 31); }

// And
static inline uint32_t _uint32_and(uint32_t a, uint32_t b) { return (a & b); }

// Exclusive Or
static inline uint32_t _uint32_xor(uint32_t a, uint32_t b) { return (a ^ b); }

// And with Complement
static inline uint32_t _uint32_andc(uint32_t a, uint32_t b) { return (a & ~b); }

// Or
static inline uint32_t _uint32_or(uint32_t a, uint32_t b) { return (a | b); }

// Shift Right Logical
static inline uint32_t _uint32_srl(uint32_t a, int sa) { return (a >> sa); }

// Shift Left Logical
static inline uint32_t _uint32_sll(uint32_t a, int sa) { return (a << sa); }

// Add
static inline uint32_t _uint32_add(uint32_t a, uint32_t b) { return (a + b); }

// Subtract
static inline uint32_t _uint32_sub(uint32_t a, uint32_t b) { return (a - b); }

// Multiply
static inline uint32_t _uint32_mul(uint32_t a, uint32_t b) { return (a * b); }

// Select on Sign bit
static inline uint32_t _uint32_sels(uint32_t test, uint32_t a, uint32_t b)
{
  const uint32_t mask = _uint32_ext(test);
  const uint32_t sel_a = _uint32_and(a, mask);
  const uint32_t sel_b = _uint32_andc(b, mask);
  const uint32_t result = _uint32_or(sel_a, sel_b);

  return (result);
}

// Select Bits on mask
static inline uint32_t _uint32_selb(uint32_t mask, uint32_t a, uint32_t b)
{
  const uint32_t sel_a = _uint32_and(a, mask);
  const uint32_t sel_b = _uint32_andc(b, mask);
  const uint32_t result = _uint32_or(sel_a, sel_b);

  return (result);
}

// Load Immediate
static inline uint16_t _uint16_li(uint16_t a) { return (a); }

// Extend sign
static inline uint16_t _uint16_ext(uint16_t a) { return (((int16_t)a) >> 15); }

// Negate
static inline uint16_t _uint16_neg(uint16_t a) { return (-a); }

// Complement
static inline uint16_t _uint16_not(uint16_t a) { return (~a); }

// Decrement
static inline uint16_t _uint16_dec(uint16_t a) { return (a - 1); }

// Shift Left Logical
static inline uint16_t _uint16_sll(uint16_t a, int sa) { return (a << sa); }

// Shift Right Logical
static inline uint16_t _uint16_srl(uint16_t a, int sa) { return (a >> sa); }

// Add
static inline uint16_t _uint16_add(uint16_t a, uint16_t b) { return (a + b); }

// Subtract
static inline uint16_t _uint16_sub(uint16_t a, uint16_t b) { return (a - b); }

// And
static inline uint16_t _uint16_and(uint16_t a, uint16_t b) { return (a & b); }

// Or
static inline uint16_t _uint16_or(uint16_t a, uint16_t b) { return (a | b); }

// Exclusive Or
static inline uint16_t _uint16_xor(uint16_t a, uint16_t b) { return (a ^ b); }

// And with Complement
static inline uint16_t _uint16_andc(uint16_t a, uint16_t b) { return (a & ~b); }

// And then Shift Right Logical
static inline uint16_t _uint16_andsrl(uint16_t a, uint16_t b, int sa) { return ((a & b) >> sa); }

// Shift Right Logical then Mask
static inline uint16_t _uint16_srlm(uint16_t a, int sa, uint16_t mask) { return ((a >> sa) & mask); }

// Add then Mask
static inline uint16_t _uint16_addm(uint16_t a, uint16_t b, uint16_t mask) { return ((a + b) & mask); }


// Select on Sign bit
static inline uint16_t _uint16_sels(uint16_t test, uint16_t a, uint16_t b)
{
  const uint16_t mask = _uint16_ext(test);
  const uint16_t sel_a = _uint16_and(a, mask);
  const uint16_t sel_b = _uint16_andc(b, mask);
  const uint16_t result = _uint16_or(sel_a, sel_b);

  return (result);
}

// Count Leading Zeros
static inline uint32_t _uint32_cntlz(uint32_t x)
{
#ifdef __GNUC__
  /* NOTE: __builtin_clz is undefined for x == 0 */
  /* On PowerPC, this will map to insn: cntlzw   */
  /* On Pentium, this will map to insn: clz      */
  uint32_t is_x_nez_msb = _uint32_neg(x);
  uint32_t nlz = __builtin_clz(x);
  uint32_t result = _uint32_sels(is_x_nez_msb, nlz, 0x00000020);
  return (result);
#else
  const uint32_t x0 = _uint32_srl(x, 1);
  const uint32_t x1 = _uint32_or(x, x0);
  const uint32_t x2 = _uint32_srl(x1, 2);
  const uint32_t x3 = _uint32_or(x1, x2);
  const uint32_t x4 = _uint32_srl(x3, 4);
  const uint32_t x5 = _uint32_or(x3, x4);
  const uint32_t x6 = _uint32_srl(x5, 8);
  const uint32_t x7 = _uint32_or(x5, x6);
  const uint32_t x8 = _uint32_srl(x7, 16);
  const uint32_t x9 = _uint32_or(x7, x8);
  const uint32_t xA = _uint32_not(x9);
  const uint32_t xB = _uint32_srl(xA, 1);
  const uint32_t xC = _uint32_and(xB, 0x55555555);
  const uint32_t xD = _uint32_sub(xA, xC);
  const uint32_t xE = _uint32_and(xD, 0x33333333);
  const uint32_t xF = _uint32_srl(xD, 2);
  const uint32_t x10 = _uint32_and(xF, 0x33333333);
  const uint32_t x11 = _uint32_add(xE, x10);
  const uint32_t x12 = _uint32_srl(x11, 4);
  const uint32_t x13 = _uint32_add(x11, x12);
  const uint32_t x14 = _uint32_and(x13, 0x0f0f0f0f);
  const uint32_t x15 = _uint32_srl(x14, 8);
  const uint32_t x16 = _uint32_add(x14, x15);
  const uint32_t x17 = _uint32_srl(x16, 16);
  const uint32_t x18 = _uint32_add(x16, x17);
  const uint32_t x19 = _uint32_and(x18, 0x0000003f);
  return (x19);
#endif
}

// Count Leading Zeros
static inline uint16_t _uint16_cntlz(uint16_t x)
{
#ifdef __GNUC__
  uint16_t nlz32 = (uint16_t)_uint32_cntlz((uint32_t)x);
  uint32_t nlz = _uint32_sub(nlz32, 16);
  return (nlz);
#else
  const uint16_t x0 = _uint16_srl(x, 1);
  const uint16_t x1 = _uint16_or(x, x0);
  const uint16_t x2 = _uint16_srl(x1, 2);
  const uint16_t x3 = _uint16_or(x1, x2);
  const uint16_t x4 = _uint16_srl(x3, 4);
  const uint16_t x5 = _uint16_or(x3, x4);
  const uint16_t x6 = _uint16_srl(x5, 8);
  const uint16_t x7 = _uint16_or(x5, x6);
  const uint16_t x8 = _uint16_not(x7);
  const uint16_t x9 = _uint16_srlm(x8, 1, 0x5555);
  const uint16_t xA = _uint16_sub(x8, x9);
  const uint16_t xB = _uint16_and(xA, 0x3333);
  const uint16_t xC = _uint16_srlm(xA, 2, 0x3333);
  const uint16_t xD = _uint16_add(xB, xC);
  const uint16_t xE = _uint16_srl(xD, 4);
  const uint16_t xF = _uint16_addm(xD, xE, 0x0f0f);
  const uint16_t x10 = _uint16_srl(xF, 8);
  const uint16_t x11 = _uint16_addm(xF, x10, 0x001f);
  return (x11);
#endif
}

extern "C" uint16_t half_from_float(uint32_t f)
{
  const uint32_t one = _uint32_li(0x00000001);
  const uint32_t f_s_mask = _uint32_li(0x80000000);
  const uint32_t f_e_mask = _uint32_li(0x7f800000);
  const uint32_t f_m_mask = _uint32_li(0x007fffff);
  const uint32_t f_m_hidden_bit = _uint32_li(0x00800000);
  const uint32_t f_m_round_bit = _uint32_li(0x00001000);
  const uint32_t f_snan_mask = _uint32_li(0x7fc00000);
  const uint32_t f_e_pos = _uint32_li(0x00000017);
  const uint32_t h_e_pos = _uint32_li(0x0000000a);
  const uint32_t h_e_mask = _uint32_li(0x00007c00);
  const uint32_t h_snan_mask = _uint32_li(0x00007e00);
  const uint32_t h_e_mask_value = _uint32_li(0x0000001f);
  const uint32_t f_h_s_pos_offset = _uint32_li(0x00000010);
  const uint32_t f_h_bias_offset = _uint32_li(0x00000070);
  const uint32_t f_h_m_pos_offset = _uint32_li(0x0000000d);
  const uint32_t h_nan_min = _uint32_li(0x00007c01);
  const uint32_t f_h_e_biased_flag = _uint32_li(0x0000008f);
  const uint32_t f_s = _uint32_and(f, f_s_mask);
  const uint32_t f_e = _uint32_and(f, f_e_mask);
  const uint16_t h_s = _uint32_srl(f_s, f_h_s_pos_offset);
  const uint32_t f_m = _uint32_and(f, f_m_mask);
  const uint16_t f_e_amount = _uint32_srl(f_e, f_e_pos);
  const uint32_t f_e_half_bias = _uint32_sub(f_e_amount, f_h_bias_offset);
  const uint32_t f_snan = _uint32_and(f, f_snan_mask);
  const uint32_t f_m_round_mask = _uint32_and(f_m, f_m_round_bit);
  const uint32_t f_m_round_offset = _uint32_sll(f_m_round_mask, one);
  const uint32_t f_m_rounded = _uint32_add(f_m, f_m_round_offset);
  const uint32_t f_m_denorm_sa = _uint32_sub(one, f_e_half_bias);
  const uint32_t f_m_with_hidden = _uint32_or(f_m_rounded, f_m_hidden_bit);
  const uint32_t f_m_denorm = _uint32_srl(f_m_with_hidden, f_m_denorm_sa);
  const uint32_t h_m_denorm = _uint32_srl(f_m_denorm, f_h_m_pos_offset);
  const uint32_t f_m_rounded_overflow = _uint32_and(f_m_rounded, f_m_hidden_bit);
  const uint32_t m_nan = _uint32_srl(f_m, f_h_m_pos_offset);
  const uint32_t h_em_nan = _uint32_or(h_e_mask, m_nan);
  const uint32_t h_e_norm_overflow_offset = _uint32_inc(f_e_half_bias);
  const uint32_t h_e_norm_overflow = _uint32_sll(h_e_norm_overflow_offset, h_e_pos);
  const uint32_t h_e_norm = _uint32_sll(f_e_half_bias, h_e_pos);
  const uint32_t h_m_norm = _uint32_srl(f_m_rounded, f_h_m_pos_offset);
  const uint32_t h_em_norm = _uint32_or(h_e_norm, h_m_norm);
  const uint32_t is_h_ndenorm_msb = _uint32_sub(f_h_bias_offset, f_e_amount);
  const uint32_t is_f_e_flagged_msb = _uint32_sub(f_h_e_biased_flag, f_e_half_bias);
  const uint32_t is_h_denorm_msb = _uint32_not(is_h_ndenorm_msb);
  const uint32_t is_f_m_eqz_msb = _uint32_dec(f_m);
  const uint32_t is_h_nan_eqz_msb = _uint32_dec(m_nan);
  const uint32_t is_f_inf_msb = _uint32_and(is_f_e_flagged_msb, is_f_m_eqz_msb);
  const uint32_t is_f_nan_underflow_msb = _uint32_and(is_f_e_flagged_msb, is_h_nan_eqz_msb);
  const uint32_t is_e_overflow_msb = _uint32_sub(h_e_mask_value, f_e_half_bias);
  const uint32_t is_h_inf_msb = _uint32_or(is_e_overflow_msb, is_f_inf_msb);
  const uint32_t is_f_nsnan_msb = _uint32_sub(f_snan, f_snan_mask);
  const uint32_t is_m_norm_overflow_msb = _uint32_neg(f_m_rounded_overflow);
  const uint32_t is_f_snan_msb = _uint32_not(is_f_nsnan_msb);
  const uint32_t h_em_overflow_result = _uint32_sels(is_m_norm_overflow_msb, h_e_norm_overflow, h_em_norm);
  const uint32_t h_em_nan_result = _uint32_sels(is_f_e_flagged_msb, h_em_nan, h_em_overflow_result);
  const uint32_t h_em_nan_underflow_result = _uint32_sels(is_f_nan_underflow_msb, h_nan_min, h_em_nan_result);
  const uint32_t h_em_inf_result = _uint32_sels(is_h_inf_msb, h_e_mask, h_em_nan_underflow_result);
  const uint32_t h_em_denorm_result = _uint32_sels(is_h_denorm_msb, h_m_denorm, h_em_inf_result);
  const uint32_t h_em_snan_result = _uint32_sels(is_f_snan_msb, h_snan_mask, h_em_denorm_result);
  const uint32_t h_result = _uint32_or(h_s, h_em_snan_result);

  return (uint16_t)(h_result);
}

extern "C" uint32_t half_to_float_uint32_t_ref(uint16_t h)
{
  const uint32_t h_e_mask = _uint32_li(0x00007c00);
  const uint32_t h_m_mask = _uint32_li(0x000003ff);
  const uint32_t h_s_mask = _uint32_li(0x00008000);
  const uint32_t h_f_s_pos_offset = _uint32_li(0x00000010);
  const uint32_t h_f_e_pos_offset = _uint32_li(0x0000000d);
  const uint32_t h_f_bias_offset = _uint32_li(0x0001c000);
  const uint32_t f_e_mask = _uint32_li(0x7f800000);
  const uint32_t f_m_mask = _uint32_li(0x007fffff);
  const uint32_t h_f_e_denorm_bias = _uint32_li(0x0000007e);
  const uint32_t h_f_m_denorm_sa_bias = _uint32_li(0x00000008);
  const uint32_t f_e_pos = _uint32_li(0x00000017);
  const uint32_t h_e_mask_minus_one = _uint32_li(0x00007bff);
  const uint32_t h_e = _uint32_and(h, h_e_mask);
  const uint32_t h_m = _uint32_and(h, h_m_mask);
  const uint32_t h_s = _uint32_and(h, h_s_mask);
  const uint32_t h_e_f_bias = _uint32_add(h_e, h_f_bias_offset);
  const uint32_t h_m_nlz = _uint32_cntlz(h_m);
  const uint32_t f_s = _uint32_sll(h_s, h_f_s_pos_offset);
  const uint32_t f_e = _uint32_sll(h_e_f_bias, h_f_e_pos_offset);
  const uint32_t f_m = _uint32_sll(h_m, h_f_e_pos_offset);
  const uint32_t f_em = _uint32_or(f_e, f_m);
  const uint32_t h_f_m_sa = _uint32_sub(h_m_nlz, h_f_m_denorm_sa_bias);
  const uint32_t f_e_denorm_unpacked = _uint32_sub(h_f_e_denorm_bias, h_f_m_sa);
  const uint32_t h_f_m = _uint32_sll(h_m, h_f_m_sa);
  const uint32_t f_m_denorm = _uint32_and(h_f_m, f_m_mask);
  const uint32_t f_e_denorm = _uint32_sll(f_e_denorm_unpacked, f_e_pos);
  const uint32_t f_em_denorm = _uint32_or(f_e_denorm, f_m_denorm);
  const uint32_t f_em_nan = _uint32_or(f_e_mask, f_m);
  const uint32_t is_e_eqz_msb = _uint32_dec(h_e);
  const uint32_t is_m_nez_msb = _uint32_neg(h_m);
  const uint32_t is_e_flagged_msb = _uint32_sub(h_e_mask_minus_one, h_e);
  const uint32_t is_zero_msb = _uint32_andc(is_e_eqz_msb, is_m_nez_msb);
  const uint32_t is_inf_msb = _uint32_andc(is_e_flagged_msb, is_m_nez_msb);
  const uint32_t is_denorm_msb = _uint32_and(is_m_nez_msb, is_e_eqz_msb);
  const uint32_t is_nan_msb = _uint32_and(is_e_flagged_msb, is_m_nez_msb);
  const uint32_t is_zero = _uint32_ext(is_zero_msb);
  const uint32_t f_zero_result = _uint32_andc(f_em, is_zero);
  const uint32_t f_denorm_result = _uint32_sels(is_denorm_msb, f_em_denorm, f_zero_result);
  const uint32_t f_inf_result = _uint32_sels(is_inf_msb, f_e_mask, f_denorm_result);
  const uint32_t f_nan_result = _uint32_sels(is_nan_msb, f_em_nan, f_inf_result);
  const uint32_t f_result = _uint32_or(f_s, f_nan_result);

  return (f_result);
}

// half_add
// --------
//
//  (SUM)        uint16_t z = half_add( x, y );
//  (DIFFERENCE) uint16_t z = half_add( x, -y );
//
//  * Difference of ZEROs is always +ZERO
//  * Sum round with guard + round + sticky bit (grs)
//  * QNaN + <x>  = QNaN
//  * <x>  + +INF = +INF
//  * <x>  - -INF = -INF
//  * INF  - INF  = SNaN
//
//  Will have exactly (0 ulps difference) the same result as:
//  (Round up)
//
//     union FLOAT_32
//     {
//       float    f32;
//       uint32_t u32;
//     };
//
//     union FLOAT_32 fx = { .u32 = half_to_float( x ) };
//     union FLOAT_32 fy = { .u32 = half_to_float( y ) };
//     union FLOAT_32 fz = { .f32 = fx.f32 + fy.f32    };
//     uint16_t       z  = float_to_half( fz );
//
extern "C" uint16_t half_add(uint16_t x, uint16_t y)
{
  const uint16_t one = _uint16_li(0x0001);
  const uint16_t msb_to_lsb_sa = _uint16_li(0x000f);
  const uint16_t h_s_mask = _uint16_li(0x8000);
  const uint16_t h_e_mask = _uint16_li(0x7c00);
  const uint16_t h_m_mask = _uint16_li(0x03ff);
  const uint16_t h_m_msb_mask = _uint16_li(0x2000);
  const uint16_t h_m_msb_sa = _uint16_li(0x000d);
  const uint16_t h_m_hidden = _uint16_li(0x0400);
  const uint16_t h_e_pos = _uint16_li(0x000a);
  const uint16_t h_e_bias_minus_one = _uint16_li(0x000e);
  const uint16_t h_m_grs_carry = _uint16_li(0x4000);
  const uint16_t h_m_grs_carry_pos = _uint16_li(0x000e);
  const uint16_t h_grs_size = _uint16_li(0x0003);
  const uint16_t h_snan = _uint16_li(0xfe00);
  const uint16_t h_e_mask_minus_one = _uint16_li(0x7bff);
  const uint16_t h_grs_round_carry = _uint16_sll(one, h_grs_size);
  const uint16_t h_grs_round_mask = _uint16_sub(h_grs_round_carry, one);
  const uint16_t x_e = _uint16_and(x, h_e_mask);
  const uint16_t y_e = _uint16_and(y, h_e_mask);
  const uint16_t is_y_e_larger_msb = _uint16_sub(x_e, y_e);
  const uint16_t a = _uint16_sels(is_y_e_larger_msb, y, x);
  const uint16_t a_s = _uint16_and(a, h_s_mask);
  const uint16_t a_e = _uint16_and(a, h_e_mask);
  const uint16_t a_m_no_hidden_bit = _uint16_and(a, h_m_mask);
  const uint16_t a_em_no_hidden_bit = _uint16_or(a_e, a_m_no_hidden_bit);
  const uint16_t b = _uint16_sels(is_y_e_larger_msb, x, y);
  const uint16_t b_s = _uint16_and(b, h_s_mask);
  const uint16_t b_e = _uint16_and(b, h_e_mask);
  const uint16_t b_m_no_hidden_bit = _uint16_and(b, h_m_mask);
  const uint16_t b_em_no_hidden_bit = _uint16_or(b_e, b_m_no_hidden_bit);
  const uint16_t is_diff_sign_msb = _uint16_xor(a_s, b_s);
  const uint16_t is_a_inf_msb = _uint16_sub(h_e_mask_minus_one, a_em_no_hidden_bit);
  const uint16_t is_b_inf_msb = _uint16_sub(h_e_mask_minus_one, b_em_no_hidden_bit);
  const uint16_t is_undenorm_msb = _uint16_dec(a_e);
  const uint16_t is_undenorm = _uint16_ext(is_undenorm_msb);
  const uint16_t is_both_inf_msb = _uint16_and(is_a_inf_msb, is_b_inf_msb);
  const uint16_t is_invalid_inf_op_msb = _uint16_and(is_both_inf_msb, b_s);
  const uint16_t is_a_e_nez_msb = _uint16_neg(a_e);
  const uint16_t is_b_e_nez_msb = _uint16_neg(b_e);
  const uint16_t is_a_e_nez = _uint16_ext(is_a_e_nez_msb);
  const uint16_t is_b_e_nez = _uint16_ext(is_b_e_nez_msb);
  const uint16_t a_m_hidden_bit = _uint16_and(is_a_e_nez, h_m_hidden);
  const uint16_t b_m_hidden_bit = _uint16_and(is_b_e_nez, h_m_hidden);
  const uint16_t a_m_no_grs = _uint16_or(a_m_no_hidden_bit, a_m_hidden_bit);
  const uint16_t b_m_no_grs = _uint16_or(b_m_no_hidden_bit, b_m_hidden_bit);
  const uint16_t diff_e = _uint16_sub(a_e, b_e);
  const uint16_t a_e_unbias = _uint16_sub(a_e, h_e_bias_minus_one);
  const uint16_t a_m = _uint16_sll(a_m_no_grs, h_grs_size);
  const uint16_t a_e_biased = _uint16_srl(a_e, h_e_pos);
  const uint16_t m_sa_unbias = _uint16_srl(a_e_unbias, h_e_pos);
  const uint16_t m_sa_default = _uint16_srl(diff_e, h_e_pos);
  const uint16_t m_sa_unbias_mask = _uint16_andc(is_a_e_nez_msb, is_b_e_nez_msb);
  const uint16_t m_sa = _uint16_sels(m_sa_unbias_mask, m_sa_unbias, m_sa_default);
  const uint16_t b_m_no_sticky = _uint16_sll(b_m_no_grs, h_grs_size);
  const uint16_t sh_m = _uint16_srl(b_m_no_sticky, m_sa);
  const uint16_t sticky_overflow = _uint16_sll(one, m_sa);
  const uint16_t sticky_mask = _uint16_dec(sticky_overflow);
  const uint16_t sticky_collect = _uint16_and(b_m_no_sticky, sticky_mask);
  const uint16_t is_sticky_set_msb = _uint16_neg(sticky_collect);
  const uint16_t sticky = _uint16_srl(is_sticky_set_msb, msb_to_lsb_sa);
  const uint16_t b_m = _uint16_or(sh_m, sticky);
  const uint16_t is_c_m_ab_pos_msb = _uint16_sub(b_m, a_m);
  const uint16_t c_inf = _uint16_or(a_s, h_e_mask);
  const uint16_t c_m_sum = _uint16_add(a_m, b_m);
  const uint16_t c_m_diff_ab = _uint16_sub(a_m, b_m);
  const uint16_t c_m_diff_ba = _uint16_sub(b_m, a_m);
  const uint16_t c_m_smag_diff = _uint16_sels(is_c_m_ab_pos_msb, c_m_diff_ab, c_m_diff_ba);
  const uint16_t c_s_diff = _uint16_sels(is_c_m_ab_pos_msb, a_s, b_s);
  const uint16_t c_s = _uint16_sels(is_diff_sign_msb, c_s_diff, a_s);
  const uint16_t c_m_smag_diff_nlz = _uint16_cntlz(c_m_smag_diff);
  const uint16_t diff_norm_sa = _uint16_sub(c_m_smag_diff_nlz, one);
  const uint16_t is_diff_denorm_msb = _uint16_sub(a_e_biased, diff_norm_sa);
  const uint16_t is_diff_denorm = _uint16_ext(is_diff_denorm_msb);
  const uint16_t is_a_or_b_norm_msb = _uint16_neg(a_e_biased);
  const uint16_t diff_denorm_sa = _uint16_dec(a_e_biased);
  const uint16_t c_m_diff_denorm = _uint16_sll(c_m_smag_diff, diff_denorm_sa);
  const uint16_t c_m_diff_norm = _uint16_sll(c_m_smag_diff, diff_norm_sa);
  const uint16_t c_e_diff_norm = _uint16_sub(a_e_biased, diff_norm_sa);
  const uint16_t c_m_diff_ab_norm = _uint16_sels(is_diff_denorm_msb, c_m_diff_denorm, c_m_diff_norm);
  const uint16_t c_e_diff_ab_norm = _uint16_andc(c_e_diff_norm, is_diff_denorm);
  const uint16_t c_m_diff = _uint16_sels(is_a_or_b_norm_msb, c_m_diff_ab_norm, c_m_smag_diff);
  const uint16_t c_e_diff = _uint16_sels(is_a_or_b_norm_msb, c_e_diff_ab_norm, a_e_biased);
  const uint16_t is_diff_eqz_msb = _uint16_dec(c_m_diff);
  const uint16_t is_diff_exactly_zero_msb = _uint16_and(is_diff_sign_msb, is_diff_eqz_msb);
  const uint16_t is_diff_exactly_zero = _uint16_ext(is_diff_exactly_zero_msb);
  const uint16_t c_m_added = _uint16_sels(is_diff_sign_msb, c_m_diff, c_m_sum);
  const uint16_t c_e_added = _uint16_sels(is_diff_sign_msb, c_e_diff, a_e_biased);
  const uint16_t c_m_carry = _uint16_and(c_m_added, h_m_grs_carry);
  const uint16_t is_c_m_carry_msb = _uint16_neg(c_m_carry);
  const uint16_t c_e_hidden_offset = _uint16_andsrl(c_m_added, h_m_grs_carry, h_m_grs_carry_pos);
  const uint16_t c_m_sub_hidden = _uint16_srl(c_m_added, one);
  const uint16_t c_m_no_hidden = _uint16_sels(is_c_m_carry_msb, c_m_sub_hidden, c_m_added);
  const uint16_t c_e_no_hidden = _uint16_add(c_e_added, c_e_hidden_offset);
  const uint16_t c_m_no_hidden_msb = _uint16_and(c_m_no_hidden, h_m_msb_mask);
  const uint16_t undenorm_m_msb_odd = _uint16_srl(c_m_no_hidden_msb, h_m_msb_sa);
  const uint16_t undenorm_fix_e = _uint16_and(is_undenorm, undenorm_m_msb_odd);
  const uint16_t c_e_fixed = _uint16_add(c_e_no_hidden, undenorm_fix_e);
  const uint16_t c_m_round_amount = _uint16_and(c_m_no_hidden, h_grs_round_mask);
  const uint16_t c_m_rounded = _uint16_add(c_m_no_hidden, c_m_round_amount);
  const uint16_t c_m_round_overflow = _uint16_andsrl(c_m_rounded, h_m_grs_carry, h_m_grs_carry_pos);
  const uint16_t c_e_rounded = _uint16_add(c_e_fixed, c_m_round_overflow);
  const uint16_t c_m_no_grs = _uint16_srlm(c_m_rounded, h_grs_size, h_m_mask);
  const uint16_t c_e = _uint16_sll(c_e_rounded, h_e_pos);
  const uint16_t c_em = _uint16_or(c_e, c_m_no_grs);
  const uint16_t c_normal = _uint16_or(c_s, c_em);
  const uint16_t c_inf_result = _uint16_sels(is_a_inf_msb, c_inf, c_normal);
  const uint16_t c_zero_result = _uint16_andc(c_inf_result, is_diff_exactly_zero);
  const uint16_t c_result = _uint16_sels(is_invalid_inf_op_msb, h_snan, c_zero_result);

  return (c_result);
}

// half_mul
// --------
//
//  May have 0 or 1 ulp difference from the following result:
//  (Round to nearest)
//  NOTE: Rounding mode differs between conversion and multiply
//
//     union FLOAT_32
//     {
//       float    f32;
//       uint32_t u32;
//     };
//
//     union FLOAT_32 fx = { .u32 = half_to_float( x ) };
//     union FLOAT_32 fy = { .u32 = half_to_float( y ) };
//     union FLOAT_32 fz = { .f32 = fx.f32 * fy.f32    };
//     uint16_t       z  = float_to_half( fz );
//
extern "C" uint16_t half_mul(uint16_t x, uint16_t y)
{
  const uint32_t one = _uint32_li(0x00000001);
  const uint32_t h_s_mask = _uint32_li(0x00008000);
  const uint32_t h_e_mask = _uint32_li(0x00007c00);
  const uint32_t h_m_mask = _uint32_li(0x000003ff);
  const uint32_t h_m_hidden = _uint32_li(0x00000400);
  const uint32_t h_e_pos = _uint32_li(0x0000000a);
  const uint32_t h_e_bias = _uint32_li(0x0000000f);
  const uint32_t h_m_bit_count = _uint32_li(0x0000000a);
  const uint32_t h_m_bit_half_count = _uint32_li(0x00000005);
  const uint32_t h_nan_min = _uint32_li(0x00007c01);
  const uint32_t h_e_mask_minus_one = _uint32_li(0x00007bff);
  const uint32_t h_snan = _uint32_li(0x0000fe00);
  const uint32_t m_round_overflow_bit = _uint32_li(0x00000020);
  const uint32_t m_hidden_bit = _uint32_li(0x00100000);
  const uint32_t a_s = _uint32_and(x, h_s_mask);
  const uint32_t b_s = _uint32_and(y, h_s_mask);
  const uint32_t c_s = _uint32_xor(a_s, b_s);
  const uint32_t x_e = _uint32_and(x, h_e_mask);
  const uint32_t x_e_eqz_msb = _uint32_dec(x_e);
  const uint32_t a = _uint32_sels(x_e_eqz_msb, y, x);
  const uint32_t b = _uint32_sels(x_e_eqz_msb, x, y);
  const uint32_t a_e = _uint32_and(a, h_e_mask);
  const uint32_t b_e = _uint32_and(b, h_e_mask);
  const uint32_t a_m = _uint32_and(a, h_m_mask);
  const uint32_t b_m = _uint32_and(b, h_m_mask);
  const uint32_t a_e_amount = _uint32_srl(a_e, h_e_pos);
  const uint32_t b_e_amount = _uint32_srl(b_e, h_e_pos);
  const uint32_t a_m_with_hidden = _uint32_or(a_m, h_m_hidden);
  const uint32_t b_m_with_hidden = _uint32_or(b_m, h_m_hidden);
  const uint32_t c_m_normal = _uint32_mul(a_m_with_hidden, b_m_with_hidden);
  const uint32_t c_m_denorm_biased = _uint32_mul(a_m_with_hidden, b_m);
  const uint32_t c_e_denorm_unbias_e = _uint32_sub(h_e_bias, a_e_amount);
  const uint32_t c_m_denorm_round_amount = _uint32_and(c_m_denorm_biased, h_m_mask);
  const uint32_t c_m_denorm_rounded = _uint32_add(c_m_denorm_biased, c_m_denorm_round_amount);
  const uint32_t c_m_denorm_inplace = _uint32_srl(c_m_denorm_rounded, h_m_bit_count);
  const uint32_t c_m_denorm_unbiased = _uint32_srl(c_m_denorm_inplace, c_e_denorm_unbias_e);
  const uint32_t c_m_denorm = _uint32_and(c_m_denorm_unbiased, h_m_mask);
  const uint32_t c_e_amount_biased = _uint32_add(a_e_amount, b_e_amount);
  const uint32_t c_e_amount_unbiased = _uint32_sub(c_e_amount_biased, h_e_bias);
  const uint32_t is_c_e_unbiased_underflow = _uint32_ext(c_e_amount_unbiased);
  const uint32_t c_e_underflow_half_sa = _uint32_neg(c_e_amount_unbiased);
  const uint32_t c_e_underflow_sa = _uint32_sll(c_e_underflow_half_sa, one);
  const uint32_t c_m_underflow = _uint32_srl(c_m_normal, c_e_underflow_sa);
  const uint32_t c_e_underflow_added = _uint32_andc(c_e_amount_unbiased, is_c_e_unbiased_underflow);
  const uint32_t c_m_underflow_added = _uint32_selb(is_c_e_unbiased_underflow, c_m_underflow, c_m_normal);
  const uint32_t is_mul_overflow_test = _uint32_and(c_e_underflow_added, m_round_overflow_bit);
  const uint32_t is_mul_overflow_msb = _uint32_neg(is_mul_overflow_test);
  const uint32_t c_e_norm_radix_corrected = _uint32_inc(c_e_underflow_added);
  const uint32_t c_m_norm_radix_corrected = _uint32_srl(c_m_underflow_added, one);
  const uint32_t c_m_norm_hidden_bit = _uint32_and(c_m_norm_radix_corrected, m_hidden_bit);
  const uint32_t is_c_m_norm_no_hidden_msb = _uint32_dec(c_m_norm_hidden_bit);
  const uint32_t c_m_norm_lo = _uint32_srl(c_m_norm_radix_corrected, h_m_bit_half_count);
  const uint32_t c_m_norm_lo_nlz = _uint16_cntlz(c_m_norm_lo);
  const uint32_t is_c_m_hidden_nunderflow_msb = _uint32_sub(c_m_norm_lo_nlz, c_e_norm_radix_corrected);
  const uint32_t is_c_m_hidden_underflow_msb = _uint32_not(is_c_m_hidden_nunderflow_msb);
  const uint32_t is_c_m_hidden_underflow = _uint32_ext(is_c_m_hidden_underflow_msb);
  const uint32_t c_m_hidden_underflow_normalized_sa = _uint32_srl(c_m_norm_lo_nlz, one);
  const uint32_t c_m_hidden_underflow_normalized = _uint32_sll(c_m_norm_radix_corrected, c_m_hidden_underflow_normalized_sa);
  const uint32_t c_m_hidden_normalized = _uint32_sll(c_m_norm_radix_corrected, c_m_norm_lo_nlz);
  const uint32_t c_e_hidden_normalized = _uint32_sub(c_e_norm_radix_corrected, c_m_norm_lo_nlz);
  const uint32_t c_e_hidden = _uint32_andc(c_e_hidden_normalized, is_c_m_hidden_underflow);
  const uint32_t c_m_hidden = _uint32_sels(is_c_m_hidden_underflow_msb, c_m_hidden_underflow_normalized, c_m_hidden_normalized);
  const uint32_t c_m_normalized = _uint32_sels(is_c_m_norm_no_hidden_msb, c_m_hidden, c_m_norm_radix_corrected);
  const uint32_t c_e_normalized = _uint32_sels(is_c_m_norm_no_hidden_msb, c_e_hidden, c_e_norm_radix_corrected);
  const uint32_t c_m_norm_round_amount = _uint32_and(c_m_normalized, h_m_mask);
  const uint32_t c_m_norm_rounded = _uint32_add(c_m_normalized, c_m_norm_round_amount);
  const uint32_t is_round_overflow_test = _uint32_and(c_e_normalized, m_round_overflow_bit);
  const uint32_t is_round_overflow_msb = _uint32_neg(is_round_overflow_test);
  const uint32_t c_m_norm_inplace = _uint32_srl(c_m_norm_rounded, h_m_bit_count);
  const uint32_t c_m = _uint32_and(c_m_norm_inplace, h_m_mask);
  const uint32_t c_e_norm_inplace = _uint32_sll(c_e_normalized, h_e_pos);
  const uint32_t c_e = _uint32_and(c_e_norm_inplace, h_e_mask);
  const uint32_t c_em_nan = _uint32_or(h_e_mask, a_m);
  const uint32_t c_nan = _uint32_or(a_s, c_em_nan);
  const uint32_t c_denorm = _uint32_or(c_s, c_m_denorm);
  const uint32_t c_inf = _uint32_or(c_s, h_e_mask);
  const uint32_t c_em_norm = _uint32_or(c_e, c_m);
  const uint32_t is_a_e_flagged_msb = _uint32_sub(h_e_mask_minus_one, a_e);
  const uint32_t is_b_e_flagged_msb = _uint32_sub(h_e_mask_minus_one, b_e);
  const uint32_t is_a_e_eqz_msb = _uint32_dec(a_e);
  const uint32_t is_a_m_eqz_msb = _uint32_dec(a_m);
  const uint32_t is_b_e_eqz_msb = _uint32_dec(b_e);
  const uint32_t is_b_m_eqz_msb = _uint32_dec(b_m);
  const uint32_t is_b_eqz_msb = _uint32_and(is_b_e_eqz_msb, is_b_m_eqz_msb);
  const uint32_t is_a_eqz_msb = _uint32_and(is_a_e_eqz_msb, is_a_m_eqz_msb);
  const uint32_t is_c_nan_via_a_msb = _uint32_andc(is_a_e_flagged_msb, is_b_e_flagged_msb);
  const uint32_t is_c_nan_via_b_msb = _uint32_andc(is_b_e_flagged_msb, is_b_m_eqz_msb);
  const uint32_t is_c_nan_msb = _uint32_or(is_c_nan_via_a_msb, is_c_nan_via_b_msb);
  const uint32_t is_c_denorm_msb = _uint32_andc(is_b_e_eqz_msb, is_a_e_flagged_msb);
  const uint32_t is_a_inf_msb = _uint32_and(is_a_e_flagged_msb, is_a_m_eqz_msb);
  const uint32_t is_c_snan_msb = _uint32_and(is_a_inf_msb, is_b_eqz_msb);
  const uint32_t is_c_nan_min_via_a_msb = _uint32_and(is_a_e_flagged_msb, is_b_eqz_msb);
  const uint32_t is_c_nan_min_via_b_msb = _uint32_and(is_b_e_flagged_msb, is_a_eqz_msb);
  const uint32_t is_c_nan_min_msb = _uint32_or(is_c_nan_min_via_a_msb, is_c_nan_min_via_b_msb);
  const uint32_t is_c_inf_msb = _uint32_or(is_a_e_flagged_msb, is_b_e_flagged_msb);
  const uint32_t is_overflow_msb = _uint32_or(is_round_overflow_msb, is_mul_overflow_msb);
  const uint32_t c_em_overflow_result = _uint32_sels(is_overflow_msb, h_e_mask, c_em_norm);
  const uint32_t c_common_result = _uint32_or(c_s, c_em_overflow_result);
  const uint32_t c_zero_result = _uint32_sels(is_b_eqz_msb, c_s, c_common_result);
  const uint32_t c_nan_result = _uint32_sels(is_c_nan_msb, c_nan, c_zero_result);
  const uint32_t c_nan_min_result = _uint32_sels(is_c_nan_min_msb, h_nan_min, c_nan_result);
  const uint32_t c_inf_result = _uint32_sels(is_c_inf_msb, c_inf, c_nan_min_result);
  const uint32_t c_denorm_result = _uint32_sels(is_c_denorm_msb, c_denorm, c_inf_result);
  const uint32_t c_result = _uint32_sels(is_c_snan_msb, h_snan, c_denorm_result);

  return (uint16_t)(c_result);
}
