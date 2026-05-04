//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>
#include <gameMath/quantization.h>
#include <gameMath/constructConvex.h>


namespace bind_dascript
{
inline uint32_t pack_unit_vec_uint(const Point3 &v, const uint32_t bits) { return gamemath::pack_unit_vec<uint32_t>(v, bits); }

inline Point3 unpack_unit_vec_uint(uint32_t val, const uint32_t bits) { return gamemath::unpack_unit_vec<uint32_t>(val, bits); }

inline uint8_t damage_packed_pack_unsigned(int bits, float value)
{
  return gamemath::pack_scalar_signed<uint8_t, float>(value, bits, 1.0, nullptr);
}
} // namespace bind_dascript