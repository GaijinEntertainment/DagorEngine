//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

//
//      Misc. noise functions from Texturing and Modeling A Procedural Approach
//  Perlin, Musgrave...
//
namespace perlin_noise
{
void init_noise(int seed);
float bias(float a, float b);
float gain(float a, float b);

float noise1(float arg);
float noise2(float vec[]);
float noise3(float vec[]);
float noise4(float x, float y, float z, float w);

float turbulence3(float *v, float freq);
int Perm(int v);

// #define MAX_OCTAVES     50

float fBm3(float point[], float H, float lacunarity, float octaves);
float fBm2(float point[], float H, float lacunarity, float octaves);
float fBm1(float point, float H, float lacunarity, float octaves);

template <typename Point2>
static inline float noise2(const Point2 &vec)
{
  return noise2((float *)&vec.x);
}
template <typename Point3>
static inline float noise3(const Point3 &vec)
{
  return noise3((float *)&vec.x);
}
template <typename Point3>
static inline float fBm3(const Point3 &p3, float h, float l, float o)
{
  return fBm3((float *)&p3.x, h, l, o);
}
template <typename Point3>
inline Point3 fractal_noise(const Point3 &p, float scale, float rough, float iterations, float time)
{
  Point3 d, sp;
  sp = p * scale + Point3(0.5f, 0.5f, 0.5f);
  // if (iterations > 0)
  //{
  d.x = perlin_noise::fBm3(Point3(sp.y, sp.z, time), rough, 2.0f, iterations);
  d.y = perlin_noise::fBm3(Point3(sp.x, sp.z, time), rough, 2.0f, iterations);
  d.z = perlin_noise::fBm3(Point3(sp.x, sp.y, time), rough, 2.0f, iterations);
  //} else
  //{
  //  d.x = perlin_noise::noise3(Point3(sp.y,sp.z,time));
  //  d.y = perlin_noise::noise3(Point3(sp.x,sp.z,time));
  //  d.z = perlin_noise::noise3(Point3(sp.x,sp.y,time));
  //}
  return d;
}
}; // namespace perlin_noise
