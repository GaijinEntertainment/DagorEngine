// Copyright (C) Gaijin Games KFT.  All rights reserved.

//
// From "Texturing and Modeling A Procedural Approach"
//
// Chapter 6 by Ken Perlin
//

#include <math/dag_mathBase.h>
// #include <math.h>
#include <math/random/dag_random.h>
#include <math/dag_noise.h>

#define MAX_OCTAVES 50

namespace perlin_noise
{

float bias(float a, float b) { return pow((double)a, log((double)b) / log(0.5)); }

float gain(float a, float b)
{
  float p = log(1. - b) / log(0.5);

  if (a < .001)
    return 0.;
  else if (a > .999)
    return 1.;
  if (a < 0.5)
    return pow(2 * a, p) / 2;
  else
    return 1. - pow(2.0 * (1. - a), (double)p) / 2;
}

static inline float noise(float vec[], int len)
{
  switch (len)
  {
    case 0: return 0.;
    case 1: return noise1(vec[0]);
    case 2: return noise2(vec);
    default: return noise3(vec);
  }
}

float turbulence3(float *v, float freq)
{
  float t, vec[3];

  for (t = 0.; freq >= 1.; freq /= 2) //-V1034
  {
    vec[0] = freq * v[0];
    vec[1] = freq * v[1];
    vec[2] = freq * v[2];
    t += fabsf(noise3(vec)) / freq;
  }
  return t;
}

/* noise functions over 1, 2, and 3 dimensions */

#define B  0x100
#define BM (B - 1)

#define N  0x1000
#define NP 12 /* 2^N */
#define NM (N - 1)

static int perlin_p[B + B + 2];
static float perlin_g3[B + B + 2][3];
static float perlin_g2[B + B + 2][2];
static float perlin_g1[B + B + 2];

int Perm(int v) { return perlin_p[v & BM]; }

#define s_curve(t) ((t) * (t) * (3. - 2. * (t)))

#define lerp(t, a, b) ((a) + (t) * ((b) - (a)))

#define setup(i, b0, b1, r0, r1) \
  t = vec[i];                    \
  it = (int)floor(t);            \
  b0 = it & BM;                  \
  b1 = (b0 + 1) & BM;            \
  r0 = t - it;                   \
  r1 = r0 - 1.;

float noise1(float arg)
{
  int bx0, bx1, it;
  float rx0, rx1, sx, t, u, v, vec[1];

  vec[0] = arg;

  setup(0, bx0, bx1, rx0, rx1);

  sx = s_curve(rx0);

  u = rx0 * perlin_g1[perlin_p[bx0]];
  v = rx1 * perlin_g1[perlin_p[bx1]];

  return lerp(sx, u, v);
}

float noise2(float vec[2])
{
  int bx0, bx1, by0, by1, b00, b10, b01, b11, it;
  float rx0, rx1, ry0, ry1, *q, sx, sy, a, b, t, u, v;
  int i, j;

  setup(0, bx0, bx1, rx0, rx1);
  setup(1, by0, by1, ry0, ry1);

  i = perlin_p[bx0];
  j = perlin_p[bx1];

  b00 = perlin_p[i + by0];
  b10 = perlin_p[j + by0];
  b01 = perlin_p[i + by1];
  b11 = perlin_p[j + by1];

  sx = s_curve(rx0);
  sy = s_curve(ry0);

#define at2(rx, ry) ((rx)*q[0] + (ry)*q[1])

  q = perlin_g2[b00];
  u = at2(rx0, ry0);
  q = perlin_g2[b10];
  v = at2(rx1, ry0);
  a = lerp(sx, u, v);

  q = perlin_g2[b01];
  u = at2(rx0, ry1);
  q = perlin_g2[b11];
  v = at2(rx1, ry1);
  b = lerp(sx, u, v);

  return lerp(sy, a, b);
}

float noise3(float vec[3])
{
  int bx0, bx1, by0, by1, bz0, bz1, b00, b10, b01, b11, it;
  float rx0, rx1, ry0, ry1, rz0, rz1, *q, sy, sz, a, b, c, d, t, u, v;
  int i, j;

  setup(0, bx0, bx1, rx0, rx1);
  setup(1, by0, by1, ry0, ry1);
  setup(2, bz0, bz1, rz0, rz1);

  i = perlin_p[bx0];
  j = perlin_p[bx1];

  b00 = perlin_p[i + by0];
  b10 = perlin_p[j + by0];
  b01 = perlin_p[i + by1];
  b11 = perlin_p[j + by1];

  t = s_curve(rx0);
  sy = s_curve(ry0);
  sz = s_curve(rz0);

#define at3(rx, ry, rz) ((rx)*q[0] + (ry)*q[1] + (rz)*q[2])

  q = perlin_g3[b00 + bz0];
  u = at3(rx0, ry0, rz0);
  q = perlin_g3[b10 + bz0];
  v = at3(rx1, ry0, rz0);
  a = lerp(t, u, v);

  q = perlin_g3[b01 + bz0];
  u = at3(rx0, ry1, rz0);
  q = perlin_g3[b11 + bz0];
  v = at3(rx1, ry1, rz0);
  b = lerp(t, u, v);

  c = lerp(sy, a, b);

  q = perlin_g3[b00 + bz1];
  u = at3(rx0, ry0, rz1);
  q = perlin_g3[b10 + bz1];
  v = at3(rx1, ry0, rz1);
  a = lerp(t, u, v);

  q = perlin_g3[b01 + bz1];
  u = at3(rx0, ry1, rz1);
  q = perlin_g3[b11 + bz1];
  v = at3(rx1, ry1, rz1);
  b = lerp(t, u, v);

  d = lerp(sy, a, b);

  return lerp(sz, c, d);
}

static void normalize2(float v[2])
{
  float s;

  s = sqrt(v[0] * v[0] + v[1] * v[1]);
  if (s > 0.f)
  {
    v[0] = v[0] / s;
    v[1] = v[1] / s;
  }
  else
  {
    v[0] = 1.f;
    v[1] = 0.f;
  }
}

static void normalize3(float v[3])
{
  float s;

  s = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  if (s > 0.f)
  {
    v[0] = v[0] / s;
    v[1] = v[1] / s;
    v[2] = v[2] / s;
  }
  else
  {
    v[0] = 1.f;
    v[1] = 0.f;
    v[2] = 0.f;
  }
}

void init_noise(int perlin_seed)
{
#define random() _rnd(perlin_seed)
  int i, j, k;

  for (i = 0; i < B; i++)
  {
    perlin_p[i] = i;

    perlin_g1[i] = (float)((random() % (B + B)) - B) / B;

    for (j = 0; j < 2; j++)
      perlin_g2[i][j] = (float)((random() % (B + B)) - B) / B;
    normalize2(perlin_g2[i]);

    for (j = 0; j < 3; j++)
      perlin_g3[i][j] = (float)((random() % (B + B)) - B) / B;
    normalize3(perlin_g3[i]);
  }

  while (--i)
  {
    k = perlin_p[i];
    perlin_p[i] = perlin_p[j = random() % B];
    perlin_p[j] = k;
  }

  for (i = 0; i < B + 2; i++)
  {
    perlin_p[B + i] = perlin_p[i];
    perlin_g1[B + i] = perlin_g1[i];
    for (j = 0; j < 2; j++)
      perlin_g2[B + i][j] = perlin_g2[i][j];
    for (j = 0; j < 3; j++)
      perlin_g3[B + i][j] = perlin_g3[i][j];
  }
#undef random
}


/*
 * Procedural fBm evaluated at "point"; returns value stored in "value".
 *
 * Copyright 1994 F. Kenton Musgrave
 *
 * Parameters:
 *    ``H''  is the fractal increment parameter
 *    ``lacunarity''  is the gap between successive frequencies
 *    ``octaves''  is the number of frequencies in the fBm
 */

template <int dimensions>
static inline float fBm(float point[], float H, float lacunarity, float octaves)
{
  static float exponent_array[MAX_OCTAVES + 1];
  static float lastH = -100000.0f;
  float value, frequency, remainder;
  int i;

  if (octaves > MAX_OCTAVES)
    octaves = MAX_OCTAVES;

  /* precompute and store spectral weights */
  if (fabsf(H - lastH) > 0.00001f)
  {
    lastH = H;
    frequency = 1.0;
    for (i = 0; i <= octaves; i++)
    {
      /* compute weight for each frequency */
      exponent_array[i] = powf(frequency, -H);
      frequency *= lacunarity;
    }
  }

  value = 0.0; /* initialize vars to proper values */
  frequency = 1.0;

  /* inner loop of spectral construction */
  for (i = 0; i < octaves; i++)
  {
    value += noise(point, dimensions) * exponent_array[i];
    for (int v = 0; v < dimensions; v++)
      point[v] *= lacunarity;
  } /* for */

  remainder = octaves - (int)octaves;
  if (remainder > 0.0f) /* add in ``octaves''  remainder */
    value += remainder * noise(point, dimensions) * exponent_array[i];

  return (value);
}

float fBm3(float point[], float H, float lacunarity, float octaves) { return fBm<3>(point, H, lacunarity, octaves); }

float fBm2(float point[], float H, float lacunarity, float octaves) { return fBm<2>(point, H, lacunarity, octaves); }

float fBm1(float point, float H, float lacunarity, float octaves)
{
  float pt[1];
  pt[0] = point;
  return fBm<1>(pt, H, lacunarity, octaves);
}


#undef B

static unsigned char permutation[512] = {151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69,
  142, 8, 99, 37, 240, 21, 10, 23, 190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33, 88,
  237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166, 77, 146, 158, 231, 83, 111, 229, 122, 60,
  211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244, 102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18,
  169, 200, 196, 135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123, 5, 202, 38, 147, 118,
  126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42, 223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70,
  221, 153, 101, 155, 167, 43, 172, 9, 129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228,
  251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239, 107, 49, 192, 214, 31, 181, 199, 106, 157,
  184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254, 138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215,
  61, 156, 180, 151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21,
  10, 23, 190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33, 88, 237, 149, 56, 87, 174,
  20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166, 77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220,
  105, 92, 41, 55, 46, 245, 40, 244, 102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196, 135,
  130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123, 5, 202, 38, 147, 118, 126, 255, 82, 85,
  212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42, 223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155,
  167, 43, 172, 9, 129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228, 251, 34, 242, 193,
  238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239, 107, 49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176,
  115, 121, 50, 45, 127, 4, 150, 254, 138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180};


static float fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }


static float grad(int hash, float x, float y, float z, float w)
{
  int h = hash & 31; // CONVERT LO 5 BITS OF HASH TO 32 GRAD DIRECTIONS.
  float a = y;       // X,Y,Z
  float b = z;
  float c = w;

  switch (h >> 3) // OR, DEPENDING ON HIGH ORDER 2 BITS:
  {
    case 1:
      a = w;
      b = x;
      c = y;
      break; // W,X,Y

    case 2:
      a = z;
      b = w;
      c = x;
      break; // Z,W,X

    case 3:
      a = y;
      b = z;
      c = w;
      break; // Y,Z,W
  }

  return ((h & 4) == 0 ? -a : a) + ((h & 2) == 0 ? -b : b) + ((h & 1) == 0 ? -c : c);
}


float noise4(float x, float y, float z, float w)
{
  int X = (int)floorf(x); // FIND UNIT HYPERCUBE
  int Y = (int)floorf(y); // THAT CONTAINS POINT.
  int Z = (int)floorf(z);
  int W = (int)floorf(w);
  x -= X; // FIND RELATIVE X,Y,Z,W
  y -= Y; // OF POINT IN CUBE.
  z -= Z;
  w -= W;
  X &= 255;
  Y &= 255;
  Z &= 255;
  W &= 255;

  float a = fade(x); // COMPUTE FADE CURVES
  float b = fade(y); // FOR EACH OF X,Y,Z,W.
  float c = fade(z);
  float d = fade(w);

  int A = permutation[X] + Y;      // HASH COORDINATES OF
  int AA = permutation[A] + Z;     // THE 16 CORNERS OF
  int AB = permutation[A + 1] + Z; // THE HYPERCUBE.
  int B = permutation[X + 1] + Y;
  int BA = permutation[B] + Z;
  int BB = permutation[B + 1] + Z;
  int AAA = permutation[AA] + W;
  int AAB = permutation[AA + 1] + W;
  int ABA = permutation[AB] + W;
  int ABB = permutation[AB + 1] + W;
  int BAA = permutation[BA] + W;
  int BAB = permutation[BA + 1] + W;
  int BBA = permutation[BB] + W;
  int BBB = permutation[BB + 1] + W;

  return lerp(d, // INTERPOLATE DOWN.
    lerp(c,
      lerp(b, lerp(a, grad(permutation[AAA], x, y, z, w), grad(permutation[BAA], x - 1, y, z, w)),
        lerp(a, grad(permutation[ABA], x, y - 1, z, w), grad(permutation[BBA], x - 1, y - 1, z, w))),

      lerp(b, lerp(a, grad(permutation[AAB], x, y, z - 1, w), grad(permutation[BAB], x - 1, y, z - 1, w)),
        lerp(a, grad(permutation[ABB], x, y - 1, z - 1, w), grad(permutation[BBB], x - 1, y - 1, z - 1, w)))),

    lerp(c,
      lerp(b, lerp(a, grad(permutation[AAA + 1], x, y, z, w - 1), grad(permutation[BAA + 1], x - 1, y, z, w - 1)),
        lerp(a, grad(permutation[ABA + 1], x, y - 1, z, w - 1), grad(permutation[BBA + 1], x - 1, y - 1, z, w - 1))),

      lerp(b, lerp(a, grad(permutation[AAB + 1], x, y, z - 1, w - 1), grad(permutation[BAB + 1], x - 1, y, z - 1, w - 1)),
        lerp(a, grad(permutation[ABB + 1], x, y - 1, z - 1, w - 1), grad(permutation[BBB + 1], x - 1, y - 1, z - 1, w - 1)))));
}
} // namespace perlin_noise
