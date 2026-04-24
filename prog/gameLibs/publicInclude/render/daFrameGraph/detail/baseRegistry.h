//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/daFrameGraph/stateRequest.h>
#include <render/daFrameGraph/virtualPassRequest.h>
#include <render/daFrameGraph/multiplexing.h>
#include <render/daFrameGraph/externalResources.h>
#include <render/daFrameGraph/priority.h>
#include <render/daFrameGraph/sideEffects.h>
#include <render/daFrameGraph/nameSpaceRequest.h>
#include <render/daFrameGraph/virtualResourceCreationSemiRequest.h>
#include <render/daFrameGraph/virtualResourceRegistrationSemiRequest.h>
#include <render/daFrameGraph/dispatchRequest.h>
#include <render/daFrameGraph/primitive.h>
#include <render/daFrameGraph/drawRequest.h>


namespace dafg
{
class NodeTracker;
} // namespace dafg

namespace dafg
{

class NodeHandle;
class Registry;

/**
 * \brief The main builder object for describing at declaration time
 * what your node intends to do at runtime.
 */
class BaseRegistry : private NameSpaceRequest
{
  friend class NameSpace;

  static constexpr RRP NewCreateRequestPolicy = RRP::None;
  static constexpr RRP TexCreateRequestPolicy = NewCreateRequestPolicy | RRP::CanSpecifyHistory;
  static constexpr RRP NonTexCreateRequestPolicy = NewRwRequestPolicy | RRP::CanSpecifyHistory;

protected:
  BaseRegistry(NodeNameId node, InternalRegistry *reg);

public:
  /**
   * \brief Allows async pipeline compilation
   * Graphics pipelines will be async compiled with draw call skip when pipeline is not yet ready
   */
  BaseRegistry allowAsyncPipelines();

  /**
   * \brief Orders the current node before a certain node.
   * This means that the node \p name will only start executing after
   * the current node has finished.
   * \note Ordering with nodes from different name spaces is not supported.
   *
   * \param name The name of the node to order before, looked up in the current namespace.
   */
  BaseRegistry orderMeBefore(const char *name);

  /**
   * \brief Alias for calling \ref orderMeBefore(const char *name)
   * several times.
   *
   * \param names List of names of the nodes to order before.
   */
  BaseRegistry orderMeBefore(std::initializer_list<const char *> names);

  /**
   * \brief Orders the current node after a certain node.
   * This means that the current node will only start executing after
   * node \p name has finished.
   * \note Ordering with nodes from different name spaces is not supported.
   *
   * \param name The name of the node to order after, looked up in the current namespace.
   */
  BaseRegistry orderMeAfter(const char *name);

  /**
   * \brief Alias for calling \ref orderMeAfter(const char *name)
   * several times.
   *
   * \param names List of names of the nodes to order after.
   */
  BaseRegistry orderMeAfter(std::initializer_list<const char *> names);

  /**
   * \brief Sets a priority for this node that will be used to order
   * parallel nodes. Should only be used for optimizations.
   *
   * \param prio The priority to set.
   */
  BaseRegistry setPriority(priority_t prio);

  /**
   * \brief Choses multiplexing mode for this node. Default is multiplexing over all axes (i.e. normal world rendering).
   *
   * \param mode The multiplexing mode to set.
   */
  BaseRegistry multiplex(multiplexing::Mode mode);

  /**
   * \brief Sets the side effect of the node that will control how
   * the framegraph handles execution.
   *
   * \param side_effect The side effect type to set.
   */
  BaseRegistry executionHas(SideEffects side_effect);

  /**
   * \brief Requests a certain global state for the execution time of this node.
   *
   * \return A builder object for specifying concrete states.
   */
  StateRequest requestState();

  /**
   * \brief Requests that this node is going to be drawing stuff, which is always
   * done inside a (possibly implicit) render pass. The returned object
   * allows fine-tuning the render pass.
   *
   * \return A builder object for specifying attachments and other details of the render pass.
   */
  VirtualPassRequest requestRenderPass();

  /**
   * \brief Requests a node to execute draw with shader.
   * \param shader_name The name of the shader to be used for drawing.
   * \param primitive The primitive type to be drawn.
   * \return A builder object for specifying the draw parameters.
   */
  DrawRequest<detail::DrawRequestPolicy::Default, false> draw(const char *shader_name, dafg::DrawPrimitive primitive);

  /**
   * \brief Requests a node to execute indexed draw with shader.
   * \param shader_name The name of the shader to be used for drawing.
   * \param primitive The primitive type to be drawn.
   * \return A builder object for specifying the indexed draw parameters.
   */
  DrawRequest<detail::DrawRequestPolicy::Default, true> drawIndexed(const char *shader_name, dafg::DrawPrimitive primitive);

  /**
   * \brief Requests a node to execute dispatch compute threads with shader.
   * \param shader_name The name of the shader to be used for dispatch.
   * \return A builder object for specifying the dispatch parameters.
   */
  DispatchComputeThreadsRequest dispatchThreads(const char *shader_name);

  /**
   * \brief Requests a node to dispatch compute thread groups with shader.
   * \param shader_name The name of the shader to be used for dispatch.
   * \return A builder object for specifying the dispatch parameters.
   */
  DispatchComputeGroupsRequest dispatchGroups(const char *shader_name);

  /**
   * \brief Requests a node to dispatch compute indirect with shader.
   * \param shader_name The name of the shader to be used for dispatch.
   * \param buffer The name of the buffer to be used for indirect dispatch.
   * \return A builder object for specifying the dispatch parameters.
   */
  DispatchComputeIndirectRequest dispatchIndirect(const char *shader_name, const char *buffer);

  /**
   * \brief Requests a node to dispatch mesh threads with shader.
   * \param shader_name The name of the shader to be used for dispatch.
   * \return A builder object for specifying the dispatch parameters.
   */
  DispatchMeshThreadsRequest dispatchMeshThreads(const char *shader_name);

  /**
   * \brief Requests a node to dispatch mesh thread groups with shader.
   * \param shader_name The name of the shader to be used for dispatch.
   * \return A builder object for specifying the dispatch parameters.
   */
  DispatchMeshGroupsRequest dispatchMeshGroups(const char *shader_name);

  /**
   * \brief Requests a node to dispatch mesh indirect with shader.
   * \param shader_name The name of the shader to be used for dispatch.
   * \param buffer The name of the buffer to be used for indirect dispatch.
   * \return A builder object for specifying the dispatch parameters.
   */
  DispatchMeshIndirectRequest dispatchMeshIndirect(const char *shader_name, const char *buffer);

  /**
   * \brief Returns a request object for this node's name space that can
   * be used to get sub-namespace request objects or request resources.
   *
   * \return NameSpaceRequest object representing the current name space.
   */
  NameSpaceRequest currNameSpace() { return *this; }

  /**
   * \brief Returns a request object for the root name space which can
   * be used to access global resources.
   *
   * \return NameSpaceRequest object representing the root name space.
   */
  NameSpaceRequest root() const;

  using NameSpaceRequest::getResolution;

  /** \name Resource requesting methods
   * Every one of these returns a request object that must be used to specify
   * further options for the request. See docs for the relevant request objects
   * for details.
   * \note All methods present here look up the resource in the current
   * node's namespace, i.e. registry.currNameSpace().
   * \note We explicitly prohibit creating resources in any namespace
   * but the current node's one.
   */
  ///@{

  /**
   * Creates a new resource at node execution time.
   * The resource will be provided by FG before the node starts executing.
   *
   * \param name A unique name identifying this resource within the current node's namespace.
   */
  VirtualResourceCreationSemiRequest create(const char *name);

  /**
   * Creates a new resource at node execution time, which will
   * not be FG-provided, but acquired before the node
   * starts executing and stored somewhere outside.
   * Note that history is not supported for external resources.
   *
   * \param name A unique name identifying this resource within the current node's namespace.
   */
  VirtualResourceRegistrationSemiRequest registerExternal(const char *name);

  /**
   * \brief Creates a new texture at node execution time, which will
   * not be FG-provided, but acquired from the callback before the node
   * starts executing and stored somewhere outside.
   * Note that history is not supported for external resources.
   *
   * \param name A unique name identifying this resource within the current node's namespace.
   * \param texture_provider_callback
   *   A callback that maps a multiplexing index to a ManagedTexView. It might be called
   *   an arbitrary number of times, but only while the current node is registered.
   */
  template <class F>
  VirtualTextureRequest<NewRwRequestPolicy> registerTexture(const char *name, F &&texture_provider_callback)
  {
    return registerExternal(name).texture(
      [impl = eastl::forward<F>(texture_provider_callback)](const multiplexing::Index &mIdx) { return ExternalResource(impl(mIdx)); });
  }

  /**
   * \brief Marks this node as the back buffer provider and creates a
   * virtual dafg resource that corresponds to it.
   * \details The back buffer is not a real resource in dagor, but
   * rather a special value that will be replaced with the actual
   * swapchain image when it's acquired on the driver thread.
   * This makes the back buffer a special case everywhere, it is not
   * possible to read from it, nor is it possible to UAV-write to it.
   * In daFG, we only to render to it, and only with a single attachment
   * at that, so no MRT and no depth-stencil even.
   * \note This is a hot discussion topic and might change in the future.
   *
   * \param name a unique name identifying the back buffer as a resource within the current node's namespace.
   */
  VirtualTextureRequest<NewRwRequestPolicy> registerBackBuffer(const char *name) { return registerExternal(name).backBuffer(); }

  /**
   * \brief Creates a new buffer at node execution time, which will
   * not be FG-provided, but acquired from the callback before the node
   * starts executing and stored somewhere outside.
   * Note that history is not supported for external resources.
   *
   * \param name A unique name identifying this resource within the current node's namespace.
   * \param buffer_provider_callback
   *   A callback that maps a multiplexing index to a ManagedBufferView. It might be called
   *   an arbitrary number of times, but only while the current node is registered.
   */
  template <class F>
  VirtualBufferRequest<NewRwRequestPolicy> registerBuffer(const char *name, F &&buffer_provider_callback)
  {
    return registerExternal(name).buffer(
      [impl = eastl::forward<F>(buffer_provider_callback)](const multiplexing::Index &mIdx) { return ExternalResource(impl(mIdx)); });
  }

  using NameSpaceRequest::historyFor;
  using NameSpaceRequest::modify;
  using NameSpaceRequest::read;
  using NameSpaceRequest::rename;

  ///@}


  /// \name Convenience aliases
  ///@{

  using NameSpaceRequest::modifyBlob;
  using NameSpaceRequest::modifyTexture;
  using NameSpaceRequest::readBlob;
  using NameSpaceRequest::readBlobHistory;
  using NameSpaceRequest::readTexture;
  using NameSpaceRequest::readTextureHistory;
  using NameSpaceRequest::renameBlob;
  using NameSpaceRequest::renameTexture;

  ///@}
};

} // namespace dafg
