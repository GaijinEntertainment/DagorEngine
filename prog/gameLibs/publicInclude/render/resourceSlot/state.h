//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/daBfg/nameSpace.h>

namespace dabfg
{
NameSpace root();
} // namespace dabfg

namespace resource_slot
{

/** State of slots storage.
 *
 * Size is kept small, can be copied to declaration_callback.
 */
struct State
{
  State();

  State(const State &) = default;
  State &operator=(const State &) = default;
  State(State &&) = default;
  State &operator=(State &&) = default;
  ~State() = default;

  /** Get name of resource in slot before the node.
   *
   * This function can be called several times inside the node.
   *
   * \param slot_name name of slot; should be requested in action_list
   * \returns current resource
   */
  const char *resourceToReadFrom(const char *slot_name) const;

  /** Get name of resource from declaration of Create or Update for using after the node.
   *
   * This function can be called several times inside the node.
   * For example, for using in readTextureHistory().
   *
   * \param slot_name name of slot; should be requested in action_list
   * \returns resource after node
   */
  const char *resourceToCreateFor(const char *slot_name) const;

  uint16_t orderInChain() const { return order; }
  uint16_t sizeOfChain() const { return size; }
  bool isNodeLastInChain() const { return order == size - 1; }

private:
  dabfg::NameSpace nameSpace;
  int nodeId;
  uint16_t order;
  uint16_t size;

  State(dabfg::NameSpace ns, int node_id, uint16_t order_in_chain, uint16_t size_of_chain);
  friend void resolve_access();
};

static_assert(sizeof(State) == sizeof(int) * 3);

} // namespace resource_slot