#ifndef LOTTES_S_CURVE_HLSL_INCLUDE
#define LOTTES_S_CURVE_HLSL_INCLUDE 1

//http://gpuopen.com/wp-content/uploads/2016/03/GdcVdrLottes.pdf

  float3 LottesSCurve_optimized( float3 hdrColor, float contrast, float shoulder, float b, float c, float hdr_max)
  {
    //float3 x = min(hdrColor, hdr_max); not sure if we need to clamp!
    float3 x = hdrColor;
    float3 z = pow(x, contrast);
    return z / (pow(z, shoulder) * b + c);
  }
  //contrast = 1.5, shoulder=0.97. mid_in = 0.3, mid_out = 0.18, hdr_max = 4
  float3 LottesSCurve( float3 hdrColor, float contrast, float shoulder, float mid_in, float mid_out, float hdr_max)
  {
    float a = contrast;// contrast
    float d = shoulder;// shoulder

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
    //-constants

    return LottesSCurve_optimized( hdrColor, contrast, shoulder, b, c, hdr_max);
  }
#endif