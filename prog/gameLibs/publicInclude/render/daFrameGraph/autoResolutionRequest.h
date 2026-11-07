//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/daFrameGraph/detail/autoResTypeNameId.h>

#include <math/integer/dag_IPoint2.h>
#include <render/daFrameGraph/detail/dispatchRequestPolicy.h>


namespace dafg
{

struct ResourceProvider;
struct InternalRegistry;

namespace jsondump
{
void dump_registry_to_json(const InternalRegistry &);
} // namespace jsondump

namespace visualization::usergraph
{
class Visualizer;
} // namespace visualization::usergraph


namespace detail
{

struct VirtualResourceRequestBase;
struct DispatchRequestBase;

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
 * \brief This class represents a daFG-managed automatic resolution
 * type for a texture. If this resolution is specified for a texture,
 * the actual texture's resolution at runtime will be the dynamic
 * resolution scaled by the multiplier and rounded down, unless roundingUp()
 * is requested, and the consumed memory will always be equal to the static
 * resolution scaled and rounded the same way.
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
  friend struct detail::DispatchRequestBase;

  friend class visualization::usergraph::Visualizer;
  friend void jsondump::dump_registry_to_json(const InternalRegistry &);

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

  /**
   * \brief Rounds the scaled resolution up instead of down, at the creation
   * and at every dynamic resolution change. For a texture whose texel covers
   * a block of pixels, such as a tile grid, so that the last partial block
   * has a texel of its own. The flag is part of the request value: a texture
   * created from one request and a get() on another agree only when both
   * carry it.
   */
  AutoResolutionRequest roundingUp() const
  {
    AutoResolutionRequest result = *this;
    result.roundUp = true;
    return result;
  }

  bool operator==(const AutoResolutionRequest<D> &) const = default;

private:
  AutoResTypeNameId autoResTypeId;
  bool roundUp = false; // in the padding of the id, a node callback that captures a request keeps its size
  float multiplier = 1.f;
  const ResourceProvider *provider;
};

} // namespace dafg