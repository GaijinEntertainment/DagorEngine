// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "coherentNoise.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define BM 0xff

#define N  0x1000
#define NP 12 /* 2^N */
#define NM 0xfff

#define S_CURVE(t) ((t) * (t) * (3. - 2. * (t)))

#define LERP(t, a, b) ((a) + (t) * ((b) - (a)))

#define SETUP(i, b0, b1, r0, r1) \
  t = (i) + N;                   \
  b0 = (((int)t) & PM) & BM;     \
  b1 = (((b0) + 1) & PM) & BM;   \
  r0 = t - (int)t;               \
  r1 = (r0)-1.;

float PerlinNoise::perlin_noise3(float x, float y, float z, int PM)
{
  int bx0, bx1, by0, by1, bz0, bz1, b00, b10, b01, b11;
  float rx0, rx1, ry0, ry1, rz0, rz1, *q, sy, sz, a, b, c, d, t, u, v;
  int i, j;

  SETUP(x, bx0, bx1, rx0, rx1);
  SETUP(y, by0, by1, ry0, ry1);
  SETUP(z, bz0, bz1, rz0, rz1);

  i = p[bx0];
  j = p[bx1];

  b00 = p[i + by0];
  b10 = p[j + by0];
  b01 = p[i + by1];
  b11 = p[j + by1];

  t = S_CURVE(rx0);
  sy = S_CURVE(ry0);
  sz = S_CURVE(rz0);

#define AT3(rx, ry, rz) ((rx)*q[0] + (ry)*q[1] + (rz)*q[2])

  q = g3[b00 + bz0];
  u = AT3(rx0, ry0, rz0);
  q = g3[b10 + bz0];
  v = AT3(rx1, ry0, rz0);
  a = LERP(t, u, v);

  q = g3[b01 + bz0];
  u = AT3(rx0, ry1, rz0);
  q = g3[b11 + bz0];
  v = AT3(rx1, ry1, rz0);
  b = LERP(t, u, v);

  c = LERP(sy, a, b);

  q = g3[b00 + bz1];
  u = AT3(rx0, ry0, rz1);
  q = g3[b10 + bz1];
  v = AT3(rx1, ry0, rz1);
  a = LERP(t, u, v);

  q = g3[b01 + bz1];
  u = AT3(rx0, ry1, rz1);
  q = g3[b11 + bz1];
  v = AT3(rx1, ry1, rz1);
  b = LERP(t, u, v);

  d = LERP(sy, a, b);

  return LERP(sz, c, d);
}

static void normalize2(float v[2])
{
  float s;

  s = sqrt(v[0] * v[0] + v[1] * v[1]);
  v[0] = v[0] / s;
  v[1] = v[1] / s;
}

static void normalize3(float v[3])
{
  float s;

  s = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  v[0] = v[0] / s;
  v[1] = v[1] / s;
  v[2] = v[2] / s;
}

inline int the_random() { return rand(); }

void PerlinNoise::perlin_init(int seed)
{
  srand(seed);
  int i, j, k;

  for (i = 0; i < B; i++)
  {
    p[i] = i;

    g1[i] = (float)((the_random() % (B + B)) - B) / B;

    for (j = 0; j < 2; j++)
      g2[i][j] = (float)((the_random() % (B + B)) - B) / B;
    normalize2(g2[i]);

    for (j = 0; j < 3; j++)
      g3[i][j] = (float)((the_random() % (B + B)) - B) / B;
    normalize3(g3[i]);
  }

  while (--i)
  {
    k = p[i];
    p[i] = p[j = the_random() % B];
    p[j] = k;
  }

  for (i = 0; i < B + 2; i++)
  {
    p[B + i] = p[i];
    g1[B + i] = g1[i];
    for (j = 0; j < 2; j++)
      g2[B + i][j] = g2[i][j];
    for (j = 0; j < 3; j++)
      g3[B + i][j] = g3[i][j];
  }
}
