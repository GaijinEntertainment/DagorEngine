
#include <util/dag_globDef.h>
#include <math/dag_adjpow2.h>
#include <math/random/dag_random.h>
#include <debug/dag_debug.h>
#include "FFT_CPU_Simulation.h"
#include <vecmath/dag_vecMath.h>
#include <math/dag_hlsl_floatx.h>
#include "../shaders/fft_spectrum.hlsli"

#include <string.h>
#if _TARGET_C1 | _TARGET_C2

#endif


#define GAUSS_MAP_RESOLUTION (1 << MAX_FFT_RESOLUTION_GAUSS)
#define GAUSS_MAP_SIZE       ((GAUSS_MAP_RESOLUTION + 2) * (GAUSS_MAP_RESOLUTION + 1))

#define B_RETURN(a) \
  {                 \
    if (!(a))       \
      return false; \
  }

#define NVSDK_aligned_malloc(size, align) ((align) <= 16 ? memalloc(size) : nullptr)
#define SAFE_ALIGNED_FREE(p) \
  {                          \
    if (p)                   \
    {                        \
      memfree_anywhere(p);   \
      (p) = NULL;            \
    }                        \
  }

//------------------------------------------------------------------------------------
// Fast sincos from AMath library: Approximated Math from Intel. License rules allow to use this code for our purposes

#ifndef AM_PI
#define AM_PI (3.14159265358979323846)
#endif

#ifndef AM_TWO_PI
#define AM_TWO_PI (6.283185307179586)
#endif


#ifndef AM_E
#define AM_E (2.718281828459045)
#endif

#ifndef AM_PI
#define PI (3.14159265358979323846f)
#endif

#define DECLARE_INT_CONST(Name, Val) alignas(16) static const int Name##_PI32_[4] = {Val, Val, Val, Val};
#define DECLARE_CONST(Name, Val)     static const vec4f Name##_PS = {Val##f, Val##f, Val##f, Val##f};
DECLARE_INT_CONST(SIGN_MASK_SINGLE, static_cast<int>(0x80000000))
#define SIGN_MASK_SINGLE *(vec4f *)SIGN_MASK_SINGLE_PI32_
DECLARE_INT_CONST(INV_SIGN_MASK_SINGLE, 0x7fffffff)
#define INV_SIGN_MASK_SINGLE *(vec4f *)INV_SIGN_MASK_SINGLE_PI32_
DECLARE_INT_CONST(ONE, 0x00000001)
#define ONE_PI32 *(vec4i *)ONE_PI32_
DECLARE_INT_CONST(INVONE, ~0x00000001)
#define INVONE_PI32 *(vec4i *)INVONE_PI32_
DECLARE_INT_CONST(FOUR, 0x00000004)
#define FOUR_PI32 *(vec4i *)FOUR_PI32_
DECLARE_INT_CONST(TWO, 0x00000002)
#define TWO_PI32 *(vec4i *)TWO_PI32_
DECLARE_CONST(DP1, -0.78515625)
DECLARE_CONST(DP2, -2.4187564849853515625e-4)
DECLARE_CONST(DP3, -3.77489497744594108e-8)
DECLARE_CONST(COSCOF_P0, 2.443315711809948E-005)
DECLARE_CONST(COSCOF_P1, -1.388731625493765E-003)
DECLARE_CONST(COSCOF_P2, 4.166664568298827E-002)
DECLARE_CONST(SINCOF_P0, -1.9515295891E-4)
DECLARE_CONST(SINCOF_P1, 8.3321608736E-3)
DECLARE_CONST(SINCOF_P2, -1.6666654611E-1)
DECLARE_CONST(HALF, 0.5)
DECLARE_CONST(ONE, 1.0)
DECLARE_CONST(FOUR_OVER_PI, 1.27323954473516)
DECLARE_CONST(TWO_PI, 6.28318530718)

// HALVES - move to common

#if _TARGET_SIMD_SSE
#define abs_ps(x) v_and(x, (vec4f)INV_SIGN_MASK_SINGLE)
#else
#define abs_ps(x) v_abs(x)
#endif


static cpu_types::float2 *global_gauss_data = NULL; // We cache the Gaussian distribution which underlies h0 in order to avoid having
                                                    // to re-run the
enum
{
  FFT_RES_START = 6, // starting mip
  FFT_RES_64 = 0,
  FFT_RES_128 = 1,
  FFT_RES_256 = 2,
  FFT_RES_512 = 3,
  FFT_RES_NUM,
};
static float *global_sqrt_table[FFT_RES_NUM] = {NULL, NULL, NULL, NULL};

static constexpr int MAX_FFT_RES_LOG2 = 12; // no more than 4096 fft

static float global_c1_weight[MAX_FFT_RES_LOG2], global_c2_weight[MAX_FFT_RES_LOG2];

static void init_global_weights()
{
  double c1 = -1.0f;
  double c2 = 0.0f;
  for (int l = 0; l < MAX_FFT_RES_LOG2; ++l)
  {
    global_c1_weight[l] = (float)c1;
    global_c2_weight[l] = (float)c2;
    c2 = sqrt((1.0 - c1) / 2.0);
    c1 = sqrt((1.0 + c1) / 2.0);
  }
}

static void init_sqrt_table(int fft_res)
{
  if (fft_res < 0 || fft_res >= FFT_RES_NUM)
    return;
  if (global_sqrt_table[fft_res])
    return;
  const int N = 1 << (fft_res + FFT_RES_START);
  global_sqrt_table[fft_res] = (float *)NVSDK_aligned_malloc(N * N * sizeof(*global_sqrt_table[fft_res]), sizeof(vec4f));
  for (int y = 0; y < N; y++)
  {
    float ky = y - N * 0.5f + 0.5F;
    float ky2 = ky * ky;
    float kx = -0.5f * N + 0.5F;

    for (int x = 0; x < N; x++, kx += 1.0f)
    {
      float sqr_k = kx * kx + ky2;
      float s = 0.0f;
      if (sqr_k > 1e-12f)
        s = 1.0f / sqrtf(sqr_k);
      global_sqrt_table[fft_res][y * N + x] = s;
    }
  }
}

static inline float calcH0(int nx, int ny, cpu_types::float2 K, float window_in, float window_out, cpu_types::float2 wind_dir, float v,
  float dir_depend, float wind_align, float a, float small_wave_fraction, int spectrum)
{
  // distance from DC, in wave-numbers
  float nr = sqrtf(float(nx * nx) + float(ny * ny));

  float sw = 0;
  switch (spectrum)
  {
    case SPECTRUM_PHILLIPS:
      sw = calcPhillips(float2(K.x, K.y), float2(wind_dir.x, wind_dir.y), v, a, dir_depend, small_wave_fraction, wind_align);
      break;
    case SPECTRUM_UNIFIED_DIRECTIONAL:
      sw = calcUnifiedDirectional(float2(K.x, K.y), float2(wind_dir.x, wind_dir.y), v, a, dir_depend, small_wave_fraction, wind_align);
      break;
  }

  float amplitude = (K.x == 0 && K.y == 0) ? 0 : sqrtf(sw);
  amplitude = nr < window_in ? 0.0 : nr > window_out ? 0.0f : amplitude;

  return amplitude;
}

// random number generator when we re-calculate h0 (e.g. when windspeed changes)

static void init_omega(int fft_resolution_bits, float fft_period, int spectrum, float *pOutOmega);

// 4 components fast approximated sin and cos computation
inline void sincos_ps(vec4f x, vec4f *s, vec4f *c)
{
  vec4f xmm1, xmm2, xmm3, sign_bit_sin, y;
  vec4i emm0, emm2, emm4;

  sign_bit_sin = x;
  // take the absolute value
  x = abs_ps(x);
  // extract the sign bit (upper one)
  sign_bit_sin = v_and(sign_bit_sin, (vec4f)SIGN_MASK_SINGLE);
  // scale by 4/Pi
  y = v_mul(x, FOUR_OVER_PI_PS);
  // store the integer part of y in mm0
  emm2 = v_cvt_vec4i(y);
  // j=(j+1) & (~1) (see the cephes sources)
  emm2 = v_addi(emm2, ONE_PI32);
  emm2 = v_andi(emm2, INVONE_PI32);
  emm4 = emm2;
  y = v_cvt_vec4f(emm2);
  // get the swap sign flag for sine
  emm0 = v_andi(emm2, FOUR_PI32);
  emm0 = v_slli(emm0, 29);
  vec4f swap_sign_bit_sin = v_cast_vec4f(emm0);
  // get the polynomial selection mask:
  // there is one polynomial for 0 <= x <= Pi/4 and another one for Pi/4<x<=Pi/2
  // both branches will be computed
  emm2 = v_andi(emm2, TWO_PI32);
  emm2 = v_cmp_eqi(emm2, v_cast_vec4i(v_zero()));
  vec4f poly_mask = v_cast_vec4f(emm2);
  // the magic pass: "Extended precision modular arithmetic"
  // x = ((x - y * DP1) - y * DP2) - y * DP3
  xmm1 = v_mul(y, DP1_PS);
  xmm2 = v_mul(y, DP2_PS);
  xmm3 = v_mul(y, DP3_PS);
  x = v_add(x, xmm1);
  x = v_add(x, xmm2);
  x = v_add(x, xmm3);
  emm4 = v_subi(emm4, TWO_PI32);
  emm4 = v_andnoti(emm4, FOUR_PI32);
  emm4 = v_slli(emm4, 29);
  vec4f sign_bit_cos = v_cast_vec4f(emm4);
  sign_bit_sin = v_xor(sign_bit_sin, swap_sign_bit_sin);
  // evaluate the first polynomial (0 <= x <= Pi/4)
  y = COSCOF_P0_PS;
  vec4f z = v_mul(x, x);
  y = v_mul(y, z);
  y = v_add(y, COSCOF_P1_PS);
  y = v_mul(y, z);
  y = v_add(y, COSCOF_P2_PS);
  y = v_mul(y, z);
  y = v_mul(y, z);
  vec4f tmp = v_mul(z, HALF_PS);
  y = v_sub(y, tmp);
  y = v_add(y, ONE_PS);
  // evaluate the second polynomial (Pi/4 <= x <= 0)
  vec4f y2 = SINCOF_P0_PS;
  y2 = v_mul(y2, z);
  y2 = v_add(y2, SINCOF_P1_PS);
  y2 = v_mul(y2, z);
  y2 = v_add(y2, SINCOF_P2_PS);
  y2 = v_mul(y2, z);
  y2 = v_mul(y2, x);
  y2 = v_add(y2, x);
  // select the correct result from the two polynomials
  vec4f ysin2 = v_and(poly_mask, y2);
  vec4f ysin1 = v_andnot(poly_mask, y);
  y2 = v_sub(y2, ysin2);
  y = v_sub(y, ysin1);
  xmm1 = v_add(ysin1, ysin2);
  xmm2 = v_add(y, y2);
  // update the sign
  *s = v_xor(xmm1, sign_bit_sin);
  *c = v_xor(xmm2, sign_bit_cos);
}

// Performs a 1D FFT inplace given x- interleaved real/imaginary array of data
// FFT2D (non-SSE code) is left here in case we need compatibility with non-SSE CPUs
void FFTc(unsigned int nn, unsigned int m, float *x)
{
  unsigned int i, i1, j, k, i2, l, l1, l2;
  float c1, c2, tx, ty, t1, t2, u1, u2, z;

  // Do the bit reversal

  i2 = nn >> 1;
  j = 0;
  for (i = 0; i < nn - 1; i++)
  {
    if (i < j)
    {
      tx = x[i * 2];
      ty = x[i * 2 + 1];
      x[i * 2] = x[j * 2];
      x[i * 2 + 1] = x[j * 2 + 1];
      x[j * 2] = tx;
      x[j * 2 + 1] = ty;
    }
    k = i2;
    while (k <= j)
    {
      j -= k;
      k >>= 1;
    }
    j += k;
  }

  // Compute the FFT
  l2 = 1;
  for (l = 0; l < m; l++)
  {
    c1 = global_c1_weight[l];
    c2 = global_c2_weight[l];
    l1 = l2;
    l2 <<= 1;
    u1 = 1.0f;
    u2 = 0.0f;
    for (j = 0; j < l1; j++)
    {
      for (i = j; i < nn; i += l2)
      {
        i1 = i + l1;
        t1 = u1 * x[i1 * 2] - u2 * x[i1 * 2 + 1];
        t2 = u1 * x[i1 * 2 + 1] + u2 * x[i1 * 2];
        x[i1 * 2] = x[i * 2] - t1;
        x[i1 * 2 + 1] = x[i * 2 + 1] - t2;
        x[i * 2] += t1;
        x[i * 2 + 1] += t2;
      }
      z = u1 * c1 - u2 * c2;
      u2 = u1 * c2 + u2 * c1;
      u1 = z;
    }
  }
}

//   Perform a 2D FFT inplace given a complex 2D array
//   The size of the array (nx,nx)
//   FFT2D (non-SSE code) is left here in case we need compatibility with non-SSE CPUs
void FFT2D(complex *c, int nx)
{
  int i, j;
  float tre, tim;

  int m = get_log2w(nx);
  int nn = nx;

  for (j = 0; j < nx; j++)
  {
    FFTc(nn, m, (float *)&c[j << m]);
  }

  // 2D matrix transpose
  for (i = 0; i < nx - 1; i++)
  {
    for (j = i + 1; j < nx; j++)
    {
      int ji = ((j << m) + i);
      int ij = ((i << m) + j);
      tre = c[ji][0];
      tim = c[ji][1];
      c[ji][0] = c[ij][0];
      c[ji][1] = c[ij][1];
      c[ij][0] = tre;
      c[ij][1] = tim;
    }
  }
  // doing 1D FFT for rows
  for (j = 0; j < nx; j++)
  {
    FFTc(nn, m, (float *)&c[j << m]);
  }

  // 2D matrix transpose
  for (i = 0; i < nx - 1; i++)
  {
    for (j = i + 1; j < nx; j++)
    {
      int ji = ((j << m) + i);
      int ij = ((i << m) + j);
      tre = c[ji][0];
      tim = c[ji][1];
      c[ji][0] = c[ij][0];
      c[ji][1] = c[ij][1];
      c[ji][0] = tre;
      c[ji][1] = tim;
    }
  }
}

//   Performs a 1D FFT inplace given x- interleaved real/imaginary array of data,
//   data is aligned to 16bytes, data is arranged the following way:
//   real0,real1,real2,real3,imag0,imag1,imag2,imag3,real4,real5,real6,real7,imag4,imag5,imag6,imag7, etc

void FFTcSSE(unsigned int nn, unsigned int m, vec4f *x)
{
  unsigned int i, i1, j, k, i2, l, l1, l2;
  vec4f c1, c2, tx, ty, t1, t2, u1, u2, z;
  vec4f tmp1, tmp2;

  // Do the bit reversal

  i2 = nn >> 1;
  j = 0;
  for (i = 0; i < nn - 1; i++)
  {
    if (i < j)
    {
      tx = x[i * 2];
      ty = x[i * 2 + 1];
      x[i * 2] = x[j * 2];
      x[i * 2 + 1] = x[j * 2 + 1];
      x[j * 2] = tx;
      x[j * 2 + 1] = ty;
    }
    k = i2;
    while (k <= j)
    {
      j -= k;
      k >>= 1;
    }
    j += k;
  }

  // Compute the FFT
  l2 = 1;
  for (l = 0; l < m; l++)
  {
    c1 = v_splats(global_c1_weight[l]); // c1= -1.0f;
    c2 = v_splats(global_c2_weight[l]); // c2 = 0.0f;
    l1 = l2;
    l2 <<= 1;
    u1 = V_C_ONE;  // u1 = 1.0f;
    u2 = v_zero(); // u2 = 0.0f;
    for (j = 0; j < l1; j++)
    {
      for (i = j; i < nn; i += l2)
      {
        i1 = i + l1;
        tmp1 = v_mul(u1, x[i1 * 2]);
        tmp2 = v_mul(u2, x[i1 * 2 + 1]);
        t1 = v_sub(tmp1, tmp2); // t1 = u1 * x[i1*2] - u2 * x[i1*2+1];

        tmp1 = v_mul(u1, x[i1 * 2 + 1]);
        tmp2 = v_mul(u2, x[i1 * 2]);
        t2 = v_add(tmp1, tmp2);                  // t2 = u1 * x[i1*2+1] + u2 * x[i1*2];
        x[i1 * 2] = v_sub(x[i * 2], t1);         // x[i1*2] = x[i*2] - t1;
        x[i1 * 2 + 1] = v_sub(x[i * 2 + 1], t2); // x[i1*2+1] = x[i*2+1] - t2;
        x[i * 2] = v_add(x[i * 2], t1);          // x[i*2] += t1;
        x[i * 2 + 1] = v_add(x[i * 2 + 1], t2);  // x[i*2+1] += t2;
      }
      tmp1 = v_mul(u1, c1);
      tmp2 = v_mul(u2, c2);
      z = v_sub(tmp1, tmp2); // z =  u1 * c1 - u2 * c2;
      tmp1 = v_mul(u1, c2);
      tmp2 = v_mul(u2, c1);
      u2 = v_add(tmp1, tmp2); // u2 = u1 * c2 + u2 * c1;
      u1 = z;
    }
  }
}

//   Perform a 2D FFT inplace given a complex 2D array
//   The size of the array (nx,nx)
void FFT2DSSE(complex *c, int nx)
{
  vec4f iv_data[512 * 2];
  vec4f shuffledata0;
  vec4f shuffledata1;
  vec4f shuffledata2;
  vec4f shuffledata3;
  float *f0;
  float *f1;
  float *f2;
  float *f3;
  float *r;
  float *r0;
  float *r1;
  float *r2;
  float *r3;

  int m = get_log2w(nx);
  int nn = 1 << m;

  for (int jr = 0, j = 0; j < nx; j += 4, jr += nx * 4)
  {
    f0 = &(c[jr][0]);
    f1 = &(c[jr + nx][0]);
    f2 = &(c[jr + nx * 2][0]);
    f3 = &(c[jr + nx * 3][0]);
    for (int i = 0; i < nx; i++)
    {
      shuffledata0 = v_merge_hw(v_ldu_half(f0), v_ldu_half(f1)); // f0.x,f1.x, f0.y,f1.y
      shuffledata1 = v_merge_hw(v_ldu_half(f2), v_ldu_half(f3)); // f2.x,f3.x, f2.y,f3.y

      iv_data[i * 2] = v_perm_xyXY(shuffledata0, shuffledata1);     // xxxx
      iv_data[i * 2 + 1] = v_perm_zwZW(shuffledata0, shuffledata1); // yyyy
      // iv_data[i*2] = v_make_vec4f(*f0,*f1,*f2,*f3);
      // iv_data[i*2+1] = v_make_vec4f(*(f0+1),*(f1+1),*(f2+1),*(f3+1));
      f0 += 2;
      f1 += 2;
      f2 += 2;
      f3 += 2;
    }

    FFTcSSE(nn, m, iv_data);

    f0 = &(c[jr][0]);
    f1 = &(c[jr + nx][0]);
    f2 = &(c[jr + nx * 2][0]);
    f3 = &(c[jr + nx * 3][0]);
    for (int i = 0; i < nx; i++)
    {
      vec4f xxyy = v_merge_hw(iv_data[i * 2], iv_data[i * 2 + 1]); // xXyY
      vec4f zzww = v_merge_lw(iv_data[i * 2], iv_data[i * 2 + 1]); // zZwW
      v_stu_half(f0, xxyy);
      v_stu_half(f1, v_perm_zwxy(xxyy));
      v_stu_half(f2, zzww);
      v_stu_half(f3, v_perm_zwxy(zzww));
      f0 += 2;
      f1 += 2;
      f2 += 2;
      f3 += 2;
    }
  }
  for (int j = 0; j < nx; j += 4)
  {

    for (int ij = j, i = 0; i < nx; i++, ij += nx)
    {
      shuffledata0 = v_ld((float *)&(c[ij][0]));
      shuffledata1 = v_ld((float *)&(c[ij + 2][0]));
      iv_data[i * 2] = v_perm_xzXZ(shuffledata0, shuffledata1);
      iv_data[i * 2 + 1] = v_perm_ywYW(shuffledata0, shuffledata1);
    }
    FFTcSSE(nn, m, iv_data);
    f0 = (float *)&(c[j][0]);
    f1 = (float *)&(c[j + nx + 0][0]);
    vec4f *iv_data_ptr = iv_data;
    for (int i = 0; i < nx; i += 2, f0 += nx * 4, f1 += nx * 4, iv_data_ptr += 4)
    {
      vec4f xxyy0 = v_merge_hw(iv_data_ptr[0], iv_data_ptr[1]);
      vec4f zzww0 = v_merge_lw(iv_data_ptr[0], iv_data_ptr[1]);
      v_st(f0, xxyy0);
      v_st(f0 + 4, zzww0);

      vec4f xxyy1 = v_merge_hw(iv_data_ptr[2], iv_data_ptr[3]);
      vec4f zzww1 = v_merge_lw(iv_data_ptr[2], iv_data_ptr[3]);
      v_st(f1, xxyy1);
      v_st(f1 + 4, zzww1);
    }
  }
}

// Updates Ht to desired time. Each call computes one scan line from source spectrum into 3 textures
void NVWaveWorks_FFT_CPU_Simulation::UpdateHt(int row)
{
  int N = (1 << m_params.fft_resolution_bits);
  int nvsf_g_InWidth = N + 2;
  int in_index = row * nvsf_g_InWidth;
  int in_mindex = N * (nvsf_g_InWidth + 1) - in_index;

  float *omega = &m_omega_data[row << m_params.fft_resolution_bits];
  cpu_types::float2 *_k = &m_h0_data[in_index];
  cpu_types::float2 *_mk = &m_h0_data[in_mindex];
  float *sqt = &m_sqrt_table[row << m_params.fft_resolution_bits];
  cpu_types::float2 *out0 = (cpu_types::float2 *)m_fftCPU_io_buffer + (row << m_params.fft_resolution_bits);
  cpu_types::float2 *out1 = out0 + ((intptr_t)1 << (m_params.fft_resolution_bits << 1));
  cpu_types::float2 *out2 = out1 + ((intptr_t)1 << (m_params.fft_resolution_bits << 1));

  // some iterated values
  float ky = row - N * 0.5f + 0.5f;
  vec4f _ky = v_make_vec4f(-ky, ky, -ky, ky);
  float kx = -0.5f * N + 0.5f;
  vec4f kx0 = v_make_vec4f(-(kx + 0.0f), kx + 0.0f, -(kx + 1.0f), kx + 1.0f);
  vec4f kx1 = v_make_vec4f(-(kx + 2.0f), kx + 2.0f, -(kx + 3.0f), kx + 3.0f);
  vec4f kxinc = v_make_vec4f(-4.0f, 4.0f, -4.0f, 4.0f);

  double dt = m_doubletime / 6.28318530718;
  vec4f v_dt = v_splats(dt);

  // perform 4 pixels simultaneously
  for (int i = 0; i < int(N); i += 4)
  {
    /*ALIGN16_BEG float ALIGN16_END o[4];
    double odt0,odt1,odt2,odt3;

    odt0 = (double)omega[i+0]*dt;
    odt1 = (double)omega[i+1]*dt;
    odt2 = (double)omega[i+2]*dt;
    odt3 = (double)omega[i+3]*dt;

    o[0] = (float)(odt0 - (int)odt0);
    o[1] = (float)(odt1 - (int)odt1);
    o[2] = (float)(odt2 - (int)odt2);
    o[3] = (float)(odt3 - (int)odt3);

    *(vec4f*)o = v_mul( *(vec4f*)&o[0],TWO_PI_PS);
    sincos_ps( *(vec4f*)&o[0] ,&sn,&cn);*/
    //(/tim)

    vec4f sn, cn;
    vec4f odt;
    odt = v_mul(v_ld((const float *)&omega[i]), v_dt);
    vec4f o = v_sub(odt, v_cvt_vec4f(v_cvt_vec4i(odt)));
    o = v_mul(o, TWO_PI_PS);
    sincos_ps(o, &sn, &cn);

    //  do not ask how it's works. This code is produced by human-mind low-level optimizer
    // (comment from Nick Chirkov)
    vec4f a = v_ldu((const float *)&_mk[-i - 1]);
    vec4f mk = v_perm_zwxy(a); //_mm_shuffle_ps( a, a, _MM_SHUFFLE(1,0,3,2) );
    vec4f s01 = v_add(*(vec4f *)&_k[i + 0], mk);
    vec4f d01 = v_sub(*(vec4f *)&_k[i + 0], mk);

    a = v_ldu((const float *)&_mk[-i - 3]);
    mk = v_perm_zwxy(a); //_mm_shuffle_ps( a, a, _MM_SHUFFLE(1,0,3,2) );
    vec4f s23 = v_add(*(vec4f *)&_k[i + 2], mk);
    vec4f d23 = v_sub(*(vec4f *)&_k[i + 2], mk);

    vec4f sx = v_perm_xzXZ(s01, s23);
    vec4f sy = v_perm_ywYW(s01, s23);
    vec4f hx = v_sub(v_mul(sx, cn), v_mul(sy, sn));

    vec4f dx = v_perm_xzXZ(d01, d23); //_mm_shuffle_ps( d01, d23, _MM_SHUFFLE(2,0,2,0) );
    vec4f dy = v_perm_ywYW(d01, d23); //_mm_shuffle_ps( d01, d23, _MM_SHUFFLE(3,1,3,1) );
    vec4f hy = v_add(v_mul(dx, sn), v_mul(dy, cn));

    // height
    v_st(&out0[i + 0].x, v_merge_hw(hx, hy));
    v_st(&out0[i + 2].x, v_merge_lw(hx, hy));


    vec4f ss = v_ld(&sqt[i + 0]);
    vec4f b = v_mul(v_merge_hw(hy, hx), v_perm_xxyy(ss)); //_mm_shuffle_ps( ss, ss, _MM_SHUFFLE(1,1,0,0) ) );
    vec4f c = v_mul(v_merge_lw(hy, hx), v_perm_zzww(ss)); //_mm_shuffle_ps( ss, ss, _MM_SHUFFLE(3,3,2,2) ) );
    v_st(&out1[i + 0].x, v_mul(kx0, b));
    v_st(&out1[i + 2].x, v_mul(kx1, c));
    v_st(&out2[i + 0].x, v_mul(_ky, b));
    v_st(&out2[i + 2].x, v_mul(_ky, c));

    kx0 = v_add(kx0, kxinc);
    kx1 = v_add(kx1, kxinc);
  }
}

#if !defined(_GNU_SOURCE) && !_TARGET_ANDROID && !SONY_HAVE_SINCOS
static void sincosf(float rad, float *s, float *c)
{
  *s = sinf(rad);
  *c = cosf(rad);
}
#endif
void NVWaveWorks_FFT_CPU_Simulation::UpdateHtC(int row)
{
  int N = (1 << m_params.fft_resolution_bits);
  int nvsf_g_InWidth = N + 2;
  int in_index = row * nvsf_g_InWidth;
  int in_mindex = N * (nvsf_g_InWidth + 1) - in_index;

  float *omega = &m_omega_data[row << m_params.fft_resolution_bits];
  cpu_types::float2 *_k = &m_h0_data[in_index];
  cpu_types::float2 *_mk = &m_h0_data[in_mindex];
  float *sqt = &m_sqrt_table[row << m_params.fft_resolution_bits];
  cpu_types::float2 *out0 = (cpu_types::float2 *)m_fftCPU_io_buffer + N * row;
  cpu_types::float2 *out1 = out0 + ((intptr_t)1 << (m_params.fft_resolution_bits << 1));
  cpu_types::float2 *out2 = out1 + ((intptr_t)1 << (m_params.fft_resolution_bits << 1));

  // some iterated values
  float ky = row - N * 0.5f + 0.5f;
  float kx = -0.5f * N + 0.5f;

  float dt = m_doubletime / AM_TWO_PI;

  for (int i = 0; i < int(N); i++)
  {
    float sn, cn;
    float o;
    float odt0;

    odt0 = omega[i] * dt;

    o = odt0 - floorf(odt0);
    o *= AM_TWO_PI;

    sincosf(o, &sn, &cn);
    cpu_types::float2 mk = _mk[-i];

    cpu_types::float2 s01, d01;
    s01.x = _k[i].x + mk.x, s01.y = _k[i].y + mk.y;
    d01.x = _k[i].x - mk.x, d01.y = _k[i].y - mk.y;

    float hx = s01.x * cn - s01.y * sn;
    float hy = d01.x * sn + d01.y * cn;

    // height
    out0[i].x = hx;
    out0[i].y = hy;


    float kxi = (kx + i);
    float s = sqt[i]; // 1.0f/sqrtf(kxi*kxi+ky*ky);
    hy *= s;
    hx *= s;
    out1[i].x = -kxi * hy;
    out1[i].y = kxi * hx;
    out2[i].x = -ky * hy;
    out2[i].y = +ky * hx;
  }
}

// Update H0 to latest parameters
void NVWaveWorks_FFT_CPU_Simulation::UpdateH0(int row)
{
  // TODO: SSE please!

  int N = (1 << m_params.fft_resolution_bits);

  const int ny = (-N / 2 + row);
  const float ky = float(ny) * (2.f * PI / m_params.fft_period);

  cpu_types::float2 wind_dir;
  wind_dir.x = m_params.wind_dir_x;
  wind_dir.y = m_params.wind_dir_y;
  float a = m_params.wave_amplitude * m_params.wave_amplitude; // Use square of amplitude, because Phillips is an *energy* spectrum
  float v = m_params.wind_speed;
  float dir_depend = m_params.wind_dependency;
  float wind_align = m_params.wind_alignment;

  int dmap_dim = N;
  int inout_width = (dmap_dim + 2); // padding

  // kStep = 2PI/L; norm = kStep * sqrt(1/2)
  float phil_norm = 2 * PI / m_params.fft_period;
  cpu_types::float2 *outH0 = &m_h0_data[inout_width * row];

  // Generate an index into the linear gauss map, which has a fixed size of 512,
  //  using the X Y coordinate of the H0 map lookup.  We also need to apply an offset
  //  so that the lookup coordinate will be centred on the gauss map, of a size equal
  //  to that of the H0 map.
  int gauss_row_size = (GAUSS_MAP_RESOLUTION + 2);
  int gauss_offset = (gauss_row_size - inout_width) / 2;
  int gauss_index = (gauss_offset + row) * gauss_row_size + gauss_offset;
  const cpu_types::float2 *inGauss = &global_gauss_data[gauss_index];

  float kx_step = (2.f * PI / m_params.fft_period);
  float kx = float(-N / 2) * kx_step;
  for (int nx = -N / 2, i = 0; i <= int(N); ++i, ++nx) // NB: <= because the h0 wave vector space needs to be inclusive for the ht calc
  {
    const float kx = float(nx) * kx_step;
    cpu_types::float2 K;
    K.x = kx;
    K.y = ky;

    float amplitude = calcH0(nx, ny, K, m_params.window_in, m_params.window_out, wind_dir, v, dir_depend, wind_align, a,
      m_params.small_wave_fraction, m_params.spectrum);
    amplitude *= phil_norm; // Normalization of the height scale to math with definition of amplitude

    amplitude *= 0.7071068f;
    outH0[i].x = amplitude * inGauss[i].x;
    outH0[i].y = amplitude * inGauss[i].y;
  }
}

void NVWaveWorks_FFT_CPU_Simulation::ComputeFFT(int index)
{
  int N = 1 << m_params.fft_resolution_bits;
  // FFT2D (non-SSE code) is left here in case we need compatibility with non-SSE CPUs
  // FFT2D(&m_fftCPU_io_buffer[index*N*N],N);
  FFT2DSSE(&m_fftCPU_io_buffer[index << (m_params.fft_resolution_bits << 1)], N);
}

// Merge all 3 results of FFT into one texture with Dx,Dz and height
void NVWaveWorks_FFT_CPU_Simulation::getHalfData(int row, cpu_types::half4 *compressed)
{
  int N = (1 << m_params.fft_resolution_bits);
  uint16_t *pTex = (uint16_t *)compressed;
  const complex *fftRes = &((complex *)m_fftCPU_io_buffer)[row << m_params.fft_resolution_bits];
  const complex *fftRes1 = fftRes + N * N;
  const complex *fftRes2 = fftRes1 + N * N;
  vec4f s[2];
  float choppy_scale = m_params.choppy_scale;
  s[row & 1] = v_make_vec4f(choppy_scale, choppy_scale, 1.0f, 1.0f);
  s[1 - (row & 1)] = v_make_vec4f(-choppy_scale, -choppy_scale, -1.0f, 1.0f);

  for (int x = 0; x < N; x += 4, pTex += 16, fftRes += 4, fftRes1 += 4, fftRes2 += 4)
  {
    vec4f a[4];

    /*
    //8 aligned reads + transpose. works a bit slower (probably, due to twice more mem reads)
    vec4f fft[4];
    vec4f fft0 = _mm_shuffle_ps(*(vec4f*)fftRes1, ((vec4f*)fftRes1)[1], _MM_SHUFFLE(2,0,2,0));//xzXZ
    vec4f fft1 = _mm_shuffle_ps(*(vec4f*)fftRes2, ((vec4f*)fftRes2)[1], _MM_SHUFFLE(2,0,2,0));//xzXZ
    vec4f fft2 = _mm_shuffle_ps(*(vec4f*)fftRes , ((vec4f*)fftRes )[1], _MM_SHUFFLE(2,0,2,0));//xzXZ

    //transpose fft0,fft1,fft2, one
    vec4f tmp3, tmp2, tmp1, tmp0;

    tmp0 = _mm_shuffle_ps(fft0, fft1, 0x44);
    tmp2 = _mm_shuffle_ps(fft0, fft1, 0xEE);
    tmp1 = _mm_shuffle_ps(fft2, one, 0x44);
    tmp3 = _mm_shuffle_ps(fft2, one, 0xEE);

    a[0] = v_mul( s[0], _mm_shuffle_ps(tmp0, tmp1, 0x88));
    a[1] = v_mul( s[1], _mm_shuffle_ps(tmp0, tmp1, 0xDD));
    a[2] = v_mul( s[0], _mm_shuffle_ps(tmp2, tmp3, 0x88));
    a[3] = v_mul( s[1], _mm_shuffle_ps(tmp2, tmp3, 0xDD));
    /*/

    {
      // vec4f a1[4];
      a[0] = v_mul(s[0], v_make_vec4f(fftRes1[0][0], fftRes2[0][0], fftRes[0][0], 1.0f));
      a[1] = v_mul(s[1], v_make_vec4f(fftRes1[1][0], fftRes2[1][0], fftRes[1][0], 1.0f));
      a[2] = v_mul(s[0], v_make_vec4f(fftRes1[2][0], fftRes2[2][0], fftRes[2][0], 1.0f));
      a[3] = v_mul(s[1], v_make_vec4f(fftRes1[3][0], fftRes2[3][0], fftRes[3][0], 1.0f));
    }
    //*/

    v_float_to_half(&pTex[0], a[0]);
    v_float_to_half(&pTex[4], a[1]);
    v_float_to_half(&pTex[8], a[2]);
    v_float_to_half(&pTex[12], a[3]);

    /*if (uncompressed)
    {
      v_st( (float*)&pRb[0], a[0] );
      v_st( (float*)&pRb[1], a[1] );
      v_st( (float*)&pRb[2], a[2] );
      v_st( (float*)&pRb[3], a[3] );
    }*/
  }
}

void NVWaveWorks_FFT_CPU_Simulation::getFloatData(int row, cpu_types::float4 *uncompressed)
{
  int N = (1 << m_params.fft_resolution_bits);
  float *pTex = (float *)uncompressed;
  const complex *fftRes = &((complex *)m_fftCPU_io_buffer)[row << m_params.fft_resolution_bits];
  const complex *fftRes1 = fftRes + ((intptr_t)1 << (m_params.fft_resolution_bits << 1));
  const complex *fftRes2 = fftRes1 + ((intptr_t)1 << (m_params.fft_resolution_bits << 1));
  vec4f s[2];
  float choppy_scale = m_params.choppy_scale;
  s[row & 1] = v_make_vec4f(choppy_scale, choppy_scale, 1.0f, 1.0f);
  s[1 - (row & 1)] = v_make_vec4f(-choppy_scale, -choppy_scale, -1.0f, 1.0f);

  for (int x = 0; x < N; x += 2, pTex += 8, fftRes += 2, fftRes1 += 2, fftRes2 += 2)
  {
    vec4f a[4];
    a[0] = v_mul(s[0], v_make_vec4f(fftRes1[0][0], fftRes2[0][0], fftRes[0][0], 1.0f));
    a[1] = v_mul(s[1], v_make_vec4f(fftRes1[1][0], fftRes2[1][0], fftRes[1][0], 1.0f));
    v_st(pTex, a[0]);
    v_st(pTex + 4, a[1]);
  }
}

void NVWaveWorks_FFT_CPU_Simulation::getUint16Data(int row, float minHt, float scaleHt, unsigned short *compressed, float &out_max_z)
{
  int N = (1 << m_params.fft_resolution_bits);
  uint16_t *pTex = (uint16_t *)compressed;
  const complex *fftRes = &((complex *)m_fftCPU_io_buffer)[row << m_params.fft_resolution_bits];
  const complex *fftRes1 = fftRes + ((intptr_t)1 << (m_params.fft_resolution_bits << 1));
  const complex *fftRes2 = fftRes1 + ((intptr_t)1 << (m_params.fft_resolution_bits << 1));
  float choppy_scale = m_params.choppy_scale;

  float scale65535 = row & 1 ? -65535. / scaleHt : 65535. / scaleHt;
  vec4f scale0 = v_make_vec4f(choppy_scale * scale65535, choppy_scale * scale65535, scale65535, 1.0f);
  vec4f scale1 = v_neg(scale0);
  vec4f max65535 = v_splats(65535);
  vec4f zero = v_zero();
  vec4f ofs = v_splats(-minHt * 65535. / scaleHt);

  vec4f v_out_max_z = v_zero();
  for (int x = 0; x < N; x += 2, pTex += 6, fftRes += 2, fftRes1 += 2, fftRes2 += 2)
  {
    vec4f a0, a1;
    a0 = v_make_vec4f(fftRes1[0][0], fftRes2[0][0], fftRes[0][0], 1.0f);
    v_out_max_z = v_max(v_out_max_z, a0);
    a0 = v_madd(a0, scale0, ofs);
    a0 = v_max(zero, a0);
    a0 = v_min(max65535, a0);
    vec4i a0i = v_cvt_roundi(a0);
    v_stui_half(pTex, v_packus(a0i));

    a1 = v_make_vec4f(fftRes1[1][0], fftRes2[1][0], fftRes[1][0], 1.0f);
    v_out_max_z = v_max(v_out_max_z, a1);
    a1 = v_madd(a1, scale1, ofs);
    a1 = v_max(zero, a1);
    a1 = v_min(max65535, a1);
    vec4i a1i = v_cvt_roundi(a1);
    v_stui_half(pTex + 3, v_packus(a1i));
  }

  out_max_z = v_extract_z(v_out_max_z);
  out_max_z *= choppy_scale;
}

NVWaveWorks_FFT_CPU_Simulation::~NVWaveWorks_FFT_CPU_Simulation() { releaseAllResources(); }

bool NVWaveWorks_FFT_CPU_Simulation::initGaussAndOmega()
{
  init_nv_wave_works(); //
  SAFE_ALIGNED_FREE(m_omega_data);
  int N = 1 << m_params.fft_resolution_bits;
  int num_height_map_samples = (N + 4) * (N + 1);
  m_omega_data = (float *)NVSDK_aligned_malloc(num_height_map_samples * sizeof(*m_omega_data), sizeof(vec4f));
  init_omega(m_params.fft_resolution_bits, m_params.fft_period, m_params.spectrum, m_omega_data);
  return true;
}

bool NVWaveWorks_FFT_CPU_Simulation::allocateAllResources()
{
  int N = (1 << m_params.fft_resolution_bits);
  // initialize rarely-updated datas
  B_RETURN(initGaussAndOmega());

  // initialize philips spectrum
  SAFE_ALIGNED_FREE(m_h0_data);
  m_h0_data = (cpu_types::float2 *)NVSDK_aligned_malloc((N + 2) * (N + 1) * sizeof(*m_h0_data), sizeof(vec4f));
  m_H0UpdateRequired = true;

  // reallocate fftw in-out buffer
  SAFE_ALIGNED_FREE(m_fftCPU_io_buffer);
  m_fftCPU_io_buffer = (complex *)NVSDK_aligned_malloc(3 * N * N * sizeof(complex), sizeof(vec4f));

  int log2N = get_log2w(N) - FFT_RES_START;
  m_sqrt_table = NULL;
  init_sqrt_table(log2N);
  if (log2N >= 0 && log2N < FFT_RES_NUM)
    m_sqrt_table = global_sqrt_table[log2N];
  return true;
}

void NVWaveWorks_FFT_CPU_Simulation::releaseAllResources()
{
  // Ensure any texture locks are relinquished
  m_sqrt_table = NULL;
  SAFE_ALIGNED_FREE(m_h0_data);
  SAFE_ALIGNED_FREE(m_omega_data);

  SAFE_ALIGNED_FREE(m_fftCPU_io_buffer);
}

NVWaveWorks_FFT_CPU_Simulation::NVWaveWorks_FFT_CPU_Simulation()
{
  m_h0_data = 0;
  m_omega_data = 0;
  m_fftCPU_io_buffer = 0;
  m_sqrt_table = 0;

  m_H0UpdateRequired = true;
  m_doubletime = 0.;
}

bool NVWaveWorks_FFT_CPU_Simulation::reinit(const Params &params, bool all_res, bool &reallocate)
{
  reallocate = false;
  bool bReinitH0 = false;
  bool bReinitGaussAndOmega = false;

  // Ensure any texture locks are relinquished

  if (m_params.fft_resolution_bits != params.fft_resolution_bits)
  {
    reallocate = true;
  }

  if (m_params.fft_period != params.fft_period || m_params.fft_resolution_bits != params.fft_resolution_bits)
  {
    bReinitGaussAndOmega = true;
  }

  if (m_params != params || bReinitGaussAndOmega)
  {
    bReinitH0 = true;
  }

  m_params = params;


  if (reallocate)
  {
    releaseAllResources();
  }

  if (reallocate && all_res)
  {
    B_RETURN(allocateAllResources());
  }
  else
  {
    // allocateAllResources() does these inits anyway, so only do them forcibly
    // if we're not re-allocating...
    if (bReinitGaussAndOmega)
    {
      // Important to do this first, because H0 relies on an up-to-date Gaussian distribution
      B_RETURN(initGaussAndOmega());
    }

    if (bReinitH0)
    {
      m_H0UpdateRequired = true;
    }
  }

  return true;
}


namespace
{
// Template algo for initializaing various aspects of simulation
template <class Functor>
void for_each_wavevector(float fft_period, const Functor &functor)
{
  for (int i = 0; i < functor.dmap_dim; i++)
  {
    // ny is y-coord wave number
    const int ny = (-functor.dmap_dim / 2 + i);

    // K is wave-vector, range [-|DX/W, |DX/W], [-|DY/H, |DY/H]
    cpu_types::float2 K;
    K.y = float(ny) * (2 * float(AM_PI) / fft_period);

    for (int j = 0; j < functor.dmap_dim; j++)
    {
      // nx is x-coord wave number
      int nx = (-functor.dmap_dim / 2 + j);

      K.x = float(nx) * (2 * float(AM_PI) / fft_period);

      functor(i, j, nx, ny, K);
    }
  }
}

struct init_omega_functor
{
  int dmap_dim;
  float *pOutOmega;
  int spectrum;

  void operator()(int i, int j, int /* not used nx*/, int /* not used ny*/, cpu_types::float2 K) const
  {
    float kLen = sqrtf(K.x * K.x + K.y * K.y);
    float omega = 0;
    switch (spectrum)
    {
      case SPECTRUM_PHILLIPS: omega = omegaPhillips(kLen); break;
      case SPECTRUM_UNIFIED_DIRECTIONAL: omega = omegaUnifiedDirectional(kLen); break;
    }
    pOutOmega[i * dmap_dim + j] = omega;
  }
};

/*static std::mt19937 g_random_number_generation;

template <class _Engine>
float generate_uniform_01(_Engine& _Eng)
{
  return ((_Eng() - (_Eng.min)()) / ((float)(_Eng.max)() - (float)(_Eng.min)() + 1.f));
}*/
int g_random_number_generation = 0x1111111;
inline float generate_uniform_01(int &g_random_number_generation) { return _frnd(g_random_number_generation); }


// Generating gaussian random number with mean 0 and standard deviation 1.
float Gauss()
{
  float u1 = generate_uniform_01(g_random_number_generation);
  float u2 = generate_uniform_01(g_random_number_generation);
  if (u1 < 1e-6f)
    u1 = 1e-6f;
  return sqrtf(-2 * logf(u1)) * cosf(2 * float(AM_PI) * u2);
}
} // namespace

static void init_omega(int fft_resolution_bits, float fft_period, int spectrum, float *pOutOmega)
{
  init_omega_functor f;
  f.dmap_dim = 1 << fft_resolution_bits;
  f.spectrum = spectrum;
  f.pOutOmega = pOutOmega;

  for_each_wavevector(fft_period, f);
}

static void init_gauss(cpu_types::float2 *pOutGauss, int resolution)
{
  // g_random_number_generation.seed();
  g_random_number_generation = 0x11111111;
  for (int i = 0; i <= resolution; i++)
  {
    for (int j = 0; j <= resolution; j++)
    {
      const int ix = i * (resolution + 2) + j;
      pOutGauss[ix].x = Gauss();
      pOutGauss[ix].y = Gauss();
    }
  }
}

void init_nv_wave_works()
{
  if (!global_gauss_data)
  {
    global_gauss_data = (cpu_types::float2 *)NVSDK_aligned_malloc(GAUSS_MAP_SIZE * sizeof(*global_gauss_data), sizeof(vec4f));
    init_gauss(global_gauss_data, GAUSS_MAP_RESOLUTION);
    init_global_weights();
  }
}
void close_nv_wave_works()
{
  SAFE_ALIGNED_FREE(global_gauss_data);
  for (int i = 0; i < FFT_RES_NUM; ++i)
    SAFE_ALIGNED_FREE(global_sqrt_table[i]);
}

const cpu_types::float2 *get_global_gauss_data(int &gauss_resolution, int &stride)
{
  init_nv_wave_works();
  gauss_resolution = GAUSS_MAP_RESOLUTION;
  stride = GAUSS_MAP_RESOLUTION + 2; // padding
  return global_gauss_data;
}

static double upper_bound_phillips_integral(float k, float v, float a, float dir_depend)
{
  if (k <= 0.f || v <= 0.001f)
    return 0.;

  // largest possible wave from constant wind of velocity v
  double l = v * v / GRAV_ACCEL;

  // integral has analytic form, yay!
  double phillips_integ = 0.5 * PI * a * l * l * exp(-1. / (k * k * l * l));

  // dir_depend affects half the domain
  phillips_integ *= (1.0 - 0.5 * dir_depend);

  // we may safely ignore 'small_wave_fraction' for an upper-bound estimate
  return phillips_integ;
}

float get_spectrum_rms_sqr(const NVWaveWorks_FFT_CPU_Simulation::Params &params)
{
  float a = params.wave_amplitude * params.wave_amplitude;
  float v = params.wind_speed;
  float dir_depend = params.wind_dependency;
  float fft_period = params.fft_period;

  float phil_norm = AM_E * 0.25f / fft_period; // This normalization ensures that the simulation is invariant w.r.t. units and/or
                                               // fft_period
  a *= sqr(phil_norm);                         // Use the square as we are accumulating RMS

  // We can compute the integral of Phillips over a disc in wave vector space analytically, and by subtracting one
  // disc from the other we can compute the integral for the ring defined by {params.window_in,params.window_out}
  const float lower_k = params.window_in * 2.f * float(AM_PI) / fft_period;
  const float upper_k = params.window_out * 2.f * float(AM_PI) / fft_period;
  double rms_est = upper_bound_phillips_integral(upper_k, v, a, dir_depend) - upper_bound_phillips_integral(lower_k, v, a, dir_depend);
  rms_est = rms_est < 0 ? 0 : rms_est;

  // Normalize to wave number space
  rms_est *= 0.25f * (fft_period * fft_period) / (float(AM_PI * AM_PI));

  return rms_est;
}

void calc_wave_height(const NVWaveWorks_FFT_CPU_Simulation::Params *fft, int num_cascades, float &out_significant_wave_height,
  float &out_max_wave_height, float *out_max_wave_size)
{
  // Based on significant wave height: http://en.wikipedia.org/wiki/Significant_wave_height
  //
  // Significant wave height is said to be 1.4x rms and represents a 1 in 3 event
  // Then, given that wave heights follow a Rayleigh distribution, and based on the form of the CDF,
  // we observe that a wave height of 4x significant should be *very* infrequent (1 in 3^16, approx)
  //
  // Hence, we use 4 x 1.4 x rms, or 5.6x with rounding up!
  //
  out_significant_wave_height = 0.0f;
  out_max_wave_height = 0.0f;
  for (int i = 0; i < num_cascades; i++)
  {
    double wave = sqrtf(get_spectrum_rms_sqr(fft[i]));
    double maxWave = wave * 5.0;
    if (out_max_wave_size)
      out_max_wave_size[i] = maxWave;
    // The RMS wave height, which is defined as square root of the average of the squares of all wave heights,
    // is approximately equal to Hs divided by 1.4
    out_significant_wave_height += wave * 1.4f;
    out_max_wave_height += maxWave;
  }
  // Use the max wave height there because a significant is not precise enough anymore but the max height quite close to the truth
  out_significant_wave_height = out_max_wave_height;
}
