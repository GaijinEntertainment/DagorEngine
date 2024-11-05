//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/resourceSlot/detail/registerAccess.h>
#include <render/resourceSlot/detail/ids.h>

namespace resource_slot
{

/** Create request
 *
 * Declared resource MUST be created in the node
 */
struct Create
{
  /** Constructor
   *
   * \param slot_name name of the slot
   * \param resource_name
   *   name of the resource,
   *   that MUST be created in current node and WILL be saved into slot
   *   for using after the node
   */
  Create(const char *slot_name, const char *resource_name);

  ::resource_slot::detail::SlotId slot;
  ::resource_slot::detail::ResourceId resource;
};

/** Update request
 *
 * Requested slot CAN be read in the node and declared resource MUST be created in the node
 */
struct Update
{
  /** Constructor
   *
   * \param slot_name name of the slot
   * \param resource_name
   *   name of the resource,
   *   that MUST be created in current node and WILL be saved into slot
   *   for using after the node
   * \param update_priority priority of update, nodes with HIGHER
   *   priority will be scheduled AFTER nodes with LOWER priority
   */
  Update(const char *slot_name, const char *resource_name, int update_priority);

  ::resource_slot::detail::SlotId slot;
  ::resource_slot::detail::ResourceId resource;
  int priority;
};

/** Read request
 *
 * Requested slot CAN be read in the node
 */
struct Read
{
  /** Constructor
   *
   * \param slot_name name of the slot
   * \param read_priority
   *   priority of read; nodes with LOWER priority
   *   will be scheduled BEFORE current node.
   *   By default Read will be scheduled after all Update of this slot.
   */
  Read(const char *slot_name, int read_priority = DEFAULT_READ_PRIORITY);

  ::resource_slot::detail::SlotId slot;
  int priority;
  static constexpr int DEFAULT_READ_PRIORITY = INT_MAX;
};

} // namespace resource_slot