//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/span.h>
#include <EASTL/string_view.h>
#include <render/daBfg/nodeHandle.h>
#include <render/daBfg/registry.h>
#include <render/daBfg/multiplexing.h>
#include <render/daBfg/externalState.h>


namespace dabfg
{

/// \cond DETAIL
#define DABFG_PP_STRINGIZE2(x) #x
#define DABFG_PP_STRINGIZE(x)  DABFG_PP_STRINGIZE2(x)
/// \endcond

/**
 * \brief A macro that expands to the current source location. Should always
 * be provided to dabfg::register_node as the `source_location` parameter.
 */
#define DABFG_PP_NODE_SRC (__FILE__ ":" DABFG_PP_STRINGIZE(__LINE__))

/**
 * \brief Either registers a new node inside the frame graph,
 * or re-registers an already existing one.
 * \param name
 *   The name that uniquely identifies the node. If the function is called
 *   twice with the same node name, the second call will overwrite the
 *   node resulting from the previous one.
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
[[nodiscard]] NodeHandle register_node(const char *name, const char *source_location, F &&declaration_callback)
{
  // Under high optimization settings, the declCb/execCb calls will get
  // inlined, therefore no perf loss, except for type erasure later
  // down the line.
  auto uid = detail::register_node(name, source_location,
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
 * \param typeName The name of the auto-res type.
 * \param value The new resolution value.
 */
void set_resolution(const char *typeName, IPoint2 value);

/**
 * \brief Updates an auto-resolution of a particular type without causing
 * a resource rescheduling, hence preserving history.
 * Note that this is only available on platforms that support heaps.
 *
 * \param typeName The name of the auto-res type.
 * \param value The new dynamic resolution value. Must be smaller than
 *   the initial resolution for this type.
 */
void set_dynamic_resolution(const char *typeName, IPoint2 value);

/**
 * \brief Returns the current dynamic resolution for a particular auto-res type.
 * \warning Should only be used for setting the d3d viewport/scissor, NEVER create
 * textures with this resolution.
 *
 * \param typeName The name of the auto-res type.
 * \return The current dynamic resolution for this type.
 */
IPoint2 get_resolution(const char *typeName);

/**
 * \brief Marks an external resource to be validated for illegal access
 * within nodes. Note that daBfg-managed resources are always automatically
 * validated for illegal access through sneaky global variables, but external
 * are often used for gradual migration to FG, at which point they are
 * indeed illegally accessed through global state and that is intended.
 *
 * \param resource The name of the resource to be validated.
 */
void mark_external_resource_for_validation(const D3dResource *resource);

/**
 * \brief Sets a value to a named slot. Named slots are a global state
 * that represents an indirection in the resource that a node will consume.
 * As an example, in a post-fx node, you might want to read the input
 * from a named slot instead of a concrete resource so that other nodes
 * can be inserted in between scene rendering and post-fx without calling
 * into the post-fx code to change it's input.
 *
 * \param slot Name of the slot
 * \param res_name Name of the resource to fill this slot with
 */
void fill_slot(NamedSlot slot, const char *res_name);

/**
 * \brief Sets the set of resources which are considered to be somehow
 * externally consumed and hence will never be optimized (pruned) out.
 *
 * \param res_names A list of resource names to assign.
 */
void update_externally_consumed_resource_set(eastl::span<const char *const> res_names);

/// \copydoc update_externally_consumed_resource_set(eastl::span<const char *const> res_names)
void update_externally_consumed_resource_set(std::initializer_list<const char *> res_names);

/**
 * \brief Marks a single resource as being externally consumed.
 * See dabfg::update_externally_consumed_resource_set(eastl::span<const char *const> res_names)
 *
 * \param res_name Name of the resource to mark.
 */
void mark_resource_externally_consumed(const char *res_name);

/**
 * \brief Unmarks a single resource as being externally consumed.
 * See dabfg::update_externally_consumed_resource_set(eastl::span<const char *const> res_names)
 *
 * \param res_name Name of the resource to unmark.
 */
void unmark_resource_externally_consumed(const char *res_name);

/**
 * \brief Sets the multiplexing extents for the following frames.
 * \details May safely be called every frame, only triggers a
 * recompilation when the values actually change
 */
void set_multiplexing_extents(multiplexing::Extents extents);

/// \brief Executes the frame graph, possibly recompiling it.
void run_nodes();

/// \brief Initializes the daBfg backend.
void startup();

/// \brief Frees resources held by the daBfg backend.
void shutdown();

/// \brief Sets various global state that is external to daBfg.
void update_external_state(ExternalState state);


} // namespace dabfg
