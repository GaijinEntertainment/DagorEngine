//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <render/lights/lightsBase.h>

class LightsEncoder
{
public:
  enum
  {
    SPOT_LIGHT_FLAG = (1 << 30),
    INVALID_LIGHT = 0xFFFFFFFF & (~SPOT_LIGHT_FLAG)
  };

  struct DecodedLightId
  {
    LightType type;
    uint32_t id;
  };

  __forceinline static DecodedLightId decodeLightId(uint32_t id)
  {
    if (id == INVALID_LIGHT)
      return {LightType::Invalid, uint32_t(INVALID_LIGHT)};

    if (id & SPOT_LIGHT_FLAG)
    {
      id &= ~SPOT_LIGHT_FLAG;
      return {LightType::Spot, id};
    }
    else
      return {LightType::Omni, id};
  }
  __forceinline static uint32_t encodeLightId(LightType type, uint32_t id)
  {
    if (type == LightType::Spot)
      return id | SPOT_LIGHT_FLAG;
    return id;
  };
};
