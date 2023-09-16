//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/fixed_function.h>
#include <render/daBfg/virtualResourceSemiRequest.h>
#include <render/daBfg/virtualResourceCreationSemiRequest.h>
#include <render/daBfg/stateRequest.h>
#include <render/daBfg/virtualPassRequest.h>
#include <render/daBfg/multiplexing.h>
#include <render/daBfg/externalResources.h>
#include <render/daBfg/priority.h>
#include <render/daBfg/sideEffects.h>
#include <render/daBfg/history.h>


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
class Registry
{
  using RRP = detail::ResourceRequestPolicy;

  static constexpr RRP NewRwRequestPolicy = RRP::None;
  static constexpr RRP NewRoRequestPolicy = RRP::Readonly;
  static constexpr RRP NewHistRequestPolicy = RRP::Readonly | RRP::History;


  template <RRP policy>
  using VirtualTextureRequest = VirtualResourceRequest<BaseTexture, policy>;

  template <class T, RRP policy>
  using VirtualBlobRequest = VirtualResourceRequest<T, policy>;

  template <class F>
  friend NodeHandle register_node(const char *name, const char *source_location, F &&declaration_callback);

  Registry(NodeNameId node, InternalRegistry *reg);

public:
  /**
   * \brief Orders the current node before a certain node.
   * This means that the node \p name will only start executing after
   * the current node has finished.
   *
   * \param name The name of the node to order before.
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
   *
   * \param name The name of the node to order after.
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

  /** \name Resource requesting methods
   * Every one of these returns a request object that must be used to specify
   * further options for the request. See docs for the relevant request objects
   * for details.
   */
  ///@{

  /**
   * Creates a new resource at node execution time.
   * The resource will be provided by FG before the node starts executing.
   *
   * \param name A unique name identifying this resource.
   * \param history
   *   Specifies the way in which history should be handled for this texture.
   *   A history for a virtual resource is a physical resource that
   *   contains the data this virtual resource had at the end of the
   *   previous frame. See dabfg::History for details.
   */
  VirtualResourceCreationSemiRequest create(const char *name, History history);

  /**
   * \brief Reads an existing resource at node execution time.
   * Reads always happen after all modifications and before the renaming.
   *
   * \param name The name of the resource to read.
   */
  VirtualResourceSemiRequest<NewRoRequestPolicy> read(const char *name);

  /**
   * \brief Reads an existing resource at node execution time, but indirectly,
   * through a "slot", which must be specified with dabfg::fill_slot.
   * Reads always happen after all modifications and before the renaming.
   *
   * \param slot_name The name of the slot to be used for this read.
   */
  VirtualResourceSemiRequest<NewRoRequestPolicy> read(NamedSlot slot_name);

  /**
   * \brief Reads the history of an existing resource at node execution time.
   *
   * \param name The name of the resource the history of which to read.
   */
  VirtualResourceSemiRequest<NewHistRequestPolicy> historyFor(const char *name);

  /**
   * \brief Modifies an existing resource at node execution time.
   * Modifications always happen after creation and before all reads.
   *
   * \param name The name of the resource to modify.
   */
  VirtualResourceSemiRequest<NewRwRequestPolicy> modify(const char *name);

  /**
   * \brief Modifies an existing resource at node execution time, but indirectly,
   * through a "slot", which must be specified with dabfg::fill_slot.
   * Modifications always happen after creation and before all reads.
   *
   * \param slot_name The name of the slot to be used for this modification.
   */
  VirtualResourceSemiRequest<NewRwRequestPolicy> modify(NamedSlot slot_name);

  /**
   * \brief Modifies and renames an existing resource at node execution time.
   * Renaming always happens last among all operations on a resource.
   * The renamed version is considered to be a new virtual resource.
   *
   * \param from Old resource name. Note that this resource may not
   *   have history enabled, as renaming it "consumes" it on the
   *   current frame, so it is not possible to read it on the next one
   * \param to New resource name.
   * \param history Whether the new resource needs history enabled,
   *   see \ref create for details about history.
   */
  VirtualResourceSemiRequest<NewRwRequestPolicy> rename(const char *from, const char *to, History history);

  /**
   * \brief Creates a new texture at node execution time, which will
   * not be FG-provided, but acquired from the callback before the node
   * starts executing and stored somewhere outside.
   * Note that history is not supported for external resources.
   *
   * \param name A unique name identifying this resource.
   * \param texture_provider_callback A callback that maps a multiplexing index to a ManagedTexView.
   */
  template <class F>
  VirtualTextureRequest<NewRwRequestPolicy> registerTexture2d(const char *name, F &&texture_provider_callback)
  {
    auto uid = registerTexture2dImpl(name,
      [impl = eastl::forward<F>(texture_provider_callback)](const multiplexing::Index &mIdx) { return ExternalResource(impl(mIdx)); });
    return {uid, nodeId, registry};
  }

  ///@}

  /// \name Convenience aliases
  ///@{

  /// \brief Alias. See \ref create function for details.
  VirtualTextureRequest<NewRwRequestPolicy> createTexture2d(const char *name, History history, Texture2dCreateInfo info)
  {
    return create(name, history).texture(info);
  }

  /// \brief Alias. See \ref read functions for details.
  VirtualTextureRequest<NewRoRequestPolicy> readTexture(const char *name) { return read(name).texture(); }

  /// \brief Alias. See \ref read functions for details.
  VirtualTextureRequest<NewRoRequestPolicy> readTexture(NamedSlot slot_name) { return read(slot_name).texture(); }

  /// \brief Alias. See \ref historyFor function for details.
  VirtualTextureRequest<NewHistRequestPolicy> readTextureHistory(const char *name) { return historyFor(name).texture(); }

  /// \brief Alias. See \ref modify function for details.
  VirtualTextureRequest<NewRwRequestPolicy> modifyTexture(const char *name) { return modify(name).texture(); }

  /// \brief Alias. See \ref modify function for details.
  VirtualTextureRequest<NewRwRequestPolicy> modifyTexture(NamedSlot slot_name) { return modify(slot_name).texture(); }

  /// \brief Alias. See \ref rename function for details.
  VirtualTextureRequest<NewRwRequestPolicy> renameTexture(const char *from, const char *to, History history)
  {
    return rename(from, to, history).texture();
  }

  /// \brief Alias. See \ref create function for details.
  template <class T>
  VirtualBlobRequest<T, NewRwRequestPolicy> createBlob(const char *name, History history)
  {
    return create(name, history).blob<T>();
  }

  /// \brief Alias. See \ref read functions for details.
  template <class T>
  VirtualBlobRequest<T, NewRoRequestPolicy> readBlob(const char *name)
  {
    return read(name).blob<T>();
  }

  /// \brief Alias. See \ref historyFor function for details.
  template <class T>
  VirtualBlobRequest<T, NewHistRequestPolicy> readBlobHistory(const char *name)
  {
    return historyFor(name).blob<T>();
  }

  /// \brief Alias. See \ref modify function for details.
  template <class T>
  VirtualBlobRequest<T, NewRwRequestPolicy> modifyBlob(const char *name)
  {
    return modify(name).blob<T>();
  }

  /// \brief Alias. See \ref rename function for details.
  template <class T>
  VirtualBlobRequest<T, NewRwRequestPolicy> renameBlob(const char *from, const char *to, History history)
  {
    return rename(from, to, history).blob<T>();
  }

  ///@}

private:
  detail::ResUid registerTexture2dImpl(const char *name, dabfg::ExternalResourceProvider &&p);

  NodeNameId nodeId;
  InternalRegistry *registry;
};

} // namespace dabfg
