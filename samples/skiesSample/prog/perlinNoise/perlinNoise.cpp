// Copyright (C) Gaijin Games KFT.  All rights reserved.

/********************************************************
Perlin Noise2 and Noise3 functions Optimised for
Intel SIMD (PIII CeleronII) Instruction Set.

SIMD versions process 4 input vectors at once.

SIMD versions run more than twice as fast :)

This is my first attempt at using SIMD so constructive
comments welcome - any one from Intel reading ?

Please excuse the gross use of prefetches! I just threw
them in there (they ARE needed)!

I'm SURE these are sub-optimal.

Compiled with Intel's excellent Performance Compiler V5.0
30 day trial version from Intel's Website
(Boy does this produce fast with vanilla C!!)

Rob James

email	robjames@europe.com
home	http://www.pocketmoon.com
April 2001

******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <xmmintrin.h> // intel simd header
#include "perlinNoise.h"

//***  Note use of floats ***

// SIMD is basically 4 floats at once (or 2 doubles...)


// from standard Noise functions
#define B  0x100
#define BM 0xff
#define N  0x100000
// #define N	 ((float)1048576.0)
#define NP 12 /* 2^N */
#define NM 0xfff
int p[B + B + 2];

// alignment required for SIMD intrinsics to work
_declspec(align(32)) float g2[B + B + 2][2]; // CHANGED FROM DOUBLE TO FLOAT and 3 to 2//
_declspec(align(32)) float g3[B + B + 2][3]; // CHANGED FROM DOUBLE TO FLOAT and 3 to 2//


/* cubic spline interpolation */
#define s_curve(t) (t * t * (3. - 2. * t))

/* linear interpolation */
#define lerp(t, a, b) (a + t * (b - a))

// For noise3
#define at3(rx, ry, rz) (rx * q[0] + ry * q[1] + rz * q[2])

// for noise2
#define at2(rx, ry) (rx * q[0] + ry * q[1])

#define setup(u, b0, b1, r0, r1) \
  t = u + N;                     \
  b0 = ((int)t) & BM;            \
  b1 = (b0 + 1) & BM;            \
  r0 = t - (int)t;               \
  r1 = r0 - 1.;


// anyone who's used Procedural noise should recognise these two functiomns ...
void Noise3(Vector vec, float *result)
{
  int bx0, bx1, by0, by1, bz0, bz1, b00, b10, b01, b11;
  float rx0, rx1, ry0, ry1, rz0, rz1, *q, sx, sy, sz;
  float a, b, c, d, u, v, t;
  int i, j;

  setup(vec.x, bx0, bx1, rx0, rx1);
  setup(vec.y, by0, by1, ry0, ry1);
  setup(vec.z, bz0, bz1, rz0, rz1);

  i = p[bx0];
  j = p[bx1];

  b00 = p[i + by0];
  b10 = p[j + by0];
  b01 = p[i + by1];
  b11 = p[j + by1];

  sx = s_curve(rx0);
  sy = s_curve(ry0);
  sz = s_curve(rz0);

  q = g3[b00 + bz0];
  u = at3(rx0, ry0, rz0);
  q = g3[b10 + bz0];
  v = at3(rx1, ry0, rz0);
  a = lerp(sx, u, v);

  q = g3[b01 + bz0];
  u = at3(rx0, ry1, rz0);
  q = g3[b11 + bz0];
  v = at3(rx1, ry1, rz0);
  b = lerp(sx, u, v);

  c = lerp(sy, a, b); /* interpolate in y at lo z */

  q = g3[b00 + bz1];
  u = at3(rx0, ry0, rz1);
  q = g3[b10 + bz1];
  v = at3(rx1, ry0, rz1);
  a = lerp(sx, u, v);

  q = g3[b01 + bz1];
  u = at3(rx0, ry1, rz1);
  q = g3[b11 + bz1];
  v = at3(rx1, ry1, rz1);
  b = lerp(sx, u, v);

  d = lerp(sy, a, b);             /* interpolate in y at hi z */
  *result = 1.5 * lerp(sz, c, d); /* interpolate in z */
}


void Noise2(Vector vec, float *result)
{
  int bx0, bx1, by0, by1, b00, b10, b01, b11;
  float rx0, rx1, ry0, ry1, *q, sx, sy, a, b, u, v, t;
  int i, j;

  setup(vec.x, bx0, bx1, rx0, rx1);
  setup(vec.y, by0, by1, ry0, ry1);

  i = p[bx0];
  j = p[bx1];

  b00 = p[i + by0];
  b10 = p[j + by0];
  b01 = p[i + by1];
  b11 = p[j + by1];

  sx = s_curve(rx0);
  sy = s_curve(ry0);

  q = g2[b00];       /* get random gradient */
  u = at2(rx0, ry0); /* get weight on lo x side (lo y) */
  q = g2[b10];
  v = at2(rx1, ry0);  /* get weight on hi x side (lo y) */
  a = lerp(sx, u, v); /* get value at distance sx between u & v */

  /* similarly at hi y... */
  q = g2[b01];
  u = at2(rx0, ry1);
  q = g2[b11];
  v = at2(rx1, ry1);
  b = lerp(sx, u, v);

  *result = 1.5 * lerp(sy, a, b); /* interpolate in y */
}


// Now my 4-at-a-time functions using Intels SIMD intrinsics...
void Noise24(Vector vec1, Vector vec2, Vector vec3, Vector vec4, float *result) /* MUST BE ALIGN(16)*/
{

  int b000, b001, b002, b003, b100, b101, b102, b103, b010, b011, b012, b013, b110, b111, b112, b113;

  int bx00, bx01, bx02, bx03, bx10, bx11, bx12, bx13, by00, by01, by02, by03, by10, by11, by12, by13;

  float tx0, tx1, tx2, tx3, ty0, ty1, ty2, ty3;


  // first sight of an intrinsic!
  __m128 rx0, rx1, ry0, ry1, tmp, tmp2, q0, q1;
  __m128 sx, sy, a, b, u, v, two, three;

  int i0, i1, i2, i3, j1, j2, j3, j0;

  // put (vec.x + 0x100000) into a temp var ??
  // ISSUE : bx0 etc are ints converted from floats
  // this is possible using __m64 _mm_cvtps_pi16( __m128 a) (composite)
  // but would entail using MMX (swich penalty) hmmm...

  //
  // When mixing MMX and SIMD we need to do a mm_empty - this could be slow
  // maybe not needed when using convert ???
  //

#define MM 1048576.0

  int itx1, itx2, itx3, itx0, ity1, ity2, ity3, ity0;

  /*
  __m64 _mm_cvtps_pi16( __m128 a) (composite)
  Convert the four single precision FP values in a to four signed 16-bit integer values.
  r0 := (short)a0
  r1 := (short)a1
  r2 := (short)a2
  r3 := (short)a3
  */

  tx0 = (vec1.x + MM);
  tx1 = (vec2.x + MM);
  tx2 = (vec3.x + MM);
  tx3 = (vec4.x + MM);

  itx0 = (int)(tx0);
  itx1 = (int)(tx1);
  itx2 = (int)(tx2);
  itx3 = (int)(tx3);

  bx00 = itx0 & 0xff;
  bx10 = (bx00 + 1) & 0xff;

  bx01 = itx1 & 0xff;
  bx11 = (bx01 + 1) & 0xff;

  bx02 = itx2 & 0xff;
  bx12 = (bx02 + 1) & 0xff;

  bx03 = itx3 & 0xff;
  bx13 = (bx03 + 1) & 0xff;

  ty0 = (vec1.y + MM);
  ity0 = (int)(ty0); //-0.5
  by00 = ity0 & 0xff;
  by10 = (by00 + 1) & 0xff;

  ty1 = (vec2.y + MM);
  ity1 = (int)(ty1); //-.5
  by01 = ity1 & 0xff;
  by11 = (by01 + 1) & 0xff;

  ty2 = (vec3.y + MM);
  ity2 = (int)(ty2); //-0.5
  by02 = ity2 & 0xff;
  by12 = (by02 + 1) & 0xff;

  ty3 = (vec4.y + MM);
  ity3 = (int)(ty3); //-0.5
  by03 = ity3 & 0xff;
  by13 = (by03 + 1) & 0xff;


  // could user  post incrementing pointers here...
  i0 = p[bx00];
  j0 = p[bx10];
  i1 = p[bx01];
  j1 = p[bx11];
  i2 = p[bx02];
  j2 = p[bx12];
  i3 = p[bx03];
  j3 = p[bx13];

  b000 = p[i0 + by00];
  b010 = p[i0 + by10];
  b100 = p[j0 + by00];
  b110 = p[j0 + by10];

  b001 = p[i1 + by01];
  b011 = p[i1 + by11];
  b101 = p[j1 + by01];
  b111 = p[j1 + by11];

  b002 = p[i2 + by02];
  b012 = p[i2 + by12];
  b102 = p[j2 + by02];
  b112 = p[j2 + by12];

  b003 = p[i3 + by03];
  b013 = p[i3 + by13];
  b103 = p[j3 + by03];
  b113 = p[j3 + by13];

  //!! try using FAST floattoint ... - tried it, slower :)
  rx0 = _mm_set_ps(tx0 - itx0, tx1 - itx1, tx2 - itx2, tx3 - itx3);


  // may be faster as another mm_set_ps with -1 at the end of each term
  tmp = _mm_set_ps1(1.0);
  rx1 = _mm_sub_ps(rx0, tmp);
  ry0 = _mm_set_ps(ty0 - ity0, ty1 - ity1, ty2 - ity2, ty3 - ity3);

  ry1 = _mm_sub_ps(ry0, tmp);


  // Now, these prefetches make a HUGE difference.
  // Without them the whole function is memory bound and no faster
  // than the original functions
  //
  // I have applied no real science in placement of these prefetches
  // I just chuck them in a few statements before I need them!
  _mm_prefetch((char *)&(g2[b000]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g2[b001]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g2[b002]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g2[b003]), _MM_HINT_NTA);


  // you will now see original code as comments followed by simd equivalent...

  //	sx =  rx0 * rx0 * (3.0 - 2.0 * rx0);
  two = _mm_set_ps1(2.0); // Might be good to PRESTORE the constant arrays 1.0, 2.0 etc
  sx = _mm_mul_ps(rx0, two);
  three = _mm_set_ps1(3.0);
  tmp2 = _mm_sub_ps(three, sx);
  tmp = _mm_mul_ps(tmp2, rx0);
  sx = _mm_mul_ps(tmp, rx0);

  _mm_prefetch((char *)&(g2[b100]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g2[b101]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g2[b102]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g2[b103]), _MM_HINT_NTA);


  // sy =  ry0 * ry0 * (3.0 - 2.0 * ry0);
  // tmp  = _mm_set_ps1 (2.0)  ;
  sy = _mm_mul_ps(ry0, two);

  // tmp  = _mm_set_ps1 (3.0);
  tmp2 = _mm_sub_ps(three, sy);
  tmp = _mm_mul_ps(tmp2, ry0);
  sy = _mm_mul_ps(tmp, ry0);

  _mm_prefetch((char *)&(g2[b010]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g2[b011]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g2[b012]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g2[b013]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g2[b110]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g2[b111]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g2[b112]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g2[b113]), _MM_HINT_NTA);


  //	q = g2[ b00 ];		/* get random gradient */
  //	u = rx0 * q[0] + ry0 * q[1];
  q0 = _mm_set_ps(g2[b000][0], g2[b001][0], g2[b002][0], g2[b003][0]);
  q1 = _mm_set_ps(g2[b000][1], g2[b001][1], g2[b002][1], g2[b003][1]);
  tmp = _mm_mul_ps(q1, ry0);
  tmp2 = _mm_mul_ps(q0, rx0);
  u = _mm_add_ps(tmp, tmp2);


  //	q = g2[ b10 ];
  //	v = rx1 * q[0] + ry0 * q[1] ;
  q0 = _mm_set_ps(g2[b100][0], g2[b101][0], g2[b102][0], g2[b103][0]);
  q1 = _mm_set_ps(g2[b100][1], g2[b101][1], g2[b102][1], g2[b103][1]);
  tmp = _mm_mul_ps(q1, ry0);
  tmp2 = _mm_mul_ps(q0, rx1);
  v = _mm_add_ps(tmp, tmp2);


  //    a = u + sx * (v-u);  /* get value at distance sx between u & v */
  tmp = _mm_sub_ps(v, u);
  tmp2 = _mm_mul_ps(tmp, sx);
  a = _mm_add_ps(u, tmp2);


  //	q = g2[ b01 ];
  //	u = rx0*q[0] + ry1 * q[1];
  q0 = _mm_set_ps(g2[b010][0], g2[b011][0], g2[b012][0], g2[b013][0]);
  q1 = _mm_set_ps(g2[b010][1], g2[b011][1], g2[b012][1], g2[b013][1]);
  tmp = _mm_mul_ps(q1, ry1);
  tmp2 = _mm_mul_ps(q0, rx0);
  u = _mm_add_ps(tmp, tmp2);


  //	q = g2[ b11 ];
  //	v = rx1 * q[0] + ry1 * q[1];
  q0 = _mm_set_ps(g2[b110][0], g2[b111][0], g2[b112][0], g2[b113][0]);
  q1 = _mm_set_ps(g2[b110][1], g2[b111][1], g2[b112][1], g2[b113][1]);
  tmp = _mm_mul_ps(q1, ry1);
  tmp2 = _mm_mul_ps(q0, rx1);
  v = _mm_add_ps(tmp, tmp2);


  //	b = u + sx * (v-u);
  tmp = _mm_sub_ps(v, u);
  tmp2 = _mm_mul_ps(tmp, sx);
  b = _mm_add_ps(u, tmp2);

  //	result = 1.5 * (a + sy * (b-a))	/* interpolate in y */   why 1.5 ??
  tmp = _mm_sub_ps(b, a);
  tmp2 = _mm_mul_ps(tmp, sy);
  tmp = _mm_add_ps(tmp2, a);
  tmp2 = _mm_set_ps1(1.5);
  a = _mm_mul_ps(tmp2, tmp);

  _mm_store_ps(result, a); // aligned


  // phew!!
  //  As you see doing things 4-at-a-time does take some getting used to!
}


void Noise34(Vector vec1, Vector vec2, Vector vec3, Vector vec4, float *result) /* MUST BE ALIGN(16)*/
{

  // same as Noise24 but with the extra Z component included...

  int b000, b001, b002, b003, b100, b101, b102, b103, b010, b011, b012, b013, b110, b111, b112, b113;

  int bx00, bx01, bx02, bx03, bx10, bx11, bx12, bx13, by00, by01, by02, by03, by10, by11, by12, by13, bz00, bz01, bz02, bz03, bz10,
    bz11, bz12, bz13;

  float tx0, tx1, tx2, tx3, ty0, ty1, ty2, ty3, tz0, tz1, tz2, tz3;

  __m128 rx0, rx1, ry0, ry1, rz0, rz1, tmp, tmp2, tmp3, tmp4, q0, q1, q2;
  __m128 sx, sy, sz, a, b, c, d, u, v, two, three;

  int i0, i1, i2, i3, j1, j2, j3, j0;

  // put (vec.x + 0x100000) into a temp var ??
  // ISSUE : bx0 etc are ints converted from floats
  // this is possible using __m64 _mm_cvtps_pi16( __m128 a) (composite)
  // but would entail using MMX hmmm

  //
  // When mixing MMX and SIMD we need to do a mm_empty - this could be slow
  // maybe not needed when using convert ???
  //

#define MM 1048576.0

  int itx1, itx2, itx3, itx0, ity1, ity2, ity3, ity0, itz1, itz2, itz3, itz0;

  /*
  __m64 _mm_cvtps_pi16( __m128 a) (composite)
  Convert the four single precision FP values in a to four signed 16-bit integer values.
  r0 := (short)a0
  r1 := (short)a1
  r2 := (short)a2
  r3 := (short)a3
  */

  tx0 = (vec1.x + MM);
  tx1 = (vec2.x + MM);
  tx2 = (vec3.x + MM);
  tx3 = (vec4.x + MM);

  itx0 = (int)(tx0);
  itx1 = (int)(tx1);
  itx2 = (int)(tx2);
  itx3 = (int)(tx3);

  bx00 = itx0 & 0xff;
  bx10 = (bx00 + 1) & 0xff;

  bx01 = itx1 & 0xff;
  bx11 = (bx01 + 1) & 0xff;

  bx02 = itx2 & 0xff;
  bx12 = (bx02 + 1) & 0xff;

  bx03 = itx3 & 0xff;
  bx13 = (bx03 + 1) & 0xff;


  ty0 = (vec1.y + MM);
  ity0 = (int)(ty0);
  by00 = ity0 & 0xff;
  by10 = (by00 + 1) & 0xff;

  ty1 = (vec2.y + MM);
  ity1 = (int)(ty1);
  by01 = ity1 & 0xff;
  by11 = (by01 + 1) & 0xff;

  ty2 = (vec3.y + MM);
  ity2 = (int)(ty2);
  by02 = ity2 & 0xff;
  by12 = (by02 + 1) & 0xff;

  ty3 = (vec4.y + MM);
  ity3 = (int)(ty3);
  by03 = ity3 & 0xff;
  by13 = (by03 + 1) & 0xff;


  // ZED
  tz0 = (vec1.z + MM);
  itz0 = (int)(tz0);
  bz00 = itz0 & 0xff;
  bz10 = (bz00 + 1) & 0xff;

  tz1 = (vec2.z + MM);
  itz1 = (int)(tz1);
  bz01 = itz1 & 0xff;
  bz11 = (bz01 + 1) & 0xff;

  tz2 = (vec3.z + MM);
  itz2 = (int)(tz2);
  bz02 = itz2 & 0xff;
  bz12 = (bz02 + 1) & 0xff;

  tz3 = (vec4.z + MM);
  itz3 = (int)(tz3);
  bz03 = itz3 & 0xff;
  bz13 = (bz03 + 1) & 0xff;


  // could user  post incrementing pointers here...
  i0 = p[bx00];
  j0 = p[bx10];
  i1 = p[bx01];
  j1 = p[bx11];
  i2 = p[bx02];
  j2 = p[bx12];
  i3 = p[bx03];
  j3 = p[bx13];

  b000 = p[i0 + by00];
  b010 = p[i0 + by10];
  b100 = p[j0 + by00];
  b110 = p[j0 + by10];

  b001 = p[i1 + by01];
  b011 = p[i1 + by11];
  b101 = p[j1 + by01];
  b111 = p[j1 + by11];

  b002 = p[i2 + by02];
  b012 = p[i2 + by12];
  b102 = p[j2 + by02];
  b112 = p[j2 + by12];

  b003 = p[i3 + by03];
  b013 = p[i3 + by13];
  b103 = p[j3 + by03];
  b113 = p[j3 + by13];


  // may be faster as another mm_set_ps with -1 at the end of each term
  tmp = _mm_set_ps1(1.0);

  //!! try using FAST floattoint ...???
  rx0 = _mm_set_ps(tx0 - itx0, tx1 - itx1, tx2 - itx2, tx3 - itx3);
  rx1 = _mm_sub_ps(rx0, tmp);

  ry0 = _mm_set_ps(ty0 - ity0, ty1 - ity1, ty2 - ity2, ty3 - ity3);
  ry1 = _mm_sub_ps(ry0, tmp);


  rz0 = _mm_set_ps(tz0 - itz0, tz1 - itz1, tz2 - itz2, tz3 - itz3);
  rz1 = _mm_sub_ps(rz0, tmp);


  _mm_prefetch((char *)&(g3[b000 + bz00]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g3[b001 + bz01]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g3[b002 + bz02]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g3[b003 + bz03]), _MM_HINT_NTA);


  //	sx =  rx0 * rx0 * (3.0 - 2.0 * rx0);
  two = _mm_set_ps1(2.0); // Might be good to PRESTORE the constant arrays 1.0, 2.0 etc
  sx = _mm_mul_ps(rx0, two);
  three = _mm_set_ps1(3.0);
  tmp2 = _mm_sub_ps(three, sx);
  tmp = _mm_mul_ps(tmp2, rx0);
  sx = _mm_mul_ps(tmp, rx0);

  _mm_prefetch((char *)&(g3[b100 + bz00]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g3[b101 + bz01]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g3[b102 + bz02]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g3[b103 + bz03]), _MM_HINT_NTA);


  // sy =  ry0 * ry0 * (3.0 - 2.0 * ry0);
  // tmp  = _mm_set_ps1 (2.0)  ;
  sy = _mm_mul_ps(ry0, two);

  // tmp  = _mm_set_ps1 (3.0);
  tmp2 = _mm_sub_ps(three, sy);
  tmp = _mm_mul_ps(tmp2, ry0);
  sy = _mm_mul_ps(tmp, ry0);

  _mm_prefetch((char *)&(g3[b010 + bz00]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g3[b011 + bz01]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g3[b012 + bz02]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g3[b013 + bz03]), _MM_HINT_NTA);

  // sz =  rz0 * rz0 * (3.0 - 2.0 * rz0);
  // tmp  = _mm_set_ps1 (2.0)  ;
  sz = _mm_mul_ps(rz0, two);
  // tmp  = _mm_set_ps1 (3.0);
  tmp2 = _mm_sub_ps(three, sz);
  tmp = _mm_mul_ps(tmp2, rz0);
  sz = _mm_mul_ps(tmp, rz0);

  _mm_prefetch((char *)&(g3[b110 + bz00]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g3[b111 + bz01]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g3[b112 + bz02]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g3[b113 + bz03]), _MM_HINT_NTA);


  //	q = g3[ b00 ];		/* get random gradient */
  //	u = rx0 * q[0] + ry0 * q[1] + rz0 * q[2];
  q0 = _mm_set_ps(g3[b000 + bz00][0], g3[b001 + bz01][0], g3[b002 + bz02][0], g3[b003 + bz03][0]);
  q1 = _mm_set_ps(g3[b000 + bz00][1], g3[b001 + bz01][1], g3[b002 + bz02][1], g3[b003 + bz03][1]);
  q2 = _mm_set_ps(g3[b000 + bz00][2], g3[b001 + bz01][2], g3[b002 + bz02][2], g3[b003 + bz03][2]);

  tmp3 = _mm_mul_ps(q2, rz0);
  tmp = _mm_mul_ps(q1, ry0);
  tmp2 = _mm_mul_ps(q0, rx0);
  tmp4 = _mm_add_ps(tmp, tmp2);
  u = _mm_add_ps(tmp4, tmp3);


  _mm_prefetch((char *)&(g3[b000 + bz10]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g3[b001 + bz11]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g3[b002 + bz12]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g3[b003 + bz13]), _MM_HINT_NTA);
  //	q = g3[ b10 + bz0 ];
  //	v = rx1 * q[0] + ry0 * q[1] ;
  q0 = _mm_set_ps(g3[b100 + bz00][0], g3[b101 + bz01][0], g3[b102 + bz02][0], g3[b103 + bz03][0]);
  q1 = _mm_set_ps(g3[b100 + bz00][1], g3[b101 + bz01][1], g3[b102 + bz02][1], g3[b103 + bz03][1]);
  q2 = _mm_set_ps(g3[b100 + bz00][2], g3[b101 + bz01][2], g3[b102 + bz02][2], g3[b103 + bz03][2]);

  tmp3 = _mm_mul_ps(q2, rz0);
  tmp = _mm_mul_ps(q1, ry0);
  tmp2 = _mm_mul_ps(q0, rx1);
  tmp4 = _mm_add_ps(tmp, tmp2);
  v = _mm_add_ps(tmp4, tmp3);


  //    a = u + sx * (v-u);  /* get value at distance sx between u & v */
  tmp = _mm_sub_ps(v, u);
  tmp2 = _mm_mul_ps(tmp, sx);
  a = _mm_add_ps(u, tmp2);


  _mm_prefetch((char *)&(g3[b100 + bz10]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g3[b101 + bz11]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g3[b102 + bz12]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g3[b103 + bz13]), _MM_HINT_NTA);
  //	q = g3[ b01 ];
  //	u = rx0*q[0] + ry1 * q[1];
  q0 = _mm_set_ps(g3[b010 + bz00][0], g3[b011 + bz01][0], g3[b012 + bz02][0], g3[b013 + bz03][0]);
  q1 = _mm_set_ps(g3[b010 + bz00][1], g3[b011 + bz01][1], g3[b012 + bz02][1], g3[b013 + bz03][1]);
  q2 = _mm_set_ps(g3[b010 + bz00][2], g3[b011 + bz01][2], g3[b012 + bz02][2], g3[b013 + bz03][2]);

  tmp3 = _mm_mul_ps(q2, rz0);
  tmp = _mm_mul_ps(q1, ry1);
  tmp2 = _mm_mul_ps(q0, rx0);
  tmp4 = _mm_add_ps(tmp, tmp2);
  u = _mm_add_ps(tmp4, tmp3);


  _mm_prefetch((char *)&(g3[b010 + bz10]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g3[b011 + bz11]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g3[b012 + bz12]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g3[b013 + bz13]), _MM_HINT_NTA);
  //	q = g3[ b11 ];
  //	v = rx1 * q[0] + ry1 * q[1];
  q0 = _mm_set_ps(g3[b110 + bz00][0], g3[b111 + bz01][0], g3[b112 + bz02][0], g3[b113 + bz03][0]);
  q1 = _mm_set_ps(g3[b110 + bz00][1], g3[b111 + bz01][1], g3[b112 + bz02][1], g3[b113 + bz03][1]);
  q2 = _mm_set_ps(g3[b110 + bz00][2], g3[b111 + bz01][2], g3[b112 + bz02][2], g3[b113 + bz03][2]);

  tmp3 = _mm_mul_ps(q2, rz0);
  tmp = _mm_mul_ps(q1, ry1);
  tmp2 = _mm_mul_ps(q0, rx1);
  tmp4 = _mm_add_ps(tmp, tmp2);
  v = _mm_add_ps(tmp4, tmp3);


  //	b = u + sx * (v-u);
  tmp = _mm_sub_ps(v, u);
  tmp2 = _mm_mul_ps(tmp, sx);
  b = _mm_add_ps(u, tmp2);


  //	c = a + sy * (a-b);
  tmp = _mm_sub_ps(b, a);
  tmp2 = _mm_mul_ps(tmp, sy);
  c = _mm_add_ps(a, tmp2);

  /*NEW */

  _mm_prefetch((char *)&(g3[b110 + bz10]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g3[b111 + bz11]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g3[b112 + bz12]), _MM_HINT_NTA);
  _mm_prefetch((char *)&(g3[b113 + bz13]), _MM_HINT_NTA);

  //	q = g3[ b00 + bz1 ];
  //	u = rx0*q[0] + ry1 * q[1];
  q0 = _mm_set_ps(g3[b000 + bz10][0], g3[b001 + bz11][0], g3[b002 + bz12][0], g3[b003 + bz13][0]);
  q1 = _mm_set_ps(g3[b000 + bz10][1], g3[b001 + bz11][1], g3[b002 + bz12][1], g3[b003 + bz13][1]);
  q2 = _mm_set_ps(g3[b000 + bz10][2], g3[b001 + bz11][2], g3[b002 + bz12][2], g3[b003 + bz13][2]);

  tmp3 = _mm_mul_ps(q2, rz1);
  tmp = _mm_mul_ps(q1, ry0);
  tmp2 = _mm_mul_ps(q0, rx0);
  tmp4 = _mm_add_ps(tmp, tmp2);
  u = _mm_add_ps(tmp4, tmp3);


  //	q = g3[ b10 + bz1 ];
  //	v = rx1 * q[0] + ry1 * q[1];
  q0 = _mm_set_ps(g3[b100 + bz10][0], g3[b101 + bz11][0], g3[b102 + bz12][0], g3[b103 + bz13][0]);
  q1 = _mm_set_ps(g3[b100 + bz10][1], g3[b101 + bz11][1], g3[b102 + bz12][1], g3[b103 + bz13][1]);
  q2 = _mm_set_ps(g3[b100 + bz10][2], g3[b101 + bz11][2], g3[b102 + bz12][2], g3[b103 + bz13][2]);

  tmp3 = _mm_mul_ps(q2, rz1);
  tmp = _mm_mul_ps(q1, ry0);
  tmp2 = _mm_mul_ps(q0, rx1);
  tmp4 = _mm_add_ps(tmp, tmp2);
  v = _mm_add_ps(tmp4, tmp3);

  //	a = u + sx * (v-u);
  tmp = _mm_sub_ps(v, u);
  tmp2 = _mm_mul_ps(tmp, sx);
  a = _mm_add_ps(u, tmp2);


  //	q = g3[ b01 + bz1 ];
  //	v = rx1 * q[0] + ry1 * q[1];
  q0 = _mm_set_ps(g3[b010 + bz10][0], g3[b011 + bz11][0], g3[b012 + bz12][0], g3[b013 + bz13][0]);
  q1 = _mm_set_ps(g3[b010 + bz10][1], g3[b011 + bz11][1], g3[b012 + bz12][1], g3[b013 + bz13][1]);
  q2 = _mm_set_ps(g3[b010 + bz10][2], g3[b011 + bz11][2], g3[b012 + bz12][2], g3[b013 + bz13][2]);

  tmp3 = _mm_mul_ps(q2, rz1);
  tmp = _mm_mul_ps(q1, ry1);
  tmp2 = _mm_mul_ps(q0, rx0);
  tmp4 = _mm_add_ps(tmp, tmp2);
  u = _mm_add_ps(tmp4, tmp3);

  q0 = _mm_set_ps(g3[b110 + bz10][0], g3[b111 + bz11][0], g3[b112 + bz12][0], g3[b113 + bz13][0]);
  q1 = _mm_set_ps(g3[b110 + bz10][1], g3[b111 + bz11][1], g3[b112 + bz12][1], g3[b113 + bz13][1]);
  q2 = _mm_set_ps(g3[b110 + bz10][2], g3[b111 + bz11][2], g3[b112 + bz12][2], g3[b113 + bz13][2]);

  tmp3 = _mm_mul_ps(q2, rz1);
  tmp = _mm_mul_ps(q1, ry1);
  tmp2 = _mm_mul_ps(q0, rx1);
  tmp4 = _mm_add_ps(tmp, tmp2);
  v = _mm_add_ps(tmp4, tmp3);

  tmp = _mm_sub_ps(v, u);
  tmp2 = _mm_mul_ps(tmp, sx);
  b = _mm_add_ps(u, tmp2);

  tmp = _mm_sub_ps(b, a);
  tmp2 = _mm_mul_ps(tmp, sy);
  d = _mm_add_ps(a, tmp2);


  //	result = 1.5 * (c + sz * (d-c))	/* interpolate in y */   why 1.5 ??
  tmp = _mm_sub_ps(d, c);
  tmp2 = _mm_mul_ps(tmp, sz);
  tmp = _mm_add_ps(tmp2, c);
  tmp2 = _mm_set_ps1(1.5);
  a = _mm_mul_ps(tmp2, tmp);

  _mm_store_ps(result, a); // aligned
}

#include "noise_tab.c"

void PInitNoise()
{
  int i;

  // set up our two random number lookup tables
  // one for 2d noise the other for 3d...

  for (i = 0; i < B + B + 2; ++i)
  {
    p[i] = p_precomputed[i];
    g3[i][0] = g2[i][0] = g_precomputed[i][0];
    g3[i][1] = g2[i][1] = g_precomputed[i][1];
    g3[i][2] = g_precomputed[i][2];
  }
}
