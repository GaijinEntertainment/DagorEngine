#ifndef STOCHASTIC_ROUNDING_INCLUDED
#define STOCHASTIC_ROUNDING_INCLUDED 1

// unbiased for any store rounding; denormals truncate
float3 round_mantissa_stochastic(float3 v, uint3 mantissa_bits, uint3 dither16)
{
  uint3 dropMask = (1u << (23 - mantissa_bits)) - 1;
  return asfloat((asuint(max(v, float3(0, 0, 0))) + ((dither16 << 16) >> (9 + mantissa_bits))) & ~dropMask);
}

float3 round_r11g11b10_stochastic(float3 rgb, uint3 dither16) { return round_mantissa_stochastic(rgb, uint3(6, 6, 5), dither16); }

#endif
