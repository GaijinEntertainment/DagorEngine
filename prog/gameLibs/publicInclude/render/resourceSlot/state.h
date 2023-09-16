#pragma once

#include <render/resourceSlot/detail/registerAccess.h>
#include <render/resourceSlot/resolveAccess.h>

namespace resource_slot
{

/** State of slots storage.
 *
 * Size is 1 int, can be copied to declaration_callback.
 */
struct State
{
  State() = delete;
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

private:
  unsigned storageId : 4;
  int nodeId : 28;

  State(unsigned storage_id, int node_id);
  friend void resource_slot::resolve_access(unsigned);
};

static_assert(sizeof(State) == sizeof(int));

} // namespace resource_slot