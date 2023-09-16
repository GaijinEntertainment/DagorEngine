//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <render/daBfg/virtualResourceRequest.h>


namespace dabfg
{

/**
 * \brief A builder object for an incomplete resource creation request:
 * we want to create something, but don't know what yet.
 * Note that all methods of this class return a new request object which
 * should be used for further specification.
 * Never re-use objects of this type for several requests.
 */
class VirtualResourceCreationSemiRequest
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
   * \brief Specifies the request to be a texture creation one.
   *
   * \param info The texture creation info.
   */
  VirtualResourceRequest<BaseTexture, RRP::None> texture(const Texture2dCreateInfo &info) &&
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
  VirtualResourceRequest<BaseTexture, RRP::None> buffer(const BufferCreateInfo &info) &&
  {
    Base::buffer(info);
    return {resUid, nodeId, registry};
  }


  /**
   * \name Buffer methods
   * \details The buffer naming convention repeats the one in the d3d driver
   * interface, see functions in namespace \inlinerst :cpp:type:`d3d_buffers` \endrst
   */
  ///@{

  VirtualResourceRequest<Sbuffer, RRP::None> bufferCb(uint32_t register_count) &&
  {
    BufferCreateInfo ci{d3d_buffers::CBUFFER_REGISTER_SIZE, register_count, SBCF_CB_ONE_FRAME, 0};
    Base::buffer(ci);
    return {resUid, nodeId, registry};
  }

  VirtualResourceRequest<Sbuffer, RRP::None> byteAddressBufferUaSr(uint32_t size_in_dwords) &&
  {
    BufferCreateInfo ci{d3d_buffers::BYTE_ADDRESS_ELEMENT_SIZE, size_in_dwords, SBCF_UA_SR_BYTE_ADDRESS, 0};
    Base::buffer(ci);
    return {resUid, nodeId, registry};
  }

  VirtualResourceRequest<Sbuffer, RRP::None> byteAddressBufferUa(uint32_t size_in_dwords) &&
  {
    BufferCreateInfo ci{d3d_buffers::BYTE_ADDRESS_ELEMENT_SIZE, size_in_dwords, SBCF_UA_BYTE_ADDRESS, 0};
    Base::buffer(ci);
    return {resUid, nodeId, registry};
  }

  template <class T>
  VirtualResourceRequest<Sbuffer, RRP::None> structuredBufferUaSr(uint32_t element_count) &&
  {
    BufferCreateInfo ci{sizeof(T), element_count, SBCF_UA_SR_STRUCTURED, 0};
    Base::buffer(ci);
    return {resUid, nodeId, registry};
  }

  template <class T>
  VirtualResourceRequest<Sbuffer, RRP::None> structuredBufferUa(uint32_t element_count) &&
  {
    BufferCreateInfo ci{sizeof(T), element_count, SBCF_UA_STRUCTURED, 0};
    Base::buffer(ci);
    return {resUid, nodeId, registry};
  }

  VirtualResourceRequest<Sbuffer, RRP::None> indirectBufferUa(d3d_buffers::Indirect indirect_type, uint32_t call_count) &&
  {
    BufferCreateInfo ci{d3d_buffers::BYTE_ADDRESS_ELEMENT_SIZE, call_count * dword_count_per_call(indirect_type), SBCF_UA_INDIRECT, 0};
    Base::buffer(ci);
    return {resUid, nodeId, registry};
  }

  VirtualResourceRequest<Sbuffer, RRP::None> indirectBuffer(d3d_buffers::Indirect indirect_type, uint32_t call_count) &&
  {
    BufferCreateInfo ci{d3d_buffers::BYTE_ADDRESS_ELEMENT_SIZE, call_count * dword_count_per_call(indirect_type), SBCF_INDIRECT, 0};
    Base::buffer(ci);
    return {resUid, nodeId, registry};
  }

  VirtualResourceRequest<Sbuffer, RRP::None> stagingBuffer(uint32_t size_in_bytes) &&
  {
    BufferCreateInfo ci{1, size_in_bytes, SBCF_STAGING_BUFFER, 0};
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
  VirtualResourceRequest<T, RRP::None> blob() &&
  {
    Base::blob(BlobDescription{tag_for<T>(), sizeof(T), alignof(T),
      // IMPORTANT: {} zero-initializes structs
      +[](void *ptr) { new (ptr) T{}; }, //
      +[](void *ptr) { eastl::destroy_at<T>(reinterpret_cast<T *>(ptr)); }});

    return {resUid, nodeId, registry};
  }
};

} // namespace dabfg
