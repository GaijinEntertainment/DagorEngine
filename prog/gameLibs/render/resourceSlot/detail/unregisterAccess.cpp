#include <detail/unregisterAccess.h>


void resource_slot::detail::unregister_access(Storage &storage, NodeId node_id, unsigned generation)
{
  NodeDeclaration &node = storage.registeredNodes[node_id];

  // This node was updated, no needs to remove updated declaration
  if (generation < node.generation)
    return;

  unsigned nextGeneration = node.generation + 1;
  node = NodeDeclaration(nextGeneration);

  storage.isNodeRegisterRequired = true;
}