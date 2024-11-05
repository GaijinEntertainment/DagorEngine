//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/daBfg/nameSpace.h>


namespace das
{
class Context;
} // namespace das

namespace resource_slot
{

/** Handle for access to resource slots storage.
 *
 * Also it is proxy for dabfg::NodeHandle.
 *
 * If use in usual code:
 *
 * \code{.cpp}
 *   #include <render/resourceSlot/nodeHandleWithSlotsAccess.h>
 * \endcode
 *
 * If use in ecs:
 *
 * \code{.cpp}
 *   #include <render/resourceSlot/ecs/nodeHandleWithSlotsAccess.h>
 * \endcode
 */
struct NodeHandleWithSlotsAccess
{
  NodeHandleWithSlotsAccess();
  NodeHandleWithSlotsAccess(NodeHandleWithSlotsAccess &&h);
  NodeHandleWithSlotsAccess &operator=(NodeHandleWithSlotsAccess &&h);
  NodeHandleWithSlotsAccess(const NodeHandleWithSlotsAccess &) = delete;
  NodeHandleWithSlotsAccess &operator=(const NodeHandleWithSlotsAccess &) = delete;

  /** Reset handle if set.
   */
  void reset();

  /** Check, if handle is valid
   */
  bool valid() const;

  /** Destructor; calls reset()
   */
  ~NodeHandleWithSlotsAccess();

  /** Private constructor
   *
   * INTERNAL use only
   * \private
   */
  NodeHandleWithSlotsAccess(dabfg::NameSpace ns, int handle_id, unsigned generation_number);

  void _noteContext(const das::Context *context) const;

private:
  dabfg::NameSpace nameSpace;
  int id;
  unsigned generation : 31;
  unsigned isValid : 1;
};

static_assert(sizeof(NodeHandleWithSlotsAccess) == sizeof(unsigned) * 3);

} // namespace resource_slot