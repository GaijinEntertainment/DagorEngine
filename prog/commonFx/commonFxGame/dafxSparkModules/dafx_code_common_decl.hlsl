struct LinearDistribution
{
  float start;
  float end;
};

struct LinearDistributionBiased
{
  float start;
  float end;
  float bias;
};

struct RectangleDistribution
{
  float2 leftTop;
  float2 rightBottom;
};

struct CubeDistribution
{
  float3 center;
  float3 size;
};

struct SphereDistribution
{
  float3 center;
  float radius;
};

struct VectorDistribution
{
  float startAzimuth;
  float endAzimuth;
  float startZenith;
  float endZenith;
};

struct VectorBasis
{
  float3 direction;
  float3 scale;
  float startLength;
  float endLength;
};

// Deprecated
struct SectorDistribution
{
  float3 center;
  float3 scale;
  float radiusMin;
  float radiusMax;
  // 0..2PI
  float azimutMax;
  float azimutNoise;
  // 0..PI
  float inclinationMin;
  float inclinationMax;
  float inclinationNoise;
};

#ifndef __cplusplus

LinearDistribution LinearDistribution_load(BufferData_ref buf, in out uint ofs)
{
  LinearDistribution o;
  o.start = dafx_load_1f(buf, ofs);
  o.end = dafx_load_1f(buf, ofs);
  return o;
}

CubeDistribution CubeDistribution_load(BufferData_ref buf, in out uint ofs)
{
  CubeDistribution o;
  o.center = dafx_load_3f(buf, ofs);
  o.size = dafx_load_3f(buf, ofs);
  return o;
}

SphereDistribution SphereDistribution_load(BufferData_ref buf, in out uint ofs)
{
  SphereDistribution o;
  o.center = dafx_load_3f(buf, ofs);
  o.radius = dafx_load_1f(buf, ofs);
  return o;
}

SectorDistribution SectorDistribution_load(BufferData_ref buf, in out uint ofs)
{
  SectorDistribution o;
  o.center = dafx_load_3f(buf, ofs);
  o.scale = dafx_load_3f(buf, ofs);
  o.radiusMin = dafx_load_1f(buf, ofs);
  o.radiusMax = dafx_load_1f(buf, ofs);
  o.azimutMax = dafx_load_1f(buf, ofs);
  o.azimutNoise = dafx_load_1f(buf, ofs);
  o.inclinationMin = dafx_load_1f(buf, ofs);
  o.inclinationMax = dafx_load_1f(buf, ofs);
  o.inclinationNoise = dafx_load_1f(buf, ofs);
  return o;
}

VectorDistribution VectorDistribution_load(BufferData_ref buf, in out uint ofs)
{
  VectorDistribution o;
  o.startAzimuth = dafx_load_1f(buf, ofs);
  o.endAzimuth = dafx_load_1f(buf, ofs);
  o.startZenith = dafx_load_1f(buf, ofs);
  o.endZenith = dafx_load_1f(buf, ofs);
  return o;
}

#endif