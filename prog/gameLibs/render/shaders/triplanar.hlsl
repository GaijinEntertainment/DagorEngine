#ifndef TRIPLANAR_HLSL
#define TRIPLANAR_HLSL

struct TriplanarCoords
{
  float3 pos;
  float3 dpdx, dpdy;
  float lod;
  half3 weights;
  half3 axisSign;
};

TriplanarCoords make_triplanar_coords(float3 pos, half3 n, half sharpness)
{
  TriplanarCoords coords = (TriplanarCoords)0;
  coords.pos = pos;
  coords.axisSign = select(n >= 0, half3(1, 1, 1), half3(-1, -1, -1));
  half3 a = abs(n);
  half m = max3(a.x, a.y, a.z);
  half3 w = smoothstep(min(sharpness, 0.98h) * m, m, a);
  coords.weights = w / dot(w, 1);
  return coords;
}

void set_triplanar_derivatives(inout TriplanarCoords coords, float3 dpdx, float3 dpdy)
{
  coords.dpdx = dpdx;
  coords.dpdy = dpdy;
  coords.lod = 0.5 * log2(max(dot(dpdx, dpdx), dot(dpdy, dpdy)));
}

struct PlanarProjection
{
  float2 uv;
  float2 dx, dy;
  float lod;
  half weight;
  half3 tangent, bitangent;
};

float2 project_on_plane(float3 v, bool isX, bool isY, half uSign)
{
  return float2((isX ? v.z : v.x) * uSign, isY ? v.z : v.y);
}

PlanarProjection get_planar_projection(TriplanarCoords coords, uint axis, half weight)
{
  bool isX = axis == 0;
  bool isY = axis == 1;
  half uSign = isX ? coords.axisSign.x : (isY ? coords.axisSign.y : coords.axisSign.z);
  PlanarProjection proj;
  proj.uv = project_on_plane(coords.pos, isX, isY, uSign);
  proj.dx = project_on_plane(coords.dpdx, isX, isY, uSign);
  proj.dy = project_on_plane(coords.dpdy, isX, isY, uSign);
  proj.lod = coords.lod;
  proj.weight = weight;
  proj.tangent = half3(isX ? 0 : uSign, 0, isX ? uSign : 0);
  proj.bitangent = half3(0, isY ? 0 : 1, isY ? 1 : 0);
  return proj;
}

uint dominant_triplanar_axis(half3 w)
{
  return w.x >= max(w.y, w.z) ? 0 : (w.y >= w.z ? 1 : 2);
}

struct TriplanarOrder
{
  uint3 axis;
  half3 weight;
};

TriplanarOrder sort_triplanar_axes(half3 w)
{
  TriplanarOrder sorted;
  sorted.weight = half3(max(w.x, max(w.y, w.z)), max(min(w.x, w.y), min(max(w.x, w.y), w.z)), min(w.x, min(w.y, w.z)));
  uint first = dominant_triplanar_axis(w);
  uint last = w.z <= min(w.x, w.y) ? 2 : (w.y <= w.x ? 1 : 0);
  sorted.axis = uint3(first, 3 - first - last, last);
  return sorted;
}

#endif
