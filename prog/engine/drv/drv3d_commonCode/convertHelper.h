// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_resource.h>
#include <debug/dag_assert.h>

inline void resource_clear_value_to_float4(unsigned cflg, const ResourceClearValue &clear_value, float (&cv)[4])
{
  switch (get_tex_format_desc(cflg & TEXFMT_MASK).mainChannelsType)
  {
    case ChannelDType::UNORM:
    case ChannelDType::SNORM:
    case ChannelDType::UFLOAT:
    case ChannelDType::SFLOAT:
    {
      G_STATIC_ASSERT(sizeof(cv) == sizeof(clear_value.asFloat));
      memcpy(cv, clear_value.asFloat, sizeof(clear_value.asFloat));
      break;
    }
    case ChannelDType::UINT:
    {
      cv[0] = clear_value.asUint[0];
      cv[1] = clear_value.asUint[1];
      cv[2] = clear_value.asUint[2];
      cv[3] = clear_value.asUint[3];
      break;
    }
    case ChannelDType::SINT:
    {
      cv[0] = clear_value.asInt[0];
      cv[1] = clear_value.asInt[1];
      cv[2] = clear_value.asInt[2];
      cv[3] = clear_value.asInt[3];
      break;
    }
    default:
    {
      G_ASSERT_LOG(false, "Unknown texture format");
    }
  }
}
