#ifndef TEX2D_BICUBIC_WEIGHTS
#define TEX2D_BICUBIC_WEIGHTS  1

float w0(float a)
{
  return (1.0/6.0)*(a*(a*(-a + 3.0) - 3.0) + 1.0);
}

float w1(float a)
{
  return (1.0/6.0)*(a*a*(3.0*a - 6.0) + 4.0);
}

float w2(float a)
{
  return (1.0/6.0)*(a*(a*(-3.0*a + 3.0) + 3.0) + 1.0);
}

float w3(float a)
{
  return (1.0/6.0)*(a*a*a);
}

// g0 and g1 are the two amplitude functions
float g0(float a)
{
  return w0(a) + w1(a);
}

float g1(float a)
{
  return w2(a) + w3(a);
}

// h0 and h1 are the two offset functions
float h0(float a)
{
  return -1.0 + w1(a) / (w0(a) + w1(a));
}

float h1(float a)
{
  return 1.0 + w3(a) / (w2(a) + w3(a));
}

struct BicubicSharpenWeights
{
  // Order of taps:
  // X   uv1  X
  // uv0 uv2 uv3
  // X   uv4  X

  float2 uv0, uv1, uv2, uv3, uv4;
  half w0,w1,w2,w3,w4;
  half weightsSum;
};

void compute_bicubic_sharpen_weights(
  float2 uv,
  float2 res,
  float2 resInv,
  float sharpness,      // [0..1], 0.5 is no change.
  out BicubicSharpenWeights weights)
{
  float2 texelCoords = uv*res;
  float2 texelCenter = floor(texelCoords - 0.5) + 0.5;
  float2 f = texelCoords - texelCenter;
  float2 f2 = f * f;
  float2 f3 = f2 * f;

  float2 w0 = -sharpness*f3 + (2.0*sharpness)*f2 - sharpness * f;
  float2 w1 =  (2.0 - sharpness)*f3 - (3.0 - sharpness)* f2 + 1.0;
  float2 w2 = -(2.0 - sharpness)*f3 + (3.0 - 2.0*sharpness)*f2 + sharpness*f;
  float2 w3 =  sharpness*f3 - sharpness*f2;

  float2 w12 = w1 + w2;
  float2 tc12 = (texelCenter + w2 / w12) * resInv;
  float2 tc0 = (texelCenter - 1.0) * resInv;
  float2 tc3 = (texelCenter + 2.0) * resInv;

  weights.uv0 = float2(tc12.x, tc0.y);
  weights.uv1 = float2(tc0.x, tc12.y);
  weights.uv2 = float2(tc12.x, tc12.y);
  weights.uv3 = float2(tc3.x, tc12.y);
  weights.uv4 = float2(tc12.x, tc3.y);

  weights.w0 = half(w12.x * w0.y);
  weights.w1 = half(w0.x  * w12.y);
  weights.w2 = half(w12.x * w12.y);
  weights.w3 = half(w3.x  * w12.y);
  weights.w4 = half(w12.x * w3.y);

  weights.weightsSum = weights.w0 + weights.w1 + weights.w2 + weights.w3 + weights.w4;
}
#endif
