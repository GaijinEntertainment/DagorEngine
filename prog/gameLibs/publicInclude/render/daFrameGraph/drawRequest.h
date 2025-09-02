//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/daFrameGraph/detail/drawRequestBase.h>
#include <render/daFrameGraph/detail/drawRequestPolicy.h>
#include <render/daFrameGraph/virtualResourceRequest.h>

namespace dafg
{

struct InternalRegistry;

template <detail::DrawRequestPolicy, bool>
class DrawRequest;

// Due to weird formatting with constraints
// clang-format off

/**
 * \brief Represents a request for a draw operation to be executed.
 * This class is used to specify the draw parameters.
 * \details The intended usage is
 *    registry.draw("my_shader", DrawPrimitive::TriangleList)
 *      .startVertex(0)
 *      .primitiveCount<&myBlob::count>(registry.readBlob("my_blob"))
 *      .instanceCount(42);
 *
 * \tparam draw_policy The policy of the draw request.
 * \tparam indexed Whether the draw request is indexed.
 */
template <detail::DrawRequestPolicy draw_policy, bool indexed>
class DrawRequest : private detail::DrawRequestBase
{
  template <detail::DrawRequestPolicy, bool>
  friend class DrawRequest;

  using Base = detail::DrawRequestBase;

  using RRP = detail::ResourceRequestPolicy;
  using DRP = detail::DrawRequestPolicy;

  template <typename T, RRP policy>
  using BlobRequest = VirtualResourceRequest<T, policy>;

  template <RRP policy>
  static constexpr bool hasPolicy(RRP p) { return (policy & p) == p; }

  static constexpr bool hasPolicy(DRP p) { return (draw_policy & p) == p; }

  static constexpr DRP flipPolicy(DRP p) { return static_cast<DRP>(eastl::to_underlying(draw_policy) ^ eastl::to_underlying(p)); }

  template <typename T>
  static constexpr bool isBlob()
  {
    return eastl::is_trivially_copyable_v<T>;
  }

  template <typename T, RRP policy>
  static constexpr bool isValidBlob()
  {
    return isBlob<T>() && hasPolicy<policy>(RRP::Readonly) && eastl::is_integral_v<T> && sizeof(T) == sizeof(uint32_t);
  }

  template <auto projector, typename T, RRP policy>
  static constexpr bool isValidBlobProjector()
  {
    using ProjectedType = detail::ProjectedType<projector>;
    return isBlob<T>() && hasPolicy<policy>(RRP::Readonly) && eastl::is_invocable_v<decltype(projector), const T &> && eastl::is_integral_v<ProjectedType> && sizeof(ProjectedType) == sizeof(uint32_t);
  }

  friend class Registry;
  DrawRequest(InternalRegistry *reg, NodeNameId node_id, ShaderNameId shader_id, dafg::DrawPrimitive primitive) : Base{reg, node_id, shader_id}
  {
    if constexpr (draw_policy == DRP::Default && indexed)
      Base::drawIndexed(primitive);
    else if constexpr (draw_policy == DRP::Default)
      Base::draw(primitive);
  };

  DrawRequest(InternalRegistry *reg, NodeNameId node_id, ShaderNameId shader_id) : Base{reg, node_id, shader_id} {};

public:

  /**
   * \brief Specifies the start vertex of draw request.
   * \param startVertex The start vertex.
   */
  DrawRequest<flipPolicy(DRP::HasStartVertex), indexed> startVertex(uint32_t startVertex) &&
  requires(!indexed && !hasPolicy(DRP::HasStartVertex) && !hasPolicy(DRP::HasIndirect))
  {
    Base::setArg<ArgType::StartVertex>(startVertex);
    return {registry, nodeId, shaderId};
  };

  /**
   * \brief Specifies the start vertex of draw request with blob.
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   * \param blob The blob request to be used for the start vertex.
   */
  template <typename T, RRP policy>
  DrawRequest<flipPolicy(DRP::HasStartVertex), indexed> startVertex(BlobRequest<T, policy> blob) &&
  requires(!indexed && !hasPolicy(DRP::HasStartVertex) && !hasPolicy(DRP::HasIndirect) && isValidBlob<T, policy>())
  {
    Base::setBlobArg<ArgType::StartVertex>(blob, detail::identity_projector);
    return {registry, nodeId, shaderId};
  };

  /**
   * \brief Specifies the start vertex of draw request with blob and projector.
   * \tparam projector A function to extract the value from blob.
   *    Can be a pointer-to-member, i.e. `&BlobType::field` or a (pure) function pointer.
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   * \param blob The blob request to be used for the start vertex.
   */
  template <auto projector, typename T, RRP policy>
  DrawRequest<flipPolicy(DRP::HasStartVertex), indexed> startVertex(BlobRequest<T, policy> blob) &&
  requires(!indexed && !hasPolicy(DRP::HasStartVertex) && !hasPolicy(DRP::HasIndirect) && isValidBlobProjector<projector, T, policy>())
  {
    Base::setBlobArg<ArgType::StartVertex>(blob, detail::erase_projector_type<projector, T>());
    return {registry, nodeId, shaderId};
  };

  /**
   * \brief Specifies the base vertex of draw request.
   * \param base_vertex The base vertex.
   */
  DrawRequest<flipPolicy(DRP::HasBaseVertex), indexed> baseVertex(int32_t base_vertex) &&
  requires(indexed && !hasPolicy(DRP::HasBaseVertex) && !hasPolicy(DRP::HasIndirect))
  {
    Base::setArg<ArgType::BaseVertex>(base_vertex);
    return {registry, nodeId, shaderId};
  };

  /**
   * \brief Specifies the base vertex of indexed draw request with blob.
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   * \param blob The blob request to be used for the base vertex.
   */
  template <typename T, RRP policy>
  DrawRequest<flipPolicy(DRP::HasBaseVertex), indexed> baseVertex(BlobRequest<T, policy> blob) &&
  requires(indexed && !hasPolicy(DRP::HasBaseVertex) && !hasPolicy(DRP::HasIndirect) && isValidBlob<T, policy>())
  {
    Base::setBlobArg<ArgType::BaseVertex>(blob, detail::identity_projector);
    return {registry, nodeId, shaderId};
  };

  /**
   * \brief Specifies the base vertex of indexed draw request with blob and projector.
   * \tparam projector A function to extract the value from blob.
   *    Can be a pointer-to-member, i.e. `&BlobType::field` or a (pure) function pointer.
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   * \param blob The blob request to be used for the base vertex.
   */
  template <auto projector, typename T, RRP policy>
  DrawRequest<flipPolicy(DRP::HasBaseVertex), indexed> baseVertex(BlobRequest<T, policy> blob) &&
  requires(indexed && !hasPolicy(DRP::HasBaseVertex) && !hasPolicy(DRP::HasIndirect) && isValidBlobProjector<projector, T, policy>())
  {
    Base::setBlobArg<ArgType::BaseVertex>(blob, detail::erase_projector_type<projector, T>());
    return {registry, nodeId, shaderId};
  };

  /**
   * \brief Specifies the start index of draw request.
   * \param start_index The start index.
   */
  DrawRequest<flipPolicy(DRP::HasStartIndex), indexed> startIndex(uint32_t start_index) &&
  requires(indexed && !hasPolicy(DRP::HasStartIndex) && !hasPolicy(DRP::HasIndirect))
  {
    Base::setArg<ArgType::StartIndex>(start_index);
    return {registry, nodeId, shaderId};
  };

  /**
   * \brief Specifies the start index of indexed draw request with blob.
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   * \param blob The blob request to be used for the start index.
   */
  template <typename T, RRP policy>
  DrawRequest<flipPolicy(DRP::HasStartIndex), indexed> startIndex(BlobRequest<T, policy> blob) &&
  requires(indexed && !hasPolicy(DRP::HasStartIndex) && !hasPolicy(DRP::HasIndirect) && isValidBlob<T, policy>())
  {
    Base::setBlobArg<ArgType::StartIndex>(blob, detail::identity_projector);
    return {registry, nodeId, shaderId};
  };

  /**
   * \brief Specifies the start index of indexed draw request with blob and projector.
   * \tparam projector A function to extract the value from blob.
   *    Can be a pointer-to-member, i.e. `&BlobType::field` or a (pure) function pointer.
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   * \param blob The blob request to be used for the start index.
   */
  template <auto projector, typename T, RRP policy>
  DrawRequest<flipPolicy(DRP::HasStartIndex), indexed> startIndex(BlobRequest<T, policy> blob) &&
  requires(indexed && !hasPolicy(DRP::HasStartIndex) && !hasPolicy(DRP::HasIndirect) && isValidBlobProjector<projector, T, policy>())
  {
    Base::setBlobArg<ArgType::StartIndex>(blob, detail::erase_projector_type<projector, T>());
    return {registry, nodeId, shaderId};
  };

  /**
   * \brief Specifies the primitive count of draw request.
   * \param primitive_count The primitive count.
   */
  DrawRequest<flipPolicy(DRP::HasPrimitiveCount), indexed> primitiveCount(uint32_t primitive_count) &&
  requires(!hasPolicy(DRP::HasPrimitiveCount) && !hasPolicy(DRP::HasIndirect))
  {
    Base::setArg<ArgType::PrimitiveCount>(primitive_count);
    return {registry, nodeId, shaderId};
  };

  /**
   * \brief Specifies the primitive count of draw request with blob.
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   * \param blob The blob request to be used for the primitive count.
   */
  template <typename T, RRP policy>
  DrawRequest<flipPolicy(DRP::HasPrimitiveCount), indexed> primitiveCount(BlobRequest<T, policy> blob) &&
  requires(!hasPolicy(DRP::HasPrimitiveCount) && !hasPolicy(DRP::HasIndirect) && isValidBlob<T, policy>())
  {
    Base::setBlobArg<ArgType::PrimitiveCount>(blob, detail::identity_projector);
    return {registry, nodeId, shaderId};
  };

  /**
   * \brief Specifies the primitive count of draw request with blob and projector.
   * \tparam projector A function to extract the value from blob.
   *    Can be a pointer-to-member, i.e. `&BlobType::field` or a (pure) function pointer.
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   * \param blob The blob request to be used for the primitive count.
   */
  template <auto projector, typename T, RRP policy>
  DrawRequest<flipPolicy(DRP::HasPrimitiveCount), indexed> primitiveCount(BlobRequest<T, policy> blob) &&
  requires(!hasPolicy(DRP::HasPrimitiveCount) && !hasPolicy(DRP::HasIndirect) && isValidBlobProjector<projector, T, policy>())
  {
    Base::setBlobArg<ArgType::PrimitiveCount>(blob, detail::erase_projector_type<projector, T>());
    return {registry, nodeId, shaderId};
  };

  /**
   * \brief Specifies the start instance of draw request.
   * \param start_instance The start instance.
   */
  DrawRequest<flipPolicy(DRP::HasStartInstance), indexed> startInstance(uint32_t start_instance) &&
  requires(!hasPolicy(DRP::HasStartInstance) && !hasPolicy(DRP::HasIndirect))
  {
    Base::setArg<ArgType::StartInstance>(start_instance);
    return {registry, nodeId, shaderId};
  };

  /**
   * \brief Specifies the start instance of draw request with blob.
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   * \param blob The blob request to be used for the start instance.
   */
  template <typename T, RRP policy>
  DrawRequest<flipPolicy(DRP::HasStartInstance), indexed> startInstance(BlobRequest<T, policy> blob) &&
  requires(!hasPolicy(DRP::HasStartInstance) && !hasPolicy(DRP::HasIndirect) && isValidBlob<T, policy>())
  {
    Base::setBlobArg<ArgType::StartInstance>(blob, detail::identity_projector);
    return {registry, nodeId, shaderId};
  };

  /**
   * \brief Specifies the start instance of draw request with blob and projector.
   * \tparam projector A function to extract the value from blob.
   *    Can be a pointer-to-member, i.e. `&BlobType::field` or a (pure) function pointer.
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   * \param blob The blob request to be used for the start instance.
   */
  template <auto projector, typename T, RRP policy>
  DrawRequest<flipPolicy(DRP::HasStartInstance), indexed> startInstance(BlobRequest<T, policy> blob) &&
  requires(!hasPolicy(DRP::HasStartInstance) && !hasPolicy(DRP::HasIndirect) && isValidBlobProjector<projector, T, policy>())
  {
    Base::setBlobArg<ArgType::StartInstance>(blob, detail::erase_projector_type<projector, T>());
    return {registry, nodeId, shaderId};
  };

  /**
   * \brief Specifies the instance count of draw request.
   * \param instance_count The instance count.
   */
  DrawRequest<flipPolicy(DRP::HasInstanceCount), indexed> instanceCount(uint32_t instance_count) &&
  requires(!hasPolicy(DRP::HasInstanceCount) && !hasPolicy(DRP::HasIndirect))
  {
    Base::setArg<ArgType::InstanceCount>(instance_count);
    return {registry, nodeId, shaderId};
  };

  /**
   * \brief Specifies the instance count of draw request with blob.
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   * \param blob The blob request to be used for the instance count.
   */
  template <typename T, RRP policy>
  DrawRequest<flipPolicy(DRP::HasInstanceCount), indexed> instanceCount(BlobRequest<T, policy> blob) &&
  requires(!hasPolicy(DRP::HasInstanceCount) && !hasPolicy(DRP::HasIndirect) && isValidBlob<T, policy>())
  {
    Base::setBlobArg<ArgType::InstanceCount>(blob, detail::identity_projector);
    return {registry, nodeId, shaderId};
  };

  /**
   * \brief Specifies the instance count of draw request with blob and projector.
   * \tparam projector A function to extract the value from blob.
   *    Can be a pointer-to-member, i.e. `&BlobType::field` or a (pure) function pointer.
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   * \param blob The blob request to be used for the instance count.
   */
  template <auto projector, typename T, RRP policy>
  DrawRequest<flipPolicy(DRP::HasInstanceCount), indexed> instanceCount(BlobRequest<T, policy> blob) &&
  requires(!hasPolicy(DRP::HasInstanceCount) && !hasPolicy(DRP::HasIndirect) && isValidBlobProjector<projector, T, policy>())
  {
    Base::setBlobArg<ArgType::InstanceCount>(blob, detail::erase_projector_type<projector, T>());
    return {registry, nodeId, shaderId};
  };

  /**
   * \brief Specifies the indirect dispatch buffer.
   * \param buffer The name of the buffer to be used for indirect dispatch.
   */
  DrawRequest<flipPolicy(DRP::HasIndirect), indexed> indirect(const char *buffer) &&
  requires(!hasPolicy(DRP::HasIndirect) && !hasPolicy(DRP::HasBaseVertex) && !hasPolicy(DRP::HasStartIndex) && !hasPolicy(DRP::HasInstanceCount) && !hasPolicy(DRP::HasPrimitiveCount))
  {
    Base::indirect(buffer);
    return {registry, nodeId, shaderId};
  };

  /**
   * \brief Specifies the offset of the indirect dispatch buffer.
   * \param offset The offset to be used for indirect dispatch.
   */
  DrawRequest<flipPolicy(DRP::HasOffset), indexed> offset(size_t offset) &&
  requires(!hasPolicy(DRP::HasOffset) && hasPolicy(DRP::HasIndirect))
  {
    Base::setArg<ArgType::Offset>(offset);
    return {registry, nodeId, shaderId};
  };

  /**
   * \brief Specifies the offset of the indirect dispatch buffer with blob.
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   * \param blob The blob request to be used for the offset.
   */
  template <typename T, RRP policy>
  DrawRequest<flipPolicy(DRP::HasOffset), indexed> offset(BlobRequest<T, policy> blob) &&
  requires(!hasPolicy(DRP::HasOffset) && hasPolicy(DRP::HasIndirect) && isValidBlob<T, policy>())
  {
    Base::setBlobArg<ArgType::Offset>(blob, detail::identity_projector);
    return {registry, nodeId, shaderId};
  };

  /**
   * \brief Specifies the offset of the indirect dispatch buffer with blob and projector.
   * \tparam projector A function to extract the value from blob.
   *    Can be a pointer-to-member, i.e. `&BlobType::field` or a (pure) function pointer.
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   * \param blob The blob request to be used for the offset.
   */
  template <auto projector, typename T, RRP policy>
  DrawRequest<flipPolicy(DRP::HasOffset), indexed> offset(BlobRequest<T, policy> blob) &&
  requires(!hasPolicy(DRP::HasOffset) && hasPolicy(DRP::HasIndirect) && isValidBlobProjector<projector, T, policy>())
  {
    Base::setBlobArg<ArgType::Offset>(blob, detail::erase_projector_type<projector, T>());
    return {registry, nodeId, shaderId};
  };

  /**
   * \brief Specifies the stride of the indirect dispatch buffer.
   * \param stride The stride to be used for indirect dispatch.
   */
  DrawRequest<flipPolicy(DRP::HasStride), indexed> stride(uint32_t stride) &&
  requires(!hasPolicy(DRP::HasStride) && hasPolicy(DRP::HasIndirect))
  {
    Base::setArg<ArgType::Stride>(stride);
    return {registry, nodeId, shaderId};
  };

  /**
   * \brief Specifies the stride of the indirect dispatch buffer with blob.
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   * \param blob The blob request to be used for the stride.
   */
  template <typename T, RRP policy>
  DrawRequest<flipPolicy(DRP::HasStride), indexed> stride(BlobRequest<T, policy> blob) &&
  requires(!hasPolicy(DRP::HasStride) && hasPolicy(DRP::HasIndirect) && isValidBlob<T, policy>())
  {
    Base::setBlobArg<ArgType::Stride>(blob, detail::identity_projector);
    return {registry, nodeId, shaderId};
  };

  /**
   * \brief Specifies the stride of the indirect dispatch buffer with blob and projector.
   * \tparam projector A function to extract the value from blob.
   *    Can be a pointer-to-member, i.e. `&BlobType::field` or a (pure) function pointer.
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   * \param blob The blob request to be used for the stride.
   */
  template <auto projector, typename T, RRP policy>
  DrawRequest<flipPolicy(DRP::HasStride), indexed> stride(BlobRequest<T, policy> blob) &&
  requires(!hasPolicy(DRP::HasStride) && hasPolicy(DRP::HasIndirect) && isValidBlobProjector<projector, T, policy>())
  {
    Base::setBlobArg<ArgType::Stride>(blob, detail::erase_projector_type<projector, T>());
    return {registry, nodeId, shaderId};
  };

  /**
   * \brief Specifies the count of the indirect dispatch buffer.
   * \param count The count to be used for indirect dispatch.
   */
  DrawRequest<flipPolicy(DRP::HasCount), indexed> count(uint32_t count) &&
  requires(!hasPolicy(DRP::HasCount) && hasPolicy(DRP::HasIndirect))
  {
    Base::setArg<ArgType::Count>(count);
    return {registry, nodeId, shaderId};
  };

  /**
   * \brief Specifies the count of the indirect dispatch buffer with blob.
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   * \param blob The blob request to be used for the count.
   */
  template <typename T, RRP policy>
  DrawRequest<flipPolicy(DRP::HasCount), indexed> count(BlobRequest<T, policy> blob) &&
  requires(!hasPolicy(DRP::HasCount) && hasPolicy(DRP::HasIndirect) && isValidBlob<T, policy>())
  {
    Base::setBlobArg<ArgType::Count>(blob, detail::identity_projector);
    return {registry, nodeId, shaderId};
  };

  /**
   * \brief Specifies the count of the indirect dispatch buffer with blob and projector.
   * \tparam projector A function to extract the value from blob.
   *    Can be a pointer-to-member, i.e. `&BlobType::field` or a (pure) function pointer.
   * \tparam T The type of the CPU data blob.
   * \tparam policy The policy of the blob request.
   * \param blob The blob request to be used for the count.
   */
  template <auto projector, typename T, RRP policy>
  DrawRequest<flipPolicy(DRP::HasCount), indexed> count(BlobRequest<T, policy> blob) &&
  requires(!hasPolicy(DRP::HasCount) && hasPolicy(DRP::HasIndirect) && isValidBlobProjector<projector, T, policy>())
  {
    Base::setBlobArg<ArgType::Count>(blob, detail::erase_projector_type<projector, T>());
    return {registry, nodeId, shaderId};
  };
};

// clang-format on

} // namespace dafg
