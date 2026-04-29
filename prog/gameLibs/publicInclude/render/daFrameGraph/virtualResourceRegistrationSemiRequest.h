//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/daFrameGraph/virtualResourceRequest.h>


namespace dafg
{

/**
 * \brief A builder object for an incomplete resource registration request:
 * we want to register something, but don't know what yet.
 * Note that all methods of this class return a new request object which
 * should be used for further specification.
 * Never re-use objects of this type for several requests.
 */
class [[nodiscard]] VirtualResourceRegistrationSemiRequest
  /// \cond DETAIL
  : private detail::VirtualResourceRequestBase
/// \endcond
{
  using Base = detail::VirtualResourceRequestBase;
  using RRP = detail::ResourceRequestPolicy;

  friend class BaseRegistry;
  VirtualResourceRegistrationSemiRequest(detail::ResUid resId, NodeNameId node, InternalRegistry *reg) : Base{resId, node, reg} {}

public:
  /**
   * \brief Specifies the request to be a back buffer creation one.
   */
  VirtualResourceRequest<BaseTexture, RRP::HasClearValue> backBuffer() &&
  {
    Base::backBuffer();
    return {resUid, nodeId, registry};
  }

  /**
   * \brief Specifies the request to be an external texture creation one.
   *
   * \param external_resource_provider
   *   A callback that maps a multiplexing index to a ManagedTexView. It might be called
   *   an arbitrary number of times, but only while the current node is registered.
   */
  VirtualResourceRequest<BaseTexture, RRP::HasClearValue> texture(dafg::ExternalResourceProvider &&external_resource_provider) &&
  {
    Base::externalTexture(eastl::move(external_resource_provider));
    return {resUid, nodeId, registry};
  }

  /**
   * \brief Specifies the request to be an external buffer creation one.
   *
   * \param external_resource_provider
   *   A callback that maps a multiplexing index to a ManagedBufferView. It might be called
   *   an arbitrary number of times, but only while the current node is registered.
   */
  VirtualResourceRequest<Sbuffer, RRP::HasClearValue> buffer(dafg::ExternalResourceProvider &&external_resource_provider) &&
  {
    Base::externalBuffer(eastl::move(external_resource_provider));
    return {resUid, nodeId, registry};
  }
};

} // namespace dafg
