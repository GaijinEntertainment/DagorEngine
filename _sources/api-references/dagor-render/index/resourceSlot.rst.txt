Resource slots
=================================================

resourceSlot is a declarative api for changing resource between
nodes in daBFG

You can think about slot as "more virtual" frame graph resource.
This type of resource can change size and resource creation flags
while frame graph execution.

Use slots instead of usual resources if:

* Node have to read from and write to some resource at the same time
* Size or type of resource need to be changed in some node

For work with resource slots you need:

* Start from usual ``dabfg::register_node``, see :doc:`daBFG/declaringNodes`
* Replace ``dabfg::NodeHandle`` with ``resource_slot::NodeHandleWithSlotsAccess``
* Replace ``dabfg::register_node`` with ``resource_slot::register_access``

   * Declaration callback will get additional parameter ``resource_slot::State slotsState``

* Add ``action_list`` for the node:

  * If node creates a first resource for slot, node should declare
    ``Create{slot_name, created_resource_name}``

    * Node gets name of resource for filling the slot after node
      with ``slotsState.resourceToCreateFor``

  * If node updates a resource in the slot, node should declare
    ``Update{slot_name, updated_resource_name, priority}``

    * Node gets name of resource for filling the slot after node
      with ``slotsState.resourceToCreateFor``
    * Node gets name of resource, that was in the slot before
      the node with ``slotsState.resourceToReadFrom``
    * Nodes with higher priority will be scheduled after nodes with lower priority

  * If node read a resource from the slot, node should declare ``Read{slot_name}``

    * Node gets name of resource, that was in the slot before
      the node with ``slotsState.resourceToReadFrom``
    * Optionally, node can declare read priority for read intermediate
      resource from the slot. By default read nodes will be scheduled
      after all updating nodes.

For detailed info see :doc:`resourceSlot/registerAccess`

.. toctree::
   :maxdepth: 2
   :caption: Contents:

   resourceSlot/registerAccess
   resourceSlot/nodeHandleWithSlotsAccess
   resourceSlot/resolveAccess
   resourceSlot/resource_slot_ecs
   resourceSlot/resource_slot_das
   resourceSlot/ResourceSlotCore