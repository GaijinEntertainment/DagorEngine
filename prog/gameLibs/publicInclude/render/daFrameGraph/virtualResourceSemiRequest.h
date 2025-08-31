//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/daFrameGraph/virtualResourceRequest.h>


namespace dafg
{

/**
 * \brief A builder object for an incomplete resource creation request:
 * we want to read, modify or rename something, but don't know what yet.
 * Note that all methods of this class return a new request object which
 * should be used for further specification.
 * Never re-use objects of this type for several requests.
 *
 * \tparam policy Static data about this request's structure used for
 *   erroring out at compile time if this class is used incorrectly.
 */
template <detail::ResourceRequestPolicy policy>
class VirtualResourceSemiRequest
  /// \cond DETAIL
  : private detail::VirtualResourceRequestBase
/// \endcond
{
  using Base = detail::VirtualResourceRequestBase;

  friend class StateRequest;
  friend class NameSpaceRequest;
  VirtualResourceSemiRequest(detail::ResUid resId, NodeNameId node, InternalRegistry *reg) : Base{resId, node, reg} {}

public:
  /// \brief Specifies this to be a texture request
  VirtualResourceRequest<BaseTexture, policy> texture() && { return {resUid, nodeId, registry}; }

  /// \brief Specifies this to be a buffer request
  VirtualResourceRequest<Sbuffer, policy> buffer() && { return {resUid, nodeId, registry}; }

  /**
   * \brief Specifies this to be a blob request
   *
   * \tparam T The type of the blob to be requested.
   */
  template <class T>
  VirtualResourceRequest<T, policy> blob() &&
  {
    Base::markWithTag(tag_for<T>());
    return {resUid, nodeId, registry};
  }
};

} // namespace dafg
