#ifndef LOTTES_S_CURVE_HLSL_INCLUDE
#define LOTTES_S_CURVE_HLSL_INCLUDE 1

//http://gpuopen.com/wp-content/uploads/2016/03/GdcVdrLottes.pdf

float2 calculateLottesSCoefficients(float contrast, float shoulder, float mid_in, float mid_out, float hdr_max)
{
  float a = contrast;
  float d = shoulder;

  //constants for all pixels, can be precomputed
  float ad = a * d;

  float midi_pow_a  = pow(mid_in, a);
  float midi_pow_ad = pow(mid_in, ad);
  float hdrm_pow_a  = pow(hdr_max, a);
  float hdrm_pow_ad = pow(hdr_max, ad);

  float u = hdrm_pow_ad * mid_out - midi_pow_ad * mid_out;
  float v = midi_pow_ad * mid_out;

  float b = -((-midi_pow_a + (mid_out * (hdrm_pow_ad * midi_pow_a - hdrm_pow_a * v)) / u) / v);
  float c = (hdrm_pow_ad * midi_pow_a - hdrm_pow_a * v) / u;

  return float2(b,c);
}

float3 LottesSCurveDirect( float3 hdrColor, float contrast, float shoulder, float mid_in, float mid_out, float hdr_max)
{
  float2 coeffs = calculateLottesSCoefficients(contrast, shoulder, mid_in, mid_out, hdr_max);

  float3 z = pow(hdrColor, contrast);
  return z / (pow(z, shoulder) * coeffs.x + coeffs.y);
}

// Maps [0, 1] -> [0, hdr_max]. No closed form for shoulder != 1: Halley on h(t) = t - log(b*e^(d*t) + c) - log(y),
// which is concave with slope in [1-d, 1]; Newton in linear z diverges. Start is the exact shoulder == 1 solution.
float3 LottesSCurveDirectInverse( float3 ldrColor, float contrast, float shoulder, float mid_in, float mid_out, float hdr_max)
{
  float2 coeffs = calculateLottesSCoefficients(contrast, shoulder, mid_in, mid_out, hdr_max);
  float b = coeffs.x, c = coeffs.y, d = shoulder;

  float3 y = clamp(ldrColor, 1e-6, 1);
  float3 logY = log(y);
  float3 t = min(logY + log(c) - log(max(1 - b * y, 1e-6)), contrast * log(hdr_max));
  UNROLL
  for (int i = 0; i < 3; ++i)
  {
    float3 bw = b * exp(d * t);
    float3 s = bw / (bw + c);
    float3 h = t - log(bw + c) - logY;
    float3 dh = 1 - d * s;
    float3 ddh = -d * d * s * (1 - s);
    t -= h / (dh - 0.5 * h * ddh / dh);
  }
  return exp(t / contrast);
}
// TODO: use this in the future
float3 LottesSCurveLuma( float3 hdrColor, float contrast, float shoulder, float mid_in, float mid_out, float hdr_max)
{
  float2 coeffs = calculateLottesSCoefficients(contrast, shoulder, mid_in, mid_out, hdr_max);

  float Lin = luminance(hdrColor);
  float z = pow(Lin, contrast);
  float3 colorResidual = hdrColor / max(Lin, 1e-9);
  float mappedLuminance = z / (pow(z, shoulder) * coeffs.x + coeffs.y);
  return colorResidual * mappedLuminance;
}
#endif