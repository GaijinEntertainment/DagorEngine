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
 * we want to create something, but don't know what yet.
 * Note that all methods of this class return a new request object which
 * should be used for further specification.
 * Never re-use objects of this type for several requests.
 */
class [[nodiscard]] VirtualResourceCreationSemiRequest
  /// \cond DETAIL
  : private detail::VirtualResourceRequestBase
/// \endcond
{
  using Base = detail::VirtualResourceRequestBase;
  using RRP = detail::ResourceRequestPolicy;

  friend class Registry;
  VirtualResourceCreationSemiRequest(detail::ResUid resId, NodeNameId node, InternalRegistry *reg) : Base{resId, node, reg} {}

public:
  /**
   * \brief Specifies the request to be a 2D texture creation one.
   *
   * \param info The 2D texture creation info.
   */
  VirtualResourceRequest<BaseTexture, RRP::None> texture(const Texture2dCreateInfo &info) &&
  {
    Base::texture(info);
    return {resUid, nodeId, registry};
  }

  /**
   * \brief Specifies the request to be a 3D texture creation one.
   *
   * \param info The 3D texture creation info.
   */
  VirtualResourceRequest<BaseTexture, RRP::None> texture(const Texture3dCreateInfo &info) &&
  {
    Base::texture(info);
    return {resUid, nodeId, registry};
  }

  /**
   * \brief Specifies the request to be a buffer creation one. Note that
   * this is a legacy method that is error-prone and hard to use correctly.
   * Avoid this in favor of one of the methods below if possible.
   *
   * \param info The buffer creation info.
   */
  VirtualResourceRequest<Sbuffer, RRP::HasClearValue> buffer(const BufferCreateInfo &info) &&
  {
    Base::buffer(info);
    return {resUid, nodeId, registry};
  }


  /**
   * \name Buffer methods
   * \details The buffer naming convention repeats the one in the d3d driver
   * interface, see functions in namespace \inlinerst :cpp:type:`d3d::buffers` \endrst
   */
  ///@{

  VirtualResourceRequest<Sbuffer, RRP::HasClearValue> byteAddressBufferUaSr(uint32_t size_in_dwords) &&
  {
    BufferCreateInfo ci{d3d::buffers::BYTE_ADDRESS_ELEMENT_SIZE, size_in_dwords, SBCF_UA_SR_BYTE_ADDRESS, 0};
    Base::buffer(ci);
    return {resUid, nodeId, registry};
  }

  VirtualResourceRequest<Sbuffer, RRP::HasClearValue> byteAddressBufferUa(uint32_t size_in_dwords) &&
  {
    BufferCreateInfo ci{d3d::buffers::BYTE_ADDRESS_ELEMENT_SIZE, size_in_dwords, SBCF_UA_BYTE_ADDRESS, 0};
    Base::buffer(ci);
    return {resUid, nodeId, registry};
  }

  template <class T>
  VirtualResourceRequest<Sbuffer, RRP::HasClearValue> structuredBufferUaSr(uint32_t element_count) &&
  {
    BufferCreateInfo ci{sizeof(T), element_count, SBCF_UA_SR_STRUCTURED, 0};
    Base::buffer(ci);
    return {resUid, nodeId, registry};
  }

  template <class T>
  VirtualResourceRequest<Sbuffer, RRP::HasClearValue> structuredBufferUa(uint32_t element_count) &&
  {
    BufferCreateInfo ci{sizeof(T), element_count, SBCF_UA_STRUCTURED, 0};
    Base::buffer(ci);
    return {resUid, nodeId, registry};
  }

  VirtualResourceRequest<Sbuffer, RRP::HasClearValue> indirectBufferUa(d3d::buffers::Indirect indirect_type, uint32_t call_count) &&
  {
    BufferCreateInfo ci{
      d3d::buffers::BYTE_ADDRESS_ELEMENT_SIZE, call_count * dword_count_per_call(indirect_type), SBCF_UA_INDIRECT, 0};
    Base::buffer(ci);
    return {resUid, nodeId, registry};
  }

  VirtualResourceRequest<Sbuffer, RRP::HasClearValue> indirectBuffer(d3d::buffers::Indirect indirect_type, uint32_t call_count) &&
  {
    BufferCreateInfo ci{d3d::buffers::BYTE_ADDRESS_ELEMENT_SIZE, call_count * dword_count_per_call(indirect_type), SBCF_INDIRECT, 0};
    Base::buffer(ci);
    return {resUid, nodeId, registry};
  }

  ///@}

  /**
   * \brief Specifies the request to be a blob creation one.
   *
   * \tparam T The type of the CPU data blob
   */
  template <class T>
  VirtualResourceRequest<T, RRP::HasClearValue> blob() &&
  {
    BlobDescription desc{
      .typeTag = tag_for<T>(),
      .ctorOverride = nullptr,
    };
    Base::blob(eastl::move(desc), detail::make_rtti<T>());
    return {resUid, nodeId, registry};
  }

  /**
   * \brief Specifies the request to be a blob creation one.
   *
   * \tparam T The type of the CPU data blob
   *
   * \param defaultValue The default value to initialize the blob with.
   */
  template <class T>
  VirtualResourceRequest<T, RRP::HasClearValue> blob(const T defaultValue) &&
  {
    BlobDescription desc{
      .typeTag = tag_for<T>(),
      .ctorOverride =
        eastl::make_unique<BlobDescription::CtorT>(eastl::move([defaultValue](void *ptr) { new (ptr) T{defaultValue}; })),
    };
    Base::blob(eastl::move(desc), detail::make_rtti<T>());
    return {resUid, nodeId, registry};
  }
};

} // namespace dafg
