//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <render/daBfg/stateRequest.h>
#include <render/daBfg/virtualPassRequest.h>
#include <render/daBfg/multiplexing.h>
#include <render/daBfg/externalResources.h>
#include <render/daBfg/priority.h>
#include <render/daBfg/sideEffects.h>
#include <render/daBfg/nameSpaceRequest.h>
#include <render/daBfg/virtualResourceCreationSemiRequest.h>


namespace dabfg
{
class NodeTracker;
} // namespace dabfg

namespace dabfg
{

class NodeHandle;

/**
 * \brief The main builder object for describing at declaration time
 * what your node intends to do at runtime.
 */
class Registry final : private NameSpaceRequest
{
  friend class NameSpace;

  Registry(NodeNameId node, InternalRegistry *reg);

public:
  /**
   * \brief Orders the current node before a certain node.
   * This means that the node \p name will only start executing after
   * the current node has finished.
   * \note Ordering with nodes from different name spaces is not supported.
   *
   * \param name The name of the node to order before, looked up in the current namespace.
   */
  Registry orderMeBefore(const char *name);

  /**
   * \brief Alias for calling \ref orderMeBefore(const char *name)
   * several times.
   *
   * \param names List of names of the nodes to order before.
   */
  Registry orderMeBefore(std::initializer_list<const char *> names);

  /**
   * \brief Orders the current node after a certain node.
   * This means that the current node will only start executing after
   * node \p name has finished.
   * \note Ordering with nodes from different name spaces is not supported.
   *
   * \param name The name of the node to order after, looked up in the current namespace.
   */
  Registry orderMeAfter(const char *name);

  /**
   * \brief Alias for calling \ref orderMeAfter(const char *name)
   * several times.
   *
   * \param names List of names of the nodes to order after.
   */
  Registry orderMeAfter(std::initializer_list<const char *> names);

  /**
   * \brief Sets a priority for this node that will be used to order
   * parallel nodes. Should only be used for optimizations.
   *
   * \param prio The priority to set.
   */
  Registry setPriority(priority_t prio);

  /**
   * \brief Choses multiplexing mode for this node. Default is multiplexing over all axes (i.e. normal world rendering).
   *
   * \param mode The multiplexing mode to set.
   */
  Registry multiplex(multiplexing::Mode mode);

  /**
   * \brief Sets the side effect of the node that will control how
   * the framegraph handles execution.
   *
   * \param side_effect The side effect type to set.
   */
  Registry executionHas(SideEffects side_effect);

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
  NameSpaceRequest root();

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
   * \param history
   *   Specifies the way in which history should be handled for this texture.
   *   A history for a virtual resource is a physical resource that
   *   contains the data this virtual resource had at the end of the
   *   previous frame. See dabfg::History for details.
   */
  VirtualResourceCreationSemiRequest create(const char *name, History history);

  /**
   * \brief Creates a new texture at node execution time, which will
   * not be FG-provided, but acquired from the callback before the node
   * starts executing and stored somewhere outside.
   * Note that history is not supported for external resources.
   *
   * \param name A unique name identifying this resource within the current node's namespace.
   * \param texture_provider_callback A callback that maps a multiplexing index to a ManagedTexView.
   */
  template <class F>
  VirtualTextureRequest<NewRwRequestPolicy> registerTexture2d(const char *name, F &&texture_provider_callback)
  {
    auto uid = registerTexture2dImpl(name,
      [impl = eastl::forward<F>(texture_provider_callback)](const multiplexing::Index &mIdx) { return ExternalResource(impl(mIdx)); });
    return {uid, nodeId, registry};
  }

  using NameSpaceRequest::historyFor;
  using NameSpaceRequest::modify;
  using NameSpaceRequest::read;
  using NameSpaceRequest::rename;

  ///@}


  /// \name Convenience aliases
  ///@{

  /// \brief Alias. See \ref create function for details.
  VirtualTextureRequest<NewRwRequestPolicy> createTexture2d(const char *name, History history, Texture2dCreateInfo info)
  {
    return create(name, history).texture(info);
  }

  /// \brief Alias. See \ref create function for details.
  template <class T>
  VirtualBlobRequest<T, NewRwRequestPolicy> createBlob(const char *name, History history)
  {
    return create(name, history).blob<T>();
  }

  using NameSpaceRequest::modifyBlob;
  using NameSpaceRequest::modifyTexture;
  using NameSpaceRequest::readBlob;
  using NameSpaceRequest::readBlobHistory;
  using NameSpaceRequest::readTexture;
  using NameSpaceRequest::readTextureHistory;
  using NameSpaceRequest::renameBlob;
  using NameSpaceRequest::renameTexture;

  ///@}

private:
  detail::ResUid registerTexture2dImpl(const char *name, dabfg::ExternalResourceProvider &&p);
};

} // namespace dabfg
