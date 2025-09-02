//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/daFrameGraph/detail/nameSpaceNameId.h>

#include <render/daFrameGraph/history.h>
#include <render/daFrameGraph/externalResources.h>
#include <render/daFrameGraph/virtualResourceSemiRequest.h>


namespace dafg
{

/**
 * \brief An object representing a name space within the node declaration
 * callback. It can be used to request objects from the current namespace
 * or its sub-namespaces.
 *
 */
class NameSpaceRequest
{
  friend class Registry;

  NameSpaceRequest(NameSpaceNameId ns, NodeNameId node, InternalRegistry *reg) : nameSpaceId{ns}, nodeId{node}, registry{reg} {}

  using RRP = detail::ResourceRequestPolicy;

  static constexpr RRP NewRwRequestPolicy = RRP::HasClearValue;
  static constexpr RRP NewRoRequestPolicy = RRP::Readonly;
  static constexpr RRP NewHistRequestPolicy = RRP::Readonly | RRP::History;


  template <RRP policy>
  using VirtualTextureRequest = VirtualResourceRequest<BaseTexture, policy>;

  template <RRP policy>
  using VirtualBufferRequest = VirtualResourceRequest<Sbuffer, policy>;

  template <class T, RRP policy>
  using VirtualBlobRequest = VirtualResourceRequest<T, policy>;


public:
  /**
   * \brief Creates a namespace object for a sub-namespace of this one.
   *
   * \param child_name Name of the sub-namespace.
   * \return NameSpace Object representing the child namespace.
   */
  NameSpaceRequest operator/(const char *child_name) const;

  /**
   * \brief Get a request object for the 2D resolution of a particular type
   * inside of this namespace, which can then be used to create textures
   * or be resolved into an actual number at execution time.
   * The resulting resolution will be the product of what was set with
   * NameSpace::setResolution with the \p multiplier.
   *
   * \tparam D Dimensionality of the resolution.
   * \param type_name Auto resolution type name.
   * \param multiplier A multiplier for the resolution type.
   * \return AutoResolutionRequest Object representing the auto resolution type.
   */
  template <int D>
  AutoResolutionRequest<D> getResolution(const char *type_name, float multiplier = 1.f) const;

  /** \name Resource requesting methods
   * Every one of these returns a request object that must be used to specify
   * further options for the request. See docs for the relevant request objects
   * for details.
   * \note All methods present here look up the resource in this namespace.
   */
  ///@{

  /**
   * \brief Reads an existing resource at node execution time.
   * Reads always happen after all modifications and before the renaming.
   *
   * \param name The name of the resource to read.
   */
  VirtualResourceSemiRequest<NewRoRequestPolicy> read(const char *name) const;

  /**
   * \brief Reads an existing resource at node execution time, but indirectly,
   * through a "slot", which must be specified with dafg::fill_slot.
   * Reads always happen after all modifications and before the renaming.
   *
   * \param slot_name The name of the slot to be used for this read.
   */
  VirtualResourceSemiRequest<NewRoRequestPolicy> read(NamedSlot slot_name) const;

  /**
   * \brief Reads the history of an existing resource at node execution time.
   *
   * \param name The name of the resource the history of which to read.
   */
  VirtualResourceSemiRequest<NewHistRequestPolicy> historyFor(const char *name) const;

  /**
   * \brief Modifies an existing resource at node execution time.
   * Modifications always happen after creation and before all reads.
   *
   * \param name The name of the resource to modify.
   */
  VirtualResourceSemiRequest<NewRwRequestPolicy> modify(const char *name) const;

  /**
   * \brief Modifies an existing resource at node execution time, but indirectly,
   * through a "slot", which must be specified with dafg::fill_slot.
   * Modifications always happen after creation and before all reads.
   *
   * \param slot_name The name of the slot to be used for this modification.
   */
  VirtualResourceSemiRequest<NewRwRequestPolicy> modify(NamedSlot slot_name) const;

  /**
   * \brief Modifies and renames an existing resource at node execution time.
   * Renaming always happens last among all operations on a resource.
   * The renamed version is considered to be a new virtual resource.
   * \note Parameters \p from and \p to are looked up in different namespaces
   * when calling this function on an arbitrary NameSpaceRequest object!
   * Current node's namespace is used for creating \p to, while \p from is
   * looked up in the current request object's namespace!
   *
   * \param from Old resource name. Note that this resource may not
   *   have history enabled, as renaming it "consumes" it on the
   *   current frame, so it is not possible to read it on the next one.
   *   It will be looked up relative to this namespace.
   * \param to New resource name, created inside the current node's namespace!
   * \param history Whether the new resource needs history enabled,
   *   see \ref create for details about history.
   */
  VirtualResourceSemiRequest<NewRwRequestPolicy> rename(const char *from, const char *to, History history) const;

  ///@}

  /// \name Convenience aliases
  ///@{

  /// \brief Alias. See \ref read functions for details.
  VirtualTextureRequest<NewRoRequestPolicy> readTexture(const char *name) const { return read(name).texture(); }

  /// \brief Alias. See \ref read functions for details.
  VirtualTextureRequest<NewRoRequestPolicy> readTexture(NamedSlot slot_name) const { return read(slot_name).texture(); }

  /// \brief Alias. See \ref historyFor function for details.
  VirtualTextureRequest<NewHistRequestPolicy> readTextureHistory(const char *name) const { return historyFor(name).texture(); }

  /// \brief Alias. See \ref modify function for details.
  VirtualTextureRequest<NewRwRequestPolicy> modifyTexture(const char *name) const { return modify(name).texture(); }

  /// \brief Alias. See \ref modify function for details.
  VirtualTextureRequest<NewRwRequestPolicy> modifyTexture(NamedSlot slot_name) const { return modify(slot_name).texture(); }

  /// \brief Alias. See \ref rename function for details.
  VirtualTextureRequest<NewRwRequestPolicy> renameTexture(const char *from, const char *to, History history) const
  {
    return rename(from, to, history).texture();
  }

  /// \brief Alias. See \ref read functions for details.
  template <class T>
  VirtualBlobRequest<T, NewRoRequestPolicy> readBlob(const char *name) const
  {
    return read(name).blob<T>();
  }

  /// \brief Alias. See \ref historyFor function for details.
  template <class T>
  VirtualBlobRequest<T, NewHistRequestPolicy> readBlobHistory(const char *name) const
  {
    return historyFor(name).blob<T>();
  }

  /// \brief Alias. See \ref modify function for details.
  template <class T>
  VirtualBlobRequest<T, NewRwRequestPolicy> modifyBlob(const char *name) const
  {
    return modify(name).blob<T>();
  }

  /// \brief Alias. See \ref rename function for details.
  template <class T>
  VirtualBlobRequest<T, NewRwRequestPolicy> renameBlob(const char *from, const char *to, History history) const
  {
    return rename(from, to, history).blob<T>();
  }

  ///@}

private:
  NameSpaceNameId nameSpaceId;
  NodeNameId nodeId;
  InternalRegistry *registry;
};

} // namespace dafg