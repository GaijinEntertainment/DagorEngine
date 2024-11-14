#ifndef SMOOTHER_STEP_HLSL
#define SMOOTHER_STEP_HLSL 1
//https://en.wikipedia.org/wiki/Smoothstep
float smootherstep3(float x)//3rd order
{
  float x2 = x*x;
  return saturate(x2*x2 * (x*(x*(70 - 20*x) - 84) + 35));
}

float smootherstep(float x)
{
  // Evaluate polynomial
  return saturate(x * x * x * (x * (x * 6 - 15) + 10));
}

float smootherstep(float edge0, float edge1, float x)
{
  // Scale, and clamp x to 0..1 range
  x = saturate((x - edge0) / (edge1 - edge0));
  // Evaluate polynomial
  return smootherstep(x);
}

float2 smootherstep3(float2 x)//3rd order
{
  float2 x2 = x*x;
  return saturate(x2*x2 * (x*(x*(70 - 20*x) - 84) + 35));
}

float2 smootherstep(float2 x)
{
  // Evaluate polynomial
  return saturate(x * x * x * (x * (x * 6 - 15) + 10));
}

float2 smootherstep(float2 edge0, float2 edge1, float2 x)
{
  // Scale, and clamp x to 0..1 range
  x = saturate((x - edge0) / (edge1 - edge0));
  // Evaluate polynomial
  return smootherstep(x);
}

float4 smootherstep3(float4 x)//3rd order
{
  float4 x2 = x*x;
  return saturate(x2*x2 * (x*(x*(70 - 20*x) - 84) + 35));
}

float4 smootherstep(float4 x)
{
  // Evaluate polynomial
  return saturate(x * x * x * (x * (x * 6 - 15) + 10));
}

float4 smootherstep(float4 edge0, float4 edge1, float4 x)
{
  // Scale, and clamp x to 0..1 range
  x = saturate((x - edge0) / (edge1 - edge0));
  // Evaluate polynomial
  return smootherstep(x);
}
#endif