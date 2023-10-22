//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/span.h>

#include <render/daBfg/detail/nameSpaceNameId.h>
#include <render/daBfg/detail/bfg.h>
#include <render/daBfg/nodeHandle.h>


namespace dabfg
{

/// \cond DETAIL
#define DABFG_PP_STRINGIZE2(x) #x
#define DABFG_PP_STRINGIZE(x)  DABFG_PP_STRINGIZE2(x)
/// \endcond

/**
 * \ingroup DabfgCore
 * \brief A macro that expands to the current source location. Should always
 * be provided to dabfg::register_node as the `source_location` parameter.
 */
#define DABFG_PP_NODE_SRC (__FILE__ ":" DABFG_PP_STRINGIZE(__LINE__))


/**
 * \ingroup DabfgCore
 * \brief A type representing some namespace in the frame graph.
 */
class NameSpace
{
  friend NameSpace root();

  NameSpace();
  NameSpace(NameSpaceNameId nid);

public:
  /**
   * \brief Creates a namespace object for a sub-namespace of this one.
   *
   * \param child_name Name of the sub-namespace.
   * \return An object representing the child namespace.
   */
  NameSpace operator/(const char *child_name) const;

  /**
   * \brief Either registers a new node inside the frame graph,
   * or re-registers an already existing one.
   * \param name
   *   The name that uniquely identifies the node in the current name space.
   *   If the function is called twice with the same node name and name space,
   *   the second call will overwrite the node resulting from the previous one.
   * \param source_location Should always be the DABFG_PP_NODE_SRC macro
   * \param declaration_callback
   *   Should be a callback taking a Registry instance
   *   by value and returning an execution callback, which in turn may
   *   accept a dabfg::multiplexing::Index object (or may accept nothing).
   *   Basically, a function with signature Registry -> (Index -> ()).
   * \return
   *   A handle that represents the lifetime of the new node. The node
   *   will be unregistered when the handle is destroyed.
   *   Note that it is safe to call this function even without
   *   destroying all previous handles up to a couple hundred of times.
   *   The intended use case is doing `classField = register_node(...)`
   *   at arbitrary times, without passing these handles anywhere.
   */
  template <class F>
  [[nodiscard]] NodeHandle registerNode(const char *name, const char *source_location, F &&declaration_callback) const
  {
    // Under high optimization settings, the declCb/execCb calls will get
    // inlined, therefore no perf loss, except for type erasure later
    // down the line.
    auto uid = detail::register_node(nameId, name, source_location,
      [declCb = eastl::forward<F>(declaration_callback)](NodeNameId nodeId, InternalRegistry *r) {
        using ExecCb = eastl::invoke_result_t<F, Registry>;
        if constexpr (eastl::is_invocable_v<ExecCb, multiplexing::Index>)
        {
          return declCb({nodeId, r});
        }
        else if constexpr (eastl::is_void_v<ExecCb>)
        {
          Registry(nodeId, r).executionHas(SideEffects::None);
          declCb({nodeId, r});
          return [](multiplexing::Index) { ; };
        }
        else
        {
          return [execCb = declCb({nodeId, r})](multiplexing::Index) { execCb(); };
        }
      });
    return {uid};
  }

  /**
   * \brief Updates an auto-resolution of a particular type. Note that this
   * causes a complete resource rescheduling, invalidating all history.
   * It also resets dynamic resolution.
   *
   * \param type_name The name of the auto-res type, looked up in this namespace.
   * \param value The new resolution value.
   */
  void setResolution(const char *type_name, IPoint2 value);

  /**
   * \brief Updates an auto-resolution of a particular type without causing
   * a resource rescheduling, hence preserving history.
   * \note This is only available on platforms that support heaps.
   *
   * \param type_name The name of the auto-res type, looked up in this namespace.
   * \param value The new dynamic resolution value. Must be smaller than
   *   the initial resolution for this type.
   */
  void setDynamicResolution(const char *type_name, IPoint2 value);

  /**
   * \brief Sets a value to a named slot. Named slots are basically links
   * that allow for an indirection when looking up a resource name.
   * As an example, volumetric fog is an intrusive feature that requires
   * a bunch of different rendering nodes to read some kind of a downsampled
   * depth resource for tracing the fog, but this depth might be different
   * depending on the current settings. In this situation, it makes sense
   * to introduce a fog_depth slot and fill it in with different resources,
   * allowing one to avoid settings-dependent ifs in node declarations.
   *
   * \param slot Name of the slot, looked up in this namespace.
   * \param res_name_space Name space to look up res_name in.
   * \param res_name Name of the resource to fill this slot with, looked up in \p res_name_space.
   */
  void fillSlot(NamedSlot slot, NameSpace res_name_space, const char *res_name);

  /**
   * \brief Sets the set of resources which are considered to be somehow
   * externally consumed and hence will never be optimized (pruned) out.
   *
   * \param res_names A list of resource names to assign, looked up in this namespace.
   */
  void updateExternallyConsumedResourceSet(eastl::span<const char *const> res_names);

  /// \copydoc updateExternallyConsumedResourceSet(eastl::span<const char *const> res_names)
  void updateExternallyConsumedResourceSet(std::initializer_list<const char *> res_names)
  {
    updateExternallyConsumedResourceSet(eastl::span<const char *const>(res_names.begin(), res_names.size()));
  }

  /**
   * \brief Marks a single resource as being externally consumed.
   * See dabfg::updateExternallyConsumedResourceSet(eastl::span<const char *const> res_names)
   *
   * \param res_name Name of the resource to mark, looked up in this namespace.
   */
  void markResourceExternallyConsumed(const char *res_name);

  /**
   * \brief Unmarks a single resource as being externally consumed.
   * See dabfg::updateExternallyConsumedResourceSet(eastl::span<const char *const> res_names)
   *
   * \param res_name Name of the resource to unmark, looked up in this namespace.
   */
  void unmarkResourceExternallyConsumed(const char *res_name);

private:
  auto resolveName(const char *name) const;

  NameSpaceNameId nameId;
};

} // namespace dabfg
