//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/daFrameGraph/autoResolutionRequest.h>
#include <render/daFrameGraph/virtualResourceRequest.h>
#include <render/daFrameGraph/detail/dispatchRequestBase.h>
#include <render/daFrameGraph/detail/nodeNameId.h>
#include <render/daFrameGraph/detail/shaderNameId.h>
#include <render/daFrameGraph/detail/dispatchRequestPolicy.h>

namespace dafg
{

struct InternalRegistry;

template <detail::DispatchRequestPolicy>
class DispatchRequest;

using DispatchComputeThreadsRequest = DispatchRequest<detail::DispatchRequestPolicy::HasThreads>;
using DispatchComputeGroupsRequest = DispatchRequest<detail::DispatchRequestPolicy::HasGroups>;
using DispatchComputeIndirectRequest = DispatchRequest<detail::DispatchRequestPolicy::HasIndirect>;

using DispatchMeshThreadsRequest = DispatchRequest<detail::DispatchRequestPolicy::HasThreads | detail::DispatchRequestPolicy::HasMesh>;
using DispatchMeshGroupsRequest = DispatchRequest<detail::DispatchRequestPolicy::HasGroups | detail::DispatchRequestPolicy::HasMesh>;
using DispatchMeshIndirectRequest =
  DispatchRequest<detail::DispatchRequestPolicy::HasIndirect | detail::DispatchRequestPolicy::HasMesh>;

// Due to weird formatting with constraints
// clang-format off

/**
 * \brief Represents a request for a dispatch operation to be executed.
 * This class is used to specify the dispatch parameters.
 * \details The intended usage is
 *
 *     registry.dispatchThreads("my_shader")
 *       .x<&IPoint2::x>(registry.getResolution<2>("my_resolution_as_x"))
 *       .y<&IPoint2::y>(registry.readTexture("texture_resolution_as_y"))
 *       .z<&MyBlob::z>(registry.readBlob("blob_resolution_as_z"));
 *
 * \tparam dispatch_policy The policy of the dispatch request.
 */

template <detail::DispatchRequestPolicy dispatch_policy>
class DispatchRequest : private detail::DispatchRequestBase
{
  using RRP = detail::ResourceRequestPolicy;
  using DRP = detail::DispatchRequestPolicy;

  template <RRP policy>
  static constexpr bool hasPolicy(RRP p) { return (policy & p) == p; }

  static constexpr bool hasPolicy(DRP p) { return (dispatch_policy & p) == p; }

  static constexpr DRP flipPolicy(DRP p) { return static_cast<DRP>(eastl::to_underlying(dispatch_policy) ^ eastl::to_underlying(p)); }

  static constexpr bool hasThreadsOrGroups() { return hasPolicy(DRP::HasThreads) || hasPolicy(DRP::HasGroups); }

  template <typename T>
  static constexpr bool isBlob()
  {
    return eastl::is_trivially_copyable_v<T>;
  }

  template <typename T, RRP policy, int size>
  static constexpr bool isValidBlob()
  {
    return isBlob<T>() && hasPolicy<policy>(RRP::Readonly) && eastl::is_integral_v<T> && sizeof(T) == size;
  }

  template <auto projector, typename T, RRP policy>
  static constexpr bool isValidBlobProjector()
  {
    using ProjectedType = detail::ProjectedType<projector>;
    return isBlob<T>() && hasPolicy<policy>(RRP::Readonly) && eastl::is_invocable_v<decltype(projector), const T &> && eastl::is_integral_v<ProjectedType> && sizeof(ProjectedType) == sizeof(uint32_t);
  }

  template <auto projector, int D>
  static constexpr bool isValidResolutionProjector()
  {
    using ProjecteeType = detail::ProjecteeType<projector>;
    using ProjectedType = detail::ProjectedType<projector>;
    return ((D == 2 && eastl::is_same_v<ProjecteeType, IPoint2>) || (D == 3 && eastl::is_same_v<ProjecteeType, IPoint3>)) && eastl::is_same_v<ProjectedType, int>;
  }

  template <auto projector>
  static constexpr bool isValidTextureProjector()
  {
    using ProjecteeType = detail::ProjecteeType<projector>;
    using ProjectedType = detail::ProjectedType<projector>;
    return eastl::is_same_v<ProjectedType, int> && (eastl::is_same_v<ProjecteeType, IPoint2> || eastl::is_same_v<ProjecteeType, IPoint3>);
  }

  template <typename T, RRP policy>
  using BlobRequest = VirtualResourceRequest<T, policy>;

  template <RRP policy>
  using TextureRequest = VirtualResourceRequest<BaseTexture, policy>;

  using Base = detail::DispatchRequestBase;

  template <detail::DispatchRequestPolicy>
  friend class DispatchRequest;

  friend class Registry;
  DispatchRequest(InternalRegistry *reg, NodeNameId node_id, ShaderNameId shader_id) : Base{reg, node_id, shader_id} {};

public:

  /**
   * \brief Specifies the x dimension of dispatch.
   * \param x The x dimension.
   */
  DispatchRequest<flipPolicy(DRP::HasX)> x(uint32_t x) &&
  requires(!hasPolicy(DRP::HasX) && !hasPolicy(DRP::HasIndirect) && hasThreadsOrGroups())
  {
    Base::setArg<ArgType::X>(x);
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Specifies the x dimension of dispatch with blob.
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   * \param blob The blob request to be used for the x dimension.
   */
  template <typename T, RRP policy>
  requires(!hasPolicy(DRP::HasX) && !hasPolicy(DRP::HasIndirect) && hasThreadsOrGroups() && isValidBlob<T, policy, sizeof(uint32_t)>())
  DispatchRequest<flipPolicy(DRP::HasX)> x(BlobRequest<T, policy> blob) &&
  {
    Base::setBlobArg<ArgType::X>(blob, detail::identity_projector);
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Specifies the x dimension of dispatch with blob.
   * \tparam projector A function to extract the value from blob.
   *    Can be a pointer-to-member, i.e. `&BlobType::field` or a (pure) function pointer.
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   * \param blob The blob request to be used for the x dimension.
   */
  template <auto projector, typename T, RRP policy>
  requires(!hasPolicy(DRP::HasX) && !hasPolicy(DRP::HasIndirect) && hasThreadsOrGroups() && isValidBlobProjector<projector, T, policy>())
  DispatchRequest<flipPolicy(DRP::HasX)> x(BlobRequest<T, policy> blob) &&
  {
    Base::setBlobArg<ArgType::X>(blob, detail::erase_projector_type<projector, T>());
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Specifies the x dimension of dispatch with auto resolution.
   * \tparam projector Pointer to IPoint2/3 member to take the value from, i.e `&IPoint2::x`.
   * \tparam D The auto resolution request.
   * \param resolution The auto resolution request to be used for the x dimension.
   */
  template <auto projector, int D>
  requires(!hasPolicy(DRP::HasX) && !hasPolicy(DRP::HasIndirect) && hasThreadsOrGroups() && isValidResolutionProjector<projector, D>())
  DispatchRequest<flipPolicy(DRP::HasX)> x(AutoResolutionRequest<D> resolution) &&
  {
    Base::setAutoResolutionArg<ArgType::X>(resolution, detail::erase_projector_type<projector, detail::ProjecteeType<projector>>());
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Specifies the x dimension of dispatch with texture resolution.
   * \tparam projector Pointer to IPoint2/3 member to take the value from, i.e `&IPoint2::x`.
   * \tparam policy The policy of the texture request.
   * \param texture The texture request to be used for the x dimension.
   */
  template <auto projector, RRP policy>
  requires(!hasPolicy(DRP::HasX) && !hasPolicy(DRP::HasIndirect) && hasThreadsOrGroups() && isValidTextureProjector<projector>())
  DispatchRequest<flipPolicy(DRP::HasX)> x(TextureRequest<policy> texture) &&
  {
    Base::setTextureResolutionArg<ArgType::X>(texture, detail::erase_projector_type<projector, detail::ProjecteeType<projector>>());
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Specifies the y dimension of dispatch.
   * \param y The y dimension.
   */
  DispatchRequest<flipPolicy(DRP::HasY)> y(uint32_t y) &&
  requires(!hasPolicy(DRP::HasY) && !hasPolicy(DRP::HasIndirect) && hasThreadsOrGroups())
  {
    Base::setArg<ArgType::Y>(y);
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Specifies the y dimension of dispatch with blob.
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   * \param blob The blob request to be used for the y dimension.
   */
  template <typename T, RRP policy>
  requires(!hasPolicy(DRP::HasY) && !hasPolicy(DRP::HasIndirect) && hasThreadsOrGroups() && isValidBlob<T, policy, sizeof(uint32_t)>())
  DispatchRequest<flipPolicy(DRP::HasY)> y(BlobRequest<T, policy> blob) &&
  {
    Base::setBlobArg<ArgType::Y>(blob, detail::identity_projector);
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Specifies the y dimension of dispatch with blob.
   * \tparam projector A function to extract the value from blob.
   *    Can be a pointer-to-member, i.e. `&BlobType::field` or a (pure) function pointer.
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   * \param blob The blob request to be used for the y dimension.
   */
  template <auto projector, typename T, RRP policy>
  requires(!hasPolicy(DRP::HasY) && !hasPolicy(DRP::HasIndirect) && hasThreadsOrGroups() && isValidBlobProjector<projector, T, policy>())
  DispatchRequest<flipPolicy(DRP::HasY)> y(BlobRequest<T, policy> blob) &&
  {
    Base::setBlobArg<ArgType::Y>(blob, detail::erase_projector_type<projector, T>());
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Specifies the y dimension of dispatch with auto resolution.
   * \tparam projector Pointer to IPoint2/3 member to take the value from, i.e `&IPoint2::x`.
   * \tparam D The dimensionality of the auto resolution request.
   * \param resolution The auto resolution request to be used for the y dimension.
   */
  template <auto projector, int D>
  requires(!hasPolicy(DRP::HasY) && !hasPolicy(DRP::HasIndirect) && hasThreadsOrGroups() && isValidResolutionProjector<projector, D>())
  DispatchRequest<flipPolicy(DRP::HasY)> y(AutoResolutionRequest<D> resolution) &&
  {
    Base::setAutoResolutionArg<ArgType::Y>(resolution, detail::erase_projector_type<projector, detail::ProjecteeType<projector>>());
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Specifies the y dimension of dispatch with texture resolution.
   * \tparam projector Pointer to IPoint2/3 member to take the value from, i.e `&IPoint2::x`.
   * \tparam policy The policy of the texture request.
   * \param texture The texture request to be used for the y dimension.
   */
  template <auto projector, RRP policy>
  requires(!hasPolicy(DRP::HasY) && !hasPolicy(DRP::HasIndirect) && hasThreadsOrGroups() && isValidTextureProjector<projector>())
  DispatchRequest<flipPolicy(DRP::HasY)> y(TextureRequest<policy> texture) &&
  {
    Base::setTextureResolutionArg<ArgType::Y>(texture, detail::erase_projector_type<projector, detail::ProjecteeType<projector>>());
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Specifies the z dimension of dispatch.
   * \param z The z dimension.
   */
  DispatchRequest<flipPolicy(DRP::HasZ)> z(uint32_t z) &&
  requires(!hasPolicy(DRP::HasZ) && !hasPolicy(DRP::HasIndirect) && hasThreadsOrGroups())
  {
    Base::setArg<ArgType::Z>(z);
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Specifies the z dimension of dispatch with blob.
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   * \param blob The blob request to be used for the z dimension.
   */
  template <typename T, RRP policy>
  requires(!hasPolicy(DRP::HasZ) && !hasPolicy(DRP::HasIndirect) && hasThreadsOrGroups() && isValidBlob<T, policy, sizeof(uint32_t)>())
  DispatchRequest<flipPolicy(DRP::HasZ)> z(BlobRequest<T, policy> blob) &&
  {
    Base::setBlobArg<ArgType::Z>(blob, detail::identity_projector);
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Specifies the z dimension of dispatch with blob.
   * \tparam projector A function to extract the value from blob.
   *    Can be a pointer-to-member, i.e. `&BlobType::field` or a (pure) function pointer.
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   * \param blob The blob request to be used for the z dimension.
   */
  template <auto projector, typename T, RRP policy>
  requires(!hasPolicy(DRP::HasZ) && !hasPolicy(DRP::HasIndirect) && hasThreadsOrGroups() && isValidBlobProjector<projector, T, policy>())
  DispatchRequest<flipPolicy(DRP::HasZ)> z(BlobRequest<T, policy> blob) &&
  {
    Base::setBlobArg<ArgType::Z>(blob, detail::erase_projector_type<projector, T>());
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Specifies the z dimension of dispatch with auto resolution.
   * \tparam projector Pointer to IPoint2/3 member to take the value from, i.e `&IPoint2::x`.
   * \tparam D The dimensionality of the auto resolution request.
   * \param resolution The auto resolution request to be used for the z dimension.
   */
  template <auto projector, int D>
  requires(!hasPolicy(DRP::HasZ) && !hasPolicy(DRP::HasIndirect) && hasThreadsOrGroups() && isValidResolutionProjector<projector, D>())
  DispatchRequest<flipPolicy(DRP::HasZ)> z(AutoResolutionRequest<D> resolution) &&
  {
    Base::setAutoResolutionArg<ArgType::Z>(resolution, detail::erase_projector_type<projector, detail::ProjecteeType<projector>>());
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Specifies the z dimension of dispatch with texture resolution.
   * \tparam projector Pointer to IPoint2/3 member to take the value from, i.e `&IPoint2::x`.
   * \tparam policy The policy of the texture request.
   * \param texture The texture request to be used for the z dimension.
   */
  template <auto projector, RRP policy>
  requires(!hasPolicy(DRP::HasZ) && !hasPolicy(DRP::HasIndirect) && hasThreadsOrGroups() && isValidTextureProjector<projector>())
  DispatchRequest<flipPolicy(DRP::HasZ)> z(TextureRequest<policy> texture) &&
  {
    Base::setTextureResolutionArg<ArgType::Z>(texture, detail::erase_projector_type<projector, detail::ProjecteeType<projector>>());
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Specifies the offset of the indirect dispatch buffer.
   *
   * \param offset The offset to be used for indirect dispatch.
   */
  DispatchRequest<flipPolicy(DRP::HasOffset)> offset(size_t offset) &&
  requires(!hasPolicy(DRP::HasOffset) && hasPolicy(DRP::HasIndirect))
  {
    Base::setArg<ArgType::Offset>(offset);
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Specifies the offset of the indirect dispatch buffer.
   *
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   * \param blob The blob request to be used for the offset.
   */
  template <typename T, RRP policy>
  DispatchRequest<flipPolicy(DRP::HasOffset)> offset(BlobRequest<T, policy> blob) &&
  requires(!hasPolicy(DRP::HasOffset) && hasPolicy(DRP::HasIndirect) && isValidBlob<T, policy, sizeof(size_t)>())
  {
    Base::setBlobArg<ArgType::Offset>(blob, detail::identity_projector);
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Specifies the offset of the indirect dispatch buffer.
   *
   * \tparam projector A function to extract the value from blob.
   *    Can be a pointer-to-member, i.e. `&BlobType::field` or a (pure) function pointer.
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   */
  template <auto projector, typename T, RRP policy>
  DispatchRequest<flipPolicy(DRP::HasOffset)> offset(BlobRequest<T, policy> blob) &&
  requires(!hasPolicy(DRP::HasOffset) && hasPolicy(DRP::HasIndirect) && isValidBlobProjector<projector, T, policy>())
  {
    Base::setBlobArg<ArgType::Offset>(blob, detail::erase_projector_type<projector, T>());
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Specifies the stride of the mesh indirect dispatch buffer.
   *
   * \param stride The stride to be used for indirect dispatch.
   */
  DispatchRequest<flipPolicy(DRP::HasStride)> stride(size_t stride) &&
  requires(!hasPolicy(DRP::HasStride) && hasPolicy(DRP::HasIndirect) && hasPolicy(DRP::HasMesh))
  {
    Base::setArg<ArgType::Stride>(stride);
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Specifies the stride of the mesh indirect dispatch buffer.
   *
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   * \param blob The blob request to be used for the stride.
   */
  template <typename T, RRP policy>
  DispatchRequest<flipPolicy(DRP::HasStride)> stride(BlobRequest<T, policy> blob) &&
  requires(!hasPolicy(DRP::HasStride) && hasPolicy(DRP::HasIndirect) && hasPolicy(DRP::HasMesh) && isValidBlob<T, policy, sizeof(size_t)>())
  {
    Base::setBlobArg<ArgType::Stride>(blob, detail::identity_projector);
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Specifies the stride of the mesh indirect dispatch buffer.
   *
   * \tparam projector A function to extract the value from blob.
   *    Can be a pointer-to-member, i.e. `&BlobType::field` or a (pure) function pointer.
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   */
  template <auto projector, typename T, RRP policy>
  DispatchRequest<flipPolicy(DRP::HasStride)> stride(BlobRequest<T, policy> blob) &&
  requires(!hasPolicy(DRP::HasStride) && hasPolicy(DRP::HasIndirect) && hasPolicy(DRP::HasMesh) && isValidBlobProjector<projector, T, policy>())
  {
    Base::setBlobArg<ArgType::Stride>(blob, detail::erase_projector_type<projector, T>());
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Specifies the count of the mesh dispatches in indirect dispatch buffer.
   *
   * \param count The count of the mesh mispatches.
   */
  DispatchRequest<flipPolicy(DRP::HasCount)> count(uint32_t count) &&
  requires(!hasPolicy(DRP::HasCount) && !hasPolicy(DRP::HasIndirectCount) && hasPolicy(DRP::HasIndirect) && hasPolicy(DRP::HasMesh))
  {
    Base::setArg<ArgType::Count>(count);
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Specifies the count of the mesh dispatches in indirect dispatch buffer.
   *
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   * \param blob The blob request to be used for the count.
   */
  template <typename T, RRP policy>
  DispatchRequest<flipPolicy(DRP::HasCount)> count(BlobRequest<T, policy> blob) &&

  requires(!hasPolicy(DRP::HasCount) && !hasPolicy(DRP::HasIndirectCount) && hasPolicy(DRP::HasIndirect) && hasPolicy(DRP::HasMesh) && isValidBlob<T, policy, sizeof(uint32_t)>())
  {
    Base::setBlobArg<ArgType::Count>(blob, detail::identity_projector);
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Specifies the count of the mesh dispatches in indirect dispatch buffer.
   *
   * \tparam projector A function to extract the value from blob.
   *    Can be a pointer-to-member, i.e. `&BlobType::field` or a (pure) function pointer.
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   */
  template <auto projector, typename T, RRP policy>
  DispatchRequest<flipPolicy(DRP::HasCount)> count(BlobRequest<T, policy> blob) &&
  requires(!hasPolicy(DRP::HasCount) && !hasPolicy(DRP::HasIndirectCount) && hasPolicy(DRP::HasIndirect) && hasPolicy(DRP::HasMesh) && isValidBlobProjector<projector, T, policy>())
  {
    Base::setBlobArg<ArgType::Count>(blob, detail::erase_projector_type<projector, T>());
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Specifies the indirect count mesh dispatch buffer.
   *
   * \param buffer The name of the buffer to be used for indirect count mesh dispatch.
   */
  DispatchRequest<flipPolicy(DRP::HasIndirectCount)> count(const char *buffer) &&
  requires(!hasPolicy(DRP::HasIndirectCount) && !hasPolicy(DRP::HasCount) && hasPolicy(DRP::HasIndirect) && hasPolicy(DRP::HasMesh) )
  {
    Base::meshIndirectCount(buffer);
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Specifies the offset of indirect count buffer.
   * \param count_offset The offset to be used for indirect count buffer.
   */
  DispatchRequest<flipPolicy(DRP::HasCountOffset)> countOffset(uint32_t count_offset) &&
  requires(!hasPolicy(DRP::HasCountOffset) && hasPolicy(DRP::HasIndirectCount))
  {
    Base::setArg<ArgType::CountOffset>(count_offset);
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Specifies the offset of indirect count buffer.
   *
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   * \param blob The blob request to be used for the count offset.
   */
  template <typename T, RRP policy>
  DispatchRequest<flipPolicy(DRP::HasCountOffset)> countOffset(BlobRequest<T, policy> blob) &&
  requires(!hasPolicy(DRP::HasCountOffset) && hasPolicy(DRP::HasIndirectCount) && isValidBlob<T, policy, sizeof(uint32_t)>())
  {
    Base::setBlobArg<ArgType::CountOffset>(blob, detail::identity_projector);
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Specifies the offset of indirect count buffer.
   *
   * \tparam projector A function to extract the value from blob.
   *    Can be a pointer-to-member, i.e. `&BlobType::field` or a (pure) function pointer.
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   */
  template <auto projector, typename T, RRP policy>
  DispatchRequest<flipPolicy(DRP::HasCountOffset)> countOffset(BlobRequest<T, policy> blob) &&
  requires(!hasPolicy(DRP::HasCountOffset) && hasPolicy(DRP::HasIndirectCount) && isValidBlobProjector<projector, T, policy>())
  {
    Base::setBlobArg<ArgType::CountOffset>(blob, detail::erase_projector_type<projector, T>());
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Specifies the max count of the mesh dispatches in indirect dispatch buffer.
   *
   * \param max_count The max count of the mesh dispatches.
   */
  DispatchRequest<flipPolicy(DRP::HasMaxCount)> maxCount(uint32_t max_count) &&
  requires(!hasPolicy(DRP::HasMaxCount) && hasPolicy(DRP::HasIndirectCount))
  {
    Base::setArg<ArgType::MaxCount>(max_count);
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Specifies the max count of the mesh dispatches in indirect dispatch buffer.
   *
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   * \param blob The blob request to be used for the max count.
   */
  template <typename T, RRP policy>
  DispatchRequest<flipPolicy(DRP::HasMaxCount)> maxCount(BlobRequest<T, policy> blob) &&
  requires(!hasPolicy(DRP::HasMaxCount) && hasPolicy(DRP::HasIndirectCount) && isValidBlob<T, policy, sizeof(uint32_t)>())
  {
    Base::setBlobArg<ArgType::MaxCount>(blob, detail::identity_projector);
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Specifies the max count of the mesh dispatches in indirect dispatch buffer.
   *
   * \tparam projector A function to extract the value from blob.
   *    Can be a pointer-to-member, i.e. `&BlobType::field` or a (pure) function pointer.
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   */
  template <auto projector, typename T, RRP policy>
  DispatchRequest<flipPolicy(DRP::HasMaxCount)> maxCount(BlobRequest<T, policy> blob) &&
  requires(!hasPolicy(DRP::HasMaxCount) && hasPolicy(DRP::HasIndirectCount) && isValidBlobProjector<projector, T, policy>())
  {
    Base::setBlobArg<ArgType::MaxCount>(blob, detail::erase_projector_type<projector, T>());
    return {registry, nodeId, shaderId};
  }

private:

  /**
   * \brief Set mesh dispatch mode.
   */
  DispatchRequest<flipPolicy(DRP::HasMesh)> mesh() &&
  requires(!hasPolicy(DRP::HasMesh))
  {
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Use threads for dispatch request.
   */
  DispatchRequest<flipPolicy(DRP::HasThreads)> threads() &&
  requires(!hasThreadsOrGroups())
  {
    if constexpr (hasPolicy(DRP::HasMesh))
      Base::meshThreads();
    else
      Base::threads();
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Use thread groups for dispatch request.
   */
  DispatchRequest<flipPolicy(DRP::HasGroups)> groups() &&
  requires(!hasThreadsOrGroups())
  {
    if constexpr (hasPolicy(DRP::HasMesh))
      Base::meshGroups();
    else
      Base::groups();
    return {registry, nodeId, shaderId};
  }

  /**
   * \brief Specifies the indirect dispatch buffer.
   *
   * \param buffer The name of the buffer to be used for indirect dispatch.
   */
  DispatchRequest<flipPolicy(DRP::HasIndirect)> indirect(const char *buffer) &&
  requires(!hasPolicy(DRP::HasIndirect) && !hasThreadsOrGroups())
  {
    if constexpr (hasPolicy(DRP::HasMesh))
      Base::meshIndirect(buffer);
    else
      Base::indirect(buffer);
    return {registry, nodeId, shaderId};
  }
};

// clang-format on

} // namespace dafg
