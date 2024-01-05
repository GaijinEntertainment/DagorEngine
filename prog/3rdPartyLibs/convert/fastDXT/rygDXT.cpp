// stb_dxt.h - v1.04 - DXT1/DXT5 compressor - public domain
// original by fabian "ryg" giesen - ported to C by stb
// use '#define STB_DXT_IMPLEMENTATION' before including to create the implementation
//
// USAGE:
//   call stb_compress_dxt_block() for every block (you must pad)
//     source should be a 4x4 block of RGBA data in row-major order;
//     A is ignored if you specify alpha=0; you can turn on dithering
//     and "high quality" using mode.
//
// version history:
//   v1.04  - (ryg) default to no rounding bias for lerped colors (as per S3TC/DX10 spec);
//            single color match fix (allow for inexact color interpolation);
//            optimal DXT5 index finder; "high quality" mode that runs multiple refinement steps.
//   v1.03  - (stb) endianness support
//   v1.02  - (stb) fix alpha encoding bug
//   v1.01  - (stb) fix bug converting to RGB that messed up quality, thanks ryg & cbloom
//   v1.00  - (stb) first release

#include "rygDXT.h"
#include "internalDXT.h"
#include <vecmath/dag_vecMath.h>
#include <stdlib.h>
//#undef DXT_INTR
#if !defined(DXT_INTR)
static inline int clamp(int t, int min_val, int max_val) { return t <= min_val ? min_val : (t >= max_val ? max_val : t); }
#endif
// compression mode (bitflags)
#define STB_COMPRESS_DXT_BLOCK
// configuration options for DXT encoder. set them in the project/makefile or just define
// them at the top.

// STB_DXT_USE_ROUNDING_BIAS
//     use a rounding bias during color interpolation. this is closer to what "ideal"
//     interpolation would do but doesn't match the S3TC/DX10 spec. old versions (pre-1.03)
//     implicitly had this turned on. 
//
//     in case you're targeting a specific type of hardware (e.g. console programmers):
//     NVidia and Intel GPUs (as of 2010) as well as DX9 ref use DXT decoders that are closer
//     to STB_DXT_USE_ROUNDING_BIAS. AMD/ATI, S3 and DX10 ref are closer to rounding with no bias.
//     you also see "(a*5 + b*3) / 8" on some old GPU designs.
// #define STB_DXT_USE_ROUNDING_BIAS

#include <math.h>
#include <string.h> // memset

namespace rygDXT {

static unsigned char stb__Expand5[32];
static unsigned char stb__Expand6[64];
static unsigned char stb__OMatch5[256][2];
static unsigned char stb__OMatch6[256][2];
static unsigned char stb__QuantRBTab[256+16];
static unsigned char stb__QuantGTab[256+16];

static __forceinline int stb__Mul8Bit(int a, int b)
{
  int t = a*b + 128;
  return (t + (t >> 8)) >> 8;
}

static inline void stb__From16Bit(unsigned char *out, unsigned short v)
{
   int bv = (v & 0xf800) >> 11;
   int gv = (v & 0x07e0) >> 5;
   int rv = (v & 0x001f) ;
   
   out[0] = (rv<<3) | (rv>>2);
   out[1] = (gv<<2) | (gv>>4);
   out[2] = (bv<<3) | (bv>>2);
   out[3] = 0;
}


static __forceinline unsigned cvt32bit_to16bit32(unsigned v)
{
   unsigned rv = stb__Mul8Bit(v & 0xff, 31);
   unsigned gv = stb__Mul8Bit((v >> 8) & 0xff, 63);
   unsigned bv = stb__Mul8Bit((v >> 16) & 0xff, 31);
   return  (rv<<3)|(gv<<10)|(bv<<19);
}

// linear interpolation at 1/3 point between a and b, using desired rounding type
static inline int stb__Lerp13(int a, int b)
{
#ifdef STB_DXT_USE_ROUNDING_BIAS
   // with rounding bias
   return a + stb__Mul8Bit(b-a, 0x55);
#else
   // without rounding bias
   // replace "/ 3" by "* 0xaaab) >> 17" if your compiler sucks or you really need every ounce of speed.
   return (((a<<1) + b) * 0xaaab) >> 17;
   //return (2*a + b) / 3;
#endif
}

// lerp RGB color
static void stb__Lerp13RGB(unsigned char *out, unsigned char *p1, unsigned char *p2)
{
   out[0] = stb__Lerp13(p1[0], p2[0]);
   out[1] = stb__Lerp13(p1[1], p2[1]);
   out[2] = stb__Lerp13(p1[2], p2[2]);
}

/****************************************************************************/

// compute table to reproduce constant colors as accurately as possible
static void stb__PrepareOptTable(unsigned char *Table,const unsigned char *expand,int size)
{
   int i,mn,mx;
   for (i=0;i<256;i++) {
      int bestErr = 256;
      for (mn=0;mn<size;mn++) {
         for (mx=0;mx<size;mx++) {
            int mine = expand[mn];
            int maxe = expand[mx];
            int err = abs(stb__Lerp13(maxe, mine) - i);
            
            // DX10 spec says that interpolation must be within 3% of "correct" result,
            // add this as error term. (normally we'd expect a random distribution of
            // +-1.5% error, but nowhere in the spec does it say that the error has to be
            // unbiased - better safe than sorry).
            err += abs(maxe - mine) * 3 / 100;
            
            if(err < bestErr)
            { 
               Table[i*2+0] = mx;
               Table[i*2+1] = mn;
               bestErr = err;
            }
         }
      }
   }
}

/*// Block dithering function. Simply dithers a block to 565 RGB.
// (Floyd-Steinberg)
static void stb__DitherBlock(unsigned char *dest, unsigned char *block)
{
  int err[8],*ep1 = err,*ep2 = err+4, *et;
  int ch,y;

  // process channels seperately
  for (ch=0; ch<3; ++ch) {
      unsigned char *bp = block+ch, *dp = dest+ch;
      unsigned char *quant = (ch == 1) ? stb__QuantGTab+8 : stb__QuantRBTab+8;
      memset(err, 0, sizeof(err));
      for(y=0; y<4; ++y) {
         dp[ 0] = quant[bp[ 0] + ((3*ep2[1] + 5*ep2[0]) >> 4)];
         ep1[0] = bp[ 0] - dp[ 0];
         dp[ 4] = quant[bp[ 4] + ((7*ep1[0] + 3*ep2[2] + 5*ep2[1] + ep2[0]) >> 4)];
         ep1[1] = bp[ 4] - dp[ 4];
         dp[ 8] = quant[bp[ 8] + ((7*ep1[1] + 3*ep2[3] + 5*ep2[2] + ep2[1]) >> 4)];
         ep1[2] = bp[ 8] - dp[ 8];
         dp[12] = quant[bp[12] + ((7*ep1[2] + 5*ep2[3] + ep2[2]) >> 4)];
         ep1[3] = bp[12] - dp[12];
         bp += 16;
         dp += 16;
         et = ep1, ep1 = ep2, ep2 = et; // swap
      }
   }
}*/
// The color matching function
#define C565_5_MASK 0xF8 // 0xFF minus last three bits
#define C565_6_MASK 0xFC // 0xFF minus last two bits

inline int ColorDistance( const unsigned char *c1, const unsigned char *c2 )
{
  return ( ( c1[0] - c2[0] ) * ( c1[0] - c2[0] ) ) +
         ( ( c1[1] - c2[1] ) * ( c1[1] - c2[1] ) ) +
         ( ( c1[2] - c2[2] ) * ( c1[2] - c2[2] ) );
}

unsigned EmitColorIndicesFastPrecise( const unsigned char *colorBlock, unsigned char *color )
{
  const unsigned char *minColor = color + 4,*maxColor =  color;
  unsigned char colors[4][4];
  unsigned result = 0;

  colors[0][0] = ( maxColor[0] & C565_5_MASK ) | ( maxColor[0] >> 5 );
  colors[0][1] = ( maxColor[1] & C565_6_MASK ) | ( maxColor[1] >> 6 );
  colors[0][2] = ( maxColor[2] & C565_5_MASK ) | ( maxColor[2] >> 5 );
  colors[1][0] = ( minColor[0] & C565_5_MASK ) | ( minColor[0] >> 5 );
  colors[1][1] = ( minColor[1] & C565_6_MASK ) | ( minColor[1] >> 6 );
  colors[1][2] = ( minColor[2] & C565_5_MASK ) | ( minColor[2] >> 5 );
  colors[2][0] = ( 2 * colors[0][0] + 1 * colors[1][0] ) / 3;
  colors[2][1] = ( 2 * colors[0][1] + 1 * colors[1][1] ) / 3;
  colors[2][2] = ( 2 * colors[0][2] + 1 * colors[1][2] ) / 3;
  colors[3][0] = ( 1 * colors[0][0] + 2 * colors[1][0] ) / 3;
  colors[3][1] = ( 1 * colors[0][1] + 2 * colors[1][1] ) / 3;
  colors[3][2] = ( 1 * colors[0][2] + 2 * colors[1][2] ) / 3;

  for ( int i = 15; i >= 0; i-- ) {

    int d0 = ColorDistance(colors[0], colorBlock+i*4);
    int d1 = ColorDistance(colors[1], colorBlock+i*4);
    int d2 = ColorDistance(colors[2], colorBlock+i*4);
    int d3 = ColorDistance(colors[3], colorBlock+i*4);

    int b0 = d0 > d2;
    int b1 = d1 > d3;
    int b2 = d0 > d3;
    int b3 = d1 > d2;
    int b4 = d0 > d1; 
    int b5 = d2 > d3;

    int x0 = b1 & b2;
    int x1 = b0 & b3;
    int x2 = b2 & b5;
    int x3 = (!b3) & b4;
    result |= ( (x3 | x2) | ( ( x1 | x0 ) << 1 )) << (i<<1);
  }
  return result;
}

//uses linear distance
static unsigned int stb__MatchColorsBlock_c(unsigned char *block, unsigned char *maxMincolor)
{
   unsigned int mask = 0;
   unsigned char color[16];
   color[0] = (maxMincolor[0]&0xF8) | (maxMincolor[0]>>5);
   color[1] = (maxMincolor[1]&0xFC) | (maxMincolor[1]>>6);
   color[2] = (maxMincolor[2]&0xF8) | (maxMincolor[2]>>5);

   color[4] = (maxMincolor[4]&0xF8) | (maxMincolor[4]>>5);
   color[5] = (maxMincolor[5]&0xFC) | (maxMincolor[5]>>6);
   color[6] = (maxMincolor[6]&0xF8) | (maxMincolor[6]>>5);

   stb__Lerp13RGB(color+ 8, color+0, color+4);
   stb__Lerp13RGB(color+12, color+4, color+0);
   
   int dirr = color[0*4+0] - color[1*4+0];
   int dirg = color[0*4+1] - color[1*4+1];
   int dirb = color[0*4+2] - color[1*4+2];
   int dots[16];
   int stops[4];
   int i;
   int c0Point, halfPoint, c3Point;

   for(i=0;i<16;i++)
      dots[i] = block[i*4+0]*dirr + block[i*4+1]*dirg + block[i*4+2]*dirb;

   //for(i=0;i<4;i++)
      stops[0] = color[0*4+0]*dirr + color[0*4+1]*dirg + color[0*4+2]*dirb;
      stops[1] = color[1*4+0]*dirr + color[1*4+1]*dirg + color[1*4+2]*dirb;
      stops[2] = color[2*4+0]*dirr + color[2*4+1]*dirg + color[2*4+2]*dirb;
      stops[3] = color[3*4+0]*dirr + color[3*4+1]*dirg + color[3*4+2]*dirb;

   // think of the colors as arranged on a line; project point onto that line, then choose
   // next color out of available ones. we compute the crossover points for "best color in top
   // half"/"best in bottom half" and then the same inside that subinterval.
   //
   // relying on this 1d approximation isn't always optimal in terms of euclidean distance,
   // but it's very close and a lot faster.
   // http://cbloomrants.blogspot.com/2008/12/12-08-08-dxtc-summary.html
   
   c0Point   = (stops[1] + stops[3]) >> 1;
   halfPoint = (stops[3] + stops[2]) >> 1;
   c3Point   = (stops[2] + stops[0]) >> 1;

  // the version without dithering is straightforward
  // a bit optimized (unrolled) cycle
   mask  = (dots[ 0] < halfPoint ? ((dots[ 0] < c0Point) ? 1 : 3) : ((dots[ 0] < c3Point) ? 2 : 0))<<(2* 0);
   mask |= (dots[ 1] < halfPoint ? ((dots[ 1] < c0Point) ? 1 : 3) : ((dots[ 1] < c3Point) ? 2 : 0))<<(2* 1);
   mask |= (dots[ 2] < halfPoint ? ((dots[ 2] < c0Point) ? 1 : 3) : ((dots[ 2] < c3Point) ? 2 : 0))<<(2* 2);
   mask |= (dots[ 3] < halfPoint ? ((dots[ 3] < c0Point) ? 1 : 3) : ((dots[ 3] < c3Point) ? 2 : 0))<<(2* 3);
   mask |= (dots[ 4] < halfPoint ? ((dots[ 4] < c0Point) ? 1 : 3) : ((dots[ 4] < c3Point) ? 2 : 0))<<(2* 4);
   mask |= (dots[ 5] < halfPoint ? ((dots[ 5] < c0Point) ? 1 : 3) : ((dots[ 5] < c3Point) ? 2 : 0))<<(2* 5);
   mask |= (dots[ 6] < halfPoint ? ((dots[ 6] < c0Point) ? 1 : 3) : ((dots[ 6] < c3Point) ? 2 : 0))<<(2* 6);
   mask |= (dots[ 7] < halfPoint ? ((dots[ 7] < c0Point) ? 1 : 3) : ((dots[ 7] < c3Point) ? 2 : 0))<<(2* 7);
   mask |= (dots[ 8] < halfPoint ? ((dots[ 8] < c0Point) ? 1 : 3) : ((dots[ 8] < c3Point) ? 2 : 0))<<(2* 8);
   mask |= (dots[ 9] < halfPoint ? ((dots[ 9] < c0Point) ? 1 : 3) : ((dots[ 9] < c3Point) ? 2 : 0))<<(2* 9);
   mask |= (dots[10] < halfPoint ? ((dots[10] < c0Point) ? 1 : 3) : ((dots[10] < c3Point) ? 2 : 0))<<(2*10);
   mask |= (dots[11] < halfPoint ? ((dots[11] < c0Point) ? 1 : 3) : ((dots[11] < c3Point) ? 2 : 0))<<(2*11);
   mask |= (dots[12] < halfPoint ? ((dots[12] < c0Point) ? 1 : 3) : ((dots[12] < c3Point) ? 2 : 0))<<(2*12);
   mask |= (dots[13] < halfPoint ? ((dots[13] < c0Point) ? 1 : 3) : ((dots[13] < c3Point) ? 2 : 0))<<(2*13);
   mask |= (dots[14] < halfPoint ? ((dots[14] < c0Point) ? 1 : 3) : ((dots[14] < c3Point) ? 2 : 0))<<(2*14);
   mask |= (dots[15] < halfPoint ? ((dots[15] < c0Point) ? 1 : 3) : ((dots[15] < c3Point) ? 2 : 0))<<(2*15);

   return mask;
}


template <typename T> inline T min(const T val1, const T val2)
{
  return (val1 < val2 ? val1 : val2);
}
template <typename T> inline T max(const T val1, const T val2)
{
  return (val1 > val2 ? val1 : val2);
}

template <int nIterPower>
static __forceinline void stb__OptimizeColorsBlock_c(unsigned char *__restrict block, unsigned *__restrict maxmin32)
{
  int mind = 0x7fffffff,maxd = -0x7fffffff;
  unsigned char *minp, *maxp;
  float magn;
  //int v_r,v_g,v_b;
  short v_[3];
  float covf[6];
  float vf[3];

  // determine color distribution
  int cov[6];
  short mu[4];
  int ch,i,iter;

  mu[0] = block[0];
  mu[1] = block[1];
  mu[2] = block[2];
  for (i=4;i<64;i+=4)
  {
    mu[0] += block[i+0];
    mu[1] += block[i+1];
    mu[2] += block[i+2];
  }

  // determine covariance matrix
  ///*
  float muf[3] = {mu[0]/16.0f,mu[1]/16.0f,mu[2]/16.0f};
  for (i=0;i<6;i++)
     covf[i] = 0;

  for (i=0;i<16;i++)
  {
    float r = block[i*4+0] - muf[0];
    float g = block[i*4+1] - muf[1];
    float b = block[i*4+2] - muf[2];

    covf[0] += r*r/255.0f;
    covf[1] += r*g/255.0f;
    covf[2] += r*b/255.0f;
    covf[3] += g*g/255.0f;
    covf[4] += g*b/255.0f;
    covf[5] += b*b/255.0f;
  }

  // convert covariance matrix to float, find principal axis via power iter
  /*/
  mu[0] = (mu[0] + 8) >> 4;
  mu[1] = (mu[1] + 8) >> 4;
  mu[2] = (mu[2] + 8) >> 4;
  for (i=0;i<6;i++)
     cov[i] = 0;

  for (i=0;i<16;i++)
  {
    signed short r = block[i*4+0] - mu[0];
    signed short g = block[i*4+1] - mu[1];
    signed short b = block[i*4+2] - mu[2];

    cov[0] += r*r;
    cov[1] += r*g;
    cov[2] += r*b;
    cov[3] += g*g;
    cov[4] += g*b;
    cov[5] += b*b;
  }

  // convert covariance matrix to float, find principal axis via power iter
  for(i=0;i<6;i++)
    covf[i] = cov[i] / 255.0f;
  //*/
  vf[0] = vf[1] = vf[2] = 1.0f;//::max(::max(vf[0],vf[1]),vf[2]);

  for(iter=0;iter<nIterPower;iter++)
  {
    float r = vf[0]*covf[0] + vf[1]*covf[1] + vf[2]*covf[2];
    float g = vf[0]*covf[1] + vf[1]*covf[3] + vf[2]*covf[4];
    float b = vf[0]*covf[2] + vf[1]*covf[4] + vf[2]*covf[5];

    magn = fabs(vf[0]);
    if (fabs(vf[1]) > magn) magn = fabs(vf[1]);
    if (fabs(vf[2]) > magn) magn = fabs(vf[2]);
    float invmagn = 1.0f/magn;

    vf[0] = r*invmagn;
    vf[1] = g*invmagn;
    vf[2] = b*invmagn;
  }
   if(magn < 0.001f) { // too small, default to luminance
      v_[0] = 299; // JPEG YCbCr luma coefs, scaled by 1000.
      v_[1] = 587;
      v_[2] = 114;
   } else {
      magn = 512.0 / magn;
      v_[0] = (int) (vf[0] * magn);
      v_[1] = (int) (vf[1] * magn);
      v_[2] = (int) (vf[2] * magn);
   }

   // Pick colors at extreme points
   for(i=0;i<16;i++)
   {
      int dot = block[i*4+0]*v_[0] + block[i*4+1]*v_[1] + block[i*4+2]*v_[2];

      if (dot < mind) {
         mind = dot;
         minp = block+i*4;
      }

      if (dot > maxd) {
         maxd = dot;
         maxp = block+i*4;
      }
   }

   maxmin32[0] = cvt32bit_to16bit32((*(unsigned*)maxp));
   maxmin32[1] = cvt32bit_to16bit32((*(unsigned*)minp));
}

#if defined(DXT_INTR)

static inline unsigned short stb__As16Bit(unsigned char *color)
{
//   return (stb__Mul8Bit(color[2],31) << 11) | (stb__Mul8Bit(color[1],63) << 5) | (stb__Mul8Bit(color[0],31) << 0);
  __m128i v_color = _mm_cvtsi32_si128(*(unsigned *) color);
  v_color = _mm_unpacklo_epi8(v_color, _mm_setzero_si128());
  v_color = _mm_mullo_epi16(v_color, *((__m128i*)fastDXT::SIMD_SSE2_word_31_63_31));
  v_color = _mm_add_epi16(v_color, *((__m128i*)fastDXT::SIMD_SSE2_word_128));
  v_color = _mm_add_epi16(v_color, _mm_srli_epi16(v_color, 8));
  v_color = _mm_srli_epi16(v_color, 8);
  v_color = _mm_packus_epi16(v_color, v_color);
  unsigned packedcolor = _mm_cvtsi128_si32(v_color);
  return ((packedcolor&0x1F0000)>>5) | ((packedcolor&0x3f00) >> 3) | ((packedcolor&0x1F));
}
template<bool high>
static __forceinline unsigned int stb__MatchColorsBlockProduction(unsigned char *block, unsigned char *color)
{
  if (high)
    return fastDXT::stb__GetColorIndices_IntrinsicsPrecise(block, color);
  else
    return fastDXT::stb__GetColorIndices_Intrinsics(block, color);
}

static __forceinline unsigned int stb__MatchColorsBlockExcellent(unsigned char *block, unsigned char *color)
{
  return fastDXT::stb__GetColorIndices_IntrinsicsPrecise(block, color);
}

ALIGN16( static unsigned short SIMD_SSE2_word_8[8] ) = { 8, 8, 8, 8, 8, 8, 8, 8 };
ALIGN16( static unsigned short SSE2_word_mask01000100[8] ) = { 0, 0xFFFF, 0, 0, 0, 0xFFFF, 0, 0 };
ALIGN16( static unsigned short SSE2_word_mask01100110[8] ) = { 0, 0xFFFF, 0xFFFF, 0, 0, 0xFFFF, 0xFFFF, 0 };
ALIGN16( static float SIMD_SSE2_JPEG_LUMA[4] ) = {299.f, 587.f, 114.f, 0.f};
ALIGN16( static float SIMD_SSE2_float_inv255[4] ) = {1.0/255.0f,1.0/255.0f,1.0/255.0f,1.0/255.0f};
ALIGN16( static float SIMD_SSE2_float_512[4] ) = {512.0f,512.0f,512.0f,512.0f};
ALIGN16( static float SIMD_SSE2_float_1d16[4] ) = {1.0f/16.0f, 1.0f/16.0f, 1.0f/16.0f, 1.0f/16.0f};
ALIGN16( static float SIMD_SSE2_float_vmagn[4] ) = {0.01f,0.01f,0.01f,0.01f};
ALIGN16( static int SIMD_SSE2_maxint[4]) = { 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff};
ALIGN16( static int SIMD_SSE2_minint[4]) = {-0x7fffffff,-0x7fffffff,-0x7fffffff,-0x7fffffff};
ALIGN16( static unsigned short SIMD_SSE2_word_noalpha[8] ) = { 0xFFFF, 0xFFFF, 0xFFFF, 0, 0xFFFF, 0xFFFF, 0xFFFF, 0};
__forceinline vec4f v_perm_yyzz(vec4f a) { return _mm_shuffle_ps(a, a, _MM_SHUFFLE(2,2,1,1)); }
__forceinline vec4f v_perm_yzzz(vec4f a) { return _mm_shuffle_ps(a, a, _MM_SHUFFLE(1,2,2,2)); }

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4700)
#pragma runtime_checks( "u", off )
#endif
template<int nIterPower>
static __forceinline void stb__OptimizeColorsBlockIntrinsics(unsigned char *__restrict block, unsigned *__restrict maxmin32)
{
  //ALIGN16(int cov[8]);
  //*(__m128i*)cov = _mm_setzero_si128();
  //*((__m128i*)(cov+4)) = _mm_setzero_si128();
  //ALIGN16(char mu[4]);

  // determine color distribution
  int ch,i,iter;
  const __m128i zero = _mm_setzero_si128();

  __m128i color, colsum;
  color = _mm_load_si128((__m128i*)(block + 0));
  colsum = _mm_add_epi16(_mm_unpacklo_epi8(color, zero), _mm_unpackhi_epi8(color, zero));

  color = _mm_load_si128((__m128i*)(block + 16));
  colsum = _mm_add_epi16(colsum, _mm_unpacklo_epi8(color, zero));
  colsum = _mm_add_epi16(colsum, _mm_unpackhi_epi8(color, zero));

  color = _mm_load_si128((__m128i*)(block + 32));
  colsum = _mm_add_epi16(colsum, _mm_unpacklo_epi8(color, zero));
  colsum = _mm_add_epi16(colsum, _mm_unpackhi_epi8(color, zero));

  color = _mm_load_si128((__m128i*)(block + 48));
  colsum = _mm_add_epi16(colsum, _mm_unpacklo_epi8(color, zero));
  colsum = _mm_add_epi16(colsum, _mm_unpackhi_epi8(color, zero));

  colsum = _mm_add_epi16(colsum, _mm_shuffle_epi32(colsum, R_SHUFFLE_D(2,3, 0,1)));

  //unsigned char max2[4];
  //unsigned char min2[4];
  __m128 v_vf = _mm_set1_ps(1.0f);
  __m128 cov0f, cov1f;

  cov0f = cov1f = _mm_setzero_ps();
  __m128 colsumf = _mm_mul_ps(_mm_cvtepi32_ps(_mm_unpacklo_epi16(colsum,zero)), *(__m128*)SIMD_SSE2_float_1d16);
  //colsum = _mm_add_epi16(colsum, *(__m128i*)SIMD_SSE2_word_8);
  //colsum = _mm_srli_epi16(colsum, 4);
  //__m128 colsumf = _mm_cvtepi32_ps(_mm_unpacklo_epi16(colsum,zero));
  //__m128 inv255 = *(__m128*)SIMD_SSE2_float_inv255;
  for (i=0;i<64;i+=8)
  {
    color = _mm_loadl_epi64((__m128i*)(block +  i));
    __m128 tmp, tmp2;
    #define COVROW()\
    tmp = _mm_cvtepi32_ps(cov0);\
    tmp = _mm_sub_ps(tmp, colsumf);\
    tmp2 = _mm_mul_ps(tmp, *(__m128*)SIMD_SSE2_float_inv255);\
    cov0f = _mm_add_ps(cov0f, _mm_mul_ps(tmp, v_splat_x(tmp2)));\
    cov1f = _mm_add_ps(cov1f, _mm_mul_ps(v_perm_yyzz(tmp), v_perm_yzzz(tmp2)));

    color = _mm_unpacklo_epi8(color, zero);
    __m128i cov0 = _mm_unpacklo_epi16(color, zero);
    COVROW()
    cov0 = _mm_unpackhi_epi16(color, zero);
    COVROW()
  }

  ///*
  // convert covariance matrix to float, find principal axis via power iter
  __m128 col1 = v_perm_ayzw(v_rot_3(cov1f), v_splat_y(cov0f));
  __m128 col2 = v_perm_ayzw(cov1f, v_splat_z(cov0f));
  vec4f v_magn;
  for (iter=0;iter<nIterPower;iter++)
  {
    v_vf = _mm_add_ps(
      _mm_add_ps(_mm_mul_ps(v_splat_x(v_vf), cov0f),
                 _mm_mul_ps(v_splat_y(v_vf), col1)),
      _mm_mul_ps(v_splat_z(v_vf), col2)
    );
    //move out of cycle for speed
    v_magn = v_abs(v_vf);
    v_magn = v_max(v_max(v_splat_x(v_magn), v_splat_y(v_magn)), v_splat_z(v_magn));
    v_vf = v_div(v_vf, v_magn);
  }
  vec4i v_v_ = v_cvt_roundi(v_sel(
    v_mul(*(__m128*)SIMD_SSE2_float_512, v_vf), 
    *(vec4f*)SIMD_SSE2_JPEG_LUMA, 
    v_cmp_ge(*(vec4f*)SIMD_SSE2_float_vmagn, v_magn)));
  v_v_ = _mm_packs_epi32(v_v_, v_v_);

  v_v_ = _mm_and_si128(v_v_, *(__m128i*)SIMD_SSE2_word_noalpha);
  __m128i mind=*(__m128i*)SIMD_SSE2_maxint, maxd = *(__m128i*)SIMD_SSE2_minint;
  //mincolor and maxcolor showed as uninitialized use, due to selection operator
  __m128i mincolor, maxcolor;
  for (i=0;i<4;i++)
  {
     __m128i colorline = _mm_load_si128(((__m128i*)block)+i);
     //__m128i colorline = _mm_loadl_epi64(block+i*8);
     __m128i colorlo = _mm_unpacklo_epi8(colorline, zero);
     colorlo = _mm_madd_epi16(colorlo, v_v_);
     //a.x*b.x + a.y*b.y, a.z*b.z + a.w*b.w | ahi.x*bhi.x + ahi.y*bhi.y, ahi.z*bhi.z + ahi.w*bhi.w
     colorlo = _mm_add_epi32(colorlo, _mm_shuffle_epi32(colorlo, R_SHUFFLE_D(1,0,3,2)));
     colorlo = _mm_shuffle_epi32(colorlo, R_SHUFFLE_D(0,2,0,2));
     //(a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w)*2 | (ahi.x*bhi.x + ahi.y*bhi.y + ahi.z*bhi.z + ahi.w*bhi.w)*2
     __m128i colorhi = _mm_unpackhi_epi8(colorline, zero);
     
     colorhi = _mm_madd_epi16(colorhi, v_v_);
     colorhi = _mm_add_epi32(colorhi, _mm_shuffle_epi32(colorhi, R_SHUFFLE_D(1,0,3,2)));
     colorhi = _mm_shuffle_epi32(colorhi, R_SHUFFLE_D(0,2,0,2));
     __m128i dot = _mm_unpacklo_epi64(colorlo, colorhi);
     __m128i lessmin, moremax;
     #define COMPARE_SELECT(dot, dot_select, compare_result, color_select, colorline)\
       dot_select = v_seli(dot_select, dot, compare_result);\
       color_select = v_seli(color_select, colorline, compare_result);
     #define COMPARE_MINMAX(dot_cmp_min, mincolorline, dot_cmp_max, maxcolorline)\
       lessmin = _mm_cmpgt_epi32(mind, dot_cmp_min);\
       COMPARE_SELECT(dot_cmp_min, mind, lessmin, mincolor, mincolorline)\
       moremax = _mm_cmpgt_epi32(dot_cmp_max, maxd);\
       COMPARE_SELECT(dot_cmp_max, maxd, moremax, maxcolor, maxcolorline)
     COMPARE_MINMAX(dot, colorline, dot, colorline)
  }
  __m128i dot_cmp_min, dot_cmp_max;
  __m128i lessmin, moremax;
  dot_cmp_min = _mm_shuffle_epi32(mind, R_SHUFFLE_D(1,0,3,2));
  dot_cmp_max = _mm_shuffle_epi32(maxd, R_SHUFFLE_D(1,0,3,2));
  COMPARE_MINMAX(dot_cmp_min, _mm_shuffle_epi32(mincolor, R_SHUFFLE_D(1,0,3,2)), 
                 dot_cmp_max, _mm_shuffle_epi32(maxcolor, R_SHUFFLE_D(1,0,3,2)))

  dot_cmp_min = _mm_shuffle_epi32(mind, R_SHUFFLE_D(2,2,0,0));
  dot_cmp_max = _mm_shuffle_epi32(maxd, R_SHUFFLE_D(2,2,0,0));
  COMPARE_MINMAX(dot_cmp_min, _mm_shuffle_epi32(mincolor, R_SHUFFLE_D(2,2,0,0)), 
                 dot_cmp_max, _mm_shuffle_epi32(maxcolor, R_SHUFFLE_D(2,2,0,0)))
  
  #undef COMPARE_MINMAX
  #undef COMPARE_SELECT
  //maxmin32[0] = _mm_cvtsi128_si32(maxcolor);
  //maxmin32[1] = _mm_cvtsi128_si32(mincolor);
  maxcolor = _mm_unpacklo_epi32(maxcolor, mincolor);
  maxcolor = fastDXT::cvt32bit_to16bit32(maxcolor);
  _mm_storel_epi64((__m128i*)maxmin32, maxcolor);
}
#ifdef _MSC_VER
#pragma warning(pop)
#pragma runtime_checks( "u", restore )
#endif


template<int nIterPower>
static __forceinline void stb__OptimizeColorsBlock(unsigned char *__restrict block, unsigned *__restrict maxmin32)
{
  //simply faster. probably, should fully implement cluster fit for excellent?
  fastDXT::stb__OptimizeColorBlock(block, (unsigned char*)maxmin32);
}

static int stb__RefineBlock(unsigned char *block, unsigned int *maxMin32, unsigned int mask)
{
  return fastDXT::stb__RefineBlockIntrinsics(block, maxMin32, mask);
}

#else
static __forceinline unsigned short stb__As16Bit(unsigned char *color)
{
   return (stb__Mul8Bit(color[2],31) << 11) | (stb__Mul8Bit(color[1],63) << 5) | (stb__Mul8Bit(color[0],31) << 0);
}
template<bool>
static __forceinline unsigned int stb__MatchColorsBlockProduction(unsigned char *block, unsigned char *color)
{
  return stb__MatchColorsBlock_c(block, color);
}
static __forceinline unsigned int stb__MatchColorsBlockExcellent(unsigned char *block, unsigned char *color)
{
  return stb__MatchColorsBlock_c(block, color);
}
template<int nIterPower>
static __forceinline void stb__OptimizeColorsBlock(unsigned char *__restrict block, unsigned *__restrict maxmin32)
{
  stb__OptimizeColorsBlock_c<nIterPower>(block, maxmin32);
}
// The color optimization function. (Clever code, part 1)

// The refinement function. (Clever code, part 2)
// Tries to optimize colors to suit block contents better.
// (By solving a least squares system via normal equations+Cramer's rule)
static int stb__RefineBlock(unsigned char *block, unsigned int *maxMin32, unsigned int mask)
{
   static const int w1Tab[4] = { 3,0,2,1 };
   static const int prods[4] = { 0x090000,0x000900,0x040102,0x010402 };
   // ^some magic to save a lot of multiplies in the accumulating loop...
   // (precomputed products of weights for least squares system, accumulated inside one 32-bit register)

   float frb,fg;
   int i, akku = 0, xx,xy,yy;
   unsigned int cm = mask;
   ALIGN16(int At1[4]);
   ALIGN16(int At2[4]);
   ALIGN16(unsigned maxMin[2]);

   if((mask ^ (mask<<2)) < 4) // all pixels have the same index?
   {
      // yes, linear system would be singular; solve using optimal
      // single-color match on average color
      int r = 8, g = 8, b = 8;
      for (i=0;i<16;++i) {
         r += block[i*4+0];
         g += block[i*4+1];
         b += block[i*4+2];
      }

      r >>= 4; g >>= 4; b >>= 4;
      maxMin[0] = maxMin[1] = cvt32bit_to16bit32(r|(g<<8)|(b<<16));
      //max16 = (stb__OMatch5[r][0]<<0) | (stb__OMatch6[g][0]<<5) | (stb__OMatch5[b][0] << 11);
      //min16 = (stb__OMatch5[r][1]<<0) | (stb__OMatch6[g][1]<<5) | (stb__OMatch5[b][1] << 11);
   } else {
      At1[0] = At1[1] = At1[2] = 0;
      At2[0] = At2[1] = At2[2] = 0;
      for (i=0;i<16;++i,cm>>=2) {
         int step = cm&3;
         int w1 = w1Tab[step];
         int r = block[i*4+0];
         int g = block[i*4+1];
         int b = block[i*4+2];

         akku    += prods[step];
         At1[0]   += w1*r;
         At1[1]   += w1*g;
         At1[2]   += w1*b;
         At2[0]   += r;
         At2[1]   += g;
         At2[2]   += b;
      }

      At2[0] = 3*At2[0] - At1[0];
      At2[1] = 3*At2[1] - At1[1];
      At2[2] = 3*At2[2] - At1[2];

      // extract solutions and decide solvability
      xx = akku >> 16;
      yy = (akku >> 8) & 0xff;
      xy = (akku >> 0) & 0xff;

      frb = 3.0f * 31.0f / 255.0f / (xx*yy - xy*xy);
      fg = frb * 63.0f / 31.0f;

      // solve.
      int r, g ,b;
      r = clamp((int)((At1[0]*yy - At2[0]*xy)*frb+0.5f), 0, 31);
      g = clamp((int)((At1[1]*yy - At2[1]*xy)*fg +0.5f), 0, 63);
      b = clamp((int)((At1[2]*yy - At2[2]*xy)*frb+0.5f), 0, 31);
      maxMin[0] = (r<<3)|(g<<10)|(b<<19);
      //stb__From16Bit((unsigned char*)maxMin, max16);

      r = clamp((int)((At2[0]*xx - At1[0]*xy)*frb+0.5f), 0, 31);
      g = clamp((int)((At2[1]*xx - At1[1]*xy)*fg +0.5f), 0, 63);
      b = clamp((int)((At2[2]*xx - At1[2]*xy)*frb+0.5f), 0, 31);
      //stb__From16Bit((unsigned char*)(maxMin+1), min16);
      maxMin[1] = (r<<3)|(g<<10)|(b<<19);
   }
   if (maxMin32[0] == maxMin[0] && maxMin32[1] == maxMin[1])
     return false;
   maxMin32[0] = maxMin[0];
   maxMin32[1] = maxMin[1];
   return true;
}
#endif

// Color block compression
template<bool high>
static __forceinline void stb__CompressColorBlockProduction(unsigned char *dest, unsigned char *block)
{
   unsigned int mask;
   int i;
   unsigned short max16, min16;
   ALIGN16(unsigned int maxmin32[2]);

   // check if block is constant
   unsigned color0 = (((unsigned int *) block)[0])&0xFFFFFF;
   for (i=1;i<16;i++)
      if ((((unsigned int *) block)[i]&0xFFFFFF) != color0)
         break;

   if(i == 16) { // constant color
      mask  = 0;
      min16 = max16 = stb__As16Bit(block);
   } else {
      // second step: pca+map along principal axis
      stb__OptimizeColorsBlock<4>(block, maxmin32);
      if (maxmin32[0] != maxmin32[1])
      {
        mask = stb__MatchColorsBlockProduction<high>(block,(unsigned char*)maxmin32);

        // third step: refine 
        if (high && stb__RefineBlock(block,maxmin32,mask))
        {
           if (maxmin32[0] != maxmin32[1])
              mask = stb__MatchColorsBlockExcellent(block,(unsigned char*)maxmin32);
           else
              mask = 0;
        }
        max16 = ((maxmin32[0]&0xF8)>>3) | (((maxmin32[0]>>8)&0xFC)<<3) | (((maxmin32[0]>>16)&0xF8)<<8);
        min16 = ((maxmin32[1]&0xF8)>>3) | (((maxmin32[1]>>8)&0xFC)<<3) | (((maxmin32[1]>>16)&0xF8)<<8);

      }
      else
      {
        mask = 0;
        max16 = min16 = ((maxmin32[0]&0xF8)>>3) | (((maxmin32[0]>>8)&0xFC)<<3) | (((maxmin32[0]>>16)&0xF8)<<8);
      }
  }

  // write the color block
  if (max16 < min16)
  {
     unsigned short t = min16;
     min16 = max16;
     max16 = t;
     mask ^= 0x55555555;
  }
  #if TARGET_BE
  dest[0] = (unsigned char) (max16);
  dest[1] = (unsigned char) (max16 >> 8);
  dest[2] = (unsigned char) (min16);
  dest[3] = (unsigned char) (min16 >> 8);
  dest[4] = (unsigned char) (mask);
  dest[5] = (unsigned char) (mask >> 8);
  dest[6] = (unsigned char) (mask >> 16);
  dest[7] = (unsigned char) (mask >> 24);
  #else
  *((unsigned short*) (dest+0)) = max16;
  *((unsigned short*) (dest+2)) = min16;
  *((unsigned*) (dest+4)) = mask;
  #endif
}

static __forceinline void stb__CompressColorBlockExcellent(unsigned char *dest, unsigned char *block, int refinecount)
{
  unsigned int mask;
  int i;
  unsigned short max16, min16;
  ALIGN16(unsigned int maxmin32[2]);
  
  // check if block is constant
  unsigned color0 = (((unsigned int *) block)[0])&0xFFFFFF;
  for (i=1;i<16;i++)
     if ((((unsigned int *) block)[i]&0xFFFFFF) != color0)
        break;

  if(i == 16) { // constant color
     mask  = 0;
     min16 = max16 = stb__As16Bit(block);
  } else {
    // second step: pca+map along principal axis
    stb__OptimizeColorsBlock<6>(block, maxmin32);
    if (maxmin32[0] != maxmin32[1])
    {
      mask = stb__MatchColorsBlockExcellent(block, (unsigned char*)maxmin32);
      // third step: refine (multiple times if requested)
      for (i=0;i<refinecount;i++)
      {
        if (!stb__RefineBlock(block,maxmin32,mask))
          break;
        if (maxmin32[0] != maxmin32[1])
          mask = stb__MatchColorsBlockExcellent(block,(unsigned char*)maxmin32);
        else
        {
          mask = 0;
          break;
        }
      }
      max16 = ((maxmin32[0]&0xF8)>>3) | (((maxmin32[0]>>8)&0xFC)<<3) | (((maxmin32[0]>>16)&0xF8)<<8);
      min16 = ((maxmin32[1]&0xF8)>>3) | (((maxmin32[1]>>8)&0xFC)<<3) | (((maxmin32[1]>>16)&0xF8)<<8);
    } else
    {
      mask = 0;
      min16 = max16 = ((maxmin32[0]&0xF8)>>3) | (((maxmin32[0]>>8)&0xFC)<<3) | (((maxmin32[0]>>16)&0xF8)<<8);
    }
  }

  // write the color block
  if (max16 < min16)
  {
     unsigned short t = min16;
     min16 = max16;
     max16 = t;
     mask ^= 0x55555555;
  }
  #if TARGET_BE
  dest[0] = (unsigned char) (max16);
  dest[1] = (unsigned char) (max16 >> 8);
  dest[2] = (unsigned char) (min16);
  dest[3] = (unsigned char) (min16 >> 8);
  dest[4] = (unsigned char) (mask);
  dest[5] = (unsigned char) (mask >> 8);
  dest[6] = (unsigned char) (mask >> 16);
  dest[7] = (unsigned char) (mask >> 24);
  #else
  *((unsigned short*) (dest+0)) = max16;
  *((unsigned short*) (dest+2)) = min16;
  *((unsigned*) (dest+4)) = mask;
  #endif
}

static __forceinline int get_alpha5_error(unsigned char mn2, unsigned char mx2, unsigned char *src)
{
  unsigned char alpha0 = mn2;
  unsigned char alpha1 = mx2;
  unsigned char mid = ( alpha1 - alpha0 ) / ( 2 * 5 );
  unsigned char ab1 = alpha0 + mid;
  unsigned char a3 = ( 4 * alpha0 + 1 *  alpha1) / 5 ;
  unsigned char a4 = ( 3 * alpha0 + 2 *  alpha1) / 5 ;
  unsigned char a5 = ( 2 * alpha0 + 3 *  alpha1) / 5 ;
  unsigned char a6 = ( 1 * alpha0 + 4 *  alpha1) / 5 ;
  unsigned char ab3 = a3 + mid;
  unsigned char ab4 = a4 + mid;
  unsigned char ab5 = a5 + mid;
  unsigned char ab6 = a6 + mid;

  unsigned char *colorBlock = src+3;
  int err = 0;

  for ( int i = 0; i < 16; i++ ) {

    int a = colorBlock[i*4];
    if (a<mn2)
      ;
    else if (a<=ab1)
      err += (a-alpha0)*(a-alpha0);
    else if (a<=ab3)
      err += (a-a3)*(a-a3);
    else if (a<=ab4)
      err += (a-a4)*(a-a4);
    else if (a<=ab5)
      err += (a-a5)*(a-a5);
    else if (a<=ab6)
      err += (a-a6)*(a-a6);
    else if (a<=mx2)
      err += (a-alpha1)*(a-alpha1);
    else 
     ;
  }
  return err;
}

static __forceinline int get_alpha7_error(unsigned char mn, unsigned char mx, unsigned char *src)
{
  unsigned char alpha0 = mx;
  unsigned char alpha1 = mn;
  unsigned char mid = ( alpha0 - alpha1 ) / ( 2 * 7 );
  unsigned char ab1 = alpha1 + mid;
  unsigned char a2 = ( 6 * alpha0 + 1 * alpha1) / 7;
  unsigned char a3 = ( 5 * alpha0 + 2 * alpha1) / 7;
  unsigned char a4 = ( 4 * alpha0 + 3 * alpha1) / 7;
  unsigned char a5 = ( 3 * alpha0 + 4 * alpha1) / 7;
  unsigned char a6 = ( 2 * alpha0 + 5 * alpha1) / 7;
  unsigned char a7 = ( 1 * alpha0 + 6 * alpha1) / 7;
  unsigned char ab2 = a2 + mid;
  unsigned char ab3 = a3 + mid;
  unsigned char ab4 = a4 + mid;
  unsigned char ab5 = a5 + mid;
  unsigned char ab6 = a6 + mid;
  unsigned char ab7 = a7 + mid;

  unsigned char *colorBlock = src+3;
  int err = 0;

  for ( int i = 0; i < 16; i++ )
  {
    int a = colorBlock[i*4];
    if (a<=ab1)
      err += (a-alpha1)*(a-alpha1);
    else if (a<=ab7)
      err += (a-a7)*(a-a7);
    else if (a<=ab6)
      err += (a-a6)*(a-a6);
    else if (a<=ab5)
      err += (a-a5)*(a-a5);
    else if (a<=ab4)
      err += (a-a4)*(a-a4);
    else if (a<=ab3)
      err += (a-a3)*(a-a3);
    else if (a<=ab2)
      err += (a-a2)*(a-a2);
    else 
      err += (a-alpha0)*(a-alpha0);
  }
  return err;
}


// Alpha block compression (this is easy for a change)
static __forceinline void compress_dxt5ablock_5(unsigned char *dest,unsigned char *src, int mn2, int mx2)
{
   ((unsigned char *)dest)[0] = mn2;
   ((unsigned char *)dest)[1] = mx2;

   dest += 2;
   unsigned char indices[16];
   unsigned char alpha0 = mn2;
   unsigned char alpha1 = mx2;
   unsigned char mid = ( alpha1 - alpha0 ) / ( 2 * 5 );
   unsigned char ab1 = alpha0 + mid;
   unsigned char ab3 = ( 4 * alpha0 + 1 *  alpha1) / 5 + mid;
   unsigned char ab4 = ( 3 * alpha0 + 2 *  alpha1) / 5 + mid;
   unsigned char ab5 = ( 2 * alpha0 + 3 *  alpha1) / 5 + mid;
   unsigned char ab6 = ( 1 * alpha0 + 4 *  alpha1) / 5 + mid;

   unsigned char *colorBlock = src+3;

   for ( int i = 0; i < 16; i++ ) {

     unsigned char a = colorBlock[i*4];
     if (a<mn2)
       indices[i] = 6;
     else if (a>mx2)
       indices[i] = 7;
     else {
       int b1 = ( a <= ab1 );
       int b3 = ( a <= ab3 );
       int b4 = ( a <= ab4 );
       int b5 = ( a <= ab5 );
       int b6 = ( a <= ab6 );

       int index = ( b1 + b3 + b4 + b5 + b6 );
       static const char remap[] = {1, 5, 4, 3, 2, 0};

       indices[i] = remap[index];

     }
   }

   dest[0] = (indices[ 0] >> 0) | (indices[ 1] << 3) | (indices[ 2] << 6) ;
   dest[1] = (indices[ 2] >> 2) | (indices[ 3] << 1) | (indices[ 4] << 4) | (indices[ 5] << 7) ;
   dest[2] = (indices[ 5] >> 1) | (indices[ 6] << 2) | (indices[ 7] << 5) ;
   dest[3] = (indices[ 8] >> 0) | (indices[ 9] << 3) | (indices[10] << 6) ;
   dest[4] = (indices[10] >> 2) | (indices[11] << 1) | (indices[12] << 4) | (indices[13] << 7) ;
   dest[5] = (indices[13] >> 1) | (indices[14] << 2) | (indices[15] << 5) ;
}

static __forceinline void stb__CompressAlphaBlockExcellent(unsigned char *dest,unsigned char *src)
{
   int i,bias,dist4,dist2,bits,mask;

   // find min/max color
   int mn = 256, mx = -1;
   int mn2 = 256, mx2 = -1;

   for (i=0;i<16;i++)
   {
     int a = src[i*4+3];
     if (a < mn) mn = a;
     if (a > mx) mx = a;

     if (a != 0 && a != 255)
     {
       if (a < mn2)
         mn2 = a;
       if (a > mx2)
         mx2 = a;
     }
   }
   int dist = mx - mn;
   int sdist = mx2 - mn2;
   if (dist > 0 && sdist >= 0 && (sdist/5<dist/7 || get_alpha7_error(mn, mx, src) > get_alpha5_error(mn2, mx2, src)))
   {
     compress_dxt5ablock_5(dest, src, mn2, mx2);
     return;
   }

   // encode them
   if (dist==0)
   {
     *(unsigned*)dest = mx|(mn<<8);
     ((unsigned*)dest)[1] = 0;
   } else
     fastDXT::write_BC4_block_rgba( src, mn, mx, dest );
}

template<bool check_alpha_5 = true>
static __forceinline void stb__CompressAlphaBlockProduction(unsigned char *dest,unsigned char *src)
{
   int i,bias,dist4,dist2,bits,mask;

   // find min/max color
   int mn = 256, mx = -1;
   int mn2 = 256, mx2 = -1;

   for (i=0;i<16;i++)
   {
     int a = src[i*4+3];
     if (a < mn) mn = a;
     if (a > mx) mx = a;

     if (a != 0 && a != 255)
     {
       if (a < mn2)
         mn2 = a;
       if (a > mx2)
         mx2 = a;
     }
   }
   int dist = mx - mn;
   int sdist = mx2 - mn2;
   if (check_alpha_5 && dist > 0 && sdist >= 0 && (sdist/5<dist/7))
   {
     compress_dxt5ablock_5(dest, src, mn2, mx2);
     return;
   }

   // encode them
   if (dist==0)
   {
     *(unsigned*)dest = mx|(mn<<8);
     ((unsigned*)dest)[1] = 0;
   } else
     fastDXT::write_BC4_block_rgba( src, mn, mx, dest );
}

static void stb__InitDXT()
{
   int i;
   for(i=0;i<32;i++)
      stb__Expand5[i] = (i<<3)|(i>>2);

   for(i=0;i<64;i++)
      stb__Expand6[i] = (i<<2)|(i>>4);

   for(i=0;i<256+16;i++)
   {
      int v = i-8 < 0 ? 0 : i-8 > 255 ? 255 : i-8;
      stb__QuantRBTab[i] = stb__Expand5[stb__Mul8Bit(v,31)];
      stb__QuantGTab[i] = stb__Expand6[stb__Mul8Bit(v,63)];
   }

   stb__PrepareOptTable(&stb__OMatch5[0][0],stb__Expand5,32);
   stb__PrepareOptTable(&stb__OMatch6[0][0],stb__Expand6,64);
}

typedef unsigned char byte;

void CompressImageDXT1Excellent( const unsigned char *inBuf, unsigned char *outBuf, int width, int height, int refine, int dxt_pitch)
{
  ALIGN16( byte *outData );
  ALIGN16( byte block[64] );

  outData = outBuf;
  int stride_div = dxt_pitch - ((width + 3)&~3) * 2;
  if (width % 4 != 0 || height % 4 != 0)
  {
    for ( int j = 0; j < height; j += 4, inBuf += width * 4*4, outData += stride_div)
      for ( int i = 0; i < width; i += 4, outData += 8 )
      {
        fastDXT::ExtractBlockSmall( inBuf + i * 4, width - i, height - j, block );
        stb__CompressColorBlockExcellent(outData, block, refine);
      }
    return;
  }
  for ( int j = 0; j < height; j += 4, inBuf += width * 4*4, outData += stride_div)
    for ( int i = 0; i < width; i += 4, outData += 8 )
    {
      fastDXT::ExtractBlockU(inBuf + i * 4, width, block );

      stb__CompressColorBlockExcellent(outData, block, refine);
    }
}

void CompressImageDXT5Excellent( const unsigned char *inBuf, unsigned char *outBuf, int width, int height, int refine, int dxt_pitch)
{
  ALIGN16( byte *outData );
  ALIGN16( byte block[64] );

  outData = outBuf;
  int stride_div = dxt_pitch - ((width + 3)&~3) * 4;
  if (width % 4 != 0 || height % 4 != 0)
  {
    for ( int j = 0; j < height; j += 4, inBuf += width * 4*4, outData += stride_div)
      for ( int i = 0; i < width; i += 4, outData += 16 )
      {
        fastDXT::ExtractBlockSmall( inBuf + i * 4, width - i, height - j, block );
        stb__CompressAlphaBlockExcellent(outData, block);
        stb__CompressColorBlockExcellent(outData+8, block, refine);
      }
    return;
  }
  for ( int j = 0; j < height; j += 4, inBuf += width * 4*4, outData += stride_div)
    for ( int i = 0; i < width; i += 4, outData += 16 )
    {
      fastDXT::ExtractBlockU(inBuf + i * 4, width, block );
      stb__CompressAlphaBlockExcellent(outData, block);
      stb__CompressColorBlockExcellent(outData+8, block, refine);
    }
}

void CompressImageDXT3Excellent( const unsigned char *inBuf, unsigned char *outBuf, int width, int height, int refine, int dxt_pitch)
{
  ALIGN16( byte *outData );
  ALIGN16( byte block[64] );

  outData = outBuf;
  int stride_div = dxt_pitch - ((width + 3)&~3) * 4;
  if (width % 4 != 0 || height % 4 != 0)
  {
    for ( int j = 0; j < height; j += 4, inBuf += width * 4*4, outData += stride_div)
      for ( int i = 0; i < width; i += 4, outData += 16 )
      {
        fastDXT::ExtractBlockSmall( inBuf + i * 4, width - i, height - j, block );
        fastDXT::compress_dxt3_alpha(block, outData);
        stb__CompressColorBlockExcellent(outData+8, block, refine);
      }
    return;
  }
  for ( int j = 0; j < height; j += 4, inBuf += width * 4*4, outData += stride_div)
    for ( int i = 0; i < width; i += 4, outData += 16 )
    {
      fastDXT::ExtractBlockU(inBuf + i * 4, width, block );
      fastDXT::compress_dxt3_alpha(block, outData);
      stb__CompressColorBlockExcellent(outData+8, block, refine);
    }
}

template<bool high, bool aligned>
void CompressImageDXT1Production( const unsigned char *inBuf, unsigned char *outBuf, int width, int height, int dxt_pitch)
{
  ALIGN16( byte *outData );
  ALIGN16( byte block[64] );

  outData = outBuf;
  int stride_div = dxt_pitch - ((width + 3)&~3) * 2;
  if (width % 4 != 0 || height % 4 != 0)
  {
    for ( int j = 0; j < height; j += 4, inBuf += width * 4*4, outData += stride_div)
      for ( int i = 0; i < width; i += 4, outData += 8 )
      {
        fastDXT::ExtractBlockSmall( inBuf + i * 4, width - i, height - j, block );
        stb__CompressColorBlockProduction<high>(outData, block);
      }
    return;
  }
  for ( int j = 0; j < height; j += 4, inBuf += width * 4*4, outData += stride_div)
    for ( int i = 0; i < width; i += 4, outData += 8 )
    {
      aligned ? fastDXT::ExtractBlockA(inBuf + i * 4, width, block ) : fastDXT::ExtractBlockU(inBuf + i * 4, width, block );
      stb__CompressColorBlockProduction<high>(outData, block);
    }
}

template<bool high, bool aligned>
void CompressImageDXT5Production( const unsigned char *inBuf, unsigned char *outBuf, int width, int height, int dxt_pitch)
{
  ALIGN16( byte *outData );
  ALIGN16( byte block[64] );

  outData = outBuf;
  int stride_div = dxt_pitch - ((width + 3)&~3) * 4;
  if (width % 4 != 0 || height % 4 != 0)
  {
    for ( int j = 0; j < height; j += 4, inBuf += width * 4*4, outData += stride_div)
      for ( int i = 0; i < width; i += 4, outData += 16 )
      {
        fastDXT::ExtractBlockSmall( inBuf + i * 4, width - i, height - j, block );
        stb__CompressAlphaBlockProduction<high>(outData, block);
        stb__CompressColorBlockProduction<high>(outData+8, block);
      }
    return;
  }
  for ( int j = 0; j < height; j += 4, inBuf += width * 4*4, outData += stride_div)
    for ( int i = 0; i < width; i += 4, outData += 16 )
    {
      aligned ? fastDXT::ExtractBlockA(inBuf + i * 4, width, block ) : fastDXT::ExtractBlockU(inBuf + i * 4, width, block );
      stb__CompressAlphaBlockProduction<high>(outData, block);
      stb__CompressColorBlockProduction<high>(outData+8, block);
    }
}

template<bool high, bool aligned>
void CompressImageDXT3Production( const unsigned char *inBuf, unsigned char *outBuf, int width, int height, int dxt_pitch)
{
  ALIGN16( byte *outData );
  ALIGN16( byte block[64] );

  outData = outBuf;
  int stride_div = dxt_pitch - ((width + 3)&~3) * 4;
  if (width % 4 != 0 || height % 4 != 0)
  {
    for ( int j = 0; j < height; j += 4, inBuf += width * 4*4, outData += stride_div)
      for ( int i = 0; i < width; i += 4, outData += 16 )
      {
        fastDXT::ExtractBlockSmall( inBuf + i * 4, width - i, height - j, block );
        fastDXT::compress_dxt3_alpha(block, outData);
        stb__CompressColorBlockProduction<high>(outData+8, block);
      }
    return;
  }
  for ( int j = 0; j < height; j += 4, inBuf += width * 4*4, outData += stride_div)
    for ( int i = 0; i < width; i += 4, outData += 16 )
    {
      aligned ? fastDXT::ExtractBlockA(inBuf + i * 4, width, block ) : fastDXT::ExtractBlockU(inBuf + i * 4, width, block );
      fastDXT::compress_dxt3_alpha(block, outData);
      stb__CompressColorBlockProduction<high>(outData+8, block);
    }
}
#define CALL_FUNC_FOR_ALIGNMENT(FUNC, HIGH, INBUF, OUTBUF, W, H, P) \
  do { \
    if (intptr_t((const void*)inBuf)&15) \
      return FUNC<HIGH, false>(INBUF, OUTBUF, W, H, P); \
    else \
      return FUNC<HIGH, true>(INBUF, OUTBUF, W, H, P); \
  } while (0)

void CompressImageDXT1( const unsigned char *inBuf, unsigned char *outBuf, int width, int height, int mode, int dxt_pitch)
{
  if (mode >= STB_DXT_HIGHQUAL)
    return CompressImageDXT1Excellent(inBuf, outBuf, width, height, 4, dxt_pitch);
  else if (mode == STB_DXT_NORMAL)
    CALL_FUNC_FOR_ALIGNMENT(CompressImageDXT1Production, true, inBuf, outBuf, width, height, dxt_pitch);
  else
    CALL_FUNC_FOR_ALIGNMENT(CompressImageDXT1Production, false, inBuf, outBuf, width, height, dxt_pitch);
}
void CompressImageDXT5( const unsigned char *inBuf, unsigned char *outBuf, int width, int height, int mode, int dxt_pitch)
{
  if (mode >= STB_DXT_HIGHQUAL)
    return CompressImageDXT5Excellent(inBuf, outBuf, width, height, 4, dxt_pitch);
  else if (mode == STB_DXT_NORMAL)
    CALL_FUNC_FOR_ALIGNMENT(CompressImageDXT5Production, true, inBuf, outBuf, width, height, dxt_pitch);
  else
    CALL_FUNC_FOR_ALIGNMENT(CompressImageDXT5Production, false, inBuf, outBuf, width, height, dxt_pitch);
}

void CompressImageDXT3( const unsigned char *inBuf, unsigned char *outBuf, int width, int height, int mode, int dxt_pitch)
{
  if (mode&STB_DXT_HIGHQUAL)
    return CompressImageDXT3Excellent(inBuf, outBuf, width, height, 4, dxt_pitch);
  else if (mode == STB_DXT_NORMAL)
    CALL_FUNC_FOR_ALIGNMENT(CompressImageDXT3Production, true, inBuf, outBuf, width, height, dxt_pitch);
  else
    CALL_FUNC_FOR_ALIGNMENT(CompressImageDXT3Production, false, inBuf, outBuf, width, height, dxt_pitch);
}

static struct StbDxtInitialize
{
  StbDxtInitialize()
  {
    stb__InitDXT();
  }
} stb_initializer;

};//namespace