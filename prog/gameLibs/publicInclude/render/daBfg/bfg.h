//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/span.h>
#include <EASTL/string_view.h>
#include <render/daBfg/nameSpace.h>
#include <render/daBfg/nodeHandle.h>
#include <render/daBfg/registry.h>
#include <render/daBfg/multiplexing.h>
#include <render/daBfg/externalState.h>


namespace dabfg
{


/** \defgroup DabfgCore Core daBfg functionality
 * Core dabfg functions that everything else is accessed through.
 * @{
 */

/**
 * \brief Creates a namespace object for the root frame graph namespace.
 * \return The object representing the root namespace.
 */
NameSpace root();

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

//@}

/** \defgroup DabfgCoreAliases Core daBfg aliases
 * Compatibility aliases for access to the root name space.
 * @{
 */

/**
 * \brief Alias for dabfg::root().registerNode, see \ref NameSpace::registerNode
 */
template <class F>
[[nodiscard]] NodeHandle register_node(const char *name, const char *source_location, F &&declaration_callback)
{
  return root().registerNode(name, source_location, eastl::forward<F>(declaration_callback));
}

/**
 * \brief Alias for dabfg::root().setResolution, see \ref NameSpace::setResolution
 */
inline void set_resolution(const char *typeName, IPoint2 value) { dabfg::root().setResolution(typeName, value); }

/**
 * \brief Alias for dabfg::root().setDynamicResolution,
 * see \ref NameSpace::setDynamicResolution
 */
inline void set_dynamic_resolution(const char *typeName, IPoint2 value) { dabfg::root().setDynamicResolution(typeName, value); }

/**
 * \brief Alias for dabfg::root().fillSlot(slot, dabfg::root(), res_name), see \ref NameSpace::fillSlot
 */
inline void fill_slot(NamedSlot slot, const char *res_name) { dabfg::root().fillSlot(slot, dabfg::root(), res_name); }

/**
 * \brief Alias for dabfg::root().updateExternallyConsumedResourceSet,
 * see \ref NameSpace::updateExternallyConsumedResourceSet(eastl::span<const char *const> res_names)
 */
inline void update_externally_consumed_resource_set(eastl::span<const char *const> res_names)
{
  dabfg::root().updateExternallyConsumedResourceSet(res_names);
}

/// \copydoc update_externally_consumed_resource_set(eastl::span<const char *const> res_names)
inline void update_externally_consumed_resource_set(std::initializer_list<const char *> res_names)
{
  dabfg::root().updateExternallyConsumedResourceSet(res_names);
}

/**
 * \brief Alias for dabfg::root().markResourceExternallyConsumed,
 * see \ref NameSpace::markResourceExternallyConsumed
 */
inline void mark_resource_externally_consumed(const char *res_name) { dabfg::root().markResourceExternallyConsumed(res_name); }

/**
 * \brief Alias for dabfg::root().unmarkResourceExternallyConsumed,
 * see \ref NameSpace::unmarkResourceExternallyConsumed
 */
inline void unmark_resource_externally_consumed(const char *res_name) { dabfg::root().unmarkResourceExternallyConsumed(res_name); }

//@}

} // namespace dabfg
