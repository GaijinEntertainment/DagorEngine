//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/type_traits.h>

#include <render/daBfg/usage.h>
#include <render/daBfg/stage.h>
#include <render/daBfg/virtualResourceHandle.h>
#include <render/daBfg/detail/virtualResourceRequestBase.h>
#include <render/daBfg/detail/resourceRequestPolicy.h>


namespace dabfg
{

namespace detail
{
template <detail::ResourceRequestPolicy, detail::ResourceRequestPolicy>
class VirtualAttachmentRequest;
} // namespace detail

#ifndef DOXYGEN
// TODO: Replace this hackyness with constraints as soon as C++20 drops
#define REQUIRE_IMPL(...) class Dummy1 = void, class = typename eastl::enable_if<(__VA_ARGS__), Dummy1>::type
#define REQUIRE(...)      template <REQUIRE_IMPL(__VA_ARGS__)>
#else
#define REQUIRE_IMPL(...) class = void
#define REQUIRE(...)
#endif

/**
 * \brief Request for a virtual resource known to BFG.
 * Should be used to further configure the properties of the request.
 * \details The intended usage is
 *
 *     auto myTexHandle = registry.readTexture("my_tex")
 *       .optional()
 *       ...
 *       .handle();
 *
 * Hence methods are only rvalue-callable. Don't try to get smart with
 * these request classes. Don't cache them, don't pass them around a
 * bunch of functions, KISS.
 *
 * \tparam Res The C++ type representing this resource.
 *   BaseTex/Sbuffer for GPU resources, any C++ type for CPU resources.
 * \tparam policy Static data about this request's structure used for
 *   erroring out at compile time if this class is used incorrectly.
 */
template <class Res, detail::ResourceRequestPolicy policy>
class VirtualResourceRequest
  /// \cond DETAIL
  : private detail::VirtualResourceRequestBase
/// \endcond
{
  static constexpr bool is_gpu = eastl::is_base_of_v<D3dResource, Res>;

  using Base = detail::VirtualResourceRequestBase;

  template <detail::ResourceRequestPolicy, detail::ResourceRequestPolicy>
  friend class detail::VirtualAttachmentRequest;

  template <class, detail::ResourceRequestPolicy>
  friend class VirtualResourceRequest;

  friend class VirtualResourceCreationSemiRequest;

  template <detail::ResourceRequestPolicy>
  friend class VirtualResourceSemiRequest;
  friend class Registry;
  VirtualResourceRequest(detail::ResUid resId, NodeNameId node, InternalRegistry *reg) : Base{resId, node, reg} {}

  using RRP = detail::ResourceRequestPolicy;

  static constexpr bool hasPolicy(RRP p) { return (policy & p) == p; }

  static constexpr bool hasPolicy(RRP p, const char *) { return hasPolicy(p); }

  static constexpr RRP flipPolicy(RRP p) { return static_cast<RRP>(eastl::to_underlying(policy) ^ eastl::to_underlying(p)); }

  using HandleType =
    VirtualResourceHandle<eastl::conditional_t<hasPolicy(RRP::Readonly), const Res, Res>, is_gpu, hasPolicy(RRP::Optional)>;

public:
  /**
   * \brief Makes this request optional. The node will not get disabled
   * if the resource is missing, the handle will return an empty value.
   */
  REQUIRE(!hasPolicy(RRP::Optional, "ERROR: Please, remove the duplicate .optional() call!"))
  VirtualResourceRequest<Res, flipPolicy(RRP::Optional)> optional() &&
  {
    Base::optional();
    return {resUid, nodeId, registry};
  }

  /**
   * \brief Requests for this resource to be bound to a shader variable
   * with the specified name.
   * The usage stage MUST have been set beforehand and MUST include all
   * shader stages that this shader var will be accessed from
   * within this node.
   *
   * \param shader_var_name The name of the shader variable to bind.
   *   If not specified, the name of the resource will be used.
   */
  REQUIRE(!is_gpu || (hasPolicy(RRP::HasUsageStage, "ERROR: Please, call .atStage(usage stage) before .bindToShaderVar(...)!") &&
                       !hasPolicy(RRP::HasUsageType, "ERROR: Please, don't call .useAs(usage type) before .bindToShaderVar(...)!")))
  VirtualResourceRequest<Res, flipPolicy(RRP::HasUsageType)> bindToShaderVar(const char *shader_var_name = nullptr) &&
  {
    Base::bindToShaderVar(shader_var_name, tag_for<Res>());
    return {resUid, nodeId, registry};
  }

  /**
   * \brief Requests for a sub-object of a blob to be bound to a shader
   * variable with the specified name.
   *
   * \tparam projector A function to extract the value to bind from
   *   the blob. Can be a pointer-to-member, i.e. `&BlobType::field`.
   * \param shader_var_name The name of the shader variable to bind.
   *   If not specified, the name of the resource will be used.
   */
  template <auto projector, REQUIRE_IMPL(!is_gpu && eastl::is_invocable_v<decltype(projector), Res>)>
  VirtualResourceRequest<Res, policy> bindToShaderVar(const char *shader_var_name = nullptr) &&
  {
    using InvokeResult = eastl::invoke_result_t<decltype(projector), const Res &>;
    static_assert(detail::is_const_lvalue_reference<InvokeResult>, "Invoking a projector must return a const lvalue reference!");
    using ProjectedType = eastl::remove_cvref_t<InvokeResult>;
    Base::bindToShaderVar(shader_var_name, tag_for<ProjectedType>(), detail::erase_projector_type<projector, Res>());
    return {resUid, nodeId, registry};
  }

  /**
   * \brief Requests for a matrix-like blob to be bound as the current
   * view matrix.
   */
  REQUIRE(!is_gpu)
  VirtualResourceRequest<Res, policy> bindAsView() &&
  {
    Base::bindAsView(tag_for<Res>());
    return {resUid, nodeId, registry};
  }

  /**
   * \brief Requests for a matrix-like sub-object of a blob to be bound
   * as the current view matrix.
   *
   * \tparam projector A function to extract the value to bind from
   *   the blob. Can be a pointer-to-member, i.e. `&BlobType::field`.
   */
  template <auto projector, REQUIRE_IMPL(!is_gpu && eastl::is_invocable_v<decltype(projector), Res>)>
  VirtualResourceRequest<Res, policy> bindAsView() &&
  {
    using InvokeResult = eastl::invoke_result_t<decltype(projector), const Res &>;
    static_assert(detail::is_const_lvalue_reference<InvokeResult>, "Invoking a projector must return a const lvalue reference!");
    using ProjectedType = eastl::remove_cvref_t<InvokeResult>;
    Base::bindAsView(tag_for<ProjectedType>(), detail::erase_projector_type<projector, Res>());
    return {resUid, nodeId, registry};
  }

  /**
   * \brief Requests for a matrix-like blob to be bound as the current
   * projection matrix.
   */
  REQUIRE(!is_gpu)
  VirtualResourceRequest<Res, policy> bindAsProj() &&
  {
    Base::bindAsProj(tag_for<Res>());
    return {resUid, nodeId, registry};
  }

  /**
   * \brief Requests for a matrix-like sub-object of a blob to be bound
   * as the current projection matrix.
   *
   * \tparam projector A function to extract the value to bind from
   *   the blob. Can be a pointer-to-member, i.e. `&BlobType::field`.
   */
  template <auto projector, REQUIRE_IMPL(!is_gpu && eastl::is_invocable_v<decltype(projector), const Res &>)>
  VirtualResourceRequest<Res, policy> bindAsProj() &&
  {
    using InvokeResult = eastl::invoke_result_t<decltype(projector), const Res &>;
    static_assert(detail::is_const_lvalue_reference<InvokeResult>, "Invoking a projector must return a const lvalue reference!");
    using ProjectedType = eastl::remove_cvref_t<InvokeResult>;
    Base::bindAsProj(tag_for<ProjectedType>(), detail::erase_projector_type<projector, Res>());
    return {resUid, nodeId, registry};
  }

  /**
   * \brief Must be used when the resource is manually accessed through
   * the handle or bound to a shader var to specify what shader stage
   * this resource will be used at.
   */
  REQUIRE(is_gpu && !hasPolicy(RRP::HasUsageStage, "ERROR: Please, remove the duplicate .atStage(usage stage) call!"))
  VirtualResourceRequest<Res, flipPolicy(RRP::HasUsageStage)> atStage(Stage stage) &&
  {
    Base::atStage(stage);
    return {resUid, nodeId, registry};
  }

  /**
   * \brief Must be used when the resource is manually accessed through
   * the handle to specify how exactly the resource will be used.
   */
  REQUIRE(is_gpu && !hasPolicy(RRP::HasUsageType, "ERROR: Please, remove the duplicate .useAs(usage type) call!"))
  VirtualResourceRequest<Res, flipPolicy(RRP::HasUsageType)> useAs(Usage type) &&
  {
    Base::useAs(type);
    return {resUid, nodeId, registry};
  }

  /**
   * \brief Returns a handle that can be used to access the physical
   * resource underlying this virtual one during execution time. Note
   * that the physical resource will change throughout executions and
   * should never be cached by reference.
   * See \ref dabfg::VirtualResourceHandle for further details.
   */
  REQUIRE(!is_gpu || (hasPolicy(RRP::HasUsageStage, "ERROR: Please, call .atStage(usage stage) before .handle()!") &&
                       hasPolicy(RRP::HasUsageType, "ERROR: Please, call .useAs(usage type) before .handle()!")))
  HandleType handle() && { return {resUid, Base::provider()}; }
};

#undef REQUIRE_IMPL
#undef REQUIRE

} // namespace dabfg
