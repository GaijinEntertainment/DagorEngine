#ifndef SSGI_BRIGHTNESS_LERP_INCLUDED
#define SSGI_BRIGHTNESS_LERP_INCLUDED 1

float lerpBrightnessValue(float3 oldV, float3 newV)//get lerp value based on difference between colors and and luminance. 0..1
{
  float lumMax = luminance(max(oldV, newV));
  return saturate(lumMax*8+0.125)*(1-pow2(1-max3(abs(oldV - newV)/(oldV + newV+1e-6))));
}

#endif