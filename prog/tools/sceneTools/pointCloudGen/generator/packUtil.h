// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_Point4.h>
#include <math/dag_Point3.h>
#include <math/dag_e3dColor.h>
#include <shaders/dag_shaderCommon.h>

namespace plod
{

inline Point4 unpackToPoint4(const E3DCOLOR &c, int mod = ChannelModifier::CMOD_NONE)
{
  G_ASSERT(mod == ChannelModifier::CMOD_UNSIGNED_PACK || mod == ChannelModifier::CMOD_NONE);
  if (mod == ChannelModifier::CMOD_NONE)
    return {c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a / 255.f};
  else if (mod == ChannelModifier::CMOD_UNSIGNED_PACK)
    return {c.r / 255.f * 2 - 1, c.g / 255.f * 2 - 1, c.b / 255.f * 2 - 1, c.a / 255.f * 2 - 1};
  return Point4::ZERO;
}

inline Point3 unpackToPoint3(const E3DCOLOR &c, int mod = ChannelModifier::CMOD_NONE)
{
  Point4 p = unpackToPoint4(c, mod);
  return {p.x, p.y, p.z};
}

inline E3DCOLOR packToE3DCOLOR(const Point4 &c, int mod = ChannelModifier::CMOD_NONE)
{
  const auto touint8 = [](float v) { return static_cast<uint8_t>(eastl::clamp(v, 0.f, 255.f)); };
  G_ASSERT(mod == ChannelModifier::CMOD_UNSIGNED_PACK || mod == ChannelModifier::CMOD_NONE);
  if (mod == ChannelModifier::CMOD_NONE)
    return {touint8(c.x * 255), touint8(c.y * 255), touint8(c.z * 255), touint8(c.w * 255)};
  else if (mod == ChannelModifier::CMOD_UNSIGNED_PACK)
    return {touint8((c.x + 1) / 2 * 255), touint8((c.y + 1) / 2 * 255), touint8((c.z + 1) / 2 * 255), touint8((c.w + 1) / 2 * 255)};
  return 0;
}

inline E3DCOLOR packToE3DCOLOR(const Point3 &c, int mod = ChannelModifier::CMOD_NONE)
{
  return packToE3DCOLOR({c.x, c.y, c.z, 0.0f}, mod);
}


} // namespace plod
