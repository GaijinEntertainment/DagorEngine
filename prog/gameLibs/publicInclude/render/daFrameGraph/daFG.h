//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/span.h>
#include <EASTL/string_view.h>
#include <render/daFrameGraph/nameSpace.h>
#include <render/daFrameGraph/nodeHandle.h>
#include <render/daFrameGraph/registry.h>
#include <render/daFrameGraph/multiplexing.h>
#include <render/daFrameGraph/externalState.h>


namespace dafg
{


/** \defgroup DaFgCore Core daFG functionality
 * Core daFG functions that everything else is accessed through.
 * @{
 */

/**
 * \brief Creates a namespace object for the root frame graph namespace.
 * \return The object representing the root namespace.
 */
NameSpace root();

/**
 * \brief Sets the default multiplexing mode for nodes.
 * \details May safely be called every frame, only triggers a
 * recompilation when the values actually change
 * \param mode The default multiplexing mode for nodes.
 * \param history_mode The multiplexing mode that is applied to resources that are generated on a previous frame. If means that some of
 * the multiplexing indices for these resources will live only on previous frame and this frame we will access clamped multiplexing
 * index.
 */
void set_multiplexing_default_mode(multiplexing::Mode mode, multiplexing::Mode history_mode);

/**
 * \brief Sets the multiplexing extents for the following frames.
 * \details May safely be called every frame, only triggers a
 * recompilation when the values actually change
 */
void set_multiplexing_extents(multiplexing::Extents extents);

/// \brief Executes the frame graph, possibly recompiling it.
/// \details Return false if run is not possible (d3d device was lost for example)
bool run_nodes();

/// \brief Initializes the daFG backend.
void startup();

/// \brief Frees resources held by the daFG backend.
void shutdown();

/**
 * \brief Invalidates history
 * \details When making changes to the graph history is generally preserved
 * calling this function makes this preservation invalid and the history
 * resources will be recreated.
 */
void invalidate_history();

/// \brief Sets various global state that is external to daFG.
void update_external_state(ExternalState state);

/**
 * \brief Marks an external resource to be validated for illegal access
 * within nodes. Note that daFG-managed resources are always automatically
 * validated for illegal access through sneaky global variables, but external
 * are often used for gradual migration to FG, at which point they are
 * indeed illegally accessed through global state and that is intended.
 *
 * \param resource The name of the resource to be validated.
 */
void mark_external_resource_for_validation(const D3dResource *resource);

//@}

/** \defgroup DaFgCoreAliases Core daFG aliases
 * Compatibility aliases for access to the root name space.
 * @{
 */

/**
 * \brief Alias for dafg::root().registerNode, see \ref NameSpace::registerNode
 */
template <class F>
[[nodiscard]] NodeHandle register_node(const char *name, const char *source_location, F &&declaration_callback)
{
  return root().registerNode(name, source_location, eastl::forward<F>(declaration_callback));
}

/**
 * \brief Alias for dafg::root().setResolution, see \ref NameSpace::setResolution
 */
template <class T>
inline void set_resolution(const char *typeName, T value)
{
  dafg::root().setResolution(typeName, value);
}

/**
 * \brief Alias for dafg::root().setDynamicResolution,
 * see \ref NameSpace::setDynamicResolution
 */
template <class T>
inline void set_dynamic_resolution(const char *typeName, T value)
{
  dafg::root().setDynamicResolution(typeName, value);
}

/**
 * \brief Alias for dafg::root().fillSlot(slot, dafg::root(), res_name), see \ref NameSpace::fillSlot
 */
inline void fill_slot(NamedSlot slot, const char *res_name) { dafg::root().fillSlot(slot, dafg::root(), res_name); }

/**
 * \brief Alias for dafg::root().updateExternallyConsumedResourceSet,
 * see \ref NameSpace::updateExternallyConsumedResourceSet(eastl::span<const char *const> res_names)
 */
inline void update_externally_consumed_resource_set(eastl::span<const char *const> res_names)
{
  dafg::root().updateExternallyConsumedResourceSet(res_names);
}

/// \copydoc update_externally_consumed_resource_set(eastl::span<const char *const> res_names)
inline void update_externally_consumed_resource_set(std::initializer_list<const char *> res_names)
{
  dafg::root().updateExternallyConsumedResourceSet(res_names);
}

/**
 * \brief Alias for dafg::root().markResourceExternallyConsumed,
 * see \ref NameSpace::markResourceExternallyConsumed
 */
inline void mark_resource_externally_consumed(const char *res_name) { dafg::root().markResourceExternallyConsumed(res_name); }

/**
 * \brief Alias for dafg::root().unmarkResourceExternallyConsumed,
 * see \ref NameSpace::unmarkResourceExternallyConsumed
 */
inline void unmark_resource_externally_consumed(const char *res_name) { dafg::root().unmarkResourceExternallyConsumed(res_name); }

//@}

} // namespace dafg
