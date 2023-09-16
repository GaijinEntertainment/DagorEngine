//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/initializer_list.h>
#include <EASTL/optional.h>

#include <math/integer/dag_IPoint2.h>

#include <render/daBfg/virtualResourceRequest.h>
#include <render/daBfg/detail/access.h>
#include <3d/dag_tex3d.h>


namespace dabfg
{

struct InternalRegistry;
class VirtualPassRequest;

namespace detail
{

/**
 * \brief A helper class to make calling .color() and .depth() convenient.
 * \details This class wraps around either a request or a string name
 * of a resource, validating that the request is appropriate in the
 * first case and automatically selecting sensible request defaults in
 * the second case.
 * Generally speaking, we can use certain mips/layers as attachments,
 * so an attachment request can specify them too.
 *
 * \tparam disallowPolicies Bitmask of request policies that are not
 *   allowed for this attachment type.
 * \tparam requirePolicies Bitmask of request policies that are
 *   required for this attachment type.
 */
template <detail::ResourceRequestPolicy disallowPolicies, detail::ResourceRequestPolicy requirePolicies>
class VirtualAttachmentRequest
{
  using RRP = detail::ResourceRequestPolicy;

  static constexpr bool policyAllowed(RRP policy)
  {
    return (disallowPolicies & policy) == RRP::None && (requirePolicies & policy) == requirePolicies;
  }

  // Trick for making clang print custom strings in error messages.
  static constexpr bool policyAllowed(RRP policy, const char *) { return policyAllowed(policy); }

  friend class dabfg::VirtualPassRequest;

public:
#ifndef DOXYGEN
#define ENABLE_IF(...) class Dummy = void, class = eastl::enable_if_t<(__VA_ARGS__), Dummy>
#else
#define ENABLE_IF(...) class = void
#endif

  /**
   * \brief Implicit conversion from a resource request.
   *
   * \param res Resource request of an appropriate type.
   */
  template <RRP policy, ENABLE_IF(policyAllowed(policy, "ERROR: Invalid resource request for this type of attachment!"
                                                        " See the docs for the function you are trying to call!"))>
  VirtualAttachmentRequest(VirtualResourceRequest<BaseTexture, policy> res) : image{res.resUid}
  {}

  /**
   * \brief Implicit conversion from an init list specifying a request,
   * a mip level and a layer.
   *
   * \param res Resource request of an appropriate type.
   * \param mip_level Mip level of the texture to use as an attachment.
   * \param layer Layer of an array texture to use as an attachment.
   */
  template <RRP policy, ENABLE_IF(policyAllowed(policy, "ERROR: Invalid resource request for this type of attachment!"
                                                        " See the docs for the function you are trying to call!"))>
  VirtualAttachmentRequest(VirtualResourceRequest<BaseTexture, policy> res, uint32_t mip_level, uint32_t layer) :
    image{res.resUid}, mipLevel{mip_level}, layer{layer}
  {}

  /**
   * \brief Implicit conversion from a string name of a resource.
   * \details Automatically requests this resource for modification and marks
   * appropriate usage type and stage for it.
   *
   * \param name String name of a resource.
   */
  template <ENABLE_IF(requirePolicies == RRP::None)>
  VirtualAttachmentRequest(const char *name) : image{name}
  {}

  /**
   * \brief Implicit conversion from an init list specifying a string
   * name of a resource, a mip level and a layer.
   * \details Automatically requests this resource for modification and marks
   * appropriate usage type and stage for it.
   *
   * \param name String name of a resource.
   * \param mip_level Mip level of the texture to use as an attachment.
   * \param layer Level of an array texture to use as an attachment.
   */
  template <ENABLE_IF(requirePolicies == RRP::None)>
  VirtualAttachmentRequest(const char *name, uint32_t mip_level, uint32_t layer) : image{name}, mipLevel{mip_level}, layer{layer}
  {}

#undef ENABLE_IF

private:
  eastl::variant<ResUid, const char *> image;
  uint32_t mipLevel = 0;
  uint32_t layer = 0;
};

} // namespace detail

/**
 * \brief Requests this node to be inside a render pass with specified
 * attachments. It is a virtual pass, as FG will merge these virtual
 * passes into a single physical pass where possible. This class must
 * be used to further specify the details of a virtual pass.
 * \details Attachments is a term that includes both render targets, depth
 * targets and some other mobile-specific stuff. Note that methods that
 * specify attachments take a special helper class,
 * detail::VirtualAttachmentRequest. It is not necessary to understand
 * how this class works, just call these methods either with a resource
 * request or a string resource name, or even a {request/name, mip, layer}
 * initializer list for tricky cases like cube maps.
 */
class VirtualPassRequest
{
  friend class Registry;
  VirtualPassRequest(NodeNameId node, InternalRegistry *reg);

  using RRP = detail::ResourceRequestPolicy;

public:
  /**
   * \brief Resolve and RW depth cannot be readonly, optional or
   * history requests. Usage should not have been specified yet.
   */
  using RwVirtualAttachmentRequest =
    detail::VirtualAttachmentRequest<RRP::Readonly | RRP::Optional | RRP::History | RRP::HasUsageStage | RRP::HasUsageType, RRP::None>;
  /**
   * \brief Color requests may be optional, but cannot be
   * readonly or history. Usage is specified later.
   */
  using ColorRwVirtualAttachmentRequest =
    detail::VirtualAttachmentRequest<RRP::Readonly | RRP::History | RRP::HasUsageStage | RRP::HasUsageType, RRP::None>;
  /**
   * \brief RO depth is not allowed to be optional, as it doesn't seem useful
   * (may be changed later). Usage will be determined by us.
   */
  using DepthRoVirtualAttachmentRequest =
    detail::VirtualAttachmentRequest<RRP::Optional | RRP::HasUsageStage | RRP::HasUsageType, RRP::None>;
  /**
   * \brief For RO depth that is also sampled through a shader var (SV),
   * we also prohibit optional requests and while the usage type is
   * determined by us (simultaneous depth+sampling), stage should
   * be provided separately.
   */
  using DepthRoAndSvBindVirtualAttachmentRequest = detail::VirtualAttachmentRequest<RRP::Optional | RRP::HasUsageType, RRP::None>;
  /**
   * \brief Input attachments are not allowed to be history, as renderpasses
   * cannot span frame boundaries. Usage is determined by us.
   */
  using RoVirtualAttachmentRequest =
    detail::VirtualAttachmentRequest<RRP::History | RRP::HasUsageStage | RRP::HasUsageType, RRP::None>;

  /**
   * \brief Specifies color attachments for this virtual pass, which
   * will be bound in the order specified here. A color attachment is a
   * synonym for "render target".
   * Call this with a braced list of either request objects or resource
   * names, and optionally mip/layer numbers grouped with an object or
   * name using braces.
   * E.g. `{{"cube", 0, 1}, "normals", prevFrameMotionRequest}`
   *
   * \param attachments A list of attachments to use as color ones.
   *   This method should be usable without understanding the internals
   *   of detail::VirtualAttachmentRequest most of the time.
   */
  VirtualPassRequest color(std::initializer_list<ColorRwVirtualAttachmentRequest> attachments) &&;

  /**
   * \brief Specifies the depth attachment for this virtual pass and
   * enables depth write. The specified resource request MUST
   * be a modify request.
   * Call this with either a resource request object or an object name.
   * You can also optionally specify mip/layer using braces:
   * `{"cube", 0, 1}`.
   *
   * \param attachment The attachment to use as depth.
   *   This method should be usable without understanding the internals
   *   of detail::VirtualAttachmentRequest most of the time.
   */
  VirtualPassRequest depthRw(RwVirtualAttachmentRequest attachment) &&;

  // DEPRECATED AND POINTLESS, DON'T USE
  VirtualPassRequest depthRo(DepthRoVirtualAttachmentRequest attachment) &&;

  /**
   * \brief Specifies the depth attachment for this virtual pass,
   * disables depth write and binds it to several shader variable for
   * sampling inside a shader.
   *
   * \details This can only be called with a resource request object
   * and not a string name, due to requiring `stage` to be specified as
   * all stages where these shader vars will be used.
   *
   * Note however that the resource request may be either read or
   * modify, which will determine the ordering for this node.
   *
   * This is is only multi-usage API in daBfg, an exception to the one
   * usage per node guideline.
   *
   * Warning: switching between RO and RW depth requires decompression
   * and recompression on AMD hardware, which is expensive.
   *
   * \param attachment The resource request for the depth attachment.
   * \param shader_var_names A list of shader var names to be bound.
   */
  VirtualPassRequest depthRoAndBindToShaderVars(DepthRoAndSvBindVirtualAttachmentRequest attachment,
    std::initializer_list<const char *> shader_var_names) &&;

  // NOT IMPLEMENTED
  VirtualPassRequest clear(RwVirtualAttachmentRequest attachment, ResourceClearValue color) &&;

  /* NOT IMPLEMENTED
   * \brief Sets a custom render area for this pass, also known as a
   * scissor rect. By default, it will be the full area of all targets.
   * If the resolutions of targets don't match, a runtime error will
   * be produced.
   * \details Warning: changing the rending are breaks a render pass!
   *
   * \param from The top-left corner of the area.
   * \param to The bottom-right corner of the area.
   */
  VirtualPassRequest area(IPoint2 from, IPoint2 to) &&;

  // TODO: implement this stuff for mobile (huge task)
  // VirtualPassRequest input(std::initializer_list<RoAttachmentRequest> attachments) &&;
  // VirtualPassRequest resolve(std::initializer_list<RwAttachmentRequest> attachments) &&;
  // VirtualPassRequest preserve(std::initializer_list<RoAttachmentRequest> attachments) &&;

private:
  template <RRP disallowPolicies, RRP requirePolicies>
  detail::ResUid processAttachment(detail::VirtualAttachmentRequest<disallowPolicies, requirePolicies> attachment,
    Access accessOverride);

  NodeNameId nodeId;
  InternalRegistry *registry;
};

} // namespace dabfg
