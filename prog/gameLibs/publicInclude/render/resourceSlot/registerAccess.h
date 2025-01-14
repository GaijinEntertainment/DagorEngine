//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/resourceSlot/state.h>
#include <render/resourceSlot/actions.h>

#include <render/daBfg/bfg.h>

namespace resource_slot
{

/** Register access to resource slot state
 *
 * Enhanced version of dabfg::register_node()
 *
 * Usage:
 *
 * \code{.cpp}
 *   resource_slot::NodeHandleWithSlotsAccess handle = resource_slot::register_access("node_name", DABFG_PP_NODE_SRC,{
 *     resource_slot::Read{"slot1"},
 *       // node CAN read slot1 from slotsState
 *     resource_slot::Create{"slot2", "tex2"},
 *       // node MUST create tex2, that will be saved into slot2 after the node
 *     resource_slot::Update{"slot3", "tex3_updated_variant", priority=200}
 *       // node CAN read slot3 from slotsState
 *       // node MUST create tex3_updated_variant, that will be saved into slot3 after the node
 *       // nodes with HIGHER priority will be updated AFTER nodes with LOWER priority:
 *       // first priority=100, then priority=200
 *       // dependency cycle in slots and priorities is programming error
 *   }, [](resource_slot::State slotsState, dabfg::Registry registry)
 *   {
 *     registry.readTexture(slotsState.resourceToReadFrom("slot1")).atStage(...).bindToShaderVar(...);
 *       // read final state of slot1 after all updates by other nodes
 *
 *     registry.createTexture2d(slotsState.resourceToCreateFor("slot2"), ...);
 *       // will be saved to slot2 after this node
 *
 *     registry.readTexture(slotsState.resourceToReadFrom("slot3")).atStage(...).bindToShaderVar(...);
 *       // read previous resource in slot3, for example "tex3"
 *     registry.createTexture2d(slotsState.resourceToCreateFor("slot3"), ...);
 *       // will be saved to slot3 after this node
 *
 *     return [] {
 *      shader.render(); // render code here
 *     };
 *   });
 * \endcode
 *
 * \param ns name space where the slot, the node, and all of the resources will be looked up in
 * \param name node name
 * \param source_location SHOULD be DABFG_PP_NODE_SRC
 * \param action_list list of slots, that will be created, updated or read by the node
 * \param declaration_callback node declaration; gets State as first argument
 * \param storage_id      RESERVED for future use
 *
 * \returns handle for access to storage
 */
template <class F>
[[nodiscard]] resource_slot::NodeHandleWithSlotsAccess register_access(dabfg::NameSpace ns, const char *name,
  const char *source_location, resource_slot::detail::ActionList &&action_list, F &&declaration_callback)
{
  return resource_slot::detail::register_access(ns, name, eastl::move(action_list),
    [ns, declCb = eastl::forward<F>(declaration_callback), name, source_location](resource_slot::State s) {
      return ns.registerNode(name, source_location, [&declCb, s](dabfg::Registry r) { return declCb(s, r); });
    });
}

template <class F>
[[nodiscard]] resource_slot::NodeHandleWithSlotsAccess register_access(const char *name, const char *source_location,
  resource_slot::detail::ActionList &&action_list, F &&declaration_callback)
{
  return resource_slot::register_access(dabfg::root(), name, source_location, eastl::move(action_list),
    eastl::forward<F>(declaration_callback));
}

} // namespace resource_slot