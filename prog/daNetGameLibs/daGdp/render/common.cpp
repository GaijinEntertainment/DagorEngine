// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_assert.h>
#include <dag/dag_vector.h>
#include <generic/dag_enumerate.h>
#include <math/dag_hlsl_floatx.h>
#include "../shaders/dagdp_common.hlsli"
#include "common.h"

namespace dagdp
{

using TmpDesc = eastl::fixed_string<char, 64>;

template <typename L>
static bool set_params(const ecs::Object &object, const L &get_desc, PlaceableParams &result)
{
  auto getP2Field = [&](const char *field, Point2 &out, bool degrees = false) -> bool {
    const auto iter = object.find_as(field);
    if (iter == object.end())
      return true; // No field, do not modify value.

    const Point2 *ptr = iter->second.getNullable<Point2>();
    if (!ptr)
    {
      const auto desc = get_desc();
      logerr("daGdp: %.*s must have Point2 %s", desc.size(), desc.data(), field);
      return false;
    }

    if (degrees)
    {
      out.x = DegToRad(ptr->x);
      out.y = DegToRad(ptr->y);
    }
    else
      out = *ptr;

    return true;
  };

  if (const auto iter = object.find_as("weight"); iter != object.end())
  {
    const float *ptr = iter->second.getNullable<float>();
    if (!ptr)
    {
      const auto desc = get_desc();
      logerr("daGdp: %.*s must have float weight", desc.size(), desc.data());
      return false;
    }

    result.weight = *ptr;
  }

  if (const auto iter = object.find_as("slope_factor"); iter != object.end())
  {
    const float *ptr = iter->second.getNullable<float>();
    if (!ptr)
    {
      const auto desc = get_desc();
      logerr("daGdp: %.*s must have float slope_factor", desc.size(), desc.data());
      return false;
    }

    result.slopeFactor = *ptr;
  }

  if (const auto iter = object.find_as("slope_invert"); iter != object.end())
  {
    const bool *ptr = iter->second.getNullable<bool>();
    if (!ptr)
    {
      const auto desc = get_desc();
      logerr("daGdp: %.*s must have bool slope_invert", desc.size(), desc.data());
      return false;
    }

    result.flags &= ~SLOPE_PLACEMENT_IS_INVERTED_BIT;
    if (*ptr)
      result.flags |= SLOPE_PLACEMENT_IS_INVERTED_BIT;
  }

  if (!getP2Field("scale", result.scaleMidDev))
    return false;

  if (result.scaleMidDev.x <= 0.0f)
  {
    const auto desc = get_desc();
    logerr("daGdp: %.*s has invalid base scale", desc.size(), desc.data());
    return false;
  }

  if (result.scaleMidDev.y < 0.0f || result.scaleMidDev.y >= result.scaleMidDev.x)
  {
    const auto desc = get_desc();
    logerr("daGdp: %.*s has invalid scale deviation", desc.size(), desc.data());
    return false;
  }

  if (const auto iter = object.find_as("orientation"); iter != object.end())
  {
    const ecs::string *ptr = iter->second.getNullable<ecs::string>();

    if (ptr)
    {
      if (*ptr == "world")
      {
        result.flags &= ~(ORIENTATION_X_IS_PROJECTED_BIT | ORIENTATION_Y_IS_NORMAL_BIT);
      }
      else if (*ptr == "world_xz")
      {
        result.flags &= ~ORIENTATION_Y_IS_NORMAL_BIT;
        result.flags |= ORIENTATION_X_IS_PROJECTED_BIT;
      }
      else if (*ptr == "normal")
      {
        result.flags &= ~ORIENTATION_X_IS_PROJECTED_BIT;
        result.flags |= ORIENTATION_Y_IS_NORMAL_BIT;
      }
      else if (*ptr == "normal_xz")
      {
        result.flags |= (ORIENTATION_X_IS_PROJECTED_BIT | ORIENTATION_Y_IS_NORMAL_BIT);
      }
      else
        ptr = nullptr; // Not one of the allowed values.
    }

    if (!ptr)
    {
      const auto desc = get_desc();
      logerr("daGdp: %.*s must have orientation as one of: world, world_xz, normal, normal_xz.", desc.size(), desc.data());
      return false;
    }
  }

  if (const auto iter = object.find_as("heightmap_imitators"); iter != object.end())
  {
    const ecs::string *ptr = iter->second.getNullable<ecs::string>();

    if (ptr)
    {
      if (*ptr == "allow")
      {
        result.flags &= ~(HEIGHTMAP_IMITATORS_ARE_REJECTED_BIT | HEIGHTMAP_IMITATORS_ARE_REQUIRED_BIT);
      }
      else if (*ptr == "reject")
      {
        result.flags &= ~HEIGHTMAP_IMITATORS_ARE_REQUIRED_BIT;
        result.flags |= HEIGHTMAP_IMITATORS_ARE_REJECTED_BIT;
      }
      else if (*ptr == "require")
      {
        result.flags &= ~HEIGHTMAP_IMITATORS_ARE_REJECTED_BIT;
        result.flags |= HEIGHTMAP_IMITATORS_ARE_REQUIRED_BIT;
      }
      else
        ptr = nullptr; // Not one of the allowed values.
    }

    if (!ptr)
    {
      const auto desc = get_desc();
      logerr("daGdp: %.*s must have heightmap_imitators as one of: allow, reject, require.", desc.size(), desc.data());
      return false;
    }
  }

  if (const auto iter = object.find_as("water_placement"); iter != object.end())
  {
    const bool *ptr = iter->second.getNullable<bool>();

    if (!ptr)
    {
      const auto desc = get_desc();
      logerr("daGdp: %.*s must have bool water_placement", desc.size(), desc.data());
      return false;
    }

    result.flags &= ~WATER_PLACEMENT_BIT;
    if (*ptr)
      result.flags |= WATER_PLACEMENT_BIT;
  }

  if (const auto iter = object.find_as("delete_on_deform"); iter != object.end())
  {
    const bool *ptr = iter->second.getNullable<bool>();
    if (!ptr)
    {
      const auto desc = get_desc();
      logerr("daGdp: %.*s must have bool delete_on_deform", desc.size(), desc.data());
      return false;
    }

    result.flags &= ~PERSIST_ON_HEIGHTMAP_DEFORM_BIT;
    if (!(*ptr))
      result.flags |= PERSIST_ON_HEIGHTMAP_DEFORM_BIT;
  }

  if (const auto iter = object.find_as("density_mask_channel"); iter != object.end())
  {
    // Clearing all density mask related flag bits
    result.flags &= ~(DENSITY_MASK_CHANNEL_ALL_BITS);

    const ecs::string *ptr = iter->second.getNullable<ecs::string>();

    if (ptr)
    {
      // Invalid density flag value is 0 by default so that doesn't have to be set explicitly
      if (*ptr == "red")
      {
        result.flags |= DENSITY_MASK_CHANNEL_RED;
      }
      else if (*ptr == "green")
      {
        result.flags |= DENSITY_MASK_CHANNEL_GREEN;
      }
      else if (*ptr == "blue")
      {
        result.flags |= DENSITY_MASK_CHANNEL_BLUE;
      }
      else if (*ptr == "alpha")
      {
        result.flags |= DENSITY_MASK_CHANNEL_ALPHA;
      }
      else
        ptr = nullptr; // Not one of the allowed values.
    }

    if (!ptr)
    {
      const auto desc = get_desc();
      logerr("daGdp: %.*s must have density_mask_channel as one of: invalid, red, green, blue, alpha.", desc.size(), desc.data());
      return false;
    }
  }

  if (!getP2Field("rot_y", result.yawRadiansMidDev, true))
    return false;

  if (!getP2Field("rot_x", result.pitchRadiansMidDev, true))
    return false;

  if (!getP2Field("rot_z", result.rollRadiansMidDev, true))
    return false;

  if (result.yawRadiansMidDev.y < 0.0f)
  {
    const auto desc = get_desc();
    logerr("daGdp: %.*s has negative yaw deviation", desc.size(), desc.data());
    return false;
  }

  if (result.pitchRadiansMidDev.y < 0.0f)
  {
    const auto desc = get_desc();
    logerr("daGdp: %.*s has negative pitch deviation", desc.size(), desc.data());
    return false;
  }

  if (result.rollRadiansMidDev.y < 0.0f)
  {
    const auto desc = get_desc();
    logerr("daGdp: %.*s has negative roll deviation", desc.size(), desc.data());
    return false;
  }

  return true;
}

bool set_common_params(const ecs::Object &object, ecs::EntityId eid, PlaceableParams &result)
{
  const auto getDesc = [&]() {
    TmpDesc bufferName(TmpDesc::CtorSprintf(), "dagdp__params of entity with EID %u", static_cast<unsigned int>(eid));
    return bufferName;
  };

  return set_params(object, getDesc, result);
}

bool set_object_params(const ecs::ChildComponent &child, ecs::EntityId eid, const eastl::string &field_name, PlaceableParams &result)
{
  const auto getDesc = [&]() {
    TmpDesc bufferName(TmpDesc::CtorSprintf(), "asset %s of entity with EID %u", field_name.c_str(), static_cast<unsigned int>(eid));
    return bufferName;
  };

  const auto *subObject = child.getNullable<ecs::Object>();
  if (subObject)
  {
    if (!set_params(*subObject, getDesc, result))
      return false;
  }
  else if (child.is<float>())
  {
    const auto desc = getDesc();
    logerr("daGdp: %.*s uses disallowed weight syntax, please use `\"foo:object\"{ weight:r=1 }` instead!", desc.size(), desc.data());
    return false;
  }

  if (result.weight <= 0.0f)
  {
    const auto desc = getDesc();
    logerr("daGdp: %.*s has invalid weight %f", desc.size(), desc.data(), result.weight);
    return false;
  }

  return true;
}

} // namespace dagdp