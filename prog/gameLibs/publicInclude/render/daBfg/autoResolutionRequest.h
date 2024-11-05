//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/daBfg/detail/autoResTypeNameId.h>

#include <math/integer/dag_IPoint2.h>


namespace dabfg
{

struct ResourceProvider;
namespace detail
{
struct VirtualResourceRequestBase;

template <int D>
struct IPointImpl;
template <>
struct IPointImpl<2>
{
  using Value = IPoint2;
};
template <>
struct IPointImpl<3>
{
  using Value = IPoint3;
};

} // namespace detail

template <int D>
using IPoint = typename detail::IPointImpl<D>::Value;

/**
 * \brief This class represents a daBfg-managed automatic resolution
 * type for a texture. If this resolution is specified for a texture,
 * the actual texture's resolution at runtime will be the dynamic
 * resolution scaled by the multiplier, but the consumed memory will
 * always be equal to the static resolution times the multiplier.
 * See NameSpace::setResolution and NameSpace::setDynamicResolution.
 * Note that objects of this type MAY be captured into the execution
 * callback and used to access the actual resolution on a particular
 * frame, but the resolution should NEVER be accessed in the declaration
 * callback, as the value will be undefined.
 * \tparam D -- dimensionality of the resolution, either 2 or 3.
 */
template <int D>
class AutoResolutionRequest
{
  friend class NameSpaceRequest;
  friend struct detail::VirtualResourceRequestBase;

  AutoResolutionRequest(AutoResTypeNameId id, float mult, const ResourceProvider *p) : autoResTypeId{id}, multiplier{mult}, provider{p}
  {}

public:
  /**
   * \brief Returns the current dynamic resolution for this auto-res type.
   * \warning Should only be used for setting the d3d viewport/scissor, NEVER create
   * textures with this resolution, as it might be changing every single frame!!!
   * Also never call this outside of the execution callback for the same reason!
   *
   * \return The current dynamic resolution for this type.
   */
  IPoint<D> get() const;

private:
  AutoResTypeNameId autoResTypeId;
  float multiplier = 1.f;
  const ResourceProvider *provider;
};

} // namespace dabfg