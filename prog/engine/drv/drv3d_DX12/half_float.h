#pragma once

#include <cstring>
#include <math/dag_mathBase.h>


// We have our own half float converter here, as we may want to tweak it a bit in the future
namespace drv3d_dx12::half_float
{

inline constexpr uint32_t float_sign_mask = 0x80000000U;
inline constexpr uint32_t float_exponent_mask = 0x7F800000U;
inline constexpr uint32_t float_mantissa_mask = 0x7FFFFFU;
inline constexpr uint32_t float_mantiassa_size = 23;
inline constexpr int32_t float_mantiassa_bias = 127;
inline constexpr int32_t bias = 15;
inline constexpr int32_t exponent_size = 5;
inline constexpr int32_t mantissa_size = 10;
inline constexpr int32_t max_exponent = (1 << exponent_size) - 1;

inline float convert_to_float(uint16_t v)
{
  int32_t exponent = int32_t(((v & 0x7FFFU)) >> mantissa_size);
  bool isNan = exponent >= max_exponent;
  uint32_t exponentPart =
    exponent <= 0 ? 0U : (isNan ? float_exponent_mask : ((exponent - bias + float_mantiassa_bias) << float_mantiassa_size));
  uint32_t signPart = uint32_t(v & 0x8000U) << 16;
  uint32_t fractionPart = isNan ? float_mantissa_mask : (uint32_t(v) << (float_mantiassa_size - mantissa_size)) & float_mantissa_mask;
  uint32_t floatBits = signPart | exponentPart | fractionPart;
  float floatValue;
  memcpy(&floatValue, &floatBits, sizeof(float));
  return floatValue;
}

inline uint16_t convert_from_float(float v)
{
  uint32_t floatBits;
  memcpy(&floatBits, &v, sizeof(float));

  int32_t exponent = ((floatBits & float_exponent_mask) >> float_mantiassa_size) - float_mantiassa_bias + bias;
  uint32_t exponentPart = clamp(exponent, 0, max_exponent) << mantissa_size;
  uint32_t signPart = ((floatBits & float_sign_mask) >> 16);
  uint32_t fractionPart = exponent >= 0 ? ((floatBits & float_mantissa_mask) >> (float_mantiassa_size - mantissa_size)) : 0U;

  return signPart | exponentPart | fractionPart;
}

} // namespace drv3d_dx12::half_float
