#ifndef VIEWPORT_ELLIPSE_HLSL
#define VIEWPORT_ELLIPSE_HLSL 1

// r2 <= 1 is inside the ellipse
float viewport_ellipse_r2(float2 uv, float2 minus_center, float y_scale, float2 x_axis, float2 y_axis)
{
  float2 dr = float2(uv.x + minus_center.x, uv.y * y_scale + minus_center.y);
  float2 xy = float2(dot(dr, x_axis), dot(dr, y_axis));
  return dot(xy, xy);
}

#endif
